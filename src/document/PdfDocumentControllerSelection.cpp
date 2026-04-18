#include "document/PdfDocumentController.h"

#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>

#include "rendering/PdfRenderEngine.h"

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
