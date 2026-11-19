// CUB3 Launcher - Microsoft authentication (OAuth 2.0 device code flow)
#pragma once

#include <QObject>
#include <QString>
#include <QPointer>
#include <QTimer>
#include <QJsonObject>
#include <functional>

#include "auth/Account.h"

namespace cub3::network { class HttpClient; }

namespace cub3::auth {

/// Microsoft OAuth 2.0 authentication via the device code flow.
///
/// Flow:
///   1. Request device code from Azure AD
///   2. User opens browser, enters code
///   3. Poll token endpoint until complete
///   4. Exchange for Xbox Live token
///   5. Exchange XBL token for XSTS token
///   6. Exchange XSTS token for Minecraft access token
///   7. Fetch player profile
///
/// The system browser is opened for the user. No embedded browser.
/// AuthManager persists session tokens in the system credential store.
class MicrosoftAuth : public QObject {
    Q_OBJECT

public:
    explicit MicrosoftAuth(network::HttpClient* http, QObject* parent = nullptr);

    /// Cancel the active Microsoft device-code polling flow, if any.
    void cancelLogin();

    /// Start the device code login flow.
    /// onDeviceCode: called with the user code and verification URI.
    /// onComplete: called with the final account info on success.
    /// onError: called with an error message on failure.
    void login(std::function<void(const QString& userCode,
                                    const QString& verificationUri)> onDeviceCode,
               std::function<void(const Account&)> onComplete,
               std::function<void(const QString&)> onError);

    /// Validate an existing Minecraft access token and refresh profile metadata.
    void validate(const Account& account,
                  std::function<void(const Account&)> onComplete,
                  std::function<void(const QString&)> onError);

    /// Refresh an existing Microsoft account's tokens.
    void refresh(const Account& account,
                 std::function<void(const Account&)> onComplete,
                 std::function<void(const QString&)> onError);

private:
    // Flow steps
    void requestDeviceCode(std::function<void(const QString& userCode,
                                                const QString& verificationUri,
                                                const QString& deviceCode,
                                                int expiresIn,
                                                int interval)> onCode,
                            std::function<void(const QString&)> onError);

    void pollForToken(const QString& deviceCode, int interval, int expiresIn,
                      std::function<void(const QString& accessToken,
                                           const QString& refreshToken,
                                           int accessTokenExpiresIn)> onComplete,
                      std::function<void(const QString&)> onError);

    void authXboxLive(const QString& microsoftToken,
                      std::function<void(const QString& xblToken,
                                           const QString& userHash)> onComplete,
                      std::function<void(const QString&)> onError);

    void authXSTS(const QString& xblToken,
                  std::function<void(const QString& xstsToken)> onComplete,
                  std::function<void(const QString&)> onError);

    void authMinecraft(const QString& xstsToken, const QString& userHash,
                       std::function<void(const QString& mcToken)> onComplete,
                       std::function<void(const QString&)> onError);

    void fetchProfile(const QString& mcToken,
                       std::function<void(const QString& username,
                                            const QString& uuid,
                                            const QString& skinUrl)> onComplete,
                       std::function<void(const QString&)> onError);

    network::HttpClient* m_http;
    int m_pollInterval{5}; // seconds
    int m_maxPollAttempts{180}; // 15 minutes max
    quint64 m_loginRevision{0};
    QPointer<QTimer> m_loginPollTimer;
};

} // namespace cub3::auth
