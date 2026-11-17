// CUB3 Launcher - Settings store
#pragma once

#include <QObject>
#include <QHash>
#include <QVariant>
#include <QString>

namespace cub3::storage {

class Database;

/// Key-value settings store backed by SQLite.
/// Values are cached in memory after first read to avoid repeated
/// database queries from the UI thread.
///
/// Type-safe accessors for common settings are provided.
/// Custom keys can be used via get()/set().
class Settings : public QObject {
    Q_OBJECT

public:
    explicit Settings(Database& db, QObject* parent = nullptr);

    /// Load all settings into the in-memory cache.
    void load();

    // --- Generic accessors ---
    [[nodiscard]] QVariant get(const QString& key,
                                 const QVariant& defaultValue = {}) const;
    void set(const QString& key, const QVariant& value);
    void remove(const QString& key);

    // --- Typed convenience accessors ---

    // Java
    [[nodiscard]] QString javaPath() const;
    void setJavaPath(const QString& path);

    // Memory
    [[nodiscard]] int ramMinMB() const;
    void setRamMinMB(int mb);

    [[nodiscard]] int ramMaxMB() const;
    void setRamMaxMB(int mb);

    // Directories
    [[nodiscard]] QString instancesDir() const;
    void setInstancesDir(const QString& dir);

    // Network
    [[nodiscard]] int maxConcurrentDownloads() const;
    void setMaxConcurrentDownloads(int n);

    [[nodiscard]] bool validateDownloads() const;
    void setValidateDownloads(bool b);

    // Appearance
    [[nodiscard]] bool reducedEffects() const;
    void setReducedEffects(bool b);

    // Resolution
    [[nodiscard]] int resolutionWidth() const;
    void setResolutionWidth(int w);

    [[nodiscard]] int resolutionHeight() const;
    void setResolutionHeight(int h);

    // Active installation
    [[nodiscard]] int activeInstallationId() const;
    void setActiveInstallationId(int id);

signals:
    /// Emitted when any setting changes.
    void settingChanged(const QString& key);

private:
    void flush();

    Database& m_db;
    QHash<QString, QVariant> m_cache;
    bool m_dirty{false};
};

} // namespace cub3::storage
