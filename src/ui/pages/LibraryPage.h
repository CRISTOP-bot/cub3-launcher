// CUB3 Launcher - Library page
#pragma once

#include "LibraryModel.h"

#include <QWidget>
#include <QListWidget>

class QLabel;
class QLineEdit;
class QComboBox;
namespace cub3::application { class Cub3App; }

namespace cub3::ui::pages {

/// Lists and filters installed Minecraft instances without modifying them.
class LibraryPage : public QWidget {
    Q_OBJECT

public:
    explicit LibraryPage(application::Cub3App* app, QWidget* parent = nullptr);

signals:
    void instanceSelected(const QString& id);

private:
    void setupUi();
    void refreshList();
    void renderList();

    application::Cub3App* m_app{nullptr};
    QListWidget* m_list{nullptr};
    QLabel* m_countLabel{nullptr};
    QLineEdit* m_searchField{nullptr};
    QComboBox* m_sortSelector{nullptr};
    QList<InstallationRecord> m_installations;
    QString m_loadError;
};

} // namespace cub3::ui::pages
