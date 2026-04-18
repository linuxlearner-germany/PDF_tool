#include "document/PdfDocumentController.h"

#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>

#include "document/SelectionService.h"
#include "rendering/PdfRenderEngine.h"

void PdfDocumentController::updateTextSelection(const QRectF &imageSelectionRect)
{
    QVector<QRectF> selectionRects;
    const bool hasSelection = m_selectionService->updateTextSelection(
        *m_renderEngine,
        m_currentPageImage,
        m_currentPageIndex,
        imageSelectionRect,
        [this](const QRectF &rect) { return imageRectToPageRect(rect); },
        &selectionRects);
    if (selectionRects.isEmpty()) {
        clearSelectionInternal(true);
        return;
    }

    emit selectionHighlightChanged(selectionRects);
    emit textSelectionChanged(hasSelection);
}

void PdfDocumentController::clearTextSelection()
{
    clearSelectionInternal(true);
}

void PdfDocumentController::copySelectedText()
{
    const QString text = m_selectionService->selectedText();
    if (text.isEmpty()) {
        return;
    }

    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text, QClipboard::Clipboard);
        clipboard->setText(text, QClipboard::Selection);
    }

    emit statusMessageChanged(QStringLiteral("Text in die Zwischenablage kopiert."));
}

void PdfDocumentController::selectOverlayAt(const QPointF &imagePoint)
{
    if (!hasDocument()) {
        return;
    }

    m_selectionService->selectOverlayAt(
        m_currentPageIndex,
        imagePoint,
        m_annotationModel,
        m_redactionModel,
        [this](const QPointF &point) { return imagePointToPagePoint(point); });
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
    if (!m_selectionService->addRedactionFromSelection(m_currentPageIndex, m_redactionModel)) {
        return;
    }

    saveSidecarState();
    recordHistorySnapshot();
    emitOverlayState();
    updateOverlaySelectionSignal();
    emit statusMessageChanged(QStringLiteral("Schwärzung vorgemerkt."));
}
