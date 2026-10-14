#include "spread_renderer.h"
#include "theme_decors.h"
#include "canvas_items.h"
#include <QGraphicsScene>

void SpreadRenderer::paint(QPainter &painter, const BookTheme &theme, const BookSpread &spread,
                           const QRectF &source, const QRectF &target, bool showPageNumbers, bool drawGutter) {
    painter.save();
    painter.setClipRect(target, Qt::IntersectClip);
    painter.translate(target.topLeft());
    painter.scale(target.width() / source.width(), target.height() / source.height());
    painter.translate(-source.topLeft());

    painter.fillRect(spreadRect(), theme.paperBg);

    painter.setPen(QPen(theme.borderColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(leftPageRect().adjusted(12, 12, -12, -12));
    painter.drawRect(rightPageRect().adjusted(12, 12, -12, -12));

    ThemeDecors::draw(&painter, theme);

    if (showPageNumbers && !spread.isCover()) {
        ThemeDecors::drawPageNumbers(&painter, theme, spread.leftPageNum, spread.rightPageNum);
    }

    if (drawGutter) {
        painter.setPen(QPen(QColor(0, 0, 0, 100), 1));
        painter.drawLine(QPointF(kPageWidth, 0), QPointF(kPageWidth, kPageHeight));
    }
    painter.restore();

    QGraphicsScene scene;
    CanvasSerializer::loadIntoScene(&scene, spread.canvasData);
    scene.render(&painter, target, source, Qt::IgnoreAspectRatio);
}

QPixmap SpreadRenderer::thumbnail(const BookTheme &theme, const BookSpread &spread, const QRectF &source,
                                  int width, bool showPageNumbers) {
    const int height = qRound(width * source.height() / source.width());
    QPixmap pix(width, height);
    pix.fill(theme.paperBg);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    paint(painter, theme, spread, source, QRectF(0, 0, width, height), showPageNumbers, source.width() > kPageWidth);
    return pix;
}
