// CUB3 Launcher - Path management
#include "Paths.h"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

namespace cub3::core {

// --- Static storage -------------------------------------------------------
QString Paths::s_dataDir;
QString Paths::s_cacheDir;
QString Paths::s_instancesDir;
QString Paths::s_versionsDir;
QString Paths::s_librariesDir;
QString Paths::s_assetsDir;
QString Paths::s_logsDir;
QString Paths::s_thumbnailsDir;
QString Paths::s_databaseFile;
QString Paths::s_settingsFile;

// --- Public API -----------------------------------------------------------

const QString& Paths::dataDir() {
    if (s_dataDir.isEmpty()) {
#ifdef CUB3_PLATFORM_WINDOWS
        s_dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
#else
        const QString base =
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        s_dataDir = base.isEmpty()
            ? QDir::homePath() + QDir::separator() + ".local" + QDir::separator()
                  + "share" + QDir::separator() + "cub3-launcher"
            : QDir(base).filePath(QStringLiteral("cub3-launcher"));
#endif
        ;
        if (s_dataDir.isEmpty()) {
#ifdef CUB3_PLATFORM_WINDOWS
            s_dataDir = QDir::homePath() + QDir::separator() + "AppData"
                        + QDir::separator() + "Local" + QDir::separator()
                        + "cub3-launcher";
#else
            s_dataDir = QDir::homePath() + QDir::separator() + ".local"
                        + QDir::separator() + "share" + QDir::separator()
                        + "cub3-launcher";
#endif
        }
        if (!s_dataDir.endsWith(QDir::separator()))
            s_dataDir += QDir::separator();
    }
    return s_dataDir;
}

const QString& Paths::cacheDir() {
    if (s_cacheDir.isEmpty()) {
#ifdef CUB3_PLATFORM_WINDOWS
        s_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
        const QString base =
            QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
        s_cacheDir = base.isEmpty()
            ? QDir::homePath() + QDir::separator() + ".cache"
                  + QDir::separator() + "cub3-launcher"
            : QDir(base).filePath(QStringLiteral("cub3-launcher"));
#endif
        if (s_cacheDir.isEmpty()) {
#ifdef CUB3_PLATFORM_WINDOWS
            s_cacheDir = QDir::homePath() + QDir::separator() + "AppData"
                         + QDir::separator() + "Local" + QDir::separator()
                         + "cub3-launcher" + QDir::separator() + "cache";
#endif
        }
        if (!s_cacheDir.endsWith(QDir::separator()))
            s_cacheDir += QDir::separator();
    }
    return s_cacheDir;
}

const QString& Paths::instancesDir() {
    if (s_instancesDir.isEmpty()) {
        s_instancesDir = dataDir() + "instances" + QDir::separator();
    }
    return s_instancesDir;
}

const QString& Paths::versionsDir() {
    if (s_versionsDir.isEmpty()) {
        s_versionsDir = dataDir() + "versions" + QDir::separator();
    }
    return s_versionsDir;
}

const QString& Paths::librariesDir() {
    if (s_librariesDir.isEmpty()) {
        s_librariesDir = dataDir() + "libraries" + QDir::separator();
    }
    return s_librariesDir;
}

const QString& Paths::assetsDir() {
    if (s_assetsDir.isEmpty()) {
        s_assetsDir = dataDir() + "assets" + QDir::separator();
    }
    return s_assetsDir;
}

const QString& Paths::logsDir() {
    if (s_logsDir.isEmpty()) {
        s_logsDir = dataDir() + "logs" + QDir::separator();
    }
    return s_logsDir;
}

const QString& Paths::thumbnailsDir() {
    if (s_thumbnailsDir.isEmpty()) {
        s_thumbnailsDir = cacheDir() + "thumbnails" + QDir::separator();
    }
    return s_thumbnailsDir;
}

const QString& Paths::databaseFile() {
    if (s_databaseFile.isEmpty()) {
        s_databaseFile = dataDir() + "cub3.db";
    }
    return s_databaseFile;
}

const QString& Paths::settingsFile() {
    if (s_settingsFile.isEmpty()) {
        s_settingsFile = dataDir() + "settings.json";
    }
    return s_settingsFile;
}

bool Paths::ensureDirectories() {
    return ensureDir(dataDir())
           && ensureDir(cacheDir())
           && ensureDir(instancesDir())
           && ensureDir(versionsDir())
           && ensureDir(librariesDir())
           && ensureDir(assetsDir())
           && ensureDir(logsDir())
           && ensureDir(thumbnailsDir());
}

bool Paths::ensureDir(const QString& path) {
    if (path.isEmpty())
        return false;
    QDir dir(path);
    return dir.exists() || dir.mkpath(QStringLiteral("."));
}

} // namespace cub3::core
