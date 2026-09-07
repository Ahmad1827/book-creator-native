#pragma once

#include <QString>
#include <QColor>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <optional>

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
    int leftPageNum = 1;
    int rightPageNum = 2;
    QJsonObject canvasData;
    QList<CanvasLayer> layers;
    QString activeLayerId;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
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
    Botanical,
    Celestial,
    Victorian,
    Sakura,
    Chai,
    Brass
};

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
            case DecorType::Botanical: return "botanical";
            case DecorType::Celestial: return "celestial";
            case DecorType::Victorian: return "victorian";
            case DecorType::Sakura: return "sakura";
            case DecorType::Chai: return "chai";
            case DecorType::Brass: return "brass";
        }
        return "botanical";
    }

    static DecorType stringToDecorType(const QString &str) {
        if (str == "celestial") return DecorType::Celestial;
        if (str == "victorian") return DecorType::Victorian;
        if (str == "sakura") return DecorType::Sakura;
        if (str == "chai") return DecorType::Chai;
        if (str == "brass") return DecorType::Brass;
        return DecorType::Botanical;
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

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["title"] = title;
        obj["author"] = author;
        obj["themeId"] = themeId;
        obj["createdAt"] = createdAt;

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
        project.themeId = obj["themeId"].toString("botanical_meadow");
        project.createdAt = obj["createdAt"].toString();

        QJsonArray spreadsArr = obj["spreads"].toArray();
        for (const auto &val : spreadsArr) {
            project.spreads.append(BookSpread::fromJson(val.toObject()));
        }

        return project;
    }
};