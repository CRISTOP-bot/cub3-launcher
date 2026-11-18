// CUB3 Launcher - Download engine
#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QHash>
#include <QQueue>
#include <QMutex>
#include <QList>

class QNetworkReply;

namespace cub3::network {

class HttpClient;

/// A single file download job.
struct DownloadJob {
    QString id;           // unique identifier
    QUrl url;
    QString targetPath;   // final file path
    QString expectedHash; // SHA-1 (Mojang standard), empty to skip
    QString hashAlgorithm; // "sha1" | "sha256" | "md5", default "sha1"
    qint64 expectedSize{-1}; // -1 if unknown
    int priority{0};      // higher = sooner
    int maxRetries{3};
};

/// Download status reported to listeners.
enum class DownloadStatus {
    Queued,
    Downloading,
    Verifying,
    Completed,
    Failed,
    Cancelled,
    Skipped  // file already exists and is verified
};

/// Progress update for a single download.
struct DownloadProgress {
    QString id;
    qint64 downloaded{0};
    qint64 total{-1};
    qint64 speedBps{0};
    DownloadStatus status{DownloadStatus::Queued};
    QString error;
};

/// Central download engine. Manages a bounded queue of file downloads.
///
/// Design:
///   - Concurrency is bounded (default 4, configurable).
///   - Adapts concurrency for HDD (lower) to avoid thrashing.
///   - Files are downloaded to a .part temp before replacing the target.
///   - Hash verification after download (skip if hash matches).
///   - Existing files that pass hash check are skipped (no re-download).
///   - Retries transient network failures up to the job's retry limit.
///   - Progress updates are throttled (max 4 per second per job).
///   - Safe cancellation — partial files are cleaned up.
class DownloadEngine : public QObject {
    Q_OBJECT

public:
    explicit DownloadEngine(HttpClient* http, QObject* parent = nullptr);

    /// Set maximum concurrent downloads.
    void setMaxConcurrent(int max);

    /// Toggle size/hash checks before reusing and after downloading files.
    void setValidateDownloads(bool enabled) { m_validateDownloads = enabled; }

    /// Submit a download job. Returns the job ID (or empty on invalid input).
    QString submit(const DownloadJob& job);

    /// Submit multiple jobs at once (batched, more efficient).
    void submitBatch(const QList<DownloadJob>& jobs);

    /// Cancel a specific download.
    void cancel(const QString& id);

    /// Cancel all downloads.
    void cancelAll();

    /// Number of jobs (queued + active).
    [[nodiscard]] int activeCount() const;

    /// Adapt concurrency for storage type.
    void adaptForHDD(bool isHDD);

signals:
    /// Progress update for a single download.
    void progressChanged(const DownloadProgress& progress);

    /// A download completed (successfully or not).
    void downloadFinished(const QString& id, DownloadStatus status,
                          const QString& error);

    /// Global speed update (all active downloads combined).
    void globalSpeedChanged(qint64 bytesPerSec);

private:
    void processQueue();
    void startJob(const DownloadJob& job);
    void onJobDone(const QString& id, DownloadStatus status,
                   const QString& error, bool retryable = false);
    bool verifyHash(const QString& filePath, const QString& expected,
                    const QString& algorithm);

    HttpClient* m_http;
    int m_maxConcurrent{4};
    bool m_validateDownloads{true};

    struct ActiveJob {
        DownloadJob job;
        qint64 downloaded{0};
        qint64 startTime{0};
        QNetworkReply* reply{nullptr};
        bool cancelled{false};
    };

    QHash<QString, ActiveJob> m_active;
    QQueue<DownloadJob> m_queue;
    mutable QMutex m_mutex;
};

} // namespace cub3::network
