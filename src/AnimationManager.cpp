#include "AnimationManager.h"
#include <cmath>

AnimationManager::AnimationManager(QObject* parent)
    : QObject(parent)
{
    m_timer.setInterval(TICK_MS);
    connect(&m_timer, &QTimer::timeout, this, &AnimationManager::tick);
}

void AnimationManager::animate(QPointF from, QPointF to, int durationMs,
                               std::function<void()> onComplete)
{
    m_timer.stop();

    m_state.startPos    = from;
    m_state.endPos      = to;
    m_state.durationMs  = durationMs;
    m_state.elapsedMs   = 0;
    m_state.active      = true;
    m_state.onComplete  = onComplete;

    m_timer.start();
}

void AnimationManager::cancel()
{
    m_timer.stop();
    m_state.active = false;
}

// DSA: ease-out cubic — pieces decelerate smoothly as they arrive
float AnimationManager::easeOutCubic(float t)
{
    float f = 1.0f - t;
    return 1.0f - (f * f * f);
}

float AnimationManager::progress() const
{
    if (!m_state.active || m_state.durationMs <= 0) return 1.0f;
    float t = static_cast<float>(m_state.elapsedMs) /
              static_cast<float>(m_state.durationMs);
    return std::min(1.0f, t);
}

QPointF AnimationManager::currentPos() const
{
    float t = easeOutCubic(progress());
    return m_state.startPos + t * (m_state.endPos - m_state.startPos);
}

// ── Timer tick ───────────────────────────────────────────────────────────────
void AnimationManager::tick()
{
    m_state.elapsedMs += TICK_MS;

    emit frameReady();   // signal → BoardWidget::update()

    if (m_state.elapsedMs >= m_state.durationMs) {
        m_timer.stop();
        m_state.active = false;

        if (m_state.onComplete)
            m_state.onComplete();

        emit animationFinished();
    }
}
