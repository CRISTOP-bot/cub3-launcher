// CUB3 Launcher - Library page implementation
#include "LibraryPage.h"
#include "application/Cub3App.h"
#include "storage/Database.h"
#include "styles/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>

namespace cub3::ui::pages {

LibraryPage::LibraryPage(application::Cub3App* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    setupUi();
    refreshList();
}

void LibraryPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge);
    layout->setSpacing(theme::sizes::kSpacingMedium);

    auto* titleLabel = new QLabel(QStringLiteral("Perfiles"), this);
    titleLabel->setObjectName(QStringLiteral("PageTitle"));
    layout->addWidget(titleLabel);

    m_countLabel = new QLabel(QStringLiteral("Cargando perfiles..."), this);
    m_countLabel->setObjectName(QStringLiteral("PageSubtitle"));
    layout->addWidget(m_countLabel);

    // Instance list
    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("InstanceList"));
    layout->addWidget(m_list, 1);

    // Action row
    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(theme::sizes::kSpacingSmall);
    actionRow->addStretch();

    auto* newBtn = new QPushButton(QStringLiteral("Nuevo perfil"), this);
    newBtn->setObjectName(QStringLiteral("Primary"));
    newBtn->setEnabled(false);
    newBtn->setToolTip(QStringLiteral("La creación de perfiles aún no está disponible."));
    actionRow->addWidget(newBtn);

    auto* refreshBtn = new QPushButton(QStringLiteral("Actualizar"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &LibraryPage::refreshList);
    actionRow->addWidget(refreshBtn);

    layout->addLayout(actionRow);
}

void LibraryPage::refreshList() {
    m_list->clear();
    if (!m_app) {
        m_countLabel->setText(QStringLiteral("No hay conexión con la aplicación."));
        return;
    }

    QSqlQuery query = m_app->database().exec(
        QStringLiteral("SELECT id, name, version, loader FROM installations ORDER BY created_at DESC"));
    if (query.lastError().isValid()) {
        m_countLabel->setText(QStringLiteral("No se pudieron cargar los perfiles."));
        return;
    }

    int count = 0;
    while (query.next()) {
        const QString name = query.value(1).toString();
        const QString version = query.value(2).toString();
        const QString loader = query.value(3).toString();
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  ·  %2  ·  %3").arg(name, version, loader), m_list);
        item->setData(Qt::UserRole, query.value(0));
        ++count;
    }

    m_countLabel->setText(count == 1
        ? QStringLiteral("1 perfil instalado")
        : QStringLiteral("%1 perfiles instalados").arg(count));
    if (count == 0) {
        auto* emptyItem = new QListWidgetItem(
            QStringLiteral("Aún no tienes perfiles instalados."), m_list);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEnabled);
    }
}

} // namespace cub3::ui::pages
