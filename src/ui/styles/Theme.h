// CUB3 Launcher - Visual theme constants
#pragma once

#include <QColor>
#include <QString>

namespace cub3::ui {

/// All visual constants live here. No magic numbers scattered across widgets.
/// Colours are defined once and reused via these constants.
namespace theme {

// --- Colour palette -------------------------------------------------------
// Deep navy surfaces with warm green accents.

namespace colors {
    // Backgrounds (dark, layered)
    inline const QColor bgPrimary     {0x0D, 0x13, 0x18};  // Main canvas
    inline const QColor bgSecondary   {0x13, 0x1C, 0x22};  // Sidebar and cards
    inline const QColor bgTertiary    {0x1A, 0x26, 0x2B};  // Hover surfaces
    inline const QColor bgRaised       {0x23, 0x32, 0x36};  // Active controls

    // Borders / dividers
    inline const QColor border         {0x29, 0x3A, 0x3D};
    inline const QColor borderHover     {0x3A, 0x55, 0x50};

    // Text
    inline const QColor textPrimary    {0xF1, 0xF5, 0xF2};
    inline const QColor textSecondary   {0xA9, 0xB9, 0xB3};
    inline const QColor textDisabled    {0x72, 0x83, 0x7D};

    // Restrained CUB3 green accent
    inline const QColor accent          {0x48, 0xC9, 0x70};
    inline const QColor accentHover     {0x67, 0xDB, 0x88};
    inline const QColor accentPressed   {0x35, 0xA9, 0x5B};
    inline const QColor accentDim       {0x48, 0xC9, 0x70, 0x30};

    // Status
    inline const QColor success         {0x3A, 0xD1, 0x5E};
    inline const QColor warning         {0xFF, 0xB0, 0x20};
    inline const QColor error           {0xFF, 0x5C, 0x5C};
    inline const QColor info            {0x4D, 0xA8, 0xFF};

    // Skin / player placeholders
    inline const QColor skinBg          {0x2A, 0x2A, 0x36};
} // namespace colors

// --- Sizing ---------------------------------------------------------------
namespace sizes {
    // Window
    inline constexpr int kWindowWidth   = 1280;
    inline constexpr int kWindowHeight   = 760;
    inline constexpr int kWindowMinWidth  = 940;
    inline constexpr int kWindowMinHeight = 640;

    // Sidebar
    inline constexpr int kSidebarWidth   = 218;

    // Play button
    inline constexpr int kPlayButtonW    = 200;
    inline constexpr int kPlayButtonH    = 56;

    // Spacing
    inline constexpr int kSpacingSmall   = 8;
    inline constexpr int kSpacingMedium  = 16;
    inline constexpr int kSpacingLarge   = 24;
    inline constexpr int kSpacingXLarge  = 32;

    // Corner radius
    inline constexpr int kRadiusSmall    = 4;
    inline constexpr int kRadiusMedium   = 8;
    inline constexpr int kRadiusLarge    = 12;

    // Icon sizes
    inline constexpr int kIconSmall      = 16;
    inline constexpr int kIconMedium     = 24;
    inline constexpr int kIconLarge      = 32;
} // namespace sizes

// --- Animation ------------------------------------------------------------
namespace anim {
    inline constexpr int kFastMs    = 100;   // Hover, press
    inline constexpr int kMediumMs   = 200;   // Page transitions
    inline constexpr int kSlowMs     = 300;   // Complex transitions

    // Animation easing curve name (used in QSS where supported)
    inline constexpr const char* kEasing = "OutCubic";
} // namespace anim

// --- Font -----------------------------------------------------------------
namespace fonts {
    inline constexpr const char* kPrimaryFamily   = "Segoe UI";       // Windows default
    inline constexpr const char* kFallbackFamily  = "Sans Serif";      // Qt fallback
    inline constexpr const char* kLinuxFamily      = "DejaVu Sans";    // Linux default

    inline constexpr int kSizeSmall   = 12;
    inline constexpr int kSizeBody    = 14;
    inline constexpr int kSizeTitle   = 18;
    inline constexpr int kSizeHeading = 24;
    inline constexpr int kSizeLarge   = 32;

    inline constexpr int kWeightRegular = 400;
    inline constexpr int kWeightMedium  = 500;
    inline constexpr int kWeightBold     = 700;
} // namespace fonts

// --- Helper: colour to rgba() CSS string --------------------------------
inline QString colorToRgba(const QColor& c) {
    return QString("rgba(%1, %2, %3, %4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alphaF());
}

/// Returns the full QSS stylesheet. The stylesheet is generated from
/// the theme constants so colours and sizes stay in sync.
[[nodiscard]] QString stylesheet();

/// Returns the best font family for the current platform.
[[nodiscard]] QString fontFamily();

} // namespace cub3::ui::theme

} // namespace cub3::ui
