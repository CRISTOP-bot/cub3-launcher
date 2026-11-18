// CUB3 Launcher - Dashboard page
#pragma once

#include <QWidget>
#include <QPair>
#include <QList>
#include <QJsonArray>
#include <QByteArray>

class QComboBox;
class QLabel;
class QLineEdit;
class QToolButton;
class QShowEvent;
class QTimer;
class QPushButton;
class QUrl;

namespace cub3::application { class Cub3App; }
namespace cub3::ui::widgets { class PlayButton; }

namespace cub3::ui::pages {

/// Landing page with version search, launch controls, profiles, and live metrics.
class DashboardPage : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPage(application::Cub3App* app, QWidget* parent = nullptr);

    void setApp(application::Cub3App* app);
    void setPlayerName(const QString& name);
    void setVersion(const QString& version);
    void setStatus(const QString& status);

signals:
    void playRequested(const QString& versionId);
    void profilesRequested();
    void accountRequested();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void requestVersions();
    void populateVersionSelector();
    void refreshLocalSnapshot();
    void loadCachedNews();
    void applyNewsData(const QByteArray& json);
    void requestNews();
    void showNewsItem();
    void requestNewsImage(const QUrl& imageUrl);

    application::Cub3App* m_app{nullptr};
    QLineEdit* m_versionSearch{nullptr};
    QComboBox* m_versionSelector{nullptr};
    widgets::PlayButton* m_playButton{nullptr};
    QLabel* m_profileSummary{nullptr};
    QLabel* m_profileDetail{nullptr};
    QLabel* m_memoryValue{nullptr};
    QLabel* m_threadValue{nullptr};
    QLabel* m_javaValue{nullptr};
    QWidget* m_memoryChart{nullptr};
    QWidget* m_newsArtwork{nullptr};
    QLabel* m_newsTitle{nullptr};
    QLabel* m_newsCaption{nullptr};
    QLabel* m_newsDate{nullptr};
    QLabel* m_newsCounter{nullptr};
    QPushButton* m_newsOpenButton{nullptr};
    QToolButton* m_newsPrevious{nullptr};
    QToolButton* m_newsNext{nullptr};
    QToolButton* m_accountButton{nullptr};
    QLabel* m_statusLabel{nullptr};
    QTimer* m_metricsTimer{nullptr};
    QTimer* m_newsTimer{nullptr};
    QJsonArray m_newsEntries;
    int m_newsIndex{0};
    QList<QPair<QString, QString>> m_versions;
    bool m_versionsRequested{false};
    bool m_newsRequested{false};
};

} // namespace cub3::ui::pages
