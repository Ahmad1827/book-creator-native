#include "canvas_items.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QTextDocument>
#include <QTextCursor>
#include <QFocusEvent>
#include <QJsonArray>
#include <QBuffer>
#include <QUuid>
#include <QPen>
#include <QtMath>

namespace {

constexpr qreal kMaxSampleSpacing = 2.0;
constexpr qreal kMinSampleDistance = 0.4;
constexpr qreal kMinRadius = 0.3;

qreal pressureFloorForBrush(const QString &brush) {
    if (brush == "ink") return 0.15;
    if (brush == "pencil") return 0.35;
    if (brush == "crayon") return 0.4;
    if (brush == "watercolor") return 0.45;
    if (brush == "marker") return 0.7;
    return 0.3;
}

// Every piece of the outline gets the same winding direction so that
// overlapping pieces add up under Qt::WindingFill instead of cancelling out.
void addOrientedPolygon(QPainterPath &path, QPolygonF poly) {
    qreal area = 0.0;
    for (int i = 0; i < poly.size(); ++i) {
        const QPointF &a = poly[i];
        const QPointF &b = poly[(i + 1) % poly.size()];
        area += a.x() * b.y() - b.x() * a.y();
    }
    if (area < 0) std::reverse(poly.begin(), poly.end());
    path.addPolygon(poly);
    path.closeSubpath();
}

QPolygonF circlePolygon(const QPointF &c, qreal r) {
    const int n = qBound(8, int(r * 1.2), 32);
    QPolygonF poly;
    poly.reserve(n);
    for (int i = 0; i < n; ++i) {
        const qreal a = 2.0 * M_PI * i / n;
        poly << QPointF(c.x() + r * qCos(a), c.y() + r * qSin(a));
    }
    return poly;
}

qreal round2(qreal v) { return qRound(v * 100.0) / 100.0; }

QJsonObject penToJson(const QPen &pen) {
    QJsonObject o;
    o["color"] = pen.color().name(QColor::HexArgb);
    o["width"] = pen.widthF();
    o["style"] = int(pen.style());
    o["cap"] = int(pen.capStyle());
    o["join"] = int(pen.joinStyle());
    return o;
}

QPen penFromJson(const QJsonObject &o) {
    if (o.isEmpty()) return QPen(Qt::NoPen);
    QPen pen(QColor(o["color"].toString()), o["width"].toDouble(1.0));
    pen.setStyle(Qt::PenStyle(o["style"].toInt(int(Qt::SolidLine))));
    pen.setCapStyle(Qt::PenCapStyle(o["cap"].toInt(int(Qt::SquareCap))));
    pen.setJoinStyle(Qt::PenJoinStyle(o["join"].toInt(int(Qt::BevelJoin))));
    return pen;
}

QJsonValue brushToJson(const QBrush &brush) {
    if (brush.style() == Qt::NoBrush) return QJsonValue();
    return brush.color().name(QColor::HexArgb);
}

QBrush brushFromJson(const QJsonValue &v) {
    if (!v.isString()) return QBrush(Qt::NoBrush);
    return QBrush(QColor(v.toString()));
}

QJsonArray rectToJson(const QRectF &r) {
    return QJsonArray{r.x(), r.y(), r.width(), r.height()};
}

QRectF rectFromJson(const QJsonValue &v) {
    QJsonArray a = v.toArray();
    return QRectF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a[3].toDouble());
}

QJsonArray pathToJson(const QPainterPath &path) {
    QJsonArray arr;
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto el = path.elementAt(i);
        arr.append(int(el.type));
        arr.append(round2(el.x));
        arr.append(round2(el.y));
    }
    return arr;
}

QPainterPath pathFromJson(const QJsonArray &arr) {
    QPainterPath path;
    for (int i = 0; i + 2 < arr.size(); i += 3) {
        const int type = arr[i].toInt();
        const QPointF p(arr[i + 1].toDouble(), arr[i + 2].toDouble());
        if (type == QPainterPath::MoveToElement) {
            path.moveTo(p);
        } else if (type == QPainterPath::LineToElement) {
            path.lineTo(p);
        } else if (type == QPainterPath::CurveToElement && i + 8 < arr.size()) {
            const QPointF c2(arr[i + 4].toDouble(), arr[i + 5].toDouble());
            const QPointF end(arr[i + 7].toDouble(), arr[i + 8].toDouble());
            path.cubicTo(p, c2, end);
            i += 6;
        }
    }
    return path;
}

QString alignmentToString(Qt::Alignment align) {
    if (align & Qt::AlignHCenter) return "center";
    if (align & Qt::AlignRight) return "right";
    return "left";
}

Qt::Alignment alignmentFromString(const QString &s) {
    if (s == "center") return Qt::AlignHCenter;
    if (s == "right") return Qt::AlignRight;
    return Qt::AlignLeft;
}

} // namespace

// ---------------------------------------------------------------------------
// InkStrokeItem

InkStrokeItem::InkStrokeItem(const QString &brush, const QColor &color, qreal width)
    : m_brush(brush),
      m_color(color),
      m_width(width),
      m_minPressureFactor(pressureFloorForBrush(brush)) {
    m_outline.setFillRule(Qt::WindingFill);
    setPen(Qt::NoPen);
    setBrush(color);
}

qreal InkStrokeItem::radiusFor(qreal pressure) const {
    const qreal p = qBound(0.0, pressure, 1.0);
    return qMax(kMinRadius, m_width * 0.5 * (m_minPressureFactor + (1.0 - m_minPressureFactor) * p));
}

void InkStrokeItem::appendDot(const StrokeSample &s) {
    addOrientedPolygon(m_outline, circlePolygon(s.pos, radiusFor(s.pressure)));
}

void InkStrokeItem::appendSegment(const StrokeSample &a, const StrokeSample &b) {
    const QPointF d = b.pos - a.pos;
    const qreal len = qSqrt(d.x() * d.x() + d.y() * d.y());
    const qreal rb = radiusFor(b.pressure);
    if (len > 1e-6) {
        const qreal ra = radiusFor(a.pressure);
        const QPointF n(-d.y() / len, d.x() / len);
        QPolygonF quad;
        quad << a.pos + n * ra << b.pos + n * rb << b.pos - n * rb << a.pos - n * ra;
        addOrientedPolygon(m_outline, quad);
    }
    addOrientedPolygon(m_outline, circlePolygon(b.pos, rb));
}

void InkStrokeItem::addSample(const QPointF &pos, qreal pressure) {
    if (m_samples.isEmpty()) {
        StrokeSample s{pos, pressure};
        m_samples.append(s);
        appendDot(s);
        setPath(m_outline);
        return;
    }

    const StrokeSample last = m_samples.last();
    const qreal dist = QLineF(last.pos, pos).length();
    if (dist < kMinSampleDistance) return;

    // Keep samples evenly spaced so the eraser can cut fast strokes cleanly.
    const int steps = qMax(1, qCeil(dist / kMaxSampleSpacing));
    for (int i = 1; i <= steps; ++i) {
        const qreal t = qreal(i) / steps;
        StrokeSample s{last.pos + (pos - last.pos) * t, last.pressure + (pressure - last.pressure) * t};
        appendSegment(m_samples.last(), s);
        m_samples.append(s);
    }
    setPath(m_outline);
}

void InkStrokeItem::setSamples(const QVector<StrokeSample> &samples) {
    m_samples = samples;
    m_outline = QPainterPath();
    m_outline.setFillRule(Qt::WindingFill);
    for (int i = 0; i < m_samples.size(); ++i) {
        if (i == 0) appendDot(m_samples[i]);
        else appendSegment(m_samples[i - 1], m_samples[i]);
    }
    setPath(m_outline);
}

bool InkStrokeItem::eraseCircle(const QPointF &center, qreal radius, QList<QVector<StrokeSample>> &remainingRuns) const {
    bool hit = false;
    QVector<StrokeSample> run;
    for (const auto &s : m_samples) {
        if (QLineF(s.pos, center).length() <= radius + radiusFor(s.pressure) * 0.5) {
            hit = true;
            if (run.size() >= 2) remainingRuns.append(run);
            run.clear();
        } else {
            run.append(s);
        }
    }
    if (!hit) {
        remainingRuns.clear();
        return false;
    }
    if (run.size() >= 2) remainingRuns.append(run);
    return true;
}

// ---------------------------------------------------------------------------
// StoryTextItem

StoryTextItem::StoryTextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent) {
    document()->setDocumentMargin(6);
}

void StoryTextItem::setStyle(const QString &family, int pixelSize, const QColor &color, Qt::Alignment align) {
    QFont f(family);
    f.setPixelSize(qMax(4, pixelSize));
    setFont(f);
    setDefaultTextColor(color);
    setAlignment(align);
}

void StoryTextItem::setFontFamily(const QString &family) {
    QFont f = font();
    f.setFamily(family);
    setFont(f);
}

void StoryTextItem::setPixelSize(int pixelSize) {
    QFont f = font();
    f.setPixelSize(qMax(4, pixelSize));
    setFont(f);
}

void StoryTextItem::setAlignment(Qt::Alignment align) {
    QTextOption opt = document()->defaultTextOption();
    opt.setAlignment(align);
    document()->setDefaultTextOption(opt);
}

Qt::Alignment StoryTextItem::alignment() const {
    return document()->defaultTextOption().alignment();
}

void StoryTextItem::beginEditing() {
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus(Qt::MouseFocusReason);
}

bool StoryTextItem::isEditing() const {
    return textInteractionFlags() & Qt::TextEditable;
}

void StoryTextItem::focusOutEvent(QFocusEvent *event) {
    QGraphicsTextItem::focusOutEvent(event);

    // Switching windows or opening a colour picker shouldn't end the edit.
    if (event->reason() == Qt::ActiveWindowFocusReason || event->reason() == Qt::PopupFocusReason) return;
    if (!isEditing()) return;

    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
    setTextInteractionFlags(Qt::NoTextInteraction);
    emit editingFinished(this);
}

// ---------------------------------------------------------------------------
// CanvasSerializer

QString CanvasSerializer::newId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString CanvasSerializer::ensureId(QGraphicsItem *item) {
    QString id = item->data(ItemRole::Id).toString();
    if (id.isEmpty()) {
        id = newId();
        item->setData(ItemRole::Id, id);
    }
    return id;
}

QJsonObject CanvasSerializer::itemToJson(const QGraphicsItem *item) {
    QJsonObject obj;

    if (auto *stroke = dynamic_cast<const InkStrokeItem*>(item)) {
        obj["type"] = "stroke";
        obj["brush"] = stroke->brush();
        obj["color"] = stroke->color().name(QColor::HexArgb);
        obj["width"] = stroke->baseWidth();
        QJsonArray samples;
        for (const auto &s : stroke->samples()) {
            samples.append(round2(s.pos.x()));
            samples.append(round2(s.pos.y()));
            samples.append(round2(s.pressure));
        }
        obj["samples"] = samples;
    } else if (auto *text = dynamic_cast<const StoryTextItem*>(item)) {
        obj["type"] = "text";
        obj["text"] = text->toPlainText();
        obj["font"] = text->fontFamily();
        obj["size"] = text->pixelSize();
        obj["color"] = text->defaultTextColor().name(QColor::HexArgb);
        obj["align"] = alignmentToString(text->alignment());
        obj["width"] = text->textWidth();
    } else if (auto *pathItem = dynamic_cast<const QGraphicsPathItem*>(item)) {
        obj["type"] = "path";
        obj["path"] = pathToJson(pathItem->path());
        obj["fillRule"] = int(pathItem->path().fillRule());
        obj["pen"] = penToJson(pathItem->pen());
        obj["fill"] = brushToJson(pathItem->brush());
    } else if (auto *rectItem = dynamic_cast<const QGraphicsRectItem*>(item)) {
        obj["type"] = "rect";
        obj["rect"] = rectToJson(rectItem->rect());
        obj["pen"] = penToJson(rectItem->pen());
        obj["fill"] = brushToJson(rectItem->brush());
    } else if (auto *ellipseItem = dynamic_cast<const QGraphicsEllipseItem*>(item)) {
        obj["type"] = "ellipse";
        obj["rect"] = rectToJson(ellipseItem->rect());
        obj["pen"] = penToJson(ellipseItem->pen());
        obj["fill"] = brushToJson(ellipseItem->brush());
    } else if (auto *pixItem = dynamic_cast<const QGraphicsPixmapItem*>(item)) {
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        pixItem->pixmap().save(&buffer, "PNG");
        obj["type"] = "image";
        obj["png"] = QString::fromLatin1(png.toBase64());
    } else {
        return {};
    }

    obj["id"] = item->data(ItemRole::Id).toString();
    obj["x"] = round2(item->pos().x());
    obj["y"] = round2(item->pos().y());
    obj["z"] = item->zValue();
    if (item->opacity() < 1.0) obj["opacity"] = item->opacity();
    if (!item->transform().isIdentity()) {
        const QTransform t = item->transform();
        obj["transform"] = QJsonArray{t.m11(), t.m12(), t.m13(), t.m21(), t.m22(), t.m23(), t.m31(), t.m32(), t.m33()};
    }
    if (item->data(ItemRole::Locked).toBool()) obj["locked"] = true;
    if (item->data(ItemRole::IsTemplate).toBool()) obj["template"] = item->data(ItemRole::TemplateSide).toString();

    return obj;
}

QGraphicsItem *CanvasSerializer::itemFromJson(const QJsonObject &obj) {
    const QString type = obj["type"].toString();
    QGraphicsItem *item = nullptr;

    if (type == "stroke") {
        auto *stroke = new InkStrokeItem(obj["brush"].toString(), QColor(obj["color"].toString()), obj["width"].toDouble(4.0));
        QVector<StrokeSample> samples;
        const QJsonArray arr = obj["samples"].toArray();
        samples.reserve(arr.size() / 3);
        for (int i = 0; i + 2 < arr.size(); i += 3) {
            samples.append({QPointF(arr[i].toDouble(), arr[i + 1].toDouble()), arr[i + 2].toDouble()});
        }
        stroke->setSamples(samples);
        item = stroke;
    } else if (type == "text") {
        auto *text = new StoryTextItem();
        text->setStyle(obj["font"].toString(), obj["size"].toInt(28), QColor(obj["color"].toString()), alignmentFromString(obj["align"].toString()));
        text->setTextWidth(obj["width"].toDouble(400));
        text->setPlainText(obj["text"].toString());
        item = text;
    } else if (type == "path") {
        QPainterPath path = pathFromJson(obj["path"].toArray());
        path.setFillRule(Qt::FillRule(obj["fillRule"].toInt(int(Qt::OddEvenFill))));
        auto *pathItem = new QGraphicsPathItem(path);
        pathItem->setPen(penFromJson(obj["pen"].toObject()));
        pathItem->setBrush(brushFromJson(obj["fill"]));
        item = pathItem;
    } else if (type == "rect") {
        auto *rectItem = new QGraphicsRectItem(rectFromJson(obj["rect"]));
        rectItem->setPen(penFromJson(obj["pen"].toObject()));
        rectItem->setBrush(brushFromJson(obj["fill"]));
        item = rectItem;
    } else if (type == "ellipse") {
        auto *ellipseItem = new QGraphicsEllipseItem(rectFromJson(obj["rect"]));
        ellipseItem->setPen(penFromJson(obj["pen"].toObject()));
        ellipseItem->setBrush(brushFromJson(obj["fill"]));
        item = ellipseItem;
    } else if (type == "image") {
        QPixmap pix;
        pix.loadFromData(QByteArray::fromBase64(obj["png"].toString().toLatin1()), "PNG");
        auto *pixItem = new QGraphicsPixmapItem(pix);
        pixItem->setTransformationMode(Qt::SmoothTransformation);
        item = pixItem;
    } else {
        return nullptr;
    }

    item->setPos(obj["x"].toDouble(), obj["y"].toDouble());
    item->setZValue(obj["z"].toDouble());
    item->setOpacity(obj["opacity"].toDouble(1.0));
    const QJsonArray t = obj["transform"].toArray();
    if (t.size() == 9) {
        item->setTransform(QTransform(t[0].toDouble(), t[1].toDouble(), t[2].toDouble(),
                                      t[3].toDouble(), t[4].toDouble(), t[5].toDouble(),
                                      t[6].toDouble(), t[7].toDouble(), t[8].toDouble()));
    }
    item->setData(ItemRole::Id, obj["id"].toString());
    item->setData(ItemRole::Locked, obj["locked"].toBool(false));
    if (obj.contains("template")) {
        item->setData(ItemRole::IsTemplate, true);
        item->setData(ItemRole::TemplateSide, obj["template"].toString());
    }
    ensureId(item);

    return item;
}

QJsonObject CanvasSerializer::sceneToJson(const QGraphicsScene *scene) {
    QJsonArray items;
    // items() is topmost-first; save bottom-up so the file reads in paint order.
    const auto all = scene->items(Qt::AscendingOrder);
    for (auto *item : all) {
        if (item->parentItem()) continue;
        QJsonObject obj = itemToJson(item);
        if (!obj.isEmpty()) items.append(obj);
    }
    QJsonObject data;
    data["version"] = 1;
    data["items"] = items;
    return data;
}

QList<QGraphicsItem*> CanvasSerializer::loadIntoScene(QGraphicsScene *scene, const QJsonObject &canvasData) {
    QList<QGraphicsItem*> created;
    const QJsonArray items = canvasData["items"].toArray();
    for (const auto &val : items) {
        if (auto *item = itemFromJson(val.toObject())) {
            scene->addItem(item);
            created.append(item);
        }
    }
    return created;
}
