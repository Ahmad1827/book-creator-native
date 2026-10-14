#include "asset_store.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImageReader>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

QString &storeDirectory() {
    static QString dir;
    return dir;
}

QHash<QString, QPixmap> &pixmapCache() {
    static QHash<QString, QPixmap> cache;
    return cache;
}

bool hasTransparency(const QImage &image) {
    if (!image.hasAlphaChannel()) return false;
    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < argb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x) {
            if (qAlpha(line[x]) < 255) return true;
        }
    }
    return false;
}

} // namespace

void AssetStore::setDirectory(const QString &dir) {
    storeDirectory() = dir;
    pixmapCache().clear();
}

QString AssetStore::directory() {
    QString &dir = storeDirectory();
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/assets";
    }
    QDir().mkpath(dir);
    return dir;
}

bool AssetStore::isImageFile(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return QImageReader::supportedImageFormats().contains(suffix.toLatin1());
}

QString AssetStore::addImageFile(const QString &path, QString *error) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        if (error) *error = reader.errorString();
        return {};
    }
    return addImage(image);
}

QString AssetStore::addImage(const QImage &source) {
    if (source.isNull()) return {};

    QImage image = source;
    if (qMax(image.width(), image.height()) > kMaxLongEdge) {
        image = image.scaled(kMaxLongEdge, kMaxLongEdge, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // Paintings and photos compress far better as JPEG; cut-outs need PNG transparency.
    const bool transparent = hasTransparency(image);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (transparent) {
        image.save(&buffer, "PNG");
    } else {
        image.convertToFormat(QImage::Format_RGB32).save(&buffer, "JPG", 92);
    }
    return addEncoded(bytes, transparent ? "png" : "jpg");
}

QString AssetStore::addEncoded(const QByteArray &bytes, const QString &extension) {
    if (bytes.isEmpty()) return {};
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex().left(24));
    const QString id = QString("%1.%2").arg(hash, extension.toLower());
    const QString path = directory() + "/" + id;

    if (!QFile::exists(path)) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {};
        file.write(bytes);
        if (!file.commit()) return {};
    }
    return id;
}

QPixmap AssetStore::pixmap(const QString &id) {
    if (id.isEmpty() || id.contains('/') || id.contains('\\')) return {};
    auto &cache = pixmapCache();
    auto it = cache.constFind(id);
    if (it != cache.constEnd()) return it.value();

    QPixmap pix(directory() + "/" + id);
    if (!pix.isNull()) cache.insert(id, pix);
    return pix;
}

QByteArray AssetStore::bytes(const QString &id) {
    if (id.isEmpty() || id.contains('/') || id.contains('\\')) return {};
    QFile file(directory() + "/" + id);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}
