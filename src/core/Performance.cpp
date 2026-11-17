// CUB3 Launcher - Performance monitoring
#include "Performance.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <ctime>

#ifdef CUB3_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/resource.h>
    #include <fstream>
#elif defined(CUB3_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib")
#endif

namespace cub3::core {

// --- Static storage -------------------------------------------------------
QElapsedTimer Perf::s_timer;
std::chrono::steady_clock::time_point Perf::s_processStart;
std::atomic<qint64> Perf::s_dashboardMs{0};
std::atomic<qint64> Perf::s_firstWindowMs{0};

// Struct to hold named phase timings
namespace {
struct PhaseRecord {
    const char* name;
    qint64 ms;
};
constexpr int kMaxPhases = 32;
PhaseRecord g_phases[kMaxPhases];
std::atomic<int> g_phaseCount{0};
}

// --- Implementation -------------------------------------------------------

void Perf::markProcessStart() {
    s_processStart = std::chrono::steady_clock::now();
    s_timer.start();
}

void Perf::markDashboardReady() {
    s_dashboardMs.store(s_timer.elapsed(), std::memory_order_relaxed);
}

void Perf::mark(const char* name) {
    int idx = g_phaseCount.fetch_add(1, std::memory_order_relaxed);
    if (idx < kMaxPhases) {
        g_phases[idx] = {name, s_timer.elapsed()};
    }
}

qint64 Perf::elapsedMs() {
    return s_timer.elapsed();
}

QString Perf::startupReport() {
    QString report;
    report += QStringLiteral("=== CUB3 Startup Report ===\n");

    const auto total = elapsedMs();
    report += QStringLiteral("Total elapsed: %1 ms\n").arg(total);

    if (auto dash = s_dashboardMs.load(std::memory_order_relaxed); dash > 0)
        report += QStringLiteral("Dashboard ready: %1 ms\n").arg(dash);

    const int count = g_phaseCount.load(std::memory_order_relaxed);
    qint64 prev = 0;
    for (int i = 0; i < count && i < kMaxPhases; ++i) {
        const auto& p = g_phases[i];
        report += QStringLiteral("  %1: %2 ms (+%3 ms)\n")
                      .arg(QString::fromLatin1(p.name))
                      .arg(p.ms)
                      .arg(p.ms - prev);
        prev = p.ms;
    }

    report += QStringLiteral("RSS: %1 MB\n").arg(rssMB());
    report += QStringLiteral("Threads: %1\n").arg(threadCount());
    return report;
}

std::size_t Perf::rssBytes() {
#ifdef CUB3_PLATFORM_LINUX
    // Parse /proc/self/status for VmRSS — cheaper than getrusage on Linux
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.compare(0, 6, "VmRSS:") == 0) {
            // Format: "VmRSS:    12345 kB"
            std::size_t kB = 0;
            std::sscanf(line.c_str(), "VmRSS: %zu kB", &kB);
            return kB * 1024;
        }
    }
    return 0;
#elif defined(CUB3_PLATFORM_WINDOWS)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.WorkingSetSize;
    return 0;
#else
    return 0;
#endif
}

std::size_t Perf::rssMB() {
    return rssBytes() / (1024 * 1024);
}

std::size_t Perf::threadCount() {
#ifdef CUB3_PLATFORM_LINUX
    // Count entries in /proc/self/task — one per thread
    QDir taskDir(QStringLiteral("/proc/self/task"));
    return static_cast<std::size_t>(
        taskDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size());
#elif defined(CUB3_PLATFORM_WINDOWS)
    // Count threads via Toolhelp32
    // Simplified: use GetCurrentProcessId + snapshot
    return 1; // Fallback — accurate counting requires more API calls
#else
    return 1;
#endif
}

double Perf::cpuPercent() {
#ifdef CUB3_PLATFORM_LINUX
    static std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
    static clock_t lastClock = clock();

    auto now = std::chrono::steady_clock::now();
    clock_t currentClock = clock();

    double elapsedSec = std::chrono::duration<double>(now - lastTime).count();
    double cpuSec = double(currentClock - lastClock) / CLOCKS_PER_SEC;

    lastTime = now;
    lastClock = currentClock;

    if (elapsedSec < 0.001)
        return -1.0;

    return (cpuSec / elapsedSec) * 100.0;
#else
    return -1.0;
#endif
}

// --- PerfScope ------------------------------------------------------------

PerfScope::PerfScope(const char* name)
    : m_name(name)
    , m_start(std::chrono::steady_clock::now())
{
}

PerfScope::~PerfScope() {
#ifdef CUB3_DEBUG
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
    qDebug("CUB3 Perf: %s took %lld ms", m_name, static_cast<long long>(ms));
#endif
}

} // namespace cub3::core
