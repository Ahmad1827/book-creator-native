#include "book_canvas_view.h"
#include <QPainter>
#include <QScrollBar>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QSvgRenderer>
#include <QUndoCommand>
#include <QTimer>

namespace {

// Stores only the items that changed, as JSON before and after, so history stays small.
class CanvasChangeCommand : public QUndoCommand {
public:
    CanvasChangeCommand(BookCanvasView *view, const QStringList &ids,
                        const QHash<QString, QJsonObject> &before,
                        const QHash<QString, QJsonObject> &after,
                        const QString &text)
        : QUndoCommand(text), m_view(view), m_ids(ids), m_before(before), m_after(after) {}

    void undo() override { m_view->applyItemStates(m_ids, m_before); }

    void redo() override {
        // The change is already on the canvas when the command is pushed.
        if (m_skipFirstRedo) {
            m_skipFirstRedo = false;
            return;
        }
        m_view->applyItemStates(m_ids, m_after);
    }

private:
    BookCanvasView *m_view;
    QStringList m_ids;
    QHash<QString, QJsonObject> m_before;
    QHash<QString, QJsonObject> m_after;
    bool m_skipFirstRedo = true;
};

constexpr qreal kTemplateZ = -1000.0;

} // namespace

BookCanvasView::BookCanvasView(QWidget *parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this)),
      m_theme(THEMES[0]),
      m_undoGroup(new QUndoGroup(this)) {

    m_scene->setSceneRect(-10000, -10000, 21200, 20650);
    // Strokes change geometry on every pen sample; a BSP index would be rebuilt constantly.
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
    setCacheMode(QGraphicsView::CacheBackground);

    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);

    setMode(CanvasMode::Draw);
}

void BookCanvasView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    if (!m_initialFitDone && width() > 100 && height() > 100) {
        fitBookInView();
        m_initialFitDone = true;
    }
}

void BookCanvasView::fitBookInView() {
    fitInView(QRectF(-60, -40, 1320, 730), Qt::KeepAspectRatio);
}

void BookCanvasView::setTheme(const BookTheme &theme) {
    m_theme = theme;
    m_brushColor = theme.inkColor;
    m_textColor = theme.inkColor;
    resetCachedContent();
    viewport()->update();
}

void BookCanvasView::setMode(CanvasMode mode) {
    if (m_mode == CanvasMode::Text && mode != CanvasMode::Text) {
        finishTextEditing();
    }
    m_mode = mode;

    if (m_mode == CanvasMode::Pan) {
        setDragMode(QGraphicsView::NoDrag);
        setInteractive(false);
    } else if (m_mode == CanvasMode::Select) {
        setDragMode(QGraphicsView::RubberBandDrag);
        setInteractive(true);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setInteractive(true);
    }

    for (auto *item : m_scene->items()) {
        if (!item->parentItem()) applyInteractionFlags(item);
    }
    updateCursor();
}

void BookCanvasView::updateCursor() {
    if (m_isMiddlePanning || m_isSpacePanning) {
        viewport()->setCursor(Qt::ClosedHandCursor);
    } else if (m_spacePressed || m_mode == CanvasMode::Pan) {
        viewport()->setCursor(Qt::OpenHandCursor);
    } else if (m_mode == CanvasMode::Draw) {
        viewport()->setCursor(Qt::CrossCursor);
    } else if (m_mode == CanvasMode::Text) {
        viewport()->setCursor(Qt::IBeamCursor);
    } else {
        viewport()->setCursor(Qt::ArrowCursor);
    }
}

void BookCanvasView::applyInteractionFlags(QGraphicsItem *item) {
    const bool isTemplate = item->data(ItemRole::IsTemplate).toBool();
    const bool isText = item->type() == StoryTextItem::Type;
    const bool locked = item->data(ItemRole::Locked).toBool();

    bool selectable = false;
    bool movable = false;
    if (!isTemplate) {
        if (m_mode == CanvasMode::Select) {
            selectable = true;
            movable = !locked;
        } else if (m_mode == CanvasMode::Text && isText) {
            selectable = true;
        }
    }
    item->setFlag(QGraphicsItem::ItemIsSelectable, selectable);
    item->setFlag(QGraphicsItem::ItemIsMovable, movable);
}

void BookCanvasView::adoptItem(QGraphicsItem *item) {
    CanvasSerializer::ensureId(item);
    if (auto *text = dynamic_cast<StoryTextItem*>(item)) {
        connect(text, &StoryTextItem::editingFinished, this, &BookCanvasView::onTextEditingFinished);
    }
    applyInteractionFlags(item);
}

qreal BookCanvasView::nextTopZ() const {
    qreal maxZ = 0;
    for (auto *item : m_scene->items()) {
        if (item->parentItem() || item->data(ItemRole::IsTemplate).toBool()) continue;
        maxZ = qMax(maxZ, item->zValue());
    }
    return maxZ + 1;
}

void BookCanvasView::setBrushSubtype(const QString &subtype) {
    m_brushSubtype = subtype;
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

// ---------------------------------------------------------------------------
// Persistence & undo

void BookCanvasView::loadSpread(const QString &spreadId, const QJsonObject &canvasData) {
    m_isDrawing = false;
    m_isErasing = false;
    m_tabletStrokeActive = false;
    m_currentStroke = nullptr;

    m_scene->clear();
    for (auto *item : CanvasSerializer::loadIntoScene(m_scene, canvasData)) {
        adoptItem(item);
    }
    m_itemStates = captureItemStates();

    QUndoStack *&stack = m_undoStacks[spreadId];
    if (!stack) {
        stack = new QUndoStack(m_undoGroup);
        stack->setUndoLimit(150);
        m_undoGroup->addStack(stack);
    }
    m_undoGroup->setActiveStack(stack);
    emit selectionChanged(nullptr);
}

QJsonObject BookCanvasView::saveSpread() {
    finishTextEditing();

    bool removedAny = false;
    for (auto *item : m_scene->items()) {
        auto *text = dynamic_cast<StoryTextItem*>(item);
        if (text && text->toPlainText().trimmed().isEmpty()) {
            m_scene->removeItem(text);
            text->deleteLater();
            removedAny = true;
        }
    }
    if (removedAny) recordChange("Remove empty text");

    return CanvasSerializer::sceneToJson(m_scene);
}

void BookCanvasView::clearUndoHistory() {
    qDeleteAll(m_undoStacks);
    m_undoStacks.clear();
}

void BookCanvasView::undo() {
    finishTextEditing();
    if (auto *stack = m_undoGroup->activeStack()) stack->undo();
}

void BookCanvasView::redo() {
    finishTextEditing();
    if (auto *stack = m_undoGroup->activeStack()) stack->redo();
}

QHash<QString, QJsonObject> BookCanvasView::captureItemStates() const {
    QHash<QString, QJsonObject> states;
    for (auto *item : m_scene->items()) {
        if (item->parentItem() || item == m_currentStroke) continue;
        QJsonObject obj = CanvasSerializer::itemToJson(item);
        if (obj.isEmpty()) continue;
        const QString id = CanvasSerializer::ensureId(item);
        obj["id"] = id;
        states.insert(id, obj);
    }
    return states;
}

QGraphicsItem *BookCanvasView::findItemById(const QString &id) const {
    for (auto *item : m_scene->items()) {
        if (!item->parentItem() && item->data(ItemRole::Id).toString() == id) return item;
    }
    return nullptr;
}

void BookCanvasView::recordChange(const QString &label) {
    const QHash<QString, QJsonObject> now = captureItemStates();
    if (m_applyingUndo) {
        m_itemStates = now;
        return;
    }

    QStringList ids;
    QHash<QString, QJsonObject> before;
    QHash<QString, QJsonObject> after;
    for (auto it = now.constBegin(); it != now.constEnd(); ++it) {
        auto prev = m_itemStates.constFind(it.key());
        if (prev == m_itemStates.constEnd() || prev.value() != it.value()) {
            ids.append(it.key());
            after.insert(it.key(), it.value());
            if (prev != m_itemStates.constEnd()) before.insert(it.key(), prev.value());
        }
    }
    for (auto it = m_itemStates.constBegin(); it != m_itemStates.constEnd(); ++it) {
        if (!now.contains(it.key())) {
            ids.append(it.key());
            before.insert(it.key(), it.value());
        }
    }
    m_itemStates = now;

    if (ids.isEmpty()) return;
    if (auto *stack = m_undoGroup->activeStack()) {
        stack->push(new CanvasChangeCommand(this, ids, before, after, label));
    }
    emit contentChanged();
}

void BookCanvasView::applyItemStates(const QStringList &ids, const QHash<QString, QJsonObject> &states) {
    m_applyingUndo = true;
    finishTextEditing();
    m_scene->clearSelection();

    for (const QString &id : ids) {
        if (QGraphicsItem *existing = findItemById(id)) {
            m_scene->removeItem(existing);
            delete existing;
        }
        auto it = states.constFind(id);
        if (it == states.constEnd()) {
            m_itemStates.remove(id);
            continue;
        }
        if (QGraphicsItem *item = CanvasSerializer::itemFromJson(it.value())) {
            m_scene->addItem(item);
            adoptItem(item);
            m_itemStates.insert(id, it.value());
        }
    }

    m_applyingUndo = false;
    emit selectionChanged(nullptr);
    emit contentChanged();
}

// ---------------------------------------------------------------------------
// Drawing

QPointF BookCanvasView::viewportToScene(const QPointF &viewportPos) const {
    return viewportTransform().inverted().map(viewportPos);
}

void BookCanvasView::updateActiveSide(const QPointF &scenePos) {
    const QString side = scenePos.x() < 600 ? "left" : "right";
    if (side != m_activeSide) {
        m_activeSide = side;
        emit activeSideChanged(m_activeSide);
    }
}

void BookCanvasView::beginStroke(const QPointF &scenePos, qreal pressure, bool forceEraser) {
    m_isDrawing = true;
    m_isErasing = forceEraser || m_brushSubtype == "eraser";
    if (m_isErasing) {
        eraseAtPoint(scenePos);
        return;
    }

    QColor strokeColor = m_brushColor;
    qreal width = m_brushSize;

    if (m_brushSubtype == "watercolor") {
        strokeColor.setAlphaF(0.35);
        width *= 1.6;
    } else if (m_brushSubtype == "marker") {
        strokeColor.setAlphaF(0.85);
        width *= 1.25;
    } else if (m_brushSubtype == "crayon") {
        strokeColor.setAlphaF(0.70);
        width *= 1.35;
    } else {
        strokeColor.setAlphaF(m_brushOpacity);
    }

    m_currentStroke = new InkStrokeItem(m_brushSubtype, strokeColor, width);
    m_currentStroke->setZValue(nextTopZ());
    CanvasSerializer::ensureId(m_currentStroke);
    m_scene->addItem(m_currentStroke);

    m_smoothedPos = scenePos;
    m_smoothedPressure = pressure;
    m_lastRawPos = scenePos;
    m_currentStroke->addSample(scenePos, pressure);
}

void BookCanvasView::extendStroke(const QPointF &scenePos, qreal pressure) {
    if (!m_isDrawing) return;
    if (m_isErasing) {
        eraseAtPoint(scenePos);
        return;
    }
    if (!m_currentStroke) return;

    // Light smoothing takes the jitter out of the line without noticeable lag.
    m_lastRawPos = scenePos;
    m_smoothedPos += (scenePos - m_smoothedPos) * 0.5;
    m_smoothedPressure += (pressure - m_smoothedPressure) * 0.5;
    m_currentStroke->addSample(m_smoothedPos, m_smoothedPressure);
}

void BookCanvasView::endStroke() {
    if (!m_isDrawing) return;
    m_isDrawing = false;

    if (m_isErasing) {
        m_isErasing = false;
        recordChange("Erase");
        return;
    }

    if (m_currentStroke) {
        m_currentStroke->addSample(m_lastRawPos, m_smoothedPressure);
        applyInteractionFlags(m_currentStroke);
        m_currentStroke = nullptr;
    }
    recordChange("Draw");
}

void BookCanvasView::eraseAtPoint(const QPointF &scenePos) {
    qreal r = m_brushSize * 2.0;
    QRectF eraseRect(scenePos.x() - r, scenePos.y() - r, r * 2.0, r * 2.0);

    QPainterPath eraseCircle;
    eraseCircle.addEllipse(eraseRect);

    auto itemsToInspect = m_scene->items(eraseRect);
    for (auto *item : itemsToInspect) {
        if (item->parentItem() || item->data(ItemRole::IsTemplate).toBool()) continue;

        if (auto *stroke = dynamic_cast<InkStrokeItem*>(item)) {
            QList<QVector<StrokeSample>> runs;
            if (!stroke->eraseCircle(stroke->mapFromScene(scenePos), r, runs)) continue;

            if (runs.isEmpty()) {
                m_scene->removeItem(stroke);
                delete stroke;
                continue;
            }

            stroke->setSamples(runs.first());
            for (int i = 1; i < runs.size(); ++i) {
                auto *piece = new InkStrokeItem(stroke->brush(), stroke->color(), stroke->baseWidth());
                piece->setSamples(runs[i]);
                piece->setPos(stroke->pos());
                piece->setTransform(stroke->transform());
                piece->setZValue(stroke->zValue());
                m_scene->addItem(piece);
                adoptItem(piece);
            }
            continue;
        }

        auto *pathItem = dynamic_cast<QGraphicsPathItem*>(item);
        if (pathItem) {
            QPainterPath itemLocalErase = pathItem->mapFromScene(eraseCircle);
            QPainterPath currentPath = pathItem->path();

            if (currentPath.intersects(itemLocalErase)) {
                QPainterPath subtracted = currentPath.subtracted(itemLocalErase);
                if (subtracted.isEmpty() || subtracted.length() < 1.0) {
                    m_scene->removeItem(pathItem);
                    delete pathItem;
                } else {
                    pathItem->setPath(subtracted);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Text

StoryTextItem *BookCanvasView::textItemAt(const QPointF &scenePos) const {
    for (auto *item : m_scene->items(scenePos)) {
        if (auto *text = dynamic_cast<StoryTextItem*>(item)) return text;
    }
    return nullptr;
}

StoryTextItem *BookCanvasView::editingTextItem() const {
    auto *text = dynamic_cast<StoryTextItem*>(m_scene->focusItem());
    return (text && text->isEditing()) ? text : nullptr;
}

void BookCanvasView::finishTextEditing() {
    if (auto *text = editingTextItem()) {
        text->clearFocus();
    }
}

void BookCanvasView::onTextEditingFinished(StoryTextItem *item) {
    if (!item->toPlainText().trimmed().isEmpty()) {
        recordChange("Edit text");
        return;
    }

    // An abandoned empty box is removed once the focus change has finished unwinding.
    QPointer<StoryTextItem> guard(item);
    QTimer::singleShot(0, this, [this, guard]() {
        if (!guard || guard->scene() != m_scene || guard->isEditing()) return;
        if (!guard->toPlainText().trimmed().isEmpty()) return;
        m_scene->removeItem(guard);
        delete guard.data();
        recordChange("Remove empty text");
    });
}

QList<StoryTextItem*> BookCanvasView::selectedTextItems() const {
    QList<StoryTextItem*> result;
    for (auto *item : m_scene->selectedItems()) {
        if (auto *text = dynamic_cast<StoryTextItem*>(item)) result.append(text);
    }
    if (auto *editing = editingTextItem(); editing && !result.contains(editing)) {
        result.append(editing);
    }
    return result;
}

void BookCanvasView::handleTextClick(const QPointF &scenePos, QMouseEvent *event) {
    StoryTextItem *hit = textItemAt(scenePos);
    StoryTextItem *editing = editingTextItem();

    if (hit) {
        if (hit != editing) {
            m_scene->clearSelection();
            hit->setSelected(true);
            hit->beginEditing();
        }
        // Let the text item place the caret where she clicked.
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (editing) {
        // First click outside a box just finishes it; the next click starts a new one.
        finishTextEditing();
        event->accept();
        return;
    }

    auto *text = new StoryTextItem();
    text->setStyle(m_textFamily, m_textPixelSize, m_textColor, m_textAlign);
    const qreal pageRight = scenePos.x() < 600 ? 570.0 : 1170.0;
    text->setTextWidth(qBound(140.0, pageRight - scenePos.x(), 440.0));
    text->setPos(scenePos - QPointF(8, m_textPixelSize * 0.7));
    text->setZValue(nextTopZ());
    m_scene->addItem(text);
    adoptItem(text);

    m_scene->clearSelection();
    text->setSelected(true);
    text->beginEditing();
    event->accept();
}

void BookCanvasView::setTextFontFamily(const QString &family) {
    m_textFamily = family;
    const auto targets = selectedTextItems();
    for (auto *text : targets) text->setFontFamily(family);
    if (!targets.isEmpty()) recordChange("Change font");
}

void BookCanvasView::setTextPixelSize(int size) {
    m_textPixelSize = size;
    const auto targets = selectedTextItems();
    for (auto *text : targets) text->setPixelSize(size);
    if (!targets.isEmpty()) recordChange("Change text size");
}

void BookCanvasView::setTextColor(const QColor &color) {
    m_textColor = color;
    const auto targets = selectedTextItems();
    for (auto *text : targets) text->setDefaultTextColor(color);
    if (!targets.isEmpty()) recordChange("Change text colour");
}

void BookCanvasView::setTextAlignment(Qt::Alignment align) {
    m_textAlign = align;
    const auto targets = selectedTextItems();
    for (auto *text : targets) text->setAlignment(align);
    if (!targets.isEmpty()) recordChange("Align text");
}

// ---------------------------------------------------------------------------
// Background

void BookCanvasView::drawBackground(QPainter *painter, const QRectF &rect) {
    Q_UNUSED(rect);

    painter->save();

    QRectF deskRect = m_scene->sceneRect();
    QLinearGradient deskGrad(0, -10000, 0, 10650);
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

// ---------------------------------------------------------------------------
// Input

void BookCanvasView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() == 0) return;

    const qreal factor = (event->angleDelta().y() > 0) ? 1.12 : (1.0 / 1.12);

    QPointF mouseViewportPos = event->position();
    QPointF anchorScenePos = mapToScene(mouseViewportPos.toPoint());

    scale(factor, factor);

    QPointF viewportCenter = viewport()->rect().center();
    QPointF offsetFromCursor = viewportCenter - mouseViewportPos;

    qreal currentZoom = transform().m11();
    QPointF newSceneCenter = anchorScenePos + (offsetFromCursor / currentZoom);

    centerOn(newSceneCenter);
    event->accept();
}

bool BookCanvasView::viewportEvent(QEvent *event) {
    switch (event->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        auto *tabletEvent = static_cast<QTabletEvent*>(event);
        if (handleTabletEvent(tabletEvent)) {
            // Accepting stops Qt from also synthesizing mouse events for the pen.
            tabletEvent->accept();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QGraphicsView::viewportEvent(event);
}

bool BookCanvasView::handleTabletEvent(QTabletEvent *event) {
    switch (event->type()) {
    case QEvent::TabletPress: {
        // Anything but drawing (selecting, panning, the side button's right-click)
        // falls through to the regular mouse handling.
        if (m_mode != CanvasMode::Draw || event->button() != Qt::LeftButton || m_spacePressed || m_isDrawing) {
            return false;
        }
        setFocus(Qt::MouseFocusReason);
        const QPointF scenePos = viewportToScene(event->position());
        updateActiveSide(scenePos);
        m_tabletStrokeActive = true;
        const bool flippedToEraser = event->pointerType() == QPointingDevice::PointerType::Eraser;
        beginStroke(scenePos, event->pressure(), flippedToEraser);
        return true;
    }
    case QEvent::TabletMove:
        if (!m_tabletStrokeActive) return false;
        extendStroke(viewportToScene(event->position()), event->pressure());
        return true;
    case QEvent::TabletRelease:
        if (!m_tabletStrokeActive) return false;
        if (event->button() == Qt::LeftButton) {
            m_tabletStrokeActive = false;
            endStroke();
        }
        return true;
    default:
        return false;
    }
}

void BookCanvasView::keyPressEvent(QKeyEvent *event) {
    if (editingTextItem()) {
        if (event->key() == Qt::Key_Escape) {
            finishTextEditing();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
        return;
    }

    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && !m_scene->selectedItems().isEmpty()) {
        deleteSelectedItem();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        updateCursor();
    }
    QGraphicsView::keyPressEvent(event);
}

void BookCanvasView::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat() && m_spacePressed) {
        m_spacePressed = false;
        m_isSpacePanning = false;
        updateCursor();
    }
    QGraphicsView::keyReleaseEvent(event);
}

void BookCanvasView::mousePressEvent(QMouseEvent *event) {
    if (m_tabletStrokeActive) {
        event->accept();
        return;
    }

    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && (m_spacePressed || m_mode == CanvasMode::Pan))) {
        if (event->button() == Qt::MiddleButton) m_isMiddlePanning = true;
        else m_isSpacePanning = true;
        m_lastPanPoint = event->pos();
        updateCursor();
        event->accept();
        return;
    }

    QPointF scenePos = viewportToScene(event->position());
    updateActiveSide(scenePos);

    if (event->button() == Qt::RightButton) {
        QGraphicsItem *item = m_scene->itemAt(scenePos, transform());
        showContextMenu(event->globalPosition().toPoint(), item);
        event->accept();
        return;
    }

    if (m_mode == CanvasMode::Draw && event->button() == Qt::LeftButton) {
        beginStroke(scenePos, 1.0, false);
        event->accept();
        return;
    }

    if (m_mode == CanvasMode::Text && event->button() == Qt::LeftButton) {
        handleTextClick(scenePos, event);
        return;
    }

    QGraphicsView::mousePressEvent(event);

    if (m_mode == CanvasMode::Select) {
        auto selected = m_scene->selectedItems();
        emit selectionChanged(selected.isEmpty() ? nullptr : selected.first());
    }
}

void BookCanvasView::mouseMoveEvent(QMouseEvent *event) {
    if (m_tabletStrokeActive) {
        event->accept();
        return;
    }

    if (m_isMiddlePanning || m_isSpacePanning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();

        // Screen-to-scene shift: moving mouse down/right pulls scene down/right
        qreal zoom = transform().m11();
        QPointF deltaScene(delta.x() / zoom, delta.y() / zoom);
        QPointF currentCenter = mapToScene(viewport()->rect().center());
        centerOn(currentCenter - deltaScene);

        event->accept();
        return;
    }

    if (m_mode == CanvasMode::Draw && m_isDrawing && (event->buttons() & Qt::LeftButton)) {
        extendStroke(viewportToScene(event->position()), 1.0);
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void BookCanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (m_tabletStrokeActive) {
        event->accept();
        return;
    }

    if (event->button() == Qt::MiddleButton && m_isMiddlePanning) {
        m_isMiddlePanning = false;
        updateCursor();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_isSpacePanning) {
        m_isSpacePanning = false;
        updateCursor();
        event->accept();
        return;
    }

    if (m_mode == CanvasMode::Draw && event->button() == Qt::LeftButton && m_isDrawing) {
        endStroke();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);

    if (m_mode == CanvasMode::Select && event->button() == Qt::LeftButton) {
        recordChange("Move");
    }
}

void BookCanvasView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_mode == CanvasMode::Select && event->button() == Qt::LeftButton) {
        if (auto *text = textItemAt(viewportToScene(event->position()))) {
            m_scene->clearSelection();
            text->setSelected(true);
            text->beginEditing();
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

// ---------------------------------------------------------------------------
// Item actions

void BookCanvasView::showContextMenu(const QPoint &globalPos, QGraphicsItem *item) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #fbf5ea; border: 2px solid #8d6b4f; border-radius: 8px; padding: 4px; }"
        "QMenu::item { color: #3b2212; padding: 6px 20px; font-weight: bold; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #ecd9bf; color: #1a0d06; }"
        "QMenu::separator { height: 1px; background-color: #dcc6ab; margin: 4px 6px; }"
    );

    if (item && !item->data(ItemRole::IsTemplate).toBool()) {
        m_scene->clearSelection();
        item->setSelected(true);
        emit selectionChanged(item);

        QAction *actEdit = nullptr;
        if (auto *text = dynamic_cast<StoryTextItem*>(item)) {
            actEdit = menu.addAction("Edit Text");
            connect(actEdit, &QAction::triggered, [text]() { text->beginEditing(); });
            menu.addSeparator();
        }
        QAction *actDup = menu.addAction("Duplicate");
        QAction *actFront = menu.addAction("Bring to Front");
        QAction *actBack = menu.addAction("Send to Back");
        QAction *actLock = menu.addAction(item->data(ItemRole::Locked).toBool() ? "Unlock Position" : "Lock Position");
        menu.addSeparator();
        QAction *actDel = menu.addAction("Delete");

        connect(actDup, &QAction::triggered, [this, item]() { duplicateItem(item); });
        connect(actFront, &QAction::triggered, [this, item]() { bringItemToFront(item); });
        connect(actBack, &QAction::triggered, [this, item]() { sendItemToBack(item); });
        connect(actLock, &QAction::triggered, [this, item]() { toggleLockItem(item); });
        connect(actDel, &QAction::triggered, [this, item]() { deleteItem(item); });
    } else {
        QAction *actResetZoom = menu.addAction("Reset View (Fit Book)");
        connect(actResetZoom, &QAction::triggered, this, &BookCanvasView::fitBookInView);
    }

    menu.exec(globalPos);
}

void BookCanvasView::duplicateSelectedItem() {
    auto selected = m_scene->selectedItems();
    if (!selected.isEmpty()) duplicateItem(selected.first());
}

void BookCanvasView::deleteSelectedItem() {
    bool removedAny = false;
    for (auto *item : m_scene->selectedItems()) {
        if (item->data(ItemRole::IsTemplate).toBool()) continue;
        m_scene->removeItem(item);
        delete item;
        removedAny = true;
    }
    if (removedAny) {
        emit selectionChanged(nullptr);
        recordChange("Delete");
    }
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
    if (!item || item->data(ItemRole::IsTemplate).toBool()) return;

    QJsonObject obj = CanvasSerializer::itemToJson(item);
    if (obj.isEmpty()) return;
    obj["id"] = CanvasSerializer::newId();
    obj["x"] = obj["x"].toDouble() + 20;
    obj["y"] = obj["y"].toDouble() + 20;
    obj["z"] = nextTopZ();
    obj.remove("locked");

    QGraphicsItem *copy = CanvasSerializer::itemFromJson(obj);
    if (!copy) return;
    m_scene->addItem(copy);
    adoptItem(copy);

    m_scene->clearSelection();
    if (copy->flags() & QGraphicsItem::ItemIsSelectable) copy->setSelected(true);
    recordChange("Duplicate");
}

void BookCanvasView::deleteItem(QGraphicsItem *item) {
    if (!item || item->data(ItemRole::IsTemplate).toBool()) return;
    m_scene->removeItem(item);
    delete item;
    emit selectionChanged(nullptr);
    recordChange("Delete");
}

void BookCanvasView::bringItemToFront(QGraphicsItem *item) {
    if (!item || item->data(ItemRole::IsTemplate).toBool()) return;
    item->setZValue(nextTopZ());
    recordChange("Bring to front");
}

void BookCanvasView::sendItemToBack(QGraphicsItem *item) {
    if (!item || item->data(ItemRole::IsTemplate).toBool()) return;
    qreal minZ = 0;
    for (auto *i : m_scene->items()) {
        if (!i->data(ItemRole::IsTemplate).toBool() && i->zValue() < minZ) minZ = i->zValue();
    }
    item->setZValue(minZ - 1);
    recordChange("Send to back");
}

void BookCanvasView::toggleLockItem(QGraphicsItem *item) {
    if (!item || item->data(ItemRole::IsTemplate).toBool()) return;
    item->setData(ItemRole::Locked, !item->data(ItemRole::Locked).toBool());
    applyInteractionFlags(item);
    recordChange(item->data(ItemRole::Locked).toBool() ? "Lock" : "Unlock");
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
        item->setZValue(nextTopZ());
        adoptItem(item);
        m_scene->clearSelection();
        if (item->flags() & QGraphicsItem::ItemIsSelectable) item->setSelected(true);
        emit selectionChanged(item);
        recordChange("Add shape");
    }
}

void BookCanvasView::addStorySticker(const QString &stickerId, const QString &svgPath, const QColor &fill) {
    Q_UNUSED(stickerId);
    qreal posX = (m_activeSide == "left") ? 250.0 : 850.0;
    qreal posY = 220.0;

    QString fullSvg = QString(
        "<svg viewBox=\"0 0 100 100\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path d=\"%1\" fill=\"%2\" stroke=\"#2c211a\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" />"
        "</svg>"
    ).arg(svgPath, fill.name());

    QSvgRenderer renderer(fullSvg.toUtf8());
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();

    auto *pixmapItem = m_scene->addPixmap(QPixmap::fromImage(image));
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    pixmapItem->setPos(posX, posY);
    pixmapItem->setZValue(nextTopZ());
    adoptItem(pixmapItem);

    m_scene->clearSelection();
    if (pixmapItem->flags() & QGraphicsItem::ItemIsSelectable) pixmapItem->setSelected(true);
    emit selectionChanged(pixmapItem);
    recordChange("Add sticker");
}

void BookCanvasView::applyPageTemplate(const QString &side, const QString &templateType) {
    qreal pageLeft = (side == "left") ? 60.0 : 660.0;
    qreal pageWidth = 480.0;
    qreal pageCenter = (side == "left") ? 300.0 : 900.0;

    for (auto *item : m_scene->items()) {
        if (item->data(ItemRole::TemplateSide).toString() == side && item->data(ItemRole::IsTemplate).toBool()) {
            m_scene->removeItem(item);
            delete item;
        }
    }

    auto registerTemplateItem = [this, side](QGraphicsItem *item) {
        item->setData(ItemRole::TemplateSide, side);
        item->setData(ItemRole::IsTemplate, true);
        item->setZValue(kTemplateZ);
        adoptItem(item);
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

    recordChange("Page layout");
}
