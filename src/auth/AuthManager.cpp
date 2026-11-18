// CUB3 Launcher - Authentication manager implementation
#include "AuthManager.h"

#include "auth/elyby/ElybyAuth.h"
#include "auth/microsoft/MicrosoftAuth.h"
#include "network/HttpClient.h"
#include "core/Logger.h"

#include <qt6keychain/keychain.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>

namespace cub3::auth {
namespace {
const QString kCredentialService = QStringLiteral("CUB3 Launcher");
const QString kActiveAccountKey = QStringLiteral("active-account");
}

AuthManager::AuthManager(network::HttpClient* http, QObject* parent)
    : QObject(parent)
    , m_microsoft(new MicrosoftAuth(http, this))
    , m_elyby(new ElybyAuth(http, this))
{
    // Let owners connect to accountChanged before an immediate keychain reply
    // can publish the restored account.
    QTimer::singleShot(0, this, [this] { restoreSession(); });
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
            persistSession();
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
            persistSession();
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
    removePersistedSession();
    emit logoutComplete();
}

void AuthManager::restoreSession() {
    auto* job = new QKeychain::ReadPasswordJob(kCredentialService, this);
    job->setKey(kActiveAccountKey);
    connect(job, &QKeychain::Job::finished, this, [this](QKeychain::Job* finished) {
        auto* readJob = qobject_cast<QKeychain::ReadPasswordJob*>(finished);
        if (finished->error() == QKeychain::EntryNotFound) {
            finishSessionRestore();
            return;
        }
        if (finished->error() != QKeychain::NoError || !readJob) {
            CUB3_WARN(lcAuth) << "Could not restore saved account from system keychain:"
                              << finished->errorString();
            emit credentialStorageError(finished->errorString());
            finishSessionRestore();
            return;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(
            readJob->textData().toUtf8(), &parseError);
        const auto object = document.object();
        const auto provider = object.value(QStringLiteral("provider")).toString();
        if (parseError.error != QJsonParseError::NoError || !document.isObject()
            || (provider != QStringLiteral("microsoft")
                && provider != QStringLiteral("elyby"))) {
            emit credentialStorageError(
                QStringLiteral("La cuenta guardada no se pudo leer correctamente."));
            removePersistedSession();
            finishSessionRestore();
            return;
        }

        m_currentAccount = Account::fromJson(object);
        if (m_currentAccount.username.isEmpty()
            || m_currentAccount.accessToken.isEmpty()) {
            m_currentAccount = Account{};
            removePersistedSession();
            finishSessionRestore();
            return;
        }

        // Refresh expired tokens before exposing the restored account to the UI.
        if (m_currentAccount.needsRefresh() || !m_currentAccount.isValid()) {
            refreshIfNeeded([this](bool refreshed) {
                if (refreshed)
                    persistSession();
                finishSessionRestore();
            });
            return;
        }

        finishSessionRestore();
    });
    job->start();
}

void AuthManager::persistSession() {
    if (m_currentAccount.username.isEmpty() || m_currentAccount.accessToken.isEmpty())
        return;

    auto* job = new QKeychain::WritePasswordJob(kCredentialService, this);
    job->setKey(kActiveAccountKey);
    job->setInsecureFallback(false);
    job->setTextData(QString::fromUtf8(
        QJsonDocument(m_currentAccount.toJson()).toJson(QJsonDocument::Compact)));
    connect(job, &QKeychain::Job::finished, this, [this](QKeychain::Job* finished) {
        if (finished->error() != QKeychain::NoError) {
            CUB3_WARN(lcAuth) << "Could not save account in system keychain:"
                              << finished->errorString();
            emit credentialStorageError(finished->errorString());
        }
    });
    job->start();
}

void AuthManager::removePersistedSession() {
    auto* job = new QKeychain::DeletePasswordJob(kCredentialService, this);
    job->setKey(kActiveAccountKey);
    connect(job, &QKeychain::Job::finished, this, [this](QKeychain::Job* finished) {
        if (finished->error() != QKeychain::NoError
            && finished->error() != QKeychain::EntryNotFound) {
            CUB3_WARN(lcAuth) << "Could not remove saved account from system keychain:"
                              << finished->errorString();
            emit credentialStorageError(finished->errorString());
        }
    });
    job->start();
}

void AuthManager::finishSessionRestore() {
    m_restoring = false;
    emit accountChanged(m_currentAccount);
    emit sessionRestoreFinished(m_currentAccount.isValid());
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
        persistSession();
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
