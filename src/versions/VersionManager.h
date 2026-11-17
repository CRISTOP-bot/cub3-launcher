// CUB3 Launcher - Version manager
#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QJsonDocument>
#include <QByteArray>
#include <functional>

namespace cub3::network { class HttpClient; }
namespace cub3::storage  { class Database; }

namespace cub3::versions {

/// A Minecraft version descriptor.
struct VersionInfo {
    QString id;            // e.g. "1.20.4"
    QString type;          // "release" | "snapshot" | "old_beta"
    QString url;           // URL to the version JSON manifest
    QString releaseTime;
    bool installed{false};
};

/// A mod loader descriptor.
struct LoaderInfo {
    QString name;          // "fabric" | "forge" | "neoforge" | "quilt"
    QString version;
    QString gameVersion;
};

/// Manages Minecraft version metadata and installation.
///
/// Design:
///   - Version list is cached in memory and SQLite.
///   - Manifests are fetched with ETag/Last-Modified conditional requests.
///   - No full re-parse of manifests that haven't changed.
///   - Installation is version-independent (each installation has its own dir).
class VersionManager : public QObject {
    Q_OBJECT

public:
    explicit VersionManager(network::HttpClient* http,
                              storage::Database* db,
                              QObject* parent = nullptr);

    /// Fetch the version list (async). Uses cache if available.
    /// onComplete: called with the list of versions.
    void fetchVersionList(std::function<void(const QList<VersionInfo>&)> onComplete);

    /// Get the cached version list (empty if not yet fetched).
    [[nodiscard]] const QList<VersionInfo>& cachedVersions() const { return m_versions; }

    /// Fetch a version's manifest JSON.
    /// onComplete: called with the parsed JSON document.
    void fetchVersionManifest(const QString& versionId,
                               std::function<void(const QJsonDocument&)> onComplete);

    /// Resolve the libraries and assets needed for a version.
    /// Returns a list of file download descriptors.
    struct FileDescriptor {
        QString url;
        QString path;     // relative to the data directory
        QString hash;      // SHA-1
        qint64 size{-1};
    };

    void resolveVersion(const QString& versionId,
                         std::function<void(const QList<FileDescriptor>&)> onComplete);

private:
    bool parseVersionList(const QByteArray& data);

    network::HttpClient* m_http;
    storage::Database* m_db;

    QList<VersionInfo> m_versions;
    QHash<QString, VersionInfo> m_versionsById;
    bool m_versionsLoaded{false};
};

} // namespace cub3::versions
