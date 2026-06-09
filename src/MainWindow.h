#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QSplitter>

#include "ChessController.h"
#include "BoardWidget.h"
#include "MoveHistoryWidget.h"
#include "SoundManager.h"
#include "ThemeManager.h"
#include "PersistenceManager.h"
#include "StatsManager.h"
#include "StatsDialog.h"
#include "ReplayDialog.h"

// ─────────────────────────────────────────────────────────────────────────────
//  MainWindow
//
//  Top-level window: assembles BoardWidget + sidebar + toolbar + status bar.
//  All inter-widget communication goes through Signal & Slot connections made
//  here in the constructor — widgets never talk to each other directly.
// ─────────────────────────────────────────────────────────────────────────────

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onNewGame();
    void onUndo();
    void onFlipBoard();
    void onToggleTheme();
    void onToggleFullscreen();
    void onToggleSound();
    void onGameStatusChanged(GameStatus s);
    void onAIThinkingStarted();
    void onAIThinkingFinished(SearchResult r);
    void onMoveApplied(Move m, bool cap, bool chk, bool mate,
                       bool castle, bool promo);

    // ── Persistence slots ─────────────────────────────────────────
    void onSaveGame();
    void onLoadGame();
    void onExportPGN();
    void onImportPGN();
    void onExportFEN();
    void onImportFEN();
    void onAutosaved(const QString& path);

    // ── Stats / Replay slots ──────────────────────────────────────
    void onShowStats();
    void onShowReplay();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void connectSignals();
    void updateStatusLabel(GameStatus s);
    QString difficultyLabel() const;

    // ── Core components ───────────────────────────────────────────
    ChessController*    m_ctrl;
    BoardWidget*        m_board;
    MoveHistoryWidget*  m_history;
    SoundManager*       m_sound;
    PersistenceManager* m_persist;
    StatsManager*       m_stats;

    // ── UI elements ───────────────────────────────────────────────
    QLabel*   m_statusLabel;
    QLabel*   m_turnLabel;
    QLabel*   m_engineLabel;
    QLabel*   m_statsLabel;    // shows W/L/D in toolbar
    QAction*  m_undoAction;
    QAction*  m_themeAction;
    QAction*  m_soundAction;
    QAction*  m_flipAction;
    QAction*  m_fullscreenAction;

    bool m_soundOn = true;
};
