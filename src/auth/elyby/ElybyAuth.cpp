// CUB3 Launcher - Ely.by authentication implementation
#include "ElybyAuth.h"
#include "network/HttpClient.h"
#include "core/AppMeta.h"
#include "core/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace cub3::auth {

// Ely.by API endpoints
static const QString kAuthUrl =
    QStringLiteral("https://authserver.ely.by/auth/authenticate");
static const QString kValidateUrl =
    QStringLiteral("https://authserver.ely.by/auth/validate");
static const QString kRefreshUrl =
    QStringLiteral("https://authserver.ely.by/auth/refresh");
static const QString kInvalidateUrl =
    QStringLiteral("https://authserver.ely.by/auth/invalidate");

ElybyAuth::ElybyAuth(network::HttpClient* http, QObject* parent)
    : QObject(parent)
    , m_http(http)
{
}

void ElybyAuth::login(const QString& username, const QString& password,
                       std::function<void(const Account&)> onComplete,
                       std::function<void(const QString&)> onError)
{
    CUB3_INFO(lcAuth) << "Starting Ely.by login for user:" << username;

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    body["agent"] = QJsonObject{{"name", "Minecraft"}, {"version", 1}};
    body["requestUser"] = true;

    m_http->post(QUrl(kAuthUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [username, onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                auto doc = QJsonDocument::fromJson(data);
                QString errorMsg = doc.object().value("errorMessage").toString();
                if (errorMsg.isEmpty())
                    errorMsg = QStringLiteral("Authentication failed: %1").arg(status);
                onError(errorMsg);
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();
            const QString accessToken = obj.value("accessToken").toString();
            if (accessToken.isEmpty()) {
                onError(QStringLiteral("Authentication response did not contain an access token"));
                return;
            }

            Account account;
            account.provider = Provider::Elyby;
            account.username = obj.value("selectedProfile")
                                   .toObject().value("name").toString();
            if (account.username.isEmpty())
                account.username = username;
            account.uuid = obj.value("selectedProfile")
                               .toObject().value("id").toString();
            account.accessToken = accessToken;
            account.expiresAt = QDateTime::currentDateTime().addSecs(86400); // 24h estimate

            CUB3_INFO(lcAuth) << "Ely.by login complete:" << account.username;
            onComplete(account);
        });
}

void ElybyAuth::validate(const QString& accessToken,
                          std::function<void(bool)> onComplete)
{
    QJsonObject body;
    body["accessToken"] = accessToken;

    m_http->post(QUrl(kValidateUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [onComplete](const QByteArray&, int status) {
            onComplete(status == 204 || status == 200);
        });
}

void ElybyAuth::refresh(const QString& accessToken,
                         std::function<void(const Account&)> onComplete,
                         std::function<void(const QString&)> onError)
{
    QJsonObject body;
    body["accessToken"] = accessToken;
    body["requestUser"] = true;

    m_http->post(QUrl(kRefreshUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("Token refresh failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();
            const QString accessToken = obj.value("accessToken").toString();
            if (accessToken.isEmpty()) {
                onError(QStringLiteral("Token refresh response did not contain an access token"));
                return;
            }

            Account account;
            account.provider = Provider::Elyby;
            account.username = obj.value("selectedProfile")
                                   .toObject().value("name").toString();
            account.uuid = obj.value("selectedProfile")
                               .toObject().value("id").toString();
            account.accessToken = accessToken;
            account.expiresAt = QDateTime::currentDateTime().addSecs(86400);

            onComplete(account);
        });
}

void ElybyAuth::invalidate(const QString& accessToken,
                            std::function<void(bool)> onComplete)
{
    QJsonObject body;
    body["accessToken"] = accessToken;

    m_http->post(QUrl(kInvalidateUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [onComplete](const QByteArray&, int status) {
            onComplete(status == 204 || status == 200);
        });
}

} // namespace cub3::auth
