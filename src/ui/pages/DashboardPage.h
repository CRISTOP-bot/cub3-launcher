// CUB3 Launcher - Dashboard page
#pragma once

#include <QWidget>
#include <QPair>
#include <QList>

class QComboBox;
class QLabel;
class QLineEdit;
class QShowEvent;
class QTimer;

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

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void requestVersions();
    void populateVersionSelector();
    void refreshLocalSnapshot();

    application::Cub3App* m_app{nullptr};
    QLineEdit* m_versionSearch{nullptr};
    QComboBox* m_versionSelector{nullptr};
    widgets::PlayButton* m_playButton{nullptr};
    QLabel* m_profileSummary{nullptr};
    QLabel* m_memoryValue{nullptr};
    QLabel* m_threadValue{nullptr};
    QLabel* m_accountLabel{nullptr};
    QLabel* m_statusLabel{nullptr};
    QTimer* m_metricsTimer{nullptr};
    QList<QPair<QString, QString>> m_versions;
    bool m_versionsRequested{false};
};

} // namespace cub3::ui::pages
