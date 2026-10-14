#pragma once

#include <QColor>
#include <QFont>
#include <QLabel>
#include <QPainterPath>
#include <QProxyStyle>
#include <QWidget>

// The app's look: scratchy ink outlines and loose watercolour washes on cream paper,
// in the spirit of classic British children's book illustration.
namespace Sketch {
    // Paper and ink
    inline const QColor kPaper("#fff8e7");
    inline const QColor kPaperDeep("#fbecd0");
    inline const QColor kCard("#fffdf7");
    inline const QColor kDesk("#f6e7c8");
    inline const QColor kInk("#1f1a17");
    inline const QColor kInkSoft("#6f6157");

    // Watercolour washes
    inline const QColor kYellow("#ffd25a");
    inline const QColor kTomato("#f26b4f");
    inline const QColor kTeal("#5cc9bd");
    inline const QColor kPink("#ff9fb5");
    inline const QColor kGreen("#9bd06a");
    inline const QColor kSky("#7cc0ee");
    inline const QColor kLilac("#bf9cef");
    inline const QColor kOrange("#ffab5c");
    inline const QColor kBlush("#ffc2b3");

    // Big storybook titles, hand-lettered headings and buttons, and easy-reading body text.
    QFont titleFont(int pixelSize);
    QFont headingFont(int pixelSize);
    QFont bodyFont(int pixelSize);

    QLabel *title(const QString &text, QWidget *parent, int pixelSize = 34);
    QLabel *heading(const QString &text, QWidget *parent, int pixelSize = 22);
    QLabel *section(const QString &text, QWidget *parent);
    QLabel *hint(const QString &text, QWidget *parent);
    // Restyle an existing label as a section heading or a soft hint.
    void styleSection(QLabel *label);
    void styleHint(QLabel *label);
    void setTextColor(QWidget *widget, const QColor &color);

    // A stable seed from a size, so outlines don't shimmer on every repaint.
    quint32 seedFor(const QSizeF &size, quint32 salt = 0);

    QPainterPath wobblyRoundedRect(const QRectF &rect, qreal radius, quint32 seed, qreal amplitude = 1.2);
    QPainterPath wobblyEllipse(const QRectF &rect, quint32 seed, qreal amplitude = 1.0);
    QPainterPath wobblyLine(const QPointF &from, const QPointF &to, quint32 seed, qreal amplitude = 1.0);

    // A slightly doubled pen line, like a dip pen going over itself.
    void drawInk(QPainter *painter, const QPainterPath &path, const QColor &ink = kInk, qreal width = 2.0);
    // A colour wash that sits a little off the ink line, like real watercolour.
    void drawWash(QPainter *painter, const QPainterPath &path, const QColor &color, const QPointF &offset = QPointF(1.6, 1.8));

    // A tile of faint pencil doodles (stars, swirls, dots) for backgrounds.
    const QPixmap &doodleTile();

    // Hand-drawn picture of a book spine in a theme's colours, for theme pickers.
    QPixmap spinePicture(const QColor &spine, const QColor &paper, const QColor &accent, const QSize &size);

    // Installs the style, palette and fonts on the application.
    void install();
}

// Cream paper surfaces (bars, drawers, panels) with inked edges.
class SketchPanel : public QWidget {
    Q_OBJECT

public:
    enum Edge { NoEdge = 0, TopEdge = 1, BottomEdge = 2, LeftEdge = 4, RightEdge = 8 };

    explicit SketchPanel(QWidget *parent = nullptr, const QColor &fill = Sketch::kPaper, int edges = NoEdge);

    void setFill(const QColor &fill);
    void setEdges(int edges);
    void setDoodles(bool doodles);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_fill;
    int m_edges;
    bool m_doodles = false;
};

// Draws every standard control by hand. Buttons read two dynamic properties:
//   role  - "primary", "danger", "ghost", "link", "chip", "card", "rail" or "swatch"
//   wash  - a QColor for the button's watercolour (rail tools, coloured actions)
//   swatch - the QColor a colour-swatch button shows
class SketchStyle : public QProxyStyle {
    Q_OBJECT

public:
    SketchStyle();

    QPalette standardPalette() const override;
    void polish(QWidget *widget) override;
    void polish(QPalette &palette) override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;

private:
    void drawButtonPanel(const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
};
