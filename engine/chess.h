#pragma once
#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <cstdint>
#include <array>

// ─────────────────────────────────────────────
//  Piece & Color enums
// ─────────────────────────────────────────────
enum Color { NONE_COLOR = -1, WHITE = 0, BLACK = 1 };
enum PieceType {
    NONE_PIECE = 0,
    PAWN      = 1,
    KNIGHT    = 2,
    BISHOP    = 3,
    ROOK      = 4,
    QUEEN     = 5,
    KING      = 6
};

// ─────────────────────────────────────────────
//  Square
// ─────────────────────────────────────────────
struct Square {
    int row, col;
    Square(int r = -1, int c = -1) : row(r), col(c) {}
    bool valid() const { return row >= 0 && row < 8 && col >= 0 && col < 8; }
    bool operator==(const Square& o) const { return row == o.row && col == o.col; }
    bool operator!=(const Square& o) const { return !(*this == o); }
    std::string toAlgebraic() const {
        if (!valid()) return "--";
        std::string s;
        s += (char)('a' + col);
        s += (char)('1' + row);
        return s;
    }
    // DSA: Bit Manipulation — compact square index (6 bits)
    int index() const { return row * 8 + col; }
};

// ─────────────────────────────────────────────
//  Piece
// ─────────────────────────────────────────────
struct Piece {
    PieceType type;
    Color     color;
    Piece(PieceType t = NONE_PIECE, Color c = NONE_COLOR) : type(t), color(c) {}
    bool empty() const { return type == NONE_PIECE; }
    char toChar() const;
    // DSA: Bit Manipulation — encode piece as 4-bit integer [color|type]
    int encode() const { return ((int)color << 3) | (int)type; }
};

// ─────────────────────────────────────────────
//  Move
// ─────────────────────────────────────────────
struct Move {
    Square    from, to;
    PieceType promotion;
    bool      isCastle;
    bool      isEnPassant;
    Piece     captured;

    Move() : promotion(NONE_PIECE), isCastle(false), isEnPassant(false) {}
    Move(Square f, Square t, PieceType promo = NONE_PIECE,
         bool castle = false, bool ep = false)
        : from(f), to(t), promotion(promo), isCastle(castle), isEnPassant(ep) {}

    std::string toString() const {
        std::string s = from.toAlgebraic() + to.toAlgebraic();
        if (promotion != NONE_PIECE) {
            const char pt[] = " PNBRQK";
            s += (char)tolower(pt[promotion]);
        }
        return s;
    }

    bool operator==(const Move& o) const {
        return from == o.from && to == o.to && promotion == o.promotion;
    }
};

// ─────────────────────────────────────────────
//  Board state snapshot (undo)
// ─────────────────────────────────────────────
struct BoardState {
    Move     move;
    Square   enPassantSquare;
    bool     castlingRights[2][2];
    int      halfMoveClock;
    Piece    captured;
    uint64_t zobristHash;  // cached hash before the move
};

// ═══════════════════════════════════════════════════════════════
//  DSA: Hash Table + Dynamic Programming — Transposition Table
//
//  Chess positions can be reached via many move orders.  The TT
//  caches (position_hash → {score, depth, flag, best_move}) so
//  we never re-search the same position at the same depth.
//
//  TT_EXACT  : score is the true minimax value   (PV node)
//  TT_ALPHA  : score <= alpha (all-node, stored as upper bound)
//  TT_BETA   : score >= beta  (cut-node, stored as lower bound)
// ═══════════════════════════════════════════════════════════════
enum TTFlag { TT_EXACT = 0, TT_ALPHA = 1, TT_BETA = 2 };

struct TTEntry {
    uint64_t  key      = 0;
    int       score    = 0;
    int       depth    = -1;
    TTFlag    flag     = TT_EXACT;
    Move      bestMove;
    bool      valid    = false;
};

// ═══════════════════════════════════════════════════════════════
//  DSA: Bit Manipulation — Zobrist Hashing
//
//  Assign a unique 64-bit random number to every (piece, square)
//  combination.  The board hash is the XOR of all active entries.
//
//  Key property: XOR is its own inverse.
//    hash ^= pieces[color][type][sq]   adds or removes a piece
//    Incremental updates take O(1) per move.
// ═══════════════════════════════════════════════════════════════
class ZobristKeys {
public:
    uint64_t pieces[2][7][64];  // [color][piece_type][square]
    uint64_t sideToMove;
    uint64_t castling[2][2];    // [color][0=kingside,1=queenside]
    uint64_t enPassant[8];      // en-passant file 0-7

    ZobristKeys();
private:
    static uint64_t rand64(uint64_t& state);
};

// Singleton Zobrist table
extern ZobristKeys ZOBRIST;
