// CUB3 Launcher - Sidebar button implementation
#include "SidebarButton.h"
#include "styles/Theme.h"

#include <QPainter>
#include <QPen>

namespace cub3::ui::widgets {

QPixmap SidebarButton::loadIcon(const QString& path) {
    // Icons are loaded from the Qt resource cache.
    // QPixmap caches the rasterised version internally.
    return QPixmap(path);
}

SidebarButton::SidebarButton(const QString& iconPath,
                             const QString& text,
                             QWidget* parent)
    : QPushButton(text, parent)
    , m_iconPath(iconPath)
    , m_icon(loadIcon(iconPath))
{
    setObjectName(QStringLiteral("SidebarButton"));
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_StyledBackground, true);
    setText(text);
    rebuildIconCache();
}

void SidebarButton::rebuildIconCache() {
    m_iconActive = {};
    m_iconInactive = {};
    if (m_icon.isNull())
        return;

    const int iconSize = theme::sizes::kIconMedium;
    const QPixmap base = m_icon.scaled(iconSize, iconSize,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
    const auto tinted = [&base](const QColor& color) {
        QPixmap icon = base;
        QPainter painter(&icon);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(icon.rect(), color);
        return icon;
    };
    m_iconActive = tinted(theme::colors::textPrimary);
    m_iconInactive = tinted(theme::colors::textSecondary);
}

void SidebarButton::setIcon(const QString& path) {
    m_iconPath = path;
    m_icon = loadIcon(path);
    rebuildIconCache();
    update();
}

QSize SidebarButton::sizeHint() const {
    return QSize(theme::sizes::kSidebarWidth - 2 * theme::sizes::kSpacingSmall,
                 40);
}

void SidebarButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool emphasized = isEnabled() && (isChecked() || underMouse());
    if (emphasized) {
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? theme::colors::bgRaised : theme::colors::bgTertiary);
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
        if (isChecked()) {
            p.setBrush(theme::colors::accent);
            p.drawRoundedRect(QRect(0, height() / 4, 3, height() / 2), 1, 1);
        }
    }
    if (hasFocus()) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(theme::colors::accent, 2));
        p.drawRoundedRect(rect().adjusted(2, 2, -3, -3), 7, 7);
    }

    // Draw the icon at the left
    const int iconSize = theme::sizes::kIconMedium;
    const int iconX = theme::sizes::kSpacingMedium;
    const int iconY = (height() - iconSize) / 2;

    const QPixmap& icon = isChecked() || underMouse()
        ? m_iconActive : m_iconInactive;
    if (!icon.isNull())
        p.drawPixmap(iconX, iconY, icon);

    // Draw the text
    const int textX = iconX + iconSize + theme::sizes::kSpacingSmall;
    QRect textRect(textX, 0, width() - textX - theme::sizes::kSpacingSmall, height());

    p.setPen(!isEnabled() ? theme::colors::textDisabled
             : emphasized ? theme::colors::textPrimary
                          : theme::colors::textSecondary);
    p.setFont(font());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
}

} // namespace cub3::ui::widgets
