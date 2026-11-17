// CUB3 Launcher - HTTP client implementation
#include "HttpClient.h"
#include "core/AppMeta.h"
#include "core/Logger.h"

#include <QNetworkRequest>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace cub3::network {

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
{
    // QNetworkAccessManager manages its own connection pool.
    // No need to configure anything special for performance.
}

void HttpClient::applyHeaders(QNetworkRequest& request,
                              const QHash<QString, QString>& headers) {
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      cub3::core::meta::userAgent());
    request.setRawHeader("Accept", "application/json");

    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
}

void HttpClient::setTimeoutTimer(QNetworkReply* reply) {
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply] {
        if (reply->isRunning()) {
            CUB3_WARN(lcNetwork) << "Request timed out:" << reply->url();
            reply->abort();
        }
    });
    timer->start(m_timeoutMs);
    // Timer is parented to reply and auto-deleted with it
}

void HttpClient::get(const QUrl& url,
                      std::function<void(const QByteArray&, int)> onComplete,
                      const QHash<QString, QString>& headers)
{
    QNetworkRequest request(url);
    applyHeaders(request, headers);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* reply = m_nam.get(request);
    setTimeoutTimer(reply);

    connect(reply, &QNetworkReply::finished, this,
            [reply, onComplete]() {
        QByteArray body;
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            CUB3_WARN(lcNetwork) << "GET failed:" << reply->url()
                                  << "error:" << reply->error();
        }
        if (onComplete) onComplete(body, status);
        reply->deleteLater();
    });
}

void HttpClient::post(const QUrl& url,
                       const QByteArray& body,
                       std::function<void(const QByteArray&, int)> onComplete,
                       const QHash<QString, QString>& headers)
{
    QNetworkRequest request(url);
    applyHeaders(request, headers);
    QString contentType = QStringLiteral("application/json");
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        if (it.key().compare(QStringLiteral("Content-Type"),
                             Qt::CaseInsensitive) == 0) {
            contentType = it.value();
            break;
        }
    }
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* reply = m_nam.post(request, body);
    setTimeoutTimer(reply);

    connect(reply, &QNetworkReply::finished, this,
            [reply, onComplete]() {
        QByteArray responseBody;
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        responseBody = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            CUB3_WARN(lcNetwork) << "POST failed:" << reply->url()
                                  << "error:" << reply->error();
        }
        if (onComplete) onComplete(responseBody, status);
        reply->deleteLater();
    });
}

void HttpClient::getConditional(const QUrl& url,
                                 const QString& etag,
                                 const QString& lastModified,
                                 std::function<void(const QByteArray&, int, const QHash<QString, QString>&)> onComplete)
{
    QHash<QString, QString> headers;
    if (!etag.isEmpty())
        headers["If-None-Match"] = etag;
    if (!lastModified.isEmpty())
        headers["If-Modified-Since"] = lastModified;

    QNetworkRequest request(url);
    applyHeaders(request, headers);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* reply = m_nam.get(request);
    setTimeoutTimer(reply);

    connect(reply, &QNetworkReply::finished, this,
            [reply, onComplete]() {
        QByteArray body;
        int status = 0;
        QHash<QString, QString> responseHeaders;

        status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        for (const auto& header : reply->rawHeaderList())
            responseHeaders[QString::fromUtf8(header)] =
                QString::fromUtf8(reply->rawHeader(header));

        if (reply->error() != QNetworkReply::NoError && status != 304)
            CUB3_WARN(lcNetwork) << "Conditional GET failed:" << reply->url();
        body = reply->readAll();

        if (onComplete) onComplete(body, status, responseHeaders);
        reply->deleteLater();
    });
}

void HttpClient::downloadFile(const QUrl& url,
                               const QString& outputPath,
                               std::function<void(bool, const QString&)> onComplete)
{
    QNetworkRequest request(url);
    applyHeaders(request, {});

    auto* reply = m_nam.get(request);
    setTimeoutTimer(reply);

    // Ensure target directory exists
    QFileInfo fi(outputPath);
    if (!QDir().mkpath(fi.absolutePath())) {
        reply->abort();
        if (onComplete)
            onComplete(false, QStringLiteral("Cannot create output directory"));
        reply->deleteLater();
        return;
    }

    // Write to a temp file first, then rename atomically
    const QString tempPath = outputPath + ".part";
    QFile* file = new QFile(tempPath, reply);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        CUB3_ERROR(lcNetwork) << "Cannot open temp file:" << tempPath;
        if (onComplete) onComplete(false, QStringLiteral("Cannot open file"));
        reply->abort();
        reply->deleteLater();
        return;
    }

    connect(reply, &QIODevice::readyRead, this, [reply, file]() {
        const QByteArray data = reply->readAll();
        if (file->write(data) != data.size()) {
            reply->setProperty("cub3DiskWriteFailed", true);
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this,
            [reply, file, tempPath, outputPath, onComplete]() {
        bool writeFailed = reply->property("cub3DiskWriteFailed").toBool();
        if (!file->flush() || file->error() != QFile::NoError)
            writeFailed = true;
        file->close();

        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (writeFailed) {
            QFile::remove(tempPath);
            if (onComplete)
                onComplete(false, QStringLiteral("Could not write downloaded data to disk"));
        } else if (reply->error() != QNetworkReply::NoError
                   || status < 200 || status >= 300) {
            CUB3_WARN(lcNetwork) << "Download failed:" << reply->url()
                                  << "error:" << reply->error();
            QFile::remove(tempPath);
            const QString error = reply->error() == QNetworkReply::NoError
                ? QStringLiteral("Unexpected HTTP status: %1").arg(status)
                : reply->errorString();
            if (onComplete) onComplete(false, error);
        } else {
            const bool canReplace = !QFile::exists(outputPath)
                || QFile::remove(outputPath);
            if (canReplace && QFile::rename(tempPath, outputPath)) {
                if (onComplete) onComplete(true, outputPath);
            } else {
                QFile::remove(tempPath);
                if (onComplete)
                    onComplete(false, QStringLiteral("Could not move downloaded file into place"));
            }
        }
        reply->deleteLater();
    });
}

} // namespace cub3::network
