#pragma once

#include <functional>

#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include "document/PdfDocumentTypes.h"
#include "document/SelectionModel.h"

class AnnotationModel;
class FormFieldModel;
class PdfRenderEngine;
class RedactionModel;

class SelectionService
{
public:
    using ImageRectToPageRect = std::function<QRectF(const QRectF &)>;
    using ImagePointToPagePoint = std::function<QPointF(const QPointF &)>;

    void reset();

    bool hasTextSelection() const;
    bool hasAreaSelection() const;
    QString selectedText() const;
    QRectF selectionPageRect() const;
    const SelectionModel &selectionModel() const;

    bool updateTextSelection(
        PdfRenderEngine &renderEngine,
        const QImage &currentPageImage,
        int currentPageIndex,
        const QRectF &imageSelectionRect,
        const ImageRectToPageRect &imageRectToPageRect,
        QVector<QRectF> *selectionHighlightImageRects);
    void clearTextSelection();

    bool selectOverlayAt(
        int currentPageIndex,
        const QPointF &imagePoint,
        AnnotationModel &annotationModel,
        RedactionModel &redactionModel,
        const ImagePointToPagePoint &imagePointToPagePoint);

    bool addRedactionFromSelection(int currentPageIndex, RedactionModel &redactionModel);

    bool textFormFieldStyleAt(
        int currentPageIndex,
        const QPointF &imagePoint,
        const FormFieldModel &formFieldModel,
        const ImagePointToPagePoint &imagePointToPagePoint,
        QString &fieldId,
        QString &label,
        PdfTextStyle &style) const;

private:
    SelectionModel m_selectionModel;
    QRectF m_lastSelectionPageRect;
};
