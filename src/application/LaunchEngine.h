// CUB3 Launcher - Vanilla Minecraft installation and launch
#pragma once

#include <QObject>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QStringList>

#include "auth/Account.h"
#include "network/DownloadEngine.h"
#include "versions/VersionManager.h"

class QProcess;

namespace cub3::storage { class Settings; }
namespace cub3::storage { class Database; }
namespace cub3::network { class HttpClient; }

namespace cub3::application {

class LaunchEngine : public QObject {
    Q_OBJECT

public:
    LaunchEngine(versions::VersionManager* versions,
                 network::DownloadEngine* downloads,
                 network::HttpClient* http,
                 storage::Database* database,
                 storage::Settings* settings,
                 QObject* parent = nullptr);

    void launch(const QString& versionId, const auth::Account& account);
    [[nodiscard]] bool isRunning() const;

signals:
    void statusChanged(const QString& status);
    void launchFailed(const QString& message);
    void gameFinished(int exitCode);

private:
    void resolveVersionFiles();
    void beginInstall(const QList<versions::VersionManager::FileDescriptor>& files);
    void onDownloadFinished(const QString& id, network::DownloadStatus status,
                            const QString& error);
    void startGame();
    void saveInstallation(const QString& directory);
    void fail(const QString& message);
    QStringList manifestArguments(const QJsonArray& entries,
                                  const QHash<QString, QString>& values) const;

    versions::VersionManager* m_versions;
    network::DownloadEngine* m_downloads;
    network::HttpClient* m_http;
    storage::Database* m_database;
    storage::Settings* m_settings;
    QProcess* m_process{nullptr};
    QString m_versionId;
    auth::Account m_account;
    QJsonObject m_manifest;
    QList<versions::VersionManager::FileDescriptor> m_files;
    QSet<QString> m_pendingIds;
    QString m_authlibInjectorPath;
    QString m_authlibInjectorHash;
    QString m_authlibInjectorUrl;
    bool m_launching{false};
    bool m_failed{false};
};

} // namespace cub3::application
