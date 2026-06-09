#pragma once
#include "board.h"

// ═══════════════════════════════════════════════════════════════
//  Evaluator
//  DSA: Heuristic evaluation function used as leaf-node scorer
//       in the Minimax game tree.
//
//  Score convention:  positive  = good for WHITE
//                     negative  = good for BLACK
// ═══════════════════════════════════════════════════════════════

class Evaluator {
public:
    // Material values (centipawns)
    static constexpr int VAL_PAWN   = 100;
    static constexpr int VAL_KNIGHT = 320;
    static constexpr int VAL_BISHOP = 330;
    static constexpr int VAL_ROOK   = 500;
    static constexpr int VAL_QUEEN  = 900;
    static constexpr int VAL_KING   = 20000;

    static constexpr int INF        = 999999;

    // Main evaluation entry point
    static int evaluate(const Board& b);

    // Piece-square table lookup (returns bonus for piece on square)
    static int pst(PieceType pt, Color c, int row, int col, bool endgame = false);

    // Material balance only (fast, for move ordering)
    static int materialScore(const Board& b);

    // MVV-LVA score for a capture (Most Valuable Victim - Least Valuable Attacker)
    static int mvvLva(PieceType attacker, PieceType victim);

private:
    static int pieceValue(PieceType pt);

    // Piece-square tables (from White's perspective, row 0 = rank 1)
    // Stored [row][col]
    static const int PST_PAWN  [8][8];
    static const int PST_KNIGHT[8][8];
    static const int PST_BISHOP[8][8];
    static const int PST_ROOK  [8][8];
    static const int PST_QUEEN [8][8];
    static const int PST_KING_MID[8][8];  // middlegame king safety
    static const int PST_KING_END[8][8];  // endgame king activity
};
