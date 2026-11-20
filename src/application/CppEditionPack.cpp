// CUB3 Launcher - opt-in Minecraft title-screen branding resource pack
#include "CppEditionPack.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>

namespace cub3::application {
namespace {
const QString kPackFileName = QStringLiteral("CUB3-Cpp-Edition.zip");
const QString kOptionsFileName = QStringLiteral("options.txt");
const QByteArray kOptionsKey = QByteArrayLiteral("resourcePacks:");
const QString kEmbeddedPackPath = QStringLiteral(":/packs/CUB3-Cpp-Edition.zip");

bool writeAtomically(const QString& path, const QByteArray& bytes, QString* error) {
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("No se pudo abrir %1 para guardar cambios.").arg(path);
        return false;
    }
    if (output.write(bytes) != bytes.size()) {
        if (error)
            *error = QStringLiteral("No se pudieron escribir todos los datos en %1.").arg(path);
        output.cancelWriting();
        return false;
    }
    if (!output.commit()) {
        if (error)
            *error = QStringLiteral("No se pudo guardar %1 de forma segura.").arg(path);
        return false;
    }
    return true;
}
}

bool CppEditionPack::configure(const QString& gameDirectory,
                               bool enabled,
                               QString* error) {
    const QString optionsPath = QDir(gameDirectory).filePath(kOptionsFileName);
    QByteArray options;
    QFile optionsFile(optionsPath);
    const bool optionsExisted = optionsFile.exists();
    if (optionsExisted) {
        if (!optionsFile.open(QIODevice::ReadOnly)) {
            if (error)
                *error = QStringLiteral("No se pudieron leer los ajustes de Minecraft.");
            return false;
        }
        options = optionsFile.readAll();
        if (optionsFile.error() != QFileDevice::NoError) {
            if (error)
                *error = QStringLiteral("No se pudieron leer por completo los ajustes de Minecraft.");
            return false;
        }
    }

    if (enabled) {
        const QString packDirectory = QDir(gameDirectory).filePath(
            QStringLiteral("resourcepacks"));
        if (!QDir().mkpath(packDirectory)) {
            if (error)
                *error = QStringLiteral("No se pudo crear la carpeta de paquetes de recursos.");
            return false;
        }
        const QString packPath = QDir(packDirectory).filePath(kPackFileName);
        QFile embedded(kEmbeddedPackPath);
        if (!embedded.open(QIODevice::ReadOnly)) {
            if (error)
                *error = QStringLiteral("No se encontró el paquete C++ Edition integrado en CUB3.");
            return false;
        }
        const QByteArray packBytes = embedded.readAll();
        if (embedded.error() != QFileDevice::NoError) {
            if (error)
                *error = QStringLiteral("No se pudo leer el paquete C++ Edition integrado en CUB3.");
            return false;
        }
        QFile current(packPath);
        if (!current.exists() || !current.open(QIODevice::ReadOnly)
            || current.readAll() != packBytes) {
            if (!writeAtomically(packPath, packBytes, error))
                return false;
        }
    }

    if (!updateSelection(options, enabled, error))
        return false;
    if (!enabled && !optionsExisted && options.isEmpty())
        return true;
    return writeAtomically(optionsPath, options, error);
}

bool CppEditionPack::updateSelection(QByteArray& options,
                                     bool enabled,
                                     QString* error) {
    QList<QByteArray> lines = options.split('\n');
    int foundLine = -1;
    QJsonArray packs;
    for (int index = 0; index < lines.size(); ++index) {
        QByteArray line = lines.at(index);
        if (line.endsWith('\r'))
            line.chop(1);
        if (!line.startsWith(kOptionsKey))
            continue;
        if (foundLine >= 0) {
            if (error)
                *error = QStringLiteral("Minecraft tiene más de una opción resourcePacks; no se modificó el archivo.");
            return false;
        }
        foundLine = index;
        QJsonParseError parseError;
        const QJsonDocument parsed = QJsonDocument::fromJson(
            line.mid(kOptionsKey.size()), &parseError);
        if (parseError.error != QJsonParseError::NoError || !parsed.isArray()) {
            if (error)
                *error = QStringLiteral("La lista de paquetes de recursos en options.txt no es válida; no se modificó el archivo.");
            return false;
        }
        packs = parsed.array();
    }

    QJsonArray updated;
    bool alreadyPresent = false;
    for (const QJsonValue& value : packs) {
        if (!value.isString()) {
            if (error)
                *error = QStringLiteral("La lista de paquetes de recursos contiene una entrada no válida.");
            return false;
        }
        if (value.toString() == QString::fromLatin1(kResourcePackId)) {
            if (enabled && !alreadyPresent) {
                updated.append(value);
                alreadyPresent = true;
            }
            continue;
        }
        updated.append(value);
    }
    if (enabled && !alreadyPresent)
        updated.append(QString::fromLatin1(kResourcePackId));

    if (foundLine < 0) {
        if (!enabled)
            return true;
        if (!options.isEmpty() && !options.endsWith('\n'))
            options.append('\n');
        options.append(kOptionsKey);
        options.append(QJsonDocument(updated).toJson(QJsonDocument::Compact));
        options.append('\n');
        return true;
    }

    QByteArray& originalLine = lines[foundLine];
    const bool hasCarriageReturn = originalLine.endsWith('\r');
    if (hasCarriageReturn)
        originalLine.chop(1);
    originalLine = kOptionsKey + QJsonDocument(updated).toJson(QJsonDocument::Compact);
    if (hasCarriageReturn)
        originalLine.append('\r');
    options = lines.join('\n');
    return true;
}

} // namespace cub3::application
