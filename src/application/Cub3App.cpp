// CUB3 Launcher - Application orchestration
#include "Cub3App.h"
#include "LaunchEngine.h"

#include "core/Logger.h"
#include "core/Performance.h"
#include "core/Paths.h"

#include "ui/windows/MainWindow.h"
#include "storage/Database.h"
#include "storage/Settings.h"
#include "network/HttpClient.h"
#include "network/DownloadEngine.h"
#include "versions/VersionManager.h"
#include "auth/AuthManager.h"

#include <QTimer>
#include <QStandardPaths>
#include <stdexcept>

namespace cub3::application {

Cub3App::Cub3App(QObject* parent)
    : QObject(parent)
{
    CUB3_INFO(lcCub3) << "CUB3 application constructing";
}

Cub3App::~Cub3App() {
    CUB3_INFO(lcCub3) << "CUB3 application shutting down";
}

void Cub3App::showWindow() {
    // Phase 1: Show the window immediately with cached data
    initWindow();
    if (m_window) {
        m_window->show();
        CUB3_PERF_SCOPE("window_shown");
    }

    // Phase 2: Initialise storage (fast, local)
    if (!initStorage())
        return;

    // Phase 3: Schedule network-dependent init on the next event loop tick.
    // This allows the window to paint before we do any blocking work.
    QTimer::singleShot(0, this, [this] {
        deferredInit();
    });
}

void Cub3App::initWindow() {
    if (m_window) return;

    CUB3_PERF_SCOPE("init_window");
    m_window = new ui::windows::MainWindow();
    m_window->setApp(this);

    // Wire dashboard ready signal
    connect(m_window, &ui::windows::MainWindow::dashboardShown, this, [this] {
        core::Perf::markDashboardReady();
        emit dashboardReady();
        CUB3_INFO(lcUI) << "Dashboard ready in"
                        << core::Perf::elapsedMs() << "ms";
    });
}

bool Cub3App::initStorage() {
    if (m_database)
        return m_database->isOpen();

    CUB3_PERF_SCOPE("init_storage");
    // Database and settings are created here because they're fast and local.
    // They use SQLite which is a local file — no network needed.
    m_database = new storage::Database(core::Paths::databaseFile(), this);
    if (!m_database->open()) {
        const QString message = QStringLiteral("Could not open the launcher database.");
        CUB3_ERROR(lcStorage) << message;
        emit fatal(message);
        return false;
    }
    m_settings = new storage::Settings(*m_database, this);
    if (m_settings->javaPath().trimmed().isEmpty()) {
        const QString java = QStandardPaths::findExecutable(QStringLiteral("java"));
        if (!java.isEmpty())
            m_settings->setJavaPath(java);
    }
    return true;
}

void Cub3App::initNetwork() {
    if (m_http)
        return;

    CUB3_PERF_SCOPE("init_network");
    m_http = new network::HttpClient(this);
    m_downloads = new network::DownloadEngine(m_http, this);
    if (m_settings) {
        m_downloads->setMaxConcurrent(m_settings->maxConcurrentDownloads());
        m_downloads->setValidateDownloads(m_settings->validateDownloads());
    }
}

void Cub3App::deferredInit() {
    if (m_deferredInitialized)
        return;

    if (!initStorage())
        return;
    initNetwork();
    m_versions = new versions::VersionManager(m_http, m_database, this);
    m_deferredInitialized = true;

    CUB3_INFO(lcCub3) << "Deferred init complete. RSS:"
                      << core::Perf::rssMB() << "MB,"
                      << "Threads:" << core::Perf::threadCount();
}

// --- Lazy accessors -------------------------------------------------------

storage::Database& Cub3App::database() {
    if (!initStorage())
        throw std::runtime_error("Launcher database is unavailable");
    return *m_database;
}

storage::Settings& Cub3App::settings() {
    if (!initStorage() || !m_settings)
        throw std::runtime_error("Launcher settings are unavailable");
    return *m_settings;
}

network::HttpClient& Cub3App::httpClient() {
    if (!m_http)
        initNetwork();
    return *m_http;
}

network::DownloadEngine& Cub3App::downloadEngine() {
    if (!m_downloads)
        initNetwork();
    return *m_downloads;
}

versions::VersionManager& Cub3App::versionManager() {
    if (!m_versions)
        deferredInit();
    if (!m_versions)
        throw std::runtime_error("Version manager is unavailable");
    return *m_versions;
}

auth::AuthManager& Cub3App::authManager() {
    if (!m_http)
        initNetwork();
    if (!m_auth)
        m_auth = new auth::AuthManager(m_http, this);
    return *m_auth;
}

LaunchEngine& Cub3App::launchEngine() {
    if (!m_launcher) {
        m_launcher = new LaunchEngine(&versionManager(), &downloadEngine(),
                                      &httpClient(), &database(), &settings(), this);
    }
    return *m_launcher;
}

} // namespace cub3::application
