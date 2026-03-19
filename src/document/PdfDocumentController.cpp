#include "document/PdfDocumentController.h"

#include <algorithm>

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFont>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>

#include <QtPrintSupport/QPrinter>

#include "document/PageThumbnailProvider.h"
#include "rendering/PdfRenderEngine.h"

namespace
{
constexpr double kMinZoom = 0.5;
constexpr double kMaxZoom = 4.0;
constexpr double kZoomStep = 0.25;
constexpr double kRedactionExportScale = 2.0;
constexpr int kSidecarVersion = 2;
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
    for (int pageIndex = 0; pageIndex < m_renderEngine->pageCount(); ++pageIndex) {
        const QVector<PdfFormField> pageFields = m_renderEngine->formFields(pageIndex);
        fields += pageFields;
    }
    m_formFieldModel.setFields(fields);
    loadSidecarState();

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

bool PdfDocumentController::exportEditedPdf(const QString &outputFile)
{
    if (!hasDocument() || outputFile.isEmpty()) {
        return false;
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

            const QVector<PdfAnnotation> annotations = m_annotationModel.annotationsForPage(pageIndex);
            for (const PdfAnnotation &annotation : annotations) {
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
                        const double pointSize = std::clamp(imageRect.height() * 0.18, 8.0, 20.0);
                        font.setPointSizeF(pointSize);
                        imagePainter.setFont(font);
                        imagePainter.setPen(Qt::black);
                        imagePainter.drawText(imageRect.adjusted(6.0, 4.0, -6.0, -4.0),
                                              Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                                              annotation.text);
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
                    const double pointSize = std::clamp(imageRect.height() * 0.16, 8.0, 18.0);
                    font.setPointSizeF(pointSize);
                    imagePainter.setFont(font);
                    imagePainter.setPen(Qt::black);
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

        painter.drawImage(QRectF(0.0, 0.0, writer.width(), writer.height()), pageImage);
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

        painter.drawImage(QRectF(0.0, 0.0, writer.width(), writer.height()), pageImage);
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

QString PdfDocumentController::selectedTextEditText() const
{
    return hasSelectedTextEditAnnotation() ? m_annotationModel.selectedAnnotationText() : QString();
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
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Textnotiz hinzugefügt."));
}

void PdfDocumentController::addFreeTextAt(const QPointF &imagePoint, const QString &text)
{
    if (!hasDocument() || text.trimmed().isEmpty()) {
        return;
    }

    const QPointF pagePoint = imagePointToPagePoint(imagePoint);
    const QRectF textRect(pagePoint.x(), pagePoint.y(), 180.0, 48.0);
    if (!m_annotationModel.addFreeText(m_currentPageIndex, textRect, text)) {
        return;
    }

    saveSidecarState();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Textfeld hinzugefügt."));
}

void PdfDocumentController::replaceSelectedText(const QString &text)
{
    if (!hasAreaSelection() || text.trimmed().isEmpty()) {
        return;
    }

    if (!m_annotationModel.addFreeText(m_currentPageIndex, m_lastSelectionPageRect, text)) {
        return;
    }

    saveSidecarState();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Text ersetzt."));
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
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Textnotiz aktualisiert."));
}

void PdfDocumentController::updateSelectedTextEdit(const QString &text)
{
    const QString annotationId = m_annotationModel.selectedAnnotationId();
    if (annotationId.isEmpty()) {
        return;
    }

    if (!m_annotationModel.setText(annotationId, text)) {
        return;
    }

    saveSidecarState();
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Textfeld aktualisiert."));
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
    emitOverlayState();
    emit statusMessageChanged(QStringLiteral("Formularwert aktualisiert."));
}

void PdfDocumentController::setFormFieldChecked(const QString &fieldId, bool checked)
{
    if (!m_formFieldModel.setChecked(fieldId, checked)) {
        return;
    }

    saveSidecarState();
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
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Schwärzung vorgemerkt."));
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
        overlay.text = annotation.text;
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
