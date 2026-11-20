// CUB3 Launcher - Discord Rich Presence over the local Discord IPC endpoint
#include "DiscordPresence.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QtEndian>

namespace cub3::discord {
namespace {
constexpr quint32 kHandshakeOpcode = 0;
constexpr quint32 kFrameOpcode = 1;
constexpr quint32 kCloseOpcode = 2;
constexpr quint32 kPingOpcode = 3;
constexpr quint32 kPongOpcode = 4;
constexpr quint32 kMaximumPayload = 1024 * 1024;
constexpr int kRetryIntervalMs = 15000;
}

DiscordPresence::DiscordPresence(QObject* parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_retryTimer(new QTimer(this))
    , m_handshakeTimer(new QTimer(this))
{
    m_retryTimer->setInterval(kRetryIntervalMs);
    m_handshakeTimer->setSingleShot(true);
    m_handshakeTimer->setInterval(5000);
    connect(m_handshakeTimer, &QTimer::timeout, this, [this] {
        if (m_enabled && !m_ready)
            m_socket->abort();
    });
    connect(m_retryTimer, &QTimer::timeout,
            this, &DiscordPresence::connectToDiscord);
    connect(m_socket, &QLocalSocket::connected,
            this, &DiscordPresence::onConnected);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &DiscordPresence::onReadyRead);
    connect(m_socket, &QLocalSocket::disconnected, this, [this] {
        m_handshakeTimer->stop();
        m_ready = false;
        m_handshakeSent = false;
        m_receiveBuffer.clear();
        if (m_enabled && isValidApplicationId(m_applicationId)) {
            setStatus(QStringLiteral("Discord no está conectado; reintentando."));
            m_retryTimer->start();
        }
    });
    connect(m_socket, &QLocalSocket::errorOccurred,
            this, &DiscordPresence::onSocketError);
}

DiscordPresence::~DiscordPresence() {
    m_retryTimer->stop();
    clearAndDisconnect();
}

bool DiscordPresence::isValidApplicationId(const QString& applicationId) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9]{17,20}$"));
    return pattern.match(applicationId).hasMatch();
}

QByteArray DiscordPresence::encodeFrame(quint32 opcode,
                                        const QByteArray& payload) {
    QByteArray frame(8, Qt::Uninitialized);
    qToLittleEndian<quint32>(opcode,
        reinterpret_cast<uchar*>(frame.data()));
    qToLittleEndian<quint32>(static_cast<quint32>(payload.size()),
        reinterpret_cast<uchar*>(frame.data() + 4));
    frame.append(payload);
    return frame;
}

void DiscordPresence::configure(bool enabled, const QString& applicationId) {
    const QString normalizedId = applicationId.trimmed();
    const bool idChanged = normalizedId != m_applicationId;
    if (!enabled || !isValidApplicationId(normalizedId)) {
        m_enabled = false;
        m_retryTimer->stop();
        clearAndDisconnect();
        if (enabled && !normalizedId.isEmpty())
            setStatus(QStringLiteral("El ID de aplicación de Discord no es válido."));
        else if (enabled)
            setStatus(QStringLiteral("Configura un ID de aplicación para activar Discord."));
        else
            setStatus(QStringLiteral("Presencia de Discord desactivada."));
        return;
    }

    if (idChanged) {
        m_retryTimer->stop();
        clearAndDisconnect();
    }
    m_applicationId = normalizedId;
    m_enabled = true;
    if (m_socket->state() == QLocalSocket::ConnectedState && m_ready) {
        sendActivity();
        setStatus(QStringLiteral("Conectado a Discord."));
        return;
    }
    if (!m_retryTimer->isActive())
        connectToDiscord();
}

void DiscordPresence::setActivity(const QString& details, const QString& state) {
    m_details = details.left(128);
    m_state = state.left(128);
    m_activityStartTimestamp = QDateTime::currentSecsSinceEpoch();
    if (m_enabled && m_ready)
        sendActivity();
}

void DiscordPresence::discoverEndpoints() {
    m_endpoints.clear();
#ifdef Q_OS_WIN
    for (int index = 0; index < 10; ++index)
        m_endpoints.append(QStringLiteral("\\\\?\\pipe\\discord-ipc-%1").arg(index));
#else
    QStringList directories;
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!runtime.isEmpty())
        directories.append(runtime);
    const QString xdgRuntime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (!xdgRuntime.isEmpty())
        directories.append(xdgRuntime);
    const QString tempEnvironment = qEnvironmentVariable("TMPDIR");
    if (!tempEnvironment.isEmpty())
        directories.append(tempEnvironment);
    directories.append(QDir::tempPath());
    directories.removeDuplicates();
    for (const QString& directory : directories) {
        for (int index = 0; index < 10; ++index)
            m_endpoints.append(QDir(directory).filePath(
                QStringLiteral("discord-ipc-%1").arg(index)));
    }
    m_endpoints.removeDuplicates();
#endif
}

void DiscordPresence::connectToDiscord() {
    m_retryTimer->stop();
    if (!m_enabled || !isValidApplicationId(m_applicationId))
        return;
    if (m_socket->state() != QLocalSocket::UnconnectedState)
        return;
    discoverEndpoints();
    m_endpointIndex = 0;
    setStatus(QStringLiteral("Buscando Discord en este equipo…"));
    tryNextEndpoint();
}

void DiscordPresence::tryNextEndpoint() {
    if (!m_enabled || m_socket->state() != QLocalSocket::UnconnectedState)
        return;
    if (m_endpointIndex >= m_endpoints.size()) {
        setStatus(QStringLiteral("Discord no está abierto; se reintentará automáticamente."));
        m_retryTimer->start();
        return;
    }
    m_handshakeSent = false;
    m_ready = false;
    m_receiveBuffer.clear();
    m_socket->connectToServer(m_endpoints.at(m_endpointIndex++),
                              QIODevice::ReadWrite);
}

void DiscordPresence::onConnected() {
    if (!m_enabled) {
        clearAndDisconnect();
        return;
    }
    m_retryTimer->stop();
    sendHandshake();
}

void DiscordPresence::sendHandshake() {
    if (m_handshakeSent || m_socket->state() != QLocalSocket::ConnectedState)
        return;
    const QJsonObject handshake{
        {QStringLiteral("v"), 1},
        {QStringLiteral("client_id"), m_applicationId}
    };
    m_socket->write(encodeFrame(kHandshakeOpcode,
        QJsonDocument(handshake).toJson(QJsonDocument::Compact)));
    m_socket->flush();
    m_handshakeSent = true;
    m_handshakeTimer->start();
    setStatus(QStringLiteral("Conectando con Discord…"));
}

void DiscordPresence::onReadyRead() {
    m_receiveBuffer.append(m_socket->readAll());
    while (m_receiveBuffer.size() >= 8) {
        const auto* bytes = reinterpret_cast<const uchar*>(m_receiveBuffer.constData());
        const quint32 opcode = qFromLittleEndian<quint32>(bytes);
        const quint32 payloadSize = qFromLittleEndian<quint32>(bytes + 4);
        if (payloadSize > kMaximumPayload) {
            m_socket->abort();
            return;
        }
        const qsizetype frameSize = 8 + static_cast<qsizetype>(payloadSize);
        if (m_receiveBuffer.size() < frameSize)
            return;
        const QByteArray payload = m_receiveBuffer.mid(8,
            static_cast<qsizetype>(payloadSize));
        m_receiveBuffer.remove(0, frameSize);

        if (opcode == kPingOpcode) {
            m_socket->write(encodeFrame(kPongOpcode, payload));
            m_socket->flush();
        } else if (opcode == kCloseOpcode) {
            m_socket->disconnectFromServer();
            return;
        } else if (opcode == kFrameOpcode) {
            const QJsonObject response = QJsonDocument::fromJson(payload)
                .object();
            if (response.value(QStringLiteral("evt")).toString()
                    == QStringLiteral("READY")) {
                m_ready = true;
                m_handshakeTimer->stop();
                m_retryTimer->stop();
                setStatus(QStringLiteral("Conectado a Discord."));
                sendActivity();
            } else if (response.value(QStringLiteral("evt")).toString()
                           == QStringLiteral("ERROR")
                       || response.value(QStringLiteral("cmd")).toString()
                           == QStringLiteral("ERROR")) {
                m_socket->abort();
                return;
            }
        }
    }
}

void DiscordPresence::sendActivity() {
    if (!m_enabled || !m_ready
        || m_socket->state() != QLocalSocket::ConnectedState)
        return;
    if (m_activityStartTimestamp <= 0)
        m_activityStartTimestamp = QDateTime::currentSecsSinceEpoch();
    const QJsonObject activity{
        {QStringLiteral("name"), QStringLiteral("CUB3 Launcher")},
        {QStringLiteral("type"), 0},
        {QStringLiteral("details"), m_details},
        {QStringLiteral("state"), m_state},
        {QStringLiteral("timestamps"), QJsonObject{
            {QStringLiteral("start"), m_activityStartTimestamp}
        }}
    };
    const QJsonObject args{
        {QStringLiteral("pid"), static_cast<qint64>(QCoreApplication::applicationPid())},
        {QStringLiteral("activity"), activity}
    };
    const QJsonObject request{
        {QStringLiteral("cmd"), QStringLiteral("SET_ACTIVITY")},
        {QStringLiteral("args"), args},
        {QStringLiteral("nonce"), QUuid::createUuid().toString(QUuid::WithoutBraces)}
    };
    m_socket->write(encodeFrame(kFrameOpcode,
        QJsonDocument(request).toJson(QJsonDocument::Compact)));
    m_socket->flush();
}

void DiscordPresence::clearAndDisconnect() {
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        const QJsonObject request{
            {QStringLiteral("cmd"), QStringLiteral("SET_ACTIVITY")},
            {QStringLiteral("args"), QJsonObject{
                {QStringLiteral("pid"), static_cast<qint64>(QCoreApplication::applicationPid())},
                {QStringLiteral("activity"), QJsonObject{}}
            }},
            {QStringLiteral("nonce"), QUuid::createUuid().toString(QUuid::WithoutBraces)}
        };
        m_socket->write(encodeFrame(kFrameOpcode,
            QJsonDocument(request).toJson(QJsonDocument::Compact)));
        m_socket->flush();
        m_socket->disconnectFromServer();
    } else if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_handshakeTimer->stop();
    m_ready = false;
    m_handshakeSent = false;
    m_receiveBuffer.clear();
}

void DiscordPresence::onSocketError() {
    if (m_socket->state() != QLocalSocket::UnconnectedState)
        return;
    if (!m_enabled)
        return;
    tryNextEndpoint();
}

void DiscordPresence::setStatus(const QString& status) {
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged(m_status);
}

} // namespace cub3::discord
