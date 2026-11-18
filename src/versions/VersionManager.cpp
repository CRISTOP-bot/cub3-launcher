// CUB3 Launcher - Version manager implementation
#include "VersionManager.h"
#include "network/HttpClient.h"
#include "storage/Database.h"
#include "core/Logger.h"
#include "core/Paths.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>
#include <QSysInfo>
#include <utility>

namespace cub3::versions {

// Mojang's version manifest URL
static const QString kVersionManifestUrl =
    QStringLiteral("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json");

namespace {
bool libraryAllowed(const QJsonArray& rules) {
    if (rules.isEmpty())
        return true;

#ifdef Q_OS_WIN
    const QString osName = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    const QString osName = QStringLiteral("osx");
#else
    const QString osName = QStringLiteral("linux");
#endif

    const QString architecture = QSysInfo::currentCpuArchitecture();
    const QString osVersion = QSysInfo::productVersion();
    bool allowed = false;
    for (const auto& value : rules) {
        const auto rule = value.toObject();
        const auto os = rule.value("os").toObject();
        if (!os.isEmpty()) {
            if (os.contains("name") && os.value("name").toString() != osName)
                continue;
            if (os.contains("arch")
                && !QRegularExpression(os.value("arch").toString())
                        .match(architecture).hasMatch())
                continue;
            if (os.contains("version")
                && !QRegularExpression(os.value("version").toString())
                        .match(osVersion).hasMatch())
                continue;
        }

        // Launcher-specific feature flags are not available during metadata resolution.
        if (!rule.value("features").toObject().isEmpty())
            continue;

        allowed = rule.value("action").toString() == QStringLiteral("allow");
    }
    return allowed;
}
} // namespace

VersionManager::VersionManager(network::HttpClient* http,
                                 storage::Database* db,
                                 QObject* parent)
    : QObject(parent)
    , m_http(http)
    , m_db(db)
{
    // Load cached version list from the database
    auto rows = m_db->queryRows(
        QStringLiteral("SELECT url, content, etag, last_modified FROM manifest_cache "
                       "WHERE url = ?"),
        {kVersionManifestUrl});

    if (!rows.isEmpty()) {
        auto& row = rows.first();
        QByteArray content = row["content"].toByteArray();
        if (!content.isEmpty()) {
            if (parseVersionList(content)) {
                m_versionsLoaded = true;
                CUB3_INFO(lcCub3) << "Loaded cached version list:"
                                  << m_versions.size() << "versions";
            }
        }
    }
}

void VersionManager::fetchVersionList(
    std::function<void(const QList<VersionInfo>&)> onComplete)
{
    // If we have a cached list, return it immediately and refresh in background
    if (m_versionsLoaded) {
        onComplete(m_versions);
    }

    // Check for cached ETag
    auto etag = m_db->queryValue(
        QStringLiteral("SELECT etag FROM manifest_cache WHERE url = ?"),
        {kVersionManifestUrl}).toString();

    auto lastModified = m_db->queryValue(
        QStringLiteral("SELECT last_modified FROM manifest_cache WHERE url = ?"),
        {kVersionManifestUrl}).toString();

    // Conditional fetch
    m_http->getConditional(kVersionManifestUrl, etag, lastModified,
        [this, onComplete](const QByteArray& data, int status,
                           const QHash<QString, QString>& headers) {
        if (status == 304) {
            // Not modified — cache is still valid
            CUB3_INFO(lcCub3) << "Version manifest not modified (304)";
            onComplete(m_versions);
            return;
        }

        if (status == 200 && !data.isEmpty()) {
            if (!parseVersionList(data)) {
                if (!m_versionsLoaded)
                    onComplete({});
                return;
            }

            // Cache the manifest
            m_db->exec(
                QStringLiteral("INSERT OR REPLACE INTO manifest_cache "
                               "(url, content, etag, last_modified) VALUES (?, ?, ?, ?)"),
                {kVersionManifestUrl, QString::fromUtf8(data),
                 headers.value("ETag"), headers.value("Last-Modified")});

            m_versionsLoaded = true;
            CUB3_INFO(lcCub3) << "Fetched version list:"
                              << m_versions.size() << "versions";
            onComplete(m_versions);
        } else {
            CUB3_WARN(lcCub3) << "Failed to fetch version manifest, status:" << status;
            if (m_versionsLoaded) onComplete(m_versions);
            else onComplete({});
        }
    });
}

bool VersionManager::parseVersionList(const QByteArray& data) {
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        CUB3_ERROR(lcCub3) << "Failed to parse version manifest JSON:" << err.errorString();
        return false;
    }

    auto root = doc.object();
    auto versionsArray = root.value("versions").toArray();
    if (versionsArray.isEmpty()) {
        CUB3_ERROR(lcCub3) << "Version manifest contains no versions";
        return false;
    }

    QList<VersionInfo> versions;
    QHash<QString, VersionInfo> versionsById;
    versions.reserve(versionsArray.size());

    for (const auto& v : versionsArray) {
        auto obj = v.toObject();
        VersionInfo info;
        info.id = obj.value("id").toString();
        info.type = obj.value("type").toString();
        info.url = obj.value("url").toString();
        info.releaseTime = obj.value("releaseTime").toString();
        if (info.id.isEmpty() || info.url.isEmpty()) {
            CUB3_ERROR(lcCub3) << "Version manifest contains an invalid entry";
            return false;
        }
        versions.append(info);
        versionsById[info.id] = info;
    }
    m_versions = std::move(versions);
    m_versionsById = std::move(versionsById);
    return true;
}

void VersionManager::fetchVersionManifest(const QString& versionId,
    std::function<void(const QJsonDocument&)> onComplete)
{
    auto it = m_versionsById.constFind(versionId);
    if (it == m_versionsById.constEnd()) {
        CUB3_WARN(lcCub3) << "Unknown version:" << versionId;
        onComplete(QJsonDocument());
        return;
    }

    const auto& info = it.value();

    // Check cache first
    auto cached = m_db->queryValue(
        QStringLiteral("SELECT content FROM manifest_cache WHERE url = ?"),
        {info.url});

    if (cached.isValid()) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(cached.toByteArray(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            onComplete(doc);
            // Refresh the cache without invoking the one-shot callback twice.
            m_http->get(QUrl(info.url),
                [this, info](const QByteArray& data, int status) {
                if (status != 200 || data.isEmpty())
                    return;
                QJsonParseError refreshError;
                const auto refreshed = QJsonDocument::fromJson(data, &refreshError);
                if (refreshError.error == QJsonParseError::NoError
                    && refreshed.isObject()) {
                    m_db->exec(
                        QStringLiteral("INSERT OR REPLACE INTO manifest_cache "
                                       "(url, content) VALUES (?, ?)"),
                        {info.url, QString::fromUtf8(data)});
                }
            });
            return;
        }
    }

    // Fetch (or refresh) the version manifest
    m_http->get(QUrl(info.url),
        [this, versionId, info, onComplete](const QByteArray& data, int status) {
        if (status == 200 && !data.isEmpty()) {
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(data, &err);
            if (err.error == QJsonParseError::NoError) {
                m_db->exec(
                    QStringLiteral("INSERT OR REPLACE INTO manifest_cache "
                                   "(url, content) VALUES (?, ?)"),
                    {info.url, QString::fromUtf8(data)});
                onComplete(doc);
                return;
            }
        }
        onComplete(QJsonDocument());
    });
}

void VersionManager::resolveVersion(const QString& versionId,
    std::function<void(const QList<FileDescriptor>&)> onComplete)
{
    fetchVersionManifest(versionId, [this, onComplete](const QJsonDocument& doc) {
        QList<FileDescriptor> files;

        if (doc.isEmpty()) {
            onComplete(files);
            return;
        }

        auto root = doc.object();

        // Client JAR
        auto downloads = root.value("downloads").toObject();
        auto client = downloads.value("client").toObject();
        if (!client.isEmpty()) {
            FileDescriptor fd;
            fd.url = client.value("url").toString();
            fd.path = "versions/" + root.value("id").toString() + "/" +
                      root.value("id").toString() + ".jar";
            fd.hash = client.value("sha1").toString();
            fd.size = client.value("size").toVariant().toLongLong();
            files.append(fd);
        }

        // Libraries
        const QString architecture = QSysInfo::currentCpuArchitecture().contains(
            QStringLiteral("64")) ? QStringLiteral("64") : QStringLiteral("32");
        auto libraries = root.value("libraries").toArray();
        for (const auto& lib : libraries) {
            auto libObj = lib.toObject();

            auto rules = libObj.value("rules").toArray();
            if (!libraryAllowed(rules))
                continue;

            auto downloads = libObj.value("downloads").toObject();
            auto artifact = downloads.value("artifact").toObject();
            if (!artifact.isEmpty()) {
                FileDescriptor fd;
                fd.url = artifact.value("url").toString();
                fd.path = QStringLiteral("libraries/") + artifact.value("path").toString();
                fd.hash = artifact.value("sha1").toString();
                fd.size = artifact.value("size").toVariant().toLongLong();
                if (!fd.url.isEmpty() && !fd.path.endsWith('/'))
                    files.append(fd);
            }

#ifdef Q_OS_LINUX
            const auto nativeMap = libObj.value("natives").toObject();
            QString classifier = nativeMap.value(QStringLiteral("linux")).toString();
            classifier.replace(QStringLiteral("${arch}"), architecture);
            const auto nativeArtifact =
                downloads.value("classifiers").toObject().value(classifier).toObject();
            if (!nativeArtifact.isEmpty()) {
                FileDescriptor fd;
                fd.url = nativeArtifact.value("url").toString();
                fd.path = QStringLiteral("libraries/")
                    + nativeArtifact.value("path").toString();
                fd.hash = nativeArtifact.value("sha1").toString();
                fd.size = nativeArtifact.value("size").toVariant().toLongLong();
                fd.native = true;
                if (!fd.url.isEmpty() && !fd.path.endsWith('/'))
                    files.append(fd);
            }
#endif
        }

        // Asset index
        auto assetIndex = root.value("assetIndex").toObject();
        if (!assetIndex.isEmpty()) {
            FileDescriptor fd;
            fd.url = assetIndex.value("url").toString();
            fd.path = "assets/indexes/" + root.value("assets").toString() + ".json";
            fd.hash = assetIndex.value("sha1").toString();
            fd.size = assetIndex.value("size").toVariant().toLongLong();
            files.append(fd);

            const QJsonObject logFile = root.value(QStringLiteral("logging"))
                .toObject().value(QStringLiteral("client")).toObject()
                .value(QStringLiteral("file")).toObject();
            if (!logFile.value(QStringLiteral("url")).toString().isEmpty()
                && !logFile.value(QStringLiteral("id")).toString().isEmpty()) {
                FileDescriptor logging;
                logging.url = logFile.value(QStringLiteral("url")).toString();
                logging.path = QStringLiteral("assets/log_configs/")
                    + logFile.value(QStringLiteral("id")).toString();
                logging.hash = logFile.value(QStringLiteral("sha1")).toString();
                logging.size = logFile.value(QStringLiteral("size")).toVariant().toLongLong();
                files.append(std::move(logging));
            }

            const QUrl indexUrl(assetIndex.value("url").toString());
            if (!indexUrl.isValid()) {
                onComplete({});
                return;
            }
            m_http->get(indexUrl,
                [files = std::move(files), onComplete]
                (const QByteArray& data, int status) mutable {
                if (status != 200 || data.isEmpty()) {
                    onComplete({});
                    return;
                }

                QJsonParseError error;
                const QJsonDocument indexDoc = QJsonDocument::fromJson(data, &error);
                const QJsonObject objects = indexDoc.object()
                    .value(QStringLiteral("objects")).toObject();
                if (error.error != QJsonParseError::NoError || objects.isEmpty()) {
                    onComplete({});
                    return;
                }

                for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
                    const QString hash = it.value().toObject()
                        .value(QStringLiteral("hash")).toString();
                    if (hash.size() != 40)
                        continue;
                    FileDescriptor object;
                    object.url = QStringLiteral("https://resources.download.minecraft.net/%1/%2")
                        .arg(hash.left(2), hash);
                    object.path = QStringLiteral("assets/objects/%1/%2")
                        .arg(hash.left(2), hash);
                    object.hash = hash;
                    object.size = it.value().toObject()
                        .value(QStringLiteral("size")).toVariant().toLongLong();
                    files.append(std::move(object));
                }
                onComplete(files);
            });
            return;
        }

        onComplete(files);
    });
}

} // namespace cub3::versions
