#pragma once

#include <QPainter>
#include "types.h"

class ThemeDecors {
public:
    static void draw(QPainter *painter, const BookTheme &theme, qreal pageHeight = 650.0);
    // Draws page numbers at the foot of each page; 0 means no number on that side.
    static void drawPageNumbers(QPainter *painter, const BookTheme &theme, int leftNum, int rightNum, qreal pageHeight = 650.0);

private:
    static QString getCornerSvg(const BookTheme &theme, const QString &placement);
};