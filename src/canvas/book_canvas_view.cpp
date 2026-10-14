#include "book_canvas_view.h"
#include <QPainter>
#include <QScrollBar>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QSvgRenderer>
#include <QUndoCommand>
#include <QTimer>
#include <QMimeData>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileInfo>
#include <QtMath>
#include "asset_store.h"
#include "sketch.h"
#include <QRandomGenerator>

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
// A little more than 1/8 inch at every print size.
constexpr qreal kFullBleedOverreach = 10.0;

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
    setAcceptDrops(true);

    // Resize/rotate handles sit outside item bounds, so repaint them in full.
    connect(m_scene, &QGraphicsScene::selectionChanged, viewport(), qOverload<>(&QWidget::update));
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        for (auto *item : m_scene->selectedItems()) {
            if (auto *text = dynamic_cast<StoryTextItem*>(item)) m_lastTextItem = text;
            if (auto *bubble = dynamic_cast<BubbleItem*>(item)) m_lastTextItem = bubble->textItem();
        }
    });
    connect(m_scene, &QGraphicsScene::changed, this, [this]() {
        if (handleTarget()) viewport()->update();
    });

    setMode(CanvasMode::Draw);
}

void BookCanvasView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    if ((m_autoFit || !m_initialFitDone) && width() > 100 && height() > 100) {
        fitBookInView();
        m_initialFitDone = true;
    }
}

void BookCanvasView::fitBookInView() {
    m_autoFit = true;
    fitInView(QRectF(-60, -40, 1320, m_pageHeight + 80), Qt::KeepAspectRatio);
}

void BookCanvasView::setPageHeight(qreal pageHeight) {
    if (qFuzzyCompare(m_pageHeight, pageHeight)) return;
    m_pageHeight = pageHeight;
    resetCachedContent();
    viewport()->update();
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
    viewport()->update();
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
    if (item->type() == ImageItem::Type) {
        // Big scans are expensive to rescale on every repaint.
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
    if (auto *text = dynamic_cast<StoryTextItem*>(item)) {
        connect(text, &StoryTextItem::editingFinished, this, &BookCanvasView::onTextEditingFinished);
    }
    if (auto *bubble = dynamic_cast<BubbleItem*>(item)) {
        connect(bubble->textItem(), &StoryTextItem::editingFinished, this, &BookCanvasView::onTextEditingFinished);
    }
    applyInteractionFlags(item);
}

void BookCanvasView::applyNewTextStyle(StoryTextItem *text) const {
    text->setStyle(m_textFamily, m_textPixelSize, m_textColor, m_textAlign);
    text->setEffect(m_textEffect);
    text->setSeed(QRandomGenerator::global()->generate());
}

qreal BookCanvasView::nextTopZ() const {
    qreal maxZ = 0;
    for (auto *item : m_scene->items()) {
        if (item->parentItem() || item->data(ItemRole::IsTemplate).toBool()) continue;
        maxZ = qMax(maxZ, item->zValue());
    }
    return maxZ + 1;
}

void BookCanvasView::centerTransformOrigin(QGraphicsItem *item) {
    item->setTransformOriginPoint(item->boundingRect().center());
}

void BookCanvasView::setPageNumbers(int leftNum, int rightNum) {
    m_leftPageNum = leftNum;
    m_rightPageNum = rightNum;
    resetCachedContent();
    viewport()->update();
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
        // Bubbles own their text box, even an empty one.
        if (text && !text->parentItem() && text->toPlainText().trimmed().isEmpty()) {
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
    finishTextEditing();
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
        centerTransformOrigin(m_currentStroke);
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
        if (item->type() == BubbleItem::Type) continue;

        if (auto *stroke = dynamic_cast<InkStrokeItem*>(item)) {
            QList<QVector<StrokeSample>> runs;
            const qreal localRadius = r / qMax(0.01, stroke->scale());
            if (!stroke->eraseCircle(stroke->mapFromScene(scenePos), localRadius, runs)) continue;

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
                piece->setTransformOriginPoint(stroke->transformOriginPoint());
                piece->setScale(stroke->scale());
                piece->setRotation(stroke->rotation());
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
        if (auto *bubble = dynamic_cast<BubbleItem*>(item)) return bubble->textItem();
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
    m_lastTextItem = item;
    if (!item->toPlainText().trimmed().isEmpty()) {
        recordChange("Edit text");
        return;
    }

    // An abandoned empty box is removed once the focus change has finished unwinding.
    if (item->parentItem()) {
        recordChange("Edit text");
        return;
    }

    QPointer<StoryTextItem> guard(item);
    QTimer::singleShot(0, this, [this, guard]() {
        if (!guard || guard->scene() != m_scene || guard->isEditing()) return;
        if (!guard->toPlainText().trimmed().isEmpty()) return;
        m_scene->removeItem(guard);
        delete guard.data();
        recordChange("Remove empty text");
    });
}

StoryTextItem *BookCanvasView::helperTargetText() const {
    const auto selected = selectedTextItems();
    if (!selected.isEmpty()) return selected.first();
    // Switching tools clears the selection, so fall back to the box she last worked on.
    if (m_lastTextItem && m_lastTextItem->scene() == m_scene) return m_lastTextItem;
    return nullptr;
}

QString BookCanvasView::selectedStoryText() const {
    StoryTextItem *text = helperTargetText();
    return text ? text->toPlainText() : QString();
}

bool BookCanvasView::replaceSelectedStoryText(const QString &newText) {
    StoryTextItem *text = helperTargetText();
    if (!text) return false;
    finishTextEditing();
    text->setPlainText(newText);
    recordChange("Story helper text");
    return true;
}

void BookCanvasView::addStoryTextBox(const QString &content) {
    finishTextEditing();

    QRectF area;
    for (auto *item : m_scene->items()) {
        if (item->data(ItemRole::IsTemplate).toBool() && item->data(ItemRole::TemplateKind).toString() == "text"
            && item->data(ItemRole::TemplateSide).toString() == m_activeSide) {
            area = item->sceneBoundingRect().adjusted(12, 12, -12, -12);
            break;
        }
    }
    if (!area.isValid()) {
        const qreal left = m_activeSide == "left" ? 80.0 : 680.0;
        area = QRectF(left, 440, 440, 150);
    }

    auto *text = new StoryTextItem();
    applyNewTextStyle(text);
    text->setTextWidth(area.width());
    text->setPlainText(content);
    text->setPos(area.topLeft());
    text->setZValue(nextTopZ());
    m_scene->addItem(text);
    adoptItem(text);
    m_scene->clearSelection();
    if (text->flags() & QGraphicsItem::ItemIsSelectable) text->setSelected(true);
    m_lastTextItem = text;
    recordChange("Add text");
}

QList<StoryTextItem*> BookCanvasView::selectedTextItems() const {
    QList<StoryTextItem*> result;
    for (auto *item : m_scene->selectedItems()) {
        if (auto *text = dynamic_cast<StoryTextItem*>(item)) result.append(text);
        if (auto *bubble = dynamic_cast<BubbleItem*>(item)) result.append(bubble->textItem());
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
        // Deselecting means the next style she picks is for the next box, not this one.
        finishTextEditing();
        m_scene->clearSelection();
        event->accept();
        return;
    }

    auto *text = new StoryTextItem();
    applyNewTextStyle(text);

    // Clicking inside a layout's text card makes the box span the card.
    QRectF card;
    for (auto *item : m_scene->items(scenePos)) {
        if (item->data(ItemRole::IsTemplate).toBool() && item->data(ItemRole::TemplateKind).toString() == "text") {
            card = item->sceneBoundingRect();
            break;
        }
    }
    if (card.isValid()) {
        text->setTextWidth(card.width() - 24);
        text->setPos(card.left() + 12, scenePos.y() - m_textPixelSize * 0.7);
    } else {
        const qreal pageRight = scenePos.x() < 600 ? 570.0 : 1170.0;
        text->setTextWidth(qBound(140.0, pageRight - scenePos.x(), 440.0));
        text->setPos(scenePos - QPointF(8, m_textPixelSize * 0.7));
    }
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

void BookCanvasView::setTextEffect(const QString &effect) {
    m_textEffect = effect;
    const auto targets = selectedTextItems();
    for (auto *text : targets) text->setEffect(effect);
    if (!targets.isEmpty()) recordChange("Letter style");
}

void BookCanvasView::shuffleTextEffect() {
    const auto targets = selectedTextItems();
    for (auto *text : targets) text->setSeed(QRandomGenerator::global()->generate());
    if (!targets.isEmpty()) recordChange("Shuffle letters");
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

    // A warm paper desk scattered with faint pencil doodles.
    QRectF deskRect = m_scene->sceneRect();
    painter->fillRect(deskRect, Sketch::kDesk);
    painter->fillRect(deskRect, QBrush(Sketch::doodleTile()));

    const qreal h = m_pageHeight;
    QRectF casingRect(-18, -14, 1236, h + 28);
    const QPainterPath casing = Sketch::wobblyRoundedRect(casingRect, 12, Sketch::seedFor(casingRect.size(), 3), 1.6);
    QColor shadow = Sketch::kInk;
    shadow.setAlpha(45);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillPath(casing.translated(9, 11), shadow);
    painter->fillPath(casing, m_theme.spineColor);
    Sketch::drawInk(painter, casing, Sketch::kInk, 2.6);

    QRectF leftPage(0, 0, 600, h);
    QRectF rightPage(600, 0, 600, h);

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_theme.paperBg);
    painter->drawRect(leftPage);
    painter->drawRect(rightPage);

    QLinearGradient leftEdgeGrad(0, 0, 40, 0);
    leftEdgeGrad.setColorAt(0.0, QColor(0, 0, 0, 35));
    leftEdgeGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->fillRect(QRectF(0, 0, 40, h), leftEdgeGrad);

    QLinearGradient rightEdgeGrad(1200, 0, 1160, 0);
    rightEdgeGrad.setColorAt(0.0, QColor(0, 0, 0, 35));
    rightEdgeGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->fillRect(QRectF(1160, 0, 40, h), rightEdgeGrad);

    QLinearGradient gutterGrad(570, 0, 630, 0);
    gutterGrad.setColorAt(0.0, QColor(0, 0, 0, 0));
    gutterGrad.setColorAt(0.42, QColor(0, 0, 0, 60));
    gutterGrad.setColorAt(0.5, QColor(0, 0, 0, 110));
    gutterGrad.setColorAt(0.58, QColor(0, 0, 0, 60));
    gutterGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->fillRect(QRectF(570, 0, 60, h), gutterGrad);

    painter->setPen(QPen(QColor(0, 0, 0, 100), 1));
    painter->drawLine(QPointF(600, 0), QPointF(600, h));

    painter->setPen(QPen(m_theme.borderColor, 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(leftPage.adjusted(12, 12, -12, -12));
    painter->drawRect(rightPage.adjusted(12, 12, -12, -12));

    ThemeDecors::draw(painter, m_theme, h);
    ThemeDecors::drawPageNumbers(painter, m_theme, m_leftPageNum, m_rightPageNum, h);

    QColor pageInk = Sketch::kInk;
    pageInk.setAlpha(170);
    Sketch::drawInk(painter, Sketch::wobblyRoundedRect(QRectF(0, 0, 1200, h), 2, Sketch::seedFor(QSizeF(1200, h), 4), 1.0), pageInk, 1.6);

    painter->restore();
}

void BookCanvasView::drawForeground(QPainter *painter, const QRectF &rect) {
    Q_UNUSED(rect);
    QGraphicsItem *item = handleTarget();
    if (!item) return;

    const HandleGeometry g = handleGeometry(item);
    const qreal px = 1.0 / transform().m11();
    const QColor accent("#8b5b32");

    painter->save();
    QPen outlinePen(accent, 1.5, Qt::DashLine);
    outlinePen.setCosmetic(true);
    painter->setPen(outlinePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolygon(g.outline);

    QPen solid(accent, 2);
    solid.setCosmetic(true);
    painter->setPen(solid);
    painter->drawLine(g.rotateBase, g.rotate);

    painter->setBrush(QColor("#fbf5ea"));
    painter->drawEllipse(g.resize, 7 * px, 7 * px);
    painter->setBrush(QColor("#dfbe87"));
    painter->drawEllipse(g.rotate, 7 * px, 7 * px);
    painter->restore();
}

// ---------------------------------------------------------------------------
// Resize & rotate handles

QGraphicsItem *BookCanvasView::handleTarget() const {
    if (m_mode != CanvasMode::Select) return nullptr;
    const auto selected = m_scene->selectedItems();
    if (selected.size() != 1) return nullptr;
    QGraphicsItem *item = selected.first();
    if (item->data(ItemRole::Locked).toBool() || item->data(ItemRole::IsTemplate).toBool()) return nullptr;
    if (auto *text = dynamic_cast<StoryTextItem*>(item); text && text->isEditing()) return nullptr;
    return item;
}

BookCanvasView::HandleGeometry BookCanvasView::handleGeometry(QGraphicsItem *item) const {
    HandleGeometry g;
    const QRectF br = item->boundingRect();
    g.outline = item->mapToScene(br);
    g.center = item->mapToScene(br.center());
    g.resize = item->mapToScene(br.bottomRight());
    g.rotateBase = item->mapToScene(QPointF(br.center().x(), br.top()));

    QPointF up = g.rotateBase - g.center;
    const qreal len = qSqrt(up.x() * up.x() + up.y() * up.y());
    up = len > 1e-6 ? up / len : QPointF(0, -1);
    g.rotate = g.rotateBase + up * (30.0 / transform().m11());
    return g;
}

BookCanvasView::HandleDrag BookCanvasView::handleAt(const QPointF &viewportPos) const {
    QGraphicsItem *item = handleTarget();
    if (!item) return HandleDrag::None;
    const HandleGeometry g = handleGeometry(item);
    auto near = [&](const QPointF &scenePoint) {
        return QLineF(viewportTransform().map(scenePoint), viewportPos).length() <= 11.0;
    };
    if (near(g.resize)) return HandleDrag::Resize;
    if (near(g.rotate)) return HandleDrag::Rotate;
    return HandleDrag::None;
}

void BookCanvasView::beginHandleDrag(HandleDrag drag, const QPointF &scenePos) {
    QGraphicsItem *item = handleTarget();
    if (!item) return;

    // Items that were never transformed can safely pivot around their centre.
    if (qFuzzyCompare(item->scale(), 1.0) && qFuzzyIsNull(item->rotation())) {
        centerTransformOrigin(item);
    }

    m_handleDrag = drag;
    m_handleItem = item;
    m_dragCenter = item->mapToScene(item->transformOriginPoint());
    const QPointF fromCenter = scenePos - m_dragCenter;
    m_dragStartScale = item->scale();
    m_dragStartDistance = qMax(1.0, QLineF(m_dragCenter, scenePos).length());
    m_dragStartRotation = item->rotation();
    m_dragStartAngle = qRadiansToDegrees(qAtan2(fromCenter.y(), fromCenter.x()));
}

void BookCanvasView::updateHandleDrag(const QPointF &scenePos, Qt::KeyboardModifiers modifiers) {
    QGraphicsItem *item = m_handleItem;
    if (!item) return;

    if (m_handleDrag == HandleDrag::Resize) {
        if (auto *text = dynamic_cast<StoryTextItem*>(item)) {
            // Text boxes get wider or narrower; the font size stays put.
            const QPointF local = text->mapFromScene(scenePos);
            text->setTextWidth(qMax(60.0, local.x() - text->QGraphicsTextItem::boundingRect().left()));
        } else {
            const qreal ratio = QLineF(m_dragCenter, scenePos).length() / m_dragStartDistance;
            item->setScale(qBound(0.03, m_dragStartScale * ratio, 40.0));
        }
    } else if (m_handleDrag == HandleDrag::Rotate) {
        const QPointF fromCenter = scenePos - m_dragCenter;
        const qreal angle = qRadiansToDegrees(qAtan2(fromCenter.y(), fromCenter.x()));
        qreal rotation = m_dragStartRotation + (angle - m_dragStartAngle);
        if (modifiers & Qt::ShiftModifier) rotation = qRound(rotation / 15.0) * 15.0;
        item->setRotation(rotation);
    }
    viewport()->update();
}

// ---------------------------------------------------------------------------
// Pictures

QRectF BookCanvasView::pageInnerRect(const QString &side) const {
    return side == "left" ? QRectF(40, 40, 520, m_pageHeight - 80) : QRectF(640, 40, 520, m_pageHeight - 80);
}

QRectF BookCanvasView::artFrameFor(const QString &side, const QPointF *point) const {
    QRectF best;
    for (auto *item : m_scene->items()) {
        if (!item->data(ItemRole::IsTemplate).toBool() || item->data(ItemRole::TemplateKind).toString() != "art") continue;
        const QString itemSide = item->data(ItemRole::TemplateSide).toString();
        const QRectF frame = item->sceneBoundingRect().adjusted(2, 2, -2, -2);
        if (point) {
            // A drop picks the frame it lands in.
            if (frame.contains(*point)) return frame;
            continue;
        }
        if (itemSide != side && itemSide != "spread") continue;
        if (frame.width() * frame.height() > best.width() * best.height()) best = frame;
    }
    return best;
}

void BookCanvasView::fitItemInto(QGraphicsItem *item, const QRectF &frame, bool cover) {
    const QRectF br = item->boundingRect();
    if (br.isEmpty()) return;
    const qreal sx = frame.width() / br.width();
    const qreal sy = frame.height() / br.height();
    item->setRotation(0);
    item->setTransformOriginPoint(br.center());
    item->setScale(cover ? qMax(sx, sy) : qMin(sx, sy));
    item->setPos(frame.center() - br.center());
}

bool BookCanvasView::placeImage(const QString &assetId, const QPointF *dropPos) {
    if (assetId.isEmpty()) return false;

    const QString side = dropPos ? (dropPos->x() < 600 ? "left" : "right") : m_activeSide;
    QRectF frame = artFrameFor(side, dropPos);
    if (!frame.isValid()) frame = pageInnerRect(side);

    auto *image = new ImageItem(assetId);
    fitItemInto(image, frame, false);
    image->setZValue(nextTopZ());
    m_scene->addItem(image);
    adoptItem(image);

    m_scene->clearSelection();
    if (image->flags() & QGraphicsItem::ItemIsSelectable) image->setSelected(true);
    emit selectionChanged(image);
    recordChange("Add picture");
    return true;
}

bool BookCanvasView::importImageFile(const QString &path, QString *error) {
    return placeImage(AssetStore::addImageFile(path, error), nullptr);
}

bool BookCanvasView::importImage(const QImage &image) {
    return placeImage(AssetStore::addImage(image), nullptr);
}

bool BookCanvasView::pasteFromMime(const QMimeData *mime, const QPointF *dropPos) {
    if (!mime) return false;
    bool placedAny = false;
    if (mime->hasUrls()) {
        for (const QUrl &url : mime->urls()) {
            if (url.isLocalFile() && AssetStore::isImageFile(url.toLocalFile())) {
                placedAny |= placeImage(AssetStore::addImageFile(url.toLocalFile()), dropPos);
            }
        }
    }
    if (!placedAny && mime->hasImage()) {
        placedAny = placeImage(AssetStore::addImage(qvariant_cast<QImage>(mime->imageData())), dropPos);
    }
    return placedAny;
}

static bool mimeHasPicture(const QMimeData *mime) {
    if (mime->hasImage()) return true;
    for (const QUrl &url : mime->urls()) {
        if (url.isLocalFile() && AssetStore::isImageFile(url.toLocalFile())) return true;
    }
    return false;
}

void BookCanvasView::dragEnterEvent(QDragEnterEvent *event) {
    if (mimeHasPicture(event->mimeData())) event->acceptProposedAction();
    else QGraphicsView::dragEnterEvent(event);
}

void BookCanvasView::dragMoveEvent(QDragMoveEvent *event) {
    if (mimeHasPicture(event->mimeData())) event->acceptProposedAction();
    else QGraphicsView::dragMoveEvent(event);
}

void BookCanvasView::dropEvent(QDropEvent *event) {
    if (!mimeHasPicture(event->mimeData())) {
        QGraphicsView::dropEvent(event);
        return;
    }
    const QPointF scenePos = viewportToScene(event->position());
    updateActiveSide(scenePos);
    if (pasteFromMime(event->mimeData(), &scenePos)) event->acceptProposedAction();
}

// ---------------------------------------------------------------------------
// Input

void BookCanvasView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() == 0) return;

    const qreal factor = (event->angleDelta().y() > 0) ? 1.12 : (1.0 / 1.12);
    m_autoFit = false;

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

    if (event->matches(QKeySequence::Paste)) {
        pasteFromMime(QGuiApplication::clipboard()->mimeData(), nullptr);
        event->accept();
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

    if (event->button() == Qt::LeftButton && m_mode == CanvasMode::Select) {
        const HandleDrag drag = handleAt(event->position());
        if (drag != HandleDrag::None) {
            beginHandleDrag(drag, scenePos);
            event->accept();
            return;
        }
    }

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
        m_autoFit = false;
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

    if (m_handleDrag != HandleDrag::None) {
        updateHandleDrag(viewportToScene(event->position()), event->modifiers());
        event->accept();
        return;
    }

    if (m_mode == CanvasMode::Select && event->buttons() == Qt::NoButton) {
        const HandleDrag hover = handleAt(event->position());
        if (hover == HandleDrag::Resize) viewport()->setCursor(Qt::SizeFDiagCursor);
        else if (hover == HandleDrag::Rotate) viewport()->setCursor(Qt::CrossCursor);
        else updateCursor();
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

    if (m_handleDrag != HandleDrag::None && event->button() == Qt::LeftButton) {
        const bool rotated = m_handleDrag == HandleDrag::Rotate;
        m_handleDrag = HandleDrag::None;
        m_handleItem = nullptr;
        recordChange(rotated ? "Rotate" : "Resize");
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

    if (item && !item->data(ItemRole::IsTemplate).toBool()) {
        m_scene->clearSelection();
        item->setSelected(true);
        emit selectionChanged(item);

        if (auto *text = dynamic_cast<StoryTextItem*>(item)) {
            QAction *actEdit = menu.addAction("Edit Text");
            connect(actEdit, &QAction::triggered, [text]() { text->beginEditing(); });
            menu.addSeparator();
        }
        if (auto *bubble = dynamic_cast<BubbleItem*>(item)) {
            QAction *actEdit = menu.addAction("Edit Words");
            connect(actEdit, &QAction::triggered, [bubble]() { bubble->textItem()->beginEditing(); });
            if (bubble->kind() != "shout") {
                QAction *actFlip = menu.addAction("Flip Tail");
                connect(actFlip, &QAction::triggered, [this, bubble]() {
                    bubble->setFlipped(!bubble->isFlipped());
                    recordChange("Flip tail");
                });
            }
            menu.addSeparator();
        }
        if (item->type() == ImageItem::Type) {
            const QString side = item->sceneBoundingRect().center().x() < 600 ? "left" : "right";
            QAction *actFit = menu.addAction("Fit Inside Page");
            QAction *actFill = menu.addAction("Fill Whole Page (Full Bleed)");
            QAction *actSpread = menu.addAction("Fill Both Pages");
            connect(actFit, &QAction::triggered, [this, item, side]() {
                fitItemInto(item, pageInnerRect(side), false);
                recordChange("Fit picture");
            });
            // Full-bleed art runs a little past the outer edges into the printer's trim.
            const qreal b = kFullBleedOverreach;
            connect(actFill, &QAction::triggered, [this, item, side, b]() {
                const QRectF page = side == "left" ? QRectF(-b, -b, 600 + b, m_pageHeight + 2 * b)
                                                   : QRectF(600, -b, 600 + b, m_pageHeight + 2 * b);
                fitItemInto(item, page, true);
                recordChange("Fill page");
            });
            connect(actSpread, &QAction::triggered, [this, item, b]() {
                fitItemInto(item, QRectF(-b, -b, 1200 + 2 * b, m_pageHeight + 2 * b), true);
                recordChange("Fill spread");
            });
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

    if (shapeType == "speech" || shapeType == "thought" || shapeType == "shout") {
        addBubble(shapeType);
        return;
    }

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
    } else if (shapeType == "cloud") {
        QPainterPath p;
        p.addEllipse(QPointF(posX + 40, posY + 55), 32, 26);
        const QList<QPair<QPointF, qreal>> puffs = {
            {{posX + 75, posY + 38}, 34}, {{posX + 112, posY + 52}, 28}, {{posX + 90, posY + 68}, 24}, {{posX + 55, posY + 70}, 22}
        };
        for (const auto &puff : puffs) {
            QPainterPath circle;
            circle.addEllipse(puff.first, puff.second, puff.second * 0.9);
            p = p.united(circle);
        }
        item = m_scene->addPath(p, pen, brush);
    } else if (shapeType == "moon") {
        QPainterPath disc;
        disc.addEllipse(QPointF(posX + 50, posY + 50), 50, 50);
        QPainterPath bite;
        bite.addEllipse(QPointF(posX + 74, posY + 38), 44, 44);
        item = m_scene->addPath(disc.subtracted(bite), pen, brush);
    } else if (shapeType == "flower") {
        const QPointF c(posX + 55, posY + 55);
        // A disc under the petals closes the gap where their tips meet.
        QPainterPath p;
        p.addEllipse(c, 30, 30);
        for (int i = 0; i < 6; ++i) {
            const qreal a = M_PI * i / 3.0;
            QPainterPath petal;
            petal.addEllipse(c + QPointF(qCos(a) * 30, qSin(a) * 30), 24, 24);
            p = p.united(petal);
        }
        // The centre is outlined within the same shape, so the flower moves as one piece.
        p.setFillRule(Qt::WindingFill);
        p.addEllipse(c, 16, 16);
        item = m_scene->addPath(p, pen, brush);
    } else {
        item = m_scene->addRect(posX, posY, 120, 80, pen, brush);
    }

    if (item) {
        item->setZValue(nextTopZ());
        centerTransformOrigin(item);
        adoptItem(item);
        m_scene->clearSelection();
        if (item->flags() & QGraphicsItem::ItemIsSelectable) item->setSelected(true);
        emit selectionChanged(item);
        recordChange("Add shape");
    }
}

void BookCanvasView::addBubble(const QString &kind) {
    finishTextEditing();
    const QSizeF size = kind == "shout" ? QSizeF(240, 170) : QSizeF(230, 160);
    auto *bubble = new BubbleItem(kind, size, m_activeSide == "right");
    bubble->textItem()->setFontFamily(m_textFamily);
    bubble->textItem()->setDefaultTextColor(m_textColor);
    if (kind == "shout") bubble->textItem()->setEffect("shouty");

    const qreal left = m_activeSide == "left" ? 180.0 : 780.0;
    bubble->setPos(left, 120);
    centerTransformOrigin(bubble);
    bubble->setZValue(nextTopZ());
    m_scene->addItem(bubble);
    adoptItem(bubble);

    m_scene->clearSelection();
    if (bubble->flags() & QGraphicsItem::ItemIsSelectable) bubble->setSelected(true);
    m_lastTextItem = bubble->textItem();
    recordChange("Add bubble");
    // Start typing straight away.
    setFocus();
    bubble->textItem()->beginEditing();
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

    // Rendered at 4x and shown at quarter scale so stamps stay crisp in print.
    QSvgRenderer renderer(fullSvg.toUtf8());
    QImage image(400, 400, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter);
    painter.end();

    auto *pixmapItem = new ImageItem(AssetStore::addImage(image));
    centerTransformOrigin(pixmapItem);
    pixmapItem->setScale(0.25);
    pixmapItem->setPos(QPointF(posX + 50, posY + 50) - pixmapItem->boundingRect().center());
    pixmapItem->setZValue(nextTopZ());
    m_scene->addItem(pixmapItem);
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
    const bool wholeSpread = templateType == "spread_art";

    // A layout replaces the frames on its page, and any frame spanning both pages.
    for (auto *item : m_scene->items()) {
        if (!item->data(ItemRole::IsTemplate).toBool()) continue;
        const QString itemSide = item->data(ItemRole::TemplateSide).toString();
        if (wholeSpread || itemSide == side || itemSide == "spread") {
            m_scene->removeItem(item);
            delete item;
        }
    }

    auto registerTemplateItem = [this](QGraphicsItem *item, const QString &itemSide, const QString &kind) {
        item->setData(ItemRole::TemplateSide, itemSide);
        item->setData(ItemRole::IsTemplate, true);
        item->setData(ItemRole::TemplateKind, kind);
        item->setZValue(kTemplateZ);
        adoptItem(item);
    };

    // Layouts are drawn for a 650-unit page and stretched to the book's proportions.
    const qreal fy = m_pageHeight / 650.0;
    auto R = [fy](qreal x, qreal y, qreal w, qreal h) { return QRectF(x, y * fy, w, h * fy); };

    QPen borderPen(m_theme.borderColor, 2);
    QBrush artBrush(QColor(255, 255, 255, 115));
    QBrush cardBrush(QColor(255, 255, 255, 220));

    if (templateType == "top_art_bottom_text") {
        registerTemplateItem(m_scene->addRect(R(pageLeft, 50, pageWidth, 320), borderPen, artBrush), side, "art");
        registerTemplateItem(m_scene->addRect(R(pageLeft, 390, pageWidth, 210), borderPen, cardBrush), side, "text");
    } else if (templateType == "bottom_art_top_text") {
        registerTemplateItem(m_scene->addRect(R(pageLeft, 50, pageWidth, 210), borderPen, cardBrush), side, "text");
        registerTemplateItem(m_scene->addRect(R(pageLeft, 280, pageWidth, 320), borderPen, artBrush), side, "art");
    } else if (templateType == "big_art_caption") {
        registerTemplateItem(m_scene->addRect(R(pageLeft, 50, pageWidth, 440), borderPen, artBrush), side, "art");
        registerTemplateItem(m_scene->addRect(R(pageLeft, 505, pageWidth, 95), borderPen, cardBrush), side, "text");
    } else if (templateType == "full_page_art") {
        registerTemplateItem(m_scene->addRect(R(pageLeft - 36, 24, pageWidth + 72, 602), borderPen, artBrush), side, "art");
    } else if (templateType == "text_page") {
        registerTemplateItem(m_scene->addRect(R(pageLeft, 110, pageWidth, 430), borderPen, cardBrush), side, "text");
    } else if (templateType == "spread_art") {
        registerTemplateItem(m_scene->addRect(R(24, 24, 1152, 602), borderPen, artBrush), "spread", "art");
    } else if (templateType == "classic_arch_story") {
        QPen dashedAccent(m_theme.accentColor, 2, Qt::DashLine);
        registerTemplateItem(m_scene->addRect(R(pageLeft - 10, 40, pageWidth + 20, 570), dashedAccent, Qt::NoBrush), side, "");
    } else if (templateType == "storyboard_panels") {
        registerTemplateItem(m_scene->addRect(R(pageLeft, 50, pageWidth, 260), borderPen, artBrush), side, "art");
        registerTemplateItem(m_scene->addRect(R(pageLeft, 340, pageWidth, 260), borderPen, artBrush), side, "art");
    } else if (templateType == "spot_rhyme") {
        QPen dashedAccent(m_theme.accentColor, 2, Qt::DashLine);
        registerTemplateItem(m_scene->addEllipse(QRectF(pageCenter - 110 * qMin(1.0, fy), 60 * fy, 220 * qMin(1.0, fy), 220 * qMin(1.0, fy)), dashedAccent, artBrush), side, "art");
        registerTemplateItem(m_scene->addRect(R(pageLeft + 20, 330, pageWidth - 40, 270), borderPen, cardBrush), side, "text");
    } else if (templateType == "split_vertical") {
        registerTemplateItem(m_scene->addRect(R(pageLeft, 50, 230, 550), borderPen, artBrush), side, "art");
        registerTemplateItem(m_scene->addRect(R(pageLeft + 250, 50, 230, 550), borderPen, cardBrush), side, "text");
    }

    recordChange("Page layout");
}
