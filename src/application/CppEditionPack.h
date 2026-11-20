// CUB3 Launcher - opt-in Minecraft title-screen branding resource pack
#pragma once

#include <QByteArray>
#include <QString>

namespace cub3::application {

class CppEditionPack final {
public:
    static constexpr auto kResourcePackId = "file/CUB3-Cpp-Edition.zip";

    /// Install the bundled, original title texture and add/remove it from the
    /// selected resource packs in the instance options file.
    [[nodiscard]] static bool configure(const QString& gameDirectory,
                                        bool enabled,
                                        QString* error = nullptr);

    /// Rewrites only the resourcePacks option; other game options are preserved.
    [[nodiscard]] static bool updateSelection(QByteArray& options,
                                              bool enabled,
                                              QString* error = nullptr);
};

} // namespace cub3::application
