// CUB3 Launcher - Main window
#pragma once

#include <QMainWindow>
#include <QStackedWidget>

namespace cub3::application {
class Cub3App;
class LaunchEngine;
}
namespace cub3::auth { class AuthManager; }

namespace cub3::ui::widgets {
class SidebarButton;
class PlayButton;
}

namespace cub3::ui::pages {
class DashboardPage;
class LibraryPage;
class DownloadsPage;
class SettingsPage;
}

namespace cub3::ui::windows {

/// The main application window. Contains a sidebar with navigation buttons
/// and a stacked widget for pages. Pages are created lazily on first access.
///
/// Layout:
///   +--------+----------------------+
///   | Logo    |                      |
///   | Nav    |    Content Area       |
///   | Btns   |    (StackedWidget)    |
///   |        |                      |
///   | Status |                      |
///   +--------+----------------------+
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    /// Connect the application context. Must be called before show().
    void setApp(application::Cub3App* app);

    [[nodiscard]] application::Cub3App* app() const { return m_app; }

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    /// Emitted once after the window is first shown.
    void dashboardShown();

private slots:
    void onNavChanged(int index);
    void onAccountRequested();
    void onPlayRequested(const QString& versionId);

private:
    void setupUi();

    application::Cub3App* m_app{nullptr};
    application::LaunchEngine* m_launcher{nullptr};
    auth::AuthManager* m_auth{nullptr};

    QWidget* m_centralWidget{nullptr};
    QStackedWidget* m_stack{nullptr};

    // Sidebar
    QWidget* m_sidebar{nullptr};
    widgets::SidebarButton* m_btnDashboard{nullptr};
    widgets::SidebarButton* m_btnLibrary{nullptr};
    widgets::SidebarButton* m_btnDownloads{nullptr};
    widgets::SidebarButton* m_btnSettings{nullptr};

    // Pages (lazily created)
    pages::DashboardPage* m_dashboard{nullptr};
    pages::LibraryPage*   m_library{nullptr};
    pages::DownloadsPage* m_downloads{nullptr};
    pages::SettingsPage*  m_settings{nullptr};

    bool m_firstShow{true};

private slots:
    void onFirstShow();
};

} // namespace cub3::ui::windows
