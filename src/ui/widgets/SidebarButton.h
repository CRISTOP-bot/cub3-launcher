// CUB3 Launcher - Custom sidebar navigation button
#pragma once

#include <QPushButton>
#include <QPaintEvent>
#include <QEnterEvent>

namespace cub3::ui::widgets {

/// A checkable button for the sidebar navigation. Renders an icon
/// (from the resource cache) and text horizontally. Uses QPainter for
/// custom rendering to avoid QLabel/QIcon overhead.
///
/// Only the active (checked) button repaints on hover — inactive buttons
/// get a simple background change handled by the QSS.
class SidebarButton : public QPushButton {
    Q_OBJECT

public:
    explicit SidebarButton(const QString& iconPath,
                            const QString& text,
                            QWidget* parent = nullptr);

    /// Set the icon resource path. The icon is loaded from the Qt resource
    /// cache and rendered at the configured icon size.
    void setIcon(const QString& path);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_iconPath;
    QPixmap m_icon;
    static QPixmap loadIcon(const QString& path);
};

} // namespace cub3::ui::widgets
