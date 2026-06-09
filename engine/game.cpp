#include "game.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <thread>
#include <chrono>

// ─────────────────────────────────────────────────────────────
//  Construction & new game
// ─────────────────────────────────────────────────────────────
Game::Game() : gen(board), ai(board) {
    players[WHITE] = HUMAN;
    players[BLACK] = AI_PLAYER;
    aiDifficulty   = MEDIUM;
    humanColor     = WHITE;
}

void Game::newGame() {
    board.reset();
}

// ─────────────────────────────────────────────────────────────
//  Status helpers
// ─────────────────────────────────────────────────────────────
GameStatus Game::status() {
    Color side = board.sideToMove;
    if (gen.isCheckmate(side))      return CHECKMATE;
    if (gen.isStalemate(side))      return STALEMATE;
    if (board.halfMoveClock >= 100) return DRAW_50_MOVE;
    if (gen.isInCheck(side))        return CHECK;
    return PLAYING;
}

void Game::printBoard() const { board.print(); }

void Game::printStatus() const {
    Color side = board.sideToMove;
    GameStatus s = const_cast<Game*>(this)->status();
    switch (s) {
        case CHECKMATE:
            std::cout << "\n*** CHECKMATE! "
                      << colorName((Color)(1 - side)) << " wins! ***\n";
            break;
        case STALEMATE:
            std::cout << "\n*** STALEMATE – Draw! ***\n";
            break;
        case DRAW_50_MOVE:
            std::cout << "\n*** Draw by 50-move rule! ***\n";
            break;
        case CHECK:
            std::cout << colorName(side) << " is in CHECK!\n";
            break;
        case PLAYING:
            break;
    }
}

std::vector<Move> Game::legalMoves() {
    return gen.generateLegalMoves(board.sideToMove);
}

// ─────────────────────────────────────────────────────────────
//  Parse UCI string "e2e4" / "e7e8q"
// ─────────────────────────────────────────────────────────────
Move Game::parseUCI(const std::string& uci) {
    if (uci.size() < 4) throw std::invalid_argument("Bad UCI: " + uci);
    int fc = uci[0]-'a', fr = uci[1]-'1';
    int tc = uci[2]-'a', tr = uci[3]-'1';
    Square from(fr,fc), to(tr,tc);
    if (!from.valid()||!to.valid()) throw std::invalid_argument("Bad squares");
    PieceType promo = NONE_PIECE;
    if (uci.size() >= 5) {
        char pc = (char)tolower(uci[4]);
        if      (pc=='q') promo=QUEEN;
        else if (pc=='r') promo=ROOK;
        else if (pc=='b') promo=BISHOP;
        else if (pc=='n') promo=KNIGHT;
        else throw std::invalid_argument("Bad promo");
    }
    return Move(from, to, promo);
}

bool Game::applyMoveUCI(const std::string& uci) {
    Move requested;
    try { requested = parseUCI(uci); } catch (...) { return false; }
    std::vector<Move> legal = legalMoves();
    for (const Move& m : legal) {
        if (m.from == requested.from && m.to == requested.to) {
            if (m.promotion != NONE_PIECE && m.promotion != requested.promotion)
                continue;
            board.makeMove(m);
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
//  AI move
// ─────────────────────────────────────────────────────────────
void Game::doAIMove() {
    Color side = board.sideToMove;
    std::cout << "\n\033[1;36m[AI " << difficultyName(aiDifficulty)
              << " thinking...]\033[0m\n";

    SearchResult r = ai.findBestMove(side, aiDifficulty);

    if (r.bestMove.from == r.bestMove.to) {
        std::cout << "AI has no moves.\n";
        return;
    }

    board.makeMove(r.bestMove);
    printAIInfo(r);
    std::cout << "\033[1;36mAI plays: " << r.bestMove.toString() << "\033[0m\n";
}

void Game::printAIInfo(const SearchResult& r) const {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Depth: " << r.depth
              << "  |  Nodes: " << r.nodesSearched
              << "  |  TT hits: " << r.ttHits
              << "  |  Time: " << r.timeMs << " ms"
              << "  |  Score: ";
    if      (r.score >  900000) std::cout << "+M" << (Evaluator::INF - r.score) / 100;
    else if (r.score < -900000) std::cout << "-M" << (Evaluator::INF + r.score) / 100;
    else                        std::cout << std::showpos << r.score / 100.0;
    std::cout << std::noshowpos << "\n";
}

// ─────────────────────────────────────────────────────────────
//  Human move (reads from stdin)
// ─────────────────────────────────────────────────────────────
void Game::doHumanMove() {
    std::string line;
    while (true) {
        Color side = board.sideToMove;
        std::cout << colorName(side) << " (" << (players[side]==HUMAN ? "You" : "AI")
                  << ") > ";
        if (!std::getline(std::cin, line)) { exit(0); }

        // Trim
        while (!line.empty() && isspace((unsigned char)line.front())) line.erase(line.begin());
        while (!line.empty() && isspace((unsigned char)line.back()))  line.pop_back();
        if (line.empty()) continue;

        std::string cmd = line;
        for (char& c : cmd) c = (char)tolower(c);

        if (cmd=="quit"||cmd=="exit"||cmd=="q") { exit(0); }
        if (cmd=="help"||cmd=="h")   { printHelp(); continue; }
        if (cmd=="board"||cmd=="b")  { printBoard(); printStatus(); continue; }
        if (cmd=="fen")              { std::cout<<"FEN: "<<board.fen()<<"\n"; continue; }
        if (cmd=="new")              { configureGame(); return; }
        if (cmd=="moves"||cmd=="m")  {
            auto lm = legalMoves();
            std::cout << "Legal moves (" << lm.size() << "): ";
            for (auto& mv : lm) std::cout << mv.toString() << " ";
            std::cout << "\n";
            continue;
        }
        if (cmd=="undo"||cmd=="u")  {
            // Undo twice: AI move + human move
            if (board.history.empty()) { std::cout<<"Nothing to undo.\n"; continue; }
            board.undoMove();
            if (!board.history.empty() && players[board.sideToMove]==AI_PLAYER)
                board.undoMove();
            printBoard(); printStatus();
            continue;
        }

        // Try as move
        if (applyMoveUCI(line)) return;
        std::cout << "Illegal move '" << line << "'. Type 'moves' to see legal moves.\n";
    }
}

// ─────────────────────────────────────────────────────────────
//  Game configuration dialogs
// ─────────────────────────────────────────────────────────────
std::string Game::difficultyName(Difficulty d) {
    switch(d) {
        case EASY:   return "Easy";
        case MEDIUM: return "Medium";
        case HARD:   return "Hard";
        case EXPERT: return "Expert";
        default:     return "?";
    }
}

std::string Game::colorName(Color c) {
    return (c == WHITE) ? "White" : "Black";
}

void Game::selectDifficulty() {
    std::cout << "\nSelect AI difficulty:\n"
              << "  1) Easy   (depth 1 – beginner)\n"
              << "  2) Medium (depth 3 – casual)\n"
              << "  3) Hard   (depth 5 – strong)\n"
              << "  4) Expert (depth 7 – very strong)\n"
              << "Choice [1-4, default 2]: ";
    std::string line;
    std::getline(std::cin, line);
    int c = line.empty() ? 2 : (line[0]-'0');
    switch(c) {
        case 1:  aiDifficulty = EASY;   break;
        case 3:  aiDifficulty = HARD;   break;
        case 4:  aiDifficulty = EXPERT; break;
        default: aiDifficulty = MEDIUM; break;
    }
    std::cout << "Difficulty set to: " << difficultyName(aiDifficulty) << "\n";
}

void Game::selectSide() {
    std::cout << "\nPlay as:\n"
              << "  1) White (you move first)\n"
              << "  2) Black (AI moves first)\n"
              << "  3) Both sides (Human vs Human)\n"
              << "Choice [1-3, default 1]: ";
    std::string line;
    std::getline(std::cin, line);
    int c = line.empty() ? 1 : (line[0]-'0');
    if (c == 2) {
        humanColor     = BLACK;
        players[WHITE] = AI_PLAYER;
        players[BLACK] = HUMAN;
    } else if (c == 3) {
        players[WHITE] = HUMAN;
        players[BLACK] = HUMAN;
        humanColor     = WHITE;
    } else {
        humanColor     = WHITE;
        players[WHITE] = HUMAN;
        players[BLACK] = AI_PLAYER;
    }
}

void Game::configureGame() {
    newGame();

    bool hvh = false;
    // Ask mode first
    std::cout << "\nGame mode:\n"
              << "  1) Human vs AI\n"
              << "  2) Human vs Human\n"
              << "Choice [1-2, default 1]: ";
    std::string line;
    std::getline(std::cin, line);
    int mode = line.empty() ? 1 : (line[0]-'0');
    if (mode == 2) {
        players[WHITE] = HUMAN;
        players[BLACK] = HUMAN;
        hvh = true;
    }

    if (!hvh) {
        selectDifficulty();
        selectSide();
    }

    std::cout << "\n\033[1mGame started!\033[0m\n";
    if (!hvh) {
        std::cout << "You play: " << colorName(humanColor)
                  << " | AI plays: " << colorName((Color)(1-humanColor))
                  << " | Difficulty: " << difficultyName(aiDifficulty) << "\n";
    }
    printBoard();
}

// ─────────────────────────────────────────────────────────────
//  Help
// ─────────────────────────────────────────────────────────────
void Game::printHelp() const {
    std::cout << R"(
┌──────────────────────────────────────────────────────────────┐
│            C++ Chess Engine + AI  –  Commands                │
├──────────────────────────────────────────────────────────────┤
│  <move>   UCI format: e2e4, d7d5, e7e8q (promotion)         │
│  moves    list all legal moves for current side             │
│  undo     take back your last move (+ AI move)              │
│  new      start a new game (re-configure)                   │
│  fen      print current FEN string                          │
│  board    redraw the board                                  │
│  help     show this help                                    │
│  quit     exit                                              │
├──────────────────────────────────────────────────────────────┤
│  AI INFO printed after each AI move:                        │
│    Depth | Nodes searched | Time | Centipawn score          │
│    +M3 = AI sees mate in 3   -M2 = you see mate in 2       │
└──────────────────────────────────────────────────────────────┘
)";
}

// ─────────────────────────────────────────────────────────────
//  Main game loop
// ─────────────────────────────────────────────────────────────
void Game::run() {
    std::cout << "\n\033[1;33m";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║    C++ Chess Engine + AI  v2.0           ║\n";
    std::cout << "║    Minimax · Alpha-Beta · PST Eval       ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "\033[0m";

    configureGame();

    while (true) {
        GameStatus s = status();
        if (s == CHECKMATE || s == STALEMATE || s == DRAW_50_MOVE) {
            printStatus();
            std::cout << "\nType 'new' to play again, 'quit' to exit.\n";
            // Wait for command
            std::string line;
            while (true) {
                std::cout << "> ";
                if (!std::getline(std::cin, line)) return;
                for (char& c : line) c = (char)tolower(c);
                if (line == "new")  { configureGame(); break; }
                if (line == "quit" || line == "q") return;
                if (line == "fen")  std::cout << "FEN: " << board.fen() << "\n";
                if (line == "board") printBoard();
            }
            continue;
        }

        Color side = board.sideToMove;

        if (players[side] == AI_PLAYER) {
            doAIMove();
            printBoard();
            printStatus();
        } else {
            doHumanMove();
            // After human moves, check if we need to show board
            GameStatus s2 = status();
            printBoard();
            printStatus();
            if (s2 == CHECK && players[board.sideToMove] == AI_PLAYER)
                std::cout << "\033[1;31mYou put the AI in check!\033[0m\n";
        }
    }
}
