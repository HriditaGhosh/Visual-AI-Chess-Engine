#include "board.h"
#include <iomanip>

// ─────────────────────────────────────────────
//  Piece::toChar
// ─────────────────────────────────────────────
char Piece::toChar() const {
    if (empty()) return '.';
    const char syms[] = " PNBRQK";
    char c = syms[type];
    return (color == WHITE) ? c : (char)tolower(c);
}

// ─────────────────────────────────────────────
//  Board ctor / reset / clear
// ─────────────────────────────────────────────
Board::Board() {
    reset();
}

void Board::clear() {
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            grid[r][c] = Piece();
    sideToMove    = WHITE;
    enPassantSquare = Square(-1, -1);
    castlingRights[WHITE][0] = castlingRights[WHITE][1] = true;
    castlingRights[BLACK][0] = castlingRights[BLACK][1] = true;
    halfMoveClock  = 0;
    fullMoveNumber = 1;
    while (!history.empty()) history.pop();
    zobristHash = 0;  // will be computed after pieces are placed
}

void Board::reset() {
    clear();
    // Pawns
    for (int c = 0; c < 8; ++c) {
        grid[1][c] = Piece(PAWN, WHITE);
        grid[6][c] = Piece(PAWN, BLACK);
    }
    // Back rank helpers
    auto setup = [&](int row, Color col) {
        grid[row][0] = Piece(ROOK,   col);
        grid[row][1] = Piece(KNIGHT, col);
        grid[row][2] = Piece(BISHOP, col);
        grid[row][3] = Piece(QUEEN,  col);
        grid[row][4] = Piece(KING,   col);
        grid[row][5] = Piece(BISHOP, col);
        grid[row][6] = Piece(KNIGHT, col);
        grid[row][7] = Piece(ROOK,   col);
    };
    setup(0, WHITE);
    setup(7, BLACK);
    zobristHash = computeZobrist();
}

// ─────────────────────────────────────────────
//  Make / Undo
// ─────────────────────────────────────────────
void Board::makeMove(const Move& m) {
    // Save state for undo
    BoardState state;
    state.move            = m;
    state.enPassantSquare = enPassantSquare;
    state.halfMoveClock   = halfMoveClock;
    state.captured        = m.captured;
    state.zobristHash     = zobristHash;  // save hash for O(1) undo
    for (int ci = 0; ci < 2; ++ci)
        for (int j = 0; j < 2; ++j)
            state.castlingRights[ci][j] = castlingRights[ci][j];
    history.push(state);

    applyMove(m);
}

void Board::applyMove(const Move& m) {
    Piece moving = at(m.from);
    // DSA: Bit Manipulation — remove old ep hash contribution before clearing it
    if (enPassantSquare.valid())
        zobristHash ^= ZOBRIST.enPassant[enPassantSquare.col];
    enPassantSquare = Square(-1, -1);   // reset ep

    // ── En passant capture ────────────────────
    if (m.isEnPassant) {
        int epRow = (moving.color == WHITE) ? m.to.row - 1 : m.to.row + 1;
        Square capturedSq(epRow, m.to.col);
        Piece  capturedPawn = at(capturedSq);
        // DSA: Bit Manipulation — XOR out the captured pawn
        zobristHash ^= ZOBRIST.pieces[capturedPawn.color][capturedPawn.type][capturedSq.index()];
        remove(capturedSq);
    }

    // ── Castling: also move the rook ─────────
    if (m.isCastle) {
        bool kingside = (m.to.col > m.from.col);
        int rookFromCol = kingside ? 7 : 0;
        int rookToCol   = kingside ? 5 : 3;
        int row = m.from.row;
        Piece rook = at(Square(row, rookFromCol));
        // DSA: Bit Manipulation — XOR rook out of old square, into new square
        zobristHash ^= ZOBRIST.pieces[rook.color][rook.type][Square(row, rookFromCol).index()];
        zobristHash ^= ZOBRIST.pieces[rook.color][rook.type][Square(row, rookToCol).index()];
        place(Square(row, rookToCol), rook);
        remove(Square(row, rookFromCol));
    }

    // ── Normal capture ────────────────────────
    if (!m.captured.empty() && !m.isEnPassant) {
        // DSA: Bit Manipulation — XOR out captured piece
        zobristHash ^= ZOBRIST.pieces[m.captured.color][m.captured.type][m.to.index()];
        remove(m.to);
    }

    // ── Move the piece ───────────────────────
    Piece dest = moving;
    if (m.promotion != NONE_PIECE) {
        dest = Piece(m.promotion, moving.color);
    }
    // DSA: Bit Manipulation — XOR out piece from source, XOR in at destination
    zobristHash ^= ZOBRIST.pieces[moving.color][moving.type][m.from.index()];
    zobristHash ^= ZOBRIST.pieces[dest.color][dest.type][m.to.index()];
    place(m.to, dest);
    remove(m.from);

    // ── Update en passant square ─────────────
    if (moving.type == PAWN && abs(m.to.row - m.from.row) == 2) {
        int epRow = (m.from.row + m.to.row) / 2;
        enPassantSquare = Square(epRow, m.from.col);
        // DSA: Bit Manipulation — XOR in new ep file
        zobristHash ^= ZOBRIST.enPassant[m.from.col];
    }

    // ── Update castling rights ───────────────
    auto revokeCastle = [&](Square sq, Color c, int side) {
        if ((m.from == sq || m.to == sq) && castlingRights[c][side]) {
            // DSA: Bit Manipulation — XOR out the castling right being revoked
            zobristHash ^= ZOBRIST.castling[c][side];
            castlingRights[c][side] = false;
        }
    };
    // King moves — revoke both sides
    if (moving.type == KING) {
        if (castlingRights[moving.color][0]) {
            zobristHash ^= ZOBRIST.castling[moving.color][0];
            castlingRights[moving.color][0] = false;
        }
        if (castlingRights[moving.color][1]) {
            zobristHash ^= ZOBRIST.castling[moving.color][1];
            castlingRights[moving.color][1] = false;
        }
    }
    revokeCastle(Square(0,0), WHITE, 1);
    revokeCastle(Square(0,7), WHITE, 0);
    revokeCastle(Square(7,0), BLACK, 1);
    revokeCastle(Square(7,7), BLACK, 0);

    // ── Half-move clock ──────────────────────
    if (moving.type == PAWN || !m.captured.empty())
        halfMoveClock = 0;
    else
        ++halfMoveClock;

    // ── Full-move number ─────────────────────
    if (sideToMove == BLACK) ++fullMoveNumber;
    sideToMove = (sideToMove == WHITE) ? BLACK : WHITE;
    // DSA: Bit Manipulation — flip side-to-move bit
    zobristHash ^= ZOBRIST.sideToMove;
}

void Board::undoMove() {
    if (history.empty()) return;
    BoardState state = history.top();
    history.pop();

    revertMove(state);
    // DSA: Hash Table — restore hash O(1) from snapshot
    zobristHash = state.zobristHash;
}

void Board::revertMove(const BoardState& state) {
    const Move& m = state.move;

    // Restore side to move
    sideToMove = (sideToMove == WHITE) ? BLACK : WHITE;

    Piece moved = at(m.to);
    // If promotion, revert to pawn
    if (m.promotion != NONE_PIECE)
        moved = Piece(PAWN, sideToMove);

    // Move piece back
    place(m.from, moved);
    remove(m.to);

    // Restore captured piece
    if (!state.captured.empty() && !m.isEnPassant)
        place(m.to, state.captured);

    // Restore en-passant captured pawn
    if (m.isEnPassant) {
        int epRow = (sideToMove == WHITE) ? m.to.row - 1 : m.to.row + 1;
        Color opp = (sideToMove == WHITE) ? BLACK : WHITE;
        place(Square(epRow, m.to.col), Piece(PAWN, opp));
    }

    // Restore rook for castling
    if (m.isCastle) {
        bool kingside = (m.to.col > m.from.col);
        int rookToCol   = kingside ? 5 : 3;
        int rookFromCol = kingside ? 7 : 0;
        int row = m.from.row;
        place(Square(row, rookFromCol), at(Square(row, rookToCol)));
        remove(Square(row, rookToCol));
    }

    // Restore state fields
    enPassantSquare = state.enPassantSquare;
    halfMoveClock   = state.halfMoveClock;
    for (int ci = 0; ci < 2; ++ci)
        for (int j = 0; j < 2; ++j)
            castlingRights[ci][j] = state.castlingRights[ci][j];

    if (sideToMove == BLACK) --fullMoveNumber;
}

// ─────────────────────────────────────────────
//  King location
// ─────────────────────────────────────────────
Square Board::kingSquare(Color c) const {
    for (int r = 0; r < 8; ++r)
        for (int col = 0; col < 8; ++col)
            if (grid[r][col].type == KING && grid[r][col].color == c)
                return Square(r, col);
    return Square(-1, -1);
}

// ─────────────────────────────────────────────
//  Print
// ─────────────────────────────────────────────
void Board::print() const {
    std::cout << "\n    a  b  c  d  e  f  g  h\n";
    std::cout <<   "   ─────────────────────────\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << " " << (r + 1) << " │";
        for (int c = 0; c < 8; ++c) {
            char ch = grid[r][c].toChar();
            // Colour the square background with ANSI
            bool lightSq = (r + c) % 2 == 0;
            std::string bg = lightSq ? "\033[48;5;180m" : "\033[48;5;94m";
            std::string reset = "\033[0m";
            // White pieces bold white, black pieces bold black
            std::string fg = grid[r][c].empty() ? "" :
                             (grid[r][c].color == WHITE ? "\033[1;97m" : "\033[1;30m");
            std::cout << bg << fg << " " << ch << " " << reset;
        }
        std::cout << "│ " << (r + 1) << "\n";
    }
    std::cout <<   "   ─────────────────────────\n";
    std::cout <<   "    a  b  c  d  e  f  g  h\n\n";
    std::cout << "Side to move: " << (sideToMove == WHITE ? "White" : "Black") << "\n";
    if (enPassantSquare.valid())
        std::cout << "En passant target: " << enPassantSquare.toAlgebraic() << "\n";
}

// ─────────────────────────────────────────────
//  Minimal FEN export
// ─────────────────────────────────────────────
std::string Board::fen() const {
    std::ostringstream oss;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int c = 0; c < 8; ++c) {
            if (grid[r][c].empty()) { ++empty; }
            else {
                if (empty) { oss << empty; empty = 0; }
                oss << grid[r][c].toChar();
            }
        }
        if (empty) oss << empty;
        if (r > 0) oss << '/';
    }
    oss << ' ' << (sideToMove == WHITE ? 'w' : 'b') << ' ';
    // Castling
    std::string castle;
    if (castlingRights[WHITE][0]) castle += 'K';
    if (castlingRights[WHITE][1]) castle += 'Q';
    if (castlingRights[BLACK][0]) castle += 'k';
    if (castlingRights[BLACK][1]) castle += 'q';
    oss << (castle.empty() ? "-" : castle) << ' ';
    oss << (enPassantSquare.valid() ? enPassantSquare.toAlgebraic() : "-") << ' ';
    oss << halfMoveClock << ' ' << fullMoveNumber;
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════
//  computeZobrist — build full hash from scratch (used once on reset)
//  DSA: Bit Manipulation — XOR all active piece keys together
// ═══════════════════════════════════════════════════════════════
uint64_t Board::computeZobrist() const {
    uint64_t h = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            const Piece& p = grid[r][c];
            if (!p.empty())
                h ^= ZOBRIST.pieces[p.color][p.type][Square(r,c).index()];
        }
    if (sideToMove == BLACK) h ^= ZOBRIST.sideToMove;
    for (int ci = 0; ci < 2; ++ci)
        for (int s = 0; s < 2; ++s)
            if (castlingRights[ci][s]) h ^= ZOBRIST.castling[ci][s];
    if (enPassantSquare.valid())
        h ^= ZOBRIST.enPassant[enPassantSquare.col];
    return h;
}
