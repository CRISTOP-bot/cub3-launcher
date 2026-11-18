// CUB3 Launcher - Async HTTP client
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QHash>
#include <QByteArray>
#include <functional>

namespace cub3::network {

/// Lightweight wrapper around QNetworkAccessManager.
/// All requests are async — callbacks are invoked on the GUI thread
/// via QNetworkReply signals. No blocking calls.
///
/// Features:
///   - GET and POST requests
///   - Custom headers per request
///   - User-Agent set from AppMeta
///   - Automatic timeout (30s default)
///   - ETag/Last-Modified support for conditional requests
class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(QObject* parent = nullptr);

    /// Remote endpoints must use HTTPS and must not embed credentials in URLs.
    [[nodiscard]] static bool isSecureUrl(const QUrl& url) {
        return url.isValid()
            && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
            && !url.host().isEmpty()
            && url.userName().isEmpty()
            && url.password().isEmpty();
    }

    static constexpr qint64 maxBufferedResponseBytes = 32LL * 1024 * 1024;

    [[nodiscard]] static bool responseChunkWithinLimit(qint64 currentBytes,
                                                        qint64 incomingBytes) {
        return currentBytes >= 0 && incomingBytes >= 0
            && incomingBytes <= maxBufferedResponseBytes
            && currentBytes <= maxBufferedResponseBytes - incomingBytes;
    }

    /// GET request.
    /// onComplete: called with the response body and HTTP status.
    void get(const QUrl& url,
              std::function<void(const QByteArray&, int)> onComplete,
              const QHash<QString, QString>& headers = {});

    /// POST request with JSON body.
    void post(const QUrl& url,
               const QByteArray& body,
               std::function<void(const QByteArray&, int)> onComplete,
               const QHash<QString, QString>& headers = {});

    /// GET with conditional headers (If-None-Match, If-Modified-Since).
    /// Returns whether the response was 304 (not modified).
    void getConditional(const QUrl& url,
                         const QString& etag,
                         const QString& lastModified,
                         std::function<void(const QByteArray&, int, const QHash<QString, QString>&)> onComplete);

    /// Download a file to disk. The DownloadEngine handles the queueing;
    /// this is for single-file fetches.
    void downloadFile(const QUrl& url,
                      const QString& outputPath,
                      std::function<void(bool, const QString&)> onComplete);

    /// Set a custom timeout (ms). Default 30000.
    void setTimeoutMs(int ms) { m_timeoutMs = ms; }

    /// Access the underlying QNetworkAccessManager.
    /// Used by the DownloadEngine for direct download management.
    [[nodiscard]] QNetworkAccessManager& getManager() { return m_nam; }

private:
    QNetworkAccessManager m_nam;
    int m_timeoutMs{30000};

    void applyHeaders(QNetworkRequest& request,
                      const QHash<QString, QString>& headers);
    void setTimeoutTimer(QNetworkReply* reply);
};

} // namespace cub3::network
