// CUB3 Launcher - Entry point
// Lightweight by Design. Performance by Default.

#include "core/AppMeta.h"
#include "core/Paths.h"
#include "core/Performance.h"
#include "core/Logger.h"

#include "application/Cub3App.h"
#include "ui/windows/MainWindow.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>

#ifdef CUB3_DEBUG
#include <QElapsedTimer>
#endif

int main(int argc, char* argv[]) {
    // --- Mark process start as early as possible ---------------------------
    cub3::core::Perf::markProcessStart();

    // --- Qt application attributes -----------------------------------------
    // Set before QApplication is constructed.
    // Qt 6 enables high-DPI scaling and pixmaps automatically.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Limit icon sizes — avoids loading large pixmaps unnecessarily
    QApplication::setAttribute(Qt::AA_Use96Dpi, false);

    QApplication app(argc, argv);
    QApplication::setApplicationName(cub3::core::meta::kName.toString());
    QApplication::setOrganizationName(cub3::core::meta::kOrgName.toString());
    QApplication::setOrganizationDomain(cub3::core::meta::kOrgDomain.toString());
    QApplication::setApplicationVersion(cub3::core::meta::kVersionStr.toString());

    // --- Ensure directories exist -----------------------------------------
    if (!cub3::core::Paths::ensureDirectories()) {
        qCritical() << "Could not create the launcher data directories";
        return 1;
    }

    // --- Initialise logging ------------------------------------------------
    if (!cub3::core::Logger::instance().init(
            cub3::core::Paths::logsDir() + "cub3.log")) {
        qWarning() << "Could not initialize file logging; using stderr";
    }
    CUB3_INFO(lcCub3) << "CUB3 Launcher starting up — version"
                      << cub3::core::meta::kVersionStr;
    CUB3_PERF_SCOPE("startup_overhead");

    // --- Create application ------------------------------------------------
    cub3::application::Cub3App cub3App;
    QObject::connect(&cub3App, &cub3::application::Cub3App::fatal, &app,
                     [&app](const QString& message) {
        QMessageBox::critical(nullptr, QStringLiteral("CUB3 Launcher"), message);
        app.quit();
    });

    // --- Show window (fast path with cached data) --------------------------
    cub3App.showWindow();

    // --- Print startup report in debug builds ------------------------------
    QTimer::singleShot(500, &app, []() {
        auto report = cub3::core::Perf::startupReport();
        CUB3_INFO(lcCub3).noquote() << report;
    });

    // --- Enter event loop --------------------------------------------------
    const int ret = app.exec();

    // --- Cleanup -----------------------------------------------------------
    cub3::core::Logger::instance().shutdown();
    return ret;
}
