#pragma once

#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QJsonObject>
#include <QVector>

// QGraphicsItem::data() keys shared by the canvas, the serializer and the exporter.
namespace ItemRole {
    constexpr int TemplateSide = 0;
    constexpr int IsTemplate = 1;
    constexpr int Id = 2;
    constexpr int Locked = 3;
    constexpr int TemplateKind = 4;  // "art" or "text" frame of a page layout
}

struct StrokeSample {
    QPointF pos;
    qreal pressure = 1.0;
};

// A freehand stroke whose width follows pen pressure. The outline is built as a
// union of round-capped segments, so it fills cleanly at any opacity and the
// eraser can cut it by dropping samples.
class InkStrokeItem : public QGraphicsPathItem {
public:
    enum { Type = UserType + 1 };

    InkStrokeItem(const QString &brush, const QColor &color, qreal width);

    int type() const override { return Type; }

    void addSample(const QPointF &pos, qreal pressure);
    void setSamples(const QVector<StrokeSample> &samples);
    const QVector<StrokeSample> &samples() const { return m_samples; }

    QString brush() const { return m_brush; }
    QColor color() const { return m_color; }
    qreal baseWidth() const { return m_width; }

    // Splits the stroke around an erased circle (item coordinates). Returns false if nothing was hit.
    bool eraseCircle(const QPointF &center, qreal radius, QList<QVector<StrokeSample>> &remainingRuns) const;

private:
    qreal radiusFor(qreal pressure) const;
    void appendDot(const StrokeSample &s);
    void appendSegment(const StrokeSample &a, const StrokeSample &b);

    QString m_brush;
    QColor m_color;
    qreal m_width;
    qreal m_minPressureFactor;
    QVector<StrokeSample> m_samples;
    QPainterPath m_outline;
};

class StoryTextItem : public QGraphicsTextItem {
    Q_OBJECT

public:
    enum { Type = UserType + 2 };

    explicit StoryTextItem(QGraphicsItem *parent = nullptr);

    int type() const override { return Type; }

    void setStyle(const QString &family, int pixelSize, const QColor &color, Qt::Alignment align);
    void setFontFamily(const QString &family);
    void setPixelSize(int pixelSize);
    void setAlignment(Qt::Alignment align);

    QString fontFamily() const { return font().family(); }
    int pixelSize() const { return font().pixelSize(); }
    Qt::Alignment alignment() const;

    void beginEditing();
    bool isEditing() const;

    // Hand-lettered looks: "none", "wobbly", "bouncy", "shouty" or "rainbow". Each box has
    // its own seed so the jiggle stays the same every time the page is drawn.
    void setEffect(const QString &effect);
    QString effect() const { return m_effect; }
    void setSeed(quint32 seed);
    quint32 seed() const { return m_seed; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void editingFinished(StoryTextItem *item);

protected:
    void focusOutEvent(QFocusEvent *event) override;

private:
    void paintLettering(QPainter *painter);

    QString m_effect = "none";
    quint32 m_seed = 1;
};

// A speech, thought or shout bubble that carries its own text box.
class BubbleItem : public QGraphicsPathItem {
public:
    enum { Type = UserType + 4 };

    BubbleItem(const QString &kind, const QSizeF &size, bool flipped);

    int type() const override { return Type; }
    QString kind() const { return m_kind; }
    QSizeF size() const { return m_size; }
    bool isFlipped() const { return m_flipped; }
    void setFlipped(bool flipped);
    StoryTextItem *textItem() const { return m_text; }

    static QPainterPath bubblePath(const QString &kind, const QSizeF &size, bool flipped);
    // The part of the bubble the words sit in (excluding the tail).
    static QRectF bodyRect(const QString &kind, const QSizeF &size);

private:
    void layoutText();

    QString m_kind;
    QSizeF m_size;
    bool m_flipped;
    StoryTextItem *m_text;
};

// A picture from the AssetStore (imported artwork or a sticker).
class ImageItem : public QGraphicsPixmapItem {
public:
    enum { Type = UserType + 3 };

    explicit ImageItem(const QString &assetId);

    int type() const override { return Type; }
    QString assetId() const { return m_assetId; }

private:
    QString m_assetId;
};

// Shapes and page-layout frames, painted like the rest of the book: a watercolour wash
// sitting a little off a doubled ink line. They stay plain rect/ellipse/path items to the
// rest of the app, so saving, erasing and resizing treat them as before.
class SketchRectItem : public QGraphicsRectItem {
public:
    explicit SketchRectItem(const QRectF &rect, QGraphicsItem *parent = nullptr) : QGraphicsRectItem(rect, parent) {}
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

class SketchEllipseItem : public QGraphicsEllipseItem {
public:
    explicit SketchEllipseItem(const QRectF &rect, QGraphicsItem *parent = nullptr) : QGraphicsEllipseItem(rect, parent) {}
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

class SketchPathItem : public QGraphicsPathItem {
public:
    explicit SketchPathItem(const QPainterPath &path, QGraphicsItem *parent = nullptr) : QGraphicsPathItem(path, parent) {}
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

namespace CanvasSerializer {
    QString ensureId(QGraphicsItem *item);
    QString newId();

    // Returns an empty object for items that are not part of the saved artwork.
    QJsonObject itemToJson(const QGraphicsItem *item);
    QGraphicsItem *itemFromJson(const QJsonObject &obj);

    // Shared by plain text boxes and the text inside bubbles.
    void applyTextJson(StoryTextItem *text, const QJsonObject &obj);

    QJsonObject sceneToJson(const QGraphicsScene *scene);
    QList<QGraphicsItem*> loadIntoScene(QGraphicsScene *scene, const QJsonObject &canvasData);
}
