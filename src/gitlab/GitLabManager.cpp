// CUB3 Launcher - read-only GitLab account integration
#include "GitLabManager.h"
#include "core/Logger.h"
#include "network/HttpClient.h"

#include <qt6keychain/keychain.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrlQuery>
#include <memory>
#include <utility>

namespace cub3::gitlab {
namespace {
const QString kCredentialService = QStringLiteral("CUB3 Launcher");
const QString kCredentialKey = QStringLiteral("gitlab-session");
const QString kDeviceGrant = QStringLiteral("urn:ietf:params:oauth:grant-type:device_code");

QByteArray encodeForm(const QUrlQuery& query) {
    return query.query(QUrl::FullyEncoded).toUtf8();
}

bool parseObject(const QByteArray& bytes, QJsonObject* object) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;
    *object = document.object();
    return true;
}
} // namespace

GitLabManager::GitLabManager(network::HttpClient* http, QObject* parent)
    : QObject(parent), m_http(http) {
    m_pollTimer.setSingleShot(true);
    connect(&m_pollTimer, &QTimer::timeout, this, &GitLabManager::pollDeviceToken);
    restoreSession();
}

QUrl GitLabManager::endpoint(const QString& path) const {
    QUrl url(m_instanceUrl);
    QString basePath = url.path();
    if (basePath.endsWith(QLatin1Char('/')))
        basePath.chop(1);
    const qsizetype queryAt = path.indexOf(QLatin1Char('?'));
    url.setPath(basePath + (queryAt < 0 ? path : path.left(queryAt)));
    if (queryAt >= 0)
        url.setQuery(QUrlQuery(path.mid(queryAt + 1)));
    return url;
}

QString GitLabManager::apiError(const QJsonValue& value, int status) {
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        QString message = object.value(QStringLiteral("error_description")).toString();
        if (message.isEmpty()) message = object.value(QStringLiteral("message")).toString();
        if (message.isEmpty()) message = object.value(QStringLiteral("error")).toString();
        if (!message.isEmpty()) return message.left(300);
    }
    return QStringLiteral("GitLab respondió con HTTP %1 o con datos no válidos.").arg(status);
}

void GitLabManager::authorize(const QString& instanceUrl, const QString& clientId) {
    const QUrl instance(instanceUrl.trimmed());
    if (!isValidInstanceUrl(instance) || clientId.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("Indica una instancia GitLab HTTPS válida y el ID de una aplicación OAuth registrada."));
        return;
    }
    if (!m_http) {
        emit errorOccurred(QStringLiteral("El cliente de red no está disponible."));
        return;
    }
    m_pollTimer.stop();
    m_instanceUrl = instance.toString();
    m_clientId = clientId.trimmed();
    m_deviceCode.clear();

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    form.addQueryItem(QStringLiteral("scope"), QStringLiteral("read_api read_user"));
    m_http->post(endpoint(QStringLiteral("/oauth/authorize_device")), encodeForm(form),
        [this](const QByteArray& bytes, int status) {
            QJsonObject object;
            const bool parsed = parseObject(bytes, &object);
            if (status < 200 || status >= 300 || !parsed) {
                emit errorOccurred(apiError(parsed ? QJsonValue(object) : QJsonValue{}, status));
                return;
            }
            const QString code = object.value(QStringLiteral("device_code")).toString();
            const QString userCode = object.value(QStringLiteral("user_code")).toString();
            const QUrl verifyUrl(object.value(QStringLiteral("verification_uri")).toString());
            if (code.isEmpty() || userCode.isEmpty()
                || !network::HttpClient::isSecureUrl(verifyUrl)
                || verifyUrl.host().compare(QUrl(m_instanceUrl).host(), Qt::CaseInsensitive) != 0) {
                emit errorOccurred(QStringLiteral("GitLab devolvió una respuesta de autorización incompleta o insegura."));
                return;
            }
            m_deviceCode = code;
            m_pollIntervalSeconds = qBound(2, object.value(QStringLiteral("interval")).toInt(5), 60);
            const int expires = qBound(30, object.value(QStringLiteral("expires_in")).toInt(300), 1800);
            m_pollAttemptsRemaining = qMax(1, expires / m_pollIntervalSeconds);
            emit authorizationCodeReady(verifyUrl.toString(), userCode);
            m_pollTimer.start(m_pollIntervalSeconds * 1000);
        }, {{QStringLiteral("Content-Type"), QStringLiteral("application/x-www-form-urlencoded")} });
}

void GitLabManager::pollDeviceToken() {
    if (m_deviceCode.isEmpty() || m_pollAttemptsRemaining <= 0) {
        m_deviceCode.clear();
        emit errorOccurred(QStringLiteral("El código de autorización expiró. Intenta conectar de nuevo."));
        return;
    }
    --m_pollAttemptsRemaining;
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("grant_type"), kDeviceGrant);
    form.addQueryItem(QStringLiteral("device_code"), m_deviceCode);
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    m_http->post(endpoint(QStringLiteral("/oauth/token")), encodeForm(form),
        [this](const QByteArray& bytes, int status) {
            QJsonObject object;
            if (!parseObject(bytes, &object)) {
                if (status == 0 || status == 429 || status >= 500) {
                    m_pollTimer.start(m_pollIntervalSeconds * 1000);
                    return;
                }
                m_deviceCode.clear();
                emit errorOccurred(QStringLiteral("GitLab devolvió una respuesta OAuth no válida."));
                return;
            }
            const QString error = object.value(QStringLiteral("error")).toString();
            if (status >= 200 && status < 300 && !object.value(QStringLiteral("access_token")).toString().isEmpty()) {
                m_accessToken = object.value(QStringLiteral("access_token")).toString();
                m_refreshToken = object.value(QStringLiteral("refresh_token")).toString();
                m_expiresAt = QDateTime::currentDateTimeUtc().addSecs(
                    qMax(0, object.value(QStringLiteral("expires_in")).toInt(7200)));
                m_deviceCode.clear();
                verifySession();
            } else if (error == QStringLiteral("authorization_pending")) {
                m_pollTimer.start(m_pollIntervalSeconds * 1000);
            } else if (error == QStringLiteral("slow_down")) {
                m_pollIntervalSeconds = qMin(m_pollIntervalSeconds + 5, 60);
                m_pollTimer.start(m_pollIntervalSeconds * 1000);
            } else {
                m_deviceCode.clear();
                emit errorOccurred(error == QStringLiteral("expired_token")
                    ? QStringLiteral("El código de autorización expiró.") : apiError(object, status));
            }
        }, {{QStringLiteral("Content-Type"), QStringLiteral("application/x-www-form-urlencoded")} });
}

void GitLabManager::restoreSession() {
    auto* job = new QKeychain::ReadPasswordJob(kCredentialService, this);
    job->setKey(kCredentialKey);
    connect(job, &QKeychain::Job::finished, this, [this, job](QKeychain::Job* finished) {
        auto* read = qobject_cast<QKeychain::ReadPasswordJob*>(finished);
        if (!read || finished->error() != QKeychain::NoError) {
            job->deleteLater();
            return;
        }
        QJsonObject stored;
        if (!parseObject(read->textData().toUtf8(), &stored)) {
            job->deleteLater();
            emit errorOccurred(QStringLiteral("La sesión GitLab guardada no tiene un formato válido."));
            return;
        }
        job->deleteLater();
        const QString instance = stored.value(QStringLiteral("instance")).toString();
        if (!isValidInstanceUrl(QUrl(instance))) {
            emit errorOccurred(QStringLiteral("La instancia guardada de GitLab no es válida."));
            return;
        }
        m_instanceUrl = instance;
        m_clientId = stored.value(QStringLiteral("client_id")).toString();
        m_accessToken = stored.value(QStringLiteral("access_token")).toString();
        m_refreshToken = stored.value(QStringLiteral("refresh_token")).toString();
        m_username = stored.value(QStringLiteral("username")).toString();
        m_expiresAt = QDateTime::fromSecsSinceEpoch(
            stored.value(QStringLiteral("expires_at")).toVariant().toLongLong(), Qt::UTC);
        if (m_accessToken.isEmpty()) {
            clearLocalSession();
            return;
        }
        verifySession();
    });
    job->start();
}

void GitLabManager::persistSession() {
    const QJsonObject object{
        {QStringLiteral("instance"), m_instanceUrl},
        {QStringLiteral("client_id"), m_clientId},
        {QStringLiteral("access_token"), m_accessToken},
        {QStringLiteral("refresh_token"), m_refreshToken},
        {QStringLiteral("username"), m_username},
        {QStringLiteral("expires_at"), m_expiresAt.toSecsSinceEpoch()}
    };
    auto* job = new QKeychain::WritePasswordJob(kCredentialService, this);
    job->setKey(kCredentialKey);
    job->setTextData(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
    connect(job, &QKeychain::Job::finished, this, [this, job](QKeychain::Job* finished) {
        if (finished->error() != QKeychain::NoError) {
            CUB3_WARN(lcAuth) << "Could not store GitLab session in system keychain";
            emit errorOccurred(QStringLiteral("No se pudo guardar la sesión GitLab en el llavero seguro del sistema."));
            clearLocalSession();
        }
        job->deleteLater();
    });
    job->start();
}

void GitLabManager::clearLocalSession() {
    m_pollTimer.stop();
    m_deviceCode.clear();
    m_accessToken.clear();
    m_refreshToken.clear();
    m_username.clear();
    m_expiresAt = {};
    auto* job = new QKeychain::DeletePasswordJob(kCredentialService, this);
    job->setKey(kCredentialKey);
    connect(job, &QKeychain::Job::finished, job, &QObject::deleteLater);
    job->start();
    emit connectionChanged(false, {});
}

void GitLabManager::disconnectAccount() {
    clearLocalSession();
}

void GitLabManager::verifySession() {
    getApi(QStringLiteral("/api/v4/user"), [this](const QJsonValue& value, int status) {
        if (status < 200 || status >= 300 || !value.isObject()) {
            clearLocalSession();
            emit errorOccurred(apiError(value, status));
            return;
        }
        m_username = value.toObject().value(QStringLiteral("username")).toString();
        if (m_username.isEmpty()) {
            clearLocalSession();
            emit errorOccurred(QStringLiteral("GitLab no devolvió el nombre de usuario."));
            return;
        }
        persistSession();
        emit connectionChanged(true, m_username);
    });
}

void GitLabManager::ensureFreshToken(TokenCallback callback) {
    if (m_accessToken.isEmpty()) {
        if (callback) callback(false);
        return;
    }
    if (!m_expiresAt.isValid() || m_expiresAt > QDateTime::currentDateTimeUtc().addSecs(60)) {
        if (callback) callback(true);
        return;
    }
    if (m_refreshToken.isEmpty()) {
        clearLocalSession();
        if (callback) callback(false);
        return;
    }
    m_refreshWaiters.push_back(std::move(callback));
    if (!m_refreshInProgress)
        refreshToken();
}

void GitLabManager::refreshToken() {
    m_refreshInProgress = true;
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    form.addQueryItem(QStringLiteral("refresh_token"), m_refreshToken);
    form.addQueryItem(QStringLiteral("client_id"), m_clientId);
    m_http->post(endpoint(QStringLiteral("/oauth/token")), encodeForm(form),
        [this](const QByteArray& bytes, int status) {
            QJsonObject object;
            if (status >= 200 && status < 300 && parseObject(bytes, &object)
                && !object.value(QStringLiteral("access_token")).toString().isEmpty()) {
                m_accessToken = object.value(QStringLiteral("access_token")).toString();
                m_refreshToken = object.value(QStringLiteral("refresh_token")).toString();
                m_expiresAt = QDateTime::currentDateTimeUtc().addSecs(
                    qMax(0, object.value(QStringLiteral("expires_in")).toInt(7200)));
                persistSession();
                m_refreshInProgress = false;
                auto waiters = std::move(m_refreshWaiters);
                m_refreshWaiters.clear();
                for (auto& waiter : waiters) if (waiter) waiter(true);
                return;
            }
            m_refreshInProgress = false;
            auto waiters = std::move(m_refreshWaiters);
            m_refreshWaiters.clear();
            for (auto& waiter : waiters) if (waiter) waiter(false);
            clearLocalSession();
            emit errorOccurred(QStringLiteral("No se pudo renovar la sesión GitLab; vuelve a conectar la cuenta."));
        }, {{QStringLiteral("Content-Type"), QStringLiteral("application/x-www-form-urlencoded")} });
}

void GitLabManager::requestJson(const QUrl& url, JsonCallback callback) {
    m_http->get(url, [callback = std::move(callback)](const QByteArray& bytes, int status) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError) {
            if (callback) callback(QJsonObject{}, status);
            return;
        }
        const QJsonValue value = document.isArray() ? QJsonValue(document.array())
            : document.isObject() ? QJsonValue(document.object()) : QJsonValue{};
        if (callback) callback(value, status);
    }, {{QStringLiteral("Authorization"), QStringLiteral("Bearer ") + m_accessToken}});
}

void GitLabManager::getApi(const QString& path, JsonCallback callback) {
    ensureFreshToken([this, path, callback = std::move(callback)](bool valid) mutable {
        if (!valid) {
            if (callback) callback(QJsonObject{}, 401);
            return;
        }
        requestJson(endpoint(path), [this, callback = std::move(callback)](
            const QJsonValue& value, int status) mutable {
            if (status == 429) {
                emit errorOccurred(QStringLiteral("GitLab limitó temporalmente las solicitudes; espera un momento e inténtalo de nuevo."));
            } else if (status == 401) {
                clearLocalSession();
                emit errorOccurred(QStringLiteral("GitLab rechazó la sesión. Vuelve a conectar la cuenta."));
            }
            if (callback) callback(value, status);
        });
    });
}

void GitLabManager::listProjects() {
    getApi(QStringLiteral("/api/v4/projects?membership=true&per_page=100"),
        [this](const QJsonValue& value, int status) {
            if (status < 200 || status >= 300 || !value.isArray()) {
                emit errorOccurred(apiError(value, status));
                return;
            }
            emit projectsReady(value.toArray());
        });
}

void GitLabManager::loadProject(const QString& projectId, const QString& branch) {
    const QString safeId = projectId.trimmed();
    if (!isValidProjectId(safeId)) {
        emit errorOccurred(QStringLiteral("El identificador del proyecto no es válido."));
        return;
    }
    const QString prefix = QStringLiteral("/api/v4/projects/%1").arg(safeId);
    QString commitsPath = prefix + QStringLiteral("/repository/commits?per_page=20");
    if (!branch.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("per_page"), QStringLiteral("20"));
        query.addQueryItem(QStringLiteral("ref_name"), branch);
        commitsPath = prefix + QStringLiteral("/repository/commits?")
            + query.query(QUrl::FullyEncoded);
    }
    auto results = std::make_shared<QList<QJsonArray>>(4);
    auto remaining = std::make_shared<int>(4);
    auto failed = std::make_shared<bool>(false);
    const auto done = [this, results, remaining, failed](int index,
            const QJsonValue& value, int status) {
        if (status < 200 || status >= 300 || !value.isArray()) {
            if (!*failed) emit errorOccurred(apiError(value, status));
            *failed = true;
            return;
        }
        (*results)[index] = value.toArray();
        if (--(*remaining) == 0)
            emit projectDataReady((*results)[0], (*results)[1], (*results)[2], (*results)[3]);
    };
    getApi(prefix + QStringLiteral("/repository/branches?per_page=100"),
        [done](const QJsonValue& value, int status) { done(0, value, status); });
    getApi(commitsPath,
        [done](const QJsonValue& value, int status) { done(1, value, status); });
    getApi(prefix + QStringLiteral("/releases?per_page=20"),
        [done](const QJsonValue& value, int status) { done(2, value, status); });
    getApi(prefix + QStringLiteral("/repository/tags?per_page=20"),
        [done](const QJsonValue& value, int status) { done(3, value, status); });
}

} // namespace cub3::gitlab
