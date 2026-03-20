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

namespace
{
constexpr qreal kTextResizeHandleSize = 12.0;

QRectF resizeHandleRectFor(const QRectF &rect)
{
    return QRectF(rect.right() - kTextResizeHandleSize,
                  rect.bottom() - kTextResizeHandleSize,
                  kTextResizeHandleSize,
                  kTextResizeHandleSize);
}
}

PdfView::PdfView(QWidget *parent)
    : QGraphicsView(parent)
    , m_pixmapItem(m_scene.addPixmap(QPixmap()))
    , m_emptyStateItem(m_scene.addText(QStringLiteral(
          "PDFTool\n\n"
          "Datei > Oeffnen oder Strg+O\n"
          "zum Laden eines PDFs\n\n"
          "Hinweis: Die Arbeitsflaeche zeigt Seiten,"
          "\nSuche, Annotationen und Exporte.")))
    , m_selectionRectItem(m_scene.addRect(QRectF(), QPen(QColor(15, 108, 115), 1.6, Qt::DashLine), QBrush(QColor(15, 108, 115, 45))))
{
    setScene(&m_scene);
    setAlignment(Qt::AlignCenter);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setBackgroundBrush(QColor(228, 223, 213));
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setFocusPolicy(Qt::StrongFocus);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setStyleSheet(QStringLiteral(
        "QGraphicsView {"
        "  border: none;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                              stop:0 #ece5d8, stop:1 #d8d1c2);"
        "}"));

    QFont emptyStateFont;
    emptyStateFont.setPointSize(16);
    emptyStateFont.setBold(true);
    m_emptyStateItem->setFont(emptyStateFont);
    m_emptyStateItem->setDefaultTextColor(QColor(88, 98, 104));
    m_emptyStateItem->setZValue(1.0);
    m_emptyStateItem->setTextWidth(440.0);
    m_emptyStateItem->setPos(-220.0, -70.0);

    m_selectionRectItem->setVisible(false);
    m_selectionRectItem->setZValue(8.0);
}

void PdfView::setDarkMode(bool enabled)
{
    if (enabled) {
        setBackgroundBrush(QColor(28, 33, 39));
        setStyleSheet(QStringLiteral(
            "QGraphicsView {"
            "  border: none;"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "                              stop:0 #1d2329, stop:1 #11161b);"
            "}"));
    } else {
        setBackgroundBrush(QColor(228, 223, 213));
        setStyleSheet(QStringLiteral(
            "QGraphicsView {"
            "  border: none;"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "                              stop:0 #ece5d8, stop:1 #d8d1c2);"
            "}"));
    }
}

void PdfView::setPageImage(const QImage &image)
{
    m_pixmapItem->setPixmap(QPixmap::fromImage(image));
    if (image.isNull()) {
        m_scene.setSceneRect(QRectF(-320.0, -180.0, 640.0, 360.0));
        m_emptyStateItem->setVisible(true);
        clearDragSelectionOverlay();
        clearRectItems(m_selectionHighlightItems);
        clearRectItems(m_searchHighlightItems);
        clearRectItems(m_currentSearchHighlightItems);
        clearGenericItems(m_annotationItems);
        clearFormWidgets();
        clearRectItems(m_redactionItems);
        return;
    }

    m_emptyStateItem->setVisible(false);
    const QRectF pageRect = m_pixmapItem->boundingRect();
    constexpr qreal kHorizontalGap = 24.0;
    constexpr qreal kVerticalGap = 72.0;
    m_scene.setSceneRect(pageRect.adjusted(-kHorizontalGap, -kVerticalGap, kHorizontalGap, kVerticalGap));
    m_pixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

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
        auto *item = m_scene.addRect(imageRect, QPen(QColor(226, 165, 34, 110), 1.0), QBrush(QColor(255, 222, 89, 95)));
        item->setZValue(5.0);
        m_selectionHighlightItems.append(item);
    }
}

void PdfView::setSearchHighlights(const QVector<QRectF> &imageRects, const QVector<QRectF> &currentImageRects)
{
    clearRectItems(m_searchHighlightItems);
    clearRectItems(m_currentSearchHighlightItems);

    for (const QRectF &imageRect : imageRects) {
        auto *item = m_scene.addRect(imageRect, QPen(QColor(181, 115, 20), 1.0), QBrush(QColor(224, 173, 67, 65)));
        item->setZValue(3.0);
        m_searchHighlightItems.append(item);
    }

    for (const QRectF &imageRect : currentImageRects) {
        auto *item = m_scene.addRect(imageRect, QPen(QColor(168, 64, 34), 1.5), QBrush(QColor(212, 112, 58, 110)));
        item->setZValue(4.0);
        m_currentSearchHighlightItems.append(item);
    }
}

void PdfView::setAnnotationOverlays(const QVector<PdfAnnotationOverlay> &overlays)
{
    m_annotationOverlays = overlays;
    redrawAnnotationOverlays();
}

void PdfView::redrawAnnotationOverlays()
{
    clearGenericItems(m_annotationItems);

    for (const PdfAnnotationOverlay &overlay : m_annotationOverlays) {
        switch (overlay.kind) {
        case PdfAnnotationKind::Highlight:
            for (const QRectF &rawRect : overlay.imageRects) {
                const QRectF rect = previewRectForSelectedOverlay(overlay, rawRect);
                auto *item = m_scene.addRect(rect, QPen(Qt::NoPen), QBrush(overlay.color));
                item->setZValue(2.0);
                m_annotationItems.append(item);
            }
            break;
        case PdfAnnotationKind::Rectangle:
            for (const QRectF &rawRect : overlay.imageRects) {
                const QRectF rect = previewRectForSelectedOverlay(overlay, rawRect);
                QPen pen(overlay.selected ? QColor(13, 71, 161) : overlay.color, overlay.selected ? 2.5 : 1.8);
                auto *item = m_scene.addRect(rect, pen, Qt::NoBrush);
                item->setZValue(6.0);
                m_annotationItems.append(item);
            }
            break;
        case PdfAnnotationKind::Note:
            for (const QRectF &rawRect : overlay.imageRects) {
                const QRectF rect = previewRectForSelectedOverlay(overlay, rawRect);
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
            for (const QRectF &rawRect : overlay.imageRects) {
                const QRectF rect = previewRectForSelectedOverlay(overlay, rawRect);
                QPen pen(overlay.selected ? QColor(21, 101, 192) : QColor(120, 120, 120),
                         overlay.selected ? 2.0 : 1.0);
                auto *item = m_scene.addRect(rect, pen, QBrush(overlay.color));
                item->setZValue(6.2);
                m_annotationItems.append(item);

                auto *textItem = m_scene.addText(overlay.text);
                QFont textFont(overlay.textStyle.fontFamily);
                textFont.setPointSizeF(std::max(1.0, overlay.textStyle.fontSize));
                textItem->setFont(textFont);
                textItem->setDefaultTextColor(
                    overlay.textStyle.textColor.isValid() ? overlay.textStyle.textColor : Qt::black);
                textItem->setTextWidth(std::max(24.0, rect.width() - 10.0));
                textItem->setPos(rect.topLeft() + QPointF(5.0, 3.0));
                textItem->setZValue(6.3);
                m_annotationItems.append(textItem);

                if (overlay.selected) {
                    auto *handleItem = m_scene.addRect(
                        resizeHandleRectFor(rect),
                        QPen(QColor(21, 101, 192), 1.0),
                        QBrush(QColor(21, 101, 192)));
                    handleItem->setZValue(6.4);
                    m_annotationItems.append(handleItem);
                }
            }
            break;
        case PdfAnnotationKind::Signature:
            for (const QRectF &rawRect : overlay.imageRects) {
                const QRectF rect = previewRectForSelectedOverlay(overlay, rawRect);
                auto *item = m_scene.addRect(
                    rect,
                    QPen(overlay.selected ? QColor(15, 108, 115) : QColor(110, 110, 110),
                         overlay.selected ? 2.0 : 1.0,
                         overlay.selected ? Qt::DashLine : Qt::SolidLine),
                    Qt::NoBrush);
                item->setZValue(6.2);
                m_annotationItems.append(item);

                QPixmap signaturePixmap;
                signaturePixmap.loadFromData(overlay.binaryPayload);
                if (!signaturePixmap.isNull()) {
                    auto *pixmapItem = m_scene.addPixmap(signaturePixmap.scaled(
                        rect.size().toSize(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation));
                    pixmapItem->setPos(rect.topLeft());
                    pixmapItem->setZValue(6.1);
                    m_annotationItems.append(pixmapItem);
                }

                if (overlay.selected) {
                    auto *handleItem = m_scene.addRect(
                        resizeHandleRectFor(rect),
                        QPen(QColor(15, 108, 115), 1.0),
                        QBrush(QColor(15, 108, 115)));
                    handleItem->setZValue(6.4);
                    m_annotationItems.append(handleItem);
                }
            }
            break;
        }
    }
}

QRectF PdfView::previewRectForSelectedOverlay(const PdfAnnotationOverlay &overlay, const QRectF &rect) const
{
    if (!overlay.selected) {
        return rect;
    }

    if (m_isDraggingSignature
        && (overlay.kind == PdfAnnotationKind::Signature || overlay.kind == PdfAnnotationKind::FreeText)) {
        return QRectF(rect.translated(m_dragCurrent - m_dragStart)).normalized();
    }

    if (m_isResizingTextEdit && overlay.kind == PdfAnnotationKind::FreeText) {
        QRectF previewRect = rect;
        previewRect.setSize(QSizeF(qMax(48.0, rect.width() + m_dragCurrent.x() - m_dragStart.x()),
                                   qMax(24.0, rect.height() + m_dragCurrent.y() - m_dragStart.y())));
        return previewRect.normalized();
    }

    if (m_isResizingSignature && overlay.kind == PdfAnnotationKind::Signature) {
        const double currentWidth = qMax(1.0, rect.width());
        const double currentHeight = qMax(1.0, rect.height());
        const double aspectRatio = currentWidth / currentHeight;
        const double deltaX = m_dragCurrent.x() - m_dragStart.x();
        const double deltaY = m_dragCurrent.y() - m_dragStart.y();
        const double requestedWidth = qMax(32.0, currentWidth + deltaX);
        const double requestedHeight = qMax(16.0, currentHeight + deltaY);

        QSizeF newSize;
        const double normalizedDeltaX = std::abs(deltaX) / currentWidth;
        const double normalizedDeltaY = std::abs(deltaY) / currentHeight;
        if (normalizedDeltaX >= normalizedDeltaY) {
            newSize.setWidth(requestedWidth);
            newSize.setHeight(qMax(16.0, requestedWidth / aspectRatio));
        } else {
            newSize.setHeight(requestedHeight);
            newSize.setWidth(qMax(32.0, requestedHeight * aspectRatio));
        }

        QRectF previewRect = rect;
        previewRect.setSize(newSize);
        return previewRect.normalized();
    }

    return rect;
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
            QFont font = lineEdit->font();
            font.setFamily(overlay.textStyle.fontFamily);
            font.setPointSizeF(std::max(1.0, overlay.textStyle.fontSize));
            lineEdit->setFont(font);
            lineEdit->setStyleSheet(QStringLiteral(
                "QLineEdit { background: rgba(255,255,255,210); border: 1px solid #1976d2; padding: 2px; color: %1; }")
                                    .arg((overlay.textStyle.textColor.isValid() ? overlay.textStyle.textColor : Qt::black)
                                             .name(QColor::HexRgb)));

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

        const QPointF scenePoint = clampToPage(mapToScene(event->position().toPoint()));
        if (isSelectedSignatureResizeHandleHit(scenePoint)) {
            setFocus();
            m_isResizingSignature = true;
            m_dragStart = scenePoint;
            m_dragCurrent = scenePoint;
            redrawAnnotationOverlays();
            setCursor(Qt::SizeFDiagCursor);
            event->accept();
            return;
        }

        if (isSelectedTextResizeHandleHit(scenePoint)) {
            setFocus();
            m_isResizingTextEdit = true;
            m_dragStart = scenePoint;
            m_dragCurrent = scenePoint;
            redrawAnnotationOverlays();
            setCursor(Qt::SizeFDiagCursor);
            event->accept();
            return;
        }

        if (isSelectedMovableAnnotationHit(scenePoint)) {
            setFocus();
            m_isDraggingSignature = true;
            m_dragStart = scenePoint;
            m_dragCurrent = scenePoint;
            redrawAnnotationOverlays();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        setFocus();
        m_isSelecting = true;
        setDragMode(QGraphicsView::NoDrag);
        clearDragSelectionOverlay();

        m_selectionStart = scenePoint;
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
    if (m_isResizingSignature) {
        m_dragCurrent = clampToPage(mapToScene(event->position().toPoint()));
        redrawAnnotationOverlays();
        event->accept();
        return;
    }

    if (m_isResizingTextEdit) {
        m_dragCurrent = clampToPage(mapToScene(event->position().toPoint()));
        redrawAnnotationOverlays();
        event->accept();
        return;
    }

    if (m_isDraggingSignature) {
        m_dragCurrent = clampToPage(mapToScene(event->position().toPoint()));
        redrawAnnotationOverlays();
        event->accept();
        return;
    }

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
    if (m_isResizingSignature && event->button() == Qt::LeftButton) {
        m_dragCurrent = clampToPage(mapToScene(event->position().toPoint()));
        const QPointF delta = m_dragCurrent - m_dragStart;
        if (std::abs(delta.x()) >= 1.0 || std::abs(delta.y()) >= 1.0) {
            emit signatureResizeRequested(delta);
        }
        m_isResizingSignature = false;
        unsetCursor();
        redrawAnnotationOverlays();
        event->accept();
        return;
    }

    if (m_isResizingTextEdit && event->button() == Qt::LeftButton) {
        m_dragCurrent = clampToPage(mapToScene(event->position().toPoint()));
        const QPointF delta = m_dragCurrent - m_dragStart;
        if (std::abs(delta.x()) >= 1.0 || std::abs(delta.y()) >= 1.0) {
            emit textEditResizeRequested(delta);
        }
        m_isResizingTextEdit = false;
        unsetCursor();
        redrawAnnotationOverlays();
        event->accept();
        return;
    }

    if (m_isDraggingSignature && event->button() == Qt::LeftButton) {
        m_dragCurrent = clampToPage(mapToScene(event->position().toPoint()));
        const QPointF delta = m_dragCurrent - m_dragStart;
        if (std::abs(delta.x()) >= 1.0 || std::abs(delta.y()) >= 1.0) {
            emit signatureMoveRequested(delta);
        }
        m_isDraggingSignature = false;
        unsetCursor();
        redrawAnnotationOverlays();
        event->accept();
        return;
    }

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

QRectF PdfView::selectedSignatureResizeHandleRect() const
{
    for (const PdfAnnotationOverlay &overlay : m_annotationOverlays) {
        if (!overlay.selected || overlay.kind != PdfAnnotationKind::Signature || overlay.imageRects.isEmpty()) {
            continue;
        }
        return resizeHandleRectFor(overlay.imageRects.first());
    }

    return {};
}

QRectF PdfView::selectedTextResizeHandleRect() const
{
    for (const PdfAnnotationOverlay &overlay : m_annotationOverlays) {
        if (!overlay.selected || overlay.kind != PdfAnnotationKind::FreeText || overlay.imageRects.isEmpty()) {
            continue;
        }
        return resizeHandleRectFor(overlay.imageRects.first());
    }

    return {};
}

QPointF PdfView::clampToPage(const QPointF &scenePoint) const
{
    const QRectF pageRect = m_pixmapItem->boundingRect();
    return QPointF(std::clamp(scenePoint.x(), pageRect.left(), pageRect.right()),
                   std::clamp(scenePoint.y(), pageRect.top(), pageRect.bottom()));
}

bool PdfView::isSelectedSignatureResizeHandleHit(const QPointF &scenePoint) const
{
    const QRectF handleRect = selectedSignatureResizeHandleRect();
    return handleRect.isValid() && handleRect.contains(scenePoint);
}

bool PdfView::isSelectedTextResizeHandleHit(const QPointF &scenePoint) const
{
    const QRectF handleRect = selectedTextResizeHandleRect();
    return handleRect.isValid() && handleRect.contains(scenePoint);
}

bool PdfView::isSelectedMovableAnnotationHit(const QPointF &scenePoint) const
{
    for (const PdfAnnotationOverlay &overlay : m_annotationOverlays) {
        if (!overlay.selected) {
            continue;
        }
        if (overlay.kind != PdfAnnotationKind::Signature
            && overlay.kind != PdfAnnotationKind::FreeText) {
            continue;
        }
        for (const QRectF &rect : overlay.imageRects) {
            if ((overlay.kind == PdfAnnotationKind::FreeText || overlay.kind == PdfAnnotationKind::Signature)
                && resizeHandleRectFor(rect).contains(scenePoint)) {
                continue;
            }
            if (rect.contains(scenePoint)) {
                return true;
            }
        }
    }

    return false;
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
