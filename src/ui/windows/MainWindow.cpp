// CUB3 Launcher - Main window implementation
#include "MainWindow.h"

#include "application/Cub3App.h"
#include "core/Performance.h"
#include "core/Logger.h"
#include "core/Paths.h"

#include "styles/Theme.h"
#include "widgets/SidebarButton.h"

#include "pages/DashboardPage.h"
#include "pages/LibraryPage.h"
#include "pages/DownloadsPage.h"
#include "pages/SettingsPage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <QButtonGroup>
#include <QTimer>
#include <QFile>
#include <QMessageBox>

namespace cub3::ui::windows {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("CUB3 Launcher"));
    setMinimumSize(theme::sizes::kWindowMinWidth, theme::sizes::kWindowMinHeight);
    resize(theme::sizes::kWindowWidth, theme::sizes::kWindowHeight);
    setAttribute(Qt::WA_DeleteOnClose, false);

    setupUi();

    // Emit dashboardShown on the first paint
    connect(this, &MainWindow::dashboardShown, this, [this] {
        CUB3_PERF_SCOPE("dashboard_first_show");
    });

    QTimer::singleShot(0, this, &MainWindow::onFirstShow);
}

void MainWindow::setupUi() {
    CUB3_PERF_SCOPE("setup_ui");

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto* mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Sidebar -------------------------------------------------------
    m_sidebar = new QWidget(m_centralWidget);
    m_sidebar->setObjectName(QStringLiteral("Sidebar"));
    m_sidebar->setFixedWidth(theme::sizes::kSidebarWidth);

    auto* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // Brand
    auto* logo = new QLabel(QStringLiteral("CUB3"), m_sidebar);
    logo->setObjectName(QStringLiteral("SidebarLogo"));
    sidebarLayout->addWidget(logo);
    auto* logoCaption = new QLabel(QStringLiteral("GAME LAUNCHER"), m_sidebar);
    logoCaption->setObjectName(QStringLiteral("SidebarSubtitle"));
    sidebarLayout->addWidget(logoCaption);

    // Nav buttons
    m_btnDashboard = new widgets::SidebarButton(
        QStringLiteral(":/icons/dashboard.svg"), QStringLiteral("Inicio"), m_sidebar);
    m_btnLibrary = new widgets::SidebarButton(
        QStringLiteral(":/icons/library.svg"), QStringLiteral("Perfiles"), m_sidebar);
    m_btnDownloads = new widgets::SidebarButton(
        QStringLiteral(":/icons/downloads.svg"), QStringLiteral("Descargas"), m_sidebar);
    m_btnSettings = new widgets::SidebarButton(
        QStringLiteral(":/icons/settings.svg"), QStringLiteral("Ajustes"), m_sidebar);

    // Group nav buttons so only one is checked at a time
    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    navGroup->addButton(m_btnDashboard, 0);
    navGroup->addButton(m_btnLibrary, 1);
    navGroup->addButton(m_btnDownloads, 2);
    navGroup->addButton(m_btnSettings, 3);

    connect(navGroup, &QButtonGroup::idClicked, this, &MainWindow::onNavChanged);

    sidebarLayout->addSpacing(theme::sizes::kSpacingSmall);
    sidebarLayout->addWidget(m_btnDashboard);
    sidebarLayout->addWidget(m_btnLibrary);
    sidebarLayout->addWidget(m_btnDownloads);
    sidebarLayout->addWidget(m_btnSettings);
    sidebarLayout->addStretch();

    // Status indicator at the bottom
    auto* statusLabel = new QLabel(QStringLiteral("●  Aplicación lista"), m_sidebar);
    statusLabel->setObjectName(QStringLiteral("SidebarFooter"));
    sidebarLayout->addWidget(statusLabel);
    sidebarLayout->addSpacing(theme::sizes::kSpacingMedium);

    mainLayout->addWidget(m_sidebar);

    // --- Content area (stacked pages) ---------------------------------
    m_stack = new QStackedWidget(m_centralWidget);
    m_stack->setObjectName(QStringLiteral("ContentArea"));

    // Create dashboard immediately (it's the landing page)
    m_dashboard = new pages::DashboardPage(m_app, m_stack);
    m_stack->addWidget(m_dashboard);

    // Other pages are created lazily
    connect(m_dashboard, &pages::DashboardPage::profilesRequested,
            m_btnLibrary, &QPushButton::click);
    connect(m_dashboard, &pages::DashboardPage::playRequested,
            this, [this](const QString& versionId) {
        if (versionId.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("Selecciona una versión"),
                                     QStringLiteral("No hay una versión disponible para jugar."));
            return;
        }
        QMessageBox::information(
            this, QStringLiteral("Lanzamiento no disponible"),
            QStringLiteral("La versión %1 está seleccionada, pero el motor de lanzamiento aún no está disponible.")
                .arg(versionId));
    });

    mainLayout->addWidget(m_stack, 1);

    // Apply stylesheet
    setStyleSheet(theme::stylesheet());

    // Start on dashboard
    m_btnDashboard->setChecked(true);
    m_stack->setCurrentIndex(0);
}

void MainWindow::onFirstShow() {
    if (!m_firstShow)
        return;
    m_firstShow = false;
    emit dashboardShown();
}

void MainWindow::onNavChanged(int index) {
    switch (index) {
    case 0:
        m_stack->setCurrentWidget(m_dashboard);
        break;
    case 1:
        if (!m_library) {
            m_library = new pages::LibraryPage(m_app, m_stack);
            m_stack->addWidget(m_library);
        }
        m_stack->setCurrentWidget(m_library);
        break;
    case 2:
        if (!m_downloads) {
            m_downloads = new pages::DownloadsPage(m_stack);
            m_stack->addWidget(m_downloads);
        }
        m_stack->setCurrentWidget(m_downloads);
        break;
    case 3:
        if (!m_settings) {
            m_settings = new pages::SettingsPage(m_app, m_stack);
            m_stack->addWidget(m_settings);
        }
        m_stack->setCurrentWidget(m_settings);
        break;
    default:
        return;
    }
}

void MainWindow::setApp(application::Cub3App* app) {
    m_app = app;
    if (m_dashboard)
        m_dashboard->setApp(app);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    CUB3_INFO(lcUI) << "Main window closing";
    event->accept();
}

} // namespace cub3::ui::windows
