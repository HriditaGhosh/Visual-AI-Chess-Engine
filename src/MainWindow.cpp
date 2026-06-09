#include "MainWindow.h"
#include "GameSettingsDialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QKeyEvent>
#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGroupBox>
#include <QStyle>
#include <QIcon>
#include <QString>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QClipboard>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("♟ Chess — Qt GUI");
    setMinimumSize(640, 560);
    resize(920, 680);

    // ── Instantiate core components ──────────────────────────────
    m_ctrl    = new ChessController(this);
    m_sound   = new SoundManager(this);
    m_persist = new PersistenceManager(m_ctrl, this);
    m_stats   = new StatsManager(this);

    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();

    // Apply initial theme
    qApp->setPalette(ThemeManager::instance()->appPalette());
    qApp->setStyleSheet(ThemeManager::instance()->themeStyleSheet());

    m_sound->play(SoundEvent::GameStart);
    updateStatusLabel(PLAYING);
}

// ── UI construction ─────────────────────────────────────────────────────────

void MainWindow::setupUI()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* hbox = new QHBoxLayout(central);
    hbox->setContentsMargins(8, 8, 8, 8);
    hbox->setSpacing(8);

    // Board
    m_board = new BoardWidget(m_ctrl, this);
    m_board->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox->addWidget(m_board, 1);

    // Right sidebar
    auto* sidebar = new QFrame(this);
    sidebar->setFrameShape(QFrame::StyledPanel);
    sidebar->setFixedWidth(220);
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(6, 6, 6, 6);
    sideLayout->setSpacing(6);

    // Turn indicator
    m_turnLabel = new QLabel("White to move", sidebar);
    m_turnLabel->setAlignment(Qt::AlignCenter);
    QFont tf = m_turnLabel->font();
    tf.setPointSize(13);
    tf.setBold(true);
    m_turnLabel->setFont(tf);
    sideLayout->addWidget(m_turnLabel);

    // Engine info
    m_engineLabel = new QLabel("", sidebar);
    m_engineLabel->setAlignment(Qt::AlignCenter);
    m_engineLabel->setWordWrap(true);
    QFont ef = m_engineLabel->font();
    ef.setPointSize(9);
    m_engineLabel->setFont(ef);
    sideLayout->addWidget(m_engineLabel);

    // Separator
    auto* sep = new QFrame(sidebar);
    sep->setFrameShape(QFrame::HLine);
    sideLayout->addWidget(sep);

    // Move history
    m_history = new MoveHistoryWidget(sidebar);
    sideLayout->addWidget(m_history, 1);

    hbox->addWidget(sidebar);
}

void MainWindow::setupMenuBar()
{
    // ── File menu (persistence) ───────────────────────────────────
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Save Game",    this, &MainWindow::onSaveGame,   QKeySequence("Ctrl+S"));
    fileMenu->addAction("&Load Game",    this, &MainWindow::onLoadGame,   QKeySequence("Ctrl+O"));
    fileMenu->addSeparator();
    fileMenu->addAction("Export &PGN",   this, &MainWindow::onExportPGN);
    fileMenu->addAction("Import P&GN",   this, &MainWindow::onImportPGN);
    fileMenu->addSeparator();
    fileMenu->addAction("Export FE&N",   this, &MainWindow::onExportFEN);
    fileMenu->addAction("Import F&EN",   this, &MainWindow::onImportFEN);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit",         qApp, &QApplication::quit, QKeySequence("Alt+F4"));

    auto* gameMenu = menuBar()->addMenu("&Game");
    gameMenu->addAction("&New Game",    this, &MainWindow::onNewGame,          QKeySequence("Ctrl+N"));
    gameMenu->addAction("&Undo Move",   this, &MainWindow::onUndo,             QKeySequence("Ctrl+Z"));
    gameMenu->addSeparator();
    gameMenu->addAction("&Flip Board",  this, &MainWindow::onFlipBoard,        QKeySequence("F"));
    gameMenu->addAction("Full&screen",  this, &MainWindow::onToggleFullscreen, QKeySequence("F11"));

    auto* statsMenu = menuBar()->addMenu("&Stats");
    statsMenu->addAction("📊 Statistics",    this, &MainWindow::onShowStats);
    statsMenu->addAction("▶  Replay Games",  this, &MainWindow::onShowReplay);

    auto* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Toggle &Theme", this, &MainWindow::onToggleTheme, QKeySequence("T"));
    viewMenu->addAction("Toggle &Sound", this, &MainWindow::onToggleSound, QKeySequence("S"));
}

void MainWindow::setupToolBar()
{
    auto* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));

    auto* newAct = toolbar->addAction("⊞ New Game");
    newAct->setToolTip("New Game (Ctrl+N)");
    connect(newAct, &QAction::triggered, this, &MainWindow::onNewGame);

    m_undoAction = toolbar->addAction("↩ Undo");
    m_undoAction->setToolTip("Undo last move (Ctrl+Z)");
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndo);

    toolbar->addSeparator();

    m_flipAction = toolbar->addAction("⇅ Flip");
    m_flipAction->setToolTip("Flip board (F)");
    connect(m_flipAction, &QAction::triggered, this, &MainWindow::onFlipBoard);

    toolbar->addSeparator();

    m_themeAction = toolbar->addAction("☀ Light");
    m_themeAction->setToolTip("Toggle theme (T)");
    m_themeAction->setCheckable(true);
    connect(m_themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    m_soundAction = toolbar->addAction("♪ Sound");
    m_soundAction->setToolTip("Toggle sound (S)");
    m_soundAction->setCheckable(true);
    m_soundAction->setChecked(true);
    connect(m_soundAction, &QAction::triggered, this, &MainWindow::onToggleSound);

    m_fullscreenAction = toolbar->addAction("⛶ Full");
    m_fullscreenAction->setToolTip("Toggle fullscreen (F11)");
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::onToggleFullscreen);

    // Toolbar font
    QFont tbf = toolbar->font();
    tbf.setPointSize(11);
    toolbar->setFont(tbf);

    // Stats live counter
    m_statsLabel = new QLabel("  W:0 L:0 D:0  ");
    m_statsLabel->setToolTip("Wins / Losses / Draws vs AI");
    toolbar->addSeparator();
    toolbar->addWidget(m_statsLabel);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addPermanentWidget(m_statusLabel);
}

// ── Signal & Slot wiring ────────────────────────────────────────────────────

void MainWindow::connectSignals()
{
    // BoardWidget → ChessController
    connect(m_board, &BoardWidget::moveRequested,
            m_ctrl,  &ChessController::tryMove);

    // ChessController → BoardWidget
    connect(m_ctrl,  &ChessController::boardChanged,
            m_board, &BoardWidget::onBoardChanged);
    connect(m_ctrl,  &ChessController::moveAnimationRequested,
            m_board, &BoardWidget::onMoveAnimationRequested);

    // ChessController → MainWindow
    connect(m_ctrl, &ChessController::gameStatusChanged,
            this,   &MainWindow::onGameStatusChanged);
    connect(m_ctrl, &ChessController::aiThinkingStarted,
            this,   &MainWindow::onAIThinkingStarted);
    connect(m_ctrl, &ChessController::aiThinkingFinished,
            this,   &MainWindow::onAIThinkingFinished);
    connect(m_ctrl, &ChessController::moveApplied,
            this,   &MainWindow::onMoveApplied);

    // ChessController → MoveHistoryWidget
    connect(m_ctrl,    &ChessController::moveApplied,
            m_history, &MoveHistoryWidget::onMoveApplied);
    connect(m_ctrl,    &ChessController::undoPerformed,
            m_history, &MoveHistoryWidget::onUndoPerformed);

    // ChessController → SoundManager
    connect(m_ctrl,  &ChessController::moveApplied,
            m_sound, &SoundManager::onMoveApplied);
    connect(m_ctrl,  &ChessController::undoPerformed,
            this,    [this]{ m_sound->play(SoundEvent::Undo); });

    // ThemeManager → MainWindow + Board
    // Stats counter update
    connect(m_stats, &StatsManager::statsUpdated, this, [this]{
        m_statsLabel->setText(QString("  W:%1 L:%2 D:%3  ")
            .arg(m_stats->humanWins())
            .arg(m_stats->humanLosses())
            .arg(m_stats->draws()));
    });

    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            m_board, &BoardWidget::onThemeChanged);

    // Persistence
    connect(m_persist, &PersistenceManager::autosaved,
            this,      &MainWindow::onAutosaved);
    connect(m_ctrl, &ChessController::boardChanged,
            this,   [this]{ setWindowTitle("♟ Chess — Qt GUI *"); });
    connect(m_persist, &PersistenceManager::gameLoaded,
            this,   [this](const SavedGame&){
                setWindowTitle("♟ Chess — Qt GUI");
                m_history->onNewGame();
            });
}

// ── Slots ────────────────────────────────────────────────────────────────────

void MainWindow::onNewGame()
{
    GameSettingsDialog dlg(m_ctrl->config(), this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_history->onNewGame();
    m_ctrl->newGame(dlg.result());
    m_sound->play(SoundEvent::GameStart);
    m_engineLabel->clear();
    m_turnLabel->setText("White to move");
}

void MainWindow::onUndo()
{
    m_ctrl->undo();
    // Turn label updated via gameStatusChanged
}

void MainWindow::onFlipBoard()
{
    bool f = !m_board->isFlipped();
    m_board->setFlipped(f);
}

void MainWindow::onToggleTheme()
{
    ThemeManager::instance()->toggleTheme();
    bool dark = ThemeManager::instance()->currentTheme() == ThemeManager::DARK;
    m_themeAction->setText(dark ? "☀ Light" : "☾ Dark");
}

void MainWindow::onToggleFullscreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

void MainWindow::onToggleSound()
{
    m_soundOn = !m_soundOn;
    m_sound->setMuted(!m_soundOn);
    m_soundAction->setChecked(m_soundOn);
    m_soundAction->setText(m_soundOn ? "♪ Sound" : "♪ Mute");
}

void MainWindow::onGameStatusChanged(GameStatus s)
{
    updateStatusLabel(s);

    Color side = m_ctrl->board().sideToMove;
    QString sideStr = (side == WHITE) ? "White" : "Black";

    switch (s) {
    case PLAYING: case CHECK:
        m_turnLabel->setText(sideStr + (s == CHECK ? " — CHECK!" : " to move"));
        break;
    case CHECKMATE:
        m_turnLabel->setText(
            (side == WHITE ? "Black" : "White") + QString(" wins! ♛"));
        break;
    case STALEMATE:
        m_turnLabel->setText("Stalemate — Draw");
        break;
    case DRAW_50_MOVE:
        m_turnLabel->setText("50-move rule — Draw");
        break;
    }

    // ── Record completed game ─────────────────────────────────────
    if (s == CHECKMATE || s == STALEMATE || s == DRAW_50_MOVE) {
        GameRecord rec;
        rec.dateTime   = QDateTime::currentDateTime().toString(Qt::ISODate);
        rec.totalMoves = (int)m_persist->moveLog().size();
        rec.moves      = m_persist->moveLog();  // DSA: Vector copy

        // Determine mode
        switch (m_ctrl->config().mode) {
            case PlayerMode::HumanVsHuman: rec.mode = "HvH";  break;
            case PlayerMode::HumanVsAI:   rec.mode = "HvAI"; break;
            case PlayerMode::AIVsHuman:   rec.mode = "AIvH"; break;
        }
        // Difficulty
        switch (m_ctrl->config().difficulty) {
            case EASY:   rec.difficulty = "Easy";   break;
            case MEDIUM: rec.difficulty = "Medium"; break;
            case HARD:   rec.difficulty = "Hard";   break;
            case EXPERT: rec.difficulty = "Expert"; break;
            default:     rec.difficulty = "Medium"; break;
        }
        // Result
        if (s == CHECKMATE) {
            rec.result       = (side == WHITE) ? "Black" : "White";
            rec.resultReason = "Checkmate";
        } else if (s == STALEMATE) {
            rec.result       = "Draw";
            rec.resultReason = "Stalemate";
        } else {
            rec.result       = "Draw";
            rec.resultReason = "50-move";
        }

        m_stats->recordGame(rec);
    }

    m_undoAction->setEnabled(m_ctrl->canUndo());
}

void MainWindow::onAIThinkingStarted()
{
    m_statusLabel->setText("AI thinking…");
    m_undoAction->setEnabled(false);
}

void MainWindow::onAIThinkingFinished(SearchResult r)
{
    m_engineLabel->setText(
        QString("Depth %1 | Score %2 | %3 nodes")
            .arg(r.depth)
            .arg(r.score)
            .arg(r.nodesSearched));
    m_undoAction->setEnabled(m_ctrl->canUndo());
}

void MainWindow::onMoveApplied(Move, bool, bool, bool, bool, bool)
{
    // Turn label is handled by gameStatusChanged
}

void MainWindow::updateStatusLabel(GameStatus s)
{
    const char* labels[] = {
        "Playing", "Check!", "Checkmate", "Stalemate", "Draw (50-move)"
    };
    m_statusLabel->setText(labels[int(s)]);
}

// ── Keyboard shortcuts ───────────────────────────────────────────────────────

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_N:
        if (e->modifiers() & Qt::ControlModifier) onNewGame(); break;
    case Qt::Key_Z:
        if (e->modifiers() & Qt::ControlModifier) onUndo();    break;
    case Qt::Key_F11: onToggleFullscreen(); break;
    case Qt::Key_T:   onToggleTheme();      break;
    case Qt::Key_S:   onToggleSound();      break;
    case Qt::Key_F:   onFlipBoard();        break;
    default: QMainWindow::keyPressEvent(e); break;
    }
}

// ── Persistence slots ─────────────────────────────────────────────────────────

void MainWindow::onSaveGame()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Game", QDir::homePath(),
        "Chess Save Files (*.chsave);;All Files (*)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".chsave")) path += ".chsave";

    bool ok;
    QString name = QInputDialog::getText(this, "Save Name",
        "Save name:", QLineEdit::Normal, "My Game", &ok);
    if (!ok) return;

    if (m_persist->saveGame(path, name))
        setWindowTitle("♟ Chess — Qt GUI");
    else
        QMessageBox::warning(this, "Save Failed", "Could not save the game.");
}

void MainWindow::onLoadGame()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Load Game", QDir::homePath(),
        "Chess Save Files (*.chsave);;All Files (*)");
    if (path.isEmpty()) return;

    if (!m_persist->loadGame(path))
        QMessageBox::warning(this, "Load Failed", "Could not load the save file.");
}

void MainWindow::onExportPGN()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Export PGN", QDir::homePath(),
        "PGN Files (*.pgn);;All Files (*)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".pgn")) path += ".pgn";

    if (!m_persist->exportPGN(path))
        QMessageBox::warning(this, "Export Failed", "Could not export PGN.");
    else
        QMessageBox::information(this, "PGN Exported", "Game exported to:\n" + path);
}

void MainWindow::onImportPGN()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Import PGN", QDir::homePath(),
        "PGN Files (*.pgn);;All Files (*)");
    if (path.isEmpty()) return;

    if (!m_persist->importPGN(path))
        QMessageBox::warning(this, "Import Failed", "Could not import PGN.\nOnly UCI-format move notation is supported.");
}

void MainWindow::onExportFEN()
{
    QString fen = m_persist->exportFEN();
    QApplication::clipboard()->setText(fen);
    QMessageBox::information(this, "FEN Copied",
        "Current position FEN copied to clipboard:\n\n" + fen);
}

void MainWindow::onImportFEN()
{
    bool ok;
    QString fen = QInputDialog::getText(this, "Import FEN",
        "Paste FEN string:", QLineEdit::Normal,
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &ok);
    if (!ok || fen.isEmpty()) return;

    if (!m_persist->importFEN(fen))
        QMessageBox::warning(this, "Invalid FEN", "The FEN string is invalid.");
    else {
        m_history->onNewGame();
        updateStatusLabel(m_ctrl->status());
    }
}

void MainWindow::onAutosaved(const QString& path)
{
    statusBar()->showMessage("Autosaved: " + path, 3000);
}

void MainWindow::onShowStats()
{
    StatsDialog dlg(m_stats, this);
    dlg.exec();
}

void MainWindow::onShowReplay()
{
    if (m_stats->historySize() == 0) {
        QMessageBox::information(this, "No Games",
            "No completed games to replay yet.\nFinish a game first!");
        return;
    }
    ReplayDialog dlg(m_stats->history(), this);
    dlg.exec();
}
