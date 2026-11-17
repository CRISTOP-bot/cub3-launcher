// CUB3 Launcher - Play button with press animation
#pragma once

#include <QPushButton>
#include <QPropertyAnimation>

namespace cub3::ui::widgets {

/// Large PLAY button for the dashboard. Has a subtle scale-down animation
/// on press and a glow-free hover state (no heavy effects).
///
/// The animation only runs on press/release — no continuous animation.
/// It stops as soon as the transition completes.
class PlayButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)

public:
    explicit PlayButton(const QString& text, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    [[nodiscard]] qreal scale() const { return m_scale; }
    void setScale(qreal s);

    qreal m_scale{1.0};
    QPropertyAnimation* m_anim{nullptr};
};

} // namespace cub3::ui::widgets
