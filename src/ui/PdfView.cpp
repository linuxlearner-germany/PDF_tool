#include "ui/PdfView.h"

#include <algorithm>

#include <QCheckBox>
#include <QContextMenuEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScrollBar>
#include <QWheelEvent>

PdfView::PdfView(QWidget *parent)
    : QGraphicsView(parent)
    , m_pixmapItem(m_scene.addPixmap(QPixmap()))
    , m_selectionRectItem(m_scene.addRect(QRectF(), QPen(QColor(0, 120, 215), 1.5), QBrush(QColor(0, 120, 215, 50))))
{
    setScene(&m_scene);
    setAlignment(Qt::AlignCenter);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setBackgroundBrush(QColor(36, 36, 36));
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setFocusPolicy(Qt::StrongFocus);

    m_selectionRectItem->setVisible(false);
    m_selectionRectItem->setZValue(8.0);
}

void PdfView::setPageImage(const QImage &image)
{
    m_pixmapItem->setPixmap(QPixmap::fromImage(image));
    const QRectF pageRect = m_pixmapItem->boundingRect();
    constexpr qreal kHorizontalGap = 24.0;
    constexpr qreal kVerticalGap = 72.0;
    m_scene.setSceneRect(pageRect.adjusted(-kHorizontalGap, -kVerticalGap, kHorizontalGap, kVerticalGap));

    switch (m_pendingScrollTarget) {
    case PendingScrollTarget::Bottom:
        horizontalScrollBar()->setValue((horizontalScrollBar()->minimum() + horizontalScrollBar()->maximum()) / 2);
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        break;
    case PendingScrollTarget::Top:
    case PendingScrollTarget::Default:
        horizontalScrollBar()->setValue((horizontalScrollBar()->minimum() + horizontalScrollBar()->maximum()) / 2);
        verticalScrollBar()->setValue(verticalScrollBar()->minimum());
        break;
    }
    m_pendingScrollTarget = PendingScrollTarget::Default;

    clearDragSelectionOverlay();
    clearRectItems(m_selectionHighlightItems);
    clearRectItems(m_searchHighlightItems);
    clearRectItems(m_currentSearchHighlightItems);
    clearGenericItems(m_annotationItems);
    clearFormWidgets();
    clearRectItems(m_redactionItems);
}

void PdfView::setSelectionHighlights(const QVector<QRectF> &imageRects)
{
    clearRectItems(m_selectionHighlightItems);

    for (const QRectF &imageRect : imageRects) {
        auto *item = m_scene.addRect(imageRect, QPen(Qt::NoPen), QBrush(QColor(255, 235, 59, 120)));
        item->setZValue(5.0);
        m_selectionHighlightItems.append(item);
    }
}

void PdfView::setSearchHighlights(const QVector<QRectF> &imageRects, const QVector<QRectF> &currentImageRects)
{
    clearRectItems(m_searchHighlightItems);
    clearRectItems(m_currentSearchHighlightItems);

    for (const QRectF &imageRect : imageRects) {
        auto *item = m_scene.addRect(imageRect, QPen(QColor(255, 152, 0), 1.0), QBrush(QColor(255, 193, 7, 70)));
        item->setZValue(3.0);
        m_searchHighlightItems.append(item);
    }

    for (const QRectF &imageRect : currentImageRects) {
        auto *item = m_scene.addRect(imageRect, QPen(QColor(216, 67, 21), 1.5), QBrush(QColor(255, 87, 34, 120)));
        item->setZValue(4.0);
        m_currentSearchHighlightItems.append(item);
    }
}

void PdfView::setAnnotationOverlays(const QVector<PdfAnnotationOverlay> &overlays)
{
    clearGenericItems(m_annotationItems);

    for (const PdfAnnotationOverlay &overlay : overlays) {
        switch (overlay.kind) {
        case PdfAnnotationKind::Highlight:
            for (const QRectF &rect : overlay.imageRects) {
                auto *item = m_scene.addRect(rect, QPen(Qt::NoPen), QBrush(overlay.color));
                item->setZValue(2.0);
                m_annotationItems.append(item);
            }
            break;
        case PdfAnnotationKind::Rectangle:
            for (const QRectF &rect : overlay.imageRects) {
                QPen pen(overlay.selected ? QColor(13, 71, 161) : overlay.color, overlay.selected ? 2.5 : 1.8);
                auto *item = m_scene.addRect(rect, pen, Qt::NoBrush);
                item->setZValue(6.0);
                m_annotationItems.append(item);
            }
            break;
        case PdfAnnotationKind::Note:
            for (const QRectF &rect : overlay.imageRects) {
                auto *item = m_scene.addRect(rect, QPen(QColor(121, 85, 72), overlay.selected ? 2.0 : 1.0),
                                             QBrush(overlay.selected ? QColor(255, 214, 0) : overlay.color));
                item->setToolTip(overlay.text);
                item->setZValue(6.5);
                m_annotationItems.append(item);

                auto *textItem = m_scene.addSimpleText(QStringLiteral("N"));
                textItem->setBrush(Qt::black);
                textItem->setPos(rect.topLeft() + QPointF(4.0, 1.0));
                textItem->setZValue(7.0);
                m_annotationItems.append(textItem);
            }
            break;
        case PdfAnnotationKind::FreeText:
            for (const QRectF &rect : overlay.imageRects) {
                QPen pen(overlay.selected ? QColor(21, 101, 192) : QColor(120, 120, 120),
                         overlay.selected ? 2.0 : 1.0);
                auto *item = m_scene.addRect(rect, pen, QBrush(overlay.color));
                item->setZValue(6.2);
                m_annotationItems.append(item);

                auto *textItem = m_scene.addText(overlay.text);
                textItem->setDefaultTextColor(Qt::black);
                textItem->setTextWidth(std::max(24.0, rect.width() - 10.0));
                textItem->setPos(rect.topLeft() + QPointF(5.0, 3.0));
                textItem->setZValue(6.3);
                m_annotationItems.append(textItem);
            }
            break;
        }
    }
}

void PdfView::setFormFieldOverlays(const QVector<PdfFormFieldOverlay> &overlays)
{
    clearFormWidgets();

    for (const PdfFormFieldOverlay &overlay : overlays) {
        if (!overlay.imageRect.isValid() || overlay.imageRect.isEmpty()) {
            continue;
        }

        switch (overlay.kind) {
        case PdfFormFieldKind::Text: {
            auto *lineEdit = new QLineEdit();
            lineEdit->setText(overlay.textValue);
            lineEdit->setReadOnly(overlay.readOnly);
            lineEdit->setPlaceholderText(overlay.label);
            lineEdit->setStyleSheet(QStringLiteral(
                "QLineEdit { background: rgba(255,255,255,210); border: 1px solid #1976d2; padding: 2px; }"));

            connect(lineEdit, &QLineEdit::editingFinished, this, [this, lineEdit, fieldId = overlay.id]() {
                emit formTextEdited(fieldId, lineEdit->text());
            });

            QGraphicsProxyWidget *proxy = m_scene.addWidget(lineEdit);
            proxy->setPos(overlay.imageRect.topLeft());
            proxy->setMinimumSize(overlay.imageRect.size());
            proxy->setMaximumSize(overlay.imageRect.size());
            proxy->setZValue(7.5);
            m_formFieldWidgets.append(proxy);
            break;
        }
        case PdfFormFieldKind::CheckBox: {
            auto *checkBox = new QCheckBox(overlay.label);
            checkBox->setChecked(overlay.checked);
            checkBox->setEnabled(!overlay.readOnly);
            checkBox->setStyleSheet(QStringLiteral("QCheckBox { background: rgba(255,255,255,180); padding: 2px; }"));

            connect(checkBox, &QCheckBox::toggled, this, [this, fieldId = overlay.id](bool checked) {
                emit formCheckToggled(fieldId, checked);
            });

            QGraphicsProxyWidget *proxy = m_scene.addWidget(checkBox);
            proxy->setPos(overlay.imageRect.topLeft());
            proxy->setMinimumSize(overlay.imageRect.size());
            proxy->setMaximumSize(overlay.imageRect.size());
            proxy->setZValue(7.5);
            m_formFieldWidgets.append(proxy);
            break;
        }
        }
    }
}

void PdfView::setRedactionOverlays(const QVector<PdfRedactionOverlay> &overlays)
{
    clearRectItems(m_redactionItems);

    for (const PdfRedactionOverlay &overlay : overlays) {
        QPen pen(overlay.selected ? QColor(183, 28, 28) : QColor(66, 66, 66), overlay.selected ? 2.0 : 1.0);
        auto *item = m_scene.addRect(overlay.imageRect, pen, QBrush(QColor(0, 0, 0, 220)));
        item->setZValue(6.8);
        m_redactionItems.append(item);
    }
}

void PdfView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_pixmapItem->pixmap().isNull()) {
        if (QGraphicsItem *item = itemAt(event->position().toPoint())) {
            if (qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
                QGraphicsView::mousePressEvent(event);
                return;
            }
        }

        setFocus();
        m_isSelecting = true;
        setDragMode(QGraphicsView::NoDrag);
        clearDragSelectionOverlay();

        const QPointF scenePoint = mapToScene(event->position().toPoint());
        m_selectionStart = clampToPage(scenePoint);
        m_selectionCurrent = m_selectionStart;
        m_selectionRectItem->setRect(currentSelectionRect());
        m_selectionRectItem->setVisible(true);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void PdfView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isSelecting) {
        m_selectionCurrent = clampToPage(mapToScene(event->position().toPoint()));
        m_selectionRectItem->setRect(currentSelectionRect());
        emit selectionRectChanged(currentSelectionRect());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void PdfView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isSelecting && event->button() == Qt::LeftButton) {
        m_selectionCurrent = clampToPage(mapToScene(event->position().toPoint()));
        m_isSelecting = false;
        setDragMode(QGraphicsView::ScrollHandDrag);

        const QRectF selectionRect = currentSelectionRect();
        clearDragSelectionOverlay();

        if (selectionRect.width() < 2.0 || selectionRect.height() < 2.0) {
            emit pointActivated(selectionRect.center());
            emit selectionCleared();
        } else {
            emit selectionRectChanged(selectionRect);
        }

        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void PdfView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        emit copyRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete) {
        emit deleteRequested();
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void PdfView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_pixmapItem->pixmap().isNull()) {
        QGraphicsView::contextMenuEvent(event);
        return;
    }

    const QPointF scenePoint = clampToPage(mapToScene(event->pos()));
    emit contextMenuRequested(scenePoint, event->globalPos());
    event->accept();
}

void PdfView::wheelEvent(QWheelEvent *event)
{
    if (m_pixmapItem->pixmap().isNull()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const int deltaY = event->angleDelta().y();
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (deltaY > 0) {
            emit zoomInRequested();
        } else if (deltaY < 0) {
            emit zoomOutRequested();
        }
        event->accept();
        return;
    }

    if (deltaY < 0 && verticalScrollBar()->value() >= verticalScrollBar()->maximum()) {
        m_pendingScrollTarget = PendingScrollTarget::Top;
        emit nextPageRequested();
        event->accept();
        return;
    }

    if (deltaY > 0 && verticalScrollBar()->value() <= verticalScrollBar()->minimum()) {
        m_pendingScrollTarget = PendingScrollTarget::Bottom;
        emit previousPageRequested();
        event->accept();
        return;
    }

    QGraphicsView::wheelEvent(event);
}

QPointF PdfView::clampToPage(const QPointF &scenePoint) const
{
    const QRectF pageRect = m_pixmapItem->boundingRect();
    return QPointF(std::clamp(scenePoint.x(), pageRect.left(), pageRect.right()),
                   std::clamp(scenePoint.y(), pageRect.top(), pageRect.bottom()));
}

QRectF PdfView::currentSelectionRect() const
{
    return QRectF(m_selectionStart, m_selectionCurrent).normalized();
}

void PdfView::clearDragSelectionOverlay()
{
    m_selectionRectItem->setVisible(false);
    m_selectionRectItem->setRect(QRectF());
}

void PdfView::clearRectItems(QVector<QGraphicsRectItem *> &items)
{
    qDeleteAll(items);
    items.clear();
}

void PdfView::clearGenericItems(QVector<QGraphicsItem *> &items)
{
    qDeleteAll(items);
    items.clear();
}

void PdfView::clearFormWidgets()
{
    qDeleteAll(m_formFieldWidgets);
    m_formFieldWidgets.clear();
}
