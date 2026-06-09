#include "movegen.h"

// ─────────────────────────────────────────────
//  Pawn promotions helper: adds all 4 promotion moves
// ─────────────────────────────────────────────
void MoveGenerator::addPawnPromotions(Square from, Square to,
                                      std::vector<Move>& out) {
    Piece target = b.at(to);  // empty for push promotions, enemy piece for captures
    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT}) {
        Move m(from, to, pt);
        m.captured = target;  // correctly empty for push, set for capture
        out.push_back(m);
    }
}

// ─────────────────────────────────────────────
//  Sliding pieces: rook/bishop rays
// ─────────────────────────────────────────────
void MoveGenerator::slideMoves(Square from, int dr, int dc,
                                std::vector<Move>& out) {
    Color own = b.at(from).color;
    Square sq(from.row + dr, from.col + dc);
    while (sq.valid()) {
        if (!b.at(sq).empty()) {
            if (b.at(sq).color != own) {
                Move m(from, sq);
                m.captured = b.at(sq);
                out.push_back(m);
            }
            break;
        }
        out.push_back(Move(from, sq));
        sq.row += dr;
        sq.col += dc;
    }
}

// ─────────────────────────────────────────────
//  Pawn
// ─────────────────────────────────────────────
void MoveGenerator::genPawnMoves(Square from, std::vector<Move>& out) {
    Piece p   = b.at(from);
    int   dir = (p.color == WHITE) ? 1 : -1;
    int   startRow  = (p.color == WHITE) ? 1 : 6;
    int   promoRow  = (p.color == WHITE) ? 6 : 1;  // row FROM which promotion happens

    // Single push
    Square one(from.row + dir, from.col);
    if (one.valid() && b.at(one).empty()) {
        if (from.row == promoRow)
            addPawnPromotions(from, one, out);
        else
            out.push_back(Move(from, one));

        // Double push from start row
        if (from.row == startRow) {
            Square two(from.row + 2 * dir, from.col);
            if (b.at(two).empty())
                out.push_back(Move(from, two));
        }
    }

    // Captures (diagonal)
    for (int dc : {-1, 1}) {
        Square cap(from.row + dir, from.col + dc);
        if (!cap.valid()) continue;

        // Normal capture
        if (!b.at(cap).empty() && b.at(cap).color != p.color) {
            if (from.row == promoRow) {
                addPawnPromotions(from, cap, out);
            } else {
                Move m(from, cap);
                m.captured = b.at(cap);
                out.push_back(m);
            }
        }

        // En passant
        if (cap == b.enPassantSquare) {
            Move m(from, cap, NONE_PIECE, false, true);
            m.captured = Piece(PAWN, (p.color == WHITE) ? BLACK : WHITE);
            out.push_back(m);
        }
    }
}

// ─────────────────────────────────────────────
//  Knight
// ─────────────────────────────────────────────
void MoveGenerator::genKnightMoves(Square from, std::vector<Move>& out) {
    Color own = b.at(from).color;
    static const int jumps[8][2] = {
        {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
    };
    for (auto& j : jumps) {
        Square to(from.row + j[0], from.col + j[1]);
        if (!to.valid()) continue;
        if (!b.at(to).empty() && b.at(to).color == own) continue;
        Move m(from, to);
        m.captured = b.at(to);
        out.push_back(m);
    }
}

// ─────────────────────────────────────────────
//  Bishop / Rook / Queen
// ─────────────────────────────────────────────
void MoveGenerator::genBishopMoves(Square from, std::vector<Move>& out) {
    for (auto [dr,dc] : std::vector<std::pair<int,int>>{{1,1},{1,-1},{-1,1},{-1,-1}})
        slideMoves(from, dr, dc, out);
}

void MoveGenerator::genRookMoves(Square from, std::vector<Move>& out) {
    for (auto [dr,dc] : std::vector<std::pair<int,int>>{{1,0},{-1,0},{0,1},{0,-1}})
        slideMoves(from, dr, dc, out);
}

void MoveGenerator::genQueenMoves(Square from, std::vector<Move>& out) {
    genBishopMoves(from, out);
    genRookMoves(from, out);
}

// ─────────────────────────────────────────────
//  King (normal moves + castling)
// ─────────────────────────────────────────────
void MoveGenerator::genKingMoves(Square from, std::vector<Move>& out) {
    Color own = b.at(from).color;
    static const int deltas[8][2] = {
        {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}
    };
    Color opp = (own == WHITE) ? BLACK : WHITE;

    for (auto& d : deltas) {
        Square to(from.row + d[0], from.col + d[1]);
        if (!to.valid()) continue;
        if (!b.at(to).empty() && b.at(to).color == own) continue;
        Move m(from, to);
        m.captured = b.at(to);
        out.push_back(m);
    }

    // ── Castling ──────────────────────────────
    if (isInCheck(own)) return;   // can't castle while in check

    int row = (own == WHITE) ? 0 : 7;
    // Kingside (O-O)
    if (b.castlingRights[own][0]) {
        if (b.at(row, 5).empty() && b.at(row, 6).empty()) {
            if (!isSquareAttackedBy(Square(row, 5), opp) &&
                !isSquareAttackedBy(Square(row, 6), opp)) {
                out.push_back(Move(from, Square(row, 6), NONE_PIECE, true));
            }
        }
    }
    // Queenside (O-O-O)
    if (b.castlingRights[own][1]) {
        if (b.at(row, 1).empty() && b.at(row, 2).empty() && b.at(row, 3).empty()) {
            if (!isSquareAttackedBy(Square(row, 3), opp) &&
                !isSquareAttackedBy(Square(row, 2), opp)) {
                out.push_back(Move(from, Square(row, 2), NONE_PIECE, true));
            }
        }
    }
}

// ─────────────────────────────────────────────
//  Pseudo-legal generation for all pieces
// ─────────────────────────────────────────────
std::vector<Move> MoveGenerator::generatePseudoLegalMoves(Color side) {
    std::vector<Move> moves;
    moves.reserve(60);
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = b.grid[r][c];
            if (p.empty() || p.color != side) continue;
            Square from(r, c);
            switch (p.type) {
                case PAWN:   genPawnMoves  (from, moves); break;
                case KNIGHT: genKnightMoves(from, moves); break;
                case BISHOP: genBishopMoves(from, moves); break;
                case ROOK:   genRookMoves  (from, moves); break;
                case QUEEN:  genQueenMoves (from, moves); break;
                case KING:   genKingMoves  (from, moves); break;
                default: break;
            }
        }
    }
    return moves;
}

// ─────────────────────────────────────────────
//  DSA: Recursion – leavesKingInCheck makes a move,
//  then calls isInCheck (which calls generatePseudoLegalMoves →
//  isSquareAttackedBy), forming a recursive validation chain.
// ─────────────────────────────────────────────
bool MoveGenerator::leavesKingInCheck(const Move& m, Color side) {
    b.makeMove(m);
    bool inCheck = isInCheck(side);   // <-- recursive step into attack detection
    b.undoMove();
    return inCheck;
}

// ─────────────────────────────────────────────
//  Legal move generation: filter pseudo-legal
// ─────────────────────────────────────────────
std::vector<Move> MoveGenerator::generateLegalMoves(Color side) {
    std::vector<Move> pseudo = generatePseudoLegalMoves(side);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());
    for (const Move& m : pseudo) {
        if (!leavesKingInCheck(m, side))
            legal.push_back(m);
    }
    return legal;
}

// ─────────────────────────────────────────────
//  isSquareAttackedBy: checks if `sq` is attacked by `attacker`
// ─────────────────────────────────────────────
bool MoveGenerator::isSquareAttackedBy(Square sq, Color attacker) {
    // Check all attacker pieces for pseudo-legal moves that land on sq
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = b.grid[r][c];
            if (p.empty() || p.color != attacker) continue;
            Square from(r, c);
            std::vector<Move> tmp;
            switch (p.type) {
                case PAWN:   genPawnMoves  (from, tmp); break;
                case KNIGHT: genKnightMoves(from, tmp); break;
                case BISHOP: genBishopMoves(from, tmp); break;
                case ROOK:   genRookMoves  (from, tmp); break;
                case QUEEN:  genQueenMoves (from, tmp); break;
                case KING: {
                    // bare king attacks (no castling here to avoid infinite loop)
                    static const int d[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
                    for (auto& dd : d) {
                        Square to(r+dd[0], c+dd[1]);
                        if (to.valid()) tmp.push_back(Move(from, to));
                    }
                    break;
                }
                default: break;
            }
            for (const Move& m : tmp) {
                if (m.to == sq) return true;
            }
        }
    }
    return false;
}

// ─────────────────────────────────────────────
//  isInCheck
// ─────────────────────────────────────────────
bool MoveGenerator::isInCheck(Color side) {
    Square ksq = b.kingSquare(side);
    if (!ksq.valid()) return false;
    Color opp = (side == WHITE) ? BLACK : WHITE;
    return isSquareAttackedBy(ksq, opp);
}

// ─────────────────────────────────────────────
//  Checkmate / Stalemate
// ─────────────────────────────────────────────
bool MoveGenerator::isCheckmate(Color side) {
    return isInCheck(side) && generateLegalMoves(side).empty();
}

bool MoveGenerator::isStalemate(Color side) {
    return !isInCheck(side) && generateLegalMoves(side).empty();
}
