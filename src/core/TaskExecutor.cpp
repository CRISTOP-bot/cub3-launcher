// CUB3 Launcher - Task executor implementation
#include "TaskExecutor.h"
#include "Performance.h"
#include "Logger.h"

#include <QThread>
#include <QMutexLocker>
#include <algorithm>
#include <exception>

namespace cub3::core {

// --- Singleton ------------------------------------------------------------

TaskExecutor& TaskExecutor::instance() {
    static TaskExecutor pool;
    return pool;
}

// --- Construction / destruction -------------------------------------------

TaskExecutor::TaskExecutor(QObject* parent)
    : QObject(parent)
    , m_maxThreads(std::min(QThread::idealThreadCount(), 4))
{
    if (m_maxThreads < 1)
        m_maxThreads = 1;
}

TaskExecutor::~TaskExecutor() {
    shutdown();
}

// --- Configuration --------------------------------------------------------

void TaskExecutor::setMaxConcurrency(int max) {
    if (max < 1) max = 1;
    if (max > 8) max = 8;
    m_maxThreads = max;
}

void TaskExecutor::adaptForHDD(bool isHDD) {
    if (isHDD) {
        setMaxConcurrency(2);
    } else {
        setMaxConcurrency(std::min(QThread::idealThreadCount(), 4));
    }
}

// --- Task submission ------------------------------------------------------

void TaskExecutor::run(std::function<void()> task) {
    {
        QMutexLocker locker(&m_mutex);
        if (!m_running.load(std::memory_order_relaxed)) {
            CUB3_WARN(lcCub3) << "Rejected background task after executor shutdown";
            return;
        }
        m_queue.enqueue(std::move(task));
    }
    m_cond.wakeOne();
    ensureWorkers();
}

int TaskExecutor::pendingCount() const {
    QMutexLocker locker(&m_mutex);
    return m_queue.size() + m_activeTasks;
}

// --- Worker management ----------------------------------------------------

void TaskExecutor::ensureWorkers() {
    QMutexLocker locker(&m_mutex);
    while (m_activeWorkers < m_maxThreads && !m_queue.isEmpty()) {
        auto worker = std::make_unique<Worker>(this);
        worker->setObjectName(QStringLiteral("CUB3-Worker-%1")
                                  .arg(m_workers.size() + 1));
        m_workers.push_back(std::move(worker));
        m_workers.back()->start();
        ++m_activeWorkers;
    }
}

void TaskExecutor::workerLoop() {
    for (;;) {
        std::function<void()> task;
        {
            QMutexLocker locker(&m_mutex);

            while (m_queue.isEmpty() && m_running.load(std::memory_order_relaxed)) {
                m_cond.wait(&m_mutex);
            }

            if (!m_running.load(std::memory_order_relaxed) && m_queue.isEmpty()) {
                --m_activeWorkers;
                m_cond.wakeAll();
                return;
            }

            if (m_queue.isEmpty())
                continue;

            task = m_queue.dequeue();
            ++m_activeTasks;
        }

        // Execute the task outside the lock
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                CUB3_ERROR(lcCub3) << "Background task failed:" << e.what();
            } catch (...) {
                CUB3_ERROR(lcCub3) << "Background task failed with an unknown exception";
            }
        }

        {
            QMutexLocker locker(&m_mutex);
            --m_activeTasks;
            m_cond.wakeAll();
        }
    }
}

// --- Shutdown -------------------------------------------------------------

void TaskExecutor::waitForAll() {
    for (;;) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty() && m_activeTasks == 0)
            return;
        m_cond.wait(&m_mutex);
    }
}

void TaskExecutor::shutdown() {
    // Let workers finish queued tasks before exiting.
    {
        QMutexLocker locker(&m_mutex);
        m_running.store(false, std::memory_order_relaxed);
        m_cond.wakeAll();
    }

    for (auto& worker : m_workers) {
        worker->wait();
    }
    m_workers.clear();
}

} // namespace cub3::core
