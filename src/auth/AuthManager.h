// CUB3 Launcher - Authentication manager
#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <functional>

#include "Account.h"

namespace cub3::network { class HttpClient; }

namespace cub3::auth {

class MicrosoftAuth;
class ElybyAuth;

/// Central authentication manager. Coordinates Microsoft and Ely.by
/// authentication providers. Stores accounts in the database.
///
/// Design:
///   - Tokens live in memory only and are never written to disk.
///   - Callers can refresh expiring session tokens asynchronously.
///   - The launcher never stores passwords.
class AuthManager : public QObject {
    Q_OBJECT

public:
    explicit AuthManager(network::HttpClient* http, QObject* parent = nullptr);

    /// Start the Microsoft OAuth login flow.
    /// Opens the system browser for authentication.
    /// onComplete: called with the account on success, or empty on failure.
    void loginMicrosoft(std::function<void(const Account&)> onComplete,
                        std::function<void(const QString&)> onError);

    /// Start the Ely.by login flow (username + password).
    void loginElyby(const QString& username, const QString& password,
                    std::function<void(const Account&)> onComplete,
                    std::function<void(const QString&)> onError);

    /// Log out the current account.
    void logout();

    /// Get the currently active account.
    [[nodiscard]] Account currentAccount() const { return m_currentAccount; }

    /// Check if a user is logged in.
    [[nodiscard]] bool isLoggedIn() const { return m_currentAccount.isValid(); }

    /// Refresh the current account's tokens if needed.
    /// Returns true if the account was valid or was refreshed successfully.
    void refreshIfNeeded(std::function<void(bool)> onComplete);

signals:
    void accountChanged(const Account& account);
    void logoutComplete();
    void microsoftDeviceCode(const QString& userCode, const QString& verificationUri);

private:
    MicrosoftAuth* m_microsoft{nullptr};
    ElybyAuth* m_elyby{nullptr};
    Account m_currentAccount;
};

} // namespace cub3::auth
