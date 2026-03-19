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

    bool remove(const QString &annotationId);
    bool setColor(const QString &annotationId, const QColor &color);
    bool setText(const QString &annotationId, const QString &text);
    void clearSelection();
    bool selectAt(int pageIndex, const QPointF &pagePoint);
    QString selectedAnnotationId() const;
    bool hasSelectedAnnotation() const;
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
