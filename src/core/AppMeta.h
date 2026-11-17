// CUB3 Launcher - Application metadata
#pragma once

#include <QString>
#include <QStringView>

namespace cub3::core {

/// Compile-time application metadata. No runtime construction needed.
namespace meta {
    inline constexpr QStringView kName        = u"CUB3 Launcher";
    inline constexpr QStringView kOrgName     = u"CUB3";
    inline constexpr QStringView kOrgDomain   = u"cub3.launcher";
    inline constexpr QStringView kVersionStr  = u"0.1.0";
    inline constexpr int         kVersionMajor = 0;
    inline constexpr int         kVersionMinor = 1;
    inline constexpr int         kVersionPatch = 0;

    /// User-Agent string used for all HTTP requests.
    inline const QString userAgent() {
        return QString("CUB3-Launcher/%1").arg(kVersionStr.toString());
    }

    /// Display string for titles and about dialogs.
    inline const QString displayName() {
        return QString("CUB3");
    }

    /// Microsoft OAuth client ID (placeholder — must be replaced by the
    /// developer's own Azure application registration before distribution).
    inline constexpr QStringView kMicrosoftClientId =
        u"00000000-0000-0000-0000-000000000000";

    /// Ely.by API base URL.
    inline constexpr QStringView kElybyApiBase =
        u"https://authserver.ely.by";
} // namespace meta

} // namespace cub3::core
