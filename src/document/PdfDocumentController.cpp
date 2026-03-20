#include "document/PdfDocumentController.h"

#include <algorithm>

#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QDir>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFont>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QTemporaryFile>

#include <QtPrintSupport/QPrinter>

#include "document/PageThumbnailProvider.h"
#include "operations/QPdfOperations.h"
#include "rendering/PdfRenderEngine.h"
#include "services/OcrService.h"

namespace
{
constexpr double kMinZoom = 0.5;
constexpr double kMaxZoom = 4.0;
constexpr double kZoomStep = 0.25;
constexpr double kRedactionExportScale = 2.0;
constexpr int kSidecarVersion = 3;

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
}

PdfDocumentController::PdfDocumentController(std::unique_ptr<PdfRenderEngine> renderEngine, QObject *parent)
    : QObject(parent)
    , m_renderEngine(std::move(renderEngine))
    , m_thumbnailProvider(std::make_unique<PageThumbnailProvider>(m_renderEngine.get()))
{
}

PdfDocumentController::~PdfDocumentController() = default;

bool PdfDocumentController::openDocument(const QString &filePath, const QByteArray &ownerPassword, const QByteArray &userPassword)
{
    emit busyStateChanged(true, QStringLiteral("PDF wird geladen..."));

    if (!m_renderEngine->loadDocument(filePath, ownerPassword, userPassword)) {
        m_lastError = m_renderEngine->lastError();
        emit busyStateChanged(false, QString());
        emit errorOccurred(m_lastError);
        return false;
    }

    resetDocumentState();
    m_documentPath = filePath;
    m_metadata = m_renderEngine->documentMetadata();
    m_metadata.filePath = filePath;
    m_pageLabels = m_renderEngine->pageLabels();
    m_outlineEntries = m_renderEngine->outlineEntries();
    m_currentPageIndex = 0;
    m_zoomFactor = 1.0;
    m_currentOwnerPassword = ownerPassword;
    m_currentUserPassword = userPassword;

    QVector<PdfFormField> fields;
    QVector<PdfAnnotation> annotations;
    for (int pageIndex = 0; pageIndex < m_renderEngine->pageCount(); ++pageIndex) {
        annotations += m_renderEngine->annotations(pageIndex);
        const QVector<PdfFormField> pageFields = m_renderEngine->formFields(pageIndex);
        fields += pageFields;
    }
    m_annotationModel.setAnnotations(annotations);
    m_baseFormFields = fields;
    m_formFieldModel.setFields(fields);
    loadSidecarState();
    resetHistory();
    recordHistorySnapshot();

    emit documentStateChanged(true);
    emit zoomChanged(m_zoomFactor);
    emit statusMessageChanged(QStringLiteral("PDF geladen: %1").arg(filePath));
    renderCurrentPage();
    emit busyStateChanged(false, QString());
    return true;
}

void PdfDocumentController::closeDocument()
{
    m_renderEngine->closeDocument();
    resetDocumentState();

    emit pageImageChanged({});
    emit zoomChanged(m_zoomFactor);
    emit currentPageChanged(0, 0, QString());
    emit documentStateChanged(false);
    emit statusMessageChanged(QStringLiteral("Dokument geschlossen."));
}

bool PdfDocumentController::hasDocument() const
{
    return m_renderEngine->isLoaded();
}

QString PdfDocumentController::documentPath() const
{
    return m_documentPath;
}

int PdfDocumentController::pageCount() const
{
    return m_renderEngine->pageCount();
}

int PdfDocumentController::currentPageIndex() const
{
    return m_currentPageIndex;
}

QString PdfDocumentController::currentPageLabel() const
{
    if (m_currentPageIndex < 0 || m_currentPageIndex >= m_pageLabels.size()) {
        return {};
    }

    return m_pageLabels.at(m_currentPageIndex);
}

QStringList PdfDocumentController::pageLabels() const
{
    return m_pageLabels;
}

QVector<PdfOutlineEntry> PdfDocumentController::outlineEntries() const
{
    return m_outlineEntries;
}

PdfDocumentMetadata PdfDocumentController::metadata() const
{
    return m_metadata;
}

double PdfDocumentController::zoomFactor() const
{
    return m_zoomFactor;
}

QString PdfDocumentController::lastError() const
{
    return m_lastError;
}

QImage PdfDocumentController::currentPageImage() const
{
    return m_currentPageImage;
}

bool PdfDocumentController::hasTextSelection() const
{
    return !m_selectionModel.isEmpty();
}

bool PdfDocumentController::hasAreaSelection() const
{
    return !m_lastSelectionPageRect.isEmpty();
}

QString PdfDocumentController::selectedText() const
{
    return m_selectionModel.plainText();
}

bool PdfDocumentController::isOcrAvailable() const
{
    return OcrService::isAvailable();
}

bool PdfDocumentController::hasSearchResults() const
{
    return m_searchModel.hasResults();
}

QString PdfDocumentController::searchQuery() const
{
    return m_searchModel.query();
}

bool PdfDocumentController::requiresPassword() const
{
    return m_renderEngine->requiresPassword();
}

QByteArray PdfDocumentController::currentUserPassword() const
{
    return m_currentUserPassword;
}

QByteArray PdfDocumentController::currentOwnerPassword() const
{
    return m_currentOwnerPassword;
}

bool PdfDocumentController::printDocument(QPrinter *printer)
{
    if (!printer || !hasDocument()) {
        return false;
    }

    QPainter painter;
    if (!painter.begin(printer)) {
        m_lastError = QStringLiteral("Drucker konnte nicht initialisiert werden.");
        emit errorOccurred(m_lastError);
        return false;
    }

    const QRect paintRect = printer->pageLayout().paintRectPixels(printer->resolution());
    const double printScale = static_cast<double>(printer->resolution()) / 72.0;

    for (int pageIndex = 0; pageIndex < pageCount(); ++pageIndex) {
        const QImage pageImage = m_renderEngine->renderPage(pageIndex, printScale);
        if (pageImage.isNull()) {
            painter.end();
            m_lastError = m_renderEngine->lastError();
            emit errorOccurred(m_lastError);
            return false;
        }

        const QSize scaledSize = pageImage.size().scaled(paintRect.size(), Qt::KeepAspectRatio);
        const QRect targetRect(
            paintRect.x() + (paintRect.width() - scaledSize.width()) / 2,
            paintRect.y() + (paintRect.height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height());

        painter.drawImage(targetRect, pageImage);

        if (pageIndex < pageCount() - 1 && !printer->newPage()) {
            painter.end();
            m_lastError = QStringLiteral("Neue Druckseite konnte nicht erstellt werden.");
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    painter.end();
    emit statusMessageChanged(QStringLiteral("Dokument an Druckdialog übergeben."));
    return true;
}

bool PdfDocumentController::exportEditedPdf(const QString &outputFile, bool excludeSelectedSignatureAnnotation)
{
    if (!hasDocument() || outputFile.isEmpty()) {
        return false;
    }

    const QString excludedAnnotationId = (excludeSelectedSignatureAnnotation && hasSelectedSignatureAnnotation())
        ? m_annotationModel.selectedAnnotationId()
        : QString();
    const QVector<PdfAnnotation> annotations = filteredAnnotations(m_annotationModel.annotations(), excludedAnnotationId);
    const QVector<PdfFormField> formFields = m_formFieldModel.fields();
    const bool hasUnsupportedAnnotationKinds = std::any_of(
        annotations.begin(),
        annotations.end(),
        [](const PdfAnnotation &annotation) {
            return !isNativePdfAnnotationSupported(annotation);
        });
    const QVector<PdfRedaction> redactions = m_redactionModel.redactions();
    const bool formStateChanged = !formFieldsEqual(formFields, m_baseFormFields);

    if (!hasUnsupportedAnnotationKinds && redactions.isEmpty() && (!annotations.isEmpty() || formStateChanged)) {
        QVector<QSizeF> pageSizes;
        pageSizes.reserve(pageCount());
        for (int pageIndex = 0; pageIndex < pageCount(); ++pageIndex) {
            pageSizes.append(m_renderEngine->pageSizePoints(pageIndex));
        }

        QPdfOperations operations;
        const QByteArray password = m_currentOwnerPassword.isEmpty() ? m_currentUserPassword : m_currentOwnerPassword;
        if (operations.saveEditedState(m_documentPath, outputFile, annotations, formFields, redactions, pageSizes, password)) {
            emit statusMessageChanged(QStringLiteral("Bearbeitetes PDF mit nativen PDF-Aenderungen exportiert."));
            return true;
        }
    }

    emit busyStateChanged(true, QStringLiteral("Bearbeitetes PDF wird exportiert..."));

    QPdfWriter writer(outputFile);
    writer.setCreator(QStringLiteral("PDFTool"));
    writer.setResolution(144);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        emit busyStateChanged(false, QString());
        m_lastError = QStringLiteral("Ausgabe-PDF konnte nicht erstellt werden.");
        emit errorOccurred(m_lastError);
        return false;
    }

    for (int pageIndex = 0; pageIndex < pageCount(); ++pageIndex) {
        const QSizeF pageSizePoints = m_renderEngine->pageSizePoints(pageIndex);
        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            writer.setPageSize(QPageSize(pageSizePoints, QPageSize::Point, QString(), QPageSize::ExactMatch));
        }
        if (pageIndex > 0) {
            writer.newPage();
        }

        QImage pageImage = m_renderEngine->renderPage(pageIndex, kRedactionExportScale);
        if (pageImage.isNull()) {
            painter.end();
            emit busyStateChanged(false, QString());
            m_lastError = m_renderEngine->lastError();
            emit errorOccurred(m_lastError);
            return false;
        }

        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            const double scaleX = static_cast<double>(pageImage.width()) / pageSizePoints.width();
            const double scaleY = static_cast<double>(pageImage.height()) / pageSizePoints.height();

            QPainter imagePainter(&pageImage);
            imagePainter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

            const QVector<PdfAnnotation> pageAnnotations =
                filteredAnnotations(m_annotationModel.annotationsForPage(pageIndex), excludedAnnotationId);
            for (const PdfAnnotation &annotation : pageAnnotations) {
                switch (annotation.kind) {
                case PdfAnnotationKind::Highlight:
                    imagePainter.setPen(Qt::NoPen);
                    for (const QRectF &pageRect : annotation.pageRects) {
                        const QRectF imageRect(pageRect.left() * scaleX,
                                               pageRect.top() * scaleY,
                                               pageRect.width() * scaleX,
                                               pageRect.height() * scaleY);
                        imagePainter.fillRect(imageRect, annotation.color);
                    }
                    break;
                case PdfAnnotationKind::Rectangle: {
                    QPen pen(annotation.color, 2.0);
                    imagePainter.setPen(pen);
                    imagePainter.setBrush(Qt::NoBrush);
                    for (const QRectF &pageRect : annotation.pageRects) {
                        const QRectF imageRect(pageRect.left() * scaleX,
                                               pageRect.top() * scaleY,
                                               pageRect.width() * scaleX,
                                               pageRect.height() * scaleY);
                        imagePainter.drawRect(imageRect);
                    }
                    break;
                }
                case PdfAnnotationKind::Note:
                    imagePainter.setPen(QPen(QColor(121, 85, 72), 1.0));
                    imagePainter.setBrush(annotation.color);
                    for (const QRectF &pageRect : annotation.pageRects) {
                        const QRectF imageRect(pageRect.left() * scaleX,
                                               pageRect.top() * scaleY,
                                               pageRect.width() * scaleX,
                                               pageRect.height() * scaleY);
                        imagePainter.drawRect(imageRect);
                        imagePainter.setPen(Qt::black);
                        imagePainter.drawText(imageRect.adjusted(4.0, 2.0, -2.0, -2.0),
                                              Qt::AlignLeft | Qt::AlignTop,
                                              QStringLiteral("N"));
                        imagePainter.setPen(QPen(QColor(121, 85, 72), 1.0));
                    }
                    break;
                case PdfAnnotationKind::FreeText:
                    for (const QRectF &pageRect : annotation.pageRects) {
                        const QRectF imageRect(pageRect.left() * scaleX,
                                               pageRect.top() * scaleY,
                                               pageRect.width() * scaleX,
                                               pageRect.height() * scaleY);
                        imagePainter.fillRect(imageRect, annotation.color);
                        imagePainter.setPen(QPen(QColor(210, 210, 210), 1.0));
                        imagePainter.drawRect(imageRect);

                        QFont font = imagePainter.font();
                        font.setFamily(normalizedPdfFontFamily(annotation.textStyle.fontFamily));
                        const double pixelSize = std::max(1.0, annotation.textStyle.fontSize * scaleY);
                        font.setPixelSize(qRound(pixelSize));
                        imagePainter.setFont(font);
                        imagePainter.setPen(annotation.textStyle.textColor.isValid() ? annotation.textStyle.textColor : Qt::black);
                        imagePainter.drawText(imageRect.adjusted(6.0, 4.0, -6.0, -4.0),
                                              Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                                              annotation.text);
                    }
                    break;
                case PdfAnnotationKind::Signature:
                    for (const QRectF &pageRect : annotation.pageRects) {
                        QImage signatureImage;
                        signatureImage.loadFromData(annotation.binaryPayload);
                        if (signatureImage.isNull()) {
                            continue;
                        }

                        const QRectF imageRect(pageRect.left() * scaleX,
                                               pageRect.top() * scaleY,
                                               pageRect.width() * scaleX,
                                               pageRect.height() * scaleY);
                        imagePainter.drawImage(imageRect, signatureImage);
                    }
                    break;
                }
            }

            const QVector<PdfFormField> fields = m_formFieldModel.fieldsForPage(pageIndex);
            for (const PdfFormField &field : fields) {
                const QRectF imageRect(field.pageRect.left() * scaleX,
                                       field.pageRect.top() * scaleY,
                                       field.pageRect.width() * scaleX,
                                       field.pageRect.height() * scaleY);

                imagePainter.setPen(QPen(QColor(25, 118, 210), 1.0));
                imagePainter.setBrush(QColor(255, 255, 255, 230));
                imagePainter.drawRect(imageRect);

                if (field.kind == PdfFormFieldKind::Text) {
                    QFont font = imagePainter.font();
                    font.setFamily(field.textStyle.fontFamily);
                    font.setPixelSize(qMax(1, qRound(field.textStyle.fontSize * scaleY)));
                    imagePainter.setFont(font);
                    imagePainter.setPen(field.textStyle.textColor.isValid() ? field.textStyle.textColor : Qt::black);
                    imagePainter.drawText(imageRect.adjusted(4.0, 2.0, -4.0, -2.0),
                                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                                          field.textValue);
                } else if (field.kind == PdfFormFieldKind::CheckBox) {
                    const QRectF boxRect = imageRect.adjusted(3.0, 3.0, -3.0, -3.0);
                    imagePainter.setBrush(Qt::white);
                    imagePainter.setPen(QPen(Qt::black, 1.0));
                    imagePainter.drawRect(boxRect);

                    if (field.checked) {
                        imagePainter.setPen(QPen(Qt::black, 2.0));
                        imagePainter.drawLine(boxRect.topLeft() + QPointF(3.0, boxRect.height() * 0.55),
                                              boxRect.center() + QPointF(-1.0, boxRect.height() * 0.2));
                        imagePainter.drawLine(boxRect.center() + QPointF(-1.0, boxRect.height() * 0.2),
                                              boxRect.bottomRight() + QPointF(-3.0, -3.0));
                    }
                }
            }

            const QVector<PdfRedaction> redactions = m_redactionModel.redactionsForPage(pageIndex);
            imagePainter.setPen(Qt::NoPen);
            imagePainter.setBrush(Qt::black);
            for (const PdfRedaction &redaction : redactions) {
                const QRectF imageRect(redaction.pageRect.left() * scaleX,
                                       redaction.pageRect.top() * scaleY,
                                       redaction.pageRect.width() * scaleX,
                                       redaction.pageRect.height() * scaleY);
                imagePainter.drawRect(imageRect);
            }
        }

        const QRectF targetRect(0.0, 0.0, writer.width(), writer.height());
        painter.drawImage(targetRect, pageImage);
        drawSearchableOcrTextLayer(painter, pageImage, targetRect.size());
    }

    painter.end();
    emit busyStateChanged(false, QString());
    emit statusMessageChanged(QStringLiteral("Bearbeitetes PDF exportiert."));
    return true;
}

bool PdfDocumentController::exportRedactedPdf(const QString &outputFile)
{
    if (!hasDocument() || outputFile.isEmpty() || !m_redactionModel.hasRedactions()) {
        return false;
    }

    const QVector<PdfRedaction> redactions = m_redactionModel.redactions();
    QVector<QSizeF> pageSizes;
    QVector<QImage> flattenedPages;
    pageSizes.reserve(pageCount());
    flattenedPages.resize(pageCount());
    for (int pageIndex = 0; pageIndex < pageCount(); ++pageIndex) {
        pageSizes.append(m_renderEngine->pageSizePoints(pageIndex));
        const QVector<PdfRedaction> pageRedactions = m_redactionModel.redactionsForPage(pageIndex);
        if (pageRedactions.isEmpty()) {
            continue;
        }

        QImage pageImage = m_renderEngine->renderPage(pageIndex, kRedactionExportScale);
        if (pageImage.isNull()) {
            m_lastError = m_renderEngine->lastError();
            emit errorOccurred(m_lastError);
            return false;
        }

        const QSizeF pageSizePoints = m_renderEngine->pageSizePoints(pageIndex);
        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            QPainter imagePainter(&pageImage);
            const double scaleX = static_cast<double>(pageImage.width()) / pageSizePoints.width();
            const double scaleY = static_cast<double>(pageImage.height()) / pageSizePoints.height();
            for (const PdfRedaction &redaction : pageRedactions) {
                const QRectF imageRect(redaction.pageRect.left() * scaleX,
                                       redaction.pageRect.top() * scaleY,
                                       redaction.pageRect.width() * scaleX,
                                       redaction.pageRect.height() * scaleY);
                imagePainter.fillRect(imageRect, Qt::black);
            }
        }

        flattenedPages[pageIndex] = pageImage;
    }

    QPdfOperations operations;
    const QByteArray password = m_currentOwnerPassword.isEmpty() ? m_currentUserPassword : m_currentOwnerPassword;
    if (operations.saveDestructiveRedactedState(m_documentPath, outputFile, flattenedPages, pageSizes, password)) {
        emit statusMessageChanged(QStringLiteral("Geschwaerztes PDF mit destruktiver Redaction exportiert."));
        return true;
    }

    emit busyStateChanged(true, QStringLiteral("Geschwärztes PDF wird exportiert..."));

    QPdfWriter writer(outputFile);
    writer.setCreator(QStringLiteral("PDFTool"));
    writer.setResolution(144);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        emit busyStateChanged(false, QString());
        m_lastError = QStringLiteral("Ausgabe-PDF konnte nicht erstellt werden.");
        emit errorOccurred(m_lastError);
        return false;
    }

    for (int pageIndex = 0; pageIndex < pageCount(); ++pageIndex) {
        const QSizeF pageSizePoints = m_renderEngine->pageSizePoints(pageIndex);
        if (pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            writer.setPageSize(QPageSize(pageSizePoints, QPageSize::Point, QString(), QPageSize::ExactMatch));
        }
        if (pageIndex > 0) {
            writer.newPage();
        }

        QImage pageImage = m_renderEngine->renderPage(pageIndex, kRedactionExportScale);
        if (pageImage.isNull()) {
            painter.end();
            emit busyStateChanged(false, QString());
            m_lastError = m_renderEngine->lastError();
            emit errorOccurred(m_lastError);
            return false;
        }

        const QVector<PdfRedaction> redactions = m_redactionModel.redactionsForPage(pageIndex);
        if (!redactions.isEmpty() && pageSizePoints.isValid() && !pageSizePoints.isEmpty()) {
            QPainter imagePainter(&pageImage);
            const double scaleX = static_cast<double>(pageImage.width()) / pageSizePoints.width();
            const double scaleY = static_cast<double>(pageImage.height()) / pageSizePoints.height();

            for (const PdfRedaction &redaction : redactions) {
                const QRectF imageRect(redaction.pageRect.left() * scaleX,
                                       redaction.pageRect.top() * scaleY,
                                       redaction.pageRect.width() * scaleX,
                                       redaction.pageRect.height() * scaleY);
                imagePainter.fillRect(imageRect, Qt::black);
            }
        }

        const QRectF targetRect(0.0, 0.0, writer.width(), writer.height());
        painter.drawImage(targetRect, pageImage);
        drawSearchableOcrTextLayer(painter, pageImage, targetRect.size());
    }

    painter.end();
    emit busyStateChanged(false, QString());
    emit statusMessageChanged(QStringLiteral("Geschwärztes PDF exportiert."));
    return true;
}

bool PdfDocumentController::hasSelectedOverlay() const
{
    return m_annotationModel.hasSelectedAnnotation() || m_redactionModel.hasSelectedRedaction();
}

PdfAnnotationKind PdfDocumentController::selectedAnnotationKind() const
{
    return m_annotationModel.selectedAnnotationKind();
}

QColor PdfDocumentController::selectedAnnotationColor() const
{
    return m_annotationModel.selectedAnnotationColor();
}

bool PdfDocumentController::hasRedactions() const
{
    return m_redactionModel.hasRedactions();
}

bool PdfDocumentController::hasTextEditAnnotations() const
{
    return m_annotationModel.hasAnnotationKind(PdfAnnotationKind::FreeText);
}

bool PdfDocumentController::hasSelectedNoteAnnotation() const
{
    return m_annotationModel.hasSelectedAnnotation()
        && m_annotationModel.selectedAnnotationKind() == PdfAnnotationKind::Note;
}

QString PdfDocumentController::selectedNoteText() const
{
    return hasSelectedNoteAnnotation() ? m_annotationModel.selectedAnnotationText() : QString();
}

bool PdfDocumentController::hasSelectedTextEditAnnotation() const
{
    return m_annotationModel.hasSelectedAnnotation()
        && m_annotationModel.selectedAnnotationKind() == PdfAnnotationKind::FreeText;
}

bool PdfDocumentController::hasSelectedMovableAnnotation() const
{
    if (!m_annotationModel.hasSelectedAnnotation()) {
        return false;
    }

    const PdfAnnotationKind kind = m_annotationModel.selectedAnnotationKind();
    return kind == PdfAnnotationKind::Signature || kind == PdfAnnotationKind::FreeText;
}

bool PdfDocumentController::textFormFieldStyleAt(
    const QPointF &imagePoint,
    QString &fieldId,
    QString &label,
    PdfTextStyle &style) const
{
    if (!hasDocument()) {
        return false;
    }

    const QString id = m_formFieldModel.fieldIdAt(m_currentPageIndex, imagePointToPagePoint(imagePoint));
    if (id.isEmpty()) {
        return false;
    }

    PdfTextStyle fieldStyle;
    if (!m_formFieldModel.textFieldStyle(id, fieldStyle)) {
        return false;
    }

    for (const PdfFormField &field : m_formFieldModel.fieldsForPage(m_currentPageIndex)) {
        if (field.id == id && field.kind == PdfFormFieldKind::Text) {
            fieldId = id;
            label = field.label;
            style = fieldStyle;
            return true;
        }
    }

    return false;
}

QString PdfDocumentController::selectedTextEditText() const
{
    return hasSelectedTextEditAnnotation() ? m_annotationModel.selectedAnnotationText() : QString();
}

PdfTextStyle PdfDocumentController::selectedTextEditStyle() const
{
    return hasSelectedTextEditAnnotation() ? m_annotationModel.selectedAnnotationTextStyle() : PdfTextStyle();
}

QColor PdfDocumentController::selectedTextEditBackgroundColor() const
{
    return hasSelectedTextEditAnnotation() ? m_annotationModel.selectedAnnotationColor() : QColor();
}

bool PdfDocumentController::hasSelectedSignatureAnnotation() const
{
    return m_annotationModel.hasSelectedAnnotation()
        && m_annotationModel.selectedAnnotationKind() == PdfAnnotationKind::Signature;
}

bool PdfDocumentController::selectedSignatureAnnotationData(int &pageIndex, QRectF &pageRect, QByteArray &imageData) const
{
    if (!hasSelectedSignatureAnnotation()) {
        return false;
    }

    const QString annotationId = m_annotationModel.selectedAnnotationId();
    if (annotationId.isEmpty()) {
        return false;
    }

    const QVector<PdfAnnotation> annotations = m_annotationModel.annotations();
    for (const PdfAnnotation &annotation : annotations) {
        if (annotation.id != annotationId || annotation.kind != PdfAnnotationKind::Signature || annotation.pageRects.isEmpty()) {
            continue;
        }

        pageIndex = annotation.pageIndex;
        pageRect = annotation.pageRects.first().normalized();
        imageData = annotation.binaryPayload;
        return true;
    }

    return false;
}

bool PdfDocumentController::canUndo() const
{
    return m_historyIndex > 0 && m_historyIndex < m_historySnapshots.size();
}

bool PdfDocumentController::canRedo() const
{
    return m_historyIndex >= 0 && m_historyIndex < (m_historySnapshots.size() - 1);
}

QPixmap PdfDocumentController::thumbnailForPage(int pageIndex, const QSize &targetSize)
{
    if (!m_thumbnailProvider) {
        return {};
    }

    m_thumbnailProvider->setThumbnailSize(targetSize);
    return m_thumbnailProvider->thumbnailForPage(pageIndex);
}

void PdfDocumentController::setThumbnailSize(const QSize &size)
{
    if (m_thumbnailProvider) {
        m_thumbnailProvider->setThumbnailSize(size);
    }
}

void PdfDocumentController::zoomIn()
{
    setZoomFactor(m_zoomFactor + kZoomStep);
}

void PdfDocumentController::zoomOut()
{
    setZoomFactor(m_zoomFactor - kZoomStep);
}

void PdfDocumentController::resetZoom()
{
    setZoomFactor(1.0);
}

void PdfDocumentController::setZoomFactor(double factor)
{
    const double clampedFactor = std::clamp(factor, kMinZoom, kMaxZoom);
    if (qFuzzyCompare(m_zoomFactor, clampedFactor) || !hasDocument()) {
        return;
    }

    m_zoomFactor = clampedFactor;
    emit zoomChanged(m_zoomFactor);
    renderCurrentPage();
}

void PdfDocumentController::goToPage(int pageIndex)
{
    if (!hasDocument()) {
        return;
    }

    if (pageIndex < 0 || pageIndex >= pageCount() || pageIndex == m_currentPageIndex) {
        return;
    }

    clearSelectionInternal(true);
    m_currentPageIndex = pageIndex;
    renderCurrentPage();
}

void PdfDocumentController::nextPage()
{
    goToPage(m_currentPageIndex + 1);
}

void PdfDocumentController::previousPage()
{
    goToPage(m_currentPageIndex - 1);
}

void PdfDocumentController::updateTextSelection(const QRectF &imageSelectionRect)
{
    if (!hasDocument() || m_currentPageImage.isNull()) {
        clearSelectionInternal(true);
        return;
    }

    const QRectF normalizedImageRect = imageSelectionRect.normalized()
        .intersected(QRectF(QPointF(0.0, 0.0), QSizeF(m_currentPageImage.size())));

    if (normalizedImageRect.width() < 2.0 || normalizedImageRect.height() < 2.0) {
        clearSelectionInternal(true);
        return;
    }

    m_lastSelectionPageRect = imageRectToPageRect(normalizedImageRect);
    if (m_lastSelectionPageRect.isEmpty()) {
        clearSelectionInternal(true);
        return;
    }

    const PdfTextSelection selection = m_renderEngine->buildTextSelection(m_currentPageIndex, m_lastSelectionPageRect);
    emit selectionHighlightChanged({ normalizedImageRect });

    if (selection.isEmpty() || selection.toPlainText().isEmpty()) {
        m_selectionModel.clear();
        emit textSelectionChanged(false);
        return;
    }

    m_selectionModel.setSelection(m_currentPageIndex, selection);
    emit textSelectionChanged(true);
}

void PdfDocumentController::clearTextSelection()
{
    clearSelectionInternal(true);
}

void PdfDocumentController::copySelectedText()
{
    const QString text = m_selectionModel.plainText();
    if (text.isEmpty()) {
        return;
    }

    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text, QClipboard::Clipboard);
        clipboard->setText(text, QClipboard::Selection);
    }

    emit statusMessageChanged(QStringLiteral("Text in die Zwischenablage kopiert."));
}

void PdfDocumentController::addHighlightAnnotationFromSelection()
{
    if (!m_annotationModel.addHighlightFromSelection(m_selectionModel)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Highlight-Annotation hinzugefügt."));
}

void PdfDocumentController::addRectangleAnnotationFromSelection()
{
    if (!hasAreaSelection()) {
        return;
    }

    if (!m_annotationModel.addRectangle(m_currentPageIndex, m_lastSelectionPageRect)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Rechteck-Annotation hinzugefügt."));
}

void PdfDocumentController::addNoteAnnotationAt(const QPointF &imagePoint, const QString &noteText)
{
    if (!hasDocument() || noteText.trimmed().isEmpty()) {
        return;
    }

    const QPointF pagePoint = imagePointToPagePoint(imagePoint);
    const QRectF noteRect(pagePoint.x(), pagePoint.y(), 20.0, 20.0);
    if (!m_annotationModel.addNote(m_currentPageIndex, noteRect, noteText)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Textnotiz hinzugefügt."));
}

void PdfDocumentController::addFreeTextAt(
    const QPointF &imagePoint,
    const QString &text,
    const PdfTextStyle &style,
    const QColor &backgroundColor)
{
    if (!hasDocument() || text.trimmed().isEmpty()) {
        return;
    }

    const QPointF pagePoint = imagePointToPagePoint(imagePoint);
    const QRectF textRect(pagePoint.x(), pagePoint.y(), 180.0, 48.0);
    if (!m_annotationModel.addFreeText(m_currentPageIndex, textRect, text, style, backgroundColor)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Textfeld hinzugefügt."));
}

void PdfDocumentController::replaceSelectedText(
    const QString &text,
    const PdfTextStyle &style,
    const QColor &backgroundColor)
{
    if (!hasAreaSelection() || text.trimmed().isEmpty()) {
        return;
    }

    if (!m_annotationModel.addFreeText(m_currentPageIndex, m_lastSelectionPageRect, text, style, backgroundColor)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Text ersetzt."));
}

void PdfDocumentController::addSignatureFromImageAt(const QPointF &imagePoint, const QImage &signatureImage)
{
    if (!hasDocument() || signatureImage.isNull() || m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return;
    }

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    signatureImage.save(&buffer, "PNG");

    const QSize scaledSize = signatureImage.size().scaled(220, 96, Qt::KeepAspectRatio);
    const QPointF pagePoint = imagePointToPagePoint(imagePoint);
    const QRectF pageRect(
        pagePoint.x(),
        pagePoint.y(),
        scaledSize.width() * m_pageSizePoints.width() / qMax(1, m_currentPageImage.width()),
        scaledSize.height() * m_pageSizePoints.height() / qMax(1, m_currentPageImage.height()));

    if (!m_annotationModel.addSignature(m_currentPageIndex, pageRect, pngBytes)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Unterschrift eingefügt."));
}

void PdfDocumentController::addSignatureFromImageToSelection(const QImage &signatureImage)
{
    if (!hasDocument() || signatureImage.isNull() || !hasAreaSelection()) {
        return;
    }

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    signatureImage.save(&buffer, "PNG");

    if (!m_annotationModel.addSignature(m_currentPageIndex, m_lastSelectionPageRect, pngBytes)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Unterschrift platziert."));
}

void PdfDocumentController::moveSelectedSignatureBy(const QPointF &imageDelta)
{
    if (!hasSelectedMovableAnnotation() || m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return;
    }

    const QPointF pageDelta(
        imageDelta.x() * m_pageSizePoints.width() / qMax(1, m_currentPageImage.width()),
        imageDelta.y() * m_pageSizePoints.height() / qMax(1, m_currentPageImage.height()));

    if (!m_annotationModel.translateSelected(pageDelta)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(
        m_annotationModel.selectedAnnotationKind() == PdfAnnotationKind::FreeText
            ? QStringLiteral("Textfeld verschoben.")
            : QStringLiteral("Unterschrift verschoben."));
}

void PdfDocumentController::resizeSelectedSignatureBy(const QPointF &imageDelta)
{
    if (!hasSelectedSignatureAnnotation() || m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return;
    }

    const QSizeF pageDelta(
        imageDelta.x() * m_pageSizePoints.width() / qMax(1, m_currentPageImage.width()),
        imageDelta.y() * m_pageSizePoints.height() / qMax(1, m_currentPageImage.height()));

    if (!m_annotationModel.resizeSelectedSignature(pageDelta)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Unterschriftgröße geändert."));
}

void PdfDocumentController::resizeSelectedTextEditBy(const QPointF &imageDelta)
{
    if (!hasSelectedTextEditAnnotation() || m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return;
    }

    const QSizeF pageDelta(
        imageDelta.x() * m_pageSizePoints.width() / qMax(1, m_currentPageImage.width()),
        imageDelta.y() * m_pageSizePoints.height() / qMax(1, m_currentPageImage.height()));

    if (!m_annotationModel.resizeSelectedFreeText(pageDelta)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Textfeldgröße geändert."));
}

bool PdfDocumentController::remapPageOrder(const QVector<int> &newOrder)
{
    if (!hasDocument()) {
        return false;
    }

    if (newOrder.isEmpty()) {
        m_lastError = QStringLiteral("Die neue Seitenreihenfolge ist leer.");
        return false;
    }

    m_annotationModel.remapPages(newOrder);
    m_formFieldModel.remapPages(newOrder);
    m_redactionModel.remapPages(newOrder);
    clearSelectionInternal(false);
    m_searchModel.clear();
    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emitSearchState();
    updateOverlaySelectionSignal();
    return true;
}

void PdfDocumentController::selectOverlayAt(const QPointF &imagePoint)
{
    if (!hasDocument()) {
        return;
    }

    const QPointF pagePoint = imagePointToPagePoint(imagePoint);
    const bool annotationSelected = m_annotationModel.selectAt(m_currentPageIndex, pagePoint);
    const bool redactionSelected = !annotationSelected && m_redactionModel.selectAt(m_currentPageIndex, pagePoint);
    if (!annotationSelected) {
        m_annotationModel.clearSelection();
    }
    if (!redactionSelected) {
        m_redactionModel.clearSelection();
    }

    emitOverlayState();
    updateOverlaySelectionSignal();
}

void PdfDocumentController::deleteSelectedOverlay()
{
    bool changed = false;

    const QString annotationId = m_annotationModel.selectedAnnotationId();
    if (!annotationId.isEmpty()) {
        changed = m_annotationModel.remove(annotationId);
    } else {
        const QString redactionId = m_redactionModel.selectedRedactionId();
        if (!redactionId.isEmpty()) {
            changed = m_redactionModel.remove(redactionId);
        }
    }

    if (!changed) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Ausgewähltes Overlay gelöscht."));
}

void PdfDocumentController::setSelectedAnnotationColor(const QColor &color)
{
    const QString annotationId = m_annotationModel.selectedAnnotationId();
    if (annotationId.isEmpty()) {
        return;
    }

    if (!m_annotationModel.setColor(annotationId, color)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Annotierungsfarbe aktualisiert."));
}

void PdfDocumentController::updateSelectedNoteText(const QString &text)
{
    const QString annotationId = m_annotationModel.selectedAnnotationId();
    if (annotationId.isEmpty()) {
        return;
    }

    if (!m_annotationModel.setText(annotationId, text)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Textnotiz aktualisiert."));
}

void PdfDocumentController::updateSelectedTextEdit(
    const QString &text,
    const PdfTextStyle &style,
    const QColor &backgroundColor)
{
    const QString annotationId = m_annotationModel.selectedAnnotationId();
    if (annotationId.isEmpty()) {
        return;
    }

    if (!m_annotationModel.setText(annotationId, text)
        || !m_annotationModel.setFreeTextStyle(annotationId, style)
        || !m_annotationModel.setColor(annotationId, backgroundColor)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Textfeld aktualisiert."));
}

void PdfDocumentController::saveDocumentState()
{
    if (!hasDocument()) {
        return;
    }

    QTemporaryFile tempFile(QFileInfo(m_documentPath).absolutePath() + QLatin1String("/.pdftool-save-XXXXXX.pdf"));
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        m_lastError = QStringLiteral("Temporäre Zieldatei konnte nicht angelegt werden.");
        emit errorOccurred(m_lastError);
        return;
    }
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    if (!exportEditedPdf(tempPath)) {
        QFile::remove(tempPath);
        return;
    }

    QFile originalFile(m_documentPath);
    const QString backupPath = m_documentPath + QStringLiteral(".bak");
    QFile::remove(backupPath);
    if (originalFile.exists() && !originalFile.rename(backupPath)) {
        QFile::remove(tempPath);
        m_lastError = QStringLiteral("Originaldatei konnte nicht gesichert werden.");
        emit errorOccurred(m_lastError);
        return;
    }

    if (!QFile::rename(tempPath, m_documentPath)) {
        QFile::rename(backupPath, m_documentPath);
        QFile::remove(tempPath);
        m_lastError = QStringLiteral("Gespeicherte PDF konnte nicht an den Zielort verschoben werden.");
        emit errorOccurred(m_lastError);
        return;
    }

    QFile::remove(backupPath);
    QFile::remove(annotationSidecarPath());

    const int reopenPageIndex = m_currentPageIndex;
    const QByteArray ownerPassword = m_currentOwnerPassword;
    const QByteArray userPassword = m_currentUserPassword;
    if (!openDocument(m_documentPath, ownerPassword, userPassword)) {
        m_lastError = QStringLiteral("PDF wurde gespeichert, konnte aber nicht erneut geladen werden.");
        emit errorOccurred(m_lastError);
        return;
    }

    if (reopenPageIndex >= 0 && reopenPageIndex < pageCount()) {
        goToPage(reopenPageIndex);
    }

    emit statusMessageChanged(QStringLiteral("Änderungen direkt im PDF gespeichert."));
}

void PdfDocumentController::undo()
{
    if (!canUndo()) {
        return;
    }

    --m_historyIndex;
    restoreDocumentState(m_historySnapshots.at(m_historyIndex));
    saveSidecarState();
    updateHistoryState();
    emit statusMessageChanged(QStringLiteral("Letzte Änderung rückgängig gemacht."));
}

void PdfDocumentController::redo()
{
    if (!canRedo()) {
        return;
    }

    ++m_historyIndex;
    restoreDocumentState(m_historySnapshots.at(m_historyIndex));
    saveSidecarState();
    updateHistoryState();
    emit statusMessageChanged(QStringLiteral("Änderung wiederhergestellt."));
}

void PdfDocumentController::setSearchQuery(const QString &query)
{
    if (!hasDocument()) {
        return;
    }

    const QString trimmedQuery = query.trimmed();
    if (trimmedQuery.isEmpty()) {
        clearSearch();
        return;
    }

    emit busyStateChanged(true, QStringLiteral("Suche läuft..."));
    m_searchModel.setResults(trimmedQuery, m_renderEngine->search(trimmedQuery));
    emit busyStateChanged(false, QString());
    if (!m_searchModel.hasResults()) {
        emitOverlayState();
        emitSearchState(QStringLiteral("Keine Treffer für \"%1\".").arg(trimmedQuery));
        return;
    }

    if (const PdfSearchHit *hit = m_searchModel.currentHit()) {
        if (hit->pageIndex != m_currentPageIndex) {
            m_currentPageIndex = hit->pageIndex;
            renderCurrentPage();
        } else {
            emitOverlayState();
        }
    }

    emitSearchState();
}

void PdfDocumentController::findNext()
{
    if (const PdfSearchHit *hit = m_searchModel.next()) {
        if (hit->pageIndex != m_currentPageIndex) {
            m_currentPageIndex = hit->pageIndex;
            renderCurrentPage();
        } else {
            emitOverlayState();
        }
        emitSearchState();
    }
}

void PdfDocumentController::findPrevious()
{
    if (const PdfSearchHit *hit = m_searchModel.previous()) {
        if (hit->pageIndex != m_currentPageIndex) {
            m_currentPageIndex = hit->pageIndex;
            renderCurrentPage();
        } else {
            emitOverlayState();
        }
        emitSearchState();
    }
}

void PdfDocumentController::clearSearch()
{
    m_searchModel.clear();
    emitOverlayState();
    emitSearchState(QStringLiteral("Suche zurückgesetzt."));
}

void PdfDocumentController::setFormFieldText(const QString &fieldId, const QString &text)
{
    if (!m_formFieldModel.setTextValue(fieldId, text)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Formularwert aktualisiert."));
}

void PdfDocumentController::setFormFieldTextStyle(const QString &fieldId, const PdfTextStyle &style)
{
    if (!m_formFieldModel.setTextStyle(fieldId, style)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Formularschrift aktualisiert."));
}

void PdfDocumentController::setFormFieldChecked(const QString &fieldId, bool checked)
{
    if (!m_formFieldModel.setChecked(fieldId, checked)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Checkbox aktualisiert."));
}

void PdfDocumentController::addRedactionFromSelection()
{
    if (!hasAreaSelection()) {
        return;
    }

    if (!m_redactionModel.add(m_currentPageIndex, m_lastSelectionPageRect)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Schwärzung vorgemerkt."));
}

void PdfDocumentController::runOcrOnCurrentPage()
{
    if (!hasDocument()) {
        return;
    }

    emit busyStateChanged(true, QStringLiteral("OCR für aktuelle Seite läuft..."));
    QString errorMessage;
    const QImage image = m_renderEngine->renderPage(m_currentPageIndex, 2.5);
    const QString text = OcrService::recognizeImage(image, &errorMessage, QStringLiteral("deu+eng"), 3);
    emit busyStateChanged(false, QString());

    if (text.isEmpty()) {
        m_lastError = errorMessage.isEmpty() ? QStringLiteral("OCR hat keinen Text erkannt.") : errorMessage;
        emit errorOccurred(m_lastError);
        return;
    }

    emit ocrFinished(QStringLiteral("OCR: Aktuelle Seite"), text);
    emit statusMessageChanged(QStringLiteral("OCR für aktuelle Seite abgeschlossen."));
}

void PdfDocumentController::runOcrOnSelection()
{
    if (!hasDocument() || !hasAreaSelection() || m_currentPageImage.isNull()) {
        return;
    }

    const QRectF imageRect = pageRectToImageRect(m_lastSelectionPageRect)
        .intersected(QRectF(QPointF(0.0, 0.0), QSizeF(m_currentPageImage.size())));
    if (imageRect.isEmpty()) {
        return;
    }

    emit busyStateChanged(true, QStringLiteral("OCR für Auswahl läuft..."));
    QString errorMessage;
    const QImage croppedImage = m_currentPageImage.copy(imageRect.toAlignedRect());
    const QString text = OcrService::recognizeImage(croppedImage, &errorMessage, QStringLiteral("deu+eng"), 6);
    emit busyStateChanged(false, QString());

    if (text.isEmpty()) {
        m_lastError = errorMessage.isEmpty() ? QStringLiteral("OCR hat keinen Text erkannt.") : errorMessage;
        emit errorOccurred(m_lastError);
        return;
    }

    emit ocrFinished(QStringLiteral("OCR: Auswahl"), text);
    emit statusMessageChanged(QStringLiteral("OCR für Auswahl abgeschlossen."));
}

QString PdfDocumentController::annotationSidecarPath() const
{
    if (m_documentPath.isEmpty()) {
        return {};
    }

    return QStringLiteral("%1.annotations.json").arg(m_documentPath);
}

void PdfDocumentController::loadSidecarState()
{
    const QString sidecarPath = annotationSidecarPath();
    if (sidecarPath.isEmpty()) {
        return;
    }

    QFile file(sidecarPath);
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit statusMessageChanged(QStringLiteral("Sidecar-Datei konnte nicht geladen werden."));
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        emit statusMessageChanged(QStringLiteral("Sidecar-Datei ist ungültig."));
        return;
    }

    const QJsonObject root = document.object();
    if (root.contains(QStringLiteral("version"))) {
        m_annotationModel.fromJson(root.value(QStringLiteral("annotations")).toArray());
        m_formFieldModel.fromJson(root.value(QStringLiteral("formFields")).toArray());
        m_redactionModel.fromJson(root.value(QStringLiteral("redactions")).toArray());
        return;
    }

    // Backward compatibility for the earlier MVP sidecar schema.
    if (root.contains(QStringLiteral("annotations"))) {
        m_annotationModel.fromJson(root.value(QStringLiteral("annotations")).toArray());
    }
}

void PdfDocumentController::saveSidecarState() const
{
    const QString sidecarPath = annotationSidecarPath();
    if (sidecarPath.isEmpty()) {
        return;
    }

    QFile file(sidecarPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), kSidecarVersion);
    root.insert(QStringLiteral("annotations"), m_annotationModel.toJson());
    root.insert(QStringLiteral("formFields"), m_formFieldModel.toJson());
    root.insert(QStringLiteral("redactions"), m_redactionModel.toJson());
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QJsonObject PdfDocumentController::currentDocumentState() const
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), kSidecarVersion);
    root.insert(QStringLiteral("annotations"), m_annotationModel.toJson());
    root.insert(QStringLiteral("formFields"), m_formFieldModel.toJson());
    root.insert(QStringLiteral("redactions"), m_redactionModel.toJson());
    return root;
}

void PdfDocumentController::restoreDocumentState(const QJsonObject &state)
{
    m_annotationModel.fromJson(state.value(QStringLiteral("annotations")).toArray());
    m_formFieldModel.setFields(m_baseFormFields);
    m_formFieldModel.fromJson(state.value(QStringLiteral("formFields")).toArray());
    m_redactionModel.fromJson(state.value(QStringLiteral("redactions")).toArray());

    emitOverlayState();
    updateOverlaySelectionSignal();
}

void PdfDocumentController::resetHistory()
{
    m_historySnapshots.clear();
    m_historyIndex = -1;
    updateHistoryState();
}

void PdfDocumentController::recordHistorySnapshot()
{
    const QJsonObject snapshot = currentDocumentState();
    if (m_historyIndex >= 0
        && m_historyIndex < m_historySnapshots.size()
        && m_historySnapshots.at(m_historyIndex) == snapshot) {
        updateHistoryState();
        return;
    }

    while (m_historySnapshots.size() > m_historyIndex + 1) {
        m_historySnapshots.removeLast();
    }

    m_historySnapshots.append(snapshot);
    m_historyIndex = m_historySnapshots.size() - 1;
    updateHistoryState();
}

void PdfDocumentController::updateHistoryState()
{
    emit historyStateChanged(canUndo(), canRedo());
}

void PdfDocumentController::resetDocumentState()
{
    m_documentPath.clear();
    m_pageLabels.clear();
    m_outlineEntries.clear();
    m_metadata = {};
    m_selectionModel.clear();
    m_searchModel.clear();
    m_annotationModel.clear();
    m_formFieldModel.clear();
    m_redactionModel.clear();
    m_baseFormFields.clear();
    if (m_thumbnailProvider) {
        m_thumbnailProvider->clear();
    }
    m_currentPageImage = {};
    m_lastError.clear();
    m_pageSizePoints = {};
    m_lastSelectionPageRect = {};
    m_currentPageIndex = 0;
    m_zoomFactor = 1.0;
    m_currentOwnerPassword.clear();
    m_currentUserPassword.clear();
    resetHistory();
}

void PdfDocumentController::clearSelectionInternal(bool emitSignal)
{
    m_selectionModel.clear();
    m_lastSelectionPageRect = {};

    if (emitSignal) {
        emit selectionHighlightChanged({});
        emit textSelectionChanged(false);
    }
}

QRectF PdfDocumentController::imageRectToPageRect(const QRectF &imageRect) const
{
    if (m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return {};
    }

    const double scaleX = m_pageSizePoints.width() / static_cast<double>(m_currentPageImage.width());
    const double scaleY = m_pageSizePoints.height() / static_cast<double>(m_currentPageImage.height());

    return QRectF(imageRect.left() * scaleX,
                  imageRect.top() * scaleY,
                  imageRect.width() * scaleX,
                  imageRect.height() * scaleY).normalized();
}

QPointF PdfDocumentController::imagePointToPagePoint(const QPointF &imagePoint) const
{
    if (m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return {};
    }

    const double scaleX = m_pageSizePoints.width() / static_cast<double>(m_currentPageImage.width());
    const double scaleY = m_pageSizePoints.height() / static_cast<double>(m_currentPageImage.height());
    return QPointF(imagePoint.x() * scaleX, imagePoint.y() * scaleY);
}

QVector<QRectF> PdfDocumentController::pageRectsToImageRects(const QVector<QRectF> &pageRects) const
{
    QVector<QRectF> imageRects;
    if (m_currentPageImage.isNull() || m_pageSizePoints.isEmpty()) {
        return imageRects;
    }

    const double scaleX = static_cast<double>(m_currentPageImage.width()) / m_pageSizePoints.width();
    const double scaleY = static_cast<double>(m_currentPageImage.height()) / m_pageSizePoints.height();

    imageRects.reserve(pageRects.size());
    for (const QRectF &pageRect : pageRects) {
        imageRects.append(QRectF(pageRect.left() * scaleX,
                                 pageRect.top() * scaleY,
                                 pageRect.width() * scaleX,
                                 pageRect.height() * scaleY));
    }

    return imageRects;
}

QRectF PdfDocumentController::pageRectToImageRect(const QRectF &pageRect) const
{
    const QVector<QRectF> imageRects = pageRectsToImageRects({ pageRect });
    return imageRects.isEmpty() ? QRectF() : imageRects.first();
}

void PdfDocumentController::emitOverlayState()
{
    QVector<QRectF> selectionRects;
    if (!m_lastSelectionPageRect.isEmpty()) {
        selectionRects.append(pageRectToImageRect(m_lastSelectionPageRect));
    }
    emit selectionHighlightChanged(selectionRects);

    emit searchHighlightChanged(pageRectsToImageRects(m_searchModel.hitRectsForPage(m_currentPageIndex)),
                                pageRectsToImageRects(m_searchModel.currentHitRectsForPage(m_currentPageIndex)));

    QVector<PdfAnnotationOverlay> annotationOverlays;
    for (const PdfAnnotation &annotation : m_annotationModel.annotationsForPage(m_currentPageIndex)) {
        PdfAnnotationOverlay overlay;
        overlay.id = annotation.id;
        overlay.kind = annotation.kind;
        overlay.imageRects = pageRectsToImageRects(annotation.pageRects);
        overlay.color = annotation.color;
        overlay.textStyle = annotation.textStyle;
        overlay.text = annotation.text;
        overlay.binaryPayload = annotation.binaryPayload;
        overlay.selected = annotation.selected;
        annotationOverlays.append(overlay);
    }
    emit annotationOverlaysChanged(annotationOverlays);

    QVector<PdfFormFieldOverlay> formFieldOverlays;
    for (const PdfFormField &field : m_formFieldModel.fieldsForPage(m_currentPageIndex)) {
        PdfFormFieldOverlay overlay;
        overlay.id = field.id;
        overlay.kind = field.kind;
        overlay.imageRect = pageRectToImageRect(field.pageRect);
        overlay.label = field.label;
        overlay.textValue = field.textValue;
        overlay.textStyle = field.textStyle;
        overlay.checked = field.checked;
        overlay.readOnly = field.readOnly;
        formFieldOverlays.append(overlay);
    }
    emit formFieldOverlaysChanged(formFieldOverlays);

    QVector<PdfRedactionOverlay> redactionOverlays;
    for (const PdfRedaction &redaction : m_redactionModel.redactionsForPage(m_currentPageIndex)) {
        PdfRedactionOverlay overlay;
        overlay.id = redaction.id;
        overlay.imageRect = pageRectToImageRect(redaction.pageRect);
        overlay.selected = redaction.selected;
        redactionOverlays.append(overlay);
    }
    emit redactionOverlaysChanged(redactionOverlays);
}

void PdfDocumentController::emitSearchState(const QString &fallbackMessage)
{
    if (!fallbackMessage.isEmpty()) {
        emit searchStateChanged(false, 0, 0, fallbackMessage);
        return;
    }

    if (!m_searchModel.hasResults()) {
        emit searchStateChanged(false, 0, 0, QStringLiteral("Keine aktive Suche."));
        return;
    }

    const PdfSearchHit *currentHit = m_searchModel.currentHit();
    const int currentIndex = currentHit ? m_searchModel.currentIndex() + 1 : 0;
    const int totalHits = m_searchModel.hitCount();
    const QString status = QStringLiteral("Treffer %1/%2 für \"%3\"")
        .arg(currentIndex)
        .arg(totalHits)
        .arg(m_searchModel.query());
    emit searchStateChanged(true, currentIndex, totalHits, status);
}

void PdfDocumentController::updateOverlaySelectionSignal()
{
    emit overlaySelectionChanged(hasSelectedOverlay());
}

void PdfDocumentController::renderCurrentPage()
{
    m_currentPageImage = m_renderEngine->renderPage(m_currentPageIndex, m_zoomFactor);
    if (m_currentPageImage.isNull()) {
        m_lastError = m_renderEngine->lastError();
        emit errorOccurred(m_lastError);
        return;
    }

    m_pageSizePoints = m_renderEngine->pageSizePoints(m_currentPageIndex);
    m_lastError.clear();

    emit pageImageChanged(m_currentPageImage);
    emit currentPageChanged(m_currentPageIndex, pageCount(), currentPageLabel());
    emitOverlayState();
    emitSearchState();
    updateOverlaySelectionSignal();
}
