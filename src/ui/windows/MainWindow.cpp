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
#include <QLineEdit>
#include <QMenu>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QRegularExpressionValidator>
#include <QStringList>
#include <QCursor>
#include <QRegularExpression>
#include <QSettings>
#include <QSizeGrip>
#include <QToolButton>
#include <QWindow>
#include <QAbstractButton>
#include <QKeySequence>
#include <QPointer>
#include <QProgressBar>
#include <QShortcut>
#include <functional>

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

class LoginDialog final : public QDialog {
public:
    explicit LoginDialog(bool microsoftClientRequired, QWidget* parent = nullptr)
        : QDialog(parent)
        , m_microsoftClientRequired(microsoftClientRequired)
    {
        setWindowTitle(QStringLiteral("Conecta tu cuenta"));
        setMinimumSize(460, 400);
        resize(560, 480);
        setModal(true);
        setAccessibleName(QStringLiteral("Inicio de sesión de CUB3"));

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 22, 24, 20);
        layout->setSpacing(14);

        auto* heading = new QLabel(QStringLiteral("Entra a tu mundo"), this);
        heading->setObjectName(QStringLiteral("Title"));
        layout->addWidget(heading);
        auto* intro = new QLabel(
            QStringLiteral("Elige el proveedor. CUB3 no guarda tu contraseña; Microsoft abre su autorización en el navegador y Ely.by se conecta por HTTPS."), this);
        intro->setObjectName(QStringLiteral("Secondary"));
        intro->setWordWrap(true);
        layout->addWidget(intro);

        m_tabs = new QTabWidget(this);
        m_tabs->setAccessibleName(QStringLiteral("Proveedor de cuenta"));
        auto* microsoftPage = new QWidget(m_tabs);
        auto* microsoftLayout = new QVBoxLayout(microsoftPage);
        microsoftLayout->setContentsMargins(14, 18, 14, 14);
        auto* microsoftInfo = new QLabel(
            QStringLiteral("CUB3 abre la autorización oficial de Microsoft en el navegador. "
                           "No escribas tu contraseña de Microsoft aquí. En esta compilación necesitas un ID de cliente de una aplicación pública; no es tu correo ni tu contraseña."), microsoftPage);
        microsoftInfo->setWordWrap(true);
        microsoftLayout->addWidget(microsoftInfo);
        auto* clientLabel = new QLabel(QStringLiteral("ID de cliente de tu aplicación pública de Microsoft"), microsoftPage);
        clientLabel->setObjectName(QStringLiteral("Secondary"));
        clientLabel->setVisible(m_microsoftClientRequired);
        microsoftLayout->addWidget(clientLabel);
        m_clientId = new QLineEdit(microsoftPage);
        m_clientId->setPlaceholderText(QStringLiteral("GUID de la aplicación pública"));
        m_clientId->setClearButtonEnabled(true);
        m_clientId->setAccessibleName(QStringLiteral("ID GUID de la aplicación pública de Microsoft"));
        m_clientId->setToolTip(QStringLiteral("No es tu correo ni tu contraseña. Es el identificador público de la aplicación."));
        m_clientId->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("[0-9A-Fa-f]{0,8}(-[0-9A-Fa-f]{0,4}){0,3}(-[0-9A-Fa-f]{0,12})?")),
            m_clientId));
        m_clientId->setVisible(m_microsoftClientRequired);
        microsoftLayout->addWidget(m_clientId);
        microsoftLayout->addStretch(1);
        m_tabs->addTab(microsoftPage, QStringLiteral("Microsoft"));

        auto* elybyPage = new QWidget(m_tabs);
        auto* elybyLayout = new QFormLayout(elybyPage);
        elybyLayout->setContentsMargins(14, 18, 14, 14);
        elybyLayout->setVerticalSpacing(12);
        m_username = new QLineEdit(elybyPage);
        m_username->setPlaceholderText(QStringLiteral("Correo o nombre de usuario"));
        m_username->setClearButtonEnabled(true);
        m_username->setAccessibleName(QStringLiteral("Correo o nombre de usuario de Ely.by"));
        elybyLayout->addRow(QStringLiteral("Usuario"), m_username);
        m_password = new QLineEdit(elybyPage);
        m_password->setPlaceholderText(QStringLiteral("Contraseña de Ely.by"));
        m_password->setEchoMode(QLineEdit::Password);
        m_password->setAccessibleName(QStringLiteral("Contraseña de Ely.by"));
        elybyLayout->addRow(QStringLiteral("Contraseña"), m_password);
        auto* showPassword = new QCheckBox(QStringLiteral("Mostrar contraseña mientras escribo"), elybyPage);
        showPassword->setAccessibleName(QStringLiteral("Mostrar u ocultar la contraseña de Ely.by"));
        connect(showPassword, &QCheckBox::toggled, this, [this](bool visible) {
            m_password->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        });
        elybyLayout->addRow(QString(), showPassword);
        auto* privacyNote = new QLabel(QStringLiteral("La contraseña solo se usa para autenticarte y no se guarda."), elybyPage);
        privacyNote->setObjectName(QStringLiteral("Secondary"));
        privacyNote->setWordWrap(true);
        elybyLayout->addRow(QString(), privacyNote);
        m_tabs->addTab(elybyPage, QStringLiteral("Ely.by"));
        layout->addWidget(m_tabs);

        auto* buttons = new QDialogButtonBox(this);
        m_continue = buttons->addButton(QStringLiteral("Continuar"), QDialogButtonBox::AcceptRole);
        m_cancel = buttons->addButton(QStringLiteral("Cancelar"), QDialogButtonBox::RejectRole);
        m_cancel->setAccessibleName(QStringLiteral("Cancelar el inicio de sesión"));
        m_cancel->setToolTip(QStringLiteral("Cierra el diálogo y descarta el resultado de este intento"));
        m_continue->setDefault(true);
        m_continue->setObjectName(QStringLiteral("Primary"));
        m_error = new QLabel(this);
        m_error->setObjectName(QStringLiteral("LoginError"));
        m_error->setWordWrap(true);
        m_error->setTextFormat(Qt::PlainText);
        m_error->setAccessibleName(QStringLiteral("Error de inicio de sesión"));
        m_error->hide();
        m_busyStatus = new QLabel(this);
        m_busyStatus->setObjectName(QStringLiteral("Secondary"));
        m_busyStatus->setAccessibleName(QStringLiteral("Estado de inicio de sesión"));
        m_busyStatus->hide();
        m_busyIndicator = new QProgressBar(this);
        m_busyIndicator->setObjectName(QStringLiteral("LoginProgress"));
        m_busyIndicator->setRange(0, 0);
        m_busyIndicator->setTextVisible(false);
        m_busyIndicator->setAccessibleName(QStringLiteral("Inicio de sesión en curso"));
        m_busyIndicator->hide();
        layout->insertWidget(layout->count() - 1, m_error);
        layout->insertWidget(layout->count() - 1, m_busyStatus);
        layout->insertWidget(layout->count() - 1, m_busyIndicator);
        connect(m_continue, &QPushButton::clicked, this, [this] {
            QString error;
            if (provider() == 0) {
                static const QRegularExpression clientIdPattern(
                    QStringLiteral("^[0-9A-Fa-f]{8}(-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}$"));
                if (m_microsoftClientRequired && !clientIdPattern.match(clientId()).hasMatch())
                    error = QStringLiteral("Escribe un ID de cliente GUID válido de Microsoft. No es tu correo ni tu contraseña.");
            } else if (username().isEmpty() || password().isEmpty()) {
                error = QStringLiteral("Completa el correo o usuario y la contraseña de Ely.by.");
            }
            m_error->setText(error);
            m_error->setVisible(!error.isEmpty());
            if (!error.isEmpty()) {
                (provider() == 0 ? static_cast<QWidget*>(m_clientId)
                                 : static_cast<QWidget*>(m_username))->setFocus();
                return;
            }
            if (m_submitHandler)
                m_submitHandler(this);
            else
                accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
        connect(m_tabs, &QTabWidget::currentChanged, this, [this] {
            m_error->clear();
            m_error->hide();
            m_continue->setText(m_tabs->currentIndex() == 0
                ? QStringLiteral("Continuar con Microsoft")
                : QStringLiteral("Iniciar sesión con Ely.by"));
            if (m_tabs->currentIndex() == 1)
                m_username->setFocus();
        });
        m_continue->setText(QStringLiteral("Continuar con Microsoft"));
        QTimer::singleShot(0, this, [this] {
            (m_microsoftClientRequired ? static_cast<QWidget*>(m_clientId)
                                      : static_cast<QWidget*>(m_tabs))->setFocus();
        });
    }

    void reject() override {
        clearPassword();
        if (m_busy && m_cancelHandler)
            m_cancelHandler();
        QDialog::reject();
    }

    int provider() const { return m_tabs->currentIndex(); }
    QString clientId() const { return m_clientId->text().trimmed(); }
    QString username() const { return m_username->text().trimmed(); }
    QString password() const { return m_password->text(); }
    void clearPassword() { m_password->clear(); }

    void setBusy(bool busy, const QString& message = {}) {
        m_busy = busy;
        m_tabs->setEnabled(!busy);
        m_continue->setEnabled(!busy);
        m_continue->setText(busy ? QStringLiteral("Conectando…")
                                 : (provider() == 0
                                     ? QStringLiteral("Continuar con Microsoft")
                                     : QStringLiteral("Iniciar sesión con Ely.by")));
        m_cancel->setEnabled(true);
        m_busyStatus->setText(busy ? (message.isEmpty()
            ? QStringLiteral("Conectando de forma segura…") : message) : QString());
        m_busyStatus->setVisible(busy);
        m_busyIndicator->setVisible(busy);
        if (!busy) {
            m_error->clear();
            m_error->hide();
        }
    }

    void showLoginError(const QString& message) {
        setBusy(false);
        m_error->setText(message);
        m_error->show();
        if (provider() == 1)
            m_password->setFocus();
    }

    void setSubmitHandler(std::function<void(LoginDialog*)> handler) {
        m_submitHandler = std::move(handler);
    }

    void setCancelHandler(std::function<void()> handler) {
        m_cancelHandler = std::move(handler);
    }

private:
    QTabWidget* m_tabs{nullptr};
    QLineEdit* m_clientId{nullptr};
    QLineEdit* m_username{nullptr};
    QLineEdit* m_password{nullptr};
    QPushButton* m_continue{nullptr};
    QLabel* m_error{nullptr};
    QLabel* m_busyStatus{nullptr};
    QProgressBar* m_busyIndicator{nullptr};
    QPushButton* m_cancel{nullptr};
    std::function<void(LoginDialog*)> m_submitHandler;
    std::function<void()> m_cancelHandler;
    bool m_microsoftClientRequired{true};
    bool m_busy{false};
};

void showQuickTutorial(QWidget* parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Tutorial rápido de CUB3"));
    dialog.setMinimumSize(440, 360);
    dialog.resize(560, 420);
    dialog.setAccessibleName(QStringLiteral("Tutorial rápido de CUB3"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(14);
    auto* title = new QLabel(QStringLiteral("Tu primera partida"), &dialog);
    title->setObjectName(QStringLiteral("Title"));
    layout->addWidget(title);

    auto* pages = new QStackedWidget(&dialog);
    const QStringList headings{
        QStringLiteral("1. Conecta tu cuenta"),
        QStringLiteral("2. Elige una versión"),
        QStringLiteral("3. Prepara Java"),
        QStringLiteral("4. Juega y administra tus perfiles")
    };
    const QStringList descriptions{
        QStringLiteral("En Inicio, abre el botón de cuenta y conecta Microsoft o Ely.by. Microsoft continúa en tu navegador: nunca escribas aquí esa contraseña. Si se solicita, el ID es el GUID público de la aplicación; no es una clave secreta. Ely.by usa el formulario de CUB3 y la contraseña no se guarda."),
        QStringLiteral("En Inicio, escribe en el selector para filtrar las versiones y elige una de Minecraft. Espera a que termine la preparación antes de iniciar el juego; puedes seguir el avance en Descargas."),
        QStringLiteral("En Ajustes, selecciona Java y configura memoria, resolución y carpeta de juego. Usa una ruta de Java ejecutable y deja memoria suficiente para el sistema; la versión requerida depende de Minecraft."),
        QStringLiteral("Pulsa Jugar. Perfiles reúne instalaciones locales; Descargas muestra el progreso. Si algo falla, revisa el mensaje de estado y confirma la cuenta y Java. Conserva copias de tus mundos, sobre todo en builds de desarrollo.")
    };
    for (int i = 0; i < headings.size(); ++i) {
        auto* page = new QWidget(pages);
        auto* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(8, 12, 8, 18);
        auto* stepTitle = new QLabel(headings.at(i), page);
        stepTitle->setObjectName(QStringLiteral("Heading"));
        auto* description = new QLabel(descriptions.at(i), page);
        description->setObjectName(QStringLiteral("Secondary"));
        description->setWordWrap(true);
        description->setMinimumHeight(120);
        description->setAccessibleName(QStringLiteral("Descripción del paso %1").arg(i + 1));
        pageLayout->addWidget(stepTitle);
        pageLayout->addWidget(description);
        pageLayout->addStretch(1);
        pages->addWidget(page);
    }
    layout->addWidget(pages, 1);
    auto* progressBar = new QProgressBar(&dialog);
    progressBar->setObjectName(QStringLiteral("TutorialProgress"));
    progressBar->setRange(0, pages->count());
    progressBar->setTextVisible(false);
    progressBar->setAccessibleName(QStringLiteral("Avance del tutorial"));
    layout->addWidget(progressBar);
    auto* footer = new QHBoxLayout();
    auto* progress = new QLabel(&dialog);
    progress->setObjectName(QStringLiteral("Secondary"));
    progress->setAccessibleName(QStringLiteral("Progreso del tutorial"));
    auto* back = new QPushButton(QStringLiteral("Atrás"), &dialog);
    auto* next = new QPushButton(QStringLiteral("Siguiente"), &dialog);
    next->setDefault(true);
    auto* finish = new QPushButton(QStringLiteral("Terminar"), &dialog);
    finish->setObjectName(QStringLiteral("Primary"));
    finish->hide();
    footer->addWidget(progress);
    footer->addStretch(1);
    footer->addWidget(back);
    footer->addWidget(next);
    footer->addWidget(finish);
    layout->addLayout(footer);
    const auto update = [=](int index) {
        pages->setCurrentIndex(index);
        progress->setText(QStringLiteral("Paso %1 de %2").arg(index + 1).arg(pages->count()));
        progressBar->setValue(index + 1);
        back->setEnabled(index > 0);
        next->setVisible(index + 1 < pages->count());
        finish->setVisible(index + 1 == pages->count());
    };
    QObject::connect(back, &QPushButton::clicked, &dialog, [=] { update(qMax(0, pages->currentIndex() - 1)); });
    QObject::connect(next, &QPushButton::clicked, &dialog, [=] { update(qMin(pages->count() - 1, pages->currentIndex() + 1)); });
    QObject::connect(finish, &QPushButton::clicked, &dialog, &QDialog::accept);
    auto* nextShortcut = new QShortcut(QKeySequence(Qt::Key_Right), &dialog);
    nextShortcut->setContext(Qt::WindowShortcut);
    QObject::connect(nextShortcut, &QShortcut::activated, &dialog, [=] {
        if (pages->currentIndex() + 1 < pages->count())
            update(pages->currentIndex() + 1);
    });
    auto* backShortcut = new QShortcut(QKeySequence(Qt::Key_Left), &dialog);
    backShortcut->setContext(Qt::WindowShortcut);
    QObject::connect(backShortcut, &QShortcut::activated, &dialog, [=] {
        if (pages->currentIndex() > 0)
            update(pages->currentIndex() - 1);
    });
    update(0);
    dialog.exec();
}

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
    m_btnDashboard->setAccessibleName(QStringLiteral("Ir a Inicio"));
    m_btnDashboard->setToolTip(QStringLiteral("Inicio · Ctrl+1"));
    m_btnLibrary->setAccessibleName(QStringLiteral("Ir a Perfiles"));
    m_btnLibrary->setToolTip(QStringLiteral("Perfiles · Ctrl+2"));
    m_btnDownloads->setAccessibleName(QStringLiteral("Ir a Descargas"));
    m_btnDownloads->setToolTip(QStringLiteral("Descargas · Ctrl+3"));
    m_btnMods->setAccessibleName(QStringLiteral("Ir a Mods"));
    m_btnMods->setToolTip(QStringLiteral("Mods · Ctrl+4"));
    m_btnSettings->setAccessibleName(QStringLiteral("Ir a Ajustes"));
    m_btnSettings->setToolTip(QStringLiteral("Ajustes · Ctrl+5"));
    m_stack->setAccessibleName(QStringLiteral("Contenido de CUB3 Launcher"));

    // Group nav buttons so only one is checked at a time
    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    navGroup->addButton(m_btnDashboard, 0);
    navGroup->addButton(m_btnLibrary, 1);
    navGroup->addButton(m_btnDownloads, 2);
    navGroup->addButton(m_btnMods, 3);
    navGroup->addButton(m_btnSettings, 4);

    connect(navGroup, &QButtonGroup::idClicked, this, &MainWindow::onNavChanged);
    const QList<QAbstractButton*> navButtons{m_btnDashboard, m_btnLibrary,
        m_btnDownloads, m_btnMods, m_btnSettings};
    for (int i = 0; i < navButtons.size(); ++i) {
        auto* shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(i + 1)), this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, navButtons.at(i), &QAbstractButton::click);
    }

    sidebarLayout->addSpacing(theme::sizes::kSpacingSmall);
    sidebarLayout->addWidget(m_btnDashboard);
    sidebarLayout->addWidget(m_btnLibrary);
    sidebarLayout->addWidget(m_btnDownloads);
    sidebarLayout->addWidget(m_btnMods);
    sidebarLayout->addWidget(m_btnSettings);
    sidebarLayout->addStretch();

    auto* tutorialButton = new QPushButton(QStringLiteral("?   Guía rápida"), m_sidebar);
    tutorialButton->setObjectName(QStringLiteral("SecondaryButton"));
    tutorialButton->setCursor(Qt::PointingHandCursor);
    tutorialButton->setAccessibleName(QStringLiteral("Abrir guía rápida de CUB3"));
    tutorialButton->setToolTip(QStringLiteral("Recorrido guiado para conectar una cuenta, configurar y empezar a jugar"));
    connect(tutorialButton, &QPushButton::clicked, this, [this] {
        showQuickTutorial(this);
    });
    sidebarLayout->addWidget(tutorialButton);
    sidebarLayout->addSpacing(theme::sizes::kSpacingSmall);

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
    QAction* signInAction = menu.addAction(QStringLiteral("Conectar cuenta…"));
    QAction* tutorialAction = menu.addAction(QStringLiteral("Tutorial rápido"));
    QAction* logoutAction = nullptr;
    if (m_auth->isLoggedIn()) {
        menu.addSeparator();
        logoutAction = menu.addAction(QStringLiteral("Cerrar sesión"));
    }

    QAction* chosen = menu.exec(QCursor::pos());
    if (chosen == tutorialAction) {
        showQuickTutorial(this);
        return;
    }
    if (chosen == logoutAction) {
        m_auth->logout();
        return;
    }
    if (chosen != signInAction)
        return;

    QSettings settings;
    QString configuredClientId = qEnvironmentVariable("CUB3_MICROSOFT_CLIENT_ID").trimmed();
    if (configuredClientId.isEmpty())
        configuredClientId = settings.value(QStringLiteral("auth/microsoftClientId")).toString().trimmed();
    const bool microsoftClientRequired = configuredClientId.isEmpty()
        || configuredClientId == cub3::core::meta::kMicrosoftClientId.toString();
    LoginDialog dialog(microsoftClientRequired, this);
    if (!microsoftClientRequired)
        dialog.setProperty("configuredMicrosoftClientId", configuredClientId);

    QPointer<LoginDialog> dialogGuard(&dialog);
    QPointer<MainWindow> windowGuard(this);
    auto* authManager = m_auth;
    dialog.setCancelHandler([authManager] {
        if (authManager)
            authManager->cancelLogin();
    });
    dialog.setSubmitHandler([dialogGuard, windowGuard, authManager,
                             configuredClientId](LoginDialog* submitted) {
        if (!dialogGuard || !windowGuard || !authManager)
            return;

        if (submitted->provider() == 0) {
            QString clientId = configuredClientId;
            if (clientId.isEmpty()
                || clientId == cub3::core::meta::kMicrosoftClientId.toString()) {
                clientId = submitted->clientId();
                static const QRegularExpression clientIdPattern(
                    QStringLiteral("^[0-9A-Fa-f]{8}(-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}$"));
                if (!clientIdPattern.match(clientId).hasMatch()) {
                    submitted->showLoginError(
                        QStringLiteral("Escribe un ID GUID válido de la aplicación pública de Microsoft. No es tu correo ni una contraseña."));
                    return;
                }
                QSettings().setValue(QStringLiteral("auth/microsoftClientId"), clientId);
                qputenv("CUB3_MICROSOFT_CLIENT_ID", clientId.toUtf8());
            }

            submitted->setBusy(true,
                QStringLiteral("Esperando autorización de Microsoft en tu navegador…"));
            if (windowGuard->m_dashboard)
                windowGuard->m_dashboard->setStatus(
                    QStringLiteral("Esperando autorización de Microsoft…"));
            authManager->loginMicrosoft(
                [dialogGuard, windowGuard](const auth::Account&) {
                    if (!dialogGuard || !windowGuard)
                        return;
                    if (windowGuard->m_dashboard)
                        windowGuard->m_dashboard->setStatus(QStringLiteral("Cuenta conectada"));
                    dialogGuard->clearPassword();
                    dialogGuard->accept();
                },
                [dialogGuard, windowGuard](const QString& error) {
                    if (!dialogGuard || !windowGuard)
                        return;
                    if (windowGuard->m_dashboard)
                        windowGuard->m_dashboard->setStatus(
                            QStringLiteral("No se pudo conectar la cuenta"));
                    dialogGuard->showLoginError(error);
                });
            return;
        }

        const QString username = submitted->username();
        const QString password = submitted->password();
        if (username.isEmpty() || password.isEmpty()) {
            submitted->showLoginError(
                QStringLiteral("Completa tu correo o usuario y la contraseña de Ely.by."));
            return;
        }
        submitted->setBusy(true, QStringLiteral("Conectando con Ely.by por HTTPS…"));
        if (windowGuard->m_dashboard)
            windowGuard->m_dashboard->setStatus(QStringLiteral("Conectando con Ely.by…"));
        authManager->loginElyby(username, password,
            [dialogGuard, windowGuard](const auth::Account&) {
                if (!dialogGuard || !windowGuard)
                    return;
                if (windowGuard->m_dashboard)
                    windowGuard->m_dashboard->setStatus(QStringLiteral("Cuenta conectada"));
                dialogGuard->clearPassword();
                dialogGuard->accept();
            },
            [dialogGuard, windowGuard](const QString& error) {
                if (!dialogGuard || !windowGuard)
                    return;
                if (windowGuard->m_dashboard)
                    windowGuard->m_dashboard->setStatus(
                        QStringLiteral("No se pudo conectar la cuenta"));
                dialogGuard->clearPassword();
                dialogGuard->showLoginError(error);
            });
    });
    dialog.exec();
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
    m_dashboard->setPlayerName(m_auth->isRestoring()
        ? QStringLiteral("RESTAURANDO…") : m_auth->currentAccount().username);
    connect(m_auth, &auth::AuthManager::accountChanged,
            m_dashboard, [this](const auth::Account& account) {
        m_dashboard->setPlayerName(account.username);
    });
    connect(m_auth, &auth::AuthManager::sessionRestoreFailed,
            this, [this](const QString& reason) {
        QMessageBox::warning(
            this, QStringLiteral("No se pudo restaurar la sesión"),
            QStringLiteral("El proveedor no validó la sesión guardada. Las credenciales no se eliminaron; puedes volver a intentarlo iniciando sesión.\n\n%1")
                .arg(reason));
    });
      connect(m_auth, &auth::AuthManager::microsoftDeviceCode,
            this, [this](const QString& code, const QString& uri) {
        auto* dialog = new QMessageBox(
            QMessageBox::Information,
            QStringLiteral("Autoriza Microsoft"),
            QStringLiteral("Código para Microsoft: %1\n\nEl navegador se abrió en %2. Introduce el código allí y confirma el acceso; no compartas el código con nadie.")
                .arg(code, uri),
            QMessageBox::Ok, this);
        dialog->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
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
