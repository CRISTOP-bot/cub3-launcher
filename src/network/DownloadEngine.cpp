// CUB3 Launcher - Download engine implementation
#include "DownloadEngine.h"
#include "HttpClient.h"
#include "core/AppMeta.h"
#include "core/Logger.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMutexLocker>
#include <QPair>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QCryptographicHash>
#include <QTimer>
#include <QThread>
#include <QPointer>
#include <QUuid>
#include <algorithm>
#include <memory>

namespace cub3::network {

DownloadEngine::DownloadEngine(HttpClient* http, QObject* parent)
    : QObject(parent)
    , m_http(http)
{
}

void DownloadEngine::setMaxConcurrent(int max) {
    if (max < 1) max = 1;
    if (max > 16) max = 16;
    m_maxConcurrent = max;
    processQueue();
}

void DownloadEngine::adaptForHDD(bool isHDD) {
    setMaxConcurrent(isHDD ? 2 : std::min(QThread::idealThreadCount(), 4));
}

QString DownloadEngine::submit(const DownloadJob& job) {
    DownloadJob j = job;
    if (j.id.isEmpty())
        j.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    j.maxRetries = std::max(j.maxRetries, 0);

    if (j.targetPath.isEmpty() || !HttpClient::isSecureUrl(j.url)) {
        CUB3_WARN(lcDownloads) << "Rejected invalid download job:" << j.id
                               << j.url.host() << j.targetPath;
        return {};
    }

    auto isDuplicate = [this, &j] {
        if (m_active.contains(j.id))
            return true;
        const QString target = QDir::cleanPath(QFileInfo(j.targetPath).absoluteFilePath());
        const bool activeTarget = std::any_of(
            m_active.cbegin(), m_active.cend(), [&j, &target](const ActiveJob& active) {
                return active.job.targetPath == target
                    || QDir::cleanPath(QFileInfo(active.job.targetPath).absoluteFilePath())
                           == target;
            });
        return activeTarget || std::any_of(
            m_queue.cbegin(), m_queue.cend(), [&j, &target](const DownloadJob& queued) {
                return queued.id == j.id
                    || QDir::cleanPath(QFileInfo(queued.targetPath).absoluteFilePath())
                           == target;
            });
    };
    {
        QMutexLocker locker(&m_mutex);
        if (isDuplicate()) {
            CUB3_WARN(lcDownloads) << "Rejected duplicate download ID:" << j.id;
            return {};
        }
    }

    // Check if file already exists and is verified
    QFileInfo fi(j.targetPath);
    if (fi.isFile()) {
        if (!m_validateDownloads && j.expectedSize > 0
            && fi.size() == j.expectedSize) {
            QMutexLocker locker(&m_mutex);
            if (isDuplicate())
                return {};
            locker.unlock();
            emit progressChanged({j.id, fi.size(), fi.size(), 0,
                                  DownloadStatus::Skipped, {}});
            emit downloadFinished(j.id, DownloadStatus::Skipped, {});
            return j.id;
        }
        if (j.expectedHash.isEmpty()) {
            // No hash to verify — skip if size matches
            if (j.expectedSize > 0 && fi.size() == j.expectedSize) {
                QMutexLocker locker(&m_mutex);
                if (isDuplicate())
                    return {};
                locker.unlock();
                emit progressChanged({j.id, fi.size(), fi.size(), 0,
                                      DownloadStatus::Skipped, {}});
                emit downloadFinished(j.id, DownloadStatus::Skipped, {});
                return j.id;
            }
        } else if (m_validateDownloads
                   && verifyHash(j.targetPath, j.expectedHash, j.hashAlgorithm)) {
            // Hash matches — skip download
            QMutexLocker locker(&m_mutex);
            if (isDuplicate())
                return {};
            locker.unlock();
            emit progressChanged({j.id, fi.size(), fi.size(), 0,
                                  DownloadStatus::Skipped, {}});
            emit downloadFinished(j.id, DownloadStatus::Skipped, {});
            return j.id;
        }
    }

    {
        QMutexLocker locker(&m_mutex);
        if (isDuplicate()) {
            CUB3_WARN(lcDownloads) << "Rejected duplicate download ID:" << j.id;
            return {};
        }
        m_queue.enqueue(j);
    }
    processQueue();
    return j.id;
}

void DownloadEngine::submitBatch(const QList<DownloadJob>& jobs) {
    QList<QPair<QString, QString>> rejected;
    QMutexLocker locker(&m_mutex);
    for (const auto& job : jobs) {
        DownloadJob j = job;
        if (j.id.isEmpty())
            j.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QString scheme = j.url.scheme().toLower();
        if (j.targetPath.isEmpty() || !HttpClient::isSecureUrl(j.url)) {
            rejected.append({j.id, QStringLiteral("Invalid download URL or target path")});
            continue;
        }
        const QString target = QDir::cleanPath(QFileInfo(j.targetPath).absoluteFilePath());
        const bool activeTarget = std::any_of(
            m_active.cbegin(), m_active.cend(), [&j, &target](const ActiveJob& active) {
                return active.job.targetPath == target
                    || QDir::cleanPath(QFileInfo(active.job.targetPath).absoluteFilePath())
                           == target;
            });
        const bool duplicate = m_active.contains(j.id) || activeTarget
            || std::any_of(m_queue.cbegin(), m_queue.cend(),
                           [&j, &target](const DownloadJob& queued) {
                return queued.id == j.id
                    || QDir::cleanPath(QFileInfo(queued.targetPath).absoluteFilePath())
                           == target;
            });
        if (duplicate) {
            rejected.append({j.id, QStringLiteral("Duplicate download ID")});
            continue;
        }
        j.maxRetries = std::max(j.maxRetries, 0);
        m_queue.enqueue(j);
    }
    locker.unlock();
    for (const auto& item : rejected)
        emit downloadFinished(item.first, DownloadStatus::Failed, item.second);
    processQueue();
}

void DownloadEngine::cancel(const QString& id) {
    bool queued = false;
    QPointer<QNetworkReply> activeReply;
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_active.find(id);
        if (it != m_active.end() && it->reply) {
            it->cancelled = true;
            activeReply = it->reply;
        } else {
            for (int i = 0; i < m_queue.size(); ++i) {
                if (m_queue.at(i).id == id) {
                    m_queue.removeAt(i);
                    queued = true;
                    break;
                }
            }
        }
    }
    if (activeReply) {
        activeReply->abort();
        return;
    }
    if (queued)
        emit downloadFinished(id, DownloadStatus::Cancelled, {});
}

void DownloadEngine::cancelAll() {
    QList<QString> queuedIds;
    QList<QPointer<QNetworkReply>> activeReplies;
    {
        QMutexLocker locker(&m_mutex);
        for (const auto& job : m_queue)
            queuedIds.append(job.id);
        m_queue.clear();
        for (auto it = m_active.begin(); it != m_active.end(); ++it) {
            it->cancelled = true;
            if (it->reply)
                activeReplies.append(it->reply);
        }
    }
    for (const auto& reply : activeReplies) {
        if (reply)
            reply->abort();
    }
    for (const auto& id : queuedIds)
        emit downloadFinished(id, DownloadStatus::Cancelled, {});
}

int DownloadEngine::activeCount() const {
    QMutexLocker locker(&m_mutex);
    return m_active.size() + m_queue.size();
}

void DownloadEngine::processQueue() {
    QMutexLocker locker(&m_mutex);
    while (m_active.size() < m_maxConcurrent && !m_queue.isEmpty()) {
        DownloadJob job = m_queue.dequeue();
        locker.unlock();
        startJob(job);
        locker.relock();
    }
}

void DownloadEngine::startJob(const DownloadJob& job) {
    CUB3_INFO(lcDownloads) << "Starting download from host:" << job.url.host()
                           << "->" << job.targetPath;

    QNetworkRequest request(job.url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      core::meta::userAgent());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    // Ensure target directory exists
    QFileInfo fi(job.targetPath);
    if (!QDir().mkpath(fi.absolutePath())) {
        onJobDone(job.id, DownloadStatus::Failed, "Cannot create target directory");
        return;
    }

    // Write to .part first
    const QString tempPath = job.targetPath + ".part";
    QFile* file = new QFile(tempPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        delete file;
        onJobDone(job.id, DownloadStatus::Failed, "Cannot open temp file");
        return;
    }

    QMutexLocker locker(&m_mutex);
    ActiveJob active;
    active.job = job;
    active.startTime = QDateTime::currentMSecsSinceEpoch();
    m_active[job.id] = active;

    QNetworkReply* reply = m_http->getManager().get(request);
    reply->setReadBufferSize(1024 * 1024);
    m_active[job.id].reply = reply;
    locker.unlock();

    auto* timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    timeout->setInterval(30000);
    connect(timeout, &QTimer::timeout, reply, [reply] {
        reply->setProperty("cub3TimedOut", true);
        reply->abort();
    });
    timeout->start();

    // Throttle progress updates to 4 per second and enforce declared size
    // while streaming so a bad or compromised manifest cannot fill the disk.
    auto lastProgress = std::make_shared<qint64>(0);
    auto bytesWritten = std::make_shared<qint64>(0);

    connect(reply, &QIODevice::readyRead, this,
            [reply, file, timeout, job, bytesWritten]() {
        timeout->start();
        const QByteArray data = reply->readAll();
        if (job.expectedSize > 0
            && (*bytesWritten > job.expectedSize - data.size())) {
            reply->setProperty("cub3SizeExceeded", true);
            reply->abort();
            return;
        }
        if (file->write(data) != data.size()) {
            reply->setProperty("cub3DiskWriteFailed", true);
            reply->abort();
            return;
        }
        *bytesWritten += data.size();
    });

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, reply, job, lastProgress](qint64 done, qint64 total) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - *lastProgress < 250) return; // throttle to 4/s
        *lastProgress = now;

        qint64 startTime = now;
        {
            QMutexLocker locker(&m_mutex);
            const auto it = m_active.constFind(job.id);
            if (it != m_active.constEnd())
                startTime = it->startTime;
        }
        qint64 elapsed = now - startTime;
        qint64 speed = (elapsed > 0) ? (done * 1000 / elapsed) : 0;

        DownloadProgress prog;
        prog.id = job.id;
        prog.downloaded = done;
        prog.total = total;
        prog.speedBps = speed;
        prog.status = DownloadStatus::Downloading;
        emit progressChanged(prog);
    });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, job, tempPath, timeout]() {
        timeout->stop();
        bool writeFailed = reply->property("cub3DiskWriteFailed").toBool();
        const bool sizeExceeded = reply->property("cub3SizeExceeded").toBool();
        if (!file->flush() || file->error() != QFile::NoError)
            writeFailed = true;
        file->close();
        delete file;

        bool cancelled = false;
        {
            QMutexLocker locker(&m_mutex);
            const auto it = m_active.constFind(job.id);
            cancelled = it != m_active.constEnd() && it->cancelled;
        }
        if (cancelled) {
            QFile::remove(tempPath);
            onJobDone(job.id, DownloadStatus::Cancelled, {});
            reply->deleteLater();
            return;
        }

        if (sizeExceeded) {
            QFile::remove(tempPath);
            onJobDone(job.id, DownloadStatus::Failed,
                      QStringLiteral("Download exceeded the expected size"));
            reply->deleteLater();
            return;
        }

        if (writeFailed) {
            QFile::remove(tempPath);
            onJobDone(job.id, DownloadStatus::Failed,
                      QStringLiteral("Could not write downloaded data to disk"));
            reply->deleteLater();
            return;
        }

        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            QFile::remove(tempPath);
            const QString error = reply->error() == QNetworkReply::NoError
                ? QStringLiteral("Unexpected HTTP status: %1").arg(status)
                : reply->property("cub3TimedOut").toBool()
                    ? QStringLiteral("Download timed out")
                    : reply->errorString();
            onJobDone(job.id, DownloadStatus::Failed, error,
                      status == 0 || status >= 500);
            reply->deleteLater();
            return;
        }

        // Verify size
        if (m_validateDownloads && job.expectedSize > 0) {
            QFileInfo tempFi(tempPath);
            const qint64 actualSize = tempFi.size();
            if (actualSize != job.expectedSize) {
                QFile::remove(tempPath);
                onJobDone(job.id, DownloadStatus::Failed,
                          QStringLiteral("Size mismatch: expected %1, got %2")
                              .arg(job.expectedSize).arg(actualSize));
                reply->deleteLater();
                return;
            }
        }

        // Verify hash
        if (m_validateDownloads && !job.expectedHash.isEmpty()) {
            const qint64 downloadedSize = QFileInfo(tempPath).size();
            emit progressChanged({job.id, downloadedSize, downloadedSize, 0,
                                  DownloadStatus::Verifying, {}});
            if (!verifyHash(tempPath, job.expectedHash,
                            job.hashAlgorithm.isEmpty() ?
                            "sha1" : job.hashAlgorithm)) {
                QFile::remove(tempPath);
                onJobDone(job.id, DownloadStatus::Failed,
                          QStringLiteral("Hash verification failed"));
                reply->deleteLater();
                return;
            }
        }

        // Replace the target only after the completed temporary file is ready.
        if (QFile::exists(job.targetPath) && !QFile::remove(job.targetPath)) {
            QFile::remove(tempPath);
            onJobDone(job.id, DownloadStatus::Failed,
                      QStringLiteral("Could not replace existing target file"));
            reply->deleteLater();
            return;
        }
        if (!QFile::rename(tempPath, job.targetPath)) {
            QFile::remove(tempPath);
            onJobDone(job.id, DownloadStatus::Failed,
                      QStringLiteral("Could not move downloaded file into place"));
            reply->deleteLater();
            return;
        }

        const qint64 completedSize = QFileInfo(job.targetPath).size();
        emit progressChanged({job.id, completedSize, completedSize, 0,
                              DownloadStatus::Completed, {}});
        onJobDone(job.id, DownloadStatus::Completed, {});
        reply->deleteLater();
    });
}

void DownloadEngine::onJobDone(const QString& id, DownloadStatus status,
                                const QString& error, bool retryable)
{
    bool retry = false;
    DownloadJob retryJob;
    {
        QMutexLocker locker(&m_mutex);
        const auto it = m_active.find(id);
        if (it != m_active.end() && status == DownloadStatus::Failed
            && retryable && !it->cancelled && it->job.maxRetries > 0) {
            retryJob = it->job;
            --retryJob.maxRetries;
            m_queue.enqueue(retryJob);
            retry = true;
        }
        m_active.remove(id);
    }
    if (retry) {
        emit progressChanged({id, 0, retryJob.expectedSize, 0,
                              DownloadStatus::Queued, error});
        processQueue();
        return;
    }
    emit downloadFinished(id, status, error);
    processQueue();
}

bool DownloadEngine::verifyHash(const QString& filePath,
                                 const QString& expected,
                                 const QString& algorithm)
{
    QCryptographicHash::Algorithm algo;
    if (algorithm == "sha1")
        algo = QCryptographicHash::Sha1;
    else if (algorithm == "sha256")
        algo = QCryptographicHash::Sha256;
    else if (algorithm == "md5")
        algo = QCryptographicHash::Md5;
    else if (algorithm.isEmpty() || algorithm == "sha1")
        algo = QCryptographicHash::Sha1;
    else {
        CUB3_ERROR(lcDownloads) << "Unsupported hash algorithm:" << algorithm;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QCryptographicHash hash(algo);
    if (!hash.addData(&file))
        return false;

    file.close();
    return hash.result().toHex() == expected.toLower();
}

} // namespace cub3::network
