// CUB3 Launcher - Play button implementation
#include "PlayButton.h"
#include "styles/Theme.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

namespace cub3::ui::widgets {

PlayButton::PlayButton(const QString& text, QWidget* parent)
    : QPushButton(parent)
{
    setObjectName(QStringLiteral("PlayButton"));
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_StyledBackground, true);
    setText(text);
    setFixedSize(theme::sizes::kPlayButtonW, theme::sizes::kPlayButtonH);

    m_anim = new QPropertyAnimation(this, "scale", this);
    m_anim->setDuration(theme::anim::kFastMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
}

void PlayButton::setScale(qreal s) {
    m_scale = s;
    update();
}

void PlayButton::mousePressEvent(QMouseEvent* event) {
    if (m_anim->state() != QPropertyAnimation::Stopped)
        m_anim->stop();
    m_anim->setStartValue(m_scale);
    m_anim->setEndValue(0.96);
    m_anim->start();
    QPushButton::mousePressEvent(event);
}

void PlayButton::mouseReleaseEvent(QMouseEvent* event) {
    if (m_anim->state() != QPropertyAnimation::Stopped)
        m_anim->stop();
    m_anim->setStartValue(m_scale);
    m_anim->setEndValue(1.0);
    m_anim->start();
    QPushButton::mouseReleaseEvent(event);
}

void PlayButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Apply scale transform centered on the button
    const qreal s = m_scale;
    p.translate(width() * (1.0 - s) / 2.0, height() * (1.0 - s) / 2.0);
    p.scale(s, s);

    // Draw styled background
    QStyleOptionButton opt;
    opt.initFrom(this);
    opt.text = text();
    if (isDown())
        opt.state |= QStyle::State_Sunken;
    else if (underMouse())
        opt.state |= QStyle::State_MouseOver;
    if (!isEnabled())
        opt.state &= ~QStyle::State_Enabled;

    style()->drawControl(QStyle::CE_PushButtonBevel, &opt, &p, this);

    // Draw text centred
    p.setPen(Qt::white);
    QFont f = font();
    f.setPointSize(theme::fonts::kSizeTitle);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(0, 0, 0, 0), Qt::AlignCenter, text());
}

} // namespace cub3::ui::widgets
