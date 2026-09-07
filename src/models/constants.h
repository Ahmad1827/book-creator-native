#pragma once

#include "types.h"
#include <QStringList>

struct FontDescriptor {
    QString name;
    QString category;
};

inline const QList<FontDescriptor> FONTS = {
    {"Patrick Hand", "Handwritten"},
    {"Caveat", "Handwritten"},
    {"Kalam", "Handwritten"},
    {"Indie Flower", "Handwritten"},
    {"Architects Daughter", "Handwritten"},
    {"Fredoka", "Playful"},
    {"Sniglet", "Playful"},
    {"Baloo 2", "Playful"},
    {"Chewy", "Playful"},
    {"DynaPuff", "Playful"},
    {"Pangolin", "Playful"},
    {"Gaegu", "Playful"},
    {"Comfortaa", "Modern"},
    {"Quicksand", "Modern"},
    {"Lora", "Classic"},
    {"Amatic SC", "Whimsical"}
};

inline const QList<BookTheme> THEMES = {
    {
        "botanical_meadow",
        "Botanical Meadow",
        "Forest green binding, sage leaf borders & emerald vine corners",
        QColor("#f6f8f3"),
        QColor("#1d291e"),
        QColor("#223b28"),
        QColor("#cfe0d0"),
        QColor("#588157"),
        QColor("#4f772d"),
        DecorType::Botanical,
        "Patrick Hand"
    },
    {
        "celestial_nocturne",
        "Celestial Starlight",
        "Midnight indigo spine with golden crescent stars and cosmic corners",
        QColor("#1c2234"),
        QColor("#f3f5fa"),
        QColor("#0f1422"),
        QColor("#2e3752"),
        QColor("#f59e0b"),
        QColor("#fbbf24"),
        DecorType::Celestial,
        "Comfortaa"
    },
    {
        "antique_heirloom",
        "Antique Leather & Gold",
        "Burnished cognac leather, warm parchment with filigree corners",
        QColor("#faf2e3"),
        QColor("#2b1a11"),
        QColor("#3d1b0c"),
        QColor("#e3d1b6"),
        QColor("#b48448"),
        QColor("#c69255"),
        DecorType::Victorian,
        "Lora"
    },
    {
        "sakura_bloom",
        "Sakura Tea Garden",
        "Rose-tinted rice paper with cherry blossom twigs & coral corners",
        QColor("#fdf6f5"),
        QColor("#2e1a22"),
        QColor("#421d28"),
        QColor("#edd6db"),
        QColor("#e07a5f"),
        QColor("#d9777f"),
        DecorType::Sakura,
        "Caveat"
    },
    {
        "spiced_chai",
        "Spiced Chai & Tweed",
        "Warm terracotta paper, cozy autumn stitch lines & acorn motifs",
        QColor("#f7eee3"),
        QColor("#381c12"),
        QColor("#451e0e"),
        QColor("#dfc7b0"),
        QColor("#c68b59"),
        QColor("#b06d3b"),
        DecorType::Chai,
        "Kalam"
    },
    {
        "brass_study",
        "Vintage Brass Library",
        "Dark spruce boards with riveted brass corner brackets",
        QColor("#fcfbf7"),
        QColor("#232622"),
        QColor("#1d2924"),
        QColor("#dedad2"),
        QColor("#d4af37"),
        QColor("#b89730"),
        DecorType::Brass,
        "Fredoka"
    }
};

inline const QList<QColor> LOFI_PALETTE = {
    QColor("#2c211a"),
    QColor("#4d3a2e"),
    QColor("#8b6d5c"),
    QColor("#c68b59"),
    QColor("#dda15e"),
    QColor("#e07a5f"),
    QColor("#606c38"),
    QColor("#819b7a"),
    QColor("#3d5a80"),
    QColor("#847596"),
    QColor("#b48448"),
    QColor("#f3f5fa")
};

inline const BookTheme& getThemeById(const QString &id) {
    for (const auto &t : THEMES) {
        if (t.id == id) return t;
    }
    return THEMES[0];
}