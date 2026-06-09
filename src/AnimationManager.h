#pragma once
#include <QObject>
#include <QTimer>
#include <QPointF>
#include <functional>

// ─────────────────────────────────────────────────────────────────────────────
//  AnimationManager
//
//  DSA concept: Queue — animation requests are queued and executed in order.
//
//  Drives piece-move animations via a QTimer tick loop.
//  Each animation has:
//    • start/end pixel positions
//    • duration (ms)
//    • easing function (ease-out cubic)
//    • a callback invoked when the animation completes
//
//  The BoardWidget polls currentPos() each frame to draw the flying piece.
// ─────────────────────────────────────────────────────────────────────────────

struct AnimationState {
    QPointF startPos;
    QPointF endPos;
    int     durationMs;
    int     elapsedMs;
    bool    active;
    std::function<void()> onComplete;

    AnimationState() : durationMs(0), elapsedMs(0), active(false) {}
};

class AnimationManager : public QObject
{
    Q_OBJECT

public:
    explicit AnimationManager(QObject* parent = nullptr);

    // Start a new animation (cancels any in progress)
    void animate(QPointF from, QPointF to, int durationMs,
                 std::function<void()> onComplete = nullptr);

    void cancel();

    bool    isAnimating() const  { return m_state.active; }
    QPointF currentPos()  const;
    float   progress()    const;   // 0.0 → 1.0

signals:
    void frameReady();       // emitted each tick — BoardWidget connects to repaint
    void animationFinished();

private slots:
    void tick();

private:
    static float easeOutCubic(float t);

    AnimationState    m_state;
    QTimer            m_timer;
    static constexpr int TICK_MS = 16;   // ~60 fps
};
