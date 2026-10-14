#pragma once

#include <QPainter>
#include <QPixmap>
#include "types.h"

// Paints a saved spread (paper, decorations, page numbers and artwork) without the editor.
// Shared by PDF export, the page sorter thumbnails and the bookshelf covers.
namespace SpreadRenderer {
    constexpr qreal kPageWidth = 600.0;
    constexpr qreal kPageHeight = 650.0;

    inline QRectF spreadRect() { return QRectF(0, 0, kPageWidth * 2, kPageHeight); }
    inline QRectF leftPageRect() { return QRectF(0, 0, kPageWidth, kPageHeight); }
    inline QRectF rightPageRect() { return QRectF(kPageWidth, 0, kPageWidth, kPageHeight); }

    // Paints `source` (in spread coordinates) into `target` on the painter.
    void paint(QPainter &painter, const BookTheme &theme, const BookSpread &spread,
               const QRectF &source, const QRectF &target, bool showPageNumbers, bool drawGutter);

    QPixmap thumbnail(const BookTheme &theme, const BookSpread &spread, const QRectF &source,
                      int width, bool showPageNumbers);
}
