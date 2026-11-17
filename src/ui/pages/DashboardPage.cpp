// CUB3 Launcher - Dashboard page implementation
#include "DashboardPage.h"

#include "application/Cub3App.h"
#include "core/Performance.h"
#include "storage/Database.h"
#include "styles/Theme.h"
#include "versions/VersionManager.h"
#include "widgets/PlayButton.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSizePolicy>

#include <exception>

namespace cub3::ui::pages {
namespace {

class SunsetHero final : public QWidget {
public:
    explicit SunsetHero(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("SunsetHero"));
        setMinimumHeight(340);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF bounds = rect();
        QPainterPath clip;
        clip.addRoundedRect(bounds, 16, 16);
        painter.setClipPath(clip);

        QLinearGradient sky(0, 0, 0, height());
        sky.setColorAt(0.0, QColor(57, 48, 91));
        sky.setColorAt(0.48, QColor(202, 111, 101));
        sky.setColorAt(1.0, QColor(247, 171, 112));
        painter.fillRect(bounds, sky);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 215, 164, 34));
        for (int i = 0; i < 5; ++i) {
            const int x = width() * (0.50 + i * 0.075);
            const int y = height() * (0.16 + (i % 2) * 0.09);
            painter.drawRect(x, y, 44 + (i % 3) * 12, 7);
            painter.drawRect(x + 13, y - 7, 28, 7);
        }

        QRadialGradient sun(QPointF(width() * 0.76, height() * 0.51),
                            height() * 0.28);
        sun.setColorAt(0.0, QColor(255, 229, 174, 245));
        sun.setColorAt(0.42, QColor(255, 206, 154, 235));
        sun.setColorAt(1.0, QColor(255, 190, 140, 0));
        painter.setBrush(sun);
        painter.drawEllipse(QPointF(width() * 0.76, height() * 0.51),
                            height() * 0.28, height() * 0.28);
        painter.setBrush(QColor(255, 226, 174));
        const int sunSize = qMax(48, height() / 4);
        painter.drawRect(width() * 0.76 - sunSize / 2,
                         height() * 0.51 - sunSize / 2,
                         sunSize, sunSize);

        QPolygonF distantHill;
        distantHill << QPointF(0, height() * 0.69)
                    << QPointF(width() * 0.19, height() * 0.56)
                    << QPointF(width() * 0.37, height() * 0.67)
                    << QPointF(width() * 0.56, height() * 0.58)
                    << QPointF(width() * 0.77, height() * 0.70)
                    << QPointF(width(), height() * 0.59)
                    << QPointF(width(), height())
                    << QPointF(0, height());
        painter.setBrush(QColor(92, 82, 103));
        painter.drawPolygon(distantHill);

        QPolygonF nearHill;
        nearHill << QPointF(0, height() * 0.79)
                 << QPointF(width() * 0.20, height() * 0.70)
                 << QPointF(width() * 0.42, height() * 0.81)
                 << QPointF(width() * 0.64, height() * 0.72)
                 << QPointF(width() * 0.84, height() * 0.80)
                 << QPointF(width(), height() * 0.71)
                 << QPointF(width(), height())
                 << QPointF(0, height());
        painter.setBrush(QColor(44, 60, 67));
        painter.drawPolygon(nearHill);

        painter.setBrush(QColor(27, 44, 49));
        const int groundY = height() * 0.86;
        painter.drawRect(0, groundY, width(), height() - groundY);
        const QList<int> treePositions{
            static_cast<int>(width() * 0.08), static_cast<int>(width() * 0.14),
            static_cast<int>(width() * 0.59), static_cast<int>(width() * 0.93)
        };
        for (int x : treePositions) {
            const int treeHeight = height() * (x < width() * 0.2 ? 0.24 : 0.19);
            const int trunkWidth = qMax(7, width() / 100);
            const int top = groundY - treeHeight;
            painter.fillRect(x, top + treeHeight / 3, trunkWidth,
                             treeHeight * 2 / 3, QColor(32, 43, 43));
            painter.fillRect(x - trunkWidth * 2, top + treeHeight / 4,
                             trunkWidth * 5, treeHeight / 3, QColor(34, 55, 53));
            painter.fillRect(x - trunkWidth, top, trunkWidth * 3,
                             treeHeight / 3, QColor(39, 67, 58));
        }

        QLinearGradient shade(0, 0, width() * 0.78, 0);
        shade.setColorAt(0.0, QColor(17, 22, 35, 205));
        shade.setColorAt(0.62, QColor(17, 22, 35, 105));
        shade.setColorAt(1.0, QColor(17, 22, 35, 0));
        painter.fillRect(bounds, shade);
        QLinearGradient bottom(0, height() * 0.65, 0, height());
        bottom.setColorAt(0.0, QColor(17, 22, 35, 0));
        bottom.setColorAt(1.0, QColor(17, 22, 35, 145));
        painter.fillRect(bounds, bottom);
    }
};

QFrame* makeCard(QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("InfoCard"));
    card->setMinimumHeight(202);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return card;
}

QLabel* makeCardTitle(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("CardTitle"));
    return label;
}

} // namespace

DashboardPage::DashboardPage(application::Cub3App* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    setupUi();

    m_metricsTimer = new QTimer(this);
    m_metricsTimer->setInterval(5000);
    connect(m_metricsTimer, &QTimer::timeout, this, &DashboardPage::refreshLocalSnapshot);
    m_metricsTimer->start();
    refreshLocalSnapshot();
}

void DashboardPage::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    rootLayout->addWidget(scrollArea);

    auto* content = new QWidget(scrollArea);
    content->setObjectName(QStringLiteral("DashboardBody"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(theme::sizes::kSpacingLarge,
                                      theme::sizes::kSpacingMedium,
                                      theme::sizes::kSpacingLarge,
                                      theme::sizes::kSpacingSmall);
    contentLayout->setSpacing(theme::sizes::kSpacingMedium);

    auto* hero = new SunsetHero(content);
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(26, 20, 26, 24);
    heroLayout->setSpacing(0);

    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(theme::sizes::kSpacingSmall);
    auto* searchGlyph = new QLabel(QStringLiteral("⌕"), hero);
    searchGlyph->setObjectName(QStringLiteral("SearchGlyph"));
    topBar->addWidget(searchGlyph);

    m_versionSearch = new QLineEdit(hero);
    m_versionSearch->setObjectName(QStringLiteral("SearchField"));
    m_versionSearch->setPlaceholderText(QStringLiteral("Buscar versiones..."));
    m_versionSearch->setClearButtonEnabled(true);
    m_versionSearch->setMaximumWidth(420);
    topBar->addWidget(m_versionSearch, 1);
    topBar->addStretch(1);

    m_accountLabel = new QLabel(QStringLiteral("INVITADO"), hero);
    m_accountLabel->setObjectName(QStringLiteral("AccountBadge"));
    m_accountLabel->setAlignment(Qt::AlignCenter);
    topBar->addWidget(m_accountLabel);
    heroLayout->addLayout(topBar);
    heroLayout->addStretch(1);

    auto* eyebrow = new QLabel(QStringLiteral("CUB3 LAUNCHER"), hero);
    eyebrow->setObjectName(QStringLiteral("HeroEyebrow"));
    heroLayout->addWidget(eyebrow);

    auto* headline = new QLabel(QStringLiteral("Tu mundo empieza aquí"), hero);
    headline->setObjectName(QStringLiteral("HeroTitle"));
    headline->setWordWrap(true);
    heroLayout->addWidget(headline);

    auto* description = new QLabel(
        QStringLiteral("Elige una versión y prepara tu próxima aventura."),
        hero);
    description->setObjectName(QStringLiteral("HeroCaption"));
    description->setWordWrap(true);
    heroLayout->addWidget(description);
    heroLayout->addSpacing(theme::sizes::kSpacingMedium);

    auto* launchRow = new QHBoxLayout();
    launchRow->setSpacing(theme::sizes::kSpacingSmall);
    m_playButton = new widgets::PlayButton(QStringLiteral("JUGAR"), hero);
    connect(m_playButton, &QPushButton::clicked, this, [this] {
        emit playRequested(m_versionSelector->currentData().toString());
    });
    launchRow->addWidget(m_playButton);

    m_versionSelector = new QComboBox(hero);
    m_versionSelector->setObjectName(QStringLiteral("VersionSelector"));
    m_versionSelector->setMinimumWidth(210);
    m_versionSelector->setFixedHeight(theme::sizes::kPlayButtonH);
    m_versionSelector->setEnabled(false);
    m_versionSelector->addItem(QStringLiteral("Cargando versiones..."));
    launchRow->addWidget(m_versionSelector);
    launchRow->addStretch(1);
    heroLayout->addLayout(launchRow);

    contentLayout->addWidget(hero);

    auto* cardsLayout = new QGridLayout();
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setHorizontalSpacing(theme::sizes::kSpacingMedium);
    cardsLayout->setVerticalSpacing(theme::sizes::kSpacingMedium);
    cardsLayout->setColumnStretch(0, 11);
    cardsLayout->setColumnStretch(1, 10);
    cardsLayout->setColumnStretch(2, 9);

    auto* newsCard = makeCard(content);
    auto* newsLayout = new QVBoxLayout(newsCard);
    newsLayout->setContentsMargins(18, 16, 18, 16);
    newsLayout->setSpacing(10);
    newsLayout->addWidget(makeCardTitle(QStringLiteral("Novedades"), newsCard));

    auto* featureArt = new QFrame(newsCard);
    featureArt->setObjectName(QStringLiteral("FeatureArt"));
    featureArt->setMinimumHeight(62);
    auto* artLayout = new QVBoxLayout(featureArt);
    artLayout->setContentsMargins(14, 10, 14, 10);
    auto* welcomeLabel = new QLabel(QStringLiteral("Explora a tu manera"), featureArt);
    welcomeLabel->setObjectName(QStringLiteral("FeatureTitle"));
    artLayout->addWidget(welcomeLabel);
    auto* welcomeCaption = new QLabel(
        QStringLiteral("Tus versiones, reunidas en un solo lugar."),
        featureArt);
    welcomeCaption->setObjectName(QStringLiteral("CardCaption"));
    welcomeCaption->setWordWrap(true);
    artLayout->addWidget(welcomeCaption);
    newsLayout->addWidget(featureArt);

    auto* newsDetail = new QLabel(
        QStringLiteral("Busca una versión arriba para encontrarla rápidamente."),
        newsCard);
    newsDetail->setObjectName(QStringLiteral("CardCaption"));
    newsDetail->setWordWrap(true);
    newsLayout->addWidget(newsDetail);
    newsLayout->addStretch(1);
    cardsLayout->addWidget(newsCard, 0, 0);

    auto* profilesCard = makeCard(content);
    auto* profilesLayout = new QVBoxLayout(profilesCard);
    profilesLayout->setContentsMargins(18, 16, 18, 16);
    profilesLayout->setSpacing(10);
    profilesLayout->addWidget(makeCardTitle(QStringLiteral("Perfiles"), profilesCard));
    m_profileSummary = new QLabel(QStringLiteral("Cargando perfiles locales..."), profilesCard);
    m_profileSummary->setObjectName(QStringLiteral("CardCaption"));
    m_profileSummary->setWordWrap(true);
    profilesLayout->addWidget(m_profileSummary);
    profilesLayout->addStretch(1);
    auto* profilesButton = new QPushButton(QStringLiteral("Ver perfiles"), profilesCard);
    profilesButton->setObjectName(QStringLiteral("SecondaryButton"));
    connect(profilesButton, &QPushButton::clicked, this, &DashboardPage::profilesRequested);
    profilesLayout->addWidget(profilesButton, 0, Qt::AlignLeft);
    cardsLayout->addWidget(profilesCard, 0, 1);

    auto* performanceCard = makeCard(content);
    auto* performanceLayout = new QVBoxLayout(performanceCard);
    performanceLayout->setContentsMargins(18, 16, 18, 16);
    performanceLayout->setSpacing(9);
    performanceLayout->addWidget(makeCardTitle(QStringLiteral("Rendimiento"), performanceCard));

    auto* memoryRow = new QHBoxLayout();
    auto* memoryLabel = new QLabel(QStringLiteral("Memoria"), performanceCard);
    memoryLabel->setObjectName(QStringLiteral("CardCaption"));
    m_memoryValue = new QLabel(QStringLiteral("-- MB"), performanceCard);
    m_memoryValue->setObjectName(QStringLiteral("MetricValue"));
    memoryRow->addWidget(memoryLabel);
    memoryRow->addStretch(1);
    memoryRow->addWidget(m_memoryValue);
    performanceLayout->addLayout(memoryRow);

    auto* threadsRow = new QHBoxLayout();
    auto* threadsLabel = new QLabel(QStringLiteral("Hilos activos"), performanceCard);
    threadsLabel->setObjectName(QStringLiteral("CardCaption"));
    m_threadValue = new QLabel(QStringLiteral("--"), performanceCard);
    m_threadValue->setObjectName(QStringLiteral("MetricValue"));
    threadsRow->addWidget(threadsLabel);
    threadsRow->addStretch(1);
    threadsRow->addWidget(m_threadValue);
    performanceLayout->addLayout(threadsRow);
    performanceLayout->addStretch(1);
    auto* metricsNote = new QLabel(QStringLiteral("Métricas de esta aplicación"), performanceCard);
    metricsNote->setObjectName(QStringLiteral("CardCaption"));
    performanceLayout->addWidget(metricsNote);
    cardsLayout->addWidget(performanceCard, 0, 2);

    contentLayout->addLayout(cardsLayout);
    contentLayout->addStretch(1);

    auto* footer = new QHBoxLayout();
    m_statusLabel = new QLabel(QStringLiteral("Base local conectada"), content);
    m_statusLabel->setObjectName(QStringLiteral("FooterStatus"));
    footer->addWidget(m_statusLabel);
    footer->addStretch(1);
    auto* footerName = new QLabel(QStringLiteral("CUB3 LAUNCHER"), content);
    footerName->setObjectName(QStringLiteral("FooterCaption"));
    footer->addWidget(footerName);
    contentLayout->addLayout(footer);

    scrollArea->setWidget(content);

    connect(m_versionSearch, &QLineEdit::textChanged,
            this, &DashboardPage::populateVersionSelector);
}

void DashboardPage::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_versionsRequested)
        return;

    m_versionsRequested = true;
    QTimer::singleShot(0, this, &DashboardPage::requestVersions);
}

void DashboardPage::requestVersions() {
    if (!m_app)
        return;

    try {
        m_app->versionManager().fetchVersionList(
            [this](const QList<versions::VersionInfo>& versions) {
                m_versions.clear();
                m_versions.reserve(versions.size());
                for (const auto& version : versions)
                    m_versions.append(qMakePair(version.id, version.type));

                populateVersionSelector();
                if (m_versions.isEmpty()) {
                    m_statusLabel->setText(
                        QStringLiteral("No se pudieron cargar las versiones"));
                    return;
                }
                m_statusLabel->setText(QStringLiteral("Versiones actualizadas"));
            });
    } catch (const std::exception&) {
        m_versionSelector->clear();
        m_versionSelector->addItem(QStringLiteral("Versiones no disponibles"));
        m_versionSelector->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("No se pudo iniciar el servicio de versiones"));
    }
}

void DashboardPage::populateVersionSelector() {
    const QString selectedId = m_versionSelector->currentData().toString();
    const QString filter = m_versionSearch->text().trimmed();
    m_versionSelector->clear();

    int selectedIndex = -1;
    for (const auto& version : m_versions) {
        if (!filter.isEmpty() && !version.first.contains(filter, Qt::CaseInsensitive))
            continue;

        QString typeLabel = version.second;
        if (version.second == QStringLiteral("release"))
            typeLabel = QStringLiteral("Estable");
        else if (version.second == QStringLiteral("snapshot"))
            typeLabel = QStringLiteral("Instantánea");
        else if (version.second == QStringLiteral("old_beta"))
            typeLabel = QStringLiteral("Beta antigua");
        else if (version.second == QStringLiteral("old_alpha"))
            typeLabel = QStringLiteral("Alfa antigua");
        const QString label = QStringLiteral("%1  ·  %2").arg(version.first, typeLabel);
        m_versionSelector->addItem(label, version.first);
        if (version.first == selectedId)
            selectedIndex = m_versionSelector->count() - 1;
    }

    const bool hasMatches = m_versionSelector->count() > 0;
    if (!hasMatches) {
        const QString message = m_versions.isEmpty()
            ? QStringLiteral("Versiones no disponibles")
            : QStringLiteral("Sin resultados");
        m_versionSelector->addItem(message, QString());
    } else {
        if (selectedIndex < 0) {
            for (const auto& version : m_versions) {
                if (!filter.isEmpty() && !version.first.contains(filter, Qt::CaseInsensitive))
                    continue;
                if (version.second == QStringLiteral("release")) {
                    selectedIndex = m_versionSelector->findData(version.first);
                    break;
                }
            }
        }
        m_versionSelector->setCurrentIndex(selectedIndex < 0 ? 0 : selectedIndex);
    }
    m_versionSelector->setEnabled(hasMatches);
}

void DashboardPage::refreshLocalSnapshot() {
    m_memoryValue->setText(QStringLiteral("%1 MB").arg(core::Perf::rssMB()));
    m_threadValue->setText(QString::number(core::Perf::threadCount()));

    if (!m_app)
        return;
    try {
        const QVariant installationCount =
            m_app->database().queryValue(QStringLiteral("SELECT COUNT(*) FROM installations"));
        if (!installationCount.isValid()) {
            m_profileSummary->setText(QStringLiteral("No se pudieron leer los perfiles locales."));
            return;
        }

        const int count = installationCount.toInt();
        if (count == 0) {
            m_profileSummary->setText(QStringLiteral("Aún no tienes perfiles instalados."));
        } else {
            m_profileSummary->setText(QStringLiteral("%1 perfil%2 disponible%3.")
                .arg(count)
                .arg(count == 1 ? QString() : QStringLiteral("es"))
                .arg(count == 1 ? QString() : QStringLiteral("s")));
        }
    } catch (const std::exception&) {
        m_profileSummary->setText(QStringLiteral("No se pudieron leer los perfiles locales."));
    }
}

void DashboardPage::setApp(application::Cub3App* app) {
    m_app = app;
}

void DashboardPage::setPlayerName(const QString& name) {
    m_accountLabel->setText(name.trimmed().isEmpty()
        ? QStringLiteral("INVITADO")
        : name.trimmed().toUpper());
}

void DashboardPage::setVersion(const QString& version) {
    const int index = m_versionSelector->findData(version);
    if (index >= 0)
        m_versionSelector->setCurrentIndex(index);
}

void DashboardPage::setStatus(const QString& status) {
    m_statusLabel->setText(status);
}

} // namespace cub3::ui::pages
