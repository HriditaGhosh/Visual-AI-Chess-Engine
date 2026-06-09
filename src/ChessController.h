#pragma once
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QStack>
#include <QStringList>
#include <vector>
#include <memory>

// Engine headers
#include "chess.h"
#include "board.h"
#include "movegen.h"
#include "game.h"
#include "ai_engine.h"

// ─────────────────────────────────────────────────────────────────────────────
//  AIWorker  (runs engine search on a background QThread)
//
//  DSA: Event-Driven Programming — the worker lives on a separate thread.
//       When the GUI posts a "please think" request (via signal), the worker
//       computes the best move and emits moveReady() back to the main thread.
//       The GUI never blocks — the Qt event loop keeps running.
// ─────────────────────────────────────────────────────────────────────────────
class AIWorker : public QObject
{
    Q_OBJECT
public:
    explicit AIWorker(QObject* parent = nullptr);

public slots:
    // Called from controller thread via queued connection
    void findBestMove(Board boardCopy, Difficulty diff, Color side);

signals:
    void moveReady(Move bestMove, SearchResult result);
    void searchProgress(int depth, int score, long nodes);

private:
    // AIEngine is constructed fresh per search (it holds a Board& reference)
};

// ─────────────────────────────────────────────────────────────────────────────
//  GameConfig — player + difficulty settings
// ─────────────────────────────────────────────────────────────────────────────
enum class PlayerMode { HumanVsHuman, HumanVsAI, AIVsHuman };

struct GameConfig {
    PlayerMode  mode       = PlayerMode::HumanVsAI;
    Difficulty  difficulty = MEDIUM;
    Color       humanColor = WHITE;
};

// ─────────────────────────────────────────────────────────────────────────────
//  ChessController
//
//  Central controller — owns the Board, MoveGenerator, and AIWorker.
//  All GUI actions (click, drag-drop, new game, undo) call controller slots.
//  Engine results come back via signals to connected GUI widgets.
//
//  DSA: Stack (m_undoStack) — each legal move is pushed; undo pops it and
//       calls board.undoMove(), restoring the previous state in O(1).
//
//  DSA: Queue (Qt's signal/slot queued connection mechanism) — the controller
//       queues AI search requests; events are processed in FIFO order by
//       the Qt event loop on the worker thread.
// ─────────────────────────────────────────────────────────────────────────────
class ChessController : public QObject
{
    Q_OBJECT

public:
    explicit ChessController(QObject* parent = nullptr);
    ~ChessController();

    // Read-only board access for the BoardWidget painter
    const Board&             board()      const { return m_board; }
    const std::vector<Move>& legalMoves() const { return m_legalMoves; }
    GameStatus               status()     const { return m_status; }
    const GameConfig&        config()     const { return m_config; }

    // Legal moves from a specific square (for highlight)
    std::vector<Move> movesFrom(Square sq) const;

    bool canUndo() const { return !m_undoStack.isEmpty(); }
    int  undoStackSize() const { return m_undoStack.size(); }
    bool isAIThinking() const { return m_aiThinking; }
    void setBoard(const Board& b);   // FEN import support

public slots:
    void newGame(const GameConfig& cfg);
    void newGame();           // restart with same config

    // ── Human interaction ──────────────────────────────────────────
    // Returns true if the move was legal and applied
    bool tryMove(Square from, Square to, PieceType promotion = NONE_PIECE);

    // DSA: Stack — undo pops the last move off the undo stack
    void undo();

    void setFlipped(bool flipped) { m_flipped = flipped; emit boardFlipped(m_flipped); }
    bool isFlipped() const { return m_flipped; }

signals:
    // ── Board state ───────────────────────────────────────────────
    void boardChanged();              // full repaint needed
    void moveApplied(Move m, bool isCapture, bool isCheck,
                     bool isCheckmate, bool isCastle, bool isPromotion);
    void moveAnimationRequested(Square from, Square to);
    void undoPerformed();
    void gameStatusChanged(GameStatus s);
    void boardFlipped(bool flipped);

    // ── AI ────────────────────────────────────────────────────────
    void aiThinkingStarted();
    void aiThinkingFinished(SearchResult result);
    void aiProgressUpdate(int depth, int score, long nodes);

    // ── Internal bridge to worker thread (queued connection) ─────
    // DSA: Queue — request is queued into worker thread's event loop
    void requestAIMove(Board boardCopy, Difficulty diff, Color side);

private slots:
    void onAIMoveReady(Move bestMove, SearchResult result);
    void onAIProgress(int depth, int score, long nodes);

private:
    void refreshLegalMoves();
    void updateStatus();
    void triggerAIIfNeeded();
    bool isHumanTurn() const;

    // ── Engine state ─────────────────────────────────────────────
    Board         m_board;
    MoveGenerator m_gen;
    GameStatus    m_status  = PLAYING;
    GameConfig    m_config;
    bool          m_flipped = false;

    // DSA: Stack — undo system (UI side; engine has its own history stack)
    QStack<Move>  m_undoStack;

    // Move cache
    std::vector<Move> m_legalMoves;

    // ── AI threading ─────────────────────────────────────────────
    QThread*  m_aiThread  = nullptr;
    AIWorker* m_aiWorker  = nullptr;
    bool      m_aiThinking = false;
};
