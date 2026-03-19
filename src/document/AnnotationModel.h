#pragma once

#include <QJsonArray>
#include <QPointF>
#include <QString>
#include <QVector>

#include "document/PdfDocumentTypes.h"
#include "document/SelectionModel.h"

class AnnotationModel
{
public:
    void clear();

    bool addHighlightFromSelection(const SelectionModel &selection);
    bool addRectangle(int pageIndex, const QRectF &pageRect);
    bool addNote(int pageIndex, const QRectF &pageRect, const QString &text);
    bool addFreeText(int pageIndex, const QRectF &pageRect, const QString &text);
    bool addSignature(int pageIndex, const QRectF &pageRect, const QByteArray &imageBytes);

    bool remove(const QString &annotationId);
    bool setColor(const QString &annotationId, const QColor &color);
    bool setText(const QString &annotationId, const QString &text);
    bool translateSelected(const QPointF &pageDelta);
    bool resizeSelectedFreeText(const QSizeF &pageDelta, const QSizeF &minimumSize = QSizeF(48.0, 24.0));
    bool remapPages(const QVector<int> &newOrder);
    void clearSelection();
    bool selectAt(int pageIndex, const QPointF &pagePoint);
    QString selectedAnnotationId() const;
    bool hasSelectedAnnotation() const;
    bool hasAnnotationKind(PdfAnnotationKind kind) const;
    PdfAnnotationKind selectedAnnotationKind() const;
    QString selectedAnnotationText() const;

    QVector<PdfAnnotation> annotationsForPage(int pageIndex) const;
    QJsonArray toJson() const;
    bool fromJson(const QJsonArray &annotationsArray);

private:
    PdfAnnotation *findMutable(const QString &annotationId);
    bool hitTest(const PdfAnnotation &annotation, const QPointF &pagePoint) const;

    QVector<PdfAnnotation> m_annotations;
};
