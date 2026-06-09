#include "StatsManager.h"
#include <QSettings>

StatsManager::StatsManager(QObject* parent)
    : QObject(parent)
{
    load();
}

void StatsManager::recordGame(const GameRecord& rec)
{
    ++m_gamesPlayed;

    if (rec.result == "Draw") {
        ++m_draws;
    } else if (rec.mode == "HvH") {
        if (rec.result == "White") ++m_hvhWhiteWins;
        else                        ++m_hvhBlackWins;
    } else {
        // HvAI or AIvH
        bool humanIsWhite = (rec.mode == "HvAI");
        bool whiteWon     = (rec.result == "White");
        bool humanWon     = (humanIsWhite == whiteWon);
        if (humanWon) ++m_humanWins;
        else          ++m_humanLosses;
    }

    // DSA: Vector — prepend so newest is at index 0
    m_history.insert(m_history.begin(), rec);
    if ((int)m_history.size() > MAX_HISTORY)
        m_history.resize(MAX_HISTORY);

    save();
    emit statsUpdated();
}

QString StatsManager::summaryText() const
{
    return QString(
        "Games Played: %1\n"
        "Human Wins:   %2\n"
        "Human Losses: %3\n"
        "Draws:        %4\n"
        "HvH — White wins: %5  Black wins: %6"
    ).arg(m_gamesPlayed)
     .arg(m_humanWins)
     .arg(m_humanLosses)
     .arg(m_draws)
     .arg(m_hvhWhiteWins)
     .arg(m_hvhBlackWins);
}

void StatsManager::resetStats()
{
    m_gamesPlayed = m_humanWins = m_humanLosses = m_draws = 0;
    m_hvhWhiteWins = m_hvhBlackWins = 0;
    m_history.clear();
    save();
    emit statsUpdated();
}

// ── Persistence ──────────────────────────────────────────────────────────────

void StatsManager::load()
{
    QSettings s("ChessQt", "ChessGUI");
    m_gamesPlayed  = s.value("stats/gamesPlayed",  0).toInt();
    m_humanWins    = s.value("stats/humanWins",    0).toInt();
    m_humanLosses  = s.value("stats/humanLosses",  0).toInt();
    m_draws        = s.value("stats/draws",        0).toInt();
    m_hvhWhiteWins = s.value("stats/hvhWhiteWins", 0).toInt();
    m_hvhBlackWins = s.value("stats/hvhBlackWins", 0).toInt();

    // Load history
    int count = s.beginReadArray("history");
    for (int i = 0; i < count && i < MAX_HISTORY; ++i) {
        s.setArrayIndex(i);
        GameRecord rec;
        rec.dateTime   = s.value("dateTime").toString();
        rec.result     = s.value("result").toString();
        rec.resultReason = s.value("resultReason").toString();
        rec.mode       = s.value("mode").toString();
        rec.difficulty = s.value("difficulty").toString();
        rec.totalMoves = s.value("totalMoves").toInt();
        // DSA: Vector — restore move list
        QStringList ml = s.value("moves").toStringList();
        for (const QString& m : ml)
            rec.moves.push_back(m.toStdString());
        m_history.push_back(rec);
    }
    s.endArray();
}

void StatsManager::save()
{
    QSettings s("ChessQt", "ChessGUI");
    s.setValue("stats/gamesPlayed",  m_gamesPlayed);
    s.setValue("stats/humanWins",    m_humanWins);
    s.setValue("stats/humanLosses",  m_humanLosses);
    s.setValue("stats/draws",        m_draws);
    s.setValue("stats/hvhWhiteWins", m_hvhWhiteWins);
    s.setValue("stats/hvhBlackWins", m_hvhBlackWins);

    // DSA: Vector — serialise history
    s.beginWriteArray("history");
    for (int i = 0; i < (int)m_history.size(); ++i) {
        s.setArrayIndex(i);
        const GameRecord& rec = m_history[i];
        s.setValue("dateTime",    rec.dateTime);
        s.setValue("result",      rec.result);
        s.setValue("resultReason", rec.resultReason);
        s.setValue("mode",        rec.mode);
        s.setValue("difficulty",  rec.difficulty);
        s.setValue("totalMoves",  rec.totalMoves);
        QStringList ml;
        for (const auto& m : rec.moves)
            ml << QString::fromStdString(m);
        s.setValue("moves", ml);
    }
    s.endArray();
}
