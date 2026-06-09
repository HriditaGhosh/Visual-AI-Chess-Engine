#pragma once
#include "movegen.h"
#include "evaluator.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include <string>

// ═══════════════════════════════════════════════════════════════
//  Difficulty levels
// ═══════════════════════════════════════════════════════════════
enum Difficulty {
    EASY   = 1,
    MEDIUM = 3,
    HARD   = 5,
    EXPERT = 7
};

// ═══════════════════════════════════════════════════════════════
//  SearchResult — returned from findBestMove
// ═══════════════════════════════════════════════════════════════
struct SearchResult {
    Move   bestMove;
    int    score;
    long   nodesSearched;
    int    depth;
    double timeMs;
    int    ttHits;     // transposition table hits
};

// ═══════════════════════════════════════════════════════════════
//  DSA: Hash Table — Transposition Table
//
//  A fixed-size array indexed by (hash % size).
//  Stores position evaluations to avoid recomputing the same node.
//  This is the Dynamic Programming memoisation component of the engine.
// ═══════════════════════════════════════════════════════════════
class TranspositionTable {
public:
    static constexpr size_t TT_SIZE = (1 << 22);  // 4 M entries (~256 MB)
    static constexpr size_t TT_MASK = TT_SIZE - 1;

    TranspositionTable();

    // Store an entry (always-replace strategy for simplicity)
    void store(uint64_t hash, int score, int depth, TTFlag flag, const Move& best);

    // Probe: returns true if a usable entry was found
    bool probe(uint64_t hash, int depth, int alpha, int beta,
               int& score, Move& bestMove) const;

    // Retrieve best move only (for move ordering at root)
    bool probeMove(uint64_t hash, Move& bestMove) const;

    void clear();
    int  hits() const { return hitCount; }

private:
    std::vector<TTEntry> table;
    mutable int hitCount;
};

// ═══════════════════════════════════════════════════════════════
//  Opening Book — hardcoded common opening lines
//
//  DSA: Hash Table — maps Zobrist hash to a list of book moves.
//  The engine plays book moves for the first several moves,
//  providing theoretically sound openings without search cost.
// ═══════════════════════════════════════════════════════════════
class OpeningBook {
public:
    OpeningBook();

    // Returns a book move if one exists for this position hash, else null move
    bool probe(uint64_t hash, Move& out) const;

private:
    // map: zobrist_hash -> list of (move_uci, weight)
    std::unordered_map<uint64_t, std::vector<std::pair<std::string, int>>> book;

    // Seed the book with common lines
    void addLine(const std::vector<std::string>& moves, Board& scratch);
};

// ═══════════════════════════════════════════════════════════════
//  AIEngine — the full optimized search engine
//
//  New algorithms vs. the original:
//
//  ① Iterative Deepening DFS (IDDFS)
//       Search depth 1, then 2, …, then maxDepth.
//       Each shallower pass populates the TT and provides a
//       good move order so the next pass prunes aggressively.
//
//  ② Transposition Table (Hash Table + Dynamic Programming)
//       Cache (position → score) keyed by Zobrist hash.
//       Avoids re-searching transpositions; enables TT move ordering.
//
//  ③ Principal Variation Search (PVS / NegaScout)
//       After the first (PV) move, search remaining moves with a
//       zero-window [alpha, alpha+1].  If a move exceeds alpha,
//       re-search with full window.  Faster than plain alpha-beta.
//
//  ④ Null Move Pruning
//       Let the opponent move twice.  If the position is still
//       good enough (score >= beta), the subtree is pruned.
//       Safe at depth >= 3; disabled in endgame / zugzwang zones.
//
//  ⑤ Killer Move Heuristic (Priority Queue / sorted list idea)
//       Non-captures that caused a beta-cutoff in a sibling node
//       are stored as "killers" per ply.  Tried immediately after
//       captures in the move-ordering step.
//
//  ⑥ History Heuristic (Hash Table)
//       A 2D table [from][to] accumulates bonus for quiet moves
//       that caused cutoffs.  Used in move ordering.
//
//  ⑦ Quiescence Search (already present; enhanced with delta pruning)
//       Continue searching captures after depth=0 to avoid the
//       horizon effect.  Delta pruning skips hopeless captures.
//
//  ⑧ Zobrist Incremental Hashing (Bit Manipulation)
//       Board hash updated with XOR in O(1) per make/unmake.
// ═══════════════════════════════════════════════════════════════
class AIEngine {
public:
    explicit AIEngine(Board& board);

    // Find best move using Iterative Deepening up to given depth
    SearchResult findBestMove(Color side, int depth);
    SearchResult findBestMove(Color side, Difficulty diff);

    long getNodes() const { return nodes; }

    // Max killers per ply
    static constexpr int MAX_KILLERS = 2;
    // Max search depth supported
    static constexpr int MAX_DEPTH   = 64;

private:
    Board&             b;
    MoveGenerator      gen;
    TranspositionTable tt;
    OpeningBook        book;

    long  nodes;
    int   ttHits;

    // ── Killer Move Heuristic ────────────────────────────────
    // DSA: Array (per-ply priority list) — stores 2 killer moves per ply.
    // Non-capture moves that caused beta-cutoffs are stored here and
    // tried early in move ordering (before history moves).
    Move killers[MAX_DEPTH][MAX_KILLERS];

    // ── History Heuristic ─────────────────────────────────────
    // DSA: Hash Table (2D array) — records how often a [from][to]
    // move caused a beta-cutoff across the whole search.
    // Indexed by [from_sq][to_sq].
    int historyTable[64][64];

    void clearHeuristics();

    // ── PVS / Alpha-Beta ─────────────────────────────────────
    // Returns score from the perspective of the side to move.
    // DSA: Recursion + DFS + Branch-and-Bound
    int pvs(int depth, int alpha, int beta, Color side,
            bool maximising, int ply, bool nullMoveAllowed);

    // ── Quiescence Search ────────────────────────────────────
    int quiesce(int alpha, int beta, Color side, bool maximising);

    // ── Move Ordering ─────────────────────────────────────────
    // Priority queue concept: assign scores then sort descending.
    // Order: TT move > captures (MVV-LVA) > killers > history > quiet
    void orderMoves(std::vector<Move>& moves,
                    const Move& ttMove,
                    int ply);

    int  scoreMoveForOrdering(const Move& m,
                               const Move& ttMove,
                               int ply);

    // ── Killer / History updates ─────────────────────────────
    void storeKiller(int ply, const Move& m);
    bool isKiller(int ply, const Move& m) const;

    void updateHistory(const Move& m, int depth, Color side);

    // ── Helpers ──────────────────────────────────────────────
    bool isNullMoveSafe(Color side);
    static constexpr int NULL_MOVE_R = 2;  // null-move reduction
};
