// CUB3 Launcher - Settings implementation
#include "Settings.h"
#include "Database.h"
#include "core/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace cub3::storage {

Settings::Settings(Database& db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{
    load();
}

void Settings::load() {
    auto rows = m_db.queryRows(
        QStringLiteral("SELECT key, value FROM settings"));
    for (auto& row : rows)
        m_cache[row["key"].toString()] = row["value"];
}

QVariant Settings::get(const QString& key, const QVariant& defaultValue) const {
    auto it = m_cache.constFind(key);
    if (it != m_cache.end())
        return it.value();
    return defaultValue;
}

void Settings::set(const QString& key, const QVariant& value) {
    m_cache[key] = value;
    m_dirty = true;

    // Persist immediately — settings writes are infrequent and low-volume
    m_db.exec(
        QStringLiteral("INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)"),
        {key, value.toString()});

    emit settingChanged(key);
}

void Settings::remove(const QString& key) {
    m_cache.remove(key);
    m_db.exec(QStringLiteral("DELETE FROM settings WHERE key = ?"), {key});
    emit settingChanged(key);
}

// --- Typed accessors -----------------------------------------------------

QString Settings::javaPath() const {
    return get(QStringLiteral("java_path")).toString();
}

void Settings::setJavaPath(const QString& path) {
    set(QStringLiteral("java_path"), path);
}

int Settings::ramMinMB() const {
    return get(QStringLiteral("ram_min_mb"), 512).toInt();
}

void Settings::setRamMinMB(int mb) {
    set(QStringLiteral("ram_min_mb"), mb);
}

int Settings::ramMaxMB() const {
    return get(QStringLiteral("ram_max_mb"), 2048).toInt();
}

void Settings::setRamMaxMB(int mb) {
    set(QStringLiteral("ram_max_mb"), mb);
}

QString Settings::instancesDir() const {
    return get(QStringLiteral("instances_dir")).toString();
}

void Settings::setInstancesDir(const QString& dir) {
    set(QStringLiteral("instances_dir"), dir);
}

int Settings::maxConcurrentDownloads() const {
    return get(QStringLiteral("max_concurrent_dl"), 4).toInt();
}

void Settings::setMaxConcurrentDownloads(int n) {
    set(QStringLiteral("max_concurrent_dl"), n);
}

bool Settings::validateDownloads() const {
    return get(QStringLiteral("validate_downloads"), true).toBool();
}

void Settings::setValidateDownloads(bool b) {
    set(QStringLiteral("validate_downloads"), b);
}

bool Settings::reducedEffects() const {
    return get(QStringLiteral("reduced_effects"), false).toBool();
}

void Settings::setReducedEffects(bool b) {
    set(QStringLiteral("reduced_effects"), b);
}

int Settings::resolutionWidth() const {
    return get(QStringLiteral("resolution_w"), 854).toInt();
}

void Settings::setResolutionWidth(int w) {
    set(QStringLiteral("resolution_w"), w);
}

int Settings::resolutionHeight() const {
    return get(QStringLiteral("resolution_h"), 480).toInt();
}

void Settings::setResolutionHeight(int h) {
    set(QStringLiteral("resolution_h"), h);
}

int Settings::activeInstallationId() const {
    return get(QStringLiteral("active_installation_id"), 0).toInt();
}

void Settings::setActiveInstallationId(int id) {
    set(QStringLiteral("active_installation_id"), id);
}

} // namespace cub3::storage
