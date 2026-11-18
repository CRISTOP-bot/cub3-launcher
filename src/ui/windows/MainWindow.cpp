// CUB3 Launcher - Main window implementation
#include "MainWindow.h"

#include "application/Cub3App.h"
#include "application/LaunchEngine.h"
#include "auth/AuthManager.h"
#include "core/AppMeta.h"
#include "core/Performance.h"
#include "core/Logger.h"
#include "core/Paths.h"

#include "styles/Theme.h"
#include "widgets/SidebarButton.h"

#include "pages/DashboardPage.h"
#include "pages/LibraryPage.h"
#include "pages/DownloadsPage.h"
#include "pages/ModsPage.h"
#include "pages/SettingsPage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <QButtonGroup>
#include <QTimer>
#include <QFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QCursor>
#include <QRegularExpression>
#include <QSettings>
#include <QSizeGrip>
#include <QToolButton>
#include <QWindow>

namespace cub3::ui::windows {
namespace {

class WindowTitleBar final : public QWidget {
public:
    explicit WindowTitleBar(QMainWindow* window, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_window(window)
    {
        setObjectName(QStringLiteral("WindowTitleBar"));
        setFixedHeight(34);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(14, 0, 8, 0);
        layout->setSpacing(4);

        auto* title = new QLabel(QStringLiteral("CUB3 Launcher"), this);
        title->setObjectName(QStringLiteral("WindowTitleText"));
        auto* icon = new QLabel(this);
        icon->setPixmap(QIcon(QStringLiteral(":/icons/cub3.svg")).pixmap(18, 18));
        icon->setFixedSize(18, 18);
        layout->addWidget(icon);
        layout->addWidget(title);
        layout->addStretch(1);

        auto* minimizeButton = makeControl(QStringLiteral("−"),
                                            QStringLiteral("WindowControl"));
        connect(minimizeButton, &QToolButton::clicked,
                m_window, &QWidget::showMinimized);
        layout->addWidget(minimizeButton);

        m_maximizeButton = makeControl(QStringLiteral("□"),
                                       QStringLiteral("WindowControl"));
        connect(m_maximizeButton, &QToolButton::clicked, this, [this] {
            toggleMaximized();
        });
        layout->addWidget(m_maximizeButton);

        auto* closeButton = makeControl(QStringLiteral("×"),
                                        QStringLiteral("WindowCloseButton"));
        connect(closeButton, &QToolButton::clicked, m_window, &QWidget::close);
        layout->addWidget(closeButton);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() != Qt::LeftButton || m_window->isMaximized()) {
            QWidget::mousePressEvent(event);
            return;
        }

        m_dragging = !(m_window->windowHandle() &&
                       m_window->windowHandle()->startSystemMove());
        m_dragOffset = event->globalPosition().toPoint()
            - m_window->frameGeometry().topLeft();
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            m_window->move(event->globalPosition().toPoint() - m_dragOffset);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        m_dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            toggleMaximized();
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    static QToolButton* makeControl(const QString& text, const QString& objectName) {
        auto* button = new QToolButton();
        button->setText(text);
        button->setObjectName(objectName);
        button->setFixedSize(38, 30);
        button->setCursor(Qt::PointingHandCursor);
        return button;
    }

    void toggleMaximized() {
        if (m_window->isMaximized()) {
            m_window->showNormal();
            m_maximizeButton->setText(QStringLiteral("□"));
        } else {
            m_window->showMaximized();
            m_maximizeButton->setText(QStringLiteral("❐"));
        }
    }

    QMainWindow* m_window{nullptr};
    QToolButton* m_maximizeButton{nullptr};
    QPoint m_dragOffset;
    bool m_dragging{false};
};

class BrandMark final : public QWidget {
public:
    explicit BrandMark(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(180, 58);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        QIcon(QStringLiteral(":/logos/cub3-primary.svg")).paint(&painter, rect());
    }
};

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
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

    auto* windowLayout = new QVBoxLayout(m_centralWidget);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);

    windowLayout->addWidget(new WindowTitleBar(this, m_centralWidget));
    auto* contentWidget = new QWidget(m_centralWidget);
    auto* mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    windowLayout->addWidget(contentWidget, 1);

    auto* gripLayout = new QHBoxLayout();
    gripLayout->setContentsMargins(0, 0, 0, 0);
    gripLayout->addStretch(1);
    gripLayout->addWidget(new QSizeGrip(m_centralWidget), 0, Qt::AlignBottom | Qt::AlignRight);
    windowLayout->addLayout(gripLayout);

    // --- Sidebar -------------------------------------------------------
    m_sidebar = new QWidget(contentWidget);
    m_sidebar->setObjectName(QStringLiteral("Sidebar"));
    m_sidebar->setFixedWidth(theme::sizes::kSidebarWidth);

    auto* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // Brand
    auto* brandRow = new QWidget(m_sidebar);
    auto* brandLayout = new QVBoxLayout(brandRow);
    brandLayout->setContentsMargins(18, 24, 18, 18);
    brandLayout->setSpacing(0);
    brandLayout->addWidget(new BrandMark(brandRow), 0, Qt::AlignHCenter);
    sidebarLayout->addWidget(brandRow);

    // Nav buttons
    m_btnDashboard = new widgets::SidebarButton(
        QStringLiteral(":/icons/dashboard.svg"), QStringLiteral("Inicio"), m_sidebar);
    m_btnLibrary = new widgets::SidebarButton(
        QStringLiteral(":/icons/library.svg"), QStringLiteral("Perfiles"), m_sidebar);
    m_btnDownloads = new widgets::SidebarButton(
        QStringLiteral(":/icons/downloads.svg"), QStringLiteral("Descargas"), m_sidebar);
    m_btnMods = new widgets::SidebarButton(
        QStringLiteral(":/icons/mods.svg"), QStringLiteral("Mods"), m_sidebar);
    m_btnSettings = new widgets::SidebarButton(
        QStringLiteral(":/icons/settings.svg"), QStringLiteral("Ajustes"), m_sidebar);

    // Group nav buttons so only one is checked at a time
    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    navGroup->addButton(m_btnDashboard, 0);
    navGroup->addButton(m_btnLibrary, 1);
    navGroup->addButton(m_btnDownloads, 2);
    navGroup->addButton(m_btnMods, 3);
    navGroup->addButton(m_btnSettings, 4);

    connect(navGroup, &QButtonGroup::idClicked, this, &MainWindow::onNavChanged);

    sidebarLayout->addSpacing(theme::sizes::kSpacingSmall);
    sidebarLayout->addWidget(m_btnDashboard);
    sidebarLayout->addWidget(m_btnLibrary);
    sidebarLayout->addWidget(m_btnDownloads);
    sidebarLayout->addWidget(m_btnMods);
    sidebarLayout->addWidget(m_btnSettings);
    sidebarLayout->addStretch();

    // Status indicator at the bottom
    auto* statusLabel = new QLabel(QStringLiteral("●  Aplicación lista"), m_sidebar);
    statusLabel->setObjectName(QStringLiteral("SidebarFooter"));
    sidebarLayout->addWidget(statusLabel);
    sidebarLayout->addSpacing(theme::sizes::kSpacingMedium);

    mainLayout->addWidget(m_sidebar);

    // --- Content area (stacked pages) ---------------------------------
    m_stack = new QStackedWidget(contentWidget);
    m_stack->setObjectName(QStringLiteral("ContentArea"));

    // Create dashboard immediately (it's the landing page)
    m_dashboard = new pages::DashboardPage(m_app, m_stack);
    m_stack->addWidget(m_dashboard);

    // Other pages are created lazily
    connect(m_dashboard, &pages::DashboardPage::profilesRequested,
            m_btnLibrary, &QPushButton::click);
    connect(m_dashboard, &pages::DashboardPage::accountRequested,
            this, &MainWindow::onAccountRequested);
    connect(m_dashboard, &pages::DashboardPage::playRequested,
            this, &MainWindow::onPlayRequested);

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
        if (!m_mods) {
            m_mods = new pages::ModsPage(m_app, m_stack);
            m_stack->addWidget(m_mods);
        }
        m_stack->setCurrentWidget(m_mods);
        break;
    case 4:
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

void MainWindow::onAccountRequested() {
    if (!m_app) {
        QMessageBox::warning(this, QStringLiteral("Cuenta"),
                             QStringLiteral("El servicio de cuentas no está disponible."));
        return;
    }

    auto& manager = m_app->authManager();
    if (manager.isRestoring()) {
        QMessageBox::information(
            this, QStringLiteral("Cuenta"),
            QStringLiteral("Estamos restaurando tu cuenta. Inténtalo de nuevo en un momento."));
        return;
    }
    m_auth = &manager;

    QMenu menu(this);
    QAction* microsoftAction =
        menu.addAction(QStringLiteral("Iniciar sesión con Microsoft"));
    QAction* elybyAction =
        menu.addAction(QStringLiteral("Iniciar sesión con Ely.by"));
    QAction* logoutAction = nullptr;
    if (m_auth->isLoggedIn()) {
        menu.addSeparator();
        logoutAction = menu.addAction(QStringLiteral("Cerrar sesión"));
    }

    QAction* chosen = menu.exec(QCursor::pos());
    if (chosen == microsoftAction) {
        QSettings settings;
        QString clientId = qEnvironmentVariable("CUB3_MICROSOFT_CLIENT_ID").trimmed();
        if (clientId.isEmpty())
            clientId = settings.value(QStringLiteral("auth/microsoftClientId")).toString().trimmed();
        if (clientId.isEmpty()
            || clientId == cub3::core::meta::kMicrosoftClientId.toString()) {
            bool accepted = false;
            clientId = QInputDialog::getText(
                this, QStringLiteral("Configurar Microsoft"),
                QStringLiteral("ID de cliente de tu aplicación pública de Microsoft:"),
                QLineEdit::Normal, QString(), &accepted).trimmed();
            if (!accepted)
                return;
            static const QRegularExpression clientIdPattern(
                QStringLiteral("^[0-9A-Fa-f]{8}(-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}$"));
            if (!clientIdPattern.match(clientId).hasMatch()) {
                QMessageBox::warning(
                    this, QStringLiteral("ID no válido"),
                    QStringLiteral("El ID debe tener el formato GUID de una aplicación de Microsoft."));
                return;
            }
            settings.setValue(QStringLiteral("auth/microsoftClientId"), clientId);
            qputenv("CUB3_MICROSOFT_CLIENT_ID", clientId.toUtf8());
        }
        m_auth->loginMicrosoft({}, [this](const QString& error) {
            QMessageBox::warning(this, QStringLiteral("Error de inicio de sesión"),
                                 error);
        });
    } else if (chosen == elybyAction) {
        bool accepted = false;
        const QString username = QInputDialog::getText(
            this, QStringLiteral("Iniciar sesión con Ely.by"),
            QStringLiteral("Correo o nombre de usuario:"), QLineEdit::Normal,
            QString(), &accepted).trimmed();
        if (!accepted || username.isEmpty())
            return;

        const QString password = QInputDialog::getText(
            this, QStringLiteral("Iniciar sesión con Ely.by"),
            QStringLiteral("Contraseña:"), QLineEdit::Password,
            QString(), &accepted);
        if (!accepted || password.isEmpty())
            return;

        m_auth->loginElyby(username, password, {},
            [this](const QString& error) {
                QMessageBox::warning(this, QStringLiteral("Error de inicio de sesión"),
                                     error);
            });
    } else if (chosen == logoutAction) {
        m_auth->logout();
    }
}

void MainWindow::onPlayRequested(const QString& versionId) {
    if (!m_app) {
        QMessageBox::warning(this, QStringLiteral("Iniciar juego"),
                             QStringLiteral("El servicio del juego no está disponible."));
        return;
    }

    auto& authManager = m_app->authManager();
    if (authManager.isRestoring()) {
        QMessageBox::information(
            this, QStringLiteral("Iniciar juego"),
            QStringLiteral("Estamos restaurando tu cuenta. Inténtalo de nuevo en un momento."));
        return;
    }
    if (!authManager.isLoggedIn()) {
        QMessageBox::information(
            this, QStringLiteral("Inicia sesión"),
            QStringLiteral("Inicia sesión con tu cuenta Microsoft o Ely.by."));
        return;
    }

    authManager.refreshIfNeeded([this, versionId, &authManager](bool valid) {
        if (!valid) {
            QMessageBox::warning(
                this, QStringLiteral("Sesión expirada"),
                QStringLiteral("No se pudo renovar la sesión. Inicia sesión de nuevo."));
            return;
        }

        auto& launcher = m_app->launchEngine();
        if (m_launcher != &launcher) {
            m_launcher = &launcher;
            connect(m_launcher, &application::LaunchEngine::statusChanged,
                    m_dashboard, &pages::DashboardPage::setStatus);
            connect(m_launcher, &application::LaunchEngine::launchFailed,
                    this, [this](const QString& error) {
                QMessageBox::warning(this, QStringLiteral("No se pudo iniciar Minecraft"),
                                     error);
            });
        }
        m_launcher->launch(versionId, authManager.currentAccount());
    });
}

void MainWindow::setApp(application::Cub3App* app) {
    m_app = app;
    if (m_dashboard)
        m_dashboard->setApp(app);
    if (!m_app)
        return;

    m_auth = &m_app->authManager();
    connect(m_auth, &auth::AuthManager::accountChanged,
            m_dashboard, [this](const auth::Account& account) {
        m_dashboard->setPlayerName(account.username);
    });
    connect(m_auth, &auth::AuthManager::microsoftDeviceCode,
            this, [this](const QString& code, const QString& uri) {
        auto* dialog = new QMessageBox(
            QMessageBox::Information,
            QStringLiteral("Autoriza Microsoft"),
            QStringLiteral("Código: %1\n\nSe abrió %2 en el navegador para completar el acceso.")
                .arg(code, uri),
            QMessageBox::Ok, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
    connect(m_auth, &auth::AuthManager::credentialStorageError,
            this, [this](const QString& error) {
        QMessageBox::warning(
            this, QStringLiteral("No se pudo acceder a la cuenta guardada"),
            QStringLiteral("No se pudo acceder al llavero seguro del sistema. "
                           "Es posible que debas iniciar sesión de nuevo; la contraseña nunca se guarda.\n\n%1")
                .arg(error));
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    CUB3_INFO(lcUI) << "Main window closing";
    event->accept();
}

} // namespace cub3::ui::windows
