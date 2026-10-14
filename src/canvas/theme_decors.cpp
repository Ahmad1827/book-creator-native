#include "theme_decors.h"
#include "sketch.h"
#include <QRandomGenerator>
#include <QtMath>

namespace {

constexpr qreal kPageW = 600.0;
constexpr qreal kInset = 14.0;

QColor withAlpha(QColor color, int alpha) {
    color.setAlpha(alpha);
    return color;
}

// A watercolour wash under an ink line, the way every doodle in the book is drawn.
void inked(QPainter *p, const QPainterPath &path, const QColor &fill, const QColor &ink, qreal width = 1.6) {
    if (fill.isValid()) Sketch::drawWash(p, path, withAlpha(fill, 215), QPointF(1.2, 1.4));
    Sketch::drawInk(p, path, ink, width);
}

QPainterPath smoothClosed(const QPolygonF &pts) {
    QPainterPath path;
    auto mid = [](const QPointF &a, const QPointF &b) { return (a + b) / 2.0; };
    path.moveTo(mid(pts.last(), pts.first()));
    for (int i = 0; i < pts.size(); ++i) path.quadTo(pts[i], mid(pts[i], pts[(i + 1) % pts.size()]));
    path.closeSubpath();
    return path;
}

// A paint splat with a few flicked droplets around it.
QPainterPath splat(const QPointF &c, qreal r, quint32 seed) {
    QRandomGenerator rng(seed);
    QPolygonF pts;
    const int n = 13;
    for (int i = 0; i < n; ++i) {
        const qreal a = 2 * M_PI * i / n;
        qreal rr = r * (0.78 + 0.3 * rng.generateDouble());
        if (i % 4 == 1) rr *= 1.3;
        pts << c + QPointF(qCos(a) * rr, qSin(a) * rr);
    }
    QPainterPath path = smoothClosed(pts);
    for (int i = 0; i < 4; ++i) {
        const qreal a = 2 * M_PI * rng.generateDouble();
        const qreal d = r * (1.45 + 0.5 * rng.generateDouble());
        const qreal dr = r * (0.07 + 0.09 * rng.generateDouble());
        path.addEllipse(c + QPointF(qCos(a) * d, qSin(a) * d), dr, dr);
    }
    return path;
}

void drawSplat(QPainter *p, const QPointF &c, qreal r, const QColor &color, quint32 seed) {
    const QPainterPath path = splat(c, r, seed);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(withAlpha(color.darker(118), 150), 1.1));
    p->setBrush(withAlpha(color, 175));
    p->drawPath(path);
    p->restore();
}

QPainterPath star(const QPointF &c, qreal r, qreal spinDeg = 0, int points = 5, qreal inner = 0.45) {
    QPolygonF poly;
    for (int i = 0; i < points * 2; ++i) {
        const qreal a = M_PI * i / points - M_PI / 2 + qDegreesToRadians(spinDeg);
        const qreal rr = (i % 2) ? r * inner : r;
        poly << c + QPointF(qCos(a) * rr, qSin(a) * rr);
    }
    QPainterPath path;
    path.addPolygon(poly);
    path.closeSubpath();
    return path;
}

QPainterPath spiral(const QPointF &c, qreal r, qreal turns, qreal startDeg = 0) {
    QPainterPath path;
    const int steps = int(turns * 28);
    for (int i = 0; i <= steps; ++i) {
        const qreal t = qreal(i) / steps;
        const qreal a = qDegreesToRadians(startDeg) + 2 * M_PI * turns * t;
        const QPointF pt = c + QPointF(qCos(a), qSin(a)) * (r * t);
        if (i == 0) path.moveTo(pt); else path.lineTo(pt);
    }
    return path;
}

QPainterPath leaf(const QPointF &base, qreal angleDeg, qreal length, qreal width) {
    QPainterPath path;
    path.moveTo(0, 0);
    path.quadTo(length * 0.45, -width, length, 0);
    path.quadTo(length * 0.55, width, 0, 0);
    path.closeSubpath();
    QTransform t;
    t.translate(base.x(), base.y());
    t.rotate(angleDeg);
    return t.map(path);
}

QPainterPath leafVein(const QPointF &base, qreal angleDeg, qreal length) {
    QPainterPath path;
    path.moveTo(length * 0.1, 0);
    path.quadTo(length * 0.5, -1.5, length * 0.85, 0);
    QTransform t;
    t.translate(base.x(), base.y());
    t.rotate(angleDeg);
    return t.map(path);
}

void drawLeaf(QPainter *p, const QPointF &base, qreal angleDeg, qreal length, qreal width,
              const QColor &fill, const QColor &ink) {
    inked(p, leaf(base, angleDeg, length, width), fill, ink, 1.4);
    Sketch::drawInk(p, leafVein(base, angleDeg, length), withAlpha(ink, 150), 1.0);
}

// ---------------------------------------------------------------------------
// One page's doodles, drawn for a left page: the outer edge at x = 0 and the spine at x = 600.

void scribbleDecor(QPainter *p, const BookTheme &t, qreal h, qreal w, qreal inset, quint32 salt) {
    const QColor ink = t.borderColor;
    drawSplat(p, QPointF(38, 38), 21, t.accentColor, 11 + salt);
    drawSplat(p, QPointF(78, 24), 7, t.frameColor, 12 + salt);
    Sketch::drawInk(p, star(QPointF(46, 40), 11, -8), ink, 1.6);
    Sketch::drawInk(p, star(QPointF(96, 44), 5, 14), ink, 1.3);

    drawSplat(p, QPointF(34, h - 38), 15, t.frameColor, 13 + salt);
    drawSplat(p, QPointF(62, h - 24), 6, t.accentColor, 14 + salt);
    Sketch::drawInk(p, spiral(QPointF(36, h - 40), 11, 2.2, 30), ink, 1.4);

    drawSplat(p, QPointF(w - 34, 32), 6, t.accentColor, 15 + salt);
    Sketch::drawInk(p, star(QPointF(w - 52, 34), 6, 20), ink, 1.3);
}

void dreamsDecor(QPainter *p, const BookTheme &t, qreal h, qreal w, qreal inset, quint32 salt) {
    const QColor ink = t.borderColor;
    QPainterPath moon;
    moon.addEllipse(QPointF(46, 46), 21, 21);
    QPainterPath bite;
    bite.addEllipse(QPointF(56, 38), 18, 18);
    inked(p, moon.subtracted(bite), t.accentColor, ink, 1.6);

    const QPointF stars[] = {{92, 30}, {118, 52}, {30, 96}, {150, 28}};
    const qreal sizes[] = {8, 5, 6, 4};
    for (int i = 0; i < 4; ++i) inked(p, star(stars[i], sizes[i], i * 17 + salt * 9), t.accentColor, ink, 1.2);

    // Dreams float up from the bottom corner in bubbles.
    const QPointF bubbles[] = {{40, h - 44}, {70, h - 74}, {32, h - 98}, {58, h - 118}};
    const qreal radii[] = {17, 10, 7, 4};
    for (int i = 0; i < 4; ++i) {
        QPainterPath bubble;
        bubble.addEllipse(bubbles[i], radii[i], radii[i]);
        inked(p, bubble, withAlpha(t.frameColor, 130), withAlpha(ink, 200), 1.3);
        QPainterPath shine;
        shine.arcMoveTo(QRectF(bubbles[i] - QPointF(radii[i] * 0.6, radii[i] * 0.6), QSizeF(radii[i] * 1.2, radii[i] * 1.2)), 100);
        shine.arcTo(QRectF(bubbles[i] - QPointF(radii[i] * 0.6, radii[i] * 0.6), QSizeF(radii[i] * 1.2, radii[i] * 1.2)), 100, 60);
        Sketch::drawInk(p, shine, withAlpha(QColor(Qt::white), 210), 1.4);
    }
    inked(p, star(QPointF(46, h - 44), 6, 10), t.accentColor, ink, 1.0);

    const QPointF sparkles[] = {{w - 40, 36}, {w - 64, 58}, {w - 34, h - 40}};
    for (const auto &s : sparkles) inked(p, star(s, 7, 0, 4, 0.28), t.accentColor, ink, 1.1);
}

void sweetsDecor(QPainter *p, const BookTheme &t, qreal h, qreal w, qreal inset, quint32 salt) {
    const QColor ink = t.borderColor;

    // A candy-striped corner.
    QPainterPath corner;
    corner.moveTo(inset, inset);
    corner.lineTo(84, inset);
    corner.quadTo(52, 30, inset, 84);
    corner.closeSubpath();
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setClipPath(corner);
    p->fillPath(corner, withAlpha(Qt::white, 200));
    for (qreal k = 20; k < 180; k += 16) {
        QPainterPath band;
        band.moveTo(k, 0);
        band.lineTo(k + 8, 0);
        band.lineTo(0, k + 8);
        band.lineTo(0, k);
        band.closeSubpath();
        p->fillPath(band, withAlpha(t.accentColor, 220));
    }
    p->restore();
    Sketch::drawInk(p, corner, ink, 1.6);

    // A wrapped sweet in the bottom corner.
    p->save();
    p->translate(50, h - 42);
    p->rotate(-24 + salt * 10);
    QPainterPath body;
    body.addEllipse(QPointF(0, 0), 15, 11);
    QPainterPath wings;
    wings.moveTo(-13, 0); wings.lineTo(-30, -11); wings.quadTo(-26, 0, -30, 11); wings.closeSubpath();
    wings.moveTo(13, 0); wings.lineTo(30, -11); wings.quadTo(26, 0, 30, 11); wings.closeSubpath();
    inked(p, wings, t.accentColor, ink, 1.4);
    inked(p, body, t.frameColor, ink, 1.6);
    Sketch::drawInk(p, spiral(QPointF(0, 0), 7, 1.4), withAlpha(ink, 160), 1.1);
    p->restore();

    // A lollipop by the spine.
    QPainterPath stick;
    stick.moveTo(w - 40, 70);
    stick.lineTo(w - 26, 108);
    Sketch::drawInk(p, stick, ink, 2.2);
    QPainterPath pop;
    pop.addEllipse(QPointF(w - 44, 56), 16, 16);
    inked(p, pop, t.accentColor, ink, 1.6);
    Sketch::drawInk(p, spiral(QPointF(w - 44, 56), 13, 2.4, salt * 60.0), withAlpha(Qt::white, 230), 2.2);

    // Sprinkles.
    QRandomGenerator rng(40 + salt);
    const QColor sprinkleColours[] = {t.accentColor, t.frameColor, QColor("#ffd25a"), QColor("#7cc0ee")};
    const QRectF areas[] = {QRectF(90, 22, 70, 30), QRectF(22, 92, 30, 60), QRectF(84, h - 50, 60, 26)};
    for (const auto &area : areas) {
        for (int i = 0; i < 5; ++i) {
            const QPointF c(area.left() + area.width() * rng.generateDouble(), area.top() + area.height() * rng.generateDouble());
            const qreal a = 180 * rng.generateDouble();
            QPainterPath s;
            s.addRoundedRect(QRectF(-4.5, -1.6, 9, 3.2), 1.6, 1.6);
            QTransform tr;
            tr.translate(c.x(), c.y());
            tr.rotate(a);
            p->save();
            p->setRenderHint(QPainter::Antialiasing, true);
            p->fillPath(tr.map(s), sprinkleColours[i % 4]);
            p->restore();
        }
    }
}

void gardenDecor(QPainter *p, const BookTheme &t, qreal h, qreal w, qreal inset, quint32 salt) {
    const QColor ink = t.borderColor;
    const QColor green = t.frameColor;

    // Leaves spilling out of the top corner, with a peach-coloured blossom.
    drawLeaf(p, QPointF(12, 12), 18, 62, 13, green, ink);
    drawLeaf(p, QPointF(12, 12), 52, 54, 12, green.darker(112), ink);
    drawLeaf(p, QPointF(12, 12), 82, 44, 10, green, ink);
    for (int i = 0; i < 5; ++i) {
        const qreal a = 2 * M_PI * i / 5 + salt;
        QPainterPath petal;
        petal.addEllipse(QPointF(44, 40) + QPointF(qCos(a), qSin(a)) * 7, 6, 6);
        inked(p, petal, t.accentColor, ink, 1.1);
    }
    QPainterPath middle;
    middle.addEllipse(QPointF(44, 40), 4, 4);
    inked(p, middle, QColor("#ffd25a"), ink, 1.0);

    // Grass tufts along the bottom of the page, clear of the page number.
    QRandomGenerator rng(70 + salt);
    const qreal base = h - inset + 1;
    auto tuft = [&](qreal x0, qreal x1) {
        for (qreal x = x0; x < x1; x += 7 + 5 * rng.generateDouble()) {
            const qreal height = 14 + 16 * rng.generateDouble();
            const qreal lean = -8 + 16 * rng.generateDouble();
            QPainterPath blade;
            blade.moveTo(x - 3, base);
            blade.quadTo(x + lean * 0.3, base - height * 0.6, x + lean, base - height);
            blade.quadTo(x + lean * 0.3 + 2.5, base - height * 0.5, x + 3, base);
            blade.closeSubpath();
            inked(p, blade, green, withAlpha(ink, 210), 1.1);
        }
    };
    tuft(18, 150);
    tuft(w - 130, w - 16);

    // A ladybird out for a stroll.
    const QPointF bug(118, h - 40);
    QPainterPath head;
    head.addEllipse(bug + QPointF(10, -1), 5, 5);
    inked(p, head, ink, ink, 1.0);
    QPainterPath shell;
    shell.addEllipse(bug, 11, 9);
    inked(p, shell, QColor("#e8553e"), ink, 1.5);
    Sketch::drawInk(p, Sketch::wobblyLine(bug + QPointF(-10, 0), bug + QPointF(10, 0), 3, 0.3), ink, 1.0);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(ink);
    for (const QPointF &d : {QPointF(-5, -4), QPointF(4, -4), QPointF(-4, 4), QPointF(5, 3)}) p->drawEllipse(bug + d, 1.9, 1.9);
    p->restore();
}

void woodsDecor(QPainter *p, const BookTheme &t, qreal h, qreal w, qreal inset, quint32 salt) {
    const QColor ink = t.borderColor;

    // A twisty twig across the top corner with two little leaves.
    QPainterPath twig;
    twig.moveTo(inset, 108);
    twig.cubicTo(40, 70, 30, 40, 70, 30);
    twig.cubicTo(96, 24, 104, 18, 128, inset);
    Sketch::drawInk(p, twig, ink, 2.4);
    QPainterPath branch;
    branch.moveTo(34, 58);
    branch.quadTo(52, 56, 58, 70);
    branch.moveTo(88, 24);
    branch.quadTo(96, 36, 110, 38);
    Sketch::drawInk(p, branch, ink, 1.6);
    drawLeaf(p, QPointF(58, 70), 60, 22, 7, t.accentColor, ink);
    drawLeaf(p, QPointF(110, 38), 10, 22, 7, t.frameColor, ink);
    Sketch::drawInk(p, spiral(QPointF(150, 30), 9, 1.8, salt * 90.0), withAlpha(t.accentColor.darker(130), 220), 1.5);

    // A spotty toadstool in the bottom corner.
    const qreal ground = h - inset - 2;
    QPainterPath stem;
    stem.moveTo(38, ground);
    stem.quadTo(36, ground - 16, 42, ground - 24);
    stem.lineTo(54, ground - 24);
    stem.quadTo(58, ground - 14, 56, ground);
    stem.closeSubpath();
    inked(p, stem, QColor("#fff4dc"), ink, 1.5);
    QPainterPath cap;
    cap.moveTo(22, ground - 22);
    cap.cubicTo(24, ground - 52, 72, ground - 52, 74, ground - 22);
    cap.quadTo(48, ground - 28, 22, ground - 22);
    cap.closeSubpath();
    inked(p, cap, t.frameColor, ink, 1.7);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(withAlpha(Qt::white, 235));
    p->drawEllipse(QPointF(38, ground - 36), 4, 3);
    p->drawEllipse(QPointF(56, ground - 40), 3, 2.5);
    p->drawEllipse(QPointF(62, ground - 29), 2.5, 2);
    p->restore();
    QPainterPath small;
    small.moveTo(80, ground);
    small.lineTo(81, ground - 10);
    small.moveTo(72, ground - 10);
    small.cubicTo(73, ground - 22, 90, ground - 22, 91, ground - 10);
    small.closeSubpath();
    inked(p, small, t.accentColor, ink, 1.3);

    Sketch::drawInk(p, spiral(QPointF(w - 40, 40), 10, 2.0, 200), withAlpha(t.accentColor.darker(130), 220), 1.5);
    Sketch::drawInk(p, spiral(QPointF(w - 36, h - 42), 8, 1.8, 40), withAlpha(t.frameColor.darker(120), 220), 1.4);
}

void seasideDecor(QPainter *p, const BookTheme &t, qreal h, qreal w, qreal inset, quint32 salt) {
    const QColor ink = t.borderColor;

    // A cheery sun.
    const QPointF sun(46, 46);
    QPainterPath rays;
    for (int i = 0; i < 10; ++i) {
        const qreal a = 2 * M_PI * i / 10 + 0.2;
        rays.moveTo(sun + QPointF(qCos(a), qSin(a)) * 23);
        rays.lineTo(sun + QPointF(qCos(a), qSin(a)) * (i % 2 ? 30 : 34));
    }
    Sketch::drawInk(p, rays, ink, 1.6);
    QPainterPath disc;
    disc.addEllipse(sun, 18, 18);
    inked(p, disc, t.frameColor, ink, 1.7);

    // Seagulls.
    auto gull = [&](const QPointF &c, qreal s) {
        QPainterPath g;
        g.moveTo(c - QPointF(s, 0));
        g.quadTo(c - QPointF(s * 0.5, s * 0.7), c);
        g.quadTo(c + QPointF(s * 0.5, -s * 0.7), c + QPointF(s, 0));
        Sketch::drawInk(p, g, ink, 1.6);
    };
    gull(QPointF(104, 36), 10);
    gull(QPointF(130, 54), 7);
    gull(QPointF(w - 54, 42), 8);

    // Rolling waves along the outer half of the bottom edge.
    const qreal base = h - inset;
    auto waveLine = [&](qreal top, qreal phase) {
        QPainterPath wave;
        wave.moveTo(inset, top);
        for (qreal x = inset; x <= 190; x += 3) {
            // The sea runs down into the border towards its end instead of stopping short.
            const qreal fade = qBound(0.0, (x - 120) / 70.0, 1.0);
            const qreal y = top + 4 * qSin((x + phase) / 11.0);
            wave.lineTo(x, y + (base - y) * fade * fade);
        }
        return wave;
    };
    QPainterPath sea = waveLine(base - 26, salt * 7.0);
    sea.lineTo(190, base);
    sea.lineTo(inset, base);
    sea.closeSubpath();
    Sketch::drawWash(p, sea, withAlpha(t.accentColor, 190), QPointF(0, 1));
    Sketch::drawInk(p, waveLine(base - 26, salt * 7.0), ink, 1.6);
    Sketch::drawInk(p, waveLine(base - 13, salt * 7.0 + 15), withAlpha(ink, 140), 1.2);

    // A starfish washed up by the spine.
    inked(p, star(QPointF(w - 44, h - 42), 13, 12 + salt * 20, 5, 0.42), QColor("#ff9b54"), ink, 1.5);
}

void decoratePage(QPainter *p, const BookTheme &t, qreal h, quint32 salt) {
    // Doodles are drawn a size up from their sketch coordinates; the page is described
    // to them shrunk to match, so corner pieces stay tucked into their corners.
    constexpr qreal k = 1.2;
    p->save();
    p->scale(k, k);
    h /= k;
    const qreal w = kPageW / k;
    const qreal inset = kInset / k;
    switch (t.decorType) {
        case DecorType::Scribble: scribbleDecor(p, t, h, w, inset, salt); break;
        case DecorType::Dreams: dreamsDecor(p, t, h, w, inset, salt); break;
        case DecorType::Sweets: sweetsDecor(p, t, h, w, inset, salt); break;
        case DecorType::Garden: gardenDecor(p, t, h, w, inset, salt); break;
        case DecorType::Woods: woodsDecor(p, t, h, w, inset, salt); break;
        case DecorType::Seaside: seasideDecor(p, t, h, w, inset, salt); break;
    }
    p->restore();
}

// A loose double ink border, as if drawn twice in a hurry.
void drawBorder(QPainter *p, const BookTheme &t, const QRectF &page, quint32 salt) {
    const QRectF r = page.adjusted(kInset, kInset, -kInset, -kInset);
    Sketch::drawInk(p, Sketch::wobblyRoundedRect(r, 6, 101 + salt, 1.5), withAlpha(t.borderColor, 200), 1.7);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(withAlpha(t.borderColor, 80), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(Sketch::wobblyRoundedRect(r.adjusted(-2, 2, 2, -1), 8, 202 + salt, 2.4));
    p->restore();
}

} // namespace

void ThemeDecors::draw(QPainter *painter, const BookTheme &theme, qreal pageHeight) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    drawBorder(painter, theme, QRectF(0, 0, kPageW, pageHeight), 0);
    drawBorder(painter, theme, QRectF(kPageW, 0, kPageW, pageHeight), 1);

    decoratePage(painter, theme, pageHeight, 0);

    // The right page is the left one seen in a mirror, so doodles sit in the outer corners.
    painter->save();
    painter->translate(2 * kPageW, 0);
    painter->scale(-1, 1);
    decoratePage(painter, theme, pageHeight, 1);
    painter->restore();

    painter->restore();
}

void ThemeDecors::drawPageNumbers(QPainter *painter, const BookTheme &theme, int leftNum, int rightNum, qreal pageHeight) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont font(theme.recommendedFont);
    font.setPixelSize(17);
    painter->setFont(font);

    auto number = [&](int num, qreal centerX) {
        if (num <= 0) return;
        const QPointF c(centerX, pageHeight - 33);
        const QPainterPath blob = Sketch::wobblyEllipse(QRectF(c - QPointF(17, 13), QSizeF(34, 26)), 300 + num, 1.2);
        Sketch::drawWash(painter, blob, withAlpha(theme.accentColor, 120), QPointF(1, 1));
        Sketch::drawInk(painter, blob, withAlpha(theme.borderColor, 120), 1.1);
        painter->setPen(theme.inkColor);
        painter->drawText(QRectF(c - QPointF(30, 13), QSizeF(60, 26)), Qt::AlignCenter, QString::number(num));
    };
    number(leftNum, kPageW / 2);
    number(rightNum, kPageW * 1.5);
    painter->restore();
}
