// CUB3 Launcher - Path management
#pragma once

#include <QString>

namespace cub3::core {

/// Centralised access to all filesystem paths the launcher uses.
/// Every path is resolved lazily and cached on first access.
/// All paths are guaranteed to end with a trailing separator.
class Paths {
public:
    Paths() = delete;
    Paths(const Paths&) = delete;
    Paths& operator=(const Paths&) = delete;

    /// Root directory for all CUB3 data (platform-specific).
    /// Linux:   ~/.local/share/cub3-launcher
    /// Windows: %APPDATA%/cub3-launcher
    [[nodiscard]] static const QString& dataDir();

    /// Cache directory for manifests, thumbnails, temporary downloads.
    /// Linux:   ~/.cache/cub3-launcher
    /// Windows: %LOCALAPPDATA%/cub3-launcher/cache
    [[nodiscard]] static const QString& cacheDir();

    /// Minecraft instances root.
    [[nodiscard]] static const QString& instancesDir();

    /// Minecraft versions root (libraries, assets, version JSONs).
    [[nodiscard]] static const QString& versionsDir();

    /// Per-instance libraries directory.
    [[nodiscard]] static const QString& librariesDir();

    /// Minecraft assets directory.
    [[nodiscard]] static const QString& assetsDir();

    /// Log directory with rotation.
    [[nodiscard]] static const QString& logsDir();

    /// SQLite database file path.
    [[nodiscard]] static const QString& databaseFile();

    /// Settings JSON file path (fallback / export).
    [[nodiscard]] static const QString& settingsFile();

    /// Thumbnails cache directory.
    [[nodiscard]] static const QString& thumbnailsDir();

    /// Ensure all directories exist. Creates them recursively.
    /// Returns true if all directories were created (or already existed).
    static bool ensureDirectories();

private:
    static bool ensureDir(const QString& path);

    static QString s_dataDir;
    static QString s_cacheDir;
    static QString s_instancesDir;
    static QString s_versionsDir;
    static QString s_librariesDir;
    static QString s_assetsDir;
    static QString s_logsDir;
    static QString s_thumbnailsDir;
    static QString s_databaseFile;
    static QString s_settingsFile;
};

} // namespace cub3::core
