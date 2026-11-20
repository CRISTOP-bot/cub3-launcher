// CUB3 Launcher - Discord Rich Presence over the local Discord IPC endpoint
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtGlobal>

class QLocalSocket;
class QTimer;

namespace cub3::discord {

/// Optional Discord Rich Presence client. It uses Discord's local IPC protocol
/// and has no SDK or network dependency. The application ID is public and must
/// belong to an application registered by the user in the Discord Developer Portal.
class DiscordPresence final : public QObject {
    Q_OBJECT

public:
    explicit DiscordPresence(QObject* parent = nullptr);
    ~DiscordPresence() override;

    void configure(bool enabled, const QString& applicationId);
    void setActivity(const QString& details, const QString& state);
    [[nodiscard]] QString status() const { return m_status; }

    [[nodiscard]] static bool isValidApplicationId(const QString& applicationId);
    [[nodiscard]] static QByteArray encodeFrame(quint32 opcode,
                                                const QByteArray& payload);

signals:
    void statusChanged(const QString& status);

private slots:
    void connectToDiscord();
    void onConnected();
    void onReadyRead();
    void onSocketError();

private:
    void discoverEndpoints();
    void tryNextEndpoint();
    void sendHandshake();
    void sendActivity();
    void clearAndDisconnect();
    void setStatus(const QString& status);

    QLocalSocket* m_socket{nullptr};
    QTimer* m_retryTimer{nullptr};
    QTimer* m_handshakeTimer{nullptr};
    QStringList m_endpoints;
    QString m_applicationId;
    QString m_status{QStringLiteral("Desactivado")};
    QString m_details{QStringLiteral("En el launcher")};
    QString m_state{QStringLiteral("Preparando tu próxima partida")};
    QByteArray m_receiveBuffer;
    int m_endpointIndex{0};
    bool m_enabled{false};
    bool m_handshakeSent{false};
    bool m_ready{false};
    qint64 m_activityStartTimestamp{0};
};

} // namespace cub3::discord
