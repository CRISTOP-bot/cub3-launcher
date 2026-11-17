// CUB3 Launcher - Library page
#pragma once

#include <QWidget>
#include <QListWidget>

class QLabel;
namespace cub3::application { class Cub3App; }

namespace cub3::ui::pages {

/// Lists installed Minecraft instances. Each instance has a name,
/// version, and mod loader. Uses QListWidget with a custom item
/// delegate for efficient rendering (one widget per row is avoided).
class LibraryPage : public QWidget {
    Q_OBJECT

public:
    explicit LibraryPage(application::Cub3App* app, QWidget* parent = nullptr);

signals:
    void instanceSelected(const QString& id);

private:
    void setupUi();
    void refreshList();

    application::Cub3App* m_app{nullptr};
    QListWidget* m_list{nullptr};
    QLabel* m_countLabel{nullptr};
};

} // namespace cub3::ui::pages
