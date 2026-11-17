// CUB3 Launcher - Sidebar button implementation
#include "SidebarButton.h"
#include "styles/Theme.h"

#include <QPainter>

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
}

void SidebarButton::setIcon(const QString& path) {
    m_iconPath = path;
    m_icon = loadIcon(path);
    update();
}

QSize SidebarButton::sizeHint() const {
    return QSize(theme::sizes::kSidebarWidth - 2 * theme::sizes::kSpacingSmall,
                 40);
}

void SidebarButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (isChecked() || underMouse()) {
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? theme::colors::bgRaised : theme::colors::bgTertiary);
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
        if (isChecked()) {
            p.setBrush(theme::colors::accent);
            p.drawRoundedRect(QRect(0, height() / 4, 3, height() / 2), 1, 1);
        }
    }

    // Draw the icon at the left
    const int iconSize = theme::sizes::kIconMedium;
    const int iconX = theme::sizes::kSpacingMedium;
    const int iconY = (height() - iconSize) / 2;

    if (!m_icon.isNull()) {
        QPixmap icon = m_icon.scaled(iconSize, iconSize,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
        QPainter tintPainter(&icon);
        tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tintPainter.fillRect(icon.rect(), isChecked() || underMouse()
            ? theme::colors::textPrimary
            : theme::colors::textSecondary);
        tintPainter.end();
        p.drawPixmap(iconX, iconY, icon);
    }

    // Draw the text
    const int textX = iconX + iconSize + theme::sizes::kSpacingSmall;
    QRect textRect(textX, 0, width() - textX - theme::sizes::kSpacingSmall, height());

    p.setPen(isChecked() || underMouse()
             ? theme::colors::textPrimary
             : theme::colors::textSecondary);
    p.setFont(font());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
}

} // namespace cub3::ui::widgets
