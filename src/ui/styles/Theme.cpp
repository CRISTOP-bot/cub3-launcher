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
QMainWindow {
    background-color: {{bgPrimary}};
    border: 1px solid {{border}};
}
#WindowTitleBar {
    background-color: {{bgSecondary}};
    border-bottom: 1px solid {{border}};
}
QLabel#WindowTitleText {
    background: transparent;
    color: {{textSecondary}};
    font-size: 12px;
    font-weight: 600;
}
QToolButton#WindowControl, QToolButton#WindowCloseButton {
    background: transparent;
    border: none;
    border-radius: 5px;
    color: {{textSecondary}};
    font-size: 16px;
    padding: 0;
}
QToolButton#WindowControl:hover { background-color: {{bgRaised}}; }
QToolButton#WindowCloseButton:hover {
    background-color: #C83E4D;
    color: #FFFFFF;
}
QSizeGrip { background: transparent; }
QMenu {
    background-color: {{bgSecondary}};
    border: 1px solid {{borderHover}};
    border-radius: 10px;
    color: {{textPrimary}};
    padding: 6px;
}
QMenu::item {
    background: transparent;
    border-radius: 6px;
    padding: 9px 24px;
}
QMenu::item:selected { background-color: {{bgRaised}}; }
QMenu::separator {
    height: 1px;
    background-color: {{border}};
    margin: 5px 8px;
}

/* ===== Sidebar ===== */
#Sidebar {
    background-color: {{bgSecondary}};
    border-right: 1px solid {{border}};
}
#SidebarLogo {
    color: {{textPrimary}};
    font-size: 28px;
    font-weight: 800;
    padding: 0;
}
#SidebarSubtitle {
    color: {{textSecondary}};
    font-size: 12px;
    letter-spacing: 2px;
    padding: 0;
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
QScrollArea#SettingsScroll { background: transparent; border: none; }
QScrollArea#SettingsScroll QWidget#qt_scrollarea_viewport { background: transparent; }
QGroupBox#SettingsGroup {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 12px;
    color: {{textPrimary}};
    font-size: 14px;
    font-weight: 700;
    margin-top: 10px;
    padding: 18px 14px 12px 14px;
}
QGroupBox#SettingsGroup:hover { border-color: {{borderHover}}; }
QGroupBox#SettingsGroup::title {
    subcontrol-origin: margin;
    left: 14px;
    padding: 0 7px;
}
QLabel#SettingsSaveStatus[state="success"] { color: #50D66D; }
QLabel#SettingsSaveStatus[state="error"] { color: #FF7373; }
QSlider::groove:horizontal {
    height: 5px;
    background: {{bgRaised}};
    border-radius: 2px;
}
QSlider::sub-page:horizontal { background: {{accent}}; border-radius: 2px; }
QSlider::handle:horizontal {
    width: 14px;
    margin: -5px 0;
    border-radius: 7px;
    background: #F4F6FA;
    border: 2px solid {{accent}};
}

/* ===== Cards ===== */
QFrame#Card, QFrame#InfoCard {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 14px;
}
QFrame#Card:hover, QFrame#InfoCard:hover {
    background-color: {{bgTertiary}};
    border-color: {{borderHover}};
}

/* ===== Dashboard ===== */
QWidget#DashboardBody { background-color: {{bgPrimary}}; }
QWidget#SunsetHero { border-radius: 16px; }
QLabel#HeroTag {
    background-color: rgba(21, 33, 38, 175);
    border: 1px solid rgba(189, 232, 193, 75);
    border-radius: 12px;
    color: #D4F1D5;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 1px;
    padding: 6px 10px;
}
QWidget#CardHeader { background: transparent; }
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
QToolButton#AccountBadge {
    background-color: rgba(15, 20, 33, 205);
    border: 1px solid rgba(255, 255, 255, 45);
    border-radius: 9px;
    color: #F1F4F8;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1px;
    padding: 10px 15px;
}
QToolButton#AccountBadge:hover {
    background-color: rgba(37, 46, 65, 230);
    border-color: rgba(255, 255, 255, 75);
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
    font-size: 50px;
    font-weight: 900;
    letter-spacing: 1px;
    padding-bottom: 5px;
}
QLabel#HeroCaption {
    background: transparent;
    color: #E7E9EF;
    font-size: 18px;
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
QLabel#NewsIcon { color: #F7C75D; font-size: 17px; }
QLabel#ProfilesIcon { color: #B985F5; font-size: 17px; }
QLabel#PerformanceIcon { color: {{accent}}; font-size: 17px; }
QFrame#FeatureArt {
    background-color: {{bgTertiary}};
    border: 1px solid rgba(96, 202, 194, 70);
    border-radius: 12px;
}
QLabel#FeatureTitle {
    background: transparent;
    color: {{textPrimary}};
    font-size: 16px;
    font-weight: 700;
}
QLabel#CardCaption {
    background: transparent;
    color: {{textSecondary}};
    font-size: 13px;
}
QLabel#FeatureMeta {
    background: transparent;
    color: #7CD891;
    font-size: 11px;
    font-weight: 600;
}
QToolButton#NewsNavButton {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 6px;
    color: {{textPrimary}};
    font-size: 18px;
    padding: 0;
    min-width: 24px;
    max-width: 24px;
    min-height: 24px;
    max-height: 24px;
}
QToolButton#NewsNavButton:hover {
    background-color: {{bgRaised}};
    border-color: {{borderHover}};
}
QPushButton#NewsLinkButton {
    background: transparent;
    border: none;
    border-radius: 4px;
    color: {{accentHover}};
    font-size: 12px;
    font-weight: 700;
    padding: 2px 0;
    text-align: left;
}
QPushButton#NewsLinkButton:hover { color: #78E38F; }
QPushButton#NewsLinkButton:disabled { color: {{textDisabled}}; }
QLabel#ProfileCount {
    background: transparent;
    color: {{textPrimary}};
    font-size: 42px;
    font-weight: 800;
}
QLabel#JavaStatus {
    background: transparent;
    color: #FF5C5C;
    font-size: 12px;
    font-weight: 700;
}
QLabel#JavaStatus[available="true"] { color: #3AD15E; }
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
    border-radius: 10px;
    font-size: 18px;
    font-weight: 700;
    padding: 16px 52px;
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
