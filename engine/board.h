#pragma once
#include "chess.h"
#include <cstdint>

// ─────────────────────────────────────────────
//  Board: 8x8 2D array + game state
// ─────────────────────────────────────────────
class Board {
public:
    // DSA: 2D Array – the core board representation
    Piece grid[8][8];

    Color  sideToMove;
    Square enPassantSquare;           // valid target square or (-1,-1)
    bool   castlingRights[2][2];      // [color][0=kingside, 1=queenside]
    int    halfMoveClock;             // for 50-move rule
    int    fullMoveNumber;

    // DSA: Hash Table — incremental Zobrist hash of current position
    // Updated in O(1) on every makeMove/undoMove via XOR operations
    uint64_t zobristHash;

    // DSA: Stack – undo move history
    std::stack<BoardState> history;

    Board();
    void reset();       // set up starting position
    void clear();       // empty board

    // Piece access
    Piece& at(int r, int c)             { return grid[r][c]; }
    Piece& at(Square s)                 { return grid[s.row][s.col]; }
    const Piece& at(int r, int c) const { return grid[r][c]; }
    const Piece& at(Square s)    const  { return grid[s.row][s.col]; }
    void  place(Square s, Piece p)      { grid[s.row][s.col] = p; }
    void  remove(Square s)              { grid[s.row][s.col] = Piece(); }

    // Move execution
    void makeMove(const Move& m);
    void undoMove();

    // King location helper (scans board)
    Square kingSquare(Color c) const;

    // Display
    void print() const;
    std::string fen() const;

    // Compute full Zobrist hash from scratch (called once in reset/clear)
    uint64_t computeZobrist() const;

private:
    void applyMove(const Move& m);
    void revertMove(const BoardState& state);
};
