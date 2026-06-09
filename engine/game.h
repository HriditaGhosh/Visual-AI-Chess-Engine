#pragma once
#include "movegen.h"
#include "ai_engine.h"

enum GameStatus {
    PLAYING,
    CHECK,
    CHECKMATE,
    STALEMATE,
    DRAW_50_MOVE
};

enum PlayerType { HUMAN, AI_PLAYER };

// ═══════════════════════════════════════════════════════════════
//  Game – orchestrates Human vs AI (or Human vs Human) play
// ═══════════════════════════════════════════════════════════════
class Game {
public:
    Game();
    void newGame();
    void run();   // main interactive loop

    bool applyMoveUCI(const std::string& uci);

    GameStatus status();
    void printBoard() const;
    void printStatus() const;
    std::vector<Move> legalMoves();

private:
    Board         board;
    MoveGenerator gen;
    AIEngine      ai;

    // Player config
    PlayerType players[2];    // [WHITE] [BLACK]
    Difficulty aiDifficulty;
    Color      humanColor;    // which color the human plays

    // ── Setup helpers ────────────────────────────────────────
    void configureGame();
    void selectDifficulty();
    void selectSide();

    // ── Core loop helpers ────────────────────────────────────
    void doAIMove();
    void doHumanMove();

    Move parseUCI(const std::string& uci);
    void printHelp() const;
    void printAIInfo(const SearchResult& r) const;

    static std::string difficultyName(Difficulty d);
    static std::string colorName(Color c);
};
