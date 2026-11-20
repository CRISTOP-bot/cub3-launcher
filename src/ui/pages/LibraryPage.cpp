// CUB3 Launcher - Library page implementation
#include "LibraryPage.h"
#include "application/Cub3App.h"
#include "storage/Database.h"
#include "styles/Theme.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace cub3::ui::pages {

namespace {
QString loaderName(const QString& loader) {
    const QString normalized = loader.trimmed();
    return normalized.isEmpty() || normalized.compare(QStringLiteral("vanilla"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("Vanilla") : normalized;
}

QString installationSummary(const InstallationRecord& installation) {
    return QStringLiteral("Minecraft %1 · %2")
        .arg(installation.version, loaderName(installation.loader));
}
} // namespace

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

    auto* subtitle = new QLabel(
        QStringLiteral("Tus instalaciones locales de Minecraft, ordenadas a tu manera."), this);
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    auto* filterCard = new QWidget(this);
    filterCard->setObjectName(QStringLiteral("LibraryFilterCard"));
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(14, 12, 14, 12);
    filterLayout->setSpacing(theme::sizes::kSpacingSmall);

    m_searchField = new QLineEdit(filterCard);
    m_searchField->setObjectName(QStringLiteral("LibrarySearchField"));
    m_searchField->setPlaceholderText(QStringLiteral("Buscar nombre, versión o cargador…"));
    m_searchField->setAccessibleName(QStringLiteral("Buscar instalaciones por nombre, versión o cargador"));
    m_searchField->setClearButtonEnabled(true);
    filterLayout->addWidget(m_searchField, 1);

    auto* sortLabel = new QLabel(QStringLiteral("Ordenar"), filterCard);
    sortLabel->setObjectName(QStringLiteral("Secondary"));
    filterLayout->addWidget(sortLabel);

    m_sortSelector = new QComboBox(filterCard);
    m_sortSelector->setObjectName(QStringLiteral("LibrarySortSelector"));
    m_sortSelector->setAccessibleName(QStringLiteral("Ordenar instalaciones"));
    m_sortSelector->addItem(QStringLiteral("Recientes"), static_cast<int>(InstallationSort::RecentlyAdded));
    m_sortSelector->addItem(QStringLiteral("Nombre"), static_cast<int>(InstallationSort::Name));
    m_sortSelector->addItem(QStringLiteral("Versión"), static_cast<int>(InstallationSort::Version));
    filterLayout->addWidget(m_sortSelector);
    layout->addWidget(filterCard);

    m_countLabel = new QLabel(QStringLiteral("Cargando perfiles…"), this);
    m_countLabel->setObjectName(QStringLiteral("PageSubtitle"));
    m_countLabel->setAccessibleName(QStringLiteral("Cantidad de instalaciones visibles"));
    layout->addWidget(m_countLabel);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("InstanceList"));
    m_list->setAccessibleName(QStringLiteral("Instalaciones locales de Minecraft"));
    m_list->setAlternatingRowColors(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list, 1);

    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(theme::sizes::kSpacingSmall);
    actionRow->addStretch();

    auto* openFolderButton = new QPushButton(QStringLiteral("Abrir carpeta"), this);
    openFolderButton->setObjectName(QStringLiteral("SecondaryButton"));
    openFolderButton->setAccessibleName(QStringLiteral("Abrir la carpeta de la instalación seleccionada"));
    openFolderButton->setEnabled(false);
    openFolderButton->setToolTip(QStringLiteral("Selecciona una instalación para abrir su carpeta"));
    actionRow->addWidget(openFolderButton);

    auto* refreshButton = new QPushButton(QStringLiteral("Actualizar"), this);
    refreshButton->setAccessibleName(QStringLiteral("Actualizar la lista de perfiles"));
    refreshButton->setToolTip(QStringLiteral("Volver a leer las instalaciones locales"));
    connect(refreshButton, &QPushButton::clicked, this, &LibraryPage::refreshList);
    actionRow->addWidget(refreshButton);

    auto* newButton = new QPushButton(QStringLiteral("Nuevo perfil"), this);
    newButton->setObjectName(QStringLiteral("Primary"));
    newButton->setEnabled(false);
    newButton->setToolTip(QStringLiteral("La creación de perfiles aún no está disponible."));
    actionRow->addWidget(newButton);
    layout->addLayout(actionRow);

    connect(m_searchField, &QLineEdit::textChanged, this, &LibraryPage::renderList);
    connect(m_sortSelector, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &LibraryPage::renderList);
    connect(m_list, &QListWidget::itemSelectionChanged, this,
            [this, openFolderButton] {
                const auto* item = m_list->currentItem();
                const bool hasFolder = item
                    && QFileInfo(item->data(Qt::UserRole + 1).toString()).isDir();
                openFolderButton->setEnabled(hasFolder);
            });
    connect(openFolderButton, &QPushButton::clicked, this, [this] {
        const auto* item = m_list->currentItem();
        if (!item)
            return;
        const QString directory = item->data(Qt::UserRole + 1).toString();
        const QFileInfo info(directory);
        if (!directory.isEmpty() && info.isDir())
            QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
    });
}

void LibraryPage::refreshList() {
    m_installations.clear();
    m_loadError.clear();
    if (!m_app) {
        m_loadError = QStringLiteral("No hay conexión con la aplicación.");
        renderList();
        return;
    }

    QSqlQuery query = m_app->database().exec(
        QStringLiteral("SELECT id, name, version, loader, directory, created_at "
                       "FROM installations ORDER BY created_at DESC, id DESC"));
    if (query.lastError().isValid()) {
        m_loadError = QStringLiteral("No se pudieron cargar los perfiles: %1")
                          .arg(query.lastError().text());
        renderList();
        return;
    }

    while (query.next()) {
        InstallationRecord installation;
        installation.id = query.value(0).toLongLong();
        installation.name = query.value(1).toString();
        installation.version = query.value(2).toString();
        installation.loader = query.value(3).toString();
        installation.directory = query.value(4).toString();
        installation.createdAt = query.value(5).toLongLong();
        m_installations.append(std::move(installation));
    }
    renderList();
}

void LibraryPage::renderList() {
    m_list->clear();
    if (!m_loadError.isEmpty()) {
        m_countLabel->setText(QStringLiteral("No se pudieron cargar las instalaciones."));
        auto* item = new QListWidgetItem(m_loadError, m_list);
        item->setFlags(Qt::NoItemFlags);
        return;
    }

    const auto sort = static_cast<InstallationSort>(m_sortSelector->currentData().toInt());
    const auto visible = filterAndSortInstallations(m_installations,
                                                     m_searchField->text(), sort);
    if (m_installations.isEmpty()) {
        m_countLabel->setText(QStringLiteral("Sin instalaciones"));
    } else if (visible.size() == m_installations.size()) {
        m_countLabel->setText(m_installations.size() == 1
            ? QStringLiteral("1 instalación")
            : QStringLiteral("%1 instalaciones").arg(m_installations.size()));
    } else {
        m_countLabel->setText(QStringLiteral("%1 de %2 instalaciones")
                                  .arg(visible.size()).arg(m_installations.size()));
    }

    for (const auto& installation : visible) {
        const QString displayName = installation.name.trimmed().isEmpty()
            ? QStringLiteral("Instalación sin nombre") : installation.name.trimmed();
        auto* item = new QListWidgetItem(
            QStringLiteral("%1   ·   %2").arg(displayName, installationSummary(installation)), m_list);
        item->setData(Qt::UserRole, installation.id);
        item->setData(Qt::UserRole + 1, installation.directory);
        item->setToolTip(QStringLiteral("%1\n%2\n%3")
                             .arg(displayName, installationSummary(installation),
                                  installation.directory));
        item->setSizeHint(QSize(0, 52));
    }

    if (visible.isEmpty()) {
        const QString emptyText = m_installations.isEmpty()
            ? QStringLiteral("Aún no tienes instalaciones. El botón para crear perfiles estará disponible cuando esa función esté implementada.")
            : QStringLiteral("No encontramos instalaciones que coincidan. Prueba con otro nombre, versión o cargador.");
        auto* item = new QListWidgetItem(emptyText, m_list);
        item->setFlags(Qt::NoItemFlags);
        item->setToolTip(emptyText);
        item->setSizeHint(QSize(0, 76));
    }
}

} // namespace cub3::ui::pages
