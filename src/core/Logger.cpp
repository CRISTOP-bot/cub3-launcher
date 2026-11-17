// CUB3 Launcher - Logging implementation
#include "Logger.h"
#include "Paths.h"

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

Q_LOGGING_CATEGORY(lcCub3,     "cub3.core")
Q_LOGGING_CATEGORY(lcNetwork,  "cub3.network")
Q_LOGGING_CATEGORY(lcAuth,     "cub3.auth")
Q_LOGGING_CATEGORY(lcLauncher,  "cub3.launcher")
Q_LOGGING_CATEGORY(lcStorage,  "cub3.storage")
Q_LOGGING_CATEGORY(lcDownloads, "cub3.downloads")
Q_LOGGING_CATEGORY(lcUI,       "cub3.ui")

namespace cub3::core {

// --- Singleton ------------------------------------------------------------

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    shutdown();
}

// --- Initialisation ------------------------------------------------------

bool Logger::init(const QString& filePath, int maxFiles, qint64 maxSize) {
    QMutexLocker locker(&m_mutex);

    m_maxFiles = std::max(maxFiles, 1);
    m_maxSize = std::max<qint64>(maxSize, 1);

    // Ensure directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // Fallback to stderr
        m_initialised = false;
        return false;
    }

    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);
    m_initialised = true;
    m_previousHandler = qInstallMessageHandler(&Logger::qtMessageHandler);
    m_handlerInstalled = true;

    // Write a separator header
    m_stream << QDateTime::currentDateTime().toString(Qt::ISODate)
             << " CUB3 Launcher started\n";
    m_stream.flush();

    return true;
}

void Logger::shutdown() {
    if (m_handlerInstalled) {
        qInstallMessageHandler(m_previousHandler);
        m_handlerInstalled = false;
        m_previousHandler = nullptr;
    }

    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
    m_initialised = false;
}

// --- Logging --------------------------------------------------------------

void Logger::log(Level level, const QString& category, const QString& message) {
    QMutexLocker locker(&m_mutex);

    if (m_initialised && m_file.isOpen()) {
        rotateIfNeeded();

        m_stream << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
                 << " [" << levelChar(level) << "] "
                 << category << ": "
                 << message << '\n';
        m_stream.flush();
    } else {
        const QByteArray output = QStringLiteral("[%1] %2: %3\n")
                                      .arg(QChar::fromLatin1(levelChar(level)))
                                      .arg(category, message)
                                      .toUtf8();
        std::fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stderr);
        std::fflush(stderr);
    }
}

QString Logger::redact(const QString& secret) {
    if (secret.size() <= 4)
        return QStringLiteral("****");
    return QStringLiteral("****") + secret.right(4);
}

// --- Internal -------------------------------------------------------------

char Logger::levelChar(Level level) {
    static const char chars[] = {'D', 'I', 'W', 'E', 'F'};
    return chars[static_cast<int>(level)];
}

void Logger::qtMessageHandler(QtMsgType type,
                              const QMessageLogContext& context,
                              const QString& message) {
    Level level = InfoLevel;
    switch (type) {
    case QtDebugMsg: level = DebugLevel; break;
    case QtInfoMsg: level = InfoLevel; break;
    case QtWarningMsg: level = WarningLevel; break;
    case QtCriticalMsg: level = ErrorLevel; break;
    case QtFatalMsg: level = FatalLevel; break;
    }

    const QString category = context.category
        ? QString::fromUtf8(context.category)
        : QStringLiteral("default");
    Logger::instance().log(level, category, message);
    if (type == QtFatalMsg)
        std::abort();
}

void Logger::rotateIfNeeded() {
    if (m_file.size() < m_maxSize)
        return;

    // Close current file
    m_stream.flush();
    m_file.close();

    // Rotate: log -> log.1 -> log.2 -> ... delete oldest
    const QString base = m_file.fileName();
    for (int i = m_maxFiles - 1; i > 0; --i) {
        QString older = QString("%1.%2").arg(base).arg(i);
        QString newer = QString("%1.%2").arg(base).arg(i - 1);
        if (QFile::exists(newer)) {
            if (QFile::exists(older))
                QFile::remove(older);
            QFile::rename(newer, older);
        }
    }
    QFile::rename(base, base + ".1");

    // Reopen
    m_file.setFileName(base);
    m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_file);
}

} // namespace cub3::core
