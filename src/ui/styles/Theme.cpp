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
    static const QString tmpl = QString::fromUtf8(R"EOF(
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
#WindowTitleBar QToolButton {
    min-width: 36px;
    min-height: 28px;
}
#SidebarLogo {
    color: {{textPrimary}};
    letter-spacing: 1px;
}
#SidebarSubtitle {
    color: {{accent}};
}
QLabel#SidebarFooter {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 10px;
    color: {{textSecondary}};
    padding: 12px 13px;
}
QLabel#FooterStatus { color: {{textSecondary}}; }
QLabel#FooterCaption { color: {{textDisabled}}; font-weight: 700; }
QToolButton#AccountBadge {
    border-radius: 10px;
    padding: 10px 16px;
}
QToolButton#NewsNavButton {
    min-width: 28px;
    max-width: 28px;
    min-height: 28px;
    max-height: 28px;
    border-radius: 9px;
}
QPushButton#NewsLinkButton {
    padding: 7px 0;
    font-size: 13px;
}
QLabel#FeatureMeta {
    font-size: 11px;
    font-weight: 700;
}
QLabel#CardCaption {
    color: {{textSecondary}};
}
QWidget#SunsetHero {
    border: 1px solid {{borderHover}};
}
QLineEdit#SearchField {
    min-height: 24px;
    border-radius: 11px;
    padding-left: 14px;
}
QComboBox#VersionSelector {
    min-height: 30px;
    font-size: 14px;
    font-weight: 700;
}
QComboBox#VersionSelector {
    border-radius: 10px;
    padding: 10px 14px;
}
QLabel#HeroTitle {
    font-size: 54px;
    letter-spacing: 2px;
}
QLabel#ProfileCount {
    color: {{accent}};
    font-size: 46px;
}
QLabel#JavaStatus {
    background-color: {{bgTertiary}};
    border-radius: 7px;
    padding: 5px 8px;
}
QLabel#JavaStatus[available="true"] {
    background-color: rgba(72, 201, 112, 0.12);
}
QMenu::item:selected {
    color: {{accentHover}};
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border: 1px solid {{accent}};
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
    color: #092017;
    border: none;
    border-radius: 12px;
    font-size: 17px;
    font-weight: 800;
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
    width: 14px;)EOF")
        + QString::fromUtf8(R"EOF(
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

/* ===== Refined CUB3 surfaces and interaction states ===== */
#Sidebar {
    border-right: 1px solid {{border}};
}
QPushButton#SidebarButton {
    min-height: 42px;
    padding-left: 14px;
    margin: 3px 10px;
    border-radius: 9px;
    font-weight: 600;
}
QPushButton#SidebarButton:checked {
    background-color: {{bgRaised}};
    border: 1px solid {{borderHover}};
}
QFrame#Card, QFrame#InfoCard {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 15px;
}
QFrame#Card:hover, QFrame#InfoCard:hover {
    border-color: {{borderHover}};
}
QLabel#PageTitle {
    font-size: 28px;
    font-weight: 800;
    padding: 2px 0 5px;
}
QLabel#PageSubtitle {
    color: {{textSecondary}};
    font-size: 13px;
    padding-bottom: 10px;
}
QPushButton#Primary {
    border-radius: 9px;
    font-weight: 700;
    padding: 10px 18px;
}
QPushButton#SecondaryButton {
    border-radius: 9px;
    font-weight: 600;
    padding: 9px 14px;
}
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    min-height: 22px;
    border-radius: 9px;
    padding: 9px 12px;
    selection-background-color: {{accentPressed}};
}
QLineEdit:hover, QSpinBox:hover, QComboBox:hover {
    border-color: {{borderHover}};
}
QListWidget#InstanceList, QListWidget {
    background-color: {{bgSecondary}};
    alternate-background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 12px;
    padding: 7px;
    outline: none;
}
QListWidget::item, QListView::item {
    min-height: 34px;
    padding: 8px 11px;
    border: 1px solid transparent;
    border-radius: 8px;
}
QListWidget::item:hover, QListView::item:hover {
    background-color: {{bgTertiary}};
}
QListWidget::item:selected, QListView::item:selected {
    background-color: {{bgRaised}};
    border-color: {{borderHover}};
    color: {{textPrimary}};
}
QGroupBox#SettingsGroup {
    background-color: {{bgSecondary}};
    border-color: {{border}};
    border-radius: 14px;
    padding: 20px 16px 14px;
}
QGroupBox#SettingsGroup:hover { border-color: {{borderHover}}; }
QProgressBar {
    min-height: 12px;
    border: 1px solid {{border}};
    border-radius: 6px;
    background-color: {{bgTertiary}};
}
QProgressBar::chunk {
    border-radius: 5px;
}
QScrollBar:vertical { width: 9px; }
QScrollBar::handle:vertical { min-height: 36px; }

/* ===== Login and tutorial dialogs ===== */
QLabel#LoginError {
    color: #FF8A83;
    background-color: rgba(255, 92, 92, 0.10);
    border: 1px solid rgba(255, 92, 92, 0.28);
    border-radius: 8px;
    padding: 8px 10px;
}
QLabel#SettingsSaveStatus[state="dirty"] { color: #F2C879; }
QLabel#EmptyState { color: {{textSecondary}}; padding: 30px; }
QDialogButtonBox QPushButton:focus, QPushButton:focus, QToolButton:focus,
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QSlider:focus,
QCheckBox:focus, QTabBar::tab:focus {
    border: 2px solid {{accent}};
}
QPushButton#Primary:disabled { background-color: {{bgRaised}}; color: {{textDisabled}}; }
QProgressBar#TutorialProgress { min-height: 8px; max-height: 8px; }
QProgressBar#LoginProgress { min-height: 7px; max-height: 7px; }
QDialog {
    background-color: {{bgPrimary}};
    color: {{textPrimary}};
}
QTabWidget::pane {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 12px;
    top: -1px;
}
QTabBar::tab {
    background-color: {{bgSecondary}};
    border: 1px solid transparent;
    border-radius: 8px;
    color: {{textSecondary}};
    padding: 9px 16px;
    margin-right: 5px;
}
QTabBar::tab:selected {
    background-color: {{bgRaised}};
    border-color: {{borderHover}};
    color: {{textPrimary}};
}
QTabBar::tab:hover { color: {{accentHover}}; }
QDialogButtonBox QPushButton { min-width: 100px; }
QLabel#Heading { font-size: 23px; font-weight: 800; }
QLabel#Title { font-size: 20px; font-weight: 800; }

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
/* ===== CUB3 interface refinement ===== */
#ContentArea { background-color: {{bgPrimary}}; }
#WindowTitleBar { min-height: 38px; }
QLabel#PageTitle { letter-spacing: 0px; }
QPushButton {
    min-height: 36px;
    border: 1px solid {{border}};
    border-radius: 10px;
    padding: 8px 14px;
    font-weight: 600;
}
QPushButton:hover { border-color: {{borderHover}}; }
QPushButton#Primary {
    min-height: 40px;
    border: 1px solid {{accent}};
    border-radius: 10px;
    padding: 9px 16px;
    font-weight: 750;
}
QPushButton#SecondaryButton { background-color: {{bgSecondary}}; }
QPushButton:focus, QLineEdit:focus, QComboBox:focus, QListWidget:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QCheckBox:focus {
    border: 2px solid {{accent}};
}
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    min-height: 26px;
    border-radius: 10px;
    padding: 8px 12px;
}
QListWidget#InstanceList {
    border-radius: 15px;
    padding: 8px;
}
QListWidget#InstanceList::item {
    border-radius: 12px;
    padding: 8px;
}
QProgressBar { text-align: center; }
QFrame#ModrinthBanner {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 {{bgRaised}}, stop:1 {{bgSecondary}});
    border: 1px solid {{borderHover}};
    border-radius: 16px;
}
QLabel#ModrinthMark {
    background-color: #1BD96A;
    border-radius: 12px;
    color: #092016;
    font-size: 24px;
    font-weight: 900;
}
QLabel#ModrinthProviderTag, QLabel#ModsSectionEyebrow {
    color: {{accent}};
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 1px;
}
QLabel#ModrinthTitle { color: {{textPrimary}}; font-size: 15px; font-weight: 800; }
QLabel#ModrinthDescription { color: {{textSecondary}}; font-size: 12px; }
QFrame#ModsFilterCard {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 15px;
}
QLineEdit#ModsSearchField, QLineEdit#ModsVersionField,
QComboBox#ModsLoader, QComboBox#ModsSort {
    min-height: 40px;
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 10px;
}
QWidget#ModResultCard {
    background-color: {{bgTertiary}};
    border: 1px solid {{border}};
    border-radius: 12px;
}
QWidget#ModResultCard[selected="true"] {
    background-color: {{bgRaised}};
    border: 1px solid {{accent}};
}
QLabel#ModResultTitle { color: {{textPrimary}}; font-size: 15px; font-weight: 750; }
QLabel#ModResultDescription { color: {{textSecondary}}; font-size: 12px; }
QLabel#ModResultMeta { color: {{accent}}; font-size: 10px; font-weight: 700; }
QLabel#ModsStatus {
    background-color: {{bgSecondary}};
    border: 1px solid {{border}};
    border-radius: 10px;
    color: {{textSecondary}};
    padding: 9px 12px;
}
)EOF");

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
