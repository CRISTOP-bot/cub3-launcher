#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QListWidget;
class QLabel;
namespace cub3::application { class Cub3App; }

namespace cub3::ui::pages {

/// Mod discovery for Modrinth, with CurseForge as a linked provider.
class ModsPage final : public QWidget {
    Q_OBJECT
public:
    explicit ModsPage(application::Cub3App* app, QWidget* parent = nullptr);

private slots:
    void search();
    void installSelected();
    void openCurseForge();

private:
    void showError(const QString& message);
    application::Cub3App* m_app{};
    QLineEdit* m_query{};
    QLineEdit* m_gameVersion{};
    QComboBox* m_loader{};
    QListWidget* m_results{};
    QLabel* m_status{};
};

} // namespace cub3::ui::pages
