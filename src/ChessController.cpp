#include "ChessController.h"
#include <QMetaType>
#include <algorithm>

// ── Register engine types with Qt's meta-type system ────────────────────────
Q_DECLARE_METATYPE(Move)
Q_DECLARE_METATYPE(Board)
Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(Difficulty)
Q_DECLARE_METATYPE(Color)

// ═══════════════════════════════════════════════════════════════
//  AIWorker
// ═══════════════════════════════════════════════════════════════

AIWorker::AIWorker(QObject* parent) : QObject(parent) {}

void AIWorker::findBestMove(Board boardCopy, Difficulty diff, Color side)
{
    // AIEngine holds a Board& reference, so construct fresh per search
    AIEngine engine(boardCopy);
    SearchResult result = engine.findBestMove(side, diff);

    emit moveReady(result.bestMove, result);
}

// ═══════════════════════════════════════════════════════════════
//  ChessController
// ═══════════════════════════════════════════════════════════════

ChessController::ChessController(QObject* parent)
    : QObject(parent), m_gen(m_board)
{
    // Register types for cross-thread queued connections
    qRegisterMetaType<Move>("Move");
    qRegisterMetaType<Board>("Board");
    qRegisterMetaType<SearchResult>("SearchResult");
    qRegisterMetaType<Difficulty>("Difficulty");
    qRegisterMetaType<Color>("Color");
    qRegisterMetaType<GameStatus>("GameStatus");

    // ── Set up AI worker on its own thread ───────────────────────
    // DSA: Event-Driven Programming — worker lives in its own Qt event loop
    m_aiThread = new QThread(this);
    m_aiWorker = new AIWorker();
    m_aiWorker->moveToThread(m_aiThread);

    // requestAIMove → worker::findBestMove  (queued, cross-thread)
    // DSA: Queue — Qt's internal event queue delivers this in FIFO order
    connect(this,       &ChessController::requestAIMove,
            m_aiWorker, &AIWorker::findBestMove,
            Qt::QueuedConnection);

    // worker → controller (back on main thread)
    connect(m_aiWorker, &AIWorker::moveReady,
            this,       &ChessController::onAIMoveReady,
            Qt::QueuedConnection);

    connect(m_aiWorker, &AIWorker::searchProgress,
            this,       &ChessController::onAIProgress,
            Qt::QueuedConnection);

    m_aiThread->start();

    m_board.reset();
    refreshLegalMoves();
}

ChessController::~ChessController()
{
    m_aiThread->quit();
    m_aiThread->wait(3000);
    delete m_aiWorker;
}

// ── Game management ──────────────────────────────────────────────────────────

void ChessController::newGame(const GameConfig& cfg)
{
    m_config = cfg;
    newGame();
}

void ChessController::newGame()
{
    m_board.reset();
    m_undoStack.clear();   // DSA: Stack cleared for fresh game
    m_status = PLAYING;
    m_aiThinking = false;
    refreshLegalMoves();

    emit boardChanged();
    emit gameStatusChanged(m_status);

    triggerAIIfNeeded();
}

// ── Human move ───────────────────────────────────────────────────────────────

bool ChessController::tryMove(Square from, Square to, PieceType promotion)
{
    if (m_aiThinking) return false;
    if (!isHumanTurn())  return false;

    // Find matching legal move
    Move chosen;
    bool found = false;
    for (const Move& m : m_legalMoves) {
        if (m.from == from && m.to == to) {
            // Handle promotion
            if (m.promotion != NONE_PIECE) {
                if (promotion == NONE_PIECE) promotion = QUEEN; // default
                if (m.promotion != promotion) continue;
            }
            chosen = m;
            found  = true;
            break;
        }
    }
    if (!found) return false;

    bool isCapture   = !m_board.at(to).empty() || chosen.isEnPassant;
    bool isCastle    = chosen.isCastle;
    bool isPromotion = (chosen.promotion != NONE_PIECE);

    // Apply move to engine board
    m_board.makeMove(chosen);

    // DSA: Stack — push move for undo capability
    m_undoStack.push(chosen);

    refreshLegalMoves();
    updateStatus();

    bool isCheck     = (m_status == CHECK || m_status == CHECKMATE);
    bool isCheckmate = (m_status == CHECKMATE);

    emit moveAnimationRequested(from, to);
    emit moveApplied(chosen, isCapture, isCheck, isCheckmate, isCastle, isPromotion);
    emit boardChanged();
    emit gameStatusChanged(m_status);

    if (m_status == PLAYING || m_status == CHECK)
        triggerAIIfNeeded();

    return true;
}

// ── Undo ─────────────────────────────────────────────────────────────────────

void ChessController::undo()
{
    // DSA: Stack — pop; must undo AI move too if playing HumanVsAI
    if (m_undoStack.isEmpty() || m_aiThinking) return;

    // Undo one half-move
    m_board.undoMove();
    m_undoStack.pop();

    // In HvAI mode, also undo the AI's preceding move (so human plays again)
    if ((m_config.mode == PlayerMode::HumanVsAI ||
         m_config.mode == PlayerMode::AIVsHuman) &&
        !m_undoStack.isEmpty())
    {
        m_board.undoMove();
        m_undoStack.pop();
    }

    m_status = PLAYING;
    refreshLegalMoves();
    updateStatus();

    emit undoPerformed();
    emit boardChanged();
    emit gameStatusChanged(m_status);
}

// ── Legal move helpers ───────────────────────────────────────────────────────

std::vector<Move> ChessController::movesFrom(Square sq) const
{
    std::vector<Move> result;
    for (const Move& m : m_legalMoves)
        if (m.from == sq) result.push_back(m);
    return result;
}

void ChessController::refreshLegalMoves()
{
    MoveGenerator gen(m_board);
    m_legalMoves = gen.generateLegalMoves(m_board.sideToMove);
}

void ChessController::updateStatus()
{
    MoveGenerator gen(m_board);
    Color side = m_board.sideToMove;

    if (gen.isCheckmate(side))      m_status = CHECKMATE;
    else if (gen.isStalemate(side)) m_status = STALEMATE;
    else if (m_board.halfMoveClock >= 100) m_status = DRAW_50_MOVE;
    else if (gen.isInCheck(side))   m_status = CHECK;
    else                            m_status = PLAYING;
}

// ── AI integration ───────────────────────────────────────────────────────────

bool ChessController::isHumanTurn() const
{
    Color side = m_board.sideToMove;
    if (m_config.mode == PlayerMode::HumanVsHuman) return true;
    if (m_config.mode == PlayerMode::HumanVsAI)    return side == m_config.humanColor;
    if (m_config.mode == PlayerMode::AIVsHuman)    return side != m_config.humanColor;
    return true;
}

void ChessController::triggerAIIfNeeded()
{
    if (m_aiThinking)           return;
    if (isHumanTurn())          return;
    if (m_status != PLAYING &&
        m_status != CHECK)      return;

    m_aiThinking = true;
    emit aiThinkingStarted();

    // DSA: Queue — emit goes into the worker thread's event queue
    emit requestAIMove(m_board, m_config.difficulty, m_board.sideToMove);
}

void ChessController::onAIMoveReady(Move bestMove, SearchResult result)
{
    m_aiThinking = false;

    if (m_status != PLAYING && m_status != CHECK) {
        emit aiThinkingFinished(result);
        return;
    }

    // Apply AI move
    bool isCapture   = !m_board.at(bestMove.to).empty() || bestMove.isEnPassant;
    bool isCastle    = bestMove.isCastle;
    bool isPromotion = (bestMove.promotion != NONE_PIECE);

    m_board.makeMove(bestMove);
    m_undoStack.push(bestMove);  // DSA: Stack

    refreshLegalMoves();
    updateStatus();

    bool isCheck     = (m_status == CHECK || m_status == CHECKMATE);
    bool isCheckmate = (m_status == CHECKMATE);

    emit moveAnimationRequested(bestMove.from, bestMove.to);
    emit moveApplied(bestMove, isCapture, isCheck, isCheckmate, isCastle, isPromotion);
    emit boardChanged();
    emit gameStatusChanged(m_status);
    emit aiThinkingFinished(result);
}

void ChessController::onAIProgress(int depth, int score, long nodes)
{
    emit aiProgressUpdate(depth, score, nodes);
}

void ChessController::setBoard(const Board& b)
{
    m_board = b;
    m_undoStack.clear();
    m_status = PLAYING;
    refreshLegalMoves();
    updateStatus();
    emit boardChanged();
    emit gameStatusChanged(m_status);
}
