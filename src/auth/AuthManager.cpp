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
#include <utility>

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
    // Let owners connect to accountChanged before a keychain reply can publish
    // an account. The account is not exposed until provider validation finishes.
    QTimer::singleShot(0, this, [this] { restoreSession(); });
}

void AuthManager::loginMicrosoft(
    std::function<void(const Account&)> onComplete,
    std::function<void(const QString&)> onError)
{
    const quint64 loginRevision = ++m_sessionRevision;
    m_microsoft->login(
        [this](const QString& code, const QString& uri) {
            emit microsoftDeviceCode(code, uri);
        },
        [this, onComplete, loginRevision](const Account& account) {
            if (loginRevision != m_sessionRevision)
                return;
            ++m_sessionRevision;
            m_currentAccount = account;
            const quint64 committedRevision = m_sessionRevision;
            persistSession([this, onComplete, committedRevision](bool) {
                if (committedRevision != m_sessionRevision)
                    return;
                emit accountChanged(m_currentAccount);
                finishSessionRestore();
                if (onComplete)
                    onComplete(m_currentAccount);
            });
        },
        [this, onError, loginRevision](const QString& error) {
            if (loginRevision == m_sessionRevision && onError)
                onError(error);
        });
}

void AuthManager::loginElyby(
    const QString& username, const QString& password,
    std::function<void(const Account&)> onComplete,
    std::function<void(const QString&)> onError)
{
    m_microsoft->cancelLogin();
    const quint64 loginRevision = ++m_sessionRevision;
    m_elyby->login(username, password,
        [this, onComplete, loginRevision](const Account& account) {
            if (loginRevision != m_sessionRevision)
                return;
            ++m_sessionRevision;
            m_currentAccount = account;
            const quint64 committedRevision = m_sessionRevision;
            persistSession([this, onComplete, committedRevision](bool) {
                if (committedRevision != m_sessionRevision)
                    return;
                emit accountChanged(m_currentAccount);
                finishSessionRestore();
                if (onComplete)
                    onComplete(m_currentAccount);
            });
        },
        [this, onError, loginRevision](const QString& error) {
            if (loginRevision == m_sessionRevision && onError)
                onError(error);
        });
}

void AuthManager::cancelLogin() {
    ++m_sessionRevision;
    m_microsoft->cancelLogin();
}

void AuthManager::logout() {
    ++m_sessionRevision;
    m_microsoft->cancelLogin();
    m_currentAccount = Account{};
    emit accountChanged(m_currentAccount);
    removePersistedSession();
    finishSessionRestore();
    emit logoutComplete();
}

void AuthManager::restoreSession() {
    const quint64 revisionAtReadStart = m_sessionRevision;
    enqueueStorageOperation([this, revisionAtReadStart] {
        auto* job = new QKeychain::ReadPasswordJob(kCredentialService, this);
        job->setKey(kActiveAccountKey);
        connect(job, &QKeychain::Job::finished, this,
                [this, revisionAtReadStart](QKeychain::Job* finished) {
            auto* readJob = qobject_cast<QKeychain::ReadPasswordJob*>(finished);
            const auto error = finished->error();
            const QString errorText = finished->errorString();
            const QString storedText = readJob ? readJob->textData() : QString{};
            storageOperationFinished();

            // A login or logout completed while the keychain read was in flight.
            // Its newer in-memory state must never be replaced by the stale read.
            if (revisionAtReadStart != m_sessionRevision) {
                finishSessionRestore();
                return;
            }
            if (error == QKeychain::EntryNotFound) {
                finishSessionRestore();
                return;
            }
            if (error != QKeychain::NoError || !readJob) {
                CUB3_WARN(lcAuth) << "Could not restore saved account from system keychain:"
                                  << errorText;
                emit credentialStorageError(errorText);
                finishSessionRestore();
                return;
            }

            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(storedText.toUtf8(), &parseError);
            const auto object = document.object();
            const auto provider = object.value(QStringLiteral("provider")).toString();
            if (parseError.error != QJsonParseError::NoError || !document.isObject()
                || (provider != QStringLiteral("microsoft")
                    && provider != QStringLiteral("elyby"))) {
                const QString message = QStringLiteral(
                    "La cuenta guardada está dañada o no corresponde a un proveedor compatible.");
                CUB3_WARN(lcAuth) << message;
                emit credentialStorageError(message);
                // Preserve unreadable data for diagnosis/recovery; do not delete it
                // merely because a read or format check failed.
                finishSessionRestore();
                return;
            }

            Account candidate = Account::fromJson(object);
            if (candidate.username.isEmpty() || candidate.accessToken.isEmpty()) {
                const QString message = QStringLiteral(
                    "La cuenta guardada no contiene los datos mínimos de sesión.");
                emit credentialStorageError(message);
                finishSessionRestore();
                return;
            }

            const auto accept = [this, revisionAtReadStart](const Account& validated) {
                if (revisionAtReadStart != m_sessionRevision) {
                    finishSessionRestore();
                    return;
                }
                if (!validated.isValid()) {
                    const QString message = QStringLiteral(
                        "El proveedor no confirmó una sesión válida. Las credenciales guardadas se conservaron.");
                    CUB3_WARN(lcAuth) << message;
                    emit sessionRestoreFailed(message);
                    finishSessionRestore();
                    return;
                }
                m_currentAccount = validated;
                persistSession([this, revisionAtReadStart](bool) {
                    if (revisionAtReadStart != m_sessionRevision) {
                        finishSessionRestore();
                        return;
                    }
                    emit accountChanged(m_currentAccount);
                    finishSessionRestore();
                });
            };
            const auto fail = [this, revisionAtReadStart](const QString& reason) {
                if (revisionAtReadStart == m_sessionRevision) {
                    CUB3_WARN(lcAuth) << "Could not validate restored session:" << reason;
                    emit sessionRestoreFailed(reason);
                }
                finishSessionRestore();
            };

            if (candidate.provider == Provider::Microsoft) {
                if (candidate.needsRefresh()) {
                    m_microsoft->refresh(candidate, accept, fail);
                } else {
                    m_microsoft->validate(candidate, accept,
                        [this, candidate, accept, fail](const QString& validationError) {
                            // Access tokens can be revoked before their advertised expiry.
                            // Try the refresh token before giving up the saved session.
                            m_microsoft->refresh(candidate, accept,
                                [fail, validationError](const QString& refreshError) {
                                    fail(refreshError.isEmpty() ? validationError : refreshError);
                                });
                        });
                }
                return;
            }

            if (candidate.needsRefresh()) {
                m_elyby->refresh(candidate, accept, fail);
            } else {
                m_elyby->validate(candidate.accessToken,
                    [this, candidate, accept, fail](bool valid, const QString& reason) mutable {
                        if (valid) {
                            accept(candidate);
                        } else {
                            // Ely.by's refresh endpoint can recover a session even
                            // when its access token is no longer accepted.
                            m_elyby->refresh(candidate, accept,
                                [fail, reason](const QString& refreshError) {
                                    fail(refreshError.isEmpty() ? reason : refreshError);
                                });
                        }
                    });
            }
        });
        job->start();
    });
}

void AuthManager::persistSession(std::function<void(bool)> onComplete) {
    if (m_currentAccount.username.isEmpty() || m_currentAccount.accessToken.isEmpty()) {
        if (onComplete)
            onComplete(false);
        return;
    }

    const Account snapshot = m_currentAccount;
    const quint64 revision = m_sessionRevision;
    const QString serialized = QString::fromUtf8(
        QJsonDocument(snapshot.toJson()).toJson(QJsonDocument::Compact));
    enqueueStorageOperation([this, revision, serialized, onComplete] {
        // An old queued write may be skipped, but a write already running is
        // ordered before any later logout/delete by the storage queue.
        if (revision != m_sessionRevision) {
            storageOperationFinished();
            if (onComplete)
                onComplete(false);
            return;
        }
        auto* job = new QKeychain::WritePasswordJob(kCredentialService, this);
        job->setKey(kActiveAccountKey);
        job->setInsecureFallback(false);
        job->setTextData(serialized);
        connect(job, &QKeychain::Job::finished, this,
                [this, onComplete](QKeychain::Job* finished) {
            const bool saved = finished->error() == QKeychain::NoError;
            if (!saved) {
                CUB3_WARN(lcAuth) << "Could not save account in system keychain:"
                                  << finished->errorString();
                emit credentialStorageError(finished->errorString());
            } else {
                CUB3_INFO(lcAuth) << "Saved authenticated session in system keychain";
            }
            storageOperationFinished();
            if (onComplete)
                onComplete(saved);
        });
        job->start();
    });
}

void AuthManager::removePersistedSession() {
    enqueueStorageOperation([this] {
        auto* job = new QKeychain::DeletePasswordJob(kCredentialService, this);
        job->setKey(kActiveAccountKey);
        connect(job, &QKeychain::Job::finished, this,
                [this](QKeychain::Job* finished) {
            if (finished->error() != QKeychain::NoError
                && finished->error() != QKeychain::EntryNotFound) {
                CUB3_WARN(lcAuth) << "Could not remove saved account from system keychain:"
                                  << finished->errorString();
                emit credentialStorageError(finished->errorString());
            }
            storageOperationFinished();
        });
        job->start();
    });
}

void AuthManager::enqueueStorageOperation(std::function<void()> operation) {
    m_storageQueue.enqueue(std::move(operation));
    startNextStorageOperation();
}

void AuthManager::startNextStorageOperation() {
    if (m_storageBusy || m_storageQueue.isEmpty())
        return;
    m_storageBusy = true;
    auto operation = m_storageQueue.dequeue();
    operation();
}

void AuthManager::storageOperationFinished() {
    m_storageBusy = false;
    startNextStorageOperation();
}

void AuthManager::finishSessionRestore() {
    if (!m_restoring)
        return;
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

    const quint64 revision = m_sessionRevision;
    const Account previous = m_currentAccount;
    const auto finish = [this, onComplete, revision, previous](const Account& account) {
        // A refresh response for an old account must not resurrect it after logout
        // or overwrite a newer successful login.
        if (revision != m_sessionRevision || previous.accessToken != m_currentAccount.accessToken) {
            if (onComplete)
                onComplete(false);
            return;
        }
        ++m_sessionRevision;
        m_currentAccount = account;
        emit accountChanged(m_currentAccount);
        persistSession();
        if (onComplete)
            onComplete(m_currentAccount.isValid());
    };
    const auto fail = [onComplete](const QString& reason) {
        CUB3_WARN(lcAuth) << "Could not refresh authenticated session:" << reason;
        if (onComplete)
            onComplete(false);
    };

    if (m_currentAccount.provider == Provider::Microsoft) {
        m_microsoft->refresh(m_currentAccount, finish, fail);
    } else {
        m_elyby->refresh(m_currentAccount, finish, fail);
    }
}

} // namespace cub3::auth
