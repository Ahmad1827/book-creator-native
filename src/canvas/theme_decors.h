#pragma once

#include <QPainter>
#include "types.h"

class ThemeDecors {
public:
    static void draw(QPainter *painter, const BookTheme &theme);
    // Draws page numbers at the foot of each page; 0 means no number on that side.
    static void drawPageNumbers(QPainter *painter, const BookTheme &theme, int leftNum, int rightNum);

private:
    static QString getCornerSvg(const BookTheme &theme, const QString &placement);
};