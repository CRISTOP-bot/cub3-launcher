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
#include <QSettings>
#include <QDialog>
#include <QStackedWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>

#ifdef CUB3_DEBUG
#include <QElapsedTimer>
#endif

namespace {

void showFirstRunTutorial(QWidget* parent) {
    QSettings settings;
    constexpr int tutorialVersion = 2;
    if (settings.value(QStringLiteral("tutorial/version"), 0).toInt()
        >= tutorialVersion)
        return;

    struct Step {
        QString title;
        QString subtitle;
        QString description;
    };
    const QList<Step> steps{
        {QStringLiteral("¡Bienvenido a CUB3!"),
         QStringLiteral("Tu aventura empieza aquí"),
         QStringLiteral("CUB3 reúne tus versiones de Minecraft en un solo lugar. "
                        "En este recorrido conocerás lo esencial para empezar.")},
        {QStringLiteral("Elige una versión"),
         QStringLiteral("Encuentra la versión que quieres jugar"),
         QStringLiteral("En Inicio, busca por nombre y elige una versión junto al botón "
                        "Jugar. CUB3 descargará los archivos necesarios al iniciar." )},
        {QStringLiteral("Conecta tu cuenta y juega"),
         QStringLiteral("Ya casi estás listo"),
         QStringLiteral("Pulsa Invitado arriba para iniciar sesión con Microsoft o Ely.by. "
                        "Después, elige una versión y pulsa Jugar. Perfiles, descargas y "
                        "ajustes están en la barra lateral.")}
    };

    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Bienvenido a CUB3 Launcher"));
    dialog.setModal(true);
    dialog.setMinimumSize(540, 360);
    dialog.resize(580, 390);
    dialog.setObjectName(QStringLiteral("OnboardingDialog"));
    dialog.setStyleSheet(QStringLiteral(R"(
        QDialog#OnboardingDialog { background: #101320; }
        QLabel#OnboardingStep { color: #79D88B; font-size: 11px; font-weight: 700; letter-spacing: 1px; }
        QLabel#OnboardingTitle { color: #F1F3F8; font-size: 26px; font-weight: 700; }
        QLabel#OnboardingSubtitle { color: #B4BED0; font-size: 15px; }
        QLabel#OnboardingDescription { color: #9AA6BA; font-size: 14px; line-height: 1.5; }
        QProgressBar#OnboardingProgress { border: none; border-radius: 3px; background: #252E41; max-height: 6px; text-align: center; }
        QProgressBar#OnboardingProgress::chunk { border-radius: 3px; background: #35B955; }
        QPushButton { background: #1C2334; border: 1px solid #2A3245; border-radius: 8px; color: #E7EAF0; padding: 10px 18px; font-size: 13px; }
        QPushButton:hover { background: #252E41; border-color: #3B485D; }
        QPushButton#OnboardingPrimary { background: #35B955; border: none; color: white; font-weight: 700; }
        QPushButton#OnboardingPrimary:hover { background: #43CB64; }
    )"));

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(32, 28, 32, 24);
    layout->setSpacing(14);

    auto* stepLabel = new QLabel(&dialog);
    stepLabel->setObjectName(QStringLiteral("OnboardingStep"));
    layout->addWidget(stepLabel);

    auto* progress = new QProgressBar(&dialog);
    progress->setObjectName(QStringLiteral("OnboardingProgress"));
    progress->setRange(0, steps.size());
    progress->setTextVisible(false);
    progress->setFixedHeight(6);
    layout->addWidget(progress);

    auto* title = new QLabel(&dialog);
    title->setObjectName(QStringLiteral("OnboardingTitle"));
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* subtitle = new QLabel(&dialog);
    subtitle->setObjectName(QStringLiteral("OnboardingSubtitle"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    auto* description = new QLabel(&dialog);
    description->setObjectName(QStringLiteral("OnboardingDescription"));
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(description, 1);

    auto* buttons = new QHBoxLayout();
    auto* backButton = new QPushButton(QStringLiteral("Atrás"), &dialog);
    auto* cancelButton = new QPushButton(QStringLiteral("Ahora no"), &dialog);
    auto* nextButton = new QPushButton(QStringLiteral("Siguiente"), &dialog);
    nextButton->setObjectName(QStringLiteral("OnboardingPrimary"));
    buttons->addWidget(backButton);
    buttons->addStretch(1);
    buttons->addWidget(cancelButton);
    buttons->addWidget(nextButton);
    layout->addLayout(buttons);

    int currentStep = 0;
    const auto updateStep = [&] {
        const Step& step = steps.at(currentStep);
        stepLabel->setText(QStringLiteral("PASO %1 DE %2")
                               .arg(currentStep + 1).arg(steps.size()));
        progress->setValue(currentStep + 1);
        title->setText(step.title);
        subtitle->setText(step.subtitle);
        description->setText(step.description);
        backButton->setEnabled(currentStep > 0);
        nextButton->setText(currentStep + 1 == steps.size()
            ? QStringLiteral("Empezar a jugar")
            : QStringLiteral("Siguiente"));
    };
    QObject::connect(backButton, &QPushButton::clicked, &dialog, [&] {
        if (currentStep > 0) {
            --currentStep;
            updateStep();
        }
    });
    QObject::connect(nextButton, &QPushButton::clicked, &dialog, [&] {
        if (currentStep + 1 == steps.size()) {
            dialog.accept();
        } else {
            ++currentStep;
            updateStep();
        }
    });
    QObject::connect(cancelButton, &QPushButton::clicked,
                     &dialog, &QDialog::reject);
    updateStep();

    if (dialog.exec() == QDialog::Accepted)
        settings.setValue(QStringLiteral("tutorial/version"), tutorialVersion);
}

} // namespace

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
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/cub3.svg")));

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
    QTimer::singleShot(0, &app, [&cub3App] {
        showFirstRunTutorial(cub3App.window());
    });

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
