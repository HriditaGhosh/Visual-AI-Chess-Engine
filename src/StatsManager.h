#pragma once
#include <QObject>
#include <QSettings>
#include <QDateTime>
#include <vector>
#include <QString>

#include "chess.h"
#include "game.h"
#include "ChessController.h"

// ─────────────────────────────────────────────────────────────────────────────
//  GameRecord — one completed game entry
//
//  DSA: Vector — all completed games stored in std::vector<GameRecord>
//       giving O(1) random access for replay navigation (tree traversal)
// ─────────────────────────────────────────────────────────────────────────────
struct GameRecord {
    QString  dateTime;
    QString  result;       // "White", "Black", "Draw"
    QString  resultReason; // "Checkmate", "Stalemate", "50-move"
    QString  mode;         // "HvH", "HvAI", "AIvH"
    QString  difficulty;
    int      totalMoves = 0;
    std::vector<std::string> moves;  // DSA: Vector — full move list
};

// ─────────────────────────────────────────────────────────────────────────────
//  StatsManager
//
//  Tracks cumulative win/loss/draw statistics and game history.
//  Persists to QSettings (Windows registry / INI file).
//
//  DSA: Vector (game history)
//    – m_history: std::vector<GameRecord> — all past games in insertion order
//    – O(1) index access used by ReplayDialog for tree traversal navigation
//
//  DSA: Queue (move replay)
//    – ReplayDialog feeds moves from a GameRecord into a std::queue<std::string>
//      and pops one move per "Next" click, replaying in FIFO order
// ─────────────────────────────────────────────────────────────────────────────
class StatsManager : public QObject
{
    Q_OBJECT

public:
    explicit StatsManager(QObject* parent = nullptr);

    // ── Record a completed game ───────────────────────────────────
    void recordGame(const GameRecord& rec);

    // ── Statistics ────────────────────────────────────────────────
    int gamesPlayed()  const { return m_gamesPlayed; }
    int humanWins()    const { return m_humanWins; }
    int humanLosses()  const { return m_humanLosses; }
    int draws()        const { return m_draws; }
    int hvhWhiteWins() const { return m_hvhWhiteWins; }
    int hvhBlackWins() const { return m_hvhBlackWins; }

    // DSA: Vector — O(1) indexed access to game history
    const std::vector<GameRecord>& history() const { return m_history; }
    int historySize() const { return static_cast<int>(m_history.size()); }

    void resetStats();

    QString summaryText() const;

signals:
    void statsUpdated();

private:
    void load();
    void save();

    int m_gamesPlayed  = 0;
    int m_humanWins    = 0;
    int m_humanLosses  = 0;
    int m_draws        = 0;
    int m_hvhWhiteWins = 0;
    int m_hvhBlackWins = 0;

    // DSA: Vector — game history list
    std::vector<GameRecord> m_history;

    static constexpr int MAX_HISTORY = 50;
};
