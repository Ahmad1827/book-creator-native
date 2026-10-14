#include "sketch.h"
#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <QtMath>

// ---------------------------------------------------------------------------
// Fonts and labels

QFont Sketch::titleFont(int pixelSize) {
    QFont f("Henny Penny");
    f.setPixelSize(pixelSize);
    return f;
}

QFont Sketch::headingFont(int pixelSize) {
    QFont f("Gochi Hand");
    f.setPixelSize(pixelSize);
    return f;
}

QFont Sketch::bodyFont(int pixelSize) {
    QFont f("Patrick Hand");
    f.setPixelSize(pixelSize);
    return f;
}

QLabel *Sketch::title(const QString &text, QWidget *parent, int pixelSize) {
    auto *label = new QLabel(text, parent);
    label->setFont(titleFont(pixelSize));
    return label;
}

QLabel *Sketch::heading(const QString &text, QWidget *parent, int pixelSize) {
    auto *label = new QLabel(text, parent);
    label->setFont(headingFont(pixelSize));
    return label;
}

QLabel *Sketch::section(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    styleSection(label);
    return label;
}

QLabel *Sketch::hint(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    styleHint(label);
    return label;
}

void Sketch::styleSection(QLabel *label) {
    // Section names read better in sentence case when hand-lettered.
    QString nice = label->text().toLower();
    if (!nice.isEmpty()) nice[0] = nice[0].toUpper();
    label->setText(nice);
    label->setFont(headingFont(19));
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, kTomato.darker(130));
    label->setPalette(pal);
}

void Sketch::setTextColor(QWidget *widget, const QColor &color) {
    QPalette pal = widget->palette();
    pal.setColor(QPalette::WindowText, color);
    pal.setColor(QPalette::ButtonText, color);
    widget->setPalette(pal);
}

void Sketch::styleHint(QLabel *label) {
    label->setFont(bodyFont(15));
    label->setWordWrap(true);
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, kInkSoft);
    label->setPalette(pal);
}

// ---------------------------------------------------------------------------
// Wobbly ink

quint32 Sketch::seedFor(const QSizeF &size, quint32 salt) {
    quint32 h = quint32(qRound(size.width())) * 73856093u ^ quint32(qRound(size.height())) * 19349663u ^ (salt * 83492791u);
    h ^= h >> 13;
    h *= 2654435761u;
    return h ^ (h >> 16);
}

namespace {

// Smooth, seeded wobble in [-1, 1] along a closed or open outline.
qreal wobble(qreal t, quint32 seed) {
    const qreal p1 = (seed % 628) / 100.0;
    const qreal p2 = ((seed / 7) % 628) / 100.0;
    const qreal p3 = ((seed / 31) % 628) / 100.0;
    return 0.55 * qSin(2 * M_PI * t * 3 + p1) + 0.3 * qSin(2 * M_PI * t * 7 + p2) + 0.15 * qSin(2 * M_PI * t * 13 + p3);
}

QPainterPath wobblePath(const QPainterPath &base, quint32 seed, qreal amplitude, bool closed) {
    const qreal length = base.length();
    const int n = qBound(12, int(length / 7.0), 500);
    QPolygonF pts;
    pts.reserve(n + 1);
    const int count = closed ? n : n + 1;
    for (int i = 0; i < count; ++i) {
        const qreal t = qreal(i) / n;
        const QPointF p = base.pointAtPercent(qMin(t, 1.0));
        const qreal a = qDegreesToRadians(base.angleAtPercent(qMin(t, 1.0)));
        const QPointF normal(qSin(a), qCos(a));
        pts << p + normal * (amplitude * wobble(t, seed));
    }

    QPainterPath path;
    if (closed) {
        auto mid = [](const QPointF &a, const QPointF &b) { return (a + b) / 2.0; };
        path.moveTo(mid(pts.last(), pts.first()));
        for (int i = 0; i < pts.size(); ++i) {
            path.quadTo(pts[i], mid(pts[i], pts[(i + 1) % pts.size()]));
        }
        path.closeSubpath();
    } else {
        path.moveTo(pts.first());
        for (int i = 1; i < pts.size(); ++i) path.lineTo(pts[i]);
    }
    return path;
}

} // namespace

QPainterPath Sketch::wobblyRoundedRect(const QRectF &rect, qreal radius, quint32 seed, qreal amplitude) {
    QPainterPath base;
    base.addRoundedRect(rect, radius, radius);
    return wobblePath(base, seed, amplitude, true);
}

QPainterPath Sketch::wobblyEllipse(const QRectF &rect, quint32 seed, qreal amplitude) {
    QPainterPath base;
    base.addEllipse(rect);
    return wobblePath(base, seed, amplitude, true);
}

QPainterPath Sketch::wobblyLine(const QPointF &from, const QPointF &to, quint32 seed, qreal amplitude) {
    QPainterPath base(from);
    base.lineTo(to);
    return wobblePath(base, seed, amplitude, false);
}

void Sketch::drawInk(QPainter *painter, const QPainterPath &path, const QColor &ink, qreal width) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(ink, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPath(path);
    QColor ghost = ink;
    ghost.setAlphaF(ink.alphaF() * 0.35);
    painter->setPen(QPen(ghost, qMax(0.8, width * 0.45), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPath(path.translated(0.8, 0.6));
    painter->restore();
}

void Sketch::drawWash(QPainter *painter, const QPainterPath &path, const QColor &color, const QPointF &offset) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    const QRectF bounds = path.boundingRect();
    QLinearGradient grad(bounds.topLeft(), bounds.bottomLeft());
    grad.setColorAt(0.0, color.lighter(112));
    grad.setColorAt(0.75, color);
    grad.setColorAt(1.0, color.darker(106));
    painter->setBrush(grad);
    painter->drawPath(path.translated(offset));
    painter->restore();
}

const QPixmap &Sketch::doodleTile() {
    static QPixmap tile;
    if (!tile.isNull()) return tile;

    tile = QPixmap(260, 260);
    tile.fill(Qt::transparent);
    QPainter p(&tile);
    p.setRenderHint(QPainter::Antialiasing, true);
    QColor pencil = kInkSoft;
    pencil.setAlpha(34);
    QPen pen(pencil, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);

    // A star
    QPolygonF star;
    for (int i = 0; i < 10; ++i) {
        const qreal a = M_PI * i / 5 - M_PI / 2;
        const qreal r = i % 2 ? 6 : 14;
        star << QPointF(40 + qCos(a) * r, 46 + qSin(a) * r);
    }
    p.drawPolygon(star);
    // A swirl
    QPainterPath swirl;
    for (int i = 0; i <= 60; ++i) {
        const qreal a = i * 0.32;
        const QPointF pt(170 + qCos(a) * (2 + i * 0.28), 60 + qSin(a) * (2 + i * 0.28));
        if (i == 0) swirl.moveTo(pt);
        else swirl.lineTo(pt);
    }
    p.drawPath(swirl);
    // A squiggle
    QPainterPath squiggle(QPointF(30, 190));
    for (int i = 1; i <= 8; ++i) squiggle.quadTo(QPointF(30 + i * 10 - 5, 190 + (i % 2 ? -10 : 10)), QPointF(30 + i * 10, 190));
    p.drawPath(squiggle);
    // A little heart
    QPainterPath heart;
    heart.moveTo(200, 200);
    heart.cubicTo(185, 185, 190, 172, 200, 182);
    heart.cubicTo(210, 172, 215, 185, 200, 200);
    p.drawPath(heart);
    // Dots
    p.setBrush(pencil);
    for (const QPointF &d : {QPointF(110, 120), QPointF(130, 230), QPointF(240, 130), QPointF(90, 20)}) {
        p.drawEllipse(d, 2.2, 2.2);
    }
    return tile;
}

QPixmap Sketch::spinePicture(const QColor &spine, const QColor &paper, const QColor &accent, const QSize &size) {
    QPixmap pix(size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF body(3, 3, size.width() - 8, size.height() - 8);
    const quint32 seed = seedFor(size, spine.rgb());
    const QPainterPath cover = wobblyRoundedRect(body, 4, seed, 0.8);
    drawWash(&p, cover, spine, QPointF(1, 1));
    const QRectF page = body.adjusted(body.width() * 0.32, 4, -3, -4);
    QPainterPath pagePath = wobblyRoundedRect(page, 2, seed + 1, 0.6);
    p.fillPath(pagePath, paper);
    p.fillPath(wobblyEllipse(QRectF(page.center().x() - 5, page.top() + 8, 10, 10), seed + 2, 0.4), accent);
    drawInk(&p, cover, kInk, 1.6);
    drawInk(&p, pagePath, kInk, 1.0);
    return pix;
}

// ---------------------------------------------------------------------------
// SketchPanel

SketchPanel::SketchPanel(QWidget *parent, const QColor &fill, int edges)
    : QWidget(parent),
      m_fill(fill),
      m_edges(edges) {
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void SketchPanel::setFill(const QColor &fill) {
    m_fill = fill;
    update();
}

void SketchPanel::setEdges(int edges) {
    m_edges = edges;
    update();
}

void SketchPanel::setDoodles(bool doodles) {
    m_doodles = doodles;
    update();
}

void SketchPanel::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.fillRect(event->rect(), m_fill);
    if (m_doodles) p.fillRect(event->rect(), QBrush(Sketch::doodleTile()));

    const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    const quint32 seed = Sketch::seedFor(size(), 7);
    if (m_edges & TopEdge) Sketch::drawInk(&p, Sketch::wobblyLine(r.topLeft(), r.topRight(), seed, 1.0), Sketch::kInk, 2.2);
    if (m_edges & BottomEdge) Sketch::drawInk(&p, Sketch::wobblyLine(r.bottomLeft(), r.bottomRight(), seed + 1, 1.0), Sketch::kInk, 2.2);
    if (m_edges & LeftEdge) Sketch::drawInk(&p, Sketch::wobblyLine(r.topLeft(), r.bottomLeft(), seed + 2, 1.0), Sketch::kInk, 2.2);
    if (m_edges & RightEdge) Sketch::drawInk(&p, Sketch::wobblyLine(r.topRight(), r.bottomRight(), seed + 3, 1.0), Sketch::kInk, 2.2);
}

// ---------------------------------------------------------------------------
// SketchStyle

SketchStyle::SketchStyle()
    : QProxyStyle(QStyleFactory::create("Fusion")) {
}

QPalette SketchStyle::standardPalette() const {
    QPalette pal;
    pal.setColor(QPalette::Window, Sketch::kPaper);
    pal.setColor(QPalette::WindowText, Sketch::kInk);
    pal.setColor(QPalette::Base, Sketch::kCard);
    pal.setColor(QPalette::AlternateBase, Sketch::kPaperDeep);
    pal.setColor(QPalette::Text, Sketch::kInk);
    pal.setColor(QPalette::Button, Sketch::kCard);
    pal.setColor(QPalette::ButtonText, Sketch::kInk);
    pal.setColor(QPalette::BrightText, Sketch::kTomato);
    pal.setColor(QPalette::Highlight, Sketch::kYellow);
    pal.setColor(QPalette::HighlightedText, Sketch::kInk);
    pal.setColor(QPalette::ToolTipBase, QColor("#fff3b0"));
    pal.setColor(QPalette::ToolTipText, Sketch::kInk);
    pal.setColor(QPalette::PlaceholderText, Sketch::kInkSoft);
    pal.setColor(QPalette::Link, Sketch::kTomato.darker(115));
    pal.setColor(QPalette::Light, Sketch::kCard);
    pal.setColor(QPalette::Midlight, Sketch::kPaperDeep);
    pal.setColor(QPalette::Mid, Sketch::kInkSoft);
    pal.setColor(QPalette::Dark, Sketch::kInkSoft);
    pal.setColor(QPalette::Shadow, Sketch::kInk);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        QColor faded = Sketch::kInkSoft;
        faded.setAlpha(150);
        pal.setColor(QPalette::Disabled, role, faded);
    }
    return pal;
}

void SketchStyle::polish(QPalette &palette) {
    palette = standardPalette();
}

void SketchStyle::polish(QWidget *widget) {
    QProxyStyle::polish(widget);
    if (qobject_cast<QAbstractButton*>(widget) || qobject_cast<QComboBox*>(widget)) {
        widget->setAttribute(Qt::WA_Hover, true);
    }
}

int SketchStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const {
    switch (metric) {
    case PM_ButtonMargin: return 14;
    case PM_DefaultFrameWidth: return 3;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight: return 22;
    case PM_SliderThickness: return 26;
    case PM_SliderLength: return 24;
    case PM_ScrollBarExtent: return 12;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical: return 2;
    case PM_MenuPanelWidth: return 4;
    case PM_MenuHMargin:
    case PM_MenuVMargin: return 4;
    case PM_FocusFrameHMargin:
    case PM_FocusFrameVMargin: return 0;
    default: return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QSize SketchStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const {
    QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton: {
        const QString role = widget ? widget->property("role").toString() : QString();
        if (role == "swatch" || role == "rail") return s;
        // Room for the paper shadow and a roomier, friendlier button.
        return s + QSize(14, 10);
    }
    case CT_ComboBox: return s + QSize(16, 8);
    case CT_LineEdit: return s + QSize(4, 10);
    case CT_MenuItem: return s + QSize(12, 6);
    default: return s;
    }
}

int SketchStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const {
    switch (hint) {
    case SH_DialogButtonBox_ButtonsHaveIcons: return 0;
    case SH_ToolTip_Mask: return 0;
    default: return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

void SketchStyle::drawButtonPanel(const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
    const QString role = widget ? widget->property("role").toString() : QString();
    const bool enabled = option->state & State_Enabled;
    const bool down = option->state & State_Sunken;
    const bool checked = option->state & State_On;
    const bool hover = enabled && (option->state & State_MouseOver);
    const quint32 seed = Sketch::seedFor(option->rect.size(), qHash(role));

    QColor ink = Sketch::kInk;
    if (!enabled) ink.setAlpha(90);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (role == "ghost" || role == "link") {
        if (hover || down) {
            QColor glow = Sketch::kYellow;
            glow.setAlpha(down ? 200 : 130);
            Sketch::drawWash(painter, Sketch::wobblyRoundedRect(QRectF(option->rect).adjusted(2, 3, -2, -3), 10, seed, 1.0), glow, QPointF());
        }
        painter->restore();
        return;
    }

    if (role == "swatch") {
        const QColor swatch = widget ? widget->property("swatch").value<QColor>() : Qt::white;
        const qreal d = qMin(option->rect.width(), option->rect.height()) - (checked ? 4 : 7);
        QRectF circle(0, 0, d, d);
        circle.moveCenter(QRectF(option->rect).center());
        if (hover && !checked) circle.adjust(-1.5, -1.5, 1.5, 1.5);
        const QPainterPath blob = Sketch::wobblyEllipse(circle, seed, 0.8);
        painter->fillPath(blob, swatch);
        Sketch::drawInk(painter, blob, ink, checked ? 3.0 : 1.6);
        if (checked) Sketch::drawInk(painter, Sketch::wobblyEllipse(circle.adjusted(-3, -3, 3, 3), seed + 5, 0.6), Sketch::kTomato, 1.4);
        painter->restore();
        return;
    }

    QColor wash = Sketch::kCard;
    const QVariant washProp = widget ? widget->property("wash") : QVariant();
    if (role == "primary") wash = Sketch::kYellow;
    else if (role == "danger") wash = Sketch::kBlush;
    if (washProp.isValid() && (role != "rail" || checked)) wash = washProp.value<QColor>();
    if (checked && !washProp.isValid() && (role == "chip" || role == "card" || role.isEmpty())) wash = Sketch::kYellow;
    if (hover && !down) wash = wash.lighter(104);
    if (!enabled) wash = QColor(wash.red(), wash.green(), wash.blue(), 120);

    QRectF r = QRectF(option->rect).adjusted(2, 2, -4, -4);
    const qreal radius = role == "chip" ? r.height() / 2.0 : (role == "rail" ? 12.0 : 10.0);

    // A cut-paper shadow that the button sinks into when pressed.
    const QPainterPath shape = Sketch::wobblyRoundedRect(r, radius, seed, 1.1);
    if (enabled && !down) {
        QColor shadow = Sketch::kInk;
        shadow.setAlpha(45);
        painter->fillPath(shape.translated(2.5, 2.5), shadow);
    }
    QPainterPath body = shape;
    if (down) body = shape.translated(2, 2);
    else if (hover) body = shape.translated(-0.5, -0.8);

    painter->fillPath(body, Sketch::kCard);
    Sketch::drawWash(painter, Sketch::wobblyRoundedRect(body.boundingRect().adjusted(1, 1, -1, -1), radius, seed + 3, 1.6), wash);
    Sketch::drawInk(painter, body, ink, checked ? 2.8 : 2.0);
    painter->restore();
}

void SketchStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
    switch (element) {
    case PE_PanelButtonCommand:
    case PE_PanelButtonBevel:
    case PE_PanelButtonTool:
        drawButtonPanel(option, painter, widget);
        return;
    case PE_FrameDefaultButton:
    case PE_FrameButtonBevel:
    case PE_FrameFocusRect:
    case PE_FrameLineEdit:
        return;

    case PE_PanelLineEdit: {
        const bool focus = option->state & State_HasFocus;
        const QRectF r = QRectF(option->rect).adjusted(2, 2, -2, -2);
        const quint32 seed = Sketch::seedFor(option->rect.size(), 11);
        const QPainterPath box = Sketch::wobblyRoundedRect(r, 9, seed, 0.9);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        if (focus) {
            QColor glow = Sketch::kYellow;
            glow.setAlpha(150);
            painter->fillPath(box.translated(2, 3), glow);
        }
        painter->fillPath(box, Sketch::kCard);
        Sketch::drawInk(painter, box, Sketch::kInk, focus ? 2.4 : 1.7);
        painter->restore();
        return;
    }

    case PE_IndicatorCheckBox: {
        const QRectF r = QRectF(option->rect).adjusted(2, 2, -2, -2);
        const quint32 seed = Sketch::seedFor(option->rect.size(), 21);
        const QPainterPath box = Sketch::wobblyRoundedRect(r, 4, seed, 0.7);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(box, Sketch::kCard);
        Sketch::drawInk(painter, box, Sketch::kInk, 1.8);
        if (option->state & State_On) {
            // A quick scribbled tick that overshoots the box, as if done in red pen.
            QPainterPath tick;
            tick.moveTo(r.left() + r.width() * 0.15, r.top() + r.height() * 0.55);
            tick.quadTo(r.left() + r.width() * 0.3, r.top() + r.height() * 0.7, r.left() + r.width() * 0.42, r.bottom() - 1);
            tick.quadTo(r.left() + r.width() * 0.7, r.top() + r.height() * 0.3, r.right() + 3, r.top() - 3);
            painter->setPen(QPen(Sketch::kTomato.darker(110), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawPath(tick);
        }
        painter->restore();
        return;
    }

    case PE_IndicatorRadioButton: {
        const QRectF r = QRectF(option->rect).adjusted(2, 2, -2, -2);
        const quint32 seed = Sketch::seedFor(option->rect.size(), 31);
        const QPainterPath ring = Sketch::wobblyEllipse(r, seed, 0.7);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(ring, Sketch::kCard);
        if (option->state & State_On) {
            painter->fillPath(Sketch::wobblyEllipse(r.adjusted(4, 4, -4, -4), seed + 1, 0.8), Sketch::kTeal);
        }
        Sketch::drawInk(painter, ring, Sketch::kInk, 1.8);
        painter->restore();
        return;
    }

    case PE_PanelMenu:
    case PE_FrameMenu: {
        const QRectF r = QRectF(option->rect).adjusted(1.5, 1.5, -1.5, -1.5);
        const QPainterPath card = Sketch::wobblyRoundedRect(r, 10, Sketch::seedFor(option->rect.size(), 41), 1.0);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        if (element == PE_PanelMenu) painter->fillPath(card, Sketch::kCard);
        Sketch::drawInk(painter, card, Sketch::kInk, 2.0);
        painter->restore();
        return;
    }

    case PE_PanelTipLabel: {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(option->rect).adjusted(1, 1, -1, -1);
        const QPainterPath note = Sketch::wobblyRoundedRect(r, 6, Sketch::seedFor(option->rect.size(), 51), 0.8);
        painter->fillPath(note, QColor("#fff3b0"));
        Sketch::drawInk(painter, note, Sketch::kInk, 1.5);
        painter->restore();
        return;
    }

    case PE_FrameGroupBox: {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(option->rect).adjusted(2, 2, -2, -2);
        Sketch::drawInk(painter, Sketch::wobblyRoundedRect(r, 12, Sketch::seedFor(option->rect.size(), 61), 1.0), Sketch::kInk, 1.8);
        painter->restore();
        return;
    }

    case PE_PanelItemViewItem: {
        const auto *item = qstyleoption_cast<const QStyleOptionViewItem*>(option);
        const bool selected = option->state & State_Selected;
        const bool hover = option->state & State_MouseOver;
        if (!selected && !hover) {
            if (item && item->backgroundBrush.style() != Qt::NoBrush) painter->fillRect(option->rect, item->backgroundBrush);
            return;
        }
        QColor wash = Sketch::kYellow;
        wash.setAlpha(selected ? 220 : 90);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        Sketch::drawWash(painter, Sketch::wobblyRoundedRect(QRectF(option->rect).adjusted(2, 2, -2, -2), 8,
                                                            Sketch::seedFor(option->rect.size(), 71), 1.0), wash, QPointF());
        painter->restore();
        return;
    }

    case PE_IndicatorArrowDown: {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(option->rect);
        const QPointF c = r.center();
        const qreal s = qMin(r.width(), r.height()) * 0.32;
        QPainterPath chevron(QPointF(c.x() - s, c.y() - s * 0.4));
        chevron.quadTo(QPointF(c.x() - s * 0.4, c.y() + s * 0.1), QPointF(c.x(), c.y() + s * 0.6));
        chevron.quadTo(QPointF(c.x() + s * 0.5, c.y()), QPointF(c.x() + s * 1.05, c.y() - s * 0.5));
        painter->setPen(QPen(option->state & State_Enabled ? Sketch::kInk : Sketch::kInkSoft, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(chevron);
        painter->restore();
        return;
    }

    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
}

void SketchStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
    switch (element) {
    case CE_PushButtonBevel:
        drawButtonPanel(option, painter, widget);
        return;

    case CE_ShapedFrame: {
        const auto *frame = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frame || frame->frameShape == QFrame::NoFrame || frame->frameShape == QFrame::HLine || frame->frameShape == QFrame::VLine) {
            QProxyStyle::drawControl(element, option, painter, widget);
            return;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(option->rect).adjusted(1.5, 1.5, -1.5, -1.5);
        Sketch::drawInk(painter, Sketch::wobblyRoundedRect(r, 10, Sketch::seedFor(option->rect.size(), 81), 1.0), Sketch::kInk, 1.8);
        painter->restore();
        return;
    }

    case CE_MenuItem: {
        const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
        if (menuItem && (menuItem->state & State_Selected) && (menuItem->state & State_Enabled)
            && menuItem->menuItemType != QStyleOptionMenuItem::Separator) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            QColor wash = Sketch::kYellow;
            wash.setAlpha(220);
            Sketch::drawWash(painter, Sketch::wobblyRoundedRect(QRectF(option->rect).adjusted(3, 1, -3, -1), 8,
                                                                Sketch::seedFor(option->rect.size(), 91), 0.8), wash, QPointF());
            painter->restore();
            QStyleOptionMenuItem plain = *menuItem;
            plain.state &= ~State_Selected;
            QProxyStyle::drawControl(element, &plain, painter, widget);
            return;
        }
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    case CE_ProgressBarGroove: {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(option->rect).adjusted(1, 1, -1, -1);
        const QPainterPath groove = Sketch::wobblyRoundedRect(r, r.height() / 2, Sketch::seedFor(option->rect.size(), 101), 0.5);
        painter->fillPath(groove, Sketch::kPaperDeep);
        Sketch::drawInk(painter, groove, Sketch::kInk, 1.2);
        painter->restore();
        return;
    }

    default:
        QProxyStyle::drawControl(element, option, painter, widget);
    }
}

void SketchStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const {
    switch (control) {
    case CC_ComboBox: {
        drawButtonPanel(option, painter, widget);
        QStyleOption arrow = *option;
        arrow.rect = subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget).adjusted(0, 0, -4, -3);
        drawPrimitive(PE_IndicatorArrowDown, &arrow, painter, widget);
        return;
    }

    case CC_Slider: {
        const auto *slider = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (!slider || slider->orientation != Qt::Horizontal) break;
        const QRect groove = subControlRect(CC_Slider, option, SC_SliderGroove, widget);
        const QRect handle = subControlRect(CC_Slider, option, SC_SliderHandle, widget);
        const qreal y = groove.center().y() + 0.5;
        const quint32 seed = Sketch::seedFor(option->rect.size(), 111);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        // The filled part is a thick teal brush stroke; the rest is a pencil line.
        painter->setPen(QPen(Sketch::kTeal, 7, Qt::SolidLine, Qt::RoundCap));
        painter->drawPath(Sketch::wobblyLine(QPointF(groove.left() + 4, y + 1), QPointF(handle.center().x(), y + 1), seed, 0.8));
        Sketch::drawInk(painter, Sketch::wobblyLine(QPointF(groove.left() + 4, y), QPointF(groove.right() - 4, y), seed + 1, 0.9), Sketch::kInk, 1.8);

        const bool hover = slider->state & State_MouseOver;
        const qreal d = qMin(handle.width(), handle.height()) - (hover ? 2 : 4);
        QRectF knob(0, 0, d, d);
        knob.moveCenter(QRectF(handle).center());
        const QPainterPath blob = Sketch::wobblyEllipse(knob, seed + 2, 0.7);
        QColor shadow = Sketch::kInk;
        shadow.setAlpha(45);
        painter->fillPath(blob.translated(2, 2), shadow);
        painter->fillPath(blob, Sketch::kYellow);
        Sketch::drawInk(painter, blob, Sketch::kInk, 2.0);
        painter->restore();
        return;
    }

    case CC_GroupBox: {
        const auto *box = qstyleoption_cast<const QStyleOptionGroupBox*>(option);
        if (!box) break;
        // A hand-drawn frame with its title sitting in a gap in the top line.
        const QRect label = subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
        const QRectF frame = QRectF(option->rect).adjusted(2, label.height() / 2.0 + 1, -2, -2);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        Sketch::drawInk(painter, Sketch::wobblyRoundedRect(frame, 12, Sketch::seedFor(option->rect.size(), 61), 1.0), Sketch::kInk, 1.8);
        if (!box->text.isEmpty()) {
            const QRectF patch = QRectF(label).adjusted(-8, 0, 8, 0);
            painter->fillRect(patch, widget ? widget->palette().color(QPalette::Window) : Sketch::kPaper);
            painter->setFont(widget ? widget->font() : Sketch::headingFont(18));
            painter->setPen(Sketch::kTomato.darker(130));
            painter->drawText(label, Qt::AlignLeft | Qt::AlignVCenter, box->text);
        }
        painter->restore();
        return;
    }

    case CC_ScrollBar: {
        const QRect handle = subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget).adjusted(2, 2, -2, -2);
        if (handle.isEmpty()) return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QColor fill = Sketch::kInk;
        fill.setAlpha((option->state & State_MouseOver) ? 110 : 60);
        const qreal radius = qMin(handle.width(), handle.height()) / 2.0;
        painter->fillPath(Sketch::wobblyRoundedRect(handle, radius, Sketch::seedFor(handle.size(), 121), 0.5), fill);
        painter->restore();
        return;
    }

    default:
        break;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

// ---------------------------------------------------------------------------

void Sketch::install() {
    auto *style = new SketchStyle();
    QApplication::setStyle(style);
    QApplication::setPalette(style->standardPalette());

    QApplication::setFont(bodyFont(16));
    for (const char *cls : {"QPushButton", "QGroupBox", "QComboBox", "QMenu", "QRadioButton", "QCheckBox"}) {
        QApplication::setFont(headingFont(18), cls);
    }
    QApplication::setFont(bodyFont(17), "QLineEdit");
    QApplication::setFont(bodyFont(17), "QPlainTextEdit");
    QApplication::setFont(bodyFont(15), "QToolTip");
}
