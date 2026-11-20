// CUB3 Launcher - read-only GitLab account integration
#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDateTime>
#include <QTimer>
#include <QUrl>
#include <QStringList>

#include <functional>
#include <vector>
#include "network/HttpClient.h"

namespace cub3::network { class HttpClient; }

namespace cub3::gitlab {

class GitLabManager final : public QObject {
    Q_OBJECT
public:
    explicit GitLabManager(network::HttpClient* http, QObject* parent = nullptr);

    [[nodiscard]] static bool isValidInstanceUrl(const QUrl& url) {
        if (!network::HttpClient::isSecureUrl(url)
            || !url.query().isEmpty() || !url.fragment().isEmpty())
            return false;
        const QStringList segments = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
        for (const QString& segment : segments) {
            if (segment == QStringLiteral(".") || segment == QStringLiteral(".."))
                return false;
        }
        return true;
    }
    [[nodiscard]] static bool isValidProjectId(const QString& id) {
        if (id.isEmpty() || id.size() > 20)
            return false;
        for (const QChar ch : id) {
            if (!ch.isDigit()) return false;
        }
        bool ok = false;
        id.toULongLong(&ok);
        return ok;
    }
    [[nodiscard]] bool isConnected() const { return !m_accessToken.isEmpty(); }
    [[nodiscard]] QString username() const { return m_username; }
    [[nodiscard]] QString instanceUrl() const { return m_instanceUrl; }

    void authorize(const QString& instance, const QString& clientId);
    void disconnectAccount();
    void listProjects();
    void loadProject(const QString& projectId, const QString& branch);

signals:
    void connectionChanged(bool connected, const QString& username);
    void authorizationCodeReady(const QString& verificationUrl, const QString& userCode);
    void projectsReady(const QJsonArray& projects);
    void projectDataReady(const QJsonArray& branches, const QJsonArray& commits,
                          const QJsonArray& releases, const QJsonArray& tags);
    void errorOccurred(const QString& message);

private:
    using JsonCallback = std::function<void(const QJsonValue&, int)>;
    using TokenCallback = std::function<void(bool)>;

    void restoreSession();
    void pollDeviceToken();
    void verifySession();
    void ensureFreshToken(TokenCallback callback);
    void refreshToken();
    void requestJson(const QUrl& url, JsonCallback callback);
    void getApi(const QString& path, JsonCallback callback);
    void persistSession();
    void clearLocalSession();
    QUrl endpoint(const QString& path) const;
    static QString apiError(const QJsonValue& value, int status);

    network::HttpClient* m_http{nullptr};
    QTimer m_pollTimer;
    QString m_instanceUrl, m_clientId, m_username;
    QString m_accessToken, m_refreshToken, m_deviceCode;
    QDateTime m_expiresAt;
    int m_pollIntervalSeconds{5};
    int m_pollAttemptsRemaining{0};
    bool m_refreshInProgress{false};
    std::vector<TokenCallback> m_refreshWaiters;
};

} // namespace cub3::gitlab
