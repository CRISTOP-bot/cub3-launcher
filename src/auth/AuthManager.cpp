// CUB3 Launcher - Authentication manager implementation
#include "AuthManager.h"

#include "auth/elyby/ElybyAuth.h"
#include "auth/microsoft/MicrosoftAuth.h"
#include "network/HttpClient.h"

namespace cub3::auth {

AuthManager::AuthManager(network::HttpClient* http, QObject* parent)
    : QObject(parent)
    , m_microsoft(new MicrosoftAuth(http, this))
    , m_elyby(new ElybyAuth(http, this))
{
}

void AuthManager::loginMicrosoft(
    std::function<void(const Account&)> onComplete,
    std::function<void(const QString&)> onError)
{
    m_microsoft->login(
        [this](const QString& code, const QString& uri) {
            emit microsoftDeviceCode(code, uri);
        },
        [this, onComplete](const Account& account) {
            m_currentAccount = account;
            emit accountChanged(m_currentAccount);
            if (onComplete)
                onComplete(m_currentAccount);
        },
        [onError](const QString& error) {
            if (onError)
                onError(error);
        });
}

void AuthManager::loginElyby(
    const QString& username, const QString& password,
    std::function<void(const Account&)> onComplete,
    std::function<void(const QString&)> onError)
{
    m_elyby->login(username, password,
        [this, onComplete](const Account& account) {
            m_currentAccount = account;
            emit accountChanged(m_currentAccount);
            if (onComplete)
                onComplete(m_currentAccount);
        },
        [onError](const QString& error) {
            if (onError)
                onError(error);
        });
}

void AuthManager::logout() {
    m_currentAccount = Account{};
    emit accountChanged(m_currentAccount);
    emit logoutComplete();
}

void AuthManager::refreshIfNeeded(std::function<void(bool)> onComplete) {
    if (m_currentAccount.username.isEmpty() || m_currentAccount.accessToken.isEmpty()) {
        if (onComplete)
            onComplete(false);
        return;
    }

    if (!m_currentAccount.needsRefresh()) {
        if (onComplete)
            onComplete(true);
        return;
    }

    const auto finish = [this, onComplete](const Account& account) {
        m_currentAccount = account;
        emit accountChanged(m_currentAccount);
        if (onComplete)
            onComplete(m_currentAccount.isValid());
    };
    const auto fail = [onComplete](const QString&) {
        if (onComplete)
            onComplete(false);
    };

    if (m_currentAccount.provider == Provider::Microsoft) {
        m_microsoft->refresh(m_currentAccount, finish, fail);
    } else {
        m_elyby->refresh(m_currentAccount.accessToken, finish, fail);
    }
}

} // namespace cub3::auth
