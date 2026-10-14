#pragma once

#include <QPainter>
#include <QPixmap>
#include "types.h"

// Paints a saved spread (paper, decorations, page numbers and artwork) without the editor.
// Shared by PDF export, the page sorter thumbnails and the bookshelf covers.
namespace SpreadRenderer {
    constexpr qreal kPageWidth = BookFormat::kPageUnitsWide;

    inline QRectF spreadRect(qreal pageHeight) { return QRectF(0, 0, kPageWidth * 2, pageHeight); }
    inline QRectF leftPageRect(qreal pageHeight) { return QRectF(0, 0, kPageWidth, pageHeight); }
    inline QRectF rightPageRect(qreal pageHeight) { return QRectF(kPageWidth, 0, kPageWidth, pageHeight); }

    // Paints `source` (in spread coordinates, which may reach past the paper into the
    // bleed) into `target` on the painter.
    void paint(QPainter &painter, const BookTheme &theme, const BookSpread &spread, qreal pageHeight,
               const QRectF &source, const QRectF &target, bool showPageNumbers, bool drawGutter);

    QPixmap thumbnail(const BookTheme &theme, const BookSpread &spread, qreal pageHeight,
                      const QRectF &source, int width, bool showPageNumbers);
}
