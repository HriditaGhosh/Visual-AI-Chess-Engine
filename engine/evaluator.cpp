#include "evaluator.h"

// ═══════════════════════════════════════════════════════════════
//  Piece-Square Tables  (White's perspective, row 0 = rank 1)
//  Classic Simplified Evaluation Function tables (Chessprogramming wiki)
// ═══════════════════════════════════════════════════════════════

const int Evaluator::PST_PAWN[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },  // rank 1
    {  5, 10, 10,-20,-20, 10, 10,  5 },  // rank 2
    {  5, -5,-10,  0,  0,-10, -5,  5 },
    {  0,  0,  0, 20, 20,  0,  0,  0 },
    {  5,  5, 10, 25, 25, 10,  5,  5 },
    { 10, 10, 20, 30, 30, 20, 10, 10 },
    { 50, 50, 50, 50, 50, 50, 50, 50 },  // rank 7  (about to promote)
    {  0,  0,  0,  0,  0,  0,  0,  0 },  // rank 8  (promoted)
};

const int Evaluator::PST_KNIGHT[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50 },
    {-40,-20,  0,  5,  5,  0,-20,-40 },
    {-30,  5, 10, 15, 15, 10,  5,-30 },
    {-30,  0, 15, 20, 20, 15,  0,-30 },
    {-30,  5, 15, 20, 20, 15,  5,-30 },
    {-30,  0, 10, 15, 15, 10,  0,-30 },
    {-40,-20,  0,  0,  0,  0,-20,-40 },
    {-50,-40,-30,-30,-30,-30,-40,-50 },
};

const int Evaluator::PST_BISHOP[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20 },
    {-10,  5,  0,  0,  0,  0,  5,-10 },
    {-10, 10, 10, 10, 10, 10, 10,-10 },
    {-10,  0, 10, 10, 10, 10,  0,-10 },
    {-10,  5,  5, 10, 10,  5,  5,-10 },
    {-10,  0,  5, 10, 10,  5,  0,-10 },
    {-10,  0,  0,  0,  0,  0,  0,-10 },
    {-20,-10,-10,-10,-10,-10,-10,-20 },
};

const int Evaluator::PST_ROOK[8][8] = {
    {  0,  0,  0,  5,  5,  0,  0,  0 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    {  5, 10, 10, 10, 10, 10, 10,  5 },
    {  0,  0,  0,  0,  0,  0,  0,  0 },
};

const int Evaluator::PST_QUEEN[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20 },
    {-10,  0,  5,  0,  0,  0,  0,-10 },
    {-10,  5,  5,  5,  5,  5,  0,-10 },
    {  0,  0,  5,  5,  5,  5,  0, -5 },
    { -5,  0,  5,  5,  5,  5,  0, -5 },
    {-10,  0,  5,  5,  5,  5,  0,-10 },
    {-10,  0,  0,  0,  0,  0,  0,-10 },
    {-20,-10,-10, -5, -5,-10,-10,-20 },
};

// King: encourage castling / staying safe in middlegame
const int Evaluator::PST_KING_MID[8][8] = {
    { 20, 30, 10,  0,  0, 10, 30, 20 },  // rank 1 – castled king bonus
    { 20, 20,  0,  0,  0,  0, 20, 20 },
    {-10,-20,-20,-20,-20,-20,-20,-10 },
    {-20,-30,-30,-40,-40,-30,-30,-20 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
};

// King: be active in endgame — move to center
const int Evaluator::PST_KING_END[8][8] = {
    {-50,-30,-30,-30,-30,-30,-30,-50 },
    {-30,-20, 20, 20, 20, 20,-20,-30 },
    {-30,-10, 30, 40, 40, 30,-10,-30 },
    {-30,-10, 40, 50, 50, 40,-10,-30 },
    {-30,-10, 40, 50, 50, 40,-10,-30 },
    {-30,-10, 30, 40, 40, 30,-10,-30 },
    {-30,-20, 20, 20, 20, 20,-20,-30 },
    {-50,-30,-30,-30,-30,-30,-30,-50 },
};

// ═══════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════

int Evaluator::pieceValue(PieceType pt) {
    switch (pt) {
        case PAWN:   return VAL_PAWN;
        case KNIGHT: return VAL_KNIGHT;
        case BISHOP: return VAL_BISHOP;
        case ROOK:   return VAL_ROOK;
        case QUEEN:  return VAL_QUEEN;
        case KING:   return VAL_KING;
        default:     return 0;
    }
}

// PST lookup: always in White's frame; flip row for Black
int Evaluator::pst(PieceType pt, Color c, int row, int col, bool endgame) {
    int r = (c == WHITE) ? row : (7 - row);
    switch (pt) {
        case PAWN:   return PST_PAWN  [r][col];
        case KNIGHT: return PST_KNIGHT[r][col];
        case BISHOP: return PST_BISHOP[r][col];
        case ROOK:   return PST_ROOK  [r][col];
        case QUEEN:  return PST_QUEEN [r][col];
        case KING:   return endgame ? PST_KING_END[r][col] : PST_KING_MID[r][col];
        default:     return 0;
    }
}

// ═══════════════════════════════════════════════════════════════
//  Main evaluation
// ═══════════════════════════════════════════════════════════════

int Evaluator::evaluate(const Board& b) {
    int score = 0;

    // Detect endgame: no queens on board, or very low total material
    int totalMaterial = 0;
    bool noQueens = true;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            const Piece& p = b.grid[r][c];
            if (p.empty() || p.type == KING) continue;
            totalMaterial += pieceValue(p.type);
            if (p.type == QUEEN) noQueens = false;
        }
    bool endgame = noQueens || (totalMaterial < 1500);

    // Count material + piece-square bonus
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = b.grid[r][c];
            if (p.empty()) continue;

            int val = pieceValue(p.type) + pst(p.type, p.color, r, c, endgame);
            score += (p.color == WHITE) ? val : -val;
        }
    }

    // Bishop pair bonus
    int whiteBishops = 0, blackBishops = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            if (b.grid[r][c].type == BISHOP) {
                if (b.grid[r][c].color == WHITE) ++whiteBishops;
                else                              ++blackBishops;
            }
        }
    if (whiteBishops >= 2) score += 30;
    if (blackBishops >= 2) score -= 30;

    // Mobility heuristic: rough estimate via pawn structure
    // (Full mobility needs move generation — too slow here; use count of open files)

    return score;
}

// ═══════════════════════════════════════════════════════════════
//  Material only (fast path for move ordering)
// ═══════════════════════════════════════════════════════════════

int Evaluator::materialScore(const Board& b) {
    int score = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            const Piece& p = b.grid[r][c];
            if (p.empty()) continue;
            int v = pieceValue(p.type);
            score += (p.color == WHITE) ? v : -v;
        }
    return score;
}

// ═══════════════════════════════════════════════════════════════
//  MVV-LVA: score captures for move ordering
//  High victim value, low attacker value = search first
// ═══════════════════════════════════════════════════════════════

int Evaluator::mvvLva(PieceType attacker, PieceType victim) {
    return pieceValue(victim) * 10 - pieceValue(attacker);
}
