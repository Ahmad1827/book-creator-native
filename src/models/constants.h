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
    {"Amatic SC", "Whimsical"},
    {"Henny Penny", "Whimsical"},
    {"Gochi Hand", "Handwritten"}
};

// Page colours: paperBg, inkColor (text), spineColor (book casing), borderColor (ink lines),
// accentColor and frameColor (the two watercolour washes used for the doodles).
inline const QList<BookTheme> THEMES = {
    {
        "scribble_splash",
        "Scribble & Splash",
        "Cream paper, scratchy ink and splashes of tomato and sunshine",
        QColor("#fffaf0"),
        QColor("#1f1a17"),
        QColor("#e8553e"),
        QColor("#2a221d"),
        QColor("#ffd25a"),
        QColor("#f26b4f"),
        DecorType::Scribble,
        "Patrick Hand"
    },
    {
        "dream_night",
        "Dream-Catcher Night",
        "Midnight pages, golden stars and floating dream bubbles",
        QColor("#22305a"),
        QColor("#fff4dc"),
        QColor("#141c38"),
        QColor("#f3e6c4"),
        QColor("#ffd25a"),
        QColor("#bf9cef"),
        DecorType::Dreams,
        "Pangolin"
    },
    {
        "sweet_shop",
        "Sweetshop Stripes",
        "Strawberry-cream paper, candy stripes and wrapped sweets",
        QColor("#fff2f0"),
        QColor("#3a1d2b"),
        QColor("#6b3a2a"),
        QColor("#3a1d2b"),
        QColor("#ff8fab"),
        QColor("#7fd8be"),
        DecorType::Sweets,
        "Sniglet"
    },
    {
        "peachy_garden",
        "Enormous Garden",
        "Sunny peach paper, grass tufts, leaves and ladybirds",
        QColor("#fff5e8"),
        QColor("#2d2416"),
        QColor("#5f8f3a"),
        QColor("#2d2416"),
        QColor("#ffab5c"),
        QColor("#9bd06a"),
        DecorType::Garden,
        "Kalam"
    },
    {
        "wobbly_woods",
        "Wobbly Woods",
        "Misty green pages, twisty twigs, toadstools and swirls",
        QColor("#f1f2e4"),
        QColor("#24201a"),
        QColor("#5b3a7a"),
        QColor("#24201a"),
        QColor("#bf9cef"),
        QColor("#ff9b54"),
        DecorType::Woods,
        "Gaegu"
    },
    {
        "seaside",
        "Seaside Splash",
        "Pale sky paper, rolling waves, a cheery sun and seagulls",
        QColor("#f0f8fb"),
        QColor("#1a2a35"),
        QColor("#2e7f8f"),
        QColor("#1a2a35"),
        QColor("#7cc0ee"),
        QColor("#ffd25a"),
        DecorType::Seaside,
        "Indie Flower"
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
    const QString current = migrateThemeId(id);
    for (const auto &t : THEMES) {
        if (t.id == current) return t;
    }
    return THEMES[0];
}