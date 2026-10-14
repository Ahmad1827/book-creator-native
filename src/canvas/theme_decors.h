#pragma once

#include <QPainter>
#include "types.h"

// Each theme's page dressing: a scratchy ink border and watercolour doodles in the corners,
// drawn the same way on screen, in thumbnails and in the printed PDF.
class ThemeDecors {
public:
    static void draw(QPainter *painter, const BookTheme &theme, qreal pageHeight = 650.0);
    // Draws page numbers at the foot of each page; 0 means no number on that side.
    static void drawPageNumbers(QPainter *painter, const BookTheme &theme, int leftNum, int rightNum, qreal pageHeight = 650.0);
};
