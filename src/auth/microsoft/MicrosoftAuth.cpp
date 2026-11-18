// CUB3 Launcher - Microsoft authentication implementation
#include "MicrosoftAuth.h"
#include "auth/Account.h"
#include "network/HttpClient.h"
#include "core/AppMeta.h"
#include "core/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QDesktopServices>
#include <QDateTime>
#include <QSettings>
#include <algorithm>
#include <memory>

namespace cub3::auth {

// Azure AD endpoints (public Microsoft application used by many launchers)
static const QString kDeviceCodeUrl =
    QStringLiteral("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode");
static const QString kTokenUrl =
    QStringLiteral("https://login.microsoftonline.com/consumers/oauth2/v2.0/token");
static const QString kXblUrl =
    QStringLiteral("https://user.auth.xboxlive.com/user/authenticate");
static const QString kXstsUrl =
    QStringLiteral("https://xsts.auth.xboxlive.com/xsts/authorize");
static const QString kMinecraftAuthUrl =
    QStringLiteral("https://api.minecraftservices.com/authentication/login_with_xbox");
static const QString kProfileUrl =
    QStringLiteral("https://api.minecraftservices.com/minecraft/profile");

// Scopes
static const QString kScope =
    QStringLiteral("XboxLive.signin offline_access");

namespace {
QString configuredClientId() {
    const QString environmentValue =
        qEnvironmentVariable("CUB3_MICROSOFT_CLIENT_ID").trimmed();
    if (!environmentValue.isEmpty())
        return environmentValue;
    const QString savedValue =
        QSettings().value(QStringLiteral("auth/microsoftClientId")).toString().trimmed();
    return savedValue.isEmpty()
        ? cub3::core::meta::kMicrosoftClientId.toString() : savedValue;
}

bool hasConfiguredClientId() {
    const QString id = configuredClientId();
    return !id.isEmpty()
        && id != QStringLiteral("00000000-0000-0000-0000-000000000000");
}
} // namespace

MicrosoftAuth::MicrosoftAuth(network::HttpClient* http, QObject* parent)
    : QObject(parent)
    , m_http(http)
{
}

void MicrosoftAuth::login(
    std::function<void(const QString&, const QString&)> onDeviceCode,
    std::function<void(const Account&)> onComplete,
    std::function<void(const QString&)> onError)
{
    CUB3_INFO(lcAuth) << "Starting Microsoft device code flow";
    if (!hasConfiguredClientId()) {
        onError(QStringLiteral(
            "Microsoft sign-in is not configured. Set CUB3_MICROSOFT_CLIENT_ID "
            "to the client ID of your Microsoft public-client app."));
        return;
    }

    requestDeviceCode(
        [this, onDeviceCode, onComplete, onError]
        (const QString& userCode, const QString& verificationUri,
         const QString& deviceCode, int expiresIn, int interval) {
            // Show the user the code
            if (onDeviceCode)
                onDeviceCode(userCode, verificationUri);

            // Open browser automatically
            QDesktopServices::openUrl(QUrl(verificationUri));

            // Start polling
            m_pollInterval = interval;
            pollForToken(deviceCode, interval, expiresIn,
                [this, onComplete, onError]
                (const QString& msToken, const QString& refreshToken,
                 int accessTokenExpiresIn) {
                    // Got Microsoft tokens — proceed to Xbox Live
                    authXboxLive(msToken,
                        [this, onComplete, onError, msToken, refreshToken,
                         accessTokenExpiresIn]
                        (const QString& xblToken, const QString& userHash) {
                            authXSTS(xblToken,
                                [this, onComplete, onError, userHash, msToken,
                                 refreshToken, accessTokenExpiresIn]
                                (const QString& xstsToken) {
                                    authMinecraft(xstsToken, userHash,
                                        [this, onComplete, onError, msToken,
                                         refreshToken, accessTokenExpiresIn,
                                         xstsToken, userHash]
                                        (const QString& mcToken) {
                                            fetchProfile(mcToken,
                                                [this, onComplete, onError, mcToken, refreshToken,
                                                 accessTokenExpiresIn]
                                                (const QString& username,
                                                 const QString& uuid,
                                                 const QString& skinUrl) {
                                                    Account account;
                                                    account.provider = Provider::Microsoft;
                                                    account.username = username;
                                                    account.uuid = uuid;
                                                    account.accessToken = mcToken;
                                                    account.refreshToken = refreshToken;
                                                    account.skinUrl = skinUrl;
                                                    account.expiresAt =
                                                        QDateTime::currentDateTime().addSecs(accessTokenExpiresIn);
                                                    CUB3_INFO(lcAuth) << "Microsoft auth complete:"
                                                                      << username;
                                                    onComplete(account);
                                                },
                                                onError);
                                        },
                                        onError);
                                },
                                onError);
                        },
                        onError);
                },
                onError);
        },
        onError);
}

void MicrosoftAuth::validate(const Account& account,
                             std::function<void(const Account&)> onComplete,
                             std::function<void(const QString&)> onError)
{
    if (account.accessToken.isEmpty()) {
        onError(QStringLiteral("No Minecraft access token available"));
        return;
    }

    fetchProfile(account.accessToken,
        [account, onComplete](const QString& username, const QString& uuid,
                              const QString& skinUrl) mutable {
            Account validated = account;
            if (!username.isEmpty())
                validated.username = username;
            if (!uuid.isEmpty())
                validated.uuid = uuid;
            if (!skinUrl.isEmpty())
                validated.skinUrl = skinUrl;
            onComplete(validated);
        }, onError);
}

void MicrosoftAuth::refresh(const Account& account,
                            std::function<void(const Account&)> onComplete,
                            std::function<void(const QString&)> onError)
{
    if (!hasConfiguredClientId()) {
        onError(QStringLiteral(
            "Microsoft sign-in is not configured. Set CUB3_MICROSOFT_CLIENT_ID "
            "to the client ID of your Microsoft public-client app."));
        return;
    }
    if (account.refreshToken.isEmpty()) {
        onError(QStringLiteral("No refresh token available"));
        return;
    }

    QUrlQuery params;
    params.addQueryItem("client_id", configuredClientId());
    params.addQueryItem("grant_type", "refresh_token");
    params.addQueryItem("refresh_token", account.refreshToken);
    params.addQueryItem("scope", kScope);
    const QByteArray body = params.toString(QUrl::FullyEncoded).toUtf8();
    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";

    m_http->post(QUrl(kTokenUrl), body,
        [this, account, onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("Token refresh failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            const QString microsoftAccessToken = obj.value("access_token").toString();
            if (microsoftAccessToken.isEmpty()) {
                onError(QStringLiteral("Token refresh response did not contain an access token"));
                return;
            }
            const QString refreshToken = obj.value("refresh_token").toString();
            const int expiresIn =
                std::clamp(obj.value("expires_in").toInt(3600), 1, 86400);

            authXboxLive(microsoftAccessToken,
                [this, account, refreshToken, expiresIn, onComplete, onError]
                (const QString& xblToken, const QString& userHash) {
                    authXSTS(xblToken,
                        [this, account, refreshToken, expiresIn, userHash,
                         onComplete, onError](const QString& xstsToken) {
                            authMinecraft(xstsToken, userHash,
                                [this, account, refreshToken, expiresIn,
                                 onComplete, onError](const QString& minecraftToken) {
                                    fetchProfile(minecraftToken,
                                        [account, refreshToken, expiresIn,
                                         minecraftToken, onComplete]
                                        (const QString& username, const QString& uuid,
                                         const QString& skinUrl) {
                                            Account refreshed = account;
                                            refreshed.accessToken = minecraftToken;
                                            if (!refreshToken.isEmpty())
                                                refreshed.refreshToken = refreshToken;
                                            if (!username.isEmpty())
                                                refreshed.username = username;
                                            if (!uuid.isEmpty())
                                                refreshed.uuid = uuid;
                                            if (!skinUrl.isEmpty())
                                                refreshed.skinUrl = skinUrl;
                                            refreshed.expiresAt =
                                                QDateTime::currentDateTime().addSecs(expiresIn);
                                            onComplete(refreshed);
                                        }, onError);
                                }, onError);
                        }, onError);
                }, onError);
        }, headers);
}

void MicrosoftAuth::requestDeviceCode(
    std::function<void(const QString&, const QString&, const QString&, int, int)> onCode,
    std::function<void(const QString&)> onError)
{
    QUrlQuery params;
    params.addQueryItem("client_id", configuredClientId());
    params.addQueryItem("scope", kScope);

    QByteArray body = params.toString(QUrl::FullyEncoded).toUtf8();
    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";

    m_http->post(QUrl(kDeviceCodeUrl), body,
        [onCode, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("Device code request failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            QString userCode = obj.value("user_code").toString();
            QString verificationUri = obj.value("verification_uri").toString();
            QString deviceCode = obj.value("device_code").toString();
            int expiresIn = obj.value("expires_in").toInt(900);
            int interval = obj.value("interval").toInt(5);
            if (userCode.isEmpty() || verificationUri.isEmpty() || deviceCode.isEmpty()) {
                onError(QStringLiteral("Device code response is incomplete"));
                return;
            }

            CUB3_INFO(lcAuth) << "Microsoft device code issued";
            onCode(userCode, verificationUri, deviceCode, expiresIn, interval);
        }, headers);
}

void MicrosoftAuth::pollForToken(
    const QString& deviceCode, int interval, int expiresIn,
    std::function<void(const QString&, const QString&, int)> onComplete,
    std::function<void(const QString&)> onError)
{
    int attempts = 0;
    const int pollInterval = std::clamp(interval, 1, 60);

    auto* timer = new QTimer(this);
    timer->setInterval(pollInterval * 1000);

    QUrlQuery params;
    params.addQueryItem("client_id", configuredClientId());
    params.addQueryItem("device_code", deviceCode);
    params.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");

    QByteArray body = params.toString(QUrl::FullyEncoded).toUtf8();
    const QHash<QString, QString> headers{
        {QStringLiteral("Content-Type"),
         QStringLiteral("application/x-www-form-urlencoded")}
    };

    auto requestPending = std::make_shared<bool>(false);
    connect(timer, &QTimer::timeout, this,
            [this, timer, body, headers, requestPending, attempts,
             onComplete, onError]() mutable {
        if (*requestPending)
            return;
        attempts++;
        if (attempts > m_maxPollAttempts) {
            timer->stop();
            timer->deleteLater();
            onError(QStringLiteral("Authentication timed out"));
            return;
        }

        *requestPending = true;
        m_http->post(QUrl(kTokenUrl), body,
            [this, timer, requestPending, onComplete, onError]
            (const QByteArray& data, int status) {
            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            QString error = obj.value("error").toString();
            const QString accessToken = obj.value("access_token").toString();

            if (status == 200 && !accessToken.isEmpty()) {
                // Success
                timer->stop();
                timer->deleteLater();
                QString refreshToken = obj.value("refresh_token").toString();
                const int accessTokenExpiresIn =
                    std::clamp(obj.value("expires_in").toInt(3600), 1, 86400);
                CUB3_INFO(lcAuth) << "Microsoft token obtained";
                onComplete(accessToken, refreshToken, accessTokenExpiresIn);
            } else if (error == "authorization_pending") {
                *requestPending = false;
                // Keep polling — do nothing
                CUB3_DEBUG(lcAuth) << "Waiting for user authorization...";
            } else if (error == "slow_down") {
                *requestPending = false;
                // Increase interval
                timer->setInterval(timer->interval() + 5000);
            } else if (status == 0) {
                *requestPending = false;
                CUB3_WARN(lcAuth) << "Microsoft token poll failed without an HTTP response";
            } else {
                // Real error
                timer->stop();
                timer->deleteLater();
                onError(QStringLiteral("Auth error (HTTP %1): %2 — %3")
                             .arg(status)
                             .arg(error)
                             .arg(obj.value("error_description").toString()));
            }
        }, headers);
    });

    const int lifetimeSeconds = std::clamp(expiresIn, 1, 86400);
    QTimer::singleShot(lifetimeSeconds * 1000, timer, [timer, onError] {
        if (!timer->isActive())
            return;
        timer->stop();
        timer->deleteLater();
        onError(QStringLiteral("Authentication timed out"));
    });
    timer->start();
}

void MicrosoftAuth::authXboxLive(
    const QString& microsoftToken,
    std::function<void(const QString&, const QString&)> onComplete,
    std::function<void(const QString&)> onError)
{
    QJsonObject body;
    body["Properties"] = QJsonObject{
        {"AuthMethod", "RPS"},
        {"SiteName", "user.auth.xboxlive.com"},
        {"RpsTicket", "d=" + microsoftToken}
    };
    body["RelyingParty"] = "http://auth.xboxlive.com";
    body["TokenType"] = "JWT";

    QHash<QString, QString> headers;
    headers["Accept"] = "application/json";
    headers["x-xbl-contract-version"] = "1";

    m_http->post(QUrl(kXblUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("Xbox Live auth failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            QString token = obj.value("Token").toString();
            auto displayClaims = obj.value("DisplayClaims").toObject();
            auto xuiArray = displayClaims.value("xui").toArray();
            QString userHash;
            if (!xuiArray.isEmpty()) {
                userHash = xuiArray[0].toObject().value("uhs").toString();
            }
            if (token.isEmpty() || userHash.isEmpty()) {
                onError(QStringLiteral("Xbox Live response is incomplete"));
                return;
            }

            CUB3_INFO(lcAuth) << "Xbox Live auth complete";
            onComplete(token, userHash);
        },
        headers);
}

void MicrosoftAuth::authXSTS(
    const QString& xblToken,
    std::function<void(const QString&)> onComplete,
    std::function<void(const QString&)> onError)
{
    QJsonObject body;
    QJsonObject properties;
    properties["SandboxId"] = "RETAIL";
    QJsonArray userTokens;
    userTokens.append(xblToken);
    properties["UserTokens"] = userTokens;
    body["Properties"] = properties;
    body["RelyingParty"] = "rp://api.minecraftservices.com/";
    body["TokenType"] = "JWT";

    QHash<QString, QString> headers;
    headers["Accept"] = "application/json";
    headers["x-xbl-contract-version"] = "1";

    m_http->post(QUrl(kXstsUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("XSTS auth failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            QString token = obj.value("Token").toString();
            if (token.isEmpty()) {
                onError(QStringLiteral("XSTS response did not contain a token"));
                return;
            }
            CUB3_INFO(lcAuth) << "XSTS auth complete";
            onComplete(token);
        },
        headers);
}

void MicrosoftAuth::authMinecraft(
    const QString& xstsToken, const QString& userHash,
    std::function<void(const QString&)> onComplete,
    std::function<void(const QString&)> onError)
{
    QJsonObject body;
    body["identityToken"] = QString("XBL3.0 x=%1;%2").arg(userHash).arg(xstsToken);

    m_http->post(QUrl(kMinecraftAuthUrl), QJsonDocument(body).toJson(QJsonDocument::Compact),
        [onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("Minecraft auth failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            QString token = obj.value("access_token").toString();
            if (token.isEmpty()) {
                onError(QStringLiteral("Minecraft auth response did not contain a token"));
                return;
            }
            CUB3_INFO(lcAuth) << "Minecraft token obtained";
            onComplete(token);
        });
}

void MicrosoftAuth::fetchProfile(
    const QString& mcToken,
    std::function<void(const QString&, const QString&, const QString&)> onComplete,
    std::function<void(const QString&)> onError)
{
    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + mcToken;

    m_http->get(QUrl(kProfileUrl),
        [onComplete, onError](const QByteArray& data, int status) {
            if (status != 200) {
                onError(QStringLiteral("Profile fetch failed: %1").arg(status));
                return;
            }

            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();

            QString username = obj.value("name").toString();
            QString uuid = obj.value("id").toString();
            if (username.isEmpty() || uuid.isEmpty()) {
                onError(QStringLiteral("Minecraft profile response is incomplete"));
                return;
            }

            // Extract skin URL if present
            QString skinUrl;
            auto skins = obj.value("skins").toArray();
            for (const auto& skin : skins) {
                auto skinObj = skin.toObject();
                if (skinObj.value("variant").toString() == "CLASSIC") {
                    skinUrl = skinObj.value("url").toString();
                    break;
                }
            }

            CUB3_INFO(lcAuth) << "Profile fetched:" << username;
            onComplete(username, uuid, skinUrl);
        },
        headers);
}

} // namespace cub3::auth
