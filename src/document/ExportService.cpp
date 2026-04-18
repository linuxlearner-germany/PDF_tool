#include "document/ExportService.h"

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QTemporaryFile>

#include <QtPrintSupport/QPrinter>
#include <QtGlobal>

#include "document/AtomicFileTransaction.h"
#include "operations/QPdfOperations.h"
#include "rendering/PdfRenderEngine.h"
#include "services/OcrService.h"

namespace
{
constexpr double kRedactionExportScale = 2.0;

QString normalizedPdfFontFamily(const QString &fontFamily)
{
    const QString family = fontFamily.trimmed();
    if (family.contains(QStringLiteral("courier"), Qt::CaseInsensitive)) {
        return QStringLiteral("Courier");
    }
    if (family.contains(QStringLiteral("times"), Qt::CaseInsensitive)) {
        return QStringLiteral("Times New Roman");
    }
    return QStringLiteral("Helvetica");
}

bool isNativePdfAnnotationSupported(const PdfAnnotation &annotation)
{
    return annotation.kind == PdfAnnotationKind::Highlight
        || annotation.kind == PdfAnnotationKind::Rectangle
        || annotation.kind == PdfAnnotationKind::Note
        || annotation.kind == PdfAnnotationKind::FreeText
        || annotation.kind == PdfAnnotationKind::Signature;
}

bool formFieldsEqual(const QVector<PdfFormField> &lhs, const QVector<PdfFormField> &rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (int index = 0; index < lhs.size(); ++index) {
        const PdfFormField &left = lhs.at(index);
        const PdfFormField &right = rhs.at(index);
        if (left.id != right.id
            || left.kind != right.kind
            || left.textValue != right.textValue
            || left.textStyle.textColor != right.textStyle.textColor
            || left.textStyle.fontFamily != right.textStyle.fontFamily
            || !qFuzzyCompare(left.textStyle.fontSize + 1.0, right.textStyle.fontSize + 1.0)
            || left.checked != right.checked) {
            return false;
        }
    }

    return true;
}

QVector<PdfAnnotation> filteredAnnotations(
    const QVector<PdfAnnotation> &annotations,
    const QString &excludedAnnotationId)
{
    if (excludedAnnotationId.isEmpty()) {
        return annotations;
    }

    QVector<PdfAnnotation> result;
    result.reserve(annotations.size());
    for (const PdfAnnotation &annotation : annotations) {
        if (annotation.id != excludedAnnotationId) {
            result.append(annotation);
        }
    }
    return result;
}

void drawSearchableOcrTextLayer(QPainter &painter, const QImage &pageImage, const QSizeF &targetPageSize)
{
    if (pageImage.isNull() || targetPageSize.isEmpty() || !OcrService::isAvailable()) {
        return;
    }

    QString ocrError;
    const QVector<OcrWordBox> wordBoxes = OcrService::recognizeImageWords(pageImage, &ocrError);
    if (wordBoxes.isEmpty()) {
        return;
    }

    const double scaleX = targetPageSize.width() / static_cast<double>(pageImage.width());
    const double scaleY = targetPageSize.height() / static_cast<double>(pageImage.height());

    painter.save();
    painter.setPen(QColor(0, 0, 0, 1));
    for (const OcrWordBox &wordBox : wordBoxes) {
        const QRectF targetRect(wordBox.imageRect.left() * scaleX,
                                wordBox.imageRect.top() * scaleY,
                                wordBox.imageRect.width() * scaleX,
                                wordBox.imageRect.height() * scaleY);
        if (targetRect.isEmpty()) {
            continue;
        }

        QFont font = painter.font();
        font.setPixelSize(qMax(1, static_cast<int>(targetRect.height() * 0.8)));
        painter.setFont(font);
        painter.drawText(targetRect, Qt::AlignLeft | Qt::AlignVCenter, wordBox.text);
    }
    painter.restore();
}
}

CapabilityState ExportService::capabilityState() const
{
    QPdfOperations operations;
    return operations.capabilityState();
}

EditedPdfExportDecision ExportService::decideEditedPdfExport(
    const QVector<PdfAnnotation> &annotations,
    const QVector<PdfRedaction> &redactions,
    bool formStateChanged) const
{
    EditedPdfExportDecision decision;
    const CapabilityState capability = capabilityState();
    const bool hasUnsupportedAnnotationKinds = std::any_of(
        annotations.begin(),
        annotations.end(),
        [](const PdfAnnotation &annotation) {
            return !isNativePdfAnnotationSupported(annotation);
        });

    if (!capability.available) {
        decision.mode = EditedPdfExportMode::RasterizedPdf;
        decision.reason = capability.error;
        return decision;
    }

    if (hasUnsupportedAnnotationKinds || !redactions.isEmpty()) {
        decision.mode = EditedPdfExportMode::RasterizedPdf;
        decision.reason = QStringLiteral("Native Export-Regeln erfordern Rasterisierung.");
        return decision;
    }

    if (!annotations.isEmpty() || formStateChanged) {
        decision.mode = EditedPdfExportMode::NativePdf;
        return decision;
    }

    decision.mode = EditedPdfExportMode::RasterizedPdf;
    decision.reason = QStringLiteral("Keine nativen Aenderungen vorhanden.");
    return decision;
}

bool ExportService::printDocument(PdfRenderEngine &renderEngine, int pageCount, QPrinter *printer, QString &errorMessage) const
{
    errorMessage.clear();
    if (!printer) {
        return false;
    }

    QPainter painter;
    if (!painter.begin(printer)) {
        errorMessage = QStringLiteral("Drucker konnte nicht initialisiert werden.");
        return false;
    }

    const QRect paintRect = printer->pageLayout().paintRectPixels(printer->resolution());
    const double printScale = static_cast<double>(printer->resolution()) / 72.0;

    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        const QImage pageImage = renderEngine.renderPage(pageIndex, printScale);
        if (pageImage.isNull()) {
            painter.end();
            errorMessage = renderEngine.lastError();
            return false;
        }

        const QSize scaledSize = pageImage.size().scaled(paintRect.size(), Qt::KeepAspectRatio);
        const QRect targetRect(
            paintRect.x() + (paintRect.width() - scaledSize.width()) / 2,
            paintRect.y() + (paintRect.height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height());
        painter.drawImage(targetRect, pageImage);

        if (pageIndex < pageCount - 1 && !printer->newPage()) {
            painter.end();
            errorMessage = QStringLiteral("Neue Druckseite konnte nicht erstellt werden.");
            return false;
        }
    }

    painter.end();
    return true;
}

bool ExportService::exportEditedPdf(
    PdfRenderEngine &renderEngine,
    const QString &documentPath,
    const QString &outputFile,
    const QVector<PdfAnnotation> &annotations,
    const QVector<PdfFormField> &formFields,
    const QVector<PdfFormField> &baseFormFields,
    const QVector<PdfRedaction> &redactions,
    const QByteArray &ownerPassword,
    const QByteArray &userPassword,
    const QString &excludedAnnotationId,
    QString &errorMessage,
    QString &statusMessage) const
{
    errorMessage.clear();
    statusMessage.clear();
    if (outputFile.isEmpty()) {
        return false;
    }

    const QVector<PdfAnnotation> effectiveAnnotations = filteredAnnotations(annotations, excludedAnnotationId);
    const bool formStateChanged = !formFieldsEqual(formFields, baseFormFields);
    const EditedPdfExportDecision decision = decideEditedPdfExport(effectiveAnnotations, redactions, formStateChanged);

    if (decision.mode == EditedPdfExportMode::NativePdf) {
        QVector<QSizeF> pageSizes;
        pageSizes.reserve(renderEngine.pageCount());
        for (int pageIndex = 0; pageIndex < renderEngine.pageCount(); ++pageIndex) {
            pageSizes.append(renderEngine.pageSizePoints(pageIndex));
        }

        QPdfOperations operations;
        const QByteArray password = ownerPassword.isEmpty() ? userPassword : ownerPassword;
        if (operations.saveEditedState(documentPath, outputFile, effectiveAnnotations, formFields, redactions, pageSizes, password)) {
            statusMessage = QStringLiteral("Bearbeitetes PDF mit nativen PDF-Aenderungen exportiert.");
            return true;
        }

        statusMessage = QStringLiteral("Native PDF-Aenderungen nicht verfuegbar, Export faellt auf gerastertes PDF zurueck.");
    }

    QPdfWriter writer(outputFile);
    writer.setCreator(QStringLiteral("PDFTool"));
    writer.setResolution(144);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        errorMessage = QStringLiteral("Ausgabe-PDF konnte nicht erstellt werden.");
        return false;
    }

    for (int pageIndex = 0; pageIndex < renderEngine.pageCount(); ++pageIndex) {
        const QSizeF pageSizePoints = renderEngine.pageSizePoints(pageIndex);
        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            writer.setPageSize(QPageSize(pageSizePoints, QPageSize::Point, QString(), QPageSize::ExactMatch));
        }
        if (pageIndex > 0) {
            writer.newPage();
        }

        QImage pageImage = renderEngine.renderPage(pageIndex, kRedactionExportScale);
        if (pageImage.isNull()) {
            painter.end();
            errorMessage = renderEngine.lastError();
            return false;
        }

        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            const double scaleX = static_cast<double>(pageImage.width()) / pageSizePoints.width();
            const double scaleY = static_cast<double>(pageImage.height()) / pageSizePoints.height();

            QPainter imagePainter(&pageImage);
            imagePainter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

            const QVector<PdfAnnotation> pageAnnotations = filteredAnnotations(
                [&]() {
                    QVector<PdfAnnotation> result;
                    for (const PdfAnnotation &annotation : effectiveAnnotations) {
                        if (annotation.pageIndex == pageIndex) {
                            result.append(annotation);
                        }
                    }
                    return result;
                }(),
                excludedAnnotationId);

            for (const PdfAnnotation &annotation : pageAnnotations) {
                switch (annotation.kind) {
                case PdfAnnotationKind::Highlight:
                    imagePainter.setPen(Qt::NoPen);
                    for (const QRectF &pageRect : annotation.pageRects) {
                        imagePainter.fillRect(QRectF(pageRect.left() * scaleX, pageRect.top() * scaleY, pageRect.width() * scaleX, pageRect.height() * scaleY), annotation.color);
                    }
                    break;
                case PdfAnnotationKind::Rectangle:
                    imagePainter.setPen(QPen(annotation.color, 2.0));
                    imagePainter.setBrush(Qt::NoBrush);
                    for (const QRectF &pageRect : annotation.pageRects) {
                        imagePainter.drawRect(QRectF(pageRect.left() * scaleX, pageRect.top() * scaleY, pageRect.width() * scaleX, pageRect.height() * scaleY));
                    }
                    break;
                case PdfAnnotationKind::Note:
                    imagePainter.setPen(QPen(QColor(121, 85, 72), 1.0));
                    imagePainter.setBrush(annotation.color);
                    for (const QRectF &pageRect : annotation.pageRects) {
                        const QRectF imageRect(pageRect.left() * scaleX, pageRect.top() * scaleY, pageRect.width() * scaleX, pageRect.height() * scaleY);
                        imagePainter.drawRect(imageRect);
                        imagePainter.setPen(Qt::black);
                        imagePainter.drawText(imageRect.adjusted(4.0, 2.0, -2.0, -2.0), Qt::AlignLeft | Qt::AlignTop, QStringLiteral("N"));
                        imagePainter.setPen(QPen(QColor(121, 85, 72), 1.0));
                    }
                    break;
                case PdfAnnotationKind::FreeText:
                    for (const QRectF &pageRect : annotation.pageRects) {
                        const QRectF imageRect(pageRect.left() * scaleX, pageRect.top() * scaleY, pageRect.width() * scaleX, pageRect.height() * scaleY);
                        imagePainter.fillRect(imageRect, annotation.color);
                        imagePainter.setPen(QPen(QColor(210, 210, 210), 1.0));
                        imagePainter.drawRect(imageRect);
                        QFont font = imagePainter.font();
                        font.setFamily(normalizedPdfFontFamily(annotation.textStyle.fontFamily));
                        font.setPixelSize(qRound(std::max(1.0, annotation.textStyle.fontSize * scaleY)));
                        imagePainter.setFont(font);
                        imagePainter.setPen(annotation.textStyle.textColor.isValid() ? annotation.textStyle.textColor : Qt::black);
                        imagePainter.drawText(imageRect.adjusted(6.0, 4.0, -6.0, -4.0), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, annotation.text);
                    }
                    break;
                case PdfAnnotationKind::Signature:
                    for (const QRectF &pageRect : annotation.pageRects) {
                        QImage signatureImage;
                        signatureImage.loadFromData(annotation.binaryPayload);
                        if (!signatureImage.isNull()) {
                            imagePainter.drawImage(QRectF(pageRect.left() * scaleX, pageRect.top() * scaleY, pageRect.width() * scaleX, pageRect.height() * scaleY), signatureImage);
                        }
                    }
                    break;
                }
            }

            for (const PdfFormField &field : formFields) {
                if (field.pageIndex != pageIndex) {
                    continue;
                }
                const QRectF imageRect(field.pageRect.left() * scaleX, field.pageRect.top() * scaleY, field.pageRect.width() * scaleX, field.pageRect.height() * scaleY);
                imagePainter.setPen(QPen(QColor(25, 118, 210), 1.0));
                imagePainter.setBrush(QColor(255, 255, 255, 230));
                imagePainter.drawRect(imageRect);
                if (field.kind == PdfFormFieldKind::Text) {
                    QFont font = imagePainter.font();
                    font.setFamily(field.textStyle.fontFamily);
                    font.setPixelSize(qMax(1, qRound(field.textStyle.fontSize * scaleY)));
                    imagePainter.setFont(font);
                    imagePainter.setPen(field.textStyle.textColor.isValid() ? field.textStyle.textColor : Qt::black);
                    imagePainter.drawText(imageRect.adjusted(4.0, 2.0, -4.0, -2.0), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, field.textValue);
                } else if (field.kind == PdfFormFieldKind::CheckBox) {
                    const QRectF boxRect = imageRect.adjusted(3.0, 3.0, -3.0, -3.0);
                    imagePainter.setBrush(Qt::white);
                    imagePainter.setPen(QPen(Qt::black, 1.0));
                    imagePainter.drawRect(boxRect);
                    if (field.checked) {
                        imagePainter.setPen(QPen(Qt::black, 2.0));
                        imagePainter.drawLine(boxRect.topLeft() + QPointF(3.0, boxRect.height() * 0.55), boxRect.center() + QPointF(-1.0, boxRect.height() * 0.2));
                        imagePainter.drawLine(boxRect.center() + QPointF(-1.0, boxRect.height() * 0.2), boxRect.bottomRight() + QPointF(-3.0, -3.0));
                    }
                }
            }

            imagePainter.setPen(Qt::NoPen);
            imagePainter.setBrush(Qt::black);
            for (const PdfRedaction &redaction : redactions) {
                if (redaction.pageIndex != pageIndex) {
                    continue;
                }
                imagePainter.drawRect(QRectF(redaction.pageRect.left() * scaleX, redaction.pageRect.top() * scaleY, redaction.pageRect.width() * scaleX, redaction.pageRect.height() * scaleY));
            }
        }

        const QRectF targetRect(0.0, 0.0, writer.width(), writer.height());
        painter.drawImage(targetRect, pageImage);
        drawSearchableOcrTextLayer(painter, pageImage, targetRect.size());
    }

    painter.end();
    if (statusMessage.isEmpty()) {
        statusMessage = QStringLiteral("Bearbeitetes PDF exportiert.");
    }
    return true;
}

bool ExportService::exportRedactedPdf(
    PdfRenderEngine &renderEngine,
    const QString &documentPath,
    const QString &outputFile,
    const QVector<PdfRedaction> &redactions,
    int pageCount,
    const QByteArray &ownerPassword,
    const QByteArray &userPassword,
    QString &errorMessage,
    QString &statusMessage) const
{
    errorMessage.clear();
    statusMessage.clear();
    if (outputFile.isEmpty() || redactions.isEmpty()) {
        return false;
    }

    QVector<QSizeF> pageSizes;
    QVector<QImage> flattenedPages;
    pageSizes.reserve(pageCount);
    flattenedPages.resize(pageCount);
    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        pageSizes.append(renderEngine.pageSizePoints(pageIndex));
        bool hasPageRedactions = false;
        for (const PdfRedaction &redaction : redactions) {
            if (redaction.pageIndex == pageIndex) {
                hasPageRedactions = true;
                break;
            }
        }
        if (!hasPageRedactions) {
            continue;
        }

        QImage pageImage = renderEngine.renderPage(pageIndex, kRedactionExportScale);
        if (pageImage.isNull()) {
            errorMessage = renderEngine.lastError();
            return false;
        }

        const QSizeF pageSizePoints = renderEngine.pageSizePoints(pageIndex);
        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            QPainter imagePainter(&pageImage);
            const double scaleX = static_cast<double>(pageImage.width()) / pageSizePoints.width();
            const double scaleY = static_cast<double>(pageImage.height()) / pageSizePoints.height();
            for (const PdfRedaction &redaction : redactions) {
                if (redaction.pageIndex != pageIndex) {
                    continue;
                }
                imagePainter.fillRect(QRectF(redaction.pageRect.left() * scaleX, redaction.pageRect.top() * scaleY, redaction.pageRect.width() * scaleX, redaction.pageRect.height() * scaleY), Qt::black);
            }
        }
        flattenedPages[pageIndex] = pageImage;
    }

    QPdfOperations operations;
    const QByteArray password = ownerPassword.isEmpty() ? userPassword : ownerPassword;
    if (operations.saveDestructiveRedactedState(documentPath, outputFile, flattenedPages, pageSizes, password)) {
        statusMessage = QStringLiteral("Geschwaerztes PDF mit destruktiver Redaction exportiert.");
        return true;
    }

    QPdfWriter writer(outputFile);
    writer.setCreator(QStringLiteral("PDFTool"));
    writer.setResolution(144);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        errorMessage = QStringLiteral("Ausgabe-PDF konnte nicht erstellt werden.");
        return false;
    }

    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        const QSizeF pageSizePoints = renderEngine.pageSizePoints(pageIndex);
        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            writer.setPageSize(QPageSize(pageSizePoints, QPageSize::Point, QString(), QPageSize::ExactMatch));
        }
        if (pageIndex > 0) {
            writer.newPage();
        }

        QImage pageImage = renderEngine.renderPage(pageIndex, kRedactionExportScale);
        if (pageImage.isNull()) {
            painter.end();
            errorMessage = renderEngine.lastError();
            return false;
        }

        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            QPainter imagePainter(&pageImage);
            const double scaleX = static_cast<double>(pageImage.width()) / pageSizePoints.width();
            const double scaleY = static_cast<double>(pageImage.height()) / pageSizePoints.height();
            for (const PdfRedaction &redaction : redactions) {
                if (redaction.pageIndex != pageIndex) {
                    continue;
                }
                imagePainter.fillRect(QRectF(redaction.pageRect.left() * scaleX, redaction.pageRect.top() * scaleY, redaction.pageRect.width() * scaleX, redaction.pageRect.height() * scaleY), Qt::black);
            }
        }

        const QRectF targetRect(0.0, 0.0, writer.width(), writer.height());
        painter.drawImage(targetRect, pageImage);
        drawSearchableOcrTextLayer(painter, pageImage, targetRect.size());
    }

    painter.end();
    statusMessage = QStringLiteral("Geschwaerztes PDF exportiert.");
    return true;
}

bool ExportService::saveDocumentState(
    const QString &documentPath,
    const QString &sidecarPath,
    const std::function<bool(const QString &, QString &, QString &)> &exportToPath,
    QString &errorMessage,
    QString &statusMessage) const
{
    errorMessage.clear();
    statusMessage.clear();

    QTemporaryFile tempFile(QFileInfo(documentPath).absolutePath() + QLatin1String("/.pdftool-save-XXXXXX.pdf"));
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        errorMessage = QStringLiteral("Temporaere Zieldatei konnte nicht angelegt werden.");
        return false;
    }

    const QString tempPath = tempFile.fileName();
    tempFile.close();

    QString exportStatus;
    if (!exportToPath(tempPath, errorMessage, exportStatus)) {
        QFile::remove(tempPath);
        return false;
    }

    if (!AtomicFileTransaction::replaceFile(tempPath, documentPath, QStringLiteral(".bak"), &errorMessage)) {
        QFile::remove(tempPath);
        return false;
    }

    QString removeError;
    AtomicFileTransaction::removeFileIfExists(sidecarPath, &removeError);
    statusMessage = QStringLiteral("Aenderungen direkt im PDF gespeichert.");
    return true;
}
