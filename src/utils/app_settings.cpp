#include "app_settings.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <memory>

namespace {

QString &settingsFilePath() {
    static QString path;
    return path;
}

std::unique_ptr<QSettings> &settingsInstance() {
    static std::unique_ptr<QSettings> settings;
    return settings;
}

QSettings &store() {
    auto &settings = settingsInstance();
    if (!settings) {
        QString path = settingsFilePath();
        if (path.isEmpty()) {
            // An explicit file keeps settings beside projects.json without needing an organisation name.
            const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir().mkpath(dir);
            path = dir + "/settings.ini";
        }
        settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    }
    return *settings;
}

} // namespace

void AppSettings::setFilePath(const QString &path) {
    settingsFilePath() = path;
    settingsInstance().reset();
}

AppSettings::HelperBackend AppSettings::helperBackend() {
    return store().value("helper/backend", "claude_code").toString() == "off" ? HelperBackend::Off : HelperBackend::ClaudeCode;
}

void AppSettings::setHelperBackend(HelperBackend backend) {
    store().setValue("helper/backend", backend == HelperBackend::Off ? "off" : "claude_code");
    store().sync();
}

QString AppSettings::claudePath() {
    return store().value("helper/claudePath").toString();
}

void AppSettings::setClaudePath(const QString &path) {
    store().setValue("helper/claudePath", path.trimmed());
    store().sync();
}

QString AppSettings::claudeModel() {
    return store().value("helper/model").toString();
}

void AppSettings::setClaudeModel(const QString &model) {
    store().setValue("helper/model", model);
    store().sync();
}

QString AppSettings::readerAge() {
    return store().value("helper/readerAge", "5-8").toString();
}

void AppSettings::setReaderAge(const QString &age) {
    store().setValue("helper/readerAge", age);
    store().sync();
}
