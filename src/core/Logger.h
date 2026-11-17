// CUB3 Launcher - Logging with rotation
#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcCub3)
Q_DECLARE_LOGGING_CATEGORY(lcNetwork)
Q_DECLARE_LOGGING_CATEGORY(lcAuth)
Q_DECLARE_LOGGING_CATEGORY(lcLauncher)
Q_DECLARE_LOGGING_CATEGORY(lcStorage)
Q_DECLARE_LOGGING_CATEGORY(lcDownloads)
Q_DECLARE_LOGGING_CATEGORY(lcUI)

namespace cub3::core {

/// Lightweight logger that routes Qt messages to a rotating file.
///
/// Design:
///   - Writes are flushed immediately so recent diagnostics survive a crash.
///   - File size is capped; when exceeded, the file rotates.
///   - Sensitive data (tokens, passwords) is never logged.
///   - In Release builds, only warnings and errors go to stdout.
///   - Redaction: any string wrapped in redactToken() is masked.
class Logger {
public:
    enum Level {
        DebugLevel,
        InfoLevel,
        WarningLevel,
        ErrorLevel,
        FatalLevel
    };

    static Logger& instance();

    /// Initialise the logger. Opens the file and sets up rotation.
    /// Must be called once at startup, before any logging.
    /// Returns false if the file cannot be opened (logging falls back to stderr).
    bool init(const QString& filePath, int maxFiles = 3, qint64 maxSize = 1024 * 1024);

    /// Close and release the file handle.
    void shutdown();

    /// Log a message at the given level.
    void log(Level level, const QString& category, const QString& message);

    /// Mask a token/password for safe logging.
    /// Returns "****XXXX" (last 4 chars visible).
    [[nodiscard]] static QString redact(const QString& secret);

private:
    Logger() = default;
    ~Logger();

    static void qtMessageHandler(QtMsgType type,
                                 const QMessageLogContext& context,
                                 const QString& message);
    static char levelChar(Level level);
    void rotateIfNeeded();

    QFile m_file;
    QTextStream m_stream{&m_file};
    QMutex m_mutex;
    qint64 m_maxSize{1024 * 1024};
    int m_maxFiles{3};
    bool m_initialised{false};
    bool m_handlerInstalled{false};
    QtMessageHandler m_previousHandler{nullptr};
};

} // namespace cub3::core

// --- Convenience macros ---------------------------------------------------
#define CUB3_DEBUG(category)   qCDebug(category)
#define CUB3_INFO(category)   qCInfo(category)
#define CUB3_WARN(category)   qCWarning(category)
#define CUB3_ERROR(category)  qCCritical(category)
