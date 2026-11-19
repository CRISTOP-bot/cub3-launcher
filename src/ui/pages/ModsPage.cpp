#include "ModsPage.h"

#include "application/Cub3App.h"
#include "core/Paths.h"
#include "network/HttpClient.h"
#include "styles/Theme.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QUrlQuery>

namespace cub3::ui::pages {

ModsPage::ModsPage(application::Cub3App* app, QWidget* parent)
    : QWidget(parent), m_app(app) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(theme::sizes::kSpacingLarge, theme::sizes::kSpacingLarge,
                               theme::sizes::kSpacingLarge, theme::sizes::kSpacingLarge);
    layout->setSpacing(theme::sizes::kSpacingMedium);

    auto* title = new QLabel(QStringLiteral("Mods"), this);
    title->setObjectName(QStringLiteral("PageTitle"));
    layout->addWidget(title);
    auto* subtitle = new QLabel(
        QStringLiteral("Busca en Modrinth o CurseForge y filtra por versión y cargador. "
                       "Los cargadores aún no se instalan desde CUB3."), this);
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    layout->addWidget(subtitle);

    auto* filters = new QHBoxLayout();
    m_query = new QLineEdit(this);
    m_query->setPlaceholderText(QStringLiteral("Buscar mods…"));
    m_query->setAccessibleName(QStringLiteral("Buscar mods por nombre"));
    m_query->setClearButtonEnabled(true);
    filters->addWidget(m_query, 1);
    m_gameVersion = new QLineEdit(QStringLiteral("1.21.10"), this);
    m_gameVersion->setPlaceholderText(QStringLiteral("Minecraft"));
    m_gameVersion->setAccessibleName(QStringLiteral("Versión de Minecraft para filtrar mods"));
    m_gameVersion->setMaximumWidth(120);
    filters->addWidget(m_gameVersion);
    m_loader = new QComboBox(this);
    m_loader->setAccessibleName(QStringLiteral("Cargador de mods"));
    m_loader->addItem(QStringLiteral("Fabric"), QStringLiteral("fabric"));
    m_loader->addItem(QStringLiteral("Forge"), QStringLiteral("forge"));
    m_loader->addItem(QStringLiteral("NeoForge"), QStringLiteral("neoforge"));
    filters->addWidget(m_loader);
    auto* searchButton = new QPushButton(QStringLiteral("Buscar"), this);
    searchButton->setObjectName(QStringLiteral("Primary"));
    filters->addWidget(searchButton);
    layout->addLayout(filters);

    m_results = new QListWidget(this);
    m_results->setObjectName(QStringLiteral("InstanceList"));
    m_results->setAccessibleName(QStringLiteral("Resultados de mods compatibles"));
    layout->addWidget(m_results, 1);

    auto* actions = new QHBoxLayout();
    m_status = new QLabel(QStringLiteral("Modrinth: búsqueda y descarga directa."), this);
    m_status->setObjectName(QStringLiteral("Secondary"));
    actions->addWidget(m_status, 1);
    auto* installButton = new QPushButton(QStringLiteral("Descargar mod seleccionado"), this);
    installButton->setToolTip(QStringLiteral("Descarga el archivo del mod; el cargador se configura por separado"));
    connect(installButton, &QPushButton::clicked, this, &ModsPage::installSelected);
    actions->addWidget(installButton);
    auto* curseButton = new QPushButton(QStringLiteral("Buscar en CurseForge ↗"), this);
    curseButton->setAccessibleName(QStringLiteral("Abrir búsqueda de mods en CurseForge"));
    connect(curseButton, &QPushButton::clicked, this, &ModsPage::openCurseForge);
    actions->addWidget(curseButton);
    layout->addLayout(actions);

    connect(searchButton, &QPushButton::clicked, this, &ModsPage::search);
    connect(m_query, &QLineEdit::returnPressed, this, &ModsPage::search);
    connect(m_results, &QListWidget::itemDoubleClicked, this,
            [this] { installSelected(); });
}

void ModsPage::search() {
    const QString game = m_gameVersion->text().trimmed();
    if (game.isEmpty()) {
        showError(QStringLiteral("Escribe una versión de Minecraft para filtrar los mods."));
        return;
    }
    QJsonArray facets;
    facets.append(QJsonArray{QStringLiteral("project_type:mod")});
    facets.append(QJsonArray{QStringLiteral("versions:%1").arg(game)});
    facets.append(QJsonArray{QStringLiteral("categories:%1").arg(m_loader->currentData().toString())});

    QUrl url(QStringLiteral("https://api.modrinth.com/v2/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), m_query->text().trimmed());
    query.addQueryItem(QStringLiteral("facets"), QString::fromUtf8(QJsonDocument(facets).toJson(QJsonDocument::Compact)));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("30"));
    url.setQuery(query);
    m_results->clear();
    m_status->setText(QStringLiteral("Buscando en Modrinth…"));
    m_app->httpClient().get(url, [this](const QByteArray& body, int status) {
        if (status < 200 || status >= 300) {
            showError(QStringLiteral("Modrinth respondió con HTTP %1.").arg(status));
            return;
        }
        const auto root = QJsonDocument::fromJson(body).object();
        const auto hits = root.value(QStringLiteral("hits")).toArray();
        for (const auto& value : hits) {
            const auto hit = value.toObject();
            const QString title = hit.value(QStringLiteral("title")).toString();
            const QString description = hit.value(QStringLiteral("description")).toString();
            const QString id = hit.value(QStringLiteral("project_id")).toString();
            auto* item = new QListWidgetItem(
                QStringLiteral("%1\n%2\n⬇ %3")
                    .arg(title, description,
                         QString::number(hit.value(QStringLiteral("downloads")).toVariant().toLongLong())),
                m_results);
            item->setData(Qt::UserRole, id);
            item->setData(Qt::UserRole + 1, title);
            item->setToolTip(QStringLiteral("%1 · %2").arg(hit.value(QStringLiteral("slug")).toString(), id));
        }
        m_status->setText(hits.isEmpty()
            ? QStringLiteral("No se encontraron mods compatibles.")
            : QStringLiteral("%1 mods compatibles · Fuente: Modrinth").arg(hits.size()));
    });
}

void ModsPage::installSelected() {
    const auto* item = m_results->currentItem();
    if (!item) {
        showError(QStringLiteral("Selecciona un mod de la lista."));
        return;
    }
    QJsonArray loaders{m_loader->currentData().toString()};
    QJsonArray games{m_gameVersion->text().trimmed()};
    QUrl url(QStringLiteral("https://api.modrinth.com/v2/project/%1/version")
                 .arg(item->data(Qt::UserRole).toString()));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("loaders"), QString::fromUtf8(QJsonDocument(loaders).toJson(QJsonDocument::Compact)));
    query.addQueryItem(QStringLiteral("game_versions"), QString::fromUtf8(QJsonDocument(games).toJson(QJsonDocument::Compact)));
    url.setQuery(query);
    m_status->setText(QStringLiteral("Buscando una versión compatible…"));
    m_app->httpClient().get(url, [this, itemTitle = item->data(Qt::UserRole + 1).toString()]
                            (const QByteArray& body, int status) {
        if (status < 200 || status >= 300) {
            showError(QStringLiteral("No se pudo obtener la versión del mod (HTTP %1)." ).arg(status));
            return;
        }
        const auto versions = QJsonDocument::fromJson(body).array();
        if (versions.isEmpty()) {
            showError(QStringLiteral("Este mod no tiene una versión compatible con el filtro actual."));
            return;
        }
        const auto files = versions.first().toObject().value(QStringLiteral("files")).toArray();
        if (files.isEmpty()) {
            showError(QStringLiteral("Modrinth no publicó un archivo descargable para esta versión."));
            return;
        }
        auto file = files.first().toObject();
        for (const auto& entry : files) {
            if (entry.toObject().value(QStringLiteral("primary")).toBool()) {
                file = entry.toObject();
                break;
            }
        }
        const QUrl fileUrl(file.value(QStringLiteral("url")).toString());
        const QString filename = QFileInfo(file.value(QStringLiteral("filename")).toString()).fileName();
        const QString directory = QDir(core::Paths::instancesDir()).filePath(m_gameVersion->text().trimmed() + QStringLiteral("/mods"));
        if (filename.isEmpty() || !QDir().mkpath(directory)) {
            showError(QStringLiteral("No se pudo preparar la carpeta de mods."));
            return;
        }
        const QString output = QDir(directory).filePath(filename);
        m_status->setText(QStringLiteral("Descargando %1…").arg(itemTitle));
        m_app->httpClient().downloadFile(fileUrl, output, [this, output, itemTitle](bool ok, const QString& error) {
            if (!ok) {
                QFile::remove(output);
                showError(QStringLiteral("No se pudo descargar el mod: %1").arg(error));
                return;
            }
            m_status->setText(QStringLiteral("%1 se descargó en la carpeta mods de la instancia %2. "
                                            "CUB3 todavía no instala el cargador seleccionado.")
                                  .arg(itemTitle, m_gameVersion->text().trimmed()));
        });
    });
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

void ModsPage::showError(const QString& message) {
    m_status->setText(message);
    QMessageBox::warning(this, QStringLiteral("Mods"), message);
}

} // namespace cub3::ui::pages
