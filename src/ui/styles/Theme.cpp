// CUB3 Launcher - Theme implementation
#include "Theme.h"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>

namespace cub3::ui::theme {

QString fontFamily() {
#ifdef CUB3_PLATFORM_WINDOWS
    return QString(fonts::kPrimaryFamily);
#elif defined(CUB3_PLATFORM_LINUX)
    return QString(fonts::kLinuxFamily);
#else
    return QString(fonts::kFallbackFamily);
#endif
}

QString stylesheet() {
    // Build the stylesheet using simple token replacement.
    // Tokens are written as {{token}} in the template and replaced
    // with the actual colour values. This avoids QString::arg() limitations.
    static const QString tmpl = R"EOF(
/* ===== Global ===== */
QWidget {
    background-color: {{bgPrimary}};
    color: {{textPrimary}};
    font-family: "{{font}}";
    font-size: 14px;
}

/* ===== MainWindow ===== */
QMainWindow { background-color: {{bgPrimary}}; }

/* ===== Sidebar ===== */
#Sidebar {
    background-color: {{bgSecondary}};
    border-right: 1px solid {{border}};
}
#SidebarLogo {
    color: {{textPrimary}};
    font-size: 21px;
    font-weight: 700;
    padding: 22px 18px 4px 18px;
}
#SidebarSubtitle {
    color: {{textSecondary}};
    font-size: 11px;
    padding: 0 18px 18px 18px;
}

/* ===== Sidebar buttons ===== */
QPushButton#SidebarButton {
    background-color: transparent;
    border: 1px solid transparent;
    text-align: left;
    padding: 10px 12px;
    margin: 2px 10px;
    color: {{textSecondary}};
    font-size: 14px;
    font-weight: 500;
    border-radius: 8px;
}
QPushButton#SidebarButton:hover {
    background-color: {{bgTertiary}};
    color: {{textPrimary}};
}
QPushButton#SidebarButton:checked {
    background-color: {{bgRaised}};
    color: {{textPrimary}};
    border-left: 3px solid {{accent}};
}
#SidebarFooter {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 9px;
    color: {{textSecondary}};
    font-size: 12px;
    padding: 11px 12px;
    margin: 0 12px;
}

/* ===== Pages / content area ===== */
#ContentArea { background-color: {{bgPrimary}}; }
#PageTitle {
    font-size: 24px;
    font-weight: 700;
    color: {{textPrimary}};
    padding: 0px 0px 8px 0px;
}
#PageSubtitle {
    font-size: 14px;
    color: {{textSecondary}};
    padding-bottom: 16px;
}

/* ===== Cards ===== */
QFrame#Card, QFrame#InfoCard {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 12px;
}
QFrame#Card:hover, QFrame#InfoCard:hover { border-color: {{borderHover}}; }

/* ===== Dashboard ===== */
QWidget#DashboardBody { background-color: {{bgPrimary}}; }
QWidget#SunsetHero { border-radius: 16px; }
QLabel#SearchGlyph {
    background: transparent;
    color: #E8EDF5;
    font-size: 26px;
    padding: 0 2px;
}
QLineEdit#SearchField {
    background-color: rgba(15, 20, 33, 215);
    border: 1px solid rgba(255, 255, 255, 45);
    border-radius: 9px;
    color: #FFFFFF;
    padding: 10px 13px;
    min-height: 20px;
}
QLineEdit#SearchField:focus { border-color: {{accent}}; }
QLabel#AccountBadge {
    background-color: rgba(15, 20, 33, 205);
    border: 1px solid rgba(255, 255, 255, 45);
    border-radius: 9px;
    color: #F1F4F8;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1px;
    padding: 10px 15px;
}
QLabel#HeroEyebrow {
    background: transparent;
    color: #B8E5BE;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 2px;
    padding-bottom: 5px;
}
QLabel#HeroTitle {
    background: transparent;
    color: #FFFFFF;
    font-size: 32px;
    font-weight: 700;
    padding-bottom: 5px;
}
QLabel#HeroCaption {
    background: transparent;
    color: #E2E4EB;
    font-size: 14px;
}
QComboBox#VersionSelector {
    background-color: rgba(17, 23, 36, 235);
    border: 1px solid rgba(255, 255, 255, 48);
    border-radius: 9px;
    color: #FFFFFF;
    padding: 9px 14px;
    font-weight: 600;
}
QComboBox#VersionSelector QAbstractItemView {
    background-color: {{bgSecondary}};
    selection-background-color: {{bgRaised}};
    color: {{textPrimary}};
}
QLabel#CardTitle {
    color: {{textPrimary}};
    font-size: 15px;
    font-weight: 700;
}
QFrame#FeatureArt {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 9px;
}
QLabel#FeatureTitle {
    background: transparent;
    color: {{textPrimary}};
    font-size: 14px;
    font-weight: 700;
}
QLabel#CardCaption {
    background: transparent;
    color: {{textSecondary}};
    font-size: 12px;
}
QLabel#MetricValue {
    background: transparent;
    color: {{textPrimary}};
    font-size: 14px;
    font-weight: 700;
}
QPushButton#SecondaryButton {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 7px;
    color: {{textPrimary}};
    padding: 7px 12px;
}
QPushButton#SecondaryButton:hover {
    background-color: {{bgRaised}};
    border-color: {{borderHover}};
}
QLabel#FooterStatus, QLabel#FooterCaption {
    background: transparent;
    color: {{textSecondary}};
    font-size: 11px;
}
QLabel#FooterCaption { letter-spacing: 1px; }

/* ===== Play button ===== */
QPushButton#PlayButton {
    background-color: {{accent}};
    color: #FFFFFF;
    border: none;
    border-radius: 8px;
    font-size: 18px;
    font-weight: 700;
    padding: 16px 48px;
}
QPushButton#PlayButton:hover { background-color: {{accentHover}}; }
QPushButton#PlayButton:pressed { background-color: {{accentPressed}}; }
QPushButton#PlayButton:disabled {
    background-color: {{bgTertiary}};
    color: {{textDisabled}};
}

/* ===== Standard buttons ===== */
QPushButton {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 6px;
    color: {{textPrimary}};
    padding: 8px 16px;
    font-size: 14px;
}
QPushButton:hover {
    background-color: {{bgRaised}};
    border-color: {{borderHover}};
}
QPushButton:pressed { background-color: {{bgSecondary}}; }
QPushButton:disabled {
    color: {{textDisabled}};
    background-color: {{bgSecondary}};
}
QPushButton#Primary {
    background-color: {{accent}};
    border: none;
    color: #FFFFFF;
}
QPushButton#Primary:hover { background-color: {{accentHover}}; }
QPushButton#Primary:pressed { background-color: {{accentPressed}}; }

/* ===== Inputs ===== */
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 6px;
    color: {{textPrimary}};
    padding: 8px 12px;
    font-size: 14px;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: {{accent}}; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox::down-arrow { image: none; }
QComboBox QAbstractItemView {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 6px;
    selection-background-color: {{bgRaised}};
    color: {{textPrimary}};
}

/* ===== Sliders ===== */
QSlider::groove:horizontal {
    height: 4px;
    background: {{border}};
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: {{accent}};
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}
QSlider::handle:horizontal:hover { background: {{accentHover}}; }

/* ===== Progress bar ===== */
QProgressBar {
    background-color: {{bgTertiary}};
    border: none;
    border-radius: 4px;
    text-align: center;
    color: {{textPrimary}};
    font-size: 12px;
}
QProgressBar::chunk {
    background-color: {{accent}};
    border-radius: 4px;
}

/* ===== Scroll area ===== */
QScrollArea { border: none; background-color: {{bgPrimary}}; }
QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }
QScrollBar::handle:vertical {
    background: {{border}};
    border-radius: 4px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover { background: {{borderHover}}; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 8px; margin: 0; }
QScrollBar::handle:horizontal {
    background: {{border}};
    border-radius: 4px;
    min-width: 30px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ===== Labels ===== */
QLabel { color: {{textPrimary}}; background: transparent; }
QLabel#Secondary { color: {{textSecondary}}; font-size: 13px; }
QLabel#Title { color: {{textPrimary}}; font-size: 18px; font-weight: 700; }
QLabel#Heading { color: {{textPrimary}}; font-size: 24px; font-weight: 700; }
QLabel#Value { color: {{textPrimary}}; font-size: 16px; font-weight: 500; }
QLabel#Caption { color: {{textDisabled}}; font-size: 12px; }

/* ===== Group box ===== */
QGroupBox {
    border: 1px solid {{border}};
    border-radius: 8px;
    margin-top: 16px;
    padding-top: 16px;
    color: {{textPrimary}};
    font-weight: 500;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 8px;
    color: {{textSecondary}};
}

/* ===== Tabs ===== */
QTabWidget::pane {
    border: 1px solid {{border}};
    border-radius: 8px;
    background-color: {{bgPrimary}};
}
QTabBar::tab {
    background-color: transparent;
    color: {{textSecondary}};
    padding: 8px 20px;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
    color: {{textPrimary}};
    border-bottom: 2px solid {{accent}};
}
QTabBar::tab:hover { color: {{textPrimary}}; }

/* ===== List view ===== */
QListView {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 8px;
    color: {{textPrimary}};
}
QListView::item { padding: 8px 12px; border-radius: 4px; }
QListView::item:selected { background-color: {{bgRaised}}; }

/* ===== Checkboxes ===== */
QCheckBox { color: {{textPrimary}}; spacing: 8px; }
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid {{border}};
    border-radius: 3px;
    background-color: {{bgSecondary}};
}
QCheckBox::indicator:checked {
    background-color: {{accent}};
    border-color: {{accent}};
}
)EOF";

    QString result = tmpl;

    // Replace all tokens
    result.replace("{{bgPrimary}}",     colorToRgba(colors::bgPrimary));
    result.replace("{{bgSecondary}}",   colorToRgba(colors::bgSecondary));
    result.replace("{{bgTertiary}}",    colorToRgba(colors::bgTertiary));
    result.replace("{{bgRaised}}",      colorToRgba(colors::bgRaised));
    result.replace("{{border}}",        colorToRgba(colors::border));
    result.replace("{{borderHover}}",   colorToRgba(colors::borderHover));
    result.replace("{{textPrimary}}",   colorToRgba(colors::textPrimary));
    result.replace("{{textSecondary}}", colorToRgba(colors::textSecondary));
    result.replace("{{textDisabled}}",  colorToRgba(colors::textDisabled));
    result.replace("{{accent}}",        colorToRgba(colors::accent));
    result.replace("{{accentHover}}",   colorToRgba(colors::accentHover));
    result.replace("{{accentPressed}}", colorToRgba(colors::accentPressed));
    result.replace("{{font}}",          fontFamily());

    return result;
}

} // namespace cub3::ui::theme
