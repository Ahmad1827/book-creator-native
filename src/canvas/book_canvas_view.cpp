#include "book_canvas_view.h"
#include <QPainter>
#include <QScrollBar>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QSvgRenderer>

BookCanvasView::BookCanvasView(QWidget *parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this)),
      m_theme(THEMES[0]) {

    m_scene->setSceneRect(0, 0, 1200, 650);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);

    setMode(CanvasMode::Draw);
}

void BookCanvasView::setTheme(const BookTheme &theme) {
    m_theme = theme;
    m_brushColor = theme.inkColor;
    viewport()->update();
}

void BookCanvasView::setMode(CanvasMode mode) {
    m_mode = mode;

    if (m_mode == CanvasMode::Pan) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        setInteractive(false);
    } else if (m_mode == CanvasMode::Select) {
        setDragMode(QGraphicsView::RubberBandDrag);
        setInteractive(true);
        for (auto *item : m_scene->items()) {
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
            item->setFlag(QGraphicsItem::ItemIsMovable, true);
        }
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setInteractive(true);
        for (auto *item : m_scene->items()) {
            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
            item->setFlag(QGraphicsItem::ItemIsMovable, false);
        }
    }
}

void BookCanvasView::setBrushColor(const QColor &color) {
    m_brushColor = color;
}

void BookCanvasView::setBrushSize(qreal size) {
    m_brushSize = size;
}

void BookCanvasView::setBrushOpacity(qreal opacity) {
    m_brushOpacity = opacity;
}

void BookCanvasView::drawBackground(QPainter *painter, const QRectF &rect) {
    Q_UNUSED(rect);

    painter->save();

    QRectF deskRect = m_scene->sceneRect().adjusted(-800, -800, 800, 800);
    QLinearGradient deskGrad(deskRect.topLeft(), deskRect.bottomLeft());
    deskGrad.setColorAt(0.0, QColor("#442617"));
    deskGrad.setColorAt(0.5, QColor("#351c0f"));
    deskGrad.setColorAt(1.0, QColor("#29150b"));
    painter->fillRect(deskRect, deskGrad);

    QRectF casingRect(-18, -14, 1236, 678);
    painter->setBrush(m_theme.spineColor);
    painter->setPen(QPen(QColor(0, 0, 0, 140), 2));
    painter->drawRoundedRect(casingRect, 10, 10);

    QRectF leftPage(0, 0, 600, 650);
    QRectF rightPage(600, 0, 600, 650);

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_theme.paperBg);
    painter->drawRect(leftPage);
    painter->drawRect(rightPage);

    QLinearGradient leftEdgeGrad(0, 0, 40, 0);
    leftEdgeGrad.setColorAt(0.0, QColor(0, 0, 0, 35));
    leftEdgeGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->fillRect(QRectF(0, 0, 40, 650), leftEdgeGrad);

    QLinearGradient rightEdgeGrad(1200, 0, 1160, 0);
    rightEdgeGrad.setColorAt(0.0, QColor(0, 0, 0, 35));
    rightEdgeGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->fillRect(QRectF(1160, 0, 40, 650), rightEdgeGrad);

    QLinearGradient gutterGrad(570, 0, 630, 0);
    gutterGrad.setColorAt(0.0, QColor(0, 0, 0, 0));
    gutterGrad.setColorAt(0.42, QColor(0, 0, 0, 60));
    gutterGrad.setColorAt(0.5, QColor(0, 0, 0, 110));
    gutterGrad.setColorAt(0.58, QColor(0, 0, 0, 60));
    gutterGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->fillRect(QRectF(570, 0, 60, 650), gutterGrad);

    painter->setPen(QPen(QColor(0, 0, 0, 100), 1));
    painter->drawLine(600, 0, 600, 650);

    painter->setPen(QPen(m_theme.borderColor, 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(leftPage.adjusted(12, 12, -12, -12));
    painter->drawRect(rightPage.adjusted(12, 12, -12, -12));

    ThemeDecors::draw(painter, m_theme);

    painter->restore();
}

void BookCanvasView::wheelEvent(QWheelEvent *event) {
    const qreal factor = 1.12;
    if (event->angleDelta().y() > 0) {
        scale(factor, factor);
    } else {
        scale(1.0 / factor, 1.0 / factor);
    }
    event->accept();
}

void BookCanvasView::mousePressEvent(QMouseEvent *event) {
    QPointF scenePos = mapToScene(event->pos());
    m_activeSide = scenePos.x() < 600 ? "left" : "right";
    emit activeSideChanged(m_activeSide);

    if (event->button() == Qt::RightButton) {
        QGraphicsItem *item = m_scene->itemAt(scenePos, transform());
        showContextMenu(event->globalPosition().toPoint(), item);
        event->accept();
        return;
    }

    if (m_mode == CanvasMode::Draw && event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        m_currentPath = QPainterPath(scenePos);

        QColor strokeColor = m_brushColor;
        strokeColor.setAlphaF(m_brushOpacity);

        QPen pen(strokeColor, m_brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        m_currentPathItem = m_scene->addPath(m_currentPath, pen);
        m_currentPathItem->setZValue(100);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);

    if (m_mode == CanvasMode::Select) {
        auto selected = m_scene->selectedItems();
        emit selectionChanged(selected.isEmpty() ? nullptr : selected.first());
    }
}

void BookCanvasView::mouseMoveEvent(QMouseEvent *event) {
    if (m_mode == CanvasMode::Draw && m_isDrawing && (event->buttons() & Qt::LeftButton)) {
        QPointF scenePos = mapToScene(event->pos());
        m_currentPath.lineTo(scenePos);
        if (m_currentPathItem) {
            m_currentPathItem->setPath(m_currentPath);
        }
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void BookCanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (m_mode == CanvasMode::Draw && event->button() == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        m_currentPathItem = nullptr;
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void BookCanvasView::showContextMenu(const QPoint &globalPos, QGraphicsItem *item) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #fbf5ea; border: 2px solid #8d6b4f; border-radius: 8px; padding: 4px; }"
        "QMenu::item { color: #3b2212; padding: 6px 20px; font-weight: bold; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #ecd9bf; color: #1a0d06; }"
        "QMenu::separator { height: 1px; background-color: #dcc6ab; margin: 4px 6px; }"
    );

    if (item) {
        m_scene->clearSelection();
        item->setSelected(true);
        emit selectionChanged(item);

        QAction *actDup = menu.addAction("Duplicate");
        QAction *actFront = menu.addAction("Bring to Front");
        QAction *actBack = menu.addAction("Send to Back");
        QAction *actLock = menu.addAction(item->flags().testFlag(QGraphicsItem::ItemIsMovable) ? "Lock Position" : "Unlock Position");
        menu.addSeparator();
        QAction *actDel = menu.addAction("Delete");

        connect(actDup, &QAction::triggered, [this, item]() { duplicateItem(item); });
        connect(actFront, &QAction::triggered, [this, item]() { bringItemToFront(item); });
        connect(actBack, &QAction::triggered, [this, item]() { sendItemToBack(item); });
        connect(actLock, &QAction::triggered, [this, item]() { toggleLockItem(item); });
        connect(actDel, &QAction::triggered, [this, item]() { deleteItem(item); });
    } else {
        QAction *actResetZoom = menu.addAction("Reset View (100%)");
        connect(actResetZoom, &QAction::triggered, [this]() {
            resetTransform();
            centerOn(600, 325);
        });
    }

    menu.exec(globalPos);
}

void BookCanvasView::duplicateSelectedItem() {
    auto selected = m_scene->selectedItems();
    if (!selected.isEmpty()) duplicateItem(selected.first());
}

void BookCanvasView::deleteSelectedItem() {
    auto selected = m_scene->selectedItems();
    if (!selected.isEmpty()) deleteItem(selected.first());
}

void BookCanvasView::bringSelectedItemToFront() {
    auto selected = m_scene->selectedItems();
    if (!selected.isEmpty()) bringItemToFront(selected.first());
}

void BookCanvasView::sendSelectedItemToBack() {
    auto selected = m_scene->selectedItems();
    if (!selected.isEmpty()) sendItemToBack(selected.first());
}

void BookCanvasView::duplicateItem(QGraphicsItem *item) {
    if (!item) return;

    if (auto *pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
        auto *copy = m_scene->addPath(pathItem->path(), pathItem->pen(), pathItem->brush());
        copy->setPos(pathItem->pos() + QPointF(20, 20));
        copy->setZValue(pathItem->zValue() + 1);
        copy->setFlags(pathItem->flags());
    } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
        auto *copy = m_scene->addRect(rectItem->rect(), rectItem->pen(), rectItem->brush());
        copy->setPos(rectItem->pos() + QPointF(20, 20));
        copy->setZValue(rectItem->zValue() + 1);
        copy->setFlags(rectItem->flags());
    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(item)) {
        auto *copy = m_scene->addEllipse(ellipseItem->rect(), ellipseItem->pen(), ellipseItem->brush());
        copy->setPos(ellipseItem->pos() + QPointF(20, 20));
        copy->setZValue(ellipseItem->zValue() + 1);
        copy->setFlags(ellipseItem->flags());
    }
}

void BookCanvasView::deleteItem(QGraphicsItem *item) {
    if (!item) return;
    m_scene->removeItem(item);
    delete item;
    emit selectionChanged(nullptr);
}

void BookCanvasView::bringItemToFront(QGraphicsItem *item) {
    if (!item) return;
    qreal maxZ = 0;
    for (auto *i : m_scene->items()) {
        if (i->zValue() > maxZ) maxZ = i->zValue();
    }
    item->setZValue(maxZ + 1);
}

void BookCanvasView::sendItemToBack(QGraphicsItem *item) {
    if (!item) return;
    qreal minZ = 0;
    for (auto *i : m_scene->items()) {
        if (i->zValue() < minZ) minZ = i->zValue();
    }
    item->setZValue(minZ - 1);
}

void BookCanvasView::toggleLockItem(QGraphicsItem *item) {
    if (!item) return;
    bool isMovable = item->flags().testFlag(QGraphicsItem::ItemIsMovable);
    item->setFlag(QGraphicsItem::ItemIsMovable, !isMovable);
}

void BookCanvasView::addStoryShape(const QString &shapeType, const QColor &fill, const QColor &stroke, qreal strokeWidth) {
    qreal posX = (m_activeSide == "left") ? 220.0 : 820.0;
    qreal posY = 220.0;

    QPen pen(stroke, strokeWidth);
    QBrush brush(fill);

    QGraphicsItem *item = nullptr;

    if (shapeType == "circle" || shapeType == "round") {
        item = m_scene->addEllipse(posX, posY, 100, 100, pen, brush);
    } else if (shapeType == "rect" || shapeType == "frame") {
        item = m_scene->addRect(posX, posY, 140, 90, pen, brush);
    } else if (shapeType == "star") {
        QPainterPath p;
        p.moveTo(posX + 50, posY + 0);
        p.lineTo(posX + 63, posY + 35);
        p.lineTo(posX + 100, posY + 35);
        p.lineTo(posX + 70, posY + 57);
        p.lineTo(posX + 81, posY + 92);
        p.lineTo(posX + 50, posY + 70);
        p.lineTo(posX + 19, posY + 92);
        p.lineTo(posX + 30, posY + 57);
        p.lineTo(posX + 0, posY + 35);
        p.lineTo(posX + 37, posY + 35);
        p.closeSubpath();
        item = m_scene->addPath(p, pen, brush);
    } else if (shapeType == "heart") {
        QPainterPath p;
        p.moveTo(posX + 70, posY + 20);
        p.cubicTo(posX + 35, posY - 15, posX + 0, posY + 40, posX + 70, posY + 90);
        p.cubicTo(posX + 140, posY + 40, posX + 105, posY - 15, posX + 70, posY + 20);
        item = m_scene->addPath(p, pen, brush);
    } else {
        item = m_scene->addRect(posX, posY, 120, 80, pen, brush);
    }

    if (item) {
        item->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
        item->setZValue(50);
        m_scene->clearSelection();
        item->setSelected(true);
        emit selectionChanged(item);
    }
}

void BookCanvasView::addStorySticker(const QString &stickerId, const QString &svgPath, const QColor &fill) {
    Q_UNUSED(stickerId);
    qreal posX = (m_activeSide == "left") ? 250.0 : 850.0;
    qreal posY = 220.0;

    QString fullSvg = QString(R"(
        <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <path d="%1" fill="%2" stroke="#2c211a" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" />
        </svg>
    )").arg(svgPath, fill.name());

    QSvgRenderer renderer(fullSvg.toUtf8());
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();

    auto *pixmapItem = m_scene->addPixmap(QPixmap::fromImage(image));
    pixmapItem->setPos(posX, posY);
    pixmapItem->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    pixmapItem->setZValue(55);

    m_scene->clearSelection();
    pixmapItem->setSelected(true);
    emit selectionChanged(pixmapItem);
}

void BookCanvasView::applyPageTemplate(const QString &side, const QString &templateType) {
    qreal pageLeft = (side == "left") ? 60.0 : 660.0;
    qreal pageWidth = 480.0;
    qreal pageCenter = (side == "left") ? 300.0 : 900.0;

    for (auto *item : m_scene->items()) {
        if (item->data(0).toString() == side && item->data(1).toBool() == true) {
            m_scene->removeItem(item);
            delete item;
        }
    }

    auto registerTemplateItem = [side](QGraphicsItem *item) {
        item->setData(0, side);
        item->setData(1, true);
        item->setZValue(1);
    };

    QPen borderPen(m_theme.borderColor, 2);
    QBrush artBrush(QColor(255, 255, 255, 115));
    QBrush cardBrush(QColor(255, 255, 255, 220));

    if (templateType == "top_art_bottom_text") {
        auto *top = m_scene->addRect(pageLeft, 50, pageWidth, 320, borderPen, artBrush);
        auto *bottom = m_scene->addRect(pageLeft, 390, pageWidth, 210, borderPen, cardBrush);
        registerTemplateItem(top);
        registerTemplateItem(bottom);
    } else if (templateType == "bottom_art_top_text") {
        auto *top = m_scene->addRect(pageLeft, 50, pageWidth, 210, borderPen, cardBrush);
        auto *bottom = m_scene->addRect(pageLeft, 280, pageWidth, 320, borderPen, artBrush);
        registerTemplateItem(top);
        registerTemplateItem(bottom);
    } else if (templateType == "storyboard_panels") {
        auto *p1 = m_scene->addRect(pageLeft, 50, pageWidth, 260, borderPen, artBrush);
        auto *p2 = m_scene->addRect(pageLeft, 340, pageWidth, 260, borderPen, artBrush);
        registerTemplateItem(p1);
        registerTemplateItem(p2);
    } else if (templateType == "spot_rhyme") {
        QPen dashedAccent(m_theme.accentColor, 2, Qt::DashLine);
        auto *circle = m_scene->addEllipse(pageCenter - 110, 60, 220, 220, dashedAccent, artBrush);
        auto *card = m_scene->addRect(pageLeft + 20, 330, pageWidth - 40, 270, borderPen, cardBrush);
        registerTemplateItem(circle);
        registerTemplateItem(card);
    } else if (templateType == "split_vertical") {
        auto *c1 = m_scene->addRect(pageLeft, 50, 230, 550, borderPen, artBrush);
        auto *c2 = m_scene->addRect(pageLeft + 250, 50, 230, 550, borderPen, cardBrush);
        registerTemplateItem(c1);
        registerTemplateItem(c2);
    }
}