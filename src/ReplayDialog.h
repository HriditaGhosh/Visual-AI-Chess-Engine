#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QSlider>
#include <QTimer>
#include <queue>
#include <vector>
#include <string>

#include "StatsManager.h"
#include "ReplayBoardView.h"

// ─────────────────────────────────────────────────────────────────────────────
//  ReplayDialog
//
//  DSA: Queue (move replay system)
//    – m_replayQueue: std::queue<std::string> — moves loaded in FIFO order
//      Each "Next" click pops one move from the front and applies it.
//      Auto-play timer also pops one move per tick.
//
//  DSA: Vector (game history)
//    – Receives const std::vector<GameRecord>& from StatsManager
//      Games list rendered in QListWidget, O(1) index access on selection
//
//  DSA: Tree traversal (replay navigation concept)
//    – The move history forms a linear tree (linked list degenerate case).
//      m_replayIndex is a "cursor" that traverses the tree:
//        Forward  → advance cursor, apply move from m_allMoves[cursor]
//        Backward → retreat cursor, reconstruct board by replaying 0..cursor-1
//      This mirrors pre-order DFS traversal of a game tree node-by-node.
// ─────────────────────────────────────────────────────────────────────────────
class ReplayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplayDialog(const std::vector<GameRecord>& history,
                          QWidget* parent = nullptr);

private slots:
    void onGameSelected(int row);
    void onFirst();
    void onPrev();
    void onNext();
    void onLast();
    void onAutoPlay();
    void onAutoTick();
    void onSliderMoved(int value);

private:
    void loadGame(const GameRecord& rec);
    void applyMoveAt(int index);     // apply moves 0..index on scratch board
    void rebuildBoardTo(int index);  // replay from start to index
    void updateUI();
    void refreshMoveList();
    QString moveLabel(int idx, const std::string& uci) const;
    QString formatHeader(const GameRecord& rec) const;

    // ── DSA: Queue — filled from GameRecord::moves, consumed by Next/AutoPlay
    std::queue<std::string>      m_replayQueue;

    // ── DSA: Vector — full move list of currently replayed game
    std::vector<std::string>     m_allMoves;

    // ── DSA: Tree traversal cursor
    int  m_replayIndex = -1;   // current position in game tree (-1 = start)
    int  m_totalMoves  = 0;

    // ── Scratch board for replay (no AI, no signals)
    Board*          m_scratchBoard = nullptr;
    GameRecord      m_currentGame;

    // ── Auto-play
    QTimer*         m_autoTimer;
    bool            m_autoPlaying = false;

    // ── UI
    const std::vector<GameRecord>& m_history;

    QListWidget*    m_gameList;
    QListWidget*    m_moveList;
    ReplayBoardView* m_boardView;

    QLabel*         m_headerLabel;
    QLabel*         m_moveLabel;
    QLabel*         m_positionLabel;

    QPushButton*    m_btnFirst;
    QPushButton*    m_btnPrev;
    QPushButton*    m_btnNext;
    QPushButton*    m_btnLast;
    QPushButton*    m_btnAuto;
    QSlider*        m_slider;
};
