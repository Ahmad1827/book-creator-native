#include "canvas_items.h"
#include "asset_store.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QTextDocument>
#include <QTextCursor>
#include <QFocusEvent>
#include <QJsonArray>
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

// A stable pseudo-random value in [-1, 1] for one letter, so lettering never "re-jiggles".
qreal letterJitter(quint32 seed, int index, int channel) {
    quint32 h = seed * 2654435761u ^ (quint32(index) * 2246822519u) ^ (quint32(channel + 1) * 3266489917u);
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    h *= 3266489917u;
    h ^= h >> 16;
    return (h / 4294967295.0) * 2.0 - 1.0;
}

QJsonObject textToJson(const StoryTextItem *text) {
    QJsonObject obj;
    obj["text"] = text->toPlainText();
    obj["font"] = text->fontFamily();
    obj["size"] = text->pixelSize();
    obj["color"] = text->defaultTextColor().name(QColor::HexArgb);
    obj["align"] = alignmentToString(text->alignment());
    obj["width"] = text->textWidth();
    if (text->effect() != "none") {
        obj["effect"] = text->effect();
        obj["seed"] = qint64(text->seed());
    }
    return obj;
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

void StoryTextItem::setEffect(const QString &effect) {
    if (effect == m_effect) return;
    prepareGeometryChange();
    m_effect = effect.isEmpty() ? QString("none") : effect;
    update();
}

void StoryTextItem::setSeed(quint32 seed) {
    m_seed = seed;
    update();
}

QRectF StoryTextItem::boundingRect() const {
    const QRectF rect = QGraphicsTextItem::boundingRect();
    if (m_effect == "none") return rect;
    // Tilted and enlarged letters poke out of the text box a little.
    const qreal margin = pixelSize() * 0.4;
    return rect.adjusted(-margin, -margin, margin, margin);
}

QPainterPath StoryTextItem::shape() const {
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void StoryTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // While typing, show plain text so the cursor lines up with the letters.
    if (m_effect == "none" || isEditing()) {
        QGraphicsTextItem::paint(painter, option, widget);
        return;
    }
    paintLettering(painter);
    if (option->state & QStyle::State_Selected) {
        painter->save();
        QPen pen(QColor(0, 0, 0, 140), 0, Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QGraphicsTextItem::boundingRect());
        painter->restore();
    }
}

void StoryTextItem::paintLettering(QPainter *painter) {
    static const QList<QColor> rainbow = {
        QColor("#e5446d"), QColor("#f28c28"), QColor("#e9b824"), QColor("#4caf6a"), QColor("#3d8bd9"), QColor("#8e5bd1")
    };

    QTextDocument *doc = document();
    QAbstractTextDocumentLayout *docLayout = doc->documentLayout();
    const qreal size = pixelSize();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setFont(font());

    int index = 0;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextLayout *layout = block.layout();
        if (!layout) continue;
        const QPointF blockPos = docLayout->blockBoundingRect(block).topLeft();
        const QString text = block.text();

        for (int l = 0; l < layout->lineCount(); ++l) {
            const QTextLine line = layout->lineAt(l);
            const int start = line.textStart();
            const int end = start + line.textLength();
            const qreal baseline = blockPos.y() + line.y() + line.ascent();

            for (int i = start; i < end;) {
                const int len = (text.at(i).isHighSurrogate() && i + 1 < end) ? 2 : 1;
                const QString letter = text.mid(i, len);
                const qreal x0 = blockPos.x() + line.cursorToX(i);
                const qreal x1 = blockPos.x() + line.cursorToX(i + len);
                i += len;
                if (letter.trimmed().isEmpty()) {
                    ++index;
                    continue;
                }

                qreal rotation = 0.0;
                qreal lift = 0.0;
                qreal scale = 1.0;
                QColor color = defaultTextColor();
                if (m_effect == "wobbly") {
                    rotation = letterJitter(m_seed, index, 0) * 7.0;
                    lift = letterJitter(m_seed, index, 1) * 0.06 * size;
                } else if (m_effect == "bouncy") {
                    const qreal phase = index * 0.8 + (m_seed % 7);
                    lift = qSin(phase) * 0.13 * size;
                    rotation = qCos(phase) * 9.0;
                } else if (m_effect == "shouty") {
                    scale = 1.1 + letterJitter(m_seed, index, 2) * 0.28;
                    rotation = letterJitter(m_seed, index, 0) * 14.0;
                    lift = letterJitter(m_seed, index, 1) * 0.1 * size;
                } else if (m_effect == "rainbow") {
                    rotation = letterJitter(m_seed, index, 0) * 5.0;
                    lift = letterJitter(m_seed, index, 1) * 0.05 * size;
                    color = rainbow[(index + int(m_seed % rainbow.size())) % rainbow.size()];
                }

                // Pivot around the middle of the letter so it tilts in place.
                const qreal halfWidth = (x1 - x0) / 2.0;
                const qreal midHeight = size * 0.35;
                painter->save();
                painter->translate(x0 + halfWidth, baseline + lift - midHeight);
                painter->rotate(rotation);
                painter->scale(scale, scale);
                painter->setPen(color);
                painter->drawText(QPointF(-halfWidth, midHeight), letter);
                painter->restore();
                ++index;
            }
        }
    }
    painter->restore();
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
// BubbleItem

BubbleItem::BubbleItem(const QString &kind, const QSizeF &size, bool flipped)
    : m_kind(kind),
      m_size(size),
      m_flipped(flipped),
      m_text(new StoryTextItem(this)) {
    setPath(bubblePath(kind, size, flipped));
    setPen(QPen(QColor("#2c211a"), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setBrush(QColor("#ffffff"));

    m_text->setStyle("Patrick Hand", kind == "shout" ? 26 : 22, QColor("#2c211a"), Qt::AlignHCenter);
    m_text->setTextWidth(bodyRect(kind, size).width() * 0.8);
    // Keep the words centred in the bubble as she types.
    QObject::connect(m_text->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                     m_text, [this]() { layoutText(); });
    layoutText();
}

void BubbleItem::setFlipped(bool flipped) {
    m_flipped = flipped;
    setPath(bubblePath(m_kind, m_size, m_flipped));
    layoutText();
}

QRectF BubbleItem::bodyRect(const QString &kind, const QSizeF &size) {
    const qreal w = size.width();
    const qreal h = size.height();
    if (kind == "thought") return QRectF(0, 0, w, h * 0.74);
    if (kind == "shout") return QRectF(w * 0.14, h * 0.2, w * 0.72, h * 0.6);
    return QRectF(0, 0, w, h * 0.78);
}

QPainterPath BubbleItem::bubblePath(const QString &kind, const QSizeF &size, bool flipped) {
    const qreal w = size.width();
    const qreal h = size.height();
    const QRectF body = bodyRect(kind, size);
    QPainterPath path;

    if (kind == "shout") {
        // A jagged starburst for BIG noisy words.
        const QPointF c(w / 2, h / 2);
        const int spikes = 14;
        QPolygonF star;
        for (int i = 0; i < spikes * 2; ++i) {
            const qreal a = M_PI * i / spikes - M_PI / 2;
            const bool outer = i % 2 == 0;
            const qreal wobble = outer ? 0.88 + 0.12 * ((i * 37) % 5) / 4.0 : 0.72;
            star << QPointF(c.x() + qCos(a) * w / 2 * wobble, c.y() + qSin(a) * h / 2 * wobble);
        }
        path.addPolygon(star);
        path.closeSubpath();
        return path;
    }

    if (kind == "thought") {
        // A cloud: overlapping puffs around an oval, plus a trail of little bubbles.
        path.addEllipse(body.adjusted(body.width() * 0.08, body.height() * 0.12, -body.width() * 0.08, -body.height() * 0.12));
        const int puffs = 10;
        for (int i = 0; i < puffs; ++i) {
            const qreal a = 2 * M_PI * i / puffs;
            const QPointF p(body.center().x() + qCos(a) * body.width() * 0.38, body.center().y() + qSin(a) * body.height() * 0.32);
            const qreal r = body.height() * 0.24;
            QPainterPath puff;
            puff.addEllipse(p, r * 1.15, r);
            path = path.united(puff);
        }
        QPainterPath trail;
        trail.addEllipse(QPointF(w * 0.24, h * 0.85), h * 0.06, h * 0.05);
        trail.addEllipse(QPointF(w * 0.13, h * 0.96), h * 0.035, h * 0.03);
        path.addPath(trail);
    } else {
        path.addEllipse(body);
        QPolygonF tail;
        tail << QPointF(w * 0.26, h * 0.68) << QPointF(w * 0.44, h * 0.74) << QPointF(w * 0.1, h);
        QPainterPath tailPath;
        tailPath.addPolygon(tail);
        tailPath.closeSubpath();
        path = path.united(tailPath);
    }

    if (flipped) path = QTransform(-1, 0, 0, 1, w, 0).map(path);
    return path;
}

void BubbleItem::layoutText() {
    const QRectF body = bodyRect(m_kind, m_size);
    const QRectF textRect = m_text->QGraphicsTextItem::boundingRect();
    m_text->setPos(body.center().x() - textRect.width() / 2, body.center().y() - textRect.height() / 2);
}

// ---------------------------------------------------------------------------
// ImageItem

ImageItem::ImageItem(const QString &assetId)
    : m_assetId(assetId) {
    QPixmap pix = AssetStore::pixmap(assetId);
    if (pix.isNull()) {
        // Keep the slot visible so a missing file can be noticed and replaced.
        pix = QPixmap(240, 160);
        pix.fill(QColor("#e8e1d6"));
        QPainter painter(&pix);
        painter.setPen(QPen(QColor("#8b6d5c"), 2, Qt::DashLine));
        painter.drawRect(pix.rect().adjusted(1, 1, -2, -2));
        painter.drawText(pix.rect(), Qt::AlignCenter, "Missing picture");
    }
    setPixmap(pix);
    setTransformationMode(Qt::SmoothTransformation);
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
        obj = textToJson(text);
        obj["type"] = "text";
    } else if (auto *bubble = dynamic_cast<const BubbleItem*>(item)) {
        obj["type"] = "bubble";
        obj["kind"] = bubble->kind();
        obj["w"] = bubble->size().width();
        obj["h"] = bubble->size().height();
        if (bubble->isFlipped()) obj["flipped"] = true;
        obj["pen"] = penToJson(bubble->pen());
        obj["fill"] = brushToJson(bubble->brush());
        obj["text"] = textToJson(bubble->textItem());
    } else if (auto *image = dynamic_cast<const ImageItem*>(item)) {
        obj["type"] = "image";
        obj["asset"] = image->assetId();
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
    } else {
        return {};
    }

    obj["id"] = item->data(ItemRole::Id).toString();
    obj["x"] = round2(item->pos().x());
    obj["y"] = round2(item->pos().y());
    obj["z"] = item->zValue();
    if (item->opacity() < 1.0) obj["opacity"] = item->opacity();
    if (!qFuzzyCompare(item->scale(), 1.0)) obj["scale"] = item->scale();
    if (!qFuzzyIsNull(item->rotation())) obj["rotation"] = round2(item->rotation());
    if (!item->transformOriginPoint().isNull()) {
        obj["origin"] = QJsonArray{round2(item->transformOriginPoint().x()), round2(item->transformOriginPoint().y())};
    }
    if (!item->transform().isIdentity()) {
        const QTransform t = item->transform();
        obj["transform"] = QJsonArray{t.m11(), t.m12(), t.m13(), t.m21(), t.m22(), t.m23(), t.m31(), t.m32(), t.m33()};
    }
    if (item->data(ItemRole::Locked).toBool()) obj["locked"] = true;
    if (item->data(ItemRole::IsTemplate).toBool()) {
        obj["template"] = item->data(ItemRole::TemplateSide).toString();
        const QString kind = item->data(ItemRole::TemplateKind).toString();
        if (!kind.isEmpty()) obj["templateKind"] = kind;
    }

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
        applyTextJson(text, obj);
        item = text;
    } else if (type == "bubble") {
        auto *bubble = new BubbleItem(obj["kind"].toString("speech"),
                                      QSizeF(obj["w"].toDouble(220), obj["h"].toDouble(150)),
                                      obj["flipped"].toBool(false));
        bubble->setPen(penFromJson(obj["pen"].toObject()));
        bubble->setBrush(brushFromJson(obj["fill"]));
        applyTextJson(bubble->textItem(), obj["text"].toObject());
        item = bubble;
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
        QString assetId = obj["asset"].toString();
        if (assetId.isEmpty() && obj.contains("png")) {
            // Phase 1 kept sticker pixels inline; move them into the asset store.
            QImage legacy;
            legacy.loadFromData(QByteArray::fromBase64(obj["png"].toString().toLatin1()), "PNG");
            assetId = AssetStore::addImage(legacy);
        }
        item = new ImageItem(assetId);
    } else {
        return nullptr;
    }

    item->setPos(obj["x"].toDouble(), obj["y"].toDouble());
    item->setZValue(obj["z"].toDouble());
    item->setOpacity(obj["opacity"].toDouble(1.0));
    const QJsonArray origin = obj["origin"].toArray();
    if (origin.size() == 2) item->setTransformOriginPoint(origin[0].toDouble(), origin[1].toDouble());
    item->setScale(obj["scale"].toDouble(1.0));
    item->setRotation(obj["rotation"].toDouble(0.0));
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
        item->setData(ItemRole::TemplateKind, obj["templateKind"].toString());
    }
    ensureId(item);

    return item;
}

void CanvasSerializer::applyTextJson(StoryTextItem *text, const QJsonObject &obj) {
    text->setStyle(obj["font"].toString("Patrick Hand"), obj["size"].toInt(28),
                   QColor(obj["color"].toString("#ff2c211a")), alignmentFromString(obj["align"].toString()));
    text->setTextWidth(obj["width"].toDouble(400));
    text->setPlainText(obj["text"].toString());
    text->setEffect(obj["effect"].toString("none"));
    text->setSeed(quint32(obj["seed"].toInteger(1)));
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
