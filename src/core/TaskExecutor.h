// CUB3 Launcher - Task executor with bounded concurrency
#pragma once

#include <QThread>
#include <QSemaphore>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QFuture>
#include <QFutureInterface>
#include <QMutexLocker>
#include <QDebug>
#include <functional>
#include <memory>
#include <algorithm>
#include <atomic>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

namespace cub3::core {

/// A bounded thread pool that runs background tasks with limited concurrency.
///
/// Design goals:
///   - No more than `maxThreads` running simultaneously.
///   - Tasks are FIFO queued.
///   - The pool can be drained and shut down cleanly.
///   - Does NOT spawn threads until the first task is submitted (lazy start).
///   - Can adapt concurrency based on storage type (HDD vs SSD).
///
/// The GUI thread is NEVER used for task execution.
class TaskExecutor : public QObject {
    Q_OBJECT

public:
    /// Returns the global instance. Created on first call.
    /// Must be called from the GUI thread.
    static TaskExecutor& instance();

    /// Configure max concurrency. Call before submitting tasks.
    /// Values: 1-8. Default: min(CPU cores, 4).
    void setMaxConcurrency(int max);

    /// Get current max concurrency.
    [[nodiscard]] int maxConcurrency() const { return m_maxThreads; }

    /// Submit a task that returns a value.
    /// Returns a QFuture so the caller can await or poll the result.
    template<typename F, typename R = std::invoke_result_t<F>>
    QFuture<R> submit(F&& func) {
        auto interface = std::make_shared<QFutureInterface<R>>();
        interface->reportStarted();

        {
            QMutexLocker locker(&m_mutex);
            if (!m_running.load(std::memory_order_relaxed)) {
                qWarning() << "Rejected background task after executor shutdown";
                interface->reportFinished();
                return interface->future();
            }
            m_queue.enqueue([interface, f = std::forward<F>(func)]() mutable {
                try {
                    if constexpr (std::is_void_v<R>) {
                        f();
                    } else {
                        interface->reportResult(f());
                    }
                } catch (const std::exception& e) {
                    qWarning() << "Background task failed:" << e.what();
                } catch (...) {
                    qWarning() << "Background task failed with an unknown exception";
                }
                interface->reportFinished();
            });
        }
        m_cond.wakeOne();
        ensureWorkers();

        return interface->future();
    }

    /// Submit a fire-and-forget task (no return value needed).
    void run(std::function<void()> task);

    /// Number of pending + active tasks.
    [[nodiscard]] int pendingCount() const;

    /// Wait for all tasks to complete. Blocks the calling thread.
    /// Do NOT call from the GUI thread.
    void waitForAll();

    /// Shut down the executor. Stops accepting new tasks.
    /// Waits for all active and queued tasks to finish.
    void shutdown();

    /// Adapt concurrency for storage type.
    /// HDD: 2 threads (avoid thrashing).
    /// SSD: up to maxConcurrency.
    void adaptForHDD(bool isHDD);

private:
    explicit TaskExecutor(QObject* parent = nullptr);
    ~TaskExecutor() override;

    void ensureWorkers();
    void workerLoop();

    struct Worker : public QThread {
        explicit Worker(TaskExecutor* pool) : pool(pool) {}
        void run() override { pool->workerLoop(); }
        TaskExecutor* pool;
    };

    QQueue<std::function<void()>> m_queue;
    mutable QMutex m_mutex;
    QWaitCondition m_cond;
    std::vector<std::unique_ptr<Worker>> m_workers;
    int m_maxThreads;
    int m_activeWorkers{0};
    int m_activeTasks{0};
    std::atomic<bool> m_running{true};
};

} // namespace cub3::core
