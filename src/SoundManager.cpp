#include "SoundManager.h"
#include "chess.h"
#include <QUrl>

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
{
    loadSounds();
}

QSoundEffect* SoundManager::loadEffect(const QString& resourcePath)
{
    auto* effect = new QSoundEffect(this);
    effect->setSource(QUrl(resourcePath));
    effect->setVolume(m_volume);
    return effect;
}

void SoundManager::loadSounds()
{
    // Resource paths — .wav files in resources/sounds/
    m_sounds[SoundEvent::Move]      = loadEffect("qrc:/sounds/move.wav");
    m_sounds[SoundEvent::Capture]   = loadEffect("qrc:/sounds/capture.wav");
    m_sounds[SoundEvent::Check]     = loadEffect("qrc:/sounds/check.wav");
    m_sounds[SoundEvent::Checkmate] = loadEffect("qrc:/sounds/checkmate.wav");
    m_sounds[SoundEvent::Castle]    = loadEffect("qrc:/sounds/castle.wav");
    m_sounds[SoundEvent::Promotion] = loadEffect("qrc:/sounds/promotion.wav");
    m_sounds[SoundEvent::GameStart] = loadEffect("qrc:/sounds/game_start.wav");
    m_sounds[SoundEvent::Undo]      = loadEffect("qrc:/sounds/undo.wav");
}

void SoundManager::play(SoundEvent evt)
{
    if (m_muted) return;
    if (auto* fx = m_sounds.value(evt, nullptr)) {
        fx->setVolume(m_volume);
        fx->play();
    }
}

void SoundManager::setMuted(bool muted)
{
    m_muted = muted;
}

void SoundManager::setVolume(float vol)
{
    m_volume = std::clamp(vol, 0.0f, 1.0f);
    for (auto* fx : m_sounds)
        if (fx) fx->setVolume(m_volume);
}

void SoundManager::onMoveApplied(Move /*m*/, bool isCapture, bool isCheck,
                                  bool isCheckmate, bool isCastle, bool isPromotion)
{
    // Priority: checkmate > promotion > check > castle > capture > move
    if (isCheckmate)       play(SoundEvent::Checkmate);
    else if (isPromotion)  play(SoundEvent::Promotion);
    else if (isCheck)      play(SoundEvent::Check);
    else if (isCastle)     play(SoundEvent::Castle);
    else if (isCapture)    play(SoundEvent::Capture);
    else                   play(SoundEvent::Move);
}
