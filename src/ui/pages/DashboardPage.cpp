// CUB3 Launcher - Dashboard page implementation
#include "DashboardPage.h"

#include "application/Cub3App.h"
#include "core/Performance.h"
#include "core/Paths.h"
#include "network/HttpClient.h"
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
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDate>
#include <QLocale>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>
#include <QPointer>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSizePolicy>

#include <algorithm>
#include <exception>

namespace cub3::ui::pages {
namespace {

class MemorySparkline final : public QWidget {
public:
    explicit MemorySparkline(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(42);
        setMaximumHeight(48);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void addSample(int value) {
        m_samples.append(value);
        if (m_samples.size() > 32)
            m_samples.removeFirst();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF area = QRectF(rect()).adjusted(1, 2, -1, -2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(19, 30, 39, 180));
        painter.drawRoundedRect(area, 7, 7);

        if (m_samples.isEmpty())
            return;

        int low = m_samples.first();
        int high = low;
        for (const int sample : m_samples) {
            low = qMin(low, sample);
            high = qMax(high, sample);
        }
        const qreal span = qMax(16, high - low);
        QPainterPath line;
        for (qsizetype i = 0; i < m_samples.size(); ++i) {
            const qreal denominator = static_cast<qreal>(
                qMax<qsizetype>(1, m_samples.size() - 1));
            const qreal x = area.left() + area.width()
                * static_cast<qreal>(i) / denominator;
            const qreal normalized = (m_samples.at(i) - low) / span;
            const qreal y = area.bottom() - 7 - normalized * (area.height() - 15);
            if (i == 0)
                line.moveTo(x, y);
            else
                line.lineTo(x, y);
        }

        QPainterPath fill = line;
        fill.lineTo(area.right(), area.bottom());
        fill.lineTo(area.left(), area.bottom());
        fill.closeSubpath();
        QLinearGradient gradient(0, area.top(), 0, area.bottom());
        gradient.setColorAt(0.0, QColor(53, 185, 85, 85));
        gradient.setColorAt(1.0, QColor(53, 185, 85, 5));
        painter.fillPath(fill, gradient);

        QPen pen(QColor(67, 213, 101), 2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(line);
    }

private:
    QList<int> m_samples;
};

class BlockTitleLabel final : public QLabel {
public:
    explicit BlockTitleLabel(QWidget* parent = nullptr)
        : QLabel(parent) {}

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QFontMetrics metrics(font());
        QPainterPath textPath;
        textPath.addText(QPointF(2, metrics.ascent() + 2), font(), text());

        QPainterPathStroker stroker;
        stroker.setWidth(4.5);
        stroker.setJoinStyle(Qt::BevelJoin);
        const QPainterPath outline = stroker.createStroke(textPath);
        painter.fillPath(outline.translated(0, 5), QColor(9, 13, 19, 230));
        painter.fillPath(outline, QColor(20, 24, 31));

        QLinearGradient face(0, 0, 0, height());
        face.setColorAt(0.0, QColor(255, 255, 255));
        face.setColorAt(1.0, QColor(200, 208, 218));
        painter.fillPath(textPath, face);
    }
};

class SunsetHero final : public QWidget {
public:
    explicit SunsetHero(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("SunsetHero"));
        setMinimumHeight(360);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_image.load(QStringLiteral(":/images/cherry-grove.png"));
    }

    void setImage(const QPixmap& image) {
        m_image = image;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF bounds = rect();
        QPainterPath clip;
        clip.addRoundedRect(bounds, 16, 16);
        painter.setClipPath(clip);

        if (!m_image.isNull()) {
            const qreal targetAspect = width() / qreal(qMax(1, height()));
            const qreal imageAspect = m_image.width() / qreal(qMax(1, m_image.height()));
            QRectF source(0, 0, m_image.width(), m_image.height());
            if (imageAspect > targetAspect) {
                source.setWidth(m_image.height() * targetAspect);
                source.moveLeft((m_image.width() - source.width()) / 2.0);
            } else {
                source.setHeight(m_image.width() / targetAspect);
                source.moveTop((m_image.height() - source.height()) / 2.0);
            }
            painter.drawPixmap(rect(), m_image, source);
            QLinearGradient shade(0, 0, width() * 0.8, 0);
            shade.setColorAt(0.0, QColor(12, 17, 28, 205));
            shade.setColorAt(0.5, QColor(12, 17, 28, 105));
            shade.setColorAt(1.0, QColor(12, 17, 28, 5));
            painter.fillRect(bounds, shade);
            QLinearGradient bottom(0, height() * 0.6, 0, height());
            bottom.setColorAt(0.0, QColor(12, 17, 28, 0));
            bottom.setColorAt(1.0, QColor(12, 17, 28, 155));
            painter.fillRect(bounds, bottom);
            return;
        }

        QLinearGradient sky(0, 0, 0, height());
        sky.setColorAt(0.0, QColor(49, 48, 82));
        sky.setColorAt(0.42, QColor(125, 75, 105));
        sky.setColorAt(0.76, QColor(232, 123, 99));
        sky.setColorAt(1.0, QColor(255, 177, 108));
        painter.fillRect(bounds, sky);

        painter.setPen(Qt::NoPen);
        const auto px = [this](qreal x, qreal y) {
            return QPointF(width() * x, height() * y);
        };
        const auto polygon = [this, &painter, &px](const QList<QPointF>& points,
                                                   const QColor& color) {
            QPolygonF shape;
            for (const QPointF& point : points)
                shape << px(point.x(), point.y());
            painter.setBrush(color);
            painter.drawPolygon(shape);
        };

        const auto cloud = [this, &painter](qreal x, qreal y, qreal scale,
                                             const QColor& color) {
            painter.setBrush(color);
            const qreal unit = height() * scale;
            painter.drawRect(QRectF(width() * x, height() * y, unit * 3, unit));
            painter.drawRect(QRectF(width() * x + unit, height() * y - unit,
                                    unit * 3, unit));
            painter.drawRect(QRectF(width() * x + unit * 3, height() * y,
                                    unit * 2, unit));
            painter.drawRect(QRectF(width() * x + unit * 5, height() * y + unit,
                                    unit * 2, unit));
        };
        cloud(0.41, 0.20, 0.045, QColor(255, 177, 161, 70));
        cloud(0.58, 0.30, 0.035, QColor(255, 190, 166, 58));
        cloud(0.77, 0.18, 0.040, QColor(255, 184, 159, 72));
        cloud(0.91, 0.34, 0.028, QColor(255, 209, 163, 70));
        cloud(0.23, 0.31, 0.026, QColor(255, 191, 173, 45));
        cloud(0.66, 0.12, 0.023, QColor(255, 199, 176, 52));

        const QPointF sunCenter = px(0.79, 0.53);
        const qreal sunRadius = height() * 0.34;
        QRadialGradient sun(sunCenter, sunRadius);
        sun.setColorAt(0.0, QColor(255, 234, 178, 205));
        sun.setColorAt(0.45, QColor(255, 195, 139, 120));
        sun.setColorAt(1.0, QColor(255, 174, 127, 0));
        painter.setBrush(sun);
        painter.drawEllipse(sunCenter, sunRadius, sunRadius);

        const int sunSize = qMax(52, height() / 5);
        const QRectF sunRect(sunCenter.x() - sunSize / 2,
                             sunCenter.y() - sunSize / 2,
                             sunSize, sunSize);
        painter.setBrush(QColor(255, 226, 164));
        painter.drawRect(sunRect);
        painter.setBrush(QColor(255, 239, 191, 170));
        painter.drawRect(QRectF(sunRect.left(), sunRect.top(),
                                sunRect.width(), sunRect.height() * 0.22));

        polygon({{0.0, 0.70}, {0.10, 0.63}, {0.19, 0.66}, {0.31, 0.51},
                 {0.42, 0.64}, {0.54, 0.55}, {0.67, 0.70}, {0.80, 0.59},
                 {0.91, 0.65}, {1.0, 0.56}, {1.0, 1.0}, {0.0, 1.0}},
                QColor(93, 73, 91));
        polygon({{0.0, 0.79}, {0.13, 0.70}, {0.25, 0.77}, {0.39, 0.67},
                 {0.51, 0.78}, {0.65, 0.68}, {0.78, 0.75}, {0.90, 0.64},
                 {1.0, 0.70}, {1.0, 1.0}, {0.0, 1.0}},
                QColor(55, 65, 72));

        polygon({{0.49, 0.87}, {0.58, 0.79}, {0.65, 0.76}, {0.70, 0.67},
                 {0.75, 0.64}, {0.78, 0.55}, {0.83, 0.51}, {0.87, 0.42},
                 {0.92, 0.45}, {0.96, 0.38}, {1.0, 0.40}, {1.0, 1.0},
                 {0.49, 1.0}},
                QColor(51, 79, 64));
        polygon({{0.71, 0.68}, {0.78, 0.59}, {0.83, 0.54}, {0.87, 0.45},
                 {0.92, 0.48}, {0.96, 0.41}, {1.0, 0.43}, {1.0, 0.95},
                 {0.90, 0.91}, {0.83, 0.95}, {0.76, 0.86}},
                QColor(76, 63, 56));

        painter.setBrush(QColor(101, 105, 67));
        painter.drawRect(QRectF(width() * 0.78, height() * 0.59,
                                width() * 0.05, height() * 0.025));
        painter.drawRect(QRectF(width() * 0.87, height() * 0.49,
                                width() * 0.07, height() * 0.025));
        painter.drawRect(QRectF(width() * 0.94, height() * 0.44,
                                width() * 0.06, height() * 0.025));
        painter.setBrush(QColor(101, 76, 59));
        painter.drawRect(QRectF(width() * 0.75, height() * 0.69,
                                width() * 0.045, height() * 0.045));
        painter.drawRect(QRectF(width() * 0.84, height() * 0.62,
                                width() * 0.05, height() * 0.04));
        painter.drawRect(QRectF(width() * 0.92, height() * 0.56,
                                width() * 0.06, height() * 0.045));
        painter.drawRect(QRectF(width() * 0.80, height() * 0.81,
                                width() * 0.055, height() * 0.05));
        painter.drawRect(QRectF(width() * 0.91, height() * 0.75,
                                width() * 0.05, height() * 0.045));

        // A small block-built castle gives the distant ridge a clear landmark.
        painter.setBrush(QColor(92, 82, 91));
        painter.drawRect(QRectF(width() * 0.875, height() * 0.32,
                                width() * 0.035, height() * 0.18));
        painter.drawRect(QRectF(width() * 0.93, height() * 0.27,
                                width() * 0.045, height() * 0.24));
        painter.drawRect(QRectF(width() * 0.865, height() * 0.38,
                                width() * 0.14, height() * 0.10));
        painter.setBrush(QColor(117, 106, 112));
        painter.drawRect(QRectF(width() * 0.868, height() * 0.29,
                                width() * 0.05, height() * 0.045));
        painter.drawRect(QRectF(width() * 0.925, height() * 0.23,
                                width() * 0.055, height() * 0.05));
        painter.drawRect(QRectF(width() * 0.856, height() * 0.36,
                                width() * 0.025, height() * 0.035));
        painter.drawRect(QRectF(width() * 0.89, height() * 0.36,
                                width() * 0.025, height() * 0.035));
        painter.drawRect(QRectF(width() * 0.952, height() * 0.31,
                                width() * 0.02, height() * 0.035));
        painter.setBrush(QColor(32, 39, 46));
        painter.drawRect(QRectF(width() * 0.891, height() * 0.41,
                                width() * 0.012, height() * 0.035));
        painter.drawRect(QRectF(width() * 0.948, height() * 0.35,
                                width() * 0.014, height() * 0.04));

        const auto tree = [this, &painter](qreal x, qreal y, qreal scale) {
            const qreal unit = height() * scale;
            painter.setBrush(QColor(35, 44, 39));
            painter.drawRect(QRectF(width() * x - unit * 0.18,
                                    height() * y + unit * 1.1,
                                    unit * 0.36, unit * 1.6));
            painter.setBrush(QColor(35, 67, 53));
            painter.drawRect(QRectF(width() * x - unit, height() * y + unit * 0.4,
                                    unit * 2, unit * 0.9));
            painter.drawRect(QRectF(width() * x - unit * 0.72, height() * y - unit * 0.2,
                                    unit * 1.44, unit * 0.75));
            painter.setBrush(QColor(52, 91, 61));
            painter.drawRect(QRectF(width() * x - unit * 0.48, height() * y - unit * 0.65,
                                    unit * 0.96, unit * 0.72));
        };
        tree(0.72, 0.60, 0.048);
        tree(0.98, 0.58, 0.050);
        tree(0.10, 0.72, 0.025);
        tree(0.22, 0.75, 0.032);
        tree(0.36, 0.72, 0.027);
        tree(0.48, 0.76, 0.030);
        tree(0.61, 0.72, 0.025);

        polygon({{0.0, 0.85}, {0.11, 0.80}, {0.18, 0.84}, {0.28, 0.78},
                 {0.40, 0.85}, {0.51, 0.79}, {0.62, 0.86}, {0.72, 0.81},
                 {0.83, 0.87}, {0.92, 0.83}, {1.0, 0.86}, {1.0, 1.0},
                 {0.0, 1.0}},
                QColor(30, 49, 48));

        polygon({{0.38, 0.88}, {0.47, 0.84}, {0.56, 0.86}, {0.64, 0.82},
                 {0.71, 0.85}, {0.76, 0.89}, {0.73, 1.0}, {0.39, 1.0}},
                QColor(29, 70, 76, 210));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 185, 133, 125));
        painter.drawRect(QRectF(width() * 0.63, height() * 0.89,
                                width() * 0.09, height() * 0.012));
        painter.setBrush(QColor(125, 209, 194, 110));
        painter.drawRect(QRectF(width() * 0.49, height() * 0.93,
                                width() * 0.14, height() * 0.009));
        painter.drawRect(QRectF(width() * 0.57, height() * 0.97,
                                width() * 0.11, height() * 0.008));

        QLinearGradient shade(0, 0, width() * 0.78, 0);
        shade.setColorAt(0.0, QColor(17, 22, 35, 210));
        shade.setColorAt(0.48, QColor(17, 22, 35, 115));
        shade.setColorAt(1.0, QColor(17, 22, 35, 0));
        painter.fillRect(bounds, shade);
        QLinearGradient bottom(0, height() * 0.65, 0, height());
        bottom.setColorAt(0.0, QColor(17, 22, 35, 0));
        bottom.setColorAt(1.0, QColor(17, 22, 35, 145));
        painter.fillRect(bounds, bottom);
    }

private:
    QPixmap m_image;
};

class NewsArtwork final : public QFrame {
public:
    explicit NewsArtwork(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setObjectName(QStringLiteral("FeatureArt"));
        setMinimumHeight(104);
    }

    void setImage(const QPixmap& image) {
        m_image = image;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath clip;
        clip.addRoundedRect(rect(), 12, 12);
        painter.setClipPath(clip);
        painter.fillRect(rect(), QColor(20, 29, 44));
        if (m_image.isNull())
            return;

        const qreal targetAspect = width() / qreal(qMax(1, height()));
        const qreal imageAspect = m_image.width() / qreal(qMax(1, m_image.height()));
        QRectF source(0, 0, m_image.width(), m_image.height());
        if (imageAspect > targetAspect) {
            source.setWidth(m_image.height() * targetAspect);
            source.moveLeft((m_image.width() - source.width()) / 2.0);
        } else {
            source.setHeight(m_image.width() / targetAspect);
            source.moveTop((m_image.height() - source.height()) / 2.0);
        }
        painter.drawPixmap(rect(), m_image, source);
    }

private:
    QPixmap m_image;
};

QFrame* makeCard(QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("InfoCard"));
    card->setMinimumHeight(232);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return card;
}

QWidget* makeCardHeader(const QString& text, const QString& icon,
                        const QString& iconStyle, QWidget* parent) {
    auto* header = new QWidget(parent);
    header->setObjectName(QStringLiteral("CardHeader"));
    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* iconLabel = new QLabel(icon, header);
    iconLabel->setObjectName(iconStyle);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedWidth(20);
    layout->addWidget(iconLabel);

    auto* title = new QLabel(text, header);
    title->setObjectName(QStringLiteral("CardTitle"));
    layout->addWidget(title);
    layout->addStretch(1);
    return header;
}

} // namespace

DashboardPage::DashboardPage(application::Cub3App* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    setupUi();
    loadCachedNews();

    m_metricsTimer = new QTimer(this);
    m_metricsTimer->setInterval(5000);
    connect(m_metricsTimer, &QTimer::timeout, this, &DashboardPage::refreshLocalSnapshot);
    m_metricsTimer->start();
    refreshLocalSnapshot();

    m_newsTimer = new QTimer(this);
    m_newsTimer->setInterval(60 * 60 * 1000);
    connect(m_newsTimer, &QTimer::timeout, this, &DashboardPage::requestNews);
    m_newsTimer->start();
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
    m_versionSearch->setMaximumWidth(500);
    topBar->addWidget(m_versionSearch, 1);
    topBar->addStretch(1);

    m_accountButton = new QToolButton(hero);
    m_accountButton->setObjectName(QStringLiteral("AccountBadge"));
    m_accountButton->setText(QStringLiteral("INVITADO  ▾"));
    m_accountButton->setCursor(Qt::PointingHandCursor);
    connect(m_accountButton, &QToolButton::clicked,
            this, &DashboardPage::accountRequested);
    topBar->addWidget(m_accountButton);
    heroLayout->addLayout(topBar);
    heroLayout->addStretch(1);

    auto* headline = new BlockTitleLabel(hero);
    headline->setText(QStringLiteral("MINECRAFT"));
    headline->setObjectName(QStringLiteral("HeroTitle"));
    headline->setWordWrap(true);
    heroLayout->addWidget(headline);

    auto* description = new QLabel(
        QStringLiteral("Explora un mundo infinito"),
        hero);
    description->setObjectName(QStringLiteral("HeroCaption"));
    description->setWordWrap(true);
    heroLayout->addWidget(description);
    heroLayout->addSpacing(theme::sizes::kSpacingMedium);

    auto* launchRow = new QHBoxLayout();
    launchRow->setSpacing(theme::sizes::kSpacingSmall);
    m_playButton = new widgets::PlayButton(QStringLiteral("▶  Jugar"), hero);
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
    auto* newsHeader = makeCardHeader(QStringLiteral("Novedades"),
                                       QStringLiteral("✦"),
                                       QStringLiteral("NewsIcon"), newsCard);
    auto* newsHeaderLayout = qobject_cast<QHBoxLayout*>(newsHeader->layout());
    m_newsCounter = new QLabel(QStringLiteral("— / —"), newsHeader);
    m_newsCounter->setObjectName(QStringLiteral("FeatureMeta"));
    newsHeaderLayout->addWidget(m_newsCounter);
    m_newsPrevious = new QToolButton(newsHeader);
    m_newsPrevious->setObjectName(QStringLiteral("NewsNavButton"));
    m_newsPrevious->setText(QStringLiteral("‹"));
    m_newsPrevious->setCursor(Qt::PointingHandCursor);
    m_newsNext = new QToolButton(newsHeader);
    m_newsNext->setObjectName(QStringLiteral("NewsNavButton"));
    m_newsNext->setText(QStringLiteral("›"));
    m_newsNext->setCursor(Qt::PointingHandCursor);
    newsHeaderLayout->addWidget(m_newsPrevious);
    newsHeaderLayout->addWidget(m_newsNext);
    connect(m_newsPrevious, &QToolButton::clicked, this, [this] {
        if (m_newsEntries.isEmpty()) return;
        m_newsIndex = (m_newsIndex + m_newsEntries.size() - 1) % m_newsEntries.size();
        showNewsItem();
    });
    connect(m_newsNext, &QToolButton::clicked, this, [this] {
        if (m_newsEntries.isEmpty()) return;
        m_newsIndex = (m_newsIndex + 1) % m_newsEntries.size();
        showNewsItem();
    });
    newsLayout->addWidget(newsHeader);

    auto* featureRow = new QHBoxLayout();
    featureRow->setSpacing(14);
    auto* featureArt = new NewsArtwork(newsCard);
    featureArt->setFixedWidth(136);
    featureArt->setMinimumHeight(144);
    m_newsArtwork = featureArt;
    featureRow->addWidget(featureArt);

    auto* featureCopy = new QVBoxLayout();
    featureCopy->setSpacing(8);
    m_newsTitle = new QLabel(QStringLiteral("Cargando novedades oficiales…"), newsCard);
    m_newsTitle->setObjectName(QStringLiteral("FeatureTitle"));
    m_newsTitle->setWordWrap(true);
    featureCopy->addWidget(m_newsTitle);
    m_newsCaption = new QLabel(QStringLiteral("Conectando con Minecraft.net"), newsCard);
    m_newsCaption->setObjectName(QStringLiteral("CardCaption"));
    m_newsCaption->setWordWrap(true);
    featureCopy->addWidget(m_newsCaption);
    featureCopy->addStretch(1);
    m_newsDate = new QLabel(QString(), newsCard);
    m_newsDate->setObjectName(QStringLiteral("FeatureMeta"));
    featureCopy->addWidget(m_newsDate);
    m_newsOpenButton = new QPushButton(QStringLiteral("Leer artículo ↗"), newsCard);
    m_newsOpenButton->setObjectName(QStringLiteral("NewsLinkButton"));
    m_newsOpenButton->setCursor(Qt::PointingHandCursor);
    m_newsOpenButton->setEnabled(false);
    connect(m_newsOpenButton, &QPushButton::clicked, this, [this] {
        if (m_newsEntries.isEmpty()) return;
        const QUrl url(m_newsEntries.at(m_newsIndex).toObject()
                           .value(QStringLiteral("readMoreLink")).toString());
        if (url.isValid())
            QDesktopServices::openUrl(url);
    });
    featureCopy->addWidget(m_newsOpenButton);
    featureRow->addLayout(featureCopy, 1);
    newsLayout->addLayout(featureRow, 1);
    cardsLayout->addWidget(newsCard, 0, 0);

    auto* profilesCard = makeCard(content);
    auto* profilesLayout = new QVBoxLayout(profilesCard);
    profilesLayout->setContentsMargins(18, 16, 18, 16);
    profilesLayout->setSpacing(10);
    profilesLayout->addWidget(makeCardHeader(QStringLiteral("Tus instancias"),
                                              QStringLiteral("◆"),
                                              QStringLiteral("ProfilesIcon"), profilesCard));
    auto* profileSummaryRow = new QHBoxLayout();
    profileSummaryRow->setSpacing(14);
    m_profileSummary = new QLabel(QStringLiteral("—"), profilesCard);
    m_profileSummary->setObjectName(QStringLiteral("ProfileCount"));
    profileSummaryRow->addWidget(m_profileSummary, 0, Qt::AlignTop);
    m_profileDetail = new QLabel(QStringLiteral("Cargando perfiles locales..."), profilesCard);
    m_profileDetail->setObjectName(QStringLiteral("CardCaption"));
    m_profileDetail->setWordWrap(true);
    profileSummaryRow->addWidget(m_profileDetail, 1, Qt::AlignVCenter);
    profileSummaryRow->addStretch(1);
    profilesLayout->addLayout(profileSummaryRow, 1);
    auto* profilesButton = new QPushButton(QStringLiteral("Ver perfiles"), profilesCard);
    profilesButton->setObjectName(QStringLiteral("SecondaryButton"));
    connect(profilesButton, &QPushButton::clicked, this, &DashboardPage::profilesRequested);
    profilesLayout->addWidget(profilesButton, 0, Qt::AlignLeft);
    cardsLayout->addWidget(profilesCard, 0, 1);

    auto* performanceCard = makeCard(content);
    auto* performanceLayout = new QVBoxLayout(performanceCard);
    performanceLayout->setContentsMargins(18, 16, 18, 16);
    performanceLayout->setSpacing(9);
    performanceLayout->addWidget(makeCardHeader(QStringLiteral("Rendimiento"),
                                                  QStringLiteral("ϟ"),
                                                  QStringLiteral("PerformanceIcon"),
                                                  performanceCard));

    auto* memoryRow = new QHBoxLayout();
    auto* memoryLabel = new QLabel(QStringLiteral("Memoria"), performanceCard);
    memoryLabel->setObjectName(QStringLiteral("CardCaption"));
    m_memoryValue = new QLabel(QStringLiteral("-- MB"), performanceCard);
    m_memoryValue->setObjectName(QStringLiteral("MetricValue"));
    memoryRow->addWidget(memoryLabel);
    memoryRow->addStretch(1);
    memoryRow->addWidget(m_memoryValue);
    performanceLayout->addLayout(memoryRow);
    m_memoryChart = new MemorySparkline(performanceCard);
    performanceLayout->addWidget(m_memoryChart);

    auto* threadsRow = new QHBoxLayout();
    auto* threadsLabel = new QLabel(QStringLiteral("Hilos activos"), performanceCard);
    threadsLabel->setObjectName(QStringLiteral("CardCaption"));
    m_threadValue = new QLabel(QStringLiteral("--"), performanceCard);
    m_threadValue->setObjectName(QStringLiteral("MetricValue"));
    threadsRow->addWidget(threadsLabel);
    threadsRow->addStretch(1);
    threadsRow->addWidget(m_threadValue);
    performanceLayout->addLayout(threadsRow);

    auto* javaRow = new QHBoxLayout();
    auto* javaLabel = new QLabel(QStringLiteral("Java"), performanceCard);
    javaLabel->setObjectName(QStringLiteral("CardCaption"));
    m_javaValue = new QLabel(QStringLiteral("Buscando..."), performanceCard);
    m_javaValue->setObjectName(QStringLiteral("JavaStatus"));
    javaRow->addWidget(javaLabel);
    javaRow->addStretch(1);
    javaRow->addWidget(m_javaValue);
    performanceLayout->addLayout(javaRow);
    performanceLayout->addStretch(1);
    auto* metricsNote = new QLabel(QStringLiteral("Uso del launcher"), performanceCard);
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
    if (!m_versionsRequested) {
        m_versionsRequested = true;
        QTimer::singleShot(0, this, &DashboardPage::requestVersions);
    }
    if (!m_newsRequested) {
        m_newsRequested = true;
        QTimer::singleShot(0, this, &DashboardPage::requestNews);
    }
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

void DashboardPage::loadCachedNews() {
    QFile cacheFile(core::Paths::cacheDir() + QStringLiteral("news/java-news.json"));
    if (cacheFile.open(QIODevice::ReadOnly))
        applyNewsData(cacheFile.readAll());
}

void DashboardPage::applyNewsData(const QByteArray& json) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return;

    const QJsonArray entries = document.object().value(QStringLiteral("entries")).toArray();
    QJsonArray javaEntries;
    for (const QJsonValue& value : entries) {
        const QJsonObject entry = value.toObject();
        const QJsonArray types = entry.value(QStringLiteral("newsType")).toArray();
        const bool forJava = std::any_of(types.cbegin(), types.cend(), [](const QJsonValue& type) {
            return type.toString().compare(QStringLiteral("Java"), Qt::CaseInsensitive) == 0;
        });
        if (forJava)
            javaEntries.append(entry);
        if (javaEntries.size() >= 8)
            break;
    }

    // Some Mojang feed entries omit edition tags. Keep the official feed useful
    // if its metadata changes, while preferring Java-specific items when present.
    if (javaEntries.isEmpty()) {
        for (const QJsonValue& value : entries) {
            javaEntries.append(value);
            if (javaEntries.size() >= 5)
                break;
        }
    }
    if (javaEntries.isEmpty())
        return;

    m_newsEntries = javaEntries;
    m_newsIndex = qBound(0, m_newsIndex, static_cast<int>(m_newsEntries.size()) - 1);
    showNewsItem();
}

void DashboardPage::requestNews() {
    if (!m_app)
        return;

    QPointer<DashboardPage> guard(this);
    m_app->httpClient().get(
        QUrl(QStringLiteral("https://launchercontent.mojang.com/v2/news.json")),
        [guard](const QByteArray& body, int status) {
            if (!guard || status < 200 || status >= 300 || body.isEmpty())
                return;

            QDir().mkpath(core::Paths::cacheDir() + QStringLiteral("news"));
            QFile cacheFile(core::Paths::cacheDir() + QStringLiteral("news/java-news.json"));
            if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                cacheFile.write(body);
                cacheFile.close();
            }
            guard->applyNewsData(body);
        });

}

void DashboardPage::showNewsItem() {
    if (m_newsEntries.isEmpty()) {
        m_newsCounter->setText(QStringLiteral("— / —"));
        m_newsTitle->setText(QStringLiteral("Novedades oficiales"));
        m_newsCaption->setText(QStringLiteral("Conecta a internet para cargar las noticias de Minecraft."));
        m_newsDate->clear();
        m_newsOpenButton->setEnabled(false);
        m_newsPrevious->setEnabled(false);
        m_newsNext->setEnabled(false);
        static_cast<NewsArtwork*>(m_newsArtwork)->setImage(QPixmap());
        return;
    }

    const QJsonObject entry = m_newsEntries.at(m_newsIndex).toObject();
    const int total = static_cast<int>(m_newsEntries.size());
    m_newsCounter->setText(QStringLiteral("%1 / %2").arg(m_newsIndex + 1).arg(total));
    m_newsPrevious->setEnabled(total > 1);
    m_newsNext->setEnabled(total > 1);
    m_newsTitle->setText(entry.value(QStringLiteral("title")).toString());

    QString summary = entry.value(QStringLiteral("text")).toString().simplified();
    if (summary.size() > 112)
        summary = summary.left(109).trimmed() + QStringLiteral("…");
    m_newsCaption->setText(summary);

    const QDate date = QDate::fromString(
        entry.value(QStringLiteral("date")).toString().left(10), Qt::ISODate);
    const QString dateText = date.isValid()
        ? QLocale(QLocale::Spanish).toString(date, QStringLiteral("d MMM yyyy"))
        : QStringLiteral("Minecraft.net");
    const QString category = entry.value(QStringLiteral("category")).toString();
    m_newsDate->setText(category.isEmpty()
        ? dateText
        : QStringLiteral("%1 · %2").arg(category, dateText));

    const QString article = entry.value(QStringLiteral("readMoreLink")).toString();
    m_newsOpenButton->setEnabled(QUrl(article).isValid() && !article.isEmpty());

    const QJsonObject image = entry.value(QStringLiteral("playPageImage")).toObject();
    const QJsonObject newsImage = entry.value(QStringLiteral("newsPageImage")).toObject();
    QString imagePath = image.value(QStringLiteral("url")).toString();
    if (imagePath.isEmpty())
        imagePath = newsImage.value(QStringLiteral("url")).toString();
    const QUrl imageUrl = QUrl(QStringLiteral("https://launchercontent.mojang.com"))
                              .resolved(QUrl(imagePath));
    requestNewsImage(imageUrl);
}

void DashboardPage::requestNewsImage(const QUrl& imageUrl) {
    if (!imageUrl.isValid() || imageUrl.host() != QStringLiteral("launchercontent.mojang.com"))
        return;

    const QString fileName = QFileInfo(imageUrl.path()).fileName();
    if (fileName.isEmpty())
        return;
    const QString imageDirectory = core::Paths::cacheDir() + QStringLiteral("news/images/");
    const QString imagePath = imageDirectory + fileName;
    QPixmap cachedImage(imagePath);
    if (!cachedImage.isNull()) {
        static_cast<NewsArtwork*>(m_newsArtwork)->setImage(cachedImage);
        return;
    }
    static_cast<NewsArtwork*>(m_newsArtwork)->setImage(QPixmap());
    if (!m_app)
        return;

    QPointer<DashboardPage> guard(this);
    m_app->httpClient().get(imageUrl, [guard, imageUrl, imagePath, imageDirectory](
            const QByteArray& body, int status) {
        if (!guard || status < 200 || status >= 300 || body.isEmpty())
            return;
        QPixmap downloadedImage;
        if (!downloadedImage.loadFromData(body))
            return;

        QDir().mkpath(imageDirectory);
        QFile imageFile(imagePath);
        if (imageFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            imageFile.write(body);
            imageFile.close();
        }

        const QJsonObject currentEntry = guard->m_newsEntries
            .at(guard->m_newsIndex).toObject();
        QString currentPath = currentEntry.value(QStringLiteral("playPageImage"))
                                  .toObject().value(QStringLiteral("url")).toString();
        if (currentPath.isEmpty())
            currentPath = currentEntry.value(QStringLiteral("newsPageImage"))
                              .toObject().value(QStringLiteral("url")).toString();
        const QUrl currentUrl = QUrl(QStringLiteral("https://launchercontent.mojang.com"))
                                    .resolved(QUrl(currentPath));
        if (currentUrl == imageUrl)
            static_cast<NewsArtwork*>(guard->m_newsArtwork)->setImage(downloadedImage);
    });
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
    const int memoryMb = core::Perf::rssMB();
    m_memoryValue->setText(QStringLiteral("%1 MB").arg(memoryMb));
    m_threadValue->setText(QString::number(core::Perf::threadCount()));
    static_cast<MemorySparkline*>(m_memoryChart)->addSample(memoryMb);

    const QString javaPath = QStandardPaths::findExecutable(QStringLiteral("java"));
    m_javaValue->setText(javaPath.isEmpty()
        ? QStringLiteral("No instalado")
        : QStringLiteral("Disponible"));
    m_javaValue->setProperty("available", !javaPath.isEmpty());
    m_javaValue->style()->unpolish(m_javaValue);
    m_javaValue->style()->polish(m_javaValue);

    if (!m_app)
        return;
    try {
        const QVariant installationCount =
            m_app->database().queryValue(QStringLiteral("SELECT COUNT(*) FROM installations"));
        if (!installationCount.isValid()) {
            m_profileSummary->setText(QStringLiteral("—"));
            m_profileDetail->setText(QStringLiteral("No se pudieron leer los perfiles locales."));
            return;
        }

        const int count = installationCount.toInt();
        m_profileSummary->setText(QString::number(count));
        m_profileDetail->setText(count == 0
            ? QStringLiteral("Aún no tienes perfiles instalados.")
            : QStringLiteral("Perfil%1 listo%2 para jugar.")
                  .arg(count == 1 ? QString() : QStringLiteral("es"))
                  .arg(count == 1 ? QString() : QStringLiteral("s")));
    } catch (const std::exception&) {
        m_profileSummary->setText(QStringLiteral("—"));
        m_profileDetail->setText(QStringLiteral("No se pudieron leer los perfiles locales."));
    }
}

void DashboardPage::setApp(application::Cub3App* app) {
    m_app = app;
}

void DashboardPage::setPlayerName(const QString& name) {
    m_accountButton->setText(name.trimmed().isEmpty()
        ? QStringLiteral("INVITADO  ▾")
        : name.trimmed().toUpper() + QStringLiteral("  ▾"));
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
