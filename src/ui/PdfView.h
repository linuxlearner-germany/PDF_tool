#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include "document/PdfDocumentTypes.h"

class QContextMenuEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class PdfView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PdfView(QWidget *parent = nullptr);

public slots:
    void setDarkMode(bool enabled);
    void setPageImage(const QImage &image);
    void setSelectionHighlights(const QVector<QRectF> &imageRects);
    void setSearchHighlights(const QVector<QRectF> &imageRects, const QVector<QRectF> &currentImageRects);
    void setAnnotationOverlays(const QVector<PdfAnnotationOverlay> &overlays);
    void setFormFieldOverlays(const QVector<PdfFormFieldOverlay> &overlays);
    void setRedactionOverlays(const QVector<PdfRedactionOverlay> &overlays);

signals:
    void selectionRectChanged(const QRectF &imageRect);
    void selectionCleared();
    void copyRequested();
    void deleteRequested();
    void nextPageRequested();
    void previousPageRequested();
    void zoomInRequested();
    void zoomOutRequested();
    void pointActivated(const QPointF &imagePoint);
    void contextMenuRequested(const QPointF &imagePoint, const QPoint &globalPos);
    void formTextEdited(const QString &fieldId, const QString &text);
    void formCheckToggled(const QString &fieldId, bool checked);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPointF clampToPage(const QPointF &scenePoint) const;
    QRectF currentSelectionRect() const;
    void clearDragSelectionOverlay();
    void clearRectItems(QVector<QGraphicsRectItem *> &items);
    void clearGenericItems(QVector<QGraphicsItem *> &items);
    void clearFormWidgets();

    enum class PendingScrollTarget
    {
        Default,
        Top,
        Bottom
    };

    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_pixmapItem {nullptr};
    QGraphicsRectItem *m_selectionRectItem {nullptr};
    QVector<QGraphicsRectItem *> m_selectionHighlightItems;
    QVector<QGraphicsRectItem *> m_searchHighlightItems;
    QVector<QGraphicsRectItem *> m_currentSearchHighlightItems;
    QVector<QGraphicsItem *> m_annotationItems;
    QVector<QGraphicsProxyWidget *> m_formFieldWidgets;
    QVector<QGraphicsRectItem *> m_redactionItems;
    QPointF m_selectionStart;
    QPointF m_selectionCurrent;
    bool m_isSelecting {false};
    PendingScrollTarget m_pendingScrollTarget {PendingScrollTarget::Top};
};
