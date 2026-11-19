#pragma once

#include <QFileInfo>
#include <QRegularExpression>
#include <QString>

namespace cub3::ui::pages::modsValidation {

inline bool isMinecraftVersion(const QString& value) {
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._+-]{0,31}$"));
    return pattern.match(value.trimmed()).hasMatch();
}

inline bool isModrinthProjectId(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9]{8}$"));
    return pattern.match(value).hasMatch();
}

inline QString safeJarFileName(const QString& value) {
    const QString fileName = QFileInfo(value).fileName();
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._()+ -]{0,180}\\.jar$"),
        QRegularExpression::CaseInsensitiveOption);
    return pattern.match(fileName).hasMatch() ? fileName : QString{};
}

} // namespace cub3::ui::pages::modsValidation
