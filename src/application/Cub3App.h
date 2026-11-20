// CUB3 Launcher - Application orchestration
#pragma once

#include <QObject>
#include <QPointer>

namespace cub3::ui::windows {
class MainWindow;
}

namespace cub3::storage {
class Database;
class Settings;
}

namespace cub3::network {
class HttpClient;
class DownloadEngine;
}

namespace cub3::versions {
class VersionManager;
}

namespace cub3::auth {
class AuthManager;
}

namespace cub3::gitlab {
class GitLabManager;
}

namespace cub3::application {

class LaunchEngine;

/// Central orchestration object. Owns the major subsystems and
/// wires them together. Each subsystem is created lazily — only
/// when first needed — to keep startup fast.
///
/// Lifetime: created on the stack in main(), lives for the duration
/// of the application.
class Cub3App : public QObject {
    Q_OBJECT

public:
    explicit Cub3App(QObject* parent = nullptr);
    ~Cub3App() override;

    /// Show the main window. If cached data exists, the dashboard
    /// is populated immediately and refreshed in the background.
    void showWindow();

    // Accessors for subsystems (lazy-initialized)
    [[nodiscard]] storage::Database&      database();
    [[nodiscard]] storage::Settings&      settings();
    [[nodiscard]] network::HttpClient&     httpClient();
    [[nodiscard]] network::DownloadEngine& downloadEngine();
    [[nodiscard]] versions::VersionManager& versionManager();
    [[nodiscard]] auth::AuthManager& authManager();
    [[nodiscard]] gitlab::GitLabManager& gitLabManager();
    [[nodiscard]] LaunchEngine& launchEngine();
    [[nodiscard]] ui::windows::MainWindow* window() const { return m_window; }

signals:
    /// Emitted when the dashboard is fully interactive.
    void dashboardReady();

    /// Emitted when an irrecoverable error occurs.
    void fatal(const QString& message);

private:
    void initWindow();
    bool initStorage();
    void initNetwork();
    void deferredInit();

    ui::windows::MainWindow* m_window{nullptr};

    // Subsystems — stored as QObjects, parented to this
    storage::Database*       m_database{nullptr};
    storage::Settings*       m_settings{nullptr};
    network::HttpClient*     m_http{nullptr};
    network::DownloadEngine* m_downloads{nullptr};
    versions::VersionManager* m_versions{nullptr};
    auth::AuthManager* m_auth{nullptr};
    gitlab::GitLabManager* m_gitLab{nullptr};
    LaunchEngine* m_launcher{nullptr};
    bool m_deferredInitialized{false};
};

} // namespace cub3::application
