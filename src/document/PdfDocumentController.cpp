#include "document/PdfDocumentController.h"

#include <algorithm>

#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>

#include <QtPrintSupport/QPrinter>

#include "document/ExportService.h"
#include "document/HistoryService.h"
#include "document/OcrServiceController.h"
#include "document/PageThumbnailProvider.h"
#include "document/SearchService.h"
#include "document/SelectionService.h"
#include "document/SidecarService.h"
#include "rendering/PdfRenderEngine.h"

namespace
{
constexpr double kMinZoom = 0.5;
constexpr double kMaxZoom = 4.0;
constexpr double kZoomStep = 0.25;
}

PdfDocumentController::PdfDocumentController(std::unique_ptr<PdfRenderEngine> renderEngine, QObject *parent)
    : QObject(parent)
    , m_renderEngine(std::move(renderEngine))
    , m_exportService(std::make_unique<ExportService>())
    , m_historyService(std::make_unique<HistoryService>())
    , m_ocrServiceController(std::make_unique<OcrServiceController>(
          OcrServiceController::RecognizeFunction(),
          OcrServiceController::CapabilityProvider(),
          this))
    , m_searchService(std::make_unique<SearchService>())
    , m_selectionService(std::make_unique<SelectionService>())
    , m_sidecarService(std::make_unique<SidecarService>())
    , m_thumbnailProvider(std::make_unique<PageThumbnailProvider>(m_renderEngine.get()))
{
    connect(m_ocrServiceController.get(), &OcrServiceController::busyStateChanged, this, &PdfDocumentController::busyStateChanged);
    connect(m_ocrServiceController.get(), &OcrServiceController::requestCompleted, this,
            [this](int, const QString &title, const QString &text) {
                emit ocrFinished(title, text);
                emit statusMessageChanged(QStringLiteral("%1 abgeschlossen.").arg(title));
            });
    connect(m_ocrServiceController.get(), &OcrServiceController::requestFailed, this,
            [this](int, const QString &errorMessage) {
                m_lastError = errorMessage.isEmpty() ? QStringLiteral("OCR hat keinen Text erkannt.") : errorMessage;
                emit errorOccurred(m_lastError);
            });
}

PdfDocumentController::~PdfDocumentController() = default;

bool PdfDocumentController::openDocument(const QString &filePath, const QByteArray &ownerPassword, const QByteArray &userPassword)
{
    m_ocrServiceController->invalidateActiveSession();
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
    m_ocrServiceController->invalidateActiveSession();
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
    return m_selectionService->hasTextSelection();
}

bool PdfDocumentController::hasAreaSelection() const
{
    return m_selectionService->hasAreaSelection();
}

QString PdfDocumentController::selectedText() const
{
    return m_selectionService->selectedText();
}

bool PdfDocumentController::isOcrAvailable() const
{
    return ocrCapabilityState().available;
}

bool PdfDocumentController::isOcrBusy() const
{
    return m_ocrServiceController->isBusy();
}

QString PdfDocumentController::ocrAvailabilityError() const
{
    return ocrCapabilityState().error;
}

CapabilityState PdfDocumentController::ocrCapabilityState() const
{
    return m_ocrServiceController->capabilityState();
}

bool PdfDocumentController::hasSearchResults() const
{
    return m_searchService->hasResults();
}

QString PdfDocumentController::searchQuery() const
{
    return m_searchService->query();
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

    if (!m_exportService->printDocument(*m_renderEngine, pageCount(), printer, m_lastError)) {
        emit errorOccurred(m_lastError);
        return false;
    }
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
    emit busyStateChanged(true, QStringLiteral("Bearbeitetes PDF wird exportiert..."));
    QString statusMessage;
    if (!m_exportService->exportEditedPdf(
            *m_renderEngine,
            m_documentPath,
            outputFile,
            m_annotationModel.annotations(),
            m_formFieldModel.fields(),
            m_baseFormFields,
            m_redactionModel.redactions(),
            m_currentOwnerPassword,
            m_currentUserPassword,
            excludedAnnotationId,
            m_lastError,
            statusMessage)) {
        emit busyStateChanged(false, QString());
        emit errorOccurred(m_lastError);
        return false;
    }

    emit busyStateChanged(false, QString());
    emit statusMessageChanged(statusMessage);
    return true;
}

bool PdfDocumentController::exportRedactedPdf(const QString &outputFile)
{
    if (!hasDocument() || outputFile.isEmpty() || !m_redactionModel.hasRedactions()) {
        return false;
    }

    emit busyStateChanged(true, QStringLiteral("Geschwärztes PDF wird exportiert..."));
    QString statusMessage;
    if (!m_exportService->exportRedactedPdf(
            *m_renderEngine,
            m_documentPath,
            outputFile,
            m_redactionModel.redactions(),
            pageCount(),
            m_currentOwnerPassword,
            m_currentUserPassword,
            m_lastError,
            statusMessage)) {
        emit busyStateChanged(false, QString());
        emit errorOccurred(m_lastError);
        return false;
    }

    emit busyStateChanged(false, QString());
    emit statusMessageChanged(statusMessage);
    return true;
}

bool PdfDocumentController::supportsNativePdfEditExport() const
{
    return exportCapabilityState().available;
}

QString PdfDocumentController::nativePdfEditExportError() const
{
    return exportCapabilityState().error;
}

CapabilityState PdfDocumentController::exportCapabilityState() const
{
    return m_exportService->capabilityState();
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

    return m_selectionService->textFormFieldStyleAt(
        m_currentPageIndex,
        imagePoint,
        m_formFieldModel,
        [this](const QPointF &point) { return imagePointToPagePoint(point); },
        fieldId,
        label,
        style);
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
    return m_historyService->canUndo();
}

bool PdfDocumentController::canRedo() const
{
    return m_historyService->canRedo();
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

void PdfDocumentController::addHighlightAnnotationFromSelection()
{
    if (!m_annotationModel.addHighlightFromSelection(m_selectionService->selectionModel())) {
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
    const QRectF selectionPageRect = m_selectionService->selectionPageRect();
    if (selectionPageRect.isEmpty()) {
        return;
    }

    if (!m_annotationModel.addRectangle(m_currentPageIndex, selectionPageRect)) {
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
    const QRectF selectionPageRect = m_selectionService->selectionPageRect();
    if (selectionPageRect.isEmpty() || text.trimmed().isEmpty()) {
        return;
    }

    if (!m_annotationModel.addFreeText(m_currentPageIndex, selectionPageRect, text, style, backgroundColor)) {
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
    const QRectF selectionPageRect = m_selectionService->selectionPageRect();
    if (!hasDocument() || signatureImage.isNull() || selectionPageRect.isEmpty()) {
        return;
    }

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    signatureImage.save(&buffer, "PNG");

    if (!m_annotationModel.addSignature(m_currentPageIndex, selectionPageRect, pngBytes)) {
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
    m_searchService->reset();
    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    emitSearchState();
    updateOverlaySelectionSignal();
    return true;
}

void PdfDocumentController::saveDocumentState()
{
    if (!hasDocument()) {
        return;
    }

    QString statusMessage;
    if (!m_exportService->saveDocumentState(
            m_documentPath,
            annotationSidecarPath(),
            [this](const QString &path, QString &errorMessage, QString &exportStatus) {
                return m_exportService->exportEditedPdf(
                    *m_renderEngine,
                    m_documentPath,
                    path,
                    m_annotationModel.annotations(),
                    m_formFieldModel.fields(),
                    m_baseFormFields,
                    m_redactionModel.redactions(),
                    m_currentOwnerPassword,
                    m_currentUserPassword,
                    QString(),
                    errorMessage,
                    exportStatus);
            },
            m_lastError,
            statusMessage)) {
        emit errorOccurred(m_lastError);
        return;
    }

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

    emit statusMessageChanged(statusMessage);
}

void PdfDocumentController::undo()
{
    QJsonObject state;
    if (!m_historyService->undo(state)) {
        return;
    }

    restoreDocumentState(state);
    saveSidecarState();
    updateHistoryState();
    emit statusMessageChanged(QStringLiteral("Letzte Änderung rückgängig gemacht."));
}

void PdfDocumentController::redo()
{
    QJsonObject state;
    if (!m_historyService->redo(state)) {
        return;
    }

    restoreDocumentState(state);
    saveSidecarState();
    updateHistoryState();
    emit statusMessageChanged(QStringLiteral("Änderung wiederhergestellt."));
}

void PdfDocumentController::runOcrOnCurrentPage()
{
    if (!hasDocument()) {
        return;
    }

    const QImage image = m_renderEngine->renderPage(m_currentPageIndex, 2.5);
    if (image.isNull()) {
        m_lastError = m_renderEngine->lastError();
        emit errorOccurred(m_lastError);
        return;
    }

    m_ocrServiceController->startRequest({ image, QStringLiteral("OCR fuer aktuelle Seite laeuft..."), QStringLiteral("OCR: Aktuelle Seite"), 3 });
}

void PdfDocumentController::runOcrOnSelection()
{
    const QRectF selectionPageRect = m_selectionService->selectionPageRect();
    if (!hasDocument() || selectionPageRect.isEmpty() || m_currentPageImage.isNull()) {
        return;
    }

    const QRectF imageRect = pageRectToImageRect(selectionPageRect)
        .intersected(QRectF(QPointF(0.0, 0.0), QSizeF(m_currentPageImage.size())));
    if (imageRect.isEmpty()) {
        return;
    }

    const QImage croppedImage = m_currentPageImage.copy(imageRect.toAlignedRect());
    if (croppedImage.isNull()) {
        m_lastError = QStringLiteral("Auswahl konnte nicht fuer OCR vorbereitet werden.");
        emit errorOccurred(m_lastError);
        return;
    }

    m_ocrServiceController->startRequest({ croppedImage, QStringLiteral("OCR fuer Auswahl laeuft..."), QStringLiteral("OCR: Auswahl"), 6 });
}
