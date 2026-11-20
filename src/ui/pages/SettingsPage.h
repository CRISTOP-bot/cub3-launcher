// CUB3 Launcher - Settings page
#pragma once

#include <QWidget>

class QComboBox;
class QSpinBox;
class QSlider;
class QLabel;
class QCheckBox;
class QLineEdit;
class QComboBox;
class QTextBrowser;
class QPushButton;

namespace cub3::application { class Cub3App; }

namespace cub3::ui::pages {

/// Settings page with sections for Java, RAM, Directories, Network,
/// Appearance, and Performance. Each section is a QGroupBox to keep
/// the layout compact.
class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(cub3::application::Cub3App* app,
                          QWidget* parent = nullptr);

    /// Populate from settings (called on first show).
    void loadSettings();

private slots:
    void onJavaBrowse();
    void onDirBrowse();
    void onSave();
    void onGitLabConnect();
    void onGitLabRefreshProjects();
    void onGitLabProjectChanged(int index);

private:
    void setupUi();

    cub3::application::Cub3App* m_app{nullptr};

    // Java
    QLineEdit* m_javaPath{nullptr};

    // RAM
    QSlider* m_ramMinSlider{nullptr};
    QSlider* m_ramMaxSlider{nullptr};
    QLabel* m_ramMinLabel{nullptr};
    QLabel* m_ramMaxLabel{nullptr};
    QLabel* m_saveStatus{nullptr};

    // Directories
    QLineEdit* m_instancesDir{nullptr};

    // Network
    QSpinBox* m_concurrencySpin{nullptr};
    QCheckBox* m_validateDownloadsCheck{nullptr};

    // Resolution
    QSpinBox* m_resolutionW{nullptr};
    QSpinBox* m_resolutionH{nullptr};

    QLineEdit* m_gitLabInstance{nullptr};
    QLineEdit* m_gitLabClientId{nullptr};
    QLabel* m_gitLabStatus{nullptr};
    QComboBox* m_gitLabProjects{nullptr};
    QTextBrowser* m_gitLabDetails{nullptr};
    QPushButton* m_gitLabConnectButton{nullptr};
    QPushButton* m_gitLabRefreshButton{nullptr};
    QCheckBox* m_cppEditionTitle{nullptr};
    QCheckBox* m_discordEnabled{nullptr};
    QLineEdit* m_discordApplicationId{nullptr};
    QLabel* m_discordStatus{nullptr};
};

} // namespace cub3::ui::pages
