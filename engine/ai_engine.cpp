#include "ai_engine.h"
#include <cassert>
#include <cstring>
#include <random>

// ═══════════════════════════════════════════════════════════════
//  TranspositionTable implementation
//  DSA: Hash Table — fixed-size array with Zobrist key indexing
// ═══════════════════════════════════════════════════════════════

TranspositionTable::TranspositionTable()
    : table(TT_SIZE), hitCount(0) {}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry{});
    hitCount = 0;
}

void TranspositionTable::store(uint64_t hash, int score, int depth,
                                TTFlag flag, const Move& best) {
    // DSA: Bit Manipulation — index into table using low bits of hash
    size_t idx = hash & TT_MASK;
    TTEntry& e = table[idx];

    // Two-condition replacement: always fill empty slots; for occupied slots
    // only replace when the new search depth is >= stored depth so shallow
    // IDDFS passes don't evict deeply-searched results from earlier iterations.
    if (!e.valid || depth >= e.depth) {
        e.key      = hash;
        e.score    = score;
        e.depth    = depth;
        e.flag     = flag;
        e.bestMove = best;
        e.valid    = true;
    }
}

bool TranspositionTable::probe(uint64_t hash, int depth, int alpha, int beta,
                                int& score, Move& bestMove) const {
    // DSA: Bit Manipulation — index via low bits (same as store)
    size_t idx = hash & TT_MASK;
    const TTEntry& e = table[idx];

    if (!e.valid || e.key != hash) return false;

    bestMove = e.bestMove;  // always return best move for ordering

    if (e.depth < depth) return false;  // not searched deeply enough

    // DSA: Dynamic Programming — reuse cached result based on flag type
    if (e.flag == TT_EXACT) {
        score = e.score;
        ++hitCount;
        return true;
    }
    if (e.flag == TT_ALPHA && e.score <= alpha) {
        score = alpha;
        ++hitCount;
        return true;
    }
    if (e.flag == TT_BETA && e.score >= beta) {
        score = beta;
        ++hitCount;
        return true;
    }
    return false;
}

bool TranspositionTable::probeMove(uint64_t hash, Move& bestMove) const {
    size_t idx = hash & TT_MASK;
    const TTEntry& e = table[idx];
    if (e.valid && e.key == hash) {
        bestMove = e.bestMove;
        return true;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════
//  Opening Book
//  DSA: Hash Table — map from position Zobrist hash to book moves
// ═══════════════════════════════════════════════════════════════

OpeningBook::OpeningBook() {
    Board scratch;
    scratch.reset();

    // ── Common opening lines ──────────────────────────────────
    // Each line is a sequence of UCI moves from the starting position.
    // We replay each move, recording (hash -> next_move) at every step.

    // Open games (1.e4)
    addLine({"e2e4","e7e5","g1f3","b8c6","f1b5"},                  scratch); // Ruy Lopez
    addLine({"e2e4","e7e5","g1f3","b8c6","f1c4"},                  scratch); // Italian
    addLine({"e2e4","e7e5","g1f3","b8c6","f1c4","g8f6","b1c3"},    scratch); // 3-knight Italian
    addLine({"e2e4","e7e5","g1f3","g8f6"},                         scratch); // Petrov
    addLine({"e2e4","e7e5","f2f4"},                                scratch); // King's Gambit
    addLine({"e2e4","c7c5"},                                       scratch); // Sicilian
    addLine({"e2e4","c7c5","g1f3","d7d6","d2d4","c5d4","f3d4"},    scratch); // Sicilian Open
    addLine({"e2e4","e7e6"},                                       scratch); // French
    addLine({"e2e4","e7e6","d2d4","d7d5","b1c3"},                  scratch); // French Classical
    addLine({"e2e4","c7c6"},                                       scratch); // Caro-Kann
    addLine({"e2e4","d7d5"},                                       scratch); // Scandinavian

    // Closed games (1.d4)
    addLine({"d2d4","d7d5","c2c4"},                                scratch); // Queen's Gambit
    addLine({"d2d4","d7d5","c2c4","e7e6","b1c3","g8f6","c1g5"},   scratch); // QGD Orthodox
    addLine({"d2d4","d7d5","c2c4","c7c6"},                         scratch); // Slav
    addLine({"d2d4","g8f6","c2c4","e7e6"},                         scratch); // Queen's Indian / Nimzo
    addLine({"d2d4","g8f6","c2c4","g7g6","b1c3","f8g7"},           scratch); // King's Indian
    addLine({"d2d4","g8f6","c2c4","c7c5"},                         scratch); // Benoni
    addLine({"d2d4","f7f5"},                                       scratch); // Dutch

    // Flank openings
    addLine({"g1f3","d7d5","g2g3"},                                scratch); // Reti
    addLine({"c2c4"},                                              scratch); // English
    addLine({"c2c4","e7e5","b1c3","g8f6","g2g3"},                  scratch); // English Reversed Sicilian
}

void OpeningBook::addLine(const std::vector<std::string>& moves, Board& scratch) {
    scratch.reset();
    MoveGenerator gen(scratch);

    for (size_t i = 0; i + 1 < moves.size(); ++i) {
        // Parse UCI move
        const std::string& uci = moves[i];
        if (uci.size() < 4) break;

        int fc = uci[0] - 'a', fr = uci[1] - '1';
        int tc = uci[2] - 'a', tr = uci[3] - '1';
        Square from(fr, fc), to(tr, tc);

        PieceType promo = NONE_PIECE;
        if (uci.size() == 5) {
            switch (uci[4]) {
                case 'q': promo = QUEEN;  break;
                case 'r': promo = ROOK;   break;
                case 'b': promo = BISHOP; break;
                case 'n': promo = KNIGHT; break;
            }
        }

        // Find matching legal move
        auto legal = gen.generateLegalMoves(scratch.sideToMove);
        Move found;
        bool ok = false;
        for (const Move& m : legal) {
            if (m.from == from && m.to == to && m.promotion == promo) {
                found = m;
                ok    = true;
                break;
            }
        }
        if (!ok) break;

        // Record: at this hash, the next move in the line is moves[i+1]
        uint64_t h = scratch.zobristHash;
        book[h].push_back({moves[i + 1], 1});

        scratch.makeMove(found);
    }
    scratch.reset();
}

bool OpeningBook::probe(uint64_t hash, Move& out) const {
    auto it = book.find(hash);
    if (it == book.end() || it->second.empty()) return false;

    // DSA: Weighted random selection — pick among book moves proportional to weight.
    // Prevents the engine always playing the same response, which is exploitable.
    const auto& candidates = it->second;
    int totalWeight = 0;
    for (const auto& [uci, w] : candidates) totalWeight += w;

    // Use the hash itself as a lightweight seed for determinism within a position
    // but variation across different games
    std::mt19937 rng(hash ^ static_cast<uint64_t>(std::chrono::steady_clock::now()
                         .time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, totalWeight - 1);
    int roll = dist(rng);

    const std::string* chosen = nullptr;
    for (const auto& [uci, w] : candidates) {
        roll -= w;
        if (roll < 0) { chosen = &uci; break; }
    }
    if (!chosen) chosen = &candidates.back().first;

    const std::string& uci = *chosen;
    if (uci.size() < 4) return false;

    int fc = uci[0] - 'a', fr = uci[1] - '1';
    int tc = uci[2] - 'a', tr = uci[3] - '1';
    out.from = Square(fr, fc);
    out.to   = Square(tr, tc);
    out.promotion = NONE_PIECE;
    if (uci.size() == 5) {
        switch (uci[4]) {
            case 'q': out.promotion = QUEEN;  break;
            case 'r': out.promotion = ROOK;   break;
            case 'b': out.promotion = BISHOP; break;
            case 'n': out.promotion = KNIGHT; break;
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
//  AIEngine constructor
// ═══════════════════════════════════════════════════════════════

AIEngine::AIEngine(Board& board)
    : b(board), gen(board), nodes(0), ttHits(0) {
    clearHeuristics();
}

void AIEngine::clearHeuristics() {
    // Zero-init killers and history tables
    std::memset(killers,      0, sizeof(killers));
    std::memset(historyTable, 0, sizeof(historyTable));
}

// ═══════════════════════════════════════════════════════════════
//  Killer Move Heuristic helpers
//  DSA: Per-ply list (priority queue idea) — 2 slots per ply
// ═══════════════════════════════════════════════════════════════

void AIEngine::storeKiller(int ply, const Move& m) {
    if (ply >= MAX_DEPTH) return;
    // Don't store duplicates
    if (killers[ply][0] == m) return;
    // Shift: new killer goes to slot 0, old slot 0 to slot 1
    killers[ply][1] = killers[ply][0];
    killers[ply][0] = m;
}

bool AIEngine::isKiller(int ply, const Move& m) const {
    if (ply >= MAX_DEPTH) return false;
    return (killers[ply][0] == m) || (killers[ply][1] == m);
}

// ═══════════════════════════════════════════════════════════════
//  History Heuristic update
//  DSA: Hash Table (2D array) — bonus proportional to depth
// ═══════════════════════════════════════════════════════════════

void AIEngine::updateHistory(const Move& m, int depth, Color /*side*/) {
    int fi = m.from.index();
    int ti = m.to.index();
    // Depth-squared bonus so deeper cutoffs get more credit
    historyTable[fi][ti] += depth * depth;
    // Cap to prevent overflow after very long searches
    if (historyTable[fi][ti] > 1000000)
        historyTable[fi][ti] = 1000000;
}

// ═══════════════════════════════════════════════════════════════
//  Move ordering
//
//  DSA: Priority Queue concept — assign integer scores, sort descending.
//
//  Priority order (highest first):
//    1. TT/PV move          (+1 000 000)  — searched best last iteration
//    2. Winning captures    (MVV-LVA)     — most valuable victim
//    3. Queen promotions    (+900 000)
//    4. Killer moves        (+800 000)    — caused cutoff at this ply
//    5. History heuristic   (0..~1 M)     — caused cutoffs elsewhere
//    6. PST delta           (small)       — positional improvement
//    7. Losing captures     (negative)
// ═══════════════════════════════════════════════════════════════

int AIEngine::scoreMoveForOrdering(const Move& m,
                                    const Move& ttMove,
                                    int ply) {
    int score = 0;

    // ① TT / PV move — try first
    if (m == ttMove) return 2000000;

    // ② Captures (MVV-LVA)
    if (!m.captured.empty() || m.isEnPassant) {
        PieceType victim = m.isEnPassant ? PAWN : m.captured.type;
        score += 1000000 + Evaluator::mvvLva(b.at(m.from).type, victim);
        return score;
    }

    // ③ Promotions
    if (m.promotion == QUEEN)  return 900000;
    if (m.promotion != NONE_PIECE) return 800000;

    // ④ Killer moves
    if (isKiller(ply, m)) return 700000;

    // ⑤ History heuristic
    score = historyTable[m.from.index()][m.to.index()];

    // ⑥ PST delta — small positional bonus
    Piece p = b.at(m.from);
    if (!p.empty()) {
        int delta = Evaluator::pst(p.type, p.color, m.to.row, m.to.col)
                  - Evaluator::pst(p.type, p.color, m.from.row, m.from.col);
        score += delta;
    }

    return score;
}

void AIEngine::orderMoves(std::vector<Move>& moves,
                           const Move& ttMove,
                           int ply) {
    // DSA: O(n log n) sort with inline scoring — captures the priority queue idea
    std::sort(moves.begin(), moves.end(),
        [&](const Move& a, const Move& bm) {
            return scoreMoveForOrdering(a, ttMove, ply)
                 > scoreMoveForOrdering(bm, ttMove, ply);
        });
}

// ═══════════════════════════════════════════════════════════════
//  Null Move Pruning safety check
//  Unsafe in endgame positions (zugzwang risk) and when in check
// ═══════════════════════════════════════════════════════════════

bool AIEngine::isNullMoveSafe(Color side) {
    // Don't do null move if in check
    if (gen.isInCheck(side)) return false;

    // Count major/minor pieces for the side to move
    // Null move in K+P endgame can cause zugzwang errors
    int pieces = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            const Piece& p = b.grid[r][c];
            if (p.color == side && p.type != PAWN && p.type != KING)
                ++pieces;
        }
    return pieces >= 2;  // at least 2 non-pawn non-king pieces
}

// ═══════════════════════════════════════════════════════════════
//  Quiescence Search — enhanced with delta pruning
//
//  DSA: Recursion + DFS, but only capture moves are generated.
//  Delta pruning: if even the best possible capture can't improve
//  alpha, skip the whole position (fast fail).
// ═══════════════════════════════════════════════════════════════

int AIEngine::quiesce(int alpha, int beta, Color side, bool maximising) {
    ++nodes;

    // Stand-pat: evaluate current position as baseline.
    // Evaluator::evaluate always returns a White-relative score,
    // so alpha/beta comparisons are consistent throughout the tree.
    int eval = Evaluator::evaluate(b);

    if (maximising) {
        if (eval >= beta)  return beta;
        if (eval > alpha)  alpha = eval;
    } else {
        if (eval <= alpha) return alpha;
        if (eval < beta)   beta  = eval;
    }

    // Delta pruning: if even capturing the best piece can't raise alpha, skip
    static constexpr int DELTA = 900;  // roughly a queen

    std::vector<Move> moves = gen.generateLegalMoves(side);
    std::vector<Move> captures;
    captures.reserve(moves.size());
    for (const Move& m : moves)
        if (!m.captured.empty() || m.isEnPassant || m.promotion != NONE_PIECE)
            captures.push_back(m);

    Move noTT;
    orderMoves(captures, noTT, 0);

    Color opp = (side == WHITE) ? BLACK : WHITE;

    for (const Move& m : captures) {
        // Delta pruning: skip captures that can't possibly raise alpha
        if (maximising && !m.captured.empty()) {
            int gain = Evaluator::mvvLva(PAWN, m.captured.type);
            if (eval + gain + DELTA < alpha) continue;
        }

        b.makeMove(m);
        int score = quiesce(alpha, beta, opp, !maximising);
        b.undoMove();

        if (maximising) {
            if (score > alpha) alpha = score;
            if (alpha >= beta) return beta;
        } else {
            if (score < beta)  beta  = score;
            if (beta <= alpha) return alpha;
        }
    }

    return maximising ? alpha : beta;
}

// ═══════════════════════════════════════════════════════════════
//  Principal Variation Search (PVS / NegaScout)
//
//  After searching the first (PV) move with the full [alpha,beta]
//  window, subsequent moves are searched with a zero window
//  [alpha, alpha+1].  If that fails high (score > alpha), we
//  re-search with the full window.  This works because with good
//  move ordering most moves fail low, so the zero-window search
//  is fast and the re-search is rare.
//
//  DSA: Recursion + DFS + Branch-and-Bound + Dynamic Programming (TT)
//
//  Additional techniques:
//    - Transposition Table lookup / store
//    - Null Move Pruning
//    - Killer Move Heuristic
//    - History Heuristic
// ═══════════════════════════════════════════════════════════════

int AIEngine::pvs(int depth, int alpha, int beta,
                  Color side, bool maximising,
                  int ply, bool nullMoveAllowed) {
    ++nodes;

    int origAlpha = alpha;

    // ── Transposition Table probe ────────────────────────────
    // DSA: Hash Table + Dynamic Programming — retrieve cached result
    int   ttScore = 0;
    Move  ttMove;
    if (tt.probe(b.zobristHash, depth, alpha, beta, ttScore, ttMove)) {
        ++ttHits;
        return ttScore;
    }

    // ── Terminal / Leaf ──────────────────────────────────────
    std::vector<Move> legalMoves = gen.generateLegalMoves(side);

    if (legalMoves.empty()) {
        if (gen.isInCheck(side))
            return maximising ? (-Evaluator::INF + ply)
                              :  (Evaluator::INF - ply);
        return 0;  // stalemate
    }

    if (depth == 0) {
        // Extend into quiescence to avoid horizon effect
        return quiesce(alpha, beta, side, maximising);
    }

    // 50-move rule draw
    if (b.halfMoveClock >= 100) return 0;

    // ── Null Move Pruning ────────────────────────────────────
    // DSA: Branch-and-Bound variant — skip our turn; if position
    // is still winning, prune the subtree (opponent can't escape).
    if (nullMoveAllowed && depth >= 3 && isNullMoveSafe(side)) {
        // Make a "null" move: flip side to move without moving a piece
        Color opp = (side == WHITE) ? BLACK : WHITE;
        b.sideToMove = opp;
        b.zobristHash ^= ZOBRIST.sideToMove;

        int nullScore = -pvs(depth - 1 - NULL_MOVE_R, -beta, -beta + 1,
                              opp, !maximising, ply + 1, false);

        // Undo the null move
        b.sideToMove = side;
        b.zobristHash ^= ZOBRIST.sideToMove;

        if (nullScore >= beta) {
            // Null-move cutoff — store in TT and return
            tt.store(b.zobristHash, beta, depth, TT_BETA, Move{});
            return beta;
        }
    }

    // ── Move Ordering ─────────────────────────────────────────
    // DSA: Priority Queue concept — score and sort moves before DFS
    orderMoves(legalMoves, ttMove, ply);

    Color opp = (side == WHITE) ? BLACK : WHITE;
    Move  bestMove;
    int   bestScore = maximising ? -Evaluator::INF : Evaluator::INF;
    bool  firstMove = true;

    for (const Move& m : legalMoves) {
        b.makeMove(m);

        int score;
        if (firstMove) {
            // ── PV move: full-window search ──────────────────
            // DSA: Recursion — descend into child node
            score = pvs(depth - 1, alpha, beta, opp, !maximising,
                        ply + 1, true);
            firstMove = false;
        } else {
            // ── Non-PV move: zero-window search (PVS) ────────
            // Search with narrow window [alpha, alpha+1]
            if (maximising)
                score = pvs(depth - 1, alpha, alpha + 1, opp, !maximising,
                            ply + 1, true);
            else
                score = pvs(depth - 1, beta - 1, beta, opp, !maximising,
                            ply + 1, true);

            // If it raises alpha (failed high), re-search with full window
            bool failHigh = maximising ? (score > alpha && score < beta)
                                       : (score < beta  && score > alpha);
            if (failHigh) {
                score = pvs(depth - 1, alpha, beta, opp, !maximising,
                            ply + 1, true);
            }
        }

        b.undoMove();

        if (maximising) {
            if (score > bestScore) {
                bestScore = score;
                bestMove  = m;
            }
            if (score > alpha) alpha = score;
            if (alpha >= beta) {
                // Beta cutoff — update heuristics for quiet moves
                if (m.captured.empty() && m.promotion == NONE_PIECE) {
                    storeKiller(ply, m);           // Killer Heuristic
                    updateHistory(m, depth, side);  // History Heuristic
                }
                // DSA: Dynamic Programming — store lower bound in TT
                tt.store(b.zobristHash, beta, depth, TT_BETA, m);
                return beta;
            }
        } else {
            if (score < bestScore) {
                bestScore = score;
                bestMove  = m;
            }
            if (score < beta) beta = score;
            if (beta <= alpha) {
                if (m.captured.empty() && m.promotion == NONE_PIECE) {
                    storeKiller(ply, m);
                    updateHistory(m, depth, side);
                }
                tt.store(b.zobristHash, alpha, depth, TT_ALPHA, m);
                return alpha;
            }
        }
    }

    // ── Store result in Transposition Table ───────────────────
    // DSA: Hash Table + Dynamic Programming — memoize the result
    TTFlag flag = (bestScore <= origAlpha) ? TT_ALPHA : TT_EXACT;
    tt.store(b.zobristHash, bestScore, depth, flag, bestMove);

    return bestScore;
}

// ═══════════════════════════════════════════════════════════════
//  findBestMove — top-level call with Iterative Deepening
//
//  DSA: Iterative Deepening DFS (IDDFS)
//
//  We search at depth 1, 2, … up to maxDepth.
//  Benefits:
//    a) Earlier iterations populate the TT, providing good move
//       ordering for deeper iterations → more pruning.
//    b) We always have a valid best move even if time runs out.
//    c) Contrary to intuition, the extra work at shallower depths
//       is negligible because the tree grows exponentially.
// ═══════════════════════════════════════════════════════════════

SearchResult AIEngine::findBestMove(Color side, int maxDepth) {
    auto t0 = std::chrono::steady_clock::now();
    nodes  = 0;
    ttHits = 0;

    // ── Opening Book probe ───────────────────────────────────
    // DSA: Hash Table — O(1) lookup by Zobrist hash
    {
        Move bookMove;
        if (book.probe(b.zobristHash, bookMove)) {
            // Validate book move is actually legal
            auto legal = gen.generateLegalMoves(side);
            for (const Move& lm : legal) {
                if (lm.from == bookMove.from &&
                    lm.to   == bookMove.to   &&
                    lm.promotion == bookMove.promotion) {
                    auto t1 = std::chrono::steady_clock::now();
                    SearchResult r;
                    r.bestMove     = lm;
                    r.score        = 0;
                    r.nodesSearched= 1;
                    r.depth        = 0;
                    r.timeMs       = std::chrono::duration<double, std::milli>(t1-t0).count();
                    r.ttHits       = 0;
                    std::cout << "  [Book move: " << lm.toString() << "]\n";
                    return r;
                }
            }
        }
    }

    clearHeuristics();
    // TT persists across calls (populated from previous move's search)

    SearchResult result;
    result.depth        = maxDepth;
    result.nodesSearched= 0;
    result.score        = (side == WHITE) ? -Evaluator::INF : Evaluator::INF;
    result.ttHits       = 0;

    std::vector<Move> rootMoves = gen.generateLegalMoves(side);
    if (rootMoves.empty()) return result;

    bool maximising = (side == WHITE);
    // ── Iterative Deepening DFS ──────────────────────────────
    // DSA: IDDFS — search depth 1, 2, …, maxDepth in sequence
    for (int d = 1; d <= maxDepth; ++d) {
        Move  iterBest;
        int   iterScore = maximising ? -Evaluator::INF : Evaluator::INF;
        int   ia        = -Evaluator::INF;
        int   ib        =  Evaluator::INF;

        // Re-order root moves using TT from the previous iteration
        Move ttMove;
        tt.probeMove(b.zobristHash, ttMove);
        orderMoves(rootMoves, ttMove, 0);

        Color opp = (side == WHITE) ? BLACK : WHITE;

        for (const Move& m : rootMoves) {
            b.makeMove(m);
            // DSA: Recursion — each root move spawns the search subtree
            int score = pvs(d - 1, ia, ib, opp, !maximising, 1, true);
            b.undoMove();

            if (maximising) {
                if (score > iterScore) {
                    iterScore = score;
                    iterBest  = m;
                }
                if (score > ia) ia = score;
            } else {
                if (score < iterScore) {
                    iterScore = score;
                    iterBest  = m;
                }
                if (score < ib) ib = score;
            }
        }

        // Commit iteration result
        result.bestMove = iterBest;
        result.score    = iterScore;

        std::cout << "  depth=" << d
                  << "  score=" << iterScore
                  << "  nodes=" << nodes
                  << "  best="  << iterBest.toString() << "\n";

        // Early exit on forced mate
        if (std::abs(iterScore) > Evaluator::INF - 1000) break;
    }

    auto t1 = std::chrono::steady_clock::now();
    result.nodesSearched = nodes;
    result.timeMs        = std::chrono::duration<double, std::milli>(t1-t0).count();
    result.ttHits        = ttHits;
    return result;
}

SearchResult AIEngine::findBestMove(Color side, Difficulty diff) {
    return findBestMove(side, static_cast<int>(diff));
}
