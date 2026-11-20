#include "ModsPage.h"
#include "ModsValidation.h"

#include "application/Cub3App.h"
#include "core/Paths.h"
#include "storage/Settings.h"
#include "network/HttpClient.h"
#include "network/DownloadEngine.h"
#include "styles/Theme.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QPointer>
#include <QUuid>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QLocale>
#include <QAbstractItemView>
#include <memory>

namespace cub3::ui::pages {

ModsPage::ModsPage(application::Cub3App* app, QWidget* parent)
    : QWidget(parent), m_app(app) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(theme::sizes::kSpacingLarge, theme::sizes::kSpacingLarge,
                               theme::sizes::kSpacingLarge, theme::sizes::kSpacingLarge);
    layout->setSpacing(theme::sizes::kSpacingMedium);

    auto* heading = new QHBoxLayout();
    auto* title = new QLabel(QStringLiteral("Mods"), this);
    title->setObjectName(QStringLiteral("PageTitle"));
    heading->addWidget(title);
    heading->addStretch(1);
    auto* providerTag = new QLabel(QStringLiteral("MODRINTH  /  MOD DISCOVERY"), this);
    providerTag->setObjectName(QStringLiteral("ModrinthProviderTag"));
    heading->addWidget(providerTag);
    layout->addLayout(heading);

    auto* subtitle = new QLabel(
        QStringLiteral("Encuentra mods compatibles para tu versión de Minecraft y abre cada ficha antes de instalar."), this);
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    m_modrinthBanner = new QFrame(this);
    m_modrinthBanner->setObjectName(QStringLiteral("ModrinthBanner"));
    auto* bannerLayout = new QHBoxLayout(m_modrinthBanner);
    bannerLayout->setContentsMargins(18, 14, 18, 14);
    auto* brand = new QLabel(QStringLiteral("M"), m_modrinthBanner);
    brand->setObjectName(QStringLiteral("ModrinthMark"));
    brand->setAlignment(Qt::AlignCenter);
    brand->setFixedSize(38, 38);
    bannerLayout->addWidget(brand);
    auto* brandCopy = new QVBoxLayout();
    auto* brandTitle = new QLabel(QStringLiteral("Explora Modrinth"), m_modrinthBanner);
    brandTitle->setObjectName(QStringLiteral("ModrinthTitle"));
    auto* brandDescription = new QLabel(
        QStringLiteral("Catálogo y versiones desde Modrinth. CurseForge abre su búsqueda en el navegador."), m_modrinthBanner);
    brandDescription->setObjectName(QStringLiteral("ModrinthDescription"));
    brandDescription->setWordWrap(true);
    brandCopy->addWidget(brandTitle);
    brandCopy->addWidget(brandDescription);
    bannerLayout->addLayout(brandCopy, 1);
    auto* browseButton = new QPushButton(QStringLiteral("Abrir Modrinth ↗"), m_modrinthBanner);
    browseButton->setObjectName(QStringLiteral("SecondaryButton"));
    browseButton->setAccessibleName(QStringLiteral("Abrir el catálogo de mods de Modrinth en el navegador"));
    connect(browseButton, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://modrinth.com/mods")));
    });
    bannerLayout->addWidget(browseButton);
    layout->addWidget(m_modrinthBanner);

    auto* filtersFrame = new QFrame(this);
    filtersFrame->setObjectName(QStringLiteral("ModsFilterCard"));
    auto* filtersLayout = new QVBoxLayout(filtersFrame);
    filtersLayout->setContentsMargins(16, 14, 16, 14);
    filtersLayout->setSpacing(10);
    auto* filtersLabel = new QLabel(QStringLiteral("AFINA TU BÚSQUEDA"), filtersFrame);
    filtersLabel->setObjectName(QStringLiteral("ModsSectionEyebrow"));
    filtersLayout->addWidget(filtersLabel);

    auto* searchRow = new QHBoxLayout();
    searchRow->setSpacing(10);
    m_query = new QLineEdit(filtersFrame);
    m_query->setObjectName(QStringLiteral("ModsSearchField"));
    m_query->setPlaceholderText(QStringLiteral("Nombre del mod o palabra clave"));
    m_query->setAccessibleName(QStringLiteral("Buscar mods en Modrinth por nombre o palabra clave"));
    m_query->setClearButtonEnabled(true);
    searchRow->addWidget(m_query, 1);
    m_searchButton = new QPushButton(QStringLiteral("Buscar mods"), filtersFrame);
    m_searchButton->setObjectName(QStringLiteral("Primary"));
    m_searchButton->setAccessibleName(QStringLiteral("Buscar mods compatibles en Modrinth"));
    searchRow->addWidget(m_searchButton);
    filtersLayout->addLayout(searchRow);

    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(10);
    auto* versionLabel = new QLabel(QStringLiteral("Minecraft"), filtersFrame);
    versionLabel->setObjectName(QStringLiteral("Secondary"));
    filterRow->addWidget(versionLabel);
    m_gameVersion = new QLineEdit(filtersFrame);
    m_gameVersion->setObjectName(QStringLiteral("ModsVersionField"));
    m_gameVersion->setPlaceholderText(QStringLiteral("Ej. 1.21.10"));
    m_gameVersion->setAccessibleName(QStringLiteral("Versión exacta de Minecraft para filtrar mods"));
    m_gameVersion->setToolTip(QStringLiteral("Usa la versión exacta de tu instalación"));
    m_gameVersion->setMaxLength(32);
    m_gameVersion->setMaximumWidth(180);
    filterRow->addWidget(m_gameVersion);
    auto* loaderLabel = new QLabel(QStringLiteral("Cargador"), filtersFrame);
    loaderLabel->setObjectName(QStringLiteral("Secondary"));
    filterRow->addWidget(loaderLabel);
    m_loader = new QComboBox(filtersFrame);
    m_loader->setObjectName(QStringLiteral("ModsLoader"));
    m_loader->setAccessibleName(QStringLiteral("Cargador para filtrar mods compatibles"));
    m_loader->addItem(QStringLiteral("Fabric"), QStringLiteral("fabric"));
    m_loader->addItem(QStringLiteral("Forge"), QStringLiteral("forge"));
    m_loader->addItem(QStringLiteral("NeoForge"), QStringLiteral("neoforge"));
    filterRow->addWidget(m_loader);
    auto* sortLabel = new QLabel(QStringLiteral("Ordenar"), filtersFrame);
    sortLabel->setObjectName(QStringLiteral("Secondary"));
    filterRow->addWidget(sortLabel);
    m_sort = new QComboBox(filtersFrame);
    m_sort->setObjectName(QStringLiteral("ModsSort"));
    m_sort->setAccessibleName(QStringLiteral("Ordenar resultados de Modrinth"));
    m_sort->addItem(QStringLiteral("Relevancia"), QStringLiteral("relevance"));
    m_sort->addItem(QStringLiteral("Más descargas"), QStringLiteral("downloads"));
    m_sort->addItem(QStringLiteral("Más seguidores"), QStringLiteral("follows"));
    m_sort->addItem(QStringLiteral("Actualizados"), QStringLiteral("updated"));
    filterRow->addWidget(m_sort, 1);
    filtersLayout->addLayout(filterRow);
    layout->addWidget(filtersFrame);

    m_results = new QListWidget(this);
    m_results->setObjectName(QStringLiteral("InstanceList"));
    m_results->setAccessibleName(QStringLiteral("Resultados de mods compatibles de Modrinth"));
    m_results->setSelectionMode(QAbstractItemView::SingleSelection);
    m_results->setUniformItemSizes(false);
    m_results->setSpacing(7);
    layout->addWidget(m_results, 1);

    auto* actions = new QHBoxLayout();
    m_status = new QLabel(QStringLiteral("Elige Minecraft y cargador; Modrinth mostrará proyectos compatibles."), this);
    m_status->setObjectName(QStringLiteral("ModsStatus"));
    m_status->setAccessibleName(QStringLiteral("Estado de búsqueda y descarga de mods"));
    m_status->setWordWrap(true);
    actions->addWidget(m_status, 1);
    m_projectButton = new QPushButton(QStringLiteral("Ver proyecto ↗"), this);
    m_projectButton->setObjectName(QStringLiteral("SecondaryButton"));
    m_projectButton->setAccessibleName(QStringLiteral("Abrir la ficha del proyecto seleccionado en Modrinth"));
    m_projectButton->setEnabled(false);
    connect(m_projectButton, &QPushButton::clicked, this, [this] {
        const auto* item = m_results->currentItem();
        if (!item) return;
        const QString projectId = item->data(Qt::UserRole).toString();
        if (!modsValidation::isModrinthProjectId(projectId))
            return;
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://modrinth.com/mod/%1").arg(projectId)));
    });
    actions->addWidget(m_projectButton);
    m_downloadButton = new QPushButton(QStringLiteral("Descargar .jar"), this);
    m_downloadButton->setObjectName(QStringLiteral("Primary"));
    m_downloadButton->setToolTip(QStringLiteral("Descarga el archivo del mod; el cargador se configura por separado"));
    m_downloadButton->setEnabled(false);
    connect(m_downloadButton, &QPushButton::clicked, this, &ModsPage::installSelected);
    actions->addWidget(m_downloadButton);
    auto* curseButton = new QPushButton(QStringLiteral("CurseForge ↗"), this);
    curseButton->setObjectName(QStringLiteral("SecondaryButton"));
    curseButton->setAccessibleName(QStringLiteral("Abrir la búsqueda con estos filtros en CurseForge"));
    connect(curseButton, &QPushButton::clicked, this, &ModsPage::openCurseForge);
    actions->addWidget(curseButton);
    layout->addLayout(actions);

    auto* note = new QLabel(
        QStringLiteral("CUB3 descarga archivos de Modrinth, pero no instala Fabric, Forge ni NeoForge. Revisa la página y licencia del proyecto antes de instalar."), this);
    note->setObjectName(QStringLiteral("Secondary"));
    note->setWordWrap(true);
    layout->addWidget(note);

    connect(m_searchButton, &QPushButton::clicked, this, &ModsPage::search);
    connect(m_query, &QLineEdit::returnPressed, this, &ModsPage::search);
    connect(m_gameVersion, &QLineEdit::returnPressed, this, &ModsPage::search);
    connect(m_query, &QLineEdit::textChanged, this, &ModsPage::updateSelectionActions);
    connect(m_gameVersion, &QLineEdit::textChanged, this, &ModsPage::updateSelectionActions);
    connect(m_loader, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ModsPage::updateSelectionActions);
    connect(m_sort, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ModsPage::updateSelectionActions);
    connect(m_results, &QListWidget::itemSelectionChanged,
            this, &ModsPage::updateSelectionActions);
    connect(m_results, &QListWidget::itemDoubleClicked, this,
            [this] { installSelected(); });
}

void ModsPage::search() {
    if (m_downloadInProgress) {
        m_status->setText(QStringLiteral("Espera a que termine la descarga actual antes de iniciar otra búsqueda."));
        return;
    }
    const QString game = m_gameVersion->text().trimmed();
    if (!modsValidation::isMinecraftVersion(game)) {
        m_gameVersion->setFocus();
        m_status->setText(QStringLiteral("Escribe una versión de Minecraft válida, por ejemplo 1.21.10."));
        return;
    }
    if (!m_app) {
        m_status->setText(QStringLiteral("El servicio de red no está disponible."));
        return;
    }

    const quint64 revision = ++m_searchRevision;
    const QString searchText = m_query->text().trimmed();
    const QString loader = m_loader->currentData().toString();
    m_resultsForVersion = game;
    m_resultsForLoader = loader;
    m_resultsForQuery = searchText;
    m_resultsForSort = m_sort->currentData().toString();
    m_searchButton->setEnabled(false);
    m_downloadButton->setEnabled(false);
    m_projectButton->setEnabled(false);
    m_results->clear();
    auto* searchingItem = new QListWidgetItem(QStringLiteral("Buscando mods compatibles…"), m_results);
    searchingItem->setFlags(Qt::NoItemFlags);
    m_status->setText(QStringLiteral("Buscando “%1” para Minecraft %2 con %3…")
                          .arg(searchText.isEmpty() ? QStringLiteral("todos los mods") : searchText,
                               game, m_loader->currentText()));
    QPointer<ModsPage> guard(this);
    const QPointer<ModsPage> callbackGuard(guard);
    QJsonArray facets;
    facets.append(QJsonArray{QStringLiteral("project_type:mod")});
    facets.append(QJsonArray{QStringLiteral("versions:%1").arg(game)});
    facets.append(QJsonArray{QStringLiteral("categories:%1").arg(loader)});

    QUrl url(QStringLiteral("https://api.modrinth.com/v2/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), searchText);
    query.addQueryItem(QStringLiteral("facets"), QString::fromUtf8(QJsonDocument(facets).toJson(QJsonDocument::Compact)));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("30"));
    query.addQueryItem(QStringLiteral("index"), m_sort->currentData().toString());
    url.setQuery(query);
    m_app->httpClient().get(url, [callbackGuard, revision, game, loader]
                            (const QByteArray& body, int status) {
        if (!callbackGuard || callbackGuard->m_searchRevision != revision)
            return;
        callbackGuard->m_searchButton->setEnabled(true);
        if (status < 200 || status >= 300) {
            callbackGuard->m_results->clear();
            auto* errorItem = new QListWidgetItem(status == 0
                ? QStringLiteral("No hubo conexión. Comprueba internet y vuelve a buscar.")
                : QStringLiteral("La búsqueda falló; vuelve a intentarlo en un momento."),
                callbackGuard->m_results);
            errorItem->setFlags(Qt::NoItemFlags);
            callbackGuard->m_status->setText(status == 0
                ? QStringLiteral("No se pudo conectar con Modrinth. Comprueba internet y vuelve a intentar.")
                : QStringLiteral("Modrinth respondió con HTTP %1. Intenta de nuevo en un momento.").arg(status));
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            callbackGuard->m_results->clear();
            auto* errorItem = new QListWidgetItem(
                QStringLiteral("No pudimos leer los resultados. Vuelve a buscar."),
                callbackGuard->m_results);
            errorItem->setFlags(Qt::NoItemFlags);
            callbackGuard->m_status->setText(QStringLiteral("Modrinth devolvió resultados que no se pudieron leer. Vuelve a buscar."));
            return;
        }
        const auto hits = document.object().value(QStringLiteral("hits")).toArray();
        callbackGuard->m_results->clear();
        for (const auto& value : hits) {
            const auto hit = value.toObject();
            const QString title = hit.value(QStringLiteral("title")).toString().trimmed();
            const QString description = hit.value(QStringLiteral("description")).toString().simplified();
            const QString id = hit.value(QStringLiteral("project_id")).toString();
            if (title.isEmpty() || !modsValidation::isModrinthProjectId(id))
                continue;
            const qint64 downloads = hit.value(QStringLiteral("downloads")).toVariant().toLongLong();
            const QString truncatedDescription = description.size() > 180
                ? description.left(177).trimmed() + QStringLiteral("…") : description;
            const qint64 follows = hit.value(QStringLiteral("follows")).toVariant().toLongLong();
            const QString author = hit.value(QStringLiteral("author")).toString().trimmed();
            const QStringList categories = [&hit] {
                QStringList values;
                const auto array = hit.value(QStringLiteral("display_categories")).toArray();
                for (const auto& value : array) {
                    const QString category = value.toString().trimmed();
                    if (!category.isEmpty() && values.size() < 3)
                        values.append(category);
                }
                return values;
            }();
            auto* item = new QListWidgetItem(callbackGuard->m_results);
            item->setData(Qt::UserRole, id);
            item->setData(Qt::UserRole + 1, title);
            item->setData(Qt::UserRole + 2, hit.value(QStringLiteral("slug")).toString());
            item->setData(Qt::UserRole + 3, hit.value(QStringLiteral("icon_url")).toString());
            item->setData(Qt::UserRole + 4, downloads);
            item->setData(Qt::UserRole + 5, follows);
            item->setData(Qt::AccessibleTextRole, title + QStringLiteral(". ") + truncatedDescription);
            item->setToolTip(QStringLiteral("%1 · Proyecto %2 · %3")
                                 .arg(title, hit.value(QStringLiteral("slug")).toString(), id));
            auto* card = new QWidget(callbackGuard->m_results);
            card->setObjectName(QStringLiteral("ModResultCard"));
            card->setAttribute(Qt::WA_TransparentForMouseEvents);
            auto* cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(14, 11, 14, 11);
            cardLayout->setSpacing(5);
            auto* titleLabel = new QLabel(title, card);
            titleLabel->setObjectName(QStringLiteral("ModResultTitle"));
            titleLabel->setTextInteractionFlags(Qt::NoTextInteraction);
            auto* descriptionLabel = new QLabel(truncatedDescription.isEmpty()
                ? QStringLiteral("Sin descripción disponible.") : truncatedDescription, card);
            descriptionLabel->setObjectName(QStringLiteral("ModResultDescription"));
            descriptionLabel->setWordWrap(true);
            descriptionLabel->setTextInteractionFlags(Qt::NoTextInteraction);
            auto* metadata = new QLabel(
                QStringLiteral("MODRINTH     ↓ %1     ♡ %2")
                    .arg(QLocale(QLocale::Spanish).toString(downloads),
                         QLocale(QLocale::Spanish).toString(follows)), card);
            metadata->setObjectName(QStringLiteral("ModResultMeta"));
            metadata->setTextInteractionFlags(Qt::NoTextInteraction);
            QString metadataText = QStringLiteral("MODRINTH     ↓ %1     ♡ %2")
                .arg(QLocale(QLocale::Spanish).toString(downloads),
                     QLocale(QLocale::Spanish).toString(follows));
            if (!author.isEmpty())
                metadataText += QStringLiteral("     ·     por %1").arg(author);
            if (!categories.isEmpty())
                metadataText += QStringLiteral("     ·     %1").arg(categories.join(QStringLiteral(" · ")));
            metadata->setText(metadataText);
            cardLayout->addWidget(titleLabel);
            cardLayout->addWidget(descriptionLabel);
            cardLayout->addWidget(metadata);
            item->setSizeHint(QSize(0, 112));
            callbackGuard->m_results->setItemWidget(item, card);
        }
        int count = callbackGuard->m_results->count();
        if (count == 0) {
            auto* emptyItem = new QListWidgetItem(
                QStringLiteral("No hay resultados. Prueba otra versión, nombre o cargador."),
                callbackGuard->m_results);
            emptyItem->setFlags(Qt::NoItemFlags);
        }
        callbackGuard->updateSelectionActions();
        callbackGuard->m_status->setText(count == 0
            ? QStringLiteral("No encontramos mods de Modrinth para Minecraft %1 con %2. Prueba otra versión o cargador.").arg(game, loader)
            : QStringLiteral("%1 mods compatibles · Minecraft %2 · %3 · Modrinth")
                  .arg(count).arg(game, callbackGuard->m_loader->currentText()));
    });
}

void ModsPage::installSelected() {
    const auto* item = m_results->currentItem();
    if (!item || m_downloadInProgress)
        return;
    if (!m_app) {
        m_status->setText(QStringLiteral("El servicio de red no está disponible."));
        return;
    }
    if (m_gameVersion->text().trimmed() != m_resultsForVersion
        || m_loader->currentData().toString() != m_resultsForLoader
        || m_query->text().trimmed() != m_resultsForQuery) {
        m_status->setText(QStringLiteral("Cambiaste los filtros. Vuelve a buscar antes de descargar."));
        return;
    }
    const QString projectId = item->data(Qt::UserRole).toString();
    const QString itemTitle = item->data(Qt::UserRole + 1).toString();
    if (projectId.isEmpty() || itemTitle.isEmpty()) {
        m_status->setText(QStringLiteral("Selecciona un resultado válido de Modrinth."));
        return;
    }

    QJsonArray loaders{m_loader->currentData().toString()};
    QJsonArray games{m_gameVersion->text().trimmed()};
    QUrl url(QStringLiteral("https://api.modrinth.com/v2/project/%1/version").arg(projectId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("loaders"), QString::fromUtf8(QJsonDocument(loaders).toJson(QJsonDocument::Compact)));
    query.addQueryItem(QStringLiteral("game_versions"), QString::fromUtf8(QJsonDocument(games).toJson(QJsonDocument::Compact)));
    url.setQuery(query);
    m_downloadInProgress = true;
    m_downloadButton->setEnabled(false);
    m_projectButton->setEnabled(false);
    m_searchButton->setEnabled(false);
    m_status->setText(QStringLiteral("Buscando versión compatible para %1…").arg(itemTitle));
    QPointer<ModsPage> guard(this);
    m_app->httpClient().get(url, [guard, itemTitle, gameVersion = m_gameVersion->text().trimmed()]
                            (const QByteArray& body, int status) {
        if (!guard)
            return;
        if (status < 200 || status >= 300) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(status == 0
                ? QStringLiteral("No se pudo conectar con Modrinth. Vuelve a intentar la descarga.")
                : QStringLiteral("Modrinth no pudo buscar la versión compatible (HTTP %1). Puedes reintentar.").arg(status));
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(QStringLiteral("No se pudo leer la versión del mod. Vuelve a intentar."));
            return;
        }
        const QJsonArray versions = document.array();
        if (versions.isEmpty()) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(QStringLiteral("No hay una versión compatible con este Minecraft y cargador."));
            return;
        }

        QJsonObject selectedVersion;
        for (const auto& candidate : versions) {
            const QJsonObject object = candidate.toObject();
            const QString versionType = object.value(QStringLiteral("version_type")).toString();
            if (versionType == QStringLiteral("release")) {
                selectedVersion = object;
                break;
            }
            if (selectedVersion.isEmpty())
                selectedVersion = object;
        }
        const QJsonArray files = selectedVersion.value(QStringLiteral("files")).toArray();
        if (files.isEmpty()) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(QStringLiteral("Modrinth no publicó un archivo descargable para esta versión."));
            return;
        }
        QJsonObject selectedFile;
        for (const auto& entry : files) {
            const QJsonObject object = entry.toObject();
            if (object.value(QStringLiteral("primary")).toBool()) {
                selectedFile = object;
                break;
            }
        }
        if (selectedFile.isEmpty())
            selectedFile = files.first().toObject();
        const QUrl fileUrl(selectedFile.value(QStringLiteral("url")).toString());
        const QString filename = modsValidation::safeJarFileName(
            selectedFile.value(QStringLiteral("filename")).toString());
        if (!network::HttpClient::isSecureUrl(fileUrl) || filename.isEmpty()) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(QStringLiteral("Modrinth devolvió un archivo o una dirección no válidos."));
            return;
        }

        const QString configuredInstances = guard->m_app->settings().instancesDir().trimmed();
        const QString instancesRoot = configuredInstances.isEmpty()
            ? core::Paths::instancesDir() : configuredInstances;
        const QString directory = QDir(instancesRoot)
            .filePath(gameVersion + QStringLiteral("/mods"));
        if (!QDir().mkpath(directory)) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(QStringLiteral("No se pudo preparar la carpeta de mods."));
            return;
        }
        const QString output = QDir(directory).filePath(filename);
        if (QFileInfo::exists(output)) {
            const auto answer = QMessageBox::question(
                guard, QStringLiteral("El archivo ya existe"),
                QStringLiteral("¿Quieres reemplazar %1?").arg(filename),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                guard->m_downloadInProgress = false;
                guard->m_searchButton->setEnabled(true);
                guard->updateSelectionActions();
                guard->m_status->setText(QStringLiteral("Descarga cancelada; se conservó el archivo existente."));
                return;
            }
        }

        const auto answer = QMessageBox::question(
            guard, QStringLiteral("Confirmar descarga"),
            QStringLiteral("Se descargará %1 de la versión %2, con destino a:\n%3\n\n¿Continuar?\n\nRevisa la licencia y los requisitos del proyecto. CUB3 no instala el cargador de mods.")
                .arg(itemTitle, selectedVersion.value(QStringLiteral("version_number")).toString(), output),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(QStringLiteral("Descarga cancelada."));
            return;
        }
        QString backupPath;
        if (QFileInfo::exists(output)) {
            backupPath = output + QStringLiteral(".cub3-backup-")
                + QUuid::createUuid().toString(QUuid::WithoutBraces);
            if (!QFile::rename(output, backupPath)) {
                guard->m_downloadInProgress = false;
                guard->m_searchButton->setEnabled(true);
                guard->updateSelectionActions();
                guard->m_status->setText(QStringLiteral("No se pudo proteger el archivo existente antes de reemplazarlo."));
                return;
            }
        }
        network::DownloadJob job;
        job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        job.url = fileUrl;
        job.targetPath = output;
        job.expectedSize = selectedFile.value(QStringLiteral("size")).toVariant().toLongLong();
        if (job.expectedSize <= 0)
            job.expectedSize = -1;
        const QJsonObject hashes = selectedFile.value(QStringLiteral("hashes")).toObject();
        const QString sha1 = hashes.value(QStringLiteral("sha1")).toString().toLower();
        const QString sha256 = hashes.value(QStringLiteral("sha256")).toString().toLower();
        if (QRegularExpression(QStringLiteral("^[0-9a-f]{40}$")).match(sha1).hasMatch()) {
            job.expectedHash = sha1;
            job.hashAlgorithm = QStringLiteral("sha1");
        } else if (QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(sha256).hasMatch()) {
            job.expectedHash = sha256;
            job.hashAlgorithm = QStringLiteral("sha256");
        }
        job.maxRetries = 2;

        if (guard->m_downloadInProgress)
            guard->m_status->setText(QStringLiteral("Descargando %1…").arg(itemTitle));
        auto& engine = guard->m_app->downloadEngine();
        const QString downloadId = job.id;
        const QPointer<ModsPage> downloadGuard = guard;
        auto backupConnection = std::make_shared<QMetaObject::Connection>();
        auto backupRecoveryPath = std::make_shared<QString>();
        *backupConnection = QObject::connect(
            &engine, &network::DownloadEngine::downloadFinished, &engine,
            [backupConnection, backupRecoveryPath, downloadId, output, backupPath]
            (const QString& finishedId, network::DownloadStatus status, const QString&) {
                if (finishedId != downloadId)
                    return;
                QObject::disconnect(*backupConnection);
                if (backupPath.isEmpty() || !QFileInfo::exists(backupPath))
                    return;
                if (status == network::DownloadStatus::Completed
                    || status == network::DownloadStatus::Skipped) {
                    QFile::remove(backupPath);
                } else {
                    QFile::remove(output);
                    if (!QFile::rename(backupPath, output))
                        *backupRecoveryPath = backupPath;
                }
            });
        guard->m_downloadProgressConnection = QObject::connect(
            &engine, &network::DownloadEngine::progressChanged, guard,
            [downloadGuard, downloadId, itemTitle](const network::DownloadProgress& progress) {
                if (!downloadGuard || progress.id != downloadId)
                    return;
                if (progress.total > 0) {
                    const int percent = static_cast<int>(progress.downloaded * 100 / progress.total);
                    const double speed = progress.speedBps / (1024.0 * 1024.0);
                    downloadGuard->m_status->setText(
                        QStringLiteral("Descargando %1 · %2% · %3 MB/s…")
                            .arg(itemTitle).arg(percent).arg(QLocale(QLocale::Spanish).toString(speed, 'f', 1)));
                }
            });
        guard->m_downloadConnection = QObject::connect(
            &engine, &network::DownloadEngine::downloadFinished, guard,
            [downloadGuard, backupRecoveryPath, downloadId, itemTitle, filename, gameVersion]
            (const QString& finishedId, network::DownloadStatus status, const QString& error) {
                if (!downloadGuard || finishedId != downloadId)
                    return;
                QObject::disconnect(downloadGuard->m_downloadConnection);
                QObject::disconnect(downloadGuard->m_downloadProgressConnection);
                downloadGuard->m_downloadInProgress = false;
                downloadGuard->m_searchButton->setEnabled(true);
                downloadGuard->updateSelectionActions();
                if (status != network::DownloadStatus::Completed
                    && status != network::DownloadStatus::Skipped) {
                    if (!backupRecoveryPath->isEmpty()) {
                        downloadGuard->m_status->setText(
                            QStringLiteral("Falló la descarga de %1 y no se pudo restaurar el archivo anterior. Está conservado como %2")
                                .arg(itemTitle, *backupRecoveryPath));
                    } else {
                        downloadGuard->m_status->setText(
                            QStringLiteral("No se pudo descargar %1: %2. Se conservó el archivo anterior, si existía; puedes volver a intentar.")
                                .arg(itemTitle, error));
                    }
                    return;
                }
                downloadGuard->m_status->setText(
                    QStringLiteral("%1 se guardó en %2/mods. Verifica compatibilidad y licencia antes de jugar.")
                        .arg(filename, gameVersion));
            });
        if (engine.submit(job).isEmpty()) {
            QObject::disconnect(guard->m_downloadConnection);
            QObject::disconnect(guard->m_downloadProgressConnection);
            QObject::disconnect(*backupConnection);
            bool restored = true;
            if (!backupPath.isEmpty() && QFileInfo::exists(backupPath))
                restored = QFile::rename(backupPath, output);
            guard->m_downloadInProgress = false;
            guard->m_searchButton->setEnabled(true);
            guard->updateSelectionActions();
            guard->m_status->setText(restored
                ? QStringLiteral("No se pudo agregar el mod a la cola de descargas; se conservó el archivo anterior.")
                : QStringLiteral("No se pudo agregar la descarga ni restaurar el archivo anterior; está conservado como %1").arg(backupPath));
            return;
        }
    });
}

void ModsPage::updateSelectionActions() {
    const auto* item = m_results->currentItem();
    const bool sameFilters = m_gameVersion->text().trimmed() == m_resultsForVersion
        && m_loader->currentData().toString() == m_resultsForLoader
        && m_query->text().trimmed() == m_resultsForQuery
        && m_sort->currentData().toString() == m_resultsForSort;
    if (!m_resultsForVersion.isEmpty() && !sameFilters && !m_downloadInProgress) {
        m_status->setText(QStringLiteral("Cambiaste algún filtro; vuelve a buscar para actualizar estos resultados antes de descargar."));
    }
    for (int i = 0; i < m_results->count(); ++i) {
        auto* row = m_results->itemWidget(m_results->item(i));
        if (row) {
            row->setProperty("selected", m_results->item(i) == item);
            row->style()->unpolish(row);
            row->style()->polish(row);
        }
    }
    const bool valid = sameFilters && item
        && !item->data(Qt::UserRole).toString().isEmpty();
    m_downloadButton->setEnabled(valid && !m_downloadInProgress);
    m_projectButton->setEnabled(valid && !m_downloadInProgress);
}

void ModsPage::openCurseForge() {
    QUrl url(QStringLiteral("https://www.curseforge.com/minecraft/search?class=mc-mods"));
    QUrlQuery query(url);
    const QString searchText = m_query->text().trimmed();
    if (!searchText.isEmpty()) query.addQueryItem(QStringLiteral("search"), searchText);
    query.addQueryItem(QStringLiteral("gameVersion"), m_gameVersion->text().trimmed());
    const QString loader = m_loader->currentData().toString();
    const QString curseForgeLoader = loader == QStringLiteral("fabric")
        ? QStringLiteral("4")
        : loader == QStringLiteral("neoforge") ? QStringLiteral("6")
                                                : QStringLiteral("1");
    query.addQueryItem(QStringLiteral("modLoaderType"), curseForgeLoader);
    url.setQuery(query);
    QDesktopServices::openUrl(url);
}

} // namespace cub3::ui::pages
