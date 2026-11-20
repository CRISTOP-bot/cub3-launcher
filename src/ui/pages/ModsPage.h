#pragma once

#include <QWidget>
#include <QtGlobal>
#include <QMetaObject>

class QComboBox;
class QLineEdit;
class QListWidget;
class QLabel;
class QPushButton;
class QFrame;
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
    void updateSelectionActions();

private:
    application::Cub3App* m_app{};
    QLineEdit* m_query{};
    QLineEdit* m_gameVersion{};
    QComboBox* m_loader{};
    QComboBox* m_sort{};
    QFrame* m_modrinthBanner{};
    QListWidget* m_results{};
    QLabel* m_status{};
    QPushButton* m_searchButton{};
    QPushButton* m_downloadButton{};
    QPushButton* m_projectButton{};
    quint64 m_searchRevision{0};
    QString m_resultsForVersion;
    QString m_resultsForLoader;
    QString m_resultsForQuery;
    QString m_resultsForSort;
    QMetaObject::Connection m_downloadConnection;
    QMetaObject::Connection m_downloadProgressConnection;
    bool m_downloadInProgress{false};
};

} // namespace cub3::ui::pages
