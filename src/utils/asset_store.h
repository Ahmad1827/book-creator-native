#pragma once

#include <QString>
#include <QPixmap>
#include <QImage>
#include <QByteArray>

// Pictures live as files next to projects.json instead of inside it, so a book full of
// scanned paintings stays quick to autosave and undo. Ids are content hashes, so the
// same picture is stored once no matter how many books or pages use it.
class AssetStore {
public:
    static void setDirectory(const QString &dir);
    static QString directory();

    // Downscales to print resolution, fixes phone-photo rotation and stores the result.
    static QString addImageFile(const QString &path, QString *error = nullptr);
    static QString addImage(const QImage &image);
    // Stores already-encoded bytes (used when importing a book file); returns the id.
    static QString addEncoded(const QByteArray &bytes, const QString &extension);

    static QPixmap pixmap(const QString &id);
    static QByteArray bytes(const QString &id);
    static bool isImageFile(const QString &path);

    static constexpr int kMaxLongEdge = 3000;
};
