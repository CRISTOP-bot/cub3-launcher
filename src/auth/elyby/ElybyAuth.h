// CUB3 Launcher - Ely.by authentication
#pragma once

#include <QObject>
#include <QString>
#include <functional>

#include "auth/Account.h"

namespace cub3::network { class HttpClient; }

namespace cub3::auth {

/// Ely.by authentication via the Yggdrasil-compatible API.
///
/// Flow:
///   1. POST /auth/authenticate with username + password
///   2. Receive access token + player profile
///   3. Validate token periodically with /auth/validate
///   4. Refresh with /auth/refresh when needed
///
/// AuthManager keeps tokens only in memory for the current process.
/// Passwords are NEVER stored.
class ElybyAuth : public QObject {
    Q_OBJECT

public:
    explicit ElybyAuth(network::HttpClient* http, QObject* parent = nullptr);

    /// Authenticate with username and password.
    /// onComplete: called with the account on success.
    /// onError: called with an error message on failure.
    void login(const QString& username, const QString& password,
               std::function<void(const Account&)> onComplete,
               std::function<void(const QString&)> onError);

    /// Validate the current access token.
    /// Returns true if the token is still valid.
    void validate(const QString& accessToken,
                   std::function<void(bool)> onComplete);

    /// Refresh the access token.
    void refresh(const QString& accessToken,
                 std::function<void(const Account&)> onComplete,
                 std::function<void(const QString&)> onError);

    /// Invalidate (logout) the access token.
    void invalidate(const QString& accessToken,
                   std::function<void(bool)> onComplete);

private:
    network::HttpClient* m_http;
};

} // namespace cub3::auth
