#pragma once

#include <QList>
#include <QString>

// Trim sizes offered for printing. Pages are always 600 canvas units wide; the height
// follows the format's proportions, and one unit prints at trimWidth / 600 inches.
struct BookFormat {
    QString id;
    QString name;
    qreal trimWidthIn;
    qreal trimHeightIn;

    static constexpr qreal kPageUnitsWide = 600.0;
    // Printers trim 1/8 inch off the outer edges, so full-bleed art must extend that far.
    static constexpr qreal kBleedIn = 0.125;

    qreal pageHeightUnits() const { return kPageUnitsWide * trimHeightIn / trimWidthIn; }
    qreal pointsPerUnit() const { return trimWidthIn * 72.0 / kPageUnitsWide; }
    qreal unitsPerInch() const { return kPageUnitsWide / trimWidthIn; }
    qreal bleedUnits() const { return kBleedIn * unitsPerInch(); }
};

inline const QList<BookFormat> BOOK_FORMATS = {
    {"classic", "Classic (8.3 × 9 in)", 600.0 / 72.0, 650.0 / 72.0},
    {"square85", "Square (8.5 × 8.5 in)", 8.5, 8.5},
    {"portrait8x10", "Portrait (8 × 10 in)", 8.0, 10.0},
    {"landscape10x8", "Landscape (10 × 8 in)", 10.0, 8.0},
};

inline const BookFormat &getFormatById(const QString &id) {
    for (const auto &f : BOOK_FORMATS) {
        if (f.id == id) return f;
    }
    return BOOK_FORMATS[0];
}
