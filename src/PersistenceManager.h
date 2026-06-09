#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include <QStringList>
#include <vector>
#include <stack>

#include "chess.h"
#include "board.h"
#include "ChessController.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SavedGame  — in-memory snapshot of a full game
//
//  DSA: Vector  — move list stored as contiguous array for O(1) index access
//  DSA: Stack   — used by loadGame() to replay moves and restore undo stack
//  DSA: File I/O structures — serialised to/from JSON-like .chsave files
// ─────────────────────────────────────────────────────────────────────────────
struct SavedGame {
    // Metadata
    QString     saveName;
    QString     dateTime;
    QString     playerMode;    // "HvH" | "HvAI" | "AIvH"
    QString     difficulty;    // "Easy" | "Medium" | "Hard" | "Expert"
    QString     humanColor;    // "White" | "Black"

    // DSA: Vector — full ordered move list for replay / PGN export
    std::vector<std::string> moves;   // UCI strings e.g. "e2e4"

    // Current board snapshot (FEN) for quick display in load dialog
    QString     fen;

    // Half-move index where the game was saved (for partial-game saves)
    int         savedAtMove = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  PersistenceManager
//
//  Handles all persistence: save, load, autosave, PGN, FEN import/export.
//
//  DSA: File I/O structures
//    – Custom JSON-like text format: key:value lines, moves as space-delimited
//    – PGN: standard chess notation format
//    – FEN: board position string
//
//  DSA: Stack (move history restore)
//    – loadGame() replays all moves through ChessController so both the
//      Board's internal undo stack and m_undoStack are rebuilt correctly
//
//  DSA: Vector (move list storage)
//    – All moves accumulated in m_moveLog (std::vector<std::string>)
//    – On each moveApplied signal, move is push_back'd
//    – On undoPerformed, last element is pop_back'd
// ─────────────────────────────────────────────────────────────────────────────
class PersistenceManager : public QObject
{
    Q_OBJECT

public:
    explicit PersistenceManager(ChessController* ctrl, QObject* parent = nullptr);

    // ── Save / Load ───────────────────────────────────────────────
    bool saveGame(const QString& filePath, const QString& saveName = "");
    bool loadGame(const QString& filePath);

    // ── Autosave ──────────────────────────────────────────────────
    void setAutosaveEnabled(bool enabled);
    void setAutosaveIntervalSeconds(int secs);
    bool isAutosaveEnabled() const { return m_autosaveEnabled; }
    QString autosavePath() const;

    // ── PGN ───────────────────────────────────────────────────────
    bool exportPGN(const QString& filePath) const;
    bool importPGN(const QString& filePath);

    // ── FEN ───────────────────────────────────────────────────────
    QString exportFEN() const;
    bool    importFEN(const QString& fen);  // sets up position (no move history)

    // ── Move log access (DSA: Vector) ─────────────────────────────
    const std::vector<std::string>& moveLog() const { return m_moveLog; }
    void clearMoveLog();

    // ── Recent saves list ─────────────────────────────────────────
    QStringList recentSaves() const;

signals:
    void autosaved(const QString& path);
    void saveError(const QString& msg);
    void loadError(const QString& msg);
    void gameLoaded(const SavedGame& game);

public slots:
    // Connected to ChessController signals
    void onMoveApplied(Move m, bool, bool, bool, bool, bool);
    void onUndoPerformed();
    void onNewGame();

private slots:
    void doAutosave();

private:
    // Serialisation helpers
    bool        writeToFile(const QString& path, const SavedGame& sg) const;
    bool        readFromFile(const QString& path, SavedGame& sg) const;
    SavedGame   buildSaveData(const QString& saveName) const;

    // PGN helpers
    QString     moveToPGN(const std::string& uci, int moveNum, bool isWhite,
                          bool isCheck, bool isMate) const;
    std::string parsePGNMove(const QString& pgn, const Board& b) const;
    QString     pgnHeader(const QString& tag, const QString& val) const;

    // FEN import helper — parse FEN string into a Board
    bool        parseFEN(const QString& fen, Board& outBoard) const;

    // Difficulty / mode strings
    QString     diffString()   const;
    QString     modeString()   const;
    Difficulty  parseDiff(const QString& s) const;
    PlayerMode  parseMode(const QString& s) const;

    // ── State ─────────────────────────────────────────────────────
    ChessController*         m_ctrl;

    // DSA: Vector — accumulated move log (UCI strings)
    std::vector<std::string> m_moveLog;

    // Autosave
    QTimer  m_autosaveTimer;
    bool    m_autosaveEnabled  = true;
    int     m_autosaveInterval = 120;  // seconds
    bool    m_dirty            = false; // unsaved changes?

    // Recent files (simple LRU list, max 10)
    QStringList m_recentSaves;
    void addRecent(const QString& path);

    static constexpr int MAX_RECENT = 10;
};
