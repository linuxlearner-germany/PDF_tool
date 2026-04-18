#include "document/PdfDocumentController.h"

#include "document/DocumentStateStore.h"

namespace
{
constexpr int kSidecarVersion = 3;
}

QString PdfDocumentController::annotationSidecarPath() const
{
    return DocumentStateStore::sidecarPathForDocument(m_documentPath);
}

void PdfDocumentController::loadSidecarState()
{
    const QString sidecarPath = annotationSidecarPath();
    if (sidecarPath.isEmpty()) {
        return;
    }

    QJsonObject root;
    QString errorMessage;
    if (!DocumentStateStore::loadFromFile(sidecarPath, root, &errorMessage)) {
        emit statusMessageChanged(errorMessage);
        return;
    }

    if (root.isEmpty()) {
        return;
    }

    if (!DocumentStateStore::applyState(root, m_annotationModel, m_formFieldModel, m_redactionModel, &errorMessage)) {
        emit statusMessageChanged(errorMessage);
    }
}

void PdfDocumentController::saveSidecarState() const
{
    const QString sidecarPath = annotationSidecarPath();
    if (sidecarPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    DocumentStateStore::saveToFile(sidecarPath, currentDocumentState(), &errorMessage);
}

QJsonObject PdfDocumentController::currentDocumentState() const
{
    return DocumentStateStore::buildState(m_annotationModel, m_formFieldModel, m_redactionModel, kSidecarVersion);
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
    const QString status = QStringLiteral("Treffer %1/%2 fuer \"%3\"")
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
