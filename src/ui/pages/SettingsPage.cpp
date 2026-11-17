// CUB3 Launcher - Settings page implementation
#include "SettingsPage.h"
#include "styles/Theme.h"
#include "application/Cub3App.h"
#include "core/Paths.h"
#include "storage/Settings.h"
#include "network/DownloadEngine.h"

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

namespace cub3::ui::pages {

SettingsPage::SettingsPage(cub3::application::Cub3App* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    setupUi();
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

    // --- Java section ---
    auto* javaGroup = new QGroupBox(QStringLiteral("Java"), this);
    auto* javaLayout = new QFormLayout(javaGroup);
    javaLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_javaPath = new QLineEdit(javaGroup);
    m_javaPath->setPlaceholderText(QStringLiteral("Ruta del ejecutable de Java"));
    javaLayout->addRow(QStringLiteral("Ruta de Java:"), m_javaPath);

    auto* javaBrowseBtn = new QPushButton(QStringLiteral("Buscar..."), javaGroup);
    connect(javaBrowseBtn, &QPushButton::clicked, this, &SettingsPage::onJavaBrowse);
    javaLayout->addRow(QString(), javaBrowseBtn);

    layout->addWidget(javaGroup);

    // --- RAM section ---
    auto* ramGroup = new QGroupBox(QStringLiteral("Memoria del juego"), this);
    auto* ramLayout = new QVBoxLayout(ramGroup);
    ramLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_ramTotalLabel = new QLabel(QStringLiteral("Define la memoria mínima y máxima para el juego."), ramGroup);
    m_ramTotalLabel->setObjectName(QStringLiteral("Secondary"));
    ramLayout->addWidget(m_ramTotalLabel);

    // Min RAM
    auto* minRow = new QHBoxLayout();
    minRow->addWidget(new QLabel(QStringLiteral("Mínima (MB):")));
    m_ramMinSlider = new QSlider(Qt::Horizontal, ramGroup);
    m_ramMinSlider->setRange(512, 32768);
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

    layout->addWidget(ramGroup);

    // --- Directories section ---
    auto* dirGroup = new QGroupBox(QStringLiteral("Directorios"), this);
    auto* dirLayout = new QFormLayout(dirGroup);
    dirLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_instancesDir = new QLineEdit(dirGroup);
    dirLayout->addRow(QStringLiteral("Perfiles:"), m_instancesDir);

    auto* dirBrowseBtn = new QPushButton(QStringLiteral("Buscar..."), dirGroup);
    connect(dirBrowseBtn, &QPushButton::clicked, this, &SettingsPage::onDirBrowse);
    dirLayout->addRow(QString(), dirBrowseBtn);

    layout->addWidget(dirGroup);

    // --- Network section ---
    auto* netGroup = new QGroupBox(QStringLiteral("Red"), this);
    auto* netLayout = new QFormLayout(netGroup);
    netLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_concurrencySpin = new QSpinBox(netGroup);
    m_concurrencySpin->setRange(1, 16);
    m_concurrencySpin->setValue(4);
    netLayout->addRow(QStringLiteral("Descargas simultáneas:"), m_concurrencySpin);

    m_validateDownloadsCheck = new QCheckBox(QStringLiteral("Verificar los archivos descargados"), netGroup);
    m_validateDownloadsCheck->setChecked(true);
    netLayout->addRow(QString(), m_validateDownloadsCheck);

    layout->addWidget(netGroup);

    // --- Appearance + Performance ---
    auto* appGroup = new QGroupBox(QStringLiteral("Apariencia y rendimiento"), this);
    auto* appLayout = new QVBoxLayout(appGroup);
    appLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_reducedFxCheck = new QCheckBox(QStringLiteral("Reducir efectos (desactivar animaciones)"), appGroup);
    appLayout->addWidget(m_reducedFxCheck);

    auto* resRow = new QHBoxLayout();
    resRow->addWidget(new QLabel(QStringLiteral("Resolución del juego:")));
    m_resolutionW = new QSpinBox(appGroup);
    m_resolutionW->setRange(640, 7680);
    m_resolutionW->setValue(854);
    m_resolutionW->setSuffix(QStringLiteral("px"));
    resRow->addWidget(m_resolutionW);
    resRow->addWidget(new QLabel(QStringLiteral("x")));
    m_resolutionH = new QSpinBox(appGroup);
    m_resolutionH->setRange(480, 4320);
    m_resolutionH->setValue(480);
    m_resolutionH->setSuffix(QStringLiteral("px"));
    resRow->addWidget(m_resolutionH);
    resRow->addStretch();
    appLayout->addLayout(resRow);

    layout->addWidget(appGroup);

    // --- Save button ---
    auto* saveBtn = new QPushButton(QStringLiteral("Guardar ajustes"), this);
    saveBtn->setObjectName(QStringLiteral("Primary"));
    saveBtn->setMaximumWidth(200);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::onSave);
    layout->addWidget(saveBtn, 0, Qt::AlignLeft);

    layout->addStretch();

    // Load current settings
    loadSettings();
}

void SettingsPage::loadSettings() {
    if (!m_app)
        return;

    auto& settings = m_app->settings();
    m_javaPath->setText(settings.javaPath());
    m_ramMinSlider->setValue(settings.ramMinMB());
    m_ramMaxSlider->setValue(settings.ramMaxMB());
    const QString instancesDir = settings.instancesDir();
    m_instancesDir->setText(instancesDir.isEmpty()
                                ? cub3::core::Paths::instancesDir()
                                : instancesDir);
    m_concurrencySpin->setValue(settings.maxConcurrentDownloads());
    m_validateDownloadsCheck->setChecked(settings.validateDownloads());
    m_reducedFxCheck->setChecked(settings.reducedEffects());
    m_resolutionW->setValue(settings.resolutionWidth());
    m_resolutionH->setValue(settings.resolutionHeight());
}

void SettingsPage::onJavaBrowse() {
    auto path = QFileDialog::getOpenFileName(this,
        QStringLiteral("Seleccionar ejecutable de Java"),
        QStringLiteral("/usr/bin"),
#ifdef CUB3_PLATFORM_WINDOWS
        QStringLiteral("Java (java.exe)"));
#else
        QStringLiteral("Java (java java-*)"));
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

void SettingsPage::onRamChanged(int value) {
    Q_UNUSED(value)
}

void SettingsPage::onSave() {
    if (!m_app)
        return;

    auto& settings = m_app->settings();
    settings.setJavaPath(m_javaPath->text());
    settings.setRamMinMB(m_ramMinSlider->value());
    settings.setRamMaxMB(m_ramMaxSlider->value());
    settings.setInstancesDir(m_instancesDir->text());
    settings.setMaxConcurrentDownloads(m_concurrencySpin->value());
    settings.setValidateDownloads(m_validateDownloadsCheck->isChecked());
    settings.setReducedEffects(m_reducedFxCheck->isChecked());
    settings.setResolutionWidth(m_resolutionW->value());
    settings.setResolutionHeight(m_resolutionH->value());
    m_app->downloadEngine().setMaxConcurrent(m_concurrencySpin->value());
}

} // namespace cub3::ui::pages
