#pragma once
#include <QObject>
#include <QSoundEffect>
#include <QMap>
#include <QString>
#include <memory>
#include "chess.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SoundManager
//
//  Loads WAV sound effects and plays them on chess events.
//  Uses Qt Multimedia's QSoundEffect for low-latency playback.
//  Falls back silently if audio resources are missing.
// ─────────────────────────────────────────────────────────────────────────────

enum class SoundEvent {
    Move,
    Capture,
    Check,
    Checkmate,
    Castle,
    Promotion,
    GameStart,
    Undo
};

class SoundManager : public QObject
{
    Q_OBJECT

public:
    explicit SoundManager(QObject* parent = nullptr);

    void play(SoundEvent evt);
    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }
    void setVolume(float vol);   // 0.0 – 1.0

public slots:
    void onMoveApplied(Move m, bool isCapture, bool isCheck, bool isCheckmate,
                       bool isCastle, bool isPromotion);

private:
    void loadSounds();
    QSoundEffect* loadEffect(const QString& resourcePath);

    QMap<SoundEvent, QSoundEffect*> m_sounds;
    bool  m_muted  = false;
    float m_volume = 1.0f;
};
