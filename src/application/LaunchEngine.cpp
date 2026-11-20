// CUB3 Launcher - Vanilla Minecraft installation and launch
#include "LaunchEngine.h"
#include "CppEditionPack.h"

#include "core/Paths.h"
#include "network/HttpClient.h"
#include "network/DownloadEngine.h"
#include "storage/Database.h"
#include "storage/Settings.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcess>
#include <QRegularExpression>
#include <QDirIterator>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUuid>

namespace cub3::application {
namespace {

const QUrl kAuthlibInjectorMetadataUrl(
    QStringLiteral("https://authlib-injector.yushi.moe/artifact/latest.json"));
const QString kAuthlibInjectorHost =
    QStringLiteral("authlib-injector.yushi.moe");
const QString kElybyAuthlibUrl =
    QStringLiteral("https://authserver.ely.by/api/authlib-injector");

bool matchesRules(const QJsonArray& rules) {
    if (rules.isEmpty())
        return true;

#ifdef Q_OS_LINUX
    const QString osName = QStringLiteral("linux");
#elif defined(Q_OS_WIN)
    const QString osName = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    const QString osName = QStringLiteral("osx");
#else
    const QString osName;
#endif

    bool allowed = false;
    for (const QJsonValue& value : rules) {
        const QJsonObject rule = value.toObject();
        const QJsonObject os = rule.value(QStringLiteral("os")).toObject();
        if (os.contains(QStringLiteral("name"))
            && os.value(QStringLiteral("name")).toString() != osName)
            continue;
        if (os.contains(QStringLiteral("arch"))
            && !QRegularExpression(os.value(QStringLiteral("arch")).toString())
                    .match(QSysInfo::currentCpuArchitecture()).hasMatch())
            continue;
        if (!rule.value(QStringLiteral("features")).toObject().isEmpty())
            continue;
        allowed = rule.value(QStringLiteral("action")).toString()
            == QStringLiteral("allow");
    }
    return allowed;
}

QString substitute(QString value, const QHash<QString, QString>& values) {
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        value.replace(QStringLiteral("${%1}").arg(it.key()), it.value());
    return value;
}

int javaMajorVersion(const QString& executable) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable, {QStringLiteral("-version")});
    if (!process.waitForStarted(2000) || !process.waitForFinished(3000)) {
        process.kill();
        process.waitForFinished(1000);
        return -1;
    }

    static const QRegularExpression versionPattern(
        QStringLiteral("(\\d+)(?:\\.(\\d+))?"));
    const auto match = versionPattern.match(QString::fromLocal8Bit(process.readAll()));
    if (!match.hasMatch())
        return -1;
    const int first = match.captured(1).toInt();
    return first == 1 && !match.captured(2).isEmpty()
        ? match.captured(2).toInt() : first;
}

QString compatibleJava(const QString& preferred, int requiredMajor) {
    QStringList candidates;
    if (!preferred.trimmed().isEmpty())
        candidates.append(preferred.trimmed());
    for (const QString& name : {QStringLiteral("java"), QStringLiteral("java%1").arg(requiredMajor),
                                QStringLiteral("java-%1").arg(requiredMajor)}) {
        const QString executable = QStandardPaths::findExecutable(name);
        if (!executable.isEmpty())
            candidates.append(executable);
    }

#ifdef Q_OS_LINUX
    QDirIterator runtimes(QStringLiteral("/usr/lib/jvm"),
        {QStringLiteral("java"), QStringLiteral("java.exe")},
        QDir::Files, QDirIterator::Subdirectories);
    while (runtimes.hasNext())
        candidates.append(runtimes.next());
#endif

    QSet<QString> checked;
    for (const QString& candidate : candidates) {
        const QString absolute = QFileInfo(candidate).absoluteFilePath();
        if (checked.contains(absolute))
            continue;
        checked.insert(absolute);
        const QFileInfo info(absolute);
        if (info.isFile() && info.isExecutable()
            && (requiredMajor <= 0 || javaMajorVersion(absolute) >= requiredMajor))
            return absolute;
    }
    return {};
}

} // namespace

LaunchEngine::LaunchEngine(versions::VersionManager* versions,
                           network::DownloadEngine* downloads,
                           network::HttpClient* http,
                           storage::Database* database,
                           storage::Settings* settings,
                           QObject* parent)
    : QObject(parent)
    , m_versions(versions)
    , m_downloads(downloads)
    , m_http(http)
    , m_database(database)
    , m_settings(settings)
{
    connect(m_downloads, &network::DownloadEngine::downloadFinished,
            this, &LaunchEngine::onDownloadFinished);
}

bool LaunchEngine::isRunning() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

void LaunchEngine::launch(const QString& versionId, const auth::Account& account) {
    if (m_launching || isRunning()) {
        emit launchFailed(QStringLiteral("Ya hay un juego ejecutándose."));
        return;
    }
    if (versionId.isEmpty() || !account.isValid()) {
        emit launchFailed(QStringLiteral("Selecciona una versión e inicia sesión primero."));
        return;
    }
    m_launching = true;
    m_versionId = versionId;
    m_account = account;
    m_failed = false;
    m_pendingIds.clear();
    m_files.clear();
    m_authlibInjectorPath.clear();
    m_authlibInjectorHash.clear();
    m_authlibInjectorUrl.clear();
    emit statusChanged(QStringLiteral("Cargando manifiesto de %1...").arg(versionId));

    m_versions->fetchVersionManifest(versionId,
        [this](const QJsonDocument& document) {
            if (!document.isObject()) {
                fail(QStringLiteral("No se pudo cargar el manifiesto de la versión."));
                return;
            }
            m_manifest = document.object();
            if (m_account.provider != auth::Provider::Elyby) {
                resolveVersionFiles();
                return;
            }

            emit statusChanged(QStringLiteral("Preparando autenticación Ely.by..."));
            m_http->get(kAuthlibInjectorMetadataUrl,
                [this](const QByteArray& data, int status) {
                if (m_failed)
                    return;
                if (status != 200) {
                    fail(QStringLiteral(
                        "No se pudo obtener el instalador de autenticación Ely.by."));
                    return;
                }

                QJsonParseError error;
                const QJsonDocument document = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError
                    || !document.isObject()) {
                    fail(QStringLiteral(
                        "La información del instalador de autenticación no es válida."));
                    return;
                }

                const QJsonObject metadata = document.object();
                const QString version = metadata.value(QStringLiteral("version"))
                    .toString();
                const QString sha256 = metadata.value(QStringLiteral("checksums"))
                    .toObject().value(QStringLiteral("sha256")).toString();
                const QUrl downloadUrl(metadata.value(QStringLiteral("download_url"))
                    .toString());
                static const QRegularExpression versionPattern(
                    QStringLiteral("^[0-9]+\\.[0-9]+\\.[0-9]+(?:[-+][A-Za-z0-9.-]+)?$"));
                static const QRegularExpression hashPattern(
                    QStringLiteral("^[0-9A-Fa-f]{64}$"));
                static const QRegularExpression artifactPathPattern(
                    QStringLiteral("^/artifact/[0-9]+/authlib-injector-[A-Za-z0-9.+-]+\\.jar$"));
                if (!versionPattern.match(version).hasMatch()
                    || !hashPattern.match(sha256).hasMatch()
                    || !downloadUrl.isValid()
                    || downloadUrl.scheme() != QStringLiteral("https")
                    || downloadUrl.host() != kAuthlibInjectorHost
                    || !artifactPathPattern.match(downloadUrl.path()).hasMatch()
                    || !downloadUrl.userInfo().isEmpty()
                    || !downloadUrl.query().isEmpty()
                    || !downloadUrl.fragment().isEmpty()) {
                    fail(QStringLiteral(
                        "La información del instalador de autenticación no pasó la validación."));
                    return;
                }

                m_authlibInjectorPath = QStringLiteral(
                    "libraries/cub3/authlib-injector/%1/authlib-injector.jar")
                    .arg(version);
                m_authlibInjectorHash = sha256.toLower();
                m_authlibInjectorUrl = downloadUrl.toString(QUrl::FullyEncoded);
                resolveVersionFiles();
            });
        });
}

void LaunchEngine::resolveVersionFiles() {
    if (m_failed)
        return;
    m_versions->resolveVersion(m_versionId,
        [this](const QList<versions::VersionManager::FileDescriptor>& files) {
        if (m_failed)
            return;
        QList<versions::VersionManager::FileDescriptor> allFiles = files;
        if (!m_authlibInjectorPath.isEmpty()) {
            versions::VersionManager::FileDescriptor injector;
            injector.url = m_authlibInjectorUrl;
            injector.path = m_authlibInjectorPath;
            injector.hash = m_authlibInjectorHash;
            injector.hashAlgorithm = QStringLiteral("sha256");
            allFiles.append(std::move(injector));
        }
        beginInstall(allFiles);
    });
}

void LaunchEngine::beginInstall(
    const QList<versions::VersionManager::FileDescriptor>& files) {
    if (m_failed)
        return;
    if (files.isEmpty()) {
        fail(QStringLiteral("La versión no contiene archivos descargables."));
        return;
    }

    m_files = files;
    QList<network::DownloadJob> jobs;
    QSet<QString> targets;
    for (const auto& file : m_files) {
        const QString target = QDir::cleanPath(
            QDir(core::Paths::dataDir()).filePath(file.path));
        if (targets.contains(target))
            continue;
        targets.insert(target);

        if (file.url.isEmpty() || file.path.isEmpty()) {
            fail(QStringLiteral("El manifiesto contiene una descarga incompleta."));
            return;
        }
        if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
            fail(QStringLiteral("No se pudo crear el directorio de instalación."));
            return;
        }

        network::DownloadJob job;
        job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        job.url = QUrl(file.url);
        job.targetPath = target;
        job.expectedHash = file.hash;
        job.hashAlgorithm = file.hashAlgorithm;
        job.expectedSize = file.size;
        job.maxRetries = 3;
        m_pendingIds.insert(job.id);
        jobs.append(std::move(job));
    }

    if (jobs.isEmpty()) {
        fail(QStringLiteral("No se encontraron archivos de instalación."));
        return;
    }

    emit statusChanged(QStringLiteral("Descargando archivos del juego..."));
    for (const auto& job : jobs) {
        if (m_downloads->submit(job).isEmpty()) {
            fail(QStringLiteral("No se pudo poner una descarga en cola."));
            return;
        }
    }
}

void LaunchEngine::onDownloadFinished(const QString& id,
                                      network::DownloadStatus status,
                                      const QString& error) {
    if (!m_pendingIds.remove(id) || m_failed)
        return;

    if (status != network::DownloadStatus::Completed
        && status != network::DownloadStatus::Skipped) {
        fail(error.isEmpty()
            ? QStringLiteral("Falló una descarga necesaria para iniciar el juego.")
            : error);
        return;
    }

    if (m_pendingIds.isEmpty())
        startGame();
    else
        emit statusChanged(QStringLiteral("Descargando: %1 archivo(s) restante(s)...")
                               .arg(m_pendingIds.size()));
}

QStringList LaunchEngine::manifestArguments(
    const QJsonArray& entries,
    const QHash<QString, QString>& values) const
{
    QStringList result;
    for (const QJsonValue& entry : entries) {
        QStringList arguments;
        if (entry.isString()) {
            arguments.append(entry.toString());
        } else if (entry.isObject()) {
            const QJsonObject object = entry.toObject();
            if (!matchesRules(object.value(QStringLiteral("rules")).toArray()))
                continue;
            const QJsonValue argument = object.value(QStringLiteral("value"));
            if (argument.isString())
                arguments.append(argument.toString());
            else if (argument.isArray()) {
                for (const QJsonValue& value : argument.toArray()) {
                    if (value.isString())
                        arguments.append(value.toString());
                }
            }
        }
        for (const QString& argument : arguments)
            result.append(substitute(argument, values));
    }
    return result;
}

void LaunchEngine::startGame() {
    if (m_failed)
        return;

    const QJsonObject version = m_manifest;
    const int requiredJava = version.value(QStringLiteral("javaVersion"))
        .toObject().value(QStringLiteral("majorVersion")).toInt();
    const QString java = compatibleJava(m_settings->javaPath(), requiredJava);
    if (java.isEmpty()) {
        const QString requirement = requiredJava > 0
            ? QStringLiteral("Minecraft %1 requiere Java %2.")
                  .arg(m_versionId).arg(requiredJava)
            : QStringLiteral("No se encontró una instalación de Java ejecutable.");
        fail(QStringLiteral("%1 Instala esa versión de Java o selecciónala en Ajustes.")
                 .arg(requirement));
        return;
    }
    if (m_settings->javaPath() != java)
        m_settings->setJavaPath(java);
    const QString configuredInstances = m_settings->instancesDir().trimmed();
    const QString instancesDirectory = configuredInstances.isEmpty()
        ? core::Paths::instancesDir() : configuredInstances;
    const QString gameDirectory = QDir(instancesDirectory).filePath(m_versionId);
    const QString versionDirectory = QDir(core::Paths::versionsDir())
        .filePath(m_versionId);
    const QString nativesDirectory = QDir(versionDirectory)
        .filePath(m_versionId + QStringLiteral("-natives"));
    if (!QDir().mkpath(gameDirectory) || !QDir().mkpath(nativesDirectory)) {
        fail(QStringLiteral("No se pudieron crear las carpetas de Minecraft."));
        return;
    }
    const bool cppEditionTitleEnabled = m_versionId == QStringLiteral("26.3")
        && m_settings->get(QStringLiteral("game/cpp_edition_title"), true).toBool();
    QString titlePackError;
    if (!CppEditionPack::configure(gameDirectory, cppEditionTitleEnabled,
                                   &titlePackError)) {
        fail(QStringLiteral("No se pudo preparar la personalización del título: %1")
                 .arg(titlePackError));
        return;
    }

#ifdef CUB3_PLATFORM_WINDOWS
    // Windows 10 1803+ and Windows 11 include bsdtar, which can unpack ZIPs.
    // Keep the Linux path using unzip so the existing Linux packaging is unchanged.
    const QString archiveTool = QStandardPaths::findExecutable(QStringLiteral("tar.exe"));
#else
    const QString archiveTool = QStandardPaths::findExecutable(QStringLiteral("unzip"));
#endif
    for (const auto& file : m_files) {
        if (!file.native)
            continue;
        if (archiveTool.isEmpty()) {
#ifdef CUB3_PLATFORM_WINDOWS
            fail(QStringLiteral("No se encontró tar.exe. Se requiere Windows 10 versión 1809 o posterior para preparar las bibliotecas nativas."));
#else
            fail(QStringLiteral("Se necesita unzip para preparar las bibliotecas nativas."));
#endif
            return;
        }
        const QString archive = QDir(core::Paths::dataDir()).filePath(file.path);
#ifdef CUB3_PLATFORM_WINDOWS
        const QStringList extractArguments{
            QStringLiteral("-xf"), archive, QStringLiteral("-C"), nativesDirectory};
#else
        const QStringList extractArguments{
            QStringLiteral("-oq"), archive, QStringLiteral("-d"), nativesDirectory};
#endif
        if (QProcess::execute(archiveTool, extractArguments) != 0) {
            fail(QStringLiteral("No se pudieron extraer las bibliotecas nativas."));
            return;
        }
    }

    QStringList classPath;
    QStringList jvmArguments{
        QStringLiteral("-Xms%1m").arg(qMax(512, m_settings->ramMinMB())),
        QStringLiteral("-Xmx%1m").arg(qMax(m_settings->ramMinMB(),
                                            m_settings->ramMaxMB())),
        QStringLiteral("-Djava.library.path=%1").arg(nativesDirectory)
    };
    if (m_account.provider == auth::Provider::Elyby) {
        const QString injectorJar = QDir(core::Paths::dataDir())
            .filePath(m_authlibInjectorPath);
        if (m_authlibInjectorPath.isEmpty()
            || !QFileInfo(injectorJar).isFile()) {
            fail(QStringLiteral("Falta el componente de autenticación Ely.by."));
            return;
        }
        jvmArguments.prepend(QStringLiteral("-javaagent:%1=%2")
            .arg(injectorJar, kElybyAuthlibUrl));
    }

    for (const auto& file : m_files) {
        if (!file.native && file.path.startsWith(QStringLiteral("libraries/"))
            && file.path.endsWith(QStringLiteral(".jar"))
            && file.path != m_authlibInjectorPath) {
            classPath.append(QDir(core::Paths::dataDir()).filePath(file.path));
        }
    }

    const QString clientJar = QDir(core::Paths::dataDir())
        .filePath(QStringLiteral("versions/%1/%1.jar").arg(m_versionId));
    if (!QFileInfo::exists(clientJar)) {
        fail(QStringLiteral("Falta el cliente de Minecraft para esta versión."));
        return;
    }
    classPath.append(clientJar);

    const QString assetsIndex = version.value(QStringLiteral("assets")).toString();
    QHash<QString, QString> values{
        {QStringLiteral("auth_player_name"), m_account.username},
        {QStringLiteral("version_name"), m_versionId},
        {QStringLiteral("game_directory"), gameDirectory},
        {QStringLiteral("assets_root"), core::Paths::assetsDir()},
        {QStringLiteral("assets_index_name"), assetsIndex},
        {QStringLiteral("auth_uuid"), m_account.uuid},
        {QStringLiteral("auth_access_token"), m_account.accessToken},
        {QStringLiteral("user_type"),
         m_account.provider == auth::Provider::Microsoft
             ? QStringLiteral("msa") : QStringLiteral("legacy")},
        {QStringLiteral("version_type"),
         version.value(QStringLiteral("type")).toString(QStringLiteral("release"))},
        {QStringLiteral("user_properties"), QStringLiteral("{}")},
        {QStringLiteral("natives_directory"), nativesDirectory},
        {QStringLiteral("classpath"), classPath.join(QDir::listSeparator())},
        {QStringLiteral("launcher_name"), QStringLiteral("CUB3")},
        {QStringLiteral("launcher_version"), QStringLiteral("0.1.0")},
        {QStringLiteral("resolution_width"),
         QString::number(m_settings->resolutionWidth())},
        {QStringLiteral("resolution_height"),
         QString::number(m_settings->resolutionHeight())},
        {QStringLiteral("auth_xuid"), QString()},
        {QStringLiteral("clientid"), QString()}
    };

    if (assetsIndex.isEmpty()) {
        fail(QStringLiteral("El manifiesto no contiene un índice de recursos."));
        return;
    }

    const QJsonObject logging = version.value(QStringLiteral("logging"))
        .toObject().value(QStringLiteral("client")).toObject();
    if (logging.value(QStringLiteral("argument")).isString()) {
        QHash<QString, QString> loggingValues = values;
        loggingValues.insert(QStringLiteral("path"),
            QDir(core::Paths::assetsDir()).filePath(
                QStringLiteral("log_configs/%1").arg(
                    logging.value(QStringLiteral("file")).toObject()
                        .value(QStringLiteral("id")).toString())));
        jvmArguments.append(substitute(
            logging.value(QStringLiteral("argument")).toString(), loggingValues));
    }

    const QJsonObject arguments = version.value(QStringLiteral("arguments")).toObject();
    if (arguments.value(QStringLiteral("jvm")).isArray()) {
        jvmArguments.append(manifestArguments(
            arguments.value(QStringLiteral("jvm")).toArray(), values));
    }

    QStringList gameArguments;
    if (arguments.value(QStringLiteral("game")).isArray()) {
        gameArguments = manifestArguments(
            arguments.value(QStringLiteral("game")).toArray(), values);
    } else if (version.value(QStringLiteral("minecraftArguments")).isString()) {
        gameArguments = QProcess::splitCommand(
            substitute(version.value(QStringLiteral("minecraftArguments")).toString(), values));
    } else {
        fail(QStringLiteral("El manifiesto no contiene argumentos compatibles."));
        return;
    }

    const QString mainClass = version.value(QStringLiteral("mainClass")).toString();
    if (mainClass.isEmpty()) {
        fail(QStringLiteral("El manifiesto no especifica la clase principal."));
        return;
    }
    if (!jvmArguments.contains(QStringLiteral("-cp"))
        && !jvmArguments.contains(QStringLiteral("-classpath"))) {
        jvmArguments << QStringLiteral("-cp")
                     << values.value(QStringLiteral("classpath"));
    }

    QStringList processArguments = jvmArguments;
    processArguments << mainClass;
    processArguments.append(gameArguments);

    m_process = new QProcess(this);
    m_process->setProgram(java);
    m_process->setArguments(processArguments);
    m_process->setWorkingDirectory(gameDirectory);
    const QString logDirectory = QDir(gameDirectory).filePath(QStringLiteral("logs"));
    if (!QDir().mkpath(logDirectory)) {
        fail(QStringLiteral("No se pudo crear la carpeta de registros de Minecraft."));
        m_process->deleteLater();
        m_process = nullptr;
        return;
    }
    const QString launchLog = QDir(logDirectory).filePath(
        QStringLiteral("cub3-launcher.log"));
    m_process->setStandardOutputFile(launchLog, QIODevice::Append);
    m_process->setStandardErrorFile(launchLog, QIODevice::Append);
    connect(m_process, &QProcess::started, this, [this] {
        m_launching = false;
        saveInstallation(m_process->workingDirectory());
        emit gameStarted(m_versionId);
        emit statusChanged(QStringLiteral("Minecraft está ejecutándose."));
    });
    connect(m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            fail(QStringLiteral("No se pudo iniciar Java. Revisa la ruta en Ajustes."));
            m_process->deleteLater();
            m_process = nullptr;
        }
    });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, launchLog](int exitCode, QProcess::ExitStatus) {
        emit gameFinished(exitCode);
        m_launching = false;
        emit statusChanged(exitCode == 0
            ? QStringLiteral("Minecraft se cerró correctamente.")
            : QStringLiteral("Minecraft terminó con código %1.").arg(exitCode));
        if (exitCode != 0) {
            emit launchFailed(QStringLiteral(
                "Minecraft se cerró con código %1. Revisa el registro: %2")
                    .arg(exitCode).arg(launchLog));
        }
        m_process->deleteLater();
        m_process = nullptr;
    });

    emit statusChanged(QStringLiteral("Iniciando Minecraft %1...").arg(m_versionId));
    m_process->start();
}

void LaunchEngine::saveInstallation(const QString& directory) {
    if (!m_database || !m_database->isOpen())
        return;

    const QVariant existingId = m_database->queryValue(
        QStringLiteral("SELECT id FROM installations WHERE version = ? AND directory = ? LIMIT 1"),
        {m_versionId, directory});
    if (existingId.isValid()) {
        const QSqlQuery update = m_database->exec(
            QStringLiteral("UPDATE installations SET name = ?, loader = 'vanilla', "
                           "java_path = ?, ram_min = ?, ram_max = ?, resolution_w = ?, "
                           "resolution_h = ?, updated_at = strftime('%s', 'now') WHERE id = ?"),
            {m_versionId, m_settings->javaPath(), m_settings->ramMinMB(),
             m_settings->ramMaxMB(), m_settings->resolutionWidth(),
             m_settings->resolutionHeight(), existingId});
        if (!update.isActive())
            return;
        return;
    }

    const QSqlQuery insert = m_database->exec(
        QStringLiteral("INSERT INTO installations "
                       "(name, version, loader, directory, java_path, ram_min, ram_max, "
                       "resolution_w, resolution_h) "
                       "VALUES (?, ?, 'vanilla', ?, ?, ?, ?, ?, ?)"),
        {m_versionId, m_versionId, directory, m_settings->javaPath(),
         m_settings->ramMinMB(), m_settings->ramMaxMB(),
         m_settings->resolutionWidth(), m_settings->resolutionHeight()});
    Q_UNUSED(insert);
}

void LaunchEngine::fail(const QString& message) {
    if (m_failed)
        return;
    m_failed = true;
    m_launching = false;
    const QSet<QString> downloads = m_pendingIds;
    m_pendingIds.clear();
    for (const QString& id : downloads)
        m_downloads->cancel(id);
    emit statusChanged(QStringLiteral("Error al iniciar el juego."));
    emit launchFailed(message);
}

} // namespace cub3::application
