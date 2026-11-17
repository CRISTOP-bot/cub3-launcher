// CUB3 Launcher - Performance monitoring
#pragma once

#include <QElapsedTimer>
#include <QString>
#include <chrono>
#include <atomic>

namespace cub3::core {

/// Lightweight performance tracker. Designed for minimal overhead.
/// All measurements are opt-in and can be compiled out in Release.
class Perf {
public:
    Perf() = delete;

    // --- Timing -------------------------------------------------------------

    /// Mark the process start. Call as early as possible in main().
    static void markProcessStart();

    /// Mark the moment the dashboard becomes interactive.
    static void markDashboardReady();

    /// Record a named phase. Lightweight — just stores elapsed nanoseconds.
    static void mark(const char* name);

    /// Get elapsed milliseconds since process start.
    [[nodiscard]] static qint64 elapsedMs();

    /// Get the full startup report as a formatted string (for logs).
    [[nodiscard]] static QString startupReport();

    // --- Runtime metrics ---------------------------------------------------

    /// Current RSS (Resident Set Size) in bytes.
    /// Returns 0 if the platform is unsupported.
    [[nodiscard]] static std::size_t rssBytes();

    /// RSS in megabytes (rounded).
    [[nodiscard]] static std::size_t rssMB();

    /// Number of threads in the current process.
    [[nodiscard]] static std::size_t threadCount();

    /// Approximate CPU usage percentage since the last call.
    /// Returns -1.0 if unsupported.
    [[nodiscard]] static double cpuPercent();

private:
    static QElapsedTimer s_timer;
    static std::chrono::steady_clock::time_point s_processStart;
    static std::atomic<qint64> s_dashboardMs;
    static std::atomic<qint64> s_firstWindowMs;
};

/// RAII scope timer. Records the duration of a scope under a given name.
/// Overhead is negligible (one steady_clock read on construction/destruction).
class PerfScope {
public:
    explicit PerfScope(const char* name);
    ~PerfScope();

    PerfScope(const PerfScope&) = delete;
    PerfScope& operator=(const PerfScope&) = delete;

private:
    const char* m_name;
    std::chrono::steady_clock::time_point m_start;
};

#ifdef CUB3_DEBUG
    #define CUB3_PERF_SCOPE(name) cub3::core::PerfScope _perfScope_(name)
#else
    #define CUB3_PERF_SCOPE(name) (void)0
#endif

} // namespace cub3::core
