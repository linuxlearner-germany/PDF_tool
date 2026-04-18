#include "document/SelectionService.h"

#include "document/AnnotationModel.h"
#include "document/FormFieldModel.h"
#include "document/RedactionModel.h"
#include "rendering/PdfRenderEngine.h"

void SelectionService::reset()
{
    m_selectionModel.clear();
    m_lastSelectionPageRect = {};
}

bool SelectionService::hasTextSelection() const
{
    return !m_selectionModel.isEmpty();
}

bool SelectionService::hasAreaSelection() const
{
    return !m_lastSelectionPageRect.isEmpty();
}

QString SelectionService::selectedText() const
{
    return m_selectionModel.plainText();
}

QRectF SelectionService::selectionPageRect() const
{
    return m_lastSelectionPageRect;
}

const SelectionModel &SelectionService::selectionModel() const
{
    return m_selectionModel;
}

bool SelectionService::updateTextSelection(
    PdfRenderEngine &renderEngine,
    const QImage &currentPageImage,
    int currentPageIndex,
    const QRectF &imageSelectionRect,
    const ImageRectToPageRect &imageRectToPageRect,
    QVector<QRectF> *selectionHighlightImageRects)
{
    if (selectionHighlightImageRects) {
        selectionHighlightImageRects->clear();
    }

    if (currentPageImage.isNull()) {
        reset();
        return false;
    }

    const QRectF normalizedImageRect = imageSelectionRect.normalized()
        .intersected(QRectF(QPointF(0.0, 0.0), QSizeF(currentPageImage.size())));

    if (normalizedImageRect.width() < 2.0 || normalizedImageRect.height() < 2.0) {
        reset();
        return false;
    }

    m_lastSelectionPageRect = imageRectToPageRect ? imageRectToPageRect(normalizedImageRect) : QRectF();
    if (m_lastSelectionPageRect.isEmpty()) {
        reset();
        return false;
    }

    if (selectionHighlightImageRects) {
        selectionHighlightImageRects->append(normalizedImageRect);
    }

    const PdfTextSelection selection = renderEngine.buildTextSelection(currentPageIndex, m_lastSelectionPageRect);
    if (selection.isEmpty() || selection.toPlainText().isEmpty()) {
        m_selectionModel.clear();
        return false;
    }

    m_selectionModel.setSelection(currentPageIndex, selection);
    return true;
}

void SelectionService::clearTextSelection()
{
    reset();
}

bool SelectionService::selectOverlayAt(
    int currentPageIndex,
    const QPointF &imagePoint,
    AnnotationModel &annotationModel,
    RedactionModel &redactionModel,
    const ImagePointToPagePoint &imagePointToPagePoint)
{
    const QPointF pagePoint = imagePointToPagePoint ? imagePointToPagePoint(imagePoint) : QPointF();
    const bool annotationSelected = annotationModel.selectAt(currentPageIndex, pagePoint);
    const bool redactionSelected = !annotationSelected && redactionModel.selectAt(currentPageIndex, pagePoint);
    if (!annotationSelected) {
        annotationModel.clearSelection();
    }
    if (!redactionSelected) {
        redactionModel.clearSelection();
    }

    return annotationSelected || redactionSelected;
}

bool SelectionService::addRedactionFromSelection(int currentPageIndex, RedactionModel &redactionModel)
{
    if (!hasAreaSelection()) {
        return false;
    }

    return redactionModel.add(currentPageIndex, m_lastSelectionPageRect);
}

bool SelectionService::textFormFieldStyleAt(
    int currentPageIndex,
    const QPointF &imagePoint,
    const FormFieldModel &formFieldModel,
    const ImagePointToPagePoint &imagePointToPagePoint,
    QString &fieldId,
    QString &label,
    PdfTextStyle &style) const
{
    const QString id = formFieldModel.fieldIdAt(
        currentPageIndex,
        imagePointToPagePoint ? imagePointToPagePoint(imagePoint) : QPointF());
    if (id.isEmpty()) {
        return false;
    }

    PdfTextStyle fieldStyle;
    if (!formFieldModel.textFieldStyle(id, fieldStyle)) {
        return false;
    }

    for (const PdfFormField &field : formFieldModel.fieldsForPage(currentPageIndex)) {
        if (field.id == id && field.kind == PdfFormFieldKind::Text) {
            fieldId = id;
            label = field.label;
            style = fieldStyle;
            return true;
        }
    }

    return false;
}
