#pragma once

#include <QString>
#include <QColor>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <optional>
#include "book_format.h"

struct CanvasLayer {
    QString id;
    QString name;
    bool visible = true;
    bool locked = false;
    double opacity = 1.0;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["visible"] = visible;
        obj["locked"] = locked;
        obj["opacity"] = opacity;
        return obj;
    }

    static CanvasLayer fromJson(const QJsonObject &obj) {
        CanvasLayer layer;
        layer.id = obj["id"].toString();
        layer.name = obj["name"].toString();
        layer.visible = obj["visible"].toBool(true);
        layer.locked = obj["locked"].toBool(false);
        layer.opacity = obj["opacity"].toDouble(1.0);
        return layer;
    }
};

struct BookSpread {
    QString id;
    QString kind;  // "cover" for the back/front cover spread, empty for story pages
    int leftPageNum = 1;
    int rightPageNum = 2;
    QJsonObject canvasData;
    QList<CanvasLayer> layers;
    QString activeLayerId;

    bool isCover() const { return kind == "cover"; }

    static BookSpread blank(const QString &spreadId) {
        BookSpread spread;
        spread.id = spreadId;
        spread.activeLayerId = "layer-1";
        CanvasLayer layer;
        layer.id = "layer-1";
        layer.name = "Artwork Layer";
        spread.layers.append(layer);
        return spread;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        if (!kind.isEmpty()) obj["kind"] = kind;
        obj["leftPageNum"] = leftPageNum;
        obj["rightPageNum"] = rightPageNum;
        obj["canvasData"] = canvasData;
        obj["activeLayerId"] = activeLayerId;

        QJsonArray layersArr;
        for (const auto &layer : layers) {
            layersArr.append(layer.toJson());
        }
        obj["layers"] = layersArr;

        return obj;
    }

    static BookSpread fromJson(const QJsonObject &obj) {
        BookSpread spread;
        spread.id = obj["id"].toString();
        spread.kind = obj["kind"].toString();
        spread.leftPageNum = obj["leftPageNum"].toInt(1);
        spread.rightPageNum = obj["rightPageNum"].toInt(2);
        spread.canvasData = obj["canvasData"].toObject();
        spread.activeLayerId = obj["activeLayerId"].toString();

        QJsonArray layersArr = obj["layers"].toArray();
        for (const auto &val : layersArr) {
            spread.layers.append(CanvasLayer::fromJson(val.toObject()));
        }

        if (spread.layers.isEmpty()) {
            CanvasLayer defaultLayer;
            defaultLayer.id = "layer-1";
            defaultLayer.name = "Artwork Layer";
            spread.layers.append(defaultLayer);
            spread.activeLayerId = "layer-1";
        }

        return spread;
    }
};

enum class DecorType {
    Scribble,
    Dreams,
    Sweets,
    Garden,
    Woods,
    Seaside
};

// Books made before the storybook themes keep the closest new look.
inline QString migrateThemeId(const QString &id) {
    if (id == "botanical_meadow") return "peachy_garden";
    if (id == "celestial_nocturne") return "dream_night";
    if (id == "antique_heirloom") return "scribble_splash";
    if (id == "sakura_bloom") return "sweet_shop";
    if (id == "spiced_chai") return "wobbly_woods";
    if (id == "brass_study") return "seaside";
    return id;
}

struct BookTheme {
    QString id;
    QString name;
    QString tagline;
    QColor paperBg;
    QColor inkColor;
    QColor spineColor;
    QColor borderColor;
    QColor accentColor;
    QColor frameColor;
    DecorType decorType;
    QString recommendedFont;

    static QString decorTypeToString(DecorType type) {
        switch (type) {
            case DecorType::Scribble: return "scribble";
            case DecorType::Dreams: return "dreams";
            case DecorType::Sweets: return "sweets";
            case DecorType::Garden: return "garden";
            case DecorType::Woods: return "woods";
            case DecorType::Seaside: return "seaside";
        }
        return "scribble";
    }

    static DecorType stringToDecorType(const QString &str) {
        if (str == "dreams") return DecorType::Dreams;
        if (str == "sweets") return DecorType::Sweets;
        if (str == "garden") return DecorType::Garden;
        if (str == "woods") return DecorType::Woods;
        if (str == "seaside") return DecorType::Seaside;
        return DecorType::Scribble;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["tagline"] = tagline;
        obj["paperBg"] = paperBg.name();
        obj["inkColor"] = inkColor.name();
        obj["spineColor"] = spineColor.name();
        obj["borderColor"] = borderColor.name();
        obj["accentColor"] = accentColor.name();
        obj["frameColor"] = frameColor.name();
        obj["decorType"] = decorTypeToString(decorType);
        obj["recommendedFont"] = recommendedFont;
        return obj;
    }
};

struct BookProject {
    QString id;
    QString title;
    QString author;
    QString themeId;
    QList<BookSpread> spreads;
    QString createdAt;
    bool showPageNumbers = true;
    QString formatId = "classic";

    const BookFormat &format() const { return getFormatById(formatId); }
    qreal pageHeight() const { return format().pageHeightUnits(); }

    bool hasCover() const { return !spreads.isEmpty() && spreads.first().isCover(); }

    int pageCount() const {
        int count = 0;
        for (const auto &s : spreads) {
            if (!s.isCover()) count += 2;
        }
        return count;
    }

    // Story pages are numbered from 1 after the cover, in spread order.
    void renumberPages() {
        int next = 1;
        for (auto &s : spreads) {
            if (s.isCover()) {
                s.leftPageNum = 0;
                s.rightPageNum = 0;
                continue;
            }
            s.leftPageNum = next;
            s.rightPageNum = next + 1;
            next += 2;
        }
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["title"] = title;
        obj["author"] = author;
        obj["themeId"] = themeId;
        obj["createdAt"] = createdAt;
        obj["showPageNumbers"] = showPageNumbers;
        obj["format"] = formatId;

        QJsonArray spreadsArr;
        for (const auto &spread : spreads) {
            spreadsArr.append(spread.toJson());
        }
        obj["spreads"] = spreadsArr;

        return obj;
    }

    static BookProject fromJson(const QJsonObject &obj) {
        BookProject project;
        project.id = obj["id"].toString();
        project.title = obj["title"].toString();
        project.author = obj["author"].toString();
        project.themeId = migrateThemeId(obj["themeId"].toString("scribble_splash"));
        project.createdAt = obj["createdAt"].toString();
        project.showPageNumbers = obj["showPageNumbers"].toBool(true);
        project.formatId = obj["format"].toString("classic");

        QJsonArray spreadsArr = obj["spreads"].toArray();
        for (const auto &val : spreadsArr) {
            project.spreads.append(BookSpread::fromJson(val.toObject()));
        }

        return project;
    }
};