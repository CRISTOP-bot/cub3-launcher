// CUB3 Launcher - Settings page implementation
#include "SettingsPage.h"
#include "styles/Theme.h"
#include "application/Cub3App.h"
#include "core/Paths.h"
#include "storage/Settings.h"
#include "network/DownloadEngine.h"
#include "gitlab/GitLabManager.h"

#include <QDesktopServices>
#include <QComboBox>
#include <QTextBrowser>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QUrl>
#include <QStringList>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QScrollArea>
#include <QFileInfo>
#include <QDir>
#include <QStyle>
#include <QStandardPaths>

namespace cub3::ui::pages {

SettingsPage::SettingsPage(cub3::application::Cub3App* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    setupUi();
    if (!m_app)
        return;

    auto& gitLab = m_app->gitLabManager();
    connect(&gitLab, &cub3::gitlab::GitLabManager::connectionChanged,
            this, [this](bool connected, const QString& username) {
        m_gitLabStatus->setText(connected
            ? QStringLiteral("Conectado como %1").arg(username)
            : QStringLiteral("No conectado"));
        m_gitLabConnectButton->setText(connected
            ? QStringLiteral("Desconectar") : QStringLiteral("Conectar GitLab"));
        m_gitLabProjects->setEnabled(connected);
        m_gitLabRefreshButton->setEnabled(connected);
        if (!connected) {
            const QSignalBlocker blocker(m_gitLabProjects);
            m_gitLabProjects->clear();
            m_gitLabDetails->clear();
        }
    });
    connect(&gitLab, &cub3::gitlab::GitLabManager::authorizationCodeReady,
            this, [this](const QString& url, const QString& code) {
        const bool opened = QDesktopServices::openUrl(QUrl(url));
        QMessageBox dialog(this);
        dialog.setWindowTitle(QStringLiteral("Autoriza GitLab"));
        dialog.setIcon(QMessageBox::Information);
        dialog.setText(QStringLiteral("%1\n\nCódigo de autorización: %2\n\nConfirma el acceso en GitLab. No compartas este código.")
            .arg(opened ? QStringLiteral("Se abrió GitLab en tu navegador.")
                        : QStringLiteral("Abre esta dirección en tu navegador: %1").arg(url), code));
        dialog.setTextFormat(Qt::PlainText);
        dialog.exec();
    });
    connect(&gitLab, &cub3::gitlab::GitLabManager::errorOccurred,
            this, [this](const QString& message) {
        m_gitLabStatus->setText(message);
    });
    connect(&gitLab, &cub3::gitlab::GitLabManager::projectsReady,
            this, [this](const QJsonArray& projects) {
        const QSignalBlocker blocker(m_gitLabProjects);
        m_gitLabProjects->clear();
        for (const QJsonValue& item : projects) {
            const QJsonObject project = item.toObject();
            const QString name = project.value(QStringLiteral("path_with_namespace")).toString();
            const QString id = project.value(QStringLiteral("id")).toVariant().toString();
            if (!name.isEmpty() && !id.isEmpty()) {
                m_gitLabProjects->addItem(name, id);
                m_gitLabProjects->setItemData(m_gitLabProjects->count() - 1,
                    project.value(QStringLiteral("default_branch")).toString(), Qt::UserRole + 1);
            }
        }
        m_gitLabStatus->setText(QStringLiteral("Proyectos visibles: %1 (máximo 100)").arg(m_gitLabProjects->count()));
    });
    connect(&gitLab, &cub3::gitlab::GitLabManager::projectDataReady,
            this, [this](const QJsonArray& branches, const QJsonArray& commits,
                         const QJsonArray& releases, const QJsonArray& tags) {
        QStringList lines;
        lines << QStringLiteral("RAMAS");
        for (const QJsonValue& item : branches) {
            const QJsonObject branch = item.toObject();
            lines << QStringLiteral("• %1%2").arg(branch.value(QStringLiteral("name")).toString(),
                branch.value(QStringLiteral("default")).toBool() ? QStringLiteral(" (predeterminada)") : QString{});
        }
        lines << QString{} << QStringLiteral("COMMITS RECIENTES");
        for (const QJsonValue& item : commits) {
            const QJsonObject commit = item.toObject();
            lines << QStringLiteral("• %1 — %2").arg(commit.value(QStringLiteral("short_id")).toString(),
                commit.value(QStringLiteral("title")).toString());
        }
        lines << QString{} << QStringLiteral("RELEASES");
        for (const QJsonValue& item : releases) {
            const QJsonObject release = item.toObject();
            lines << QStringLiteral("• %1 — %2").arg(release.value(QStringLiteral("tag_name")).toString(),
                release.value(QStringLiteral("name")).toString());
        }
        lines << QString{} << QStringLiteral("ETIQUETAS RECIENTES");
        for (const QJsonValue& item : tags) {
            const QJsonObject tag = item.toObject();
            lines << QStringLiteral("• %1 — %2").arg(tag.value(QStringLiteral("name")).toString(),
                tag.value(QStringLiteral("message")).toString());
        }
        if (branches.isEmpty() && commits.isEmpty() && releases.isEmpty() && tags.isEmpty())
            lines << QStringLiteral("No se encontraron datos para este proyecto.");
        m_gitLabDetails->setPlainText(lines.join(QLatin1Char('\n')));
        m_gitLabStatus->setText(QStringLiteral("Datos actualizados."));
    });

    const QString savedInstance = m_app->settings().get(
        QStringLiteral("gitlab/instance"), QStringLiteral("https://gitlab.com")).toString();
    m_gitLabInstance->setText(gitLab.instanceUrl().isEmpty()
        ? savedInstance : gitLab.instanceUrl());
    m_gitLabClientId->setText(m_app->settings().get(QStringLiteral("gitlab/client_id")).toString());
    m_gitLabStatus->setText(gitLab.isConnected()
        ? QStringLiteral("Conectado como %1").arg(gitLab.username()) : QStringLiteral("No conectado"));
    m_gitLabProjects->setEnabled(gitLab.isConnected());
    m_gitLabRefreshButton->setEnabled(gitLab.isConnected());
    m_gitLabConnectButton->setText(gitLab.isConnected()
        ? QStringLiteral("Desconectar") : QStringLiteral("Conectar GitLab"));
    connect(m_gitLabConnectButton, &QPushButton::clicked,
            this, &SettingsPage::onGitLabConnect);
    connect(m_gitLabProjects, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onGitLabProjectChanged);
    connect(m_gitLabRefreshButton, &QPushButton::clicked,
            this, &SettingsPage::onGitLabRefreshProjects);
}

void SettingsPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge);
    layout->setSpacing(theme::sizes::kSpacingMedium);

    auto* titleLabel = new QLabel(QStringLiteral("Ajustes"), this);
    titleLabel->setObjectName(QStringLiteral("PageTitle"));
    layout->addWidget(titleLabel);

    auto* subtitle = new QLabel(
        QStringLiteral("Configura Java, memoria y descargas para tu juego."), this);
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    layout->addWidget(subtitle);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("SettingsScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* body = new QWidget(scroll);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(2, 2, 10, 8);
    bodyLayout->setSpacing(theme::sizes::kSpacingMedium);

    // --- Java section ---
    auto* javaGroup = new QGroupBox(QStringLiteral("Java"), this);
    auto* javaLayout = new QFormLayout(javaGroup);
    javaLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_javaPath = new QLineEdit(javaGroup);
    m_javaPath->setPlaceholderText(QStringLiteral("Ruta del ejecutable de Java"));
    m_javaPath->setAccessibleName(QStringLiteral("Ruta del ejecutable de Java"));
    m_javaPath->setToolTip(QStringLiteral("Debe apuntar a un archivo ejecutable de Java"));
    javaLayout->addRow(QStringLiteral("Ruta de Java:"), m_javaPath);

    auto* javaBrowseBtn = new QPushButton(QStringLiteral("Buscar..."), javaGroup);
    connect(javaBrowseBtn, &QPushButton::clicked, this, &SettingsPage::onJavaBrowse);
    javaLayout->addRow(QString(), javaBrowseBtn);

    javaGroup->setObjectName(QStringLiteral("SettingsGroup"));
    bodyLayout->addWidget(javaGroup);

    // --- RAM section ---
    auto* ramGroup = new QGroupBox(QStringLiteral("Memoria del juego"), this);
    auto* ramLayout = new QVBoxLayout(ramGroup);
    ramLayout->setSpacing(theme::sizes::kSpacingSmall);

    auto* ramDescription = new QLabel(
        QStringLiteral("Ajusta el mínimo y el máximo de memoria que recibe Minecraft."),
        ramGroup);
    ramDescription->setObjectName(QStringLiteral("Secondary"));
    ramDescription->setWordWrap(true);
    ramLayout->addWidget(ramDescription);

    // Min RAM
    auto* minRow = new QHBoxLayout();
    minRow->addWidget(new QLabel(QStringLiteral("Mínima (MB):")));
    m_ramMinSlider = new QSlider(Qt::Horizontal, ramGroup);
    m_ramMinSlider->setRange(512, 32768);
    m_ramMinSlider->setAccessibleName(QStringLiteral("Memoria mínima para Minecraft, en megabytes"));
    m_ramMinSlider->setToolTip(QStringLiteral("Usa las flechas para ajustar en pasos de 256 MB"));
    m_ramMinSlider->setSingleStep(256);
    m_ramMinSlider->setValue(512);
    m_ramMinLabel = new QLabel(QStringLiteral("512"), ramGroup);
    m_ramMinLabel->setFixedWidth(60);
    minRow->addWidget(m_ramMinSlider, 1);
    minRow->addWidget(m_ramMinLabel);
    ramLayout->addLayout(minRow);

    // Max RAM
    auto* maxRow = new QHBoxLayout();
    maxRow->addWidget(new QLabel(QStringLiteral("Máxima (MB):")));
    m_ramMaxSlider = new QSlider(Qt::Horizontal, ramGroup);
    m_ramMaxSlider->setRange(512, 32768);
    m_ramMaxSlider->setAccessibleName(QStringLiteral("Memoria máxima para Minecraft, en megabytes"));
    m_ramMaxSlider->setToolTip(QStringLiteral("No asignes toda la memoria del equipo al juego"));
    m_ramMaxSlider->setSingleStep(256);
    m_ramMaxSlider->setValue(2048);
    m_ramMaxLabel = new QLabel(QStringLiteral("2048"), ramGroup);
    m_ramMaxLabel->setFixedWidth(60);
    maxRow->addWidget(m_ramMaxSlider, 1);
    maxRow->addWidget(m_ramMaxLabel);
    ramLayout->addLayout(maxRow);

    connect(m_ramMinSlider, &QSlider::valueChanged, this, [this](int v) {
        m_ramMinLabel->setText(QString::number(v));
        if (v > m_ramMaxSlider->value())
            m_ramMaxSlider->setValue(v);
    });
    connect(m_ramMaxSlider, &QSlider::valueChanged, this, [this](int v) {
        m_ramMaxLabel->setText(QString::number(v));
        if (v < m_ramMinSlider->value())
            m_ramMinSlider->setValue(v);
    });

    ramGroup->setObjectName(QStringLiteral("SettingsGroup"));
    bodyLayout->addWidget(ramGroup);

    // --- Directories section ---
    auto* dirGroup = new QGroupBox(QStringLiteral("Directorios"), this);
    auto* dirLayout = new QFormLayout(dirGroup);
    dirLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_instancesDir = new QLineEdit(dirGroup);
    m_instancesDir->setAccessibleName(QStringLiteral("Carpeta de perfiles e instalaciones"));
    dirLayout->addRow(QStringLiteral("Perfiles:"), m_instancesDir);

    auto* dirBrowseBtn = new QPushButton(QStringLiteral("Buscar..."), dirGroup);
    connect(dirBrowseBtn, &QPushButton::clicked, this, &SettingsPage::onDirBrowse);
    dirLayout->addRow(QString(), dirBrowseBtn);

    dirGroup->setObjectName(QStringLiteral("SettingsGroup"));
    bodyLayout->addWidget(dirGroup);

    // --- Network section ---
    auto* netGroup = new QGroupBox(QStringLiteral("Red"), this);
    auto* netLayout = new QFormLayout(netGroup);
    netLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_concurrencySpin = new QSpinBox(netGroup);
    m_concurrencySpin->setRange(1, 16);
    m_concurrencySpin->setAccessibleName(QStringLiteral("Número máximo de descargas simultáneas"));
    m_concurrencySpin->setToolTip(QStringLiteral("Más descargas simultáneas pueden usar más red y almacenamiento"));
    m_concurrencySpin->setValue(4);
    netLayout->addRow(QStringLiteral("Descargas simultáneas:"), m_concurrencySpin);

    m_validateDownloadsCheck = new QCheckBox(QStringLiteral("Verificar los archivos descargados"), netGroup);
    m_validateDownloadsCheck->setChecked(true);
    m_validateDownloadsCheck->setToolTip(QStringLiteral("Comprueba tamaño y hash cuando el manifiesto proporciona esos datos"));
    netLayout->addRow(QString(), m_validateDownloadsCheck);

    netGroup->setObjectName(QStringLiteral("SettingsGroup"));
    bodyLayout->addWidget(netGroup);

    // --- Resolution ---
    auto* appGroup = new QGroupBox(QStringLiteral("Ventana del juego"), this);
    auto* appLayout = new QVBoxLayout(appGroup);
    appLayout->setSpacing(theme::sizes::kSpacingSmall);
    auto* resRow = new QHBoxLayout();
    resRow->addWidget(new QLabel(QStringLiteral("Resolución del juego:")));
    m_resolutionW = new QSpinBox(appGroup);
    m_resolutionW->setAccessibleName(QStringLiteral("Ancho de la ventana del juego"));
    m_resolutionW->setRange(640, 7680);
    m_resolutionW->setValue(854);
    m_resolutionW->setSuffix(QStringLiteral("px"));
    resRow->addWidget(m_resolutionW);
    resRow->addWidget(new QLabel(QStringLiteral("x")));
    m_resolutionH = new QSpinBox(appGroup);
    m_resolutionH->setAccessibleName(QStringLiteral("Alto de la ventana del juego"));
    m_resolutionH->setRange(480, 4320);
    m_resolutionH->setValue(480);
    m_resolutionH->setSuffix(QStringLiteral("px"));
    resRow->addWidget(m_resolutionH);
    resRow->addStretch();
    appLayout->addLayout(resRow);

    appGroup->setObjectName(QStringLiteral("SettingsGroup"));
    bodyLayout->addWidget(appGroup);

    auto* gitLabGroup = new QGroupBox(QStringLiteral("GitLab · proyectos y releases"), body);
    gitLabGroup->setObjectName(QStringLiteral("SettingsGroup"));
    auto* gitLabLayout = new QVBoxLayout(gitLabGroup);
    auto* gitLabNotice = new QLabel(
        QStringLiteral("Conexión OAuth de solo lectura. Registra una aplicación OAuth en GitLab con autorización de dispositivo y los permisos read_api y read_user. No pegues aquí tokens ni secretos."),
        gitLabGroup);
    gitLabNotice->setWordWrap(true);
    gitLabNotice->setObjectName(QStringLiteral("Secondary"));
    gitLabLayout->addWidget(gitLabNotice);
    auto* gitLabForm = new QFormLayout();
    m_gitLabInstance = new QLineEdit(gitLabGroup);
    m_gitLabInstance->setPlaceholderText(QStringLiteral("https://gitlab.com"));
    m_gitLabInstance->setAccessibleName(QStringLiteral("URL HTTPS de GitLab"));
    gitLabForm->addRow(QStringLiteral("Instancia:"), m_gitLabInstance);
    m_gitLabClientId = new QLineEdit(gitLabGroup);
    m_gitLabClientId->setPlaceholderText(QStringLiteral("ID de aplicación OAuth"));
    m_gitLabClientId->setAccessibleName(QStringLiteral("ID de aplicación OAuth de GitLab"));
    gitLabForm->addRow(QStringLiteral("ID OAuth:"), m_gitLabClientId);
    gitLabLayout->addLayout(gitLabForm);
    auto* gitLabActions = new QHBoxLayout();
    m_gitLabConnectButton = new QPushButton(QStringLiteral("Conectar GitLab"), gitLabGroup);
    m_gitLabConnectButton->setObjectName(QStringLiteral("Primary"));
    gitLabActions->addWidget(m_gitLabConnectButton);
    m_gitLabRefreshButton = new QPushButton(QStringLiteral("Actualizar proyectos"), gitLabGroup);
    m_gitLabRefreshButton->setEnabled(false);
    gitLabActions->addWidget(m_gitLabRefreshButton);
    m_gitLabStatus = new QLabel(QStringLiteral("No conectado"), gitLabGroup);
    m_gitLabStatus->setTextFormat(Qt::PlainText);
    m_gitLabStatus->setWordWrap(true);
    gitLabActions->addWidget(m_gitLabStatus, 1);
    gitLabLayout->addLayout(gitLabActions);
    m_gitLabProjects = new QComboBox(gitLabGroup);
    m_gitLabProjects->setAccessibleName(QStringLiteral("Proyecto GitLab"));
    m_gitLabProjects->setEnabled(false);
    gitLabLayout->addWidget(m_gitLabProjects);
    m_gitLabDetails = new QTextBrowser(gitLabGroup);
    m_gitLabDetails->setOpenExternalLinks(false);
    m_gitLabDetails->setMaximumHeight(220);
    m_gitLabDetails->setPlaceholderText(QStringLiteral("Selecciona un proyecto para consultar ramas, commits recientes, releases y etiquetas."));
    gitLabLayout->addWidget(m_gitLabDetails);
    bodyLayout->addWidget(gitLabGroup);
    bodyLayout->addStretch(1);
    scroll->setWidget(body);
    layout->addWidget(scroll, 1);

    // Keep save controls visible while the settings list scrolls.
    auto* saveRow = new QHBoxLayout();
    m_saveStatus = new QLabel(this);
    m_saveStatus->setObjectName(QStringLiteral("SettingsSaveStatus"));
    saveRow->addWidget(m_saveStatus, 1);
    auto* saveBtn = new QPushButton(QStringLiteral("Guardar cambios"), this);
    saveBtn->setObjectName(QStringLiteral("Primary"));
    saveBtn->setMinimumWidth(180);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::onSave);
    saveRow->addWidget(saveBtn, 0, Qt::AlignRight);
    layout->addLayout(saveRow);

    // Load current settings before wiring dirty-state feedback.
    loadSettings();
    const auto markDirty = [this] {
        m_saveStatus->setProperty("state", "dirty");
        m_saveStatus->setText(QStringLiteral("Tienes cambios sin guardar."));
        m_saveStatus->style()->unpolish(m_saveStatus);
        m_saveStatus->style()->polish(m_saveStatus);
    };
    connect(m_javaPath, &QLineEdit::textChanged, this, markDirty);
    connect(m_instancesDir, &QLineEdit::textChanged, this, markDirty);
    connect(m_ramMinSlider, &QSlider::valueChanged, this, markDirty);
    connect(m_ramMaxSlider, &QSlider::valueChanged, this, markDirty);
    connect(m_concurrencySpin, &QSpinBox::valueChanged, this, markDirty);
    connect(m_validateDownloadsCheck, &QCheckBox::toggled, this, markDirty);
    connect(m_resolutionW, &QSpinBox::valueChanged, this, markDirty);
    connect(m_resolutionH, &QSpinBox::valueChanged, this, markDirty);
}

void SettingsPage::loadSettings() {
    if (!m_app)
        return;

    auto& settings = m_app->settings();
    m_javaPath->setText(settings.javaPath());
    m_ramMinSlider->setValue(settings.ramMinMB());
    m_ramMaxSlider->setValue(settings.ramMaxMB());
    m_ramMinLabel->setText(QString::number(m_ramMinSlider->value()));
    m_ramMaxLabel->setText(QString::number(m_ramMaxSlider->value()));
    const QString instancesDir = settings.instancesDir();
    m_instancesDir->setText(instancesDir.isEmpty()
                                ? cub3::core::Paths::instancesDir()
                                : instancesDir);
    m_concurrencySpin->setValue(settings.maxConcurrentDownloads());
    m_validateDownloadsCheck->setChecked(settings.validateDownloads());
    m_resolutionW->setValue(settings.resolutionWidth());
    m_resolutionH->setValue(settings.resolutionHeight());
}

void SettingsPage::onJavaBrowse() {
#ifdef CUB3_PLATFORM_WINDOWS
    QString initialDirectory = QStandardPaths::findExecutable(QStringLiteral("java.exe"));
    if (!initialDirectory.isEmpty())
        initialDirectory = QFileInfo(initialDirectory).absolutePath();
    auto path = QFileDialog::getOpenFileName(this,
        QStringLiteral("Seleccionar ejecutable de Java"), initialDirectory,
        QStringLiteral("Java (java.exe) (*.exe)"));
#else
    auto path = QFileDialog::getOpenFileName(this,
        QStringLiteral("Seleccionar ejecutable de Java"),
        QStringLiteral("/usr/bin"), QStringLiteral("Java (java java-*)"));
#endif
    if (!path.isEmpty())
        m_javaPath->setText(path);
}

void SettingsPage::onDirBrowse() {
    auto dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Seleccionar directorio de perfiles"),
        m_instancesDir->text());
    if (!dir.isEmpty())
        m_instancesDir->setText(dir);
}

void SettingsPage::onGitLabConnect() {
    if (!m_app)
        return;
    auto& manager = m_app->gitLabManager();
    if (manager.isConnected()) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Desconectar GitLab"),
            QStringLiteral("¿Quieres quitar de este dispositivo la sesión GitLab guardada? Para revocar el acceso, quita también la aplicación desde GitLab → Preferencias → Aplicaciones autorizadas."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes)
            manager.disconnectAccount();
        return;
    }
    const QString instance = m_gitLabInstance->text().trimmed();
    const QString clientId = m_gitLabClientId->text().trimmed();
    if (!cub3::gitlab::GitLabManager::isValidInstanceUrl(QUrl(instance))) {
        m_gitLabStatus->setText(QStringLiteral("La instancia debe ser una URL HTTPS válida, por ejemplo https://gitlab.com."));
        return;
    }
    if (clientId.isEmpty()) {
        m_gitLabStatus->setText(QStringLiteral("Falta el ID público de la aplicación OAuth registrada en GitLab."));
        return;
    }
    m_app->settings().set(QStringLiteral("gitlab/instance"), instance);
    m_app->settings().set(QStringLiteral("gitlab/client_id"), clientId);
    m_gitLabStatus->setText(QStringLiteral("Solicitando autorización a GitLab…"));
    manager.authorize(instance, clientId);
}

void SettingsPage::onGitLabRefreshProjects() {
    if (!m_app)
        return;
    m_gitLabStatus->setText(QStringLiteral("Consultando proyectos de GitLab…"));
    m_app->gitLabManager().listProjects();
}

void SettingsPage::onGitLabProjectChanged(int index) {
    if (!m_app || index < 0)
        return;
    const QString id = m_gitLabProjects->itemData(index).toString();
    const QString branch = m_gitLabProjects->itemData(index, Qt::UserRole + 1).toString();
    if (id.isEmpty())
        return;
    m_gitLabStatus->setText(QStringLiteral("Cargando datos del proyecto…"));
    m_gitLabDetails->clear();
    m_app->gitLabManager().loadProject(id, branch);
}

void SettingsPage::onSave() {
    if (!m_app)
        return;

    const QString javaPath = m_javaPath->text().trimmed();
    if (!javaPath.isEmpty()) {
        const QFileInfo javaInfo(javaPath);
        if (!javaInfo.isFile() || !javaInfo.isExecutable()) {
            m_saveStatus->setProperty("state", "error");
            m_saveStatus->setText(QStringLiteral("La ruta de Java no es ejecutable."));
            m_saveStatus->style()->unpolish(m_saveStatus);
            m_saveStatus->style()->polish(m_saveStatus);
            return;
        }
    }

    const QString instancesDir = QDir::cleanPath(
        QDir(m_instancesDir->text().trimmed().isEmpty()
                 ? cub3::core::Paths::instancesDir()
                 : m_instancesDir->text().trimmed()).absolutePath());
    const QFileInfo directoryInfo(instancesDir);
    if (!directoryInfo.isDir() || !directoryInfo.isWritable()) {
        m_saveStatus->setProperty("state", "error");
        m_saveStatus->setText(QStringLiteral("La carpeta de perfiles no existe o no permite escribir."));
        m_saveStatus->style()->unpolish(m_saveStatus);
        m_saveStatus->style()->polish(m_saveStatus);
        return;
    }

    auto& settings = m_app->settings();
    settings.setJavaPath(javaPath);
    settings.setRamMinMB(m_ramMinSlider->value());
    settings.setRamMaxMB(m_ramMaxSlider->value());
    settings.setInstancesDir(instancesDir);
    settings.setMaxConcurrentDownloads(m_concurrencySpin->value());
    settings.setValidateDownloads(m_validateDownloadsCheck->isChecked());
    settings.setResolutionWidth(m_resolutionW->value());
    settings.setResolutionHeight(m_resolutionH->value());
    m_app->downloadEngine().setMaxConcurrent(m_concurrencySpin->value());
    m_app->downloadEngine().setValidateDownloads(
        m_validateDownloadsCheck->isChecked());
    m_saveStatus->setProperty("state", "success");
    m_saveStatus->setText(QStringLiteral("Ajustes guardados correctamente."));
    m_saveStatus->style()->unpolish(m_saveStatus);
    m_saveStatus->style()->polish(m_saveStatus);
}

} // namespace cub3::ui::pages
