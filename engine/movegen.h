#pragma once
#include "board.h"

// ─────────────────────────────────────────────
//  MoveGenerator: produces pseudo-legal and legal moves
//  DSA: Recursion is used in isInCheck (calls into attack detection
//       which in turn calls pseudo-legal generation)
// ─────────────────────────────────────────────
class MoveGenerator {
public:
    explicit MoveGenerator(Board& board) : b(board) {}

    // DSA: Vector – move list
    std::vector<Move> generateLegalMoves(Color side);
    std::vector<Move> generatePseudoLegalMoves(Color side);

    bool isInCheck(Color side);
    bool isCheckmate(Color side);
    bool isStalemate(Color side);

    // Attack queries
    bool isSquareAttackedBy(Square sq, Color attacker);

private:
    Board& b;

    // Per-piece pseudo-legal generators
    void genPawnMoves   (Square from, std::vector<Move>& out);
    void genKnightMoves (Square from, std::vector<Move>& out);
    void genBishopMoves (Square from, std::vector<Move>& out);
    void genRookMoves   (Square from, std::vector<Move>& out);
    void genQueenMoves  (Square from, std::vector<Move>& out);
    void genKingMoves   (Square from, std::vector<Move>& out);

    void addPawnPromotions(Square from, Square to, std::vector<Move>& out);
    void slideMoves(Square from, int dr, int dc, std::vector<Move>& out);

    // Recursion helper: tests if making a move leaves own king in check
    bool leavesKingInCheck(const Move& m, Color side);
};
