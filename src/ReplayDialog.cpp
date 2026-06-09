#include "ReplayDialog.h"
#include "movegen.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFont>
#include <QApplication>

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────

ReplayDialog::ReplayDialog(const std::vector<GameRecord>& history, QWidget* parent)
    : QDialog(parent), m_history(history)
{
    setWindowTitle("Game Replay");
    setMinimumSize(1000, 680);
    resize(1100, 720);

    m_scratchBoard = new Board();
    m_autoTimer    = new QTimer(this);
    m_autoTimer->setInterval(800);
    connect(m_autoTimer, &QTimer::timeout, this, &ReplayDialog::onAutoTick);

    // ── Left panel: game list ─────────────────────────────────────
    m_gameList = new QListWidget;
    m_gameList->setMaximumWidth(240);
    m_gameList->setAlternatingRowColors(true);
    for (const auto& rec : m_history) {
        QString label = QString("%1\n%2 (%3) — %4 moves")
            .arg(rec.dateTime.left(10))
            .arg(rec.result)
            .arg(rec.resultReason)
            .arg(rec.totalMoves);
        auto* item = new QListWidgetItem(label);
        m_gameList->addItem(item);
    }

    auto* gameGroup = new QGroupBox("Game History");
    auto* gameVBox  = new QVBoxLayout(gameGroup);
    gameVBox->addWidget(m_gameList);

    // ── Centre: board ─────────────────────────────────────────────
    m_boardView = new ReplayBoardView(this);

    // ── Right panel: move list ────────────────────────────────────
    m_moveList = new QListWidget;
    m_moveList->setMaximumWidth(160);

    auto* moveGroup = new QGroupBox("Moves");
    auto* moveVBox  = new QVBoxLayout(moveGroup);
    moveVBox->addWidget(m_moveList);

    // ── Controls ──────────────────────────────────────────────────
    m_headerLabel   = new QLabel("Select a game from the list");
    m_moveLabel     = new QLabel("");
    m_positionLabel = new QLabel("— / —");
    m_headerLabel->setAlignment(Qt::AlignCenter);
    m_positionLabel->setAlignment(Qt::AlignCenter);

    QFont boldFont = m_headerLabel->font();
    boldFont.setBold(true);
    boldFont.setPointSize(boldFont.pointSize() + 1);
    m_headerLabel->setFont(boldFont);

    m_btnFirst = new QPushButton("⏮");
    m_btnPrev  = new QPushButton("◀");
    m_btnNext  = new QPushButton("▶");
    m_btnLast  = new QPushButton("⏭");
    m_btnAuto  = new QPushButton("▶▶ Auto");

    for (auto* btn : {m_btnFirst, m_btnPrev, m_btnNext, m_btnLast, m_btnAuto}) {
        btn->setMinimumWidth(60);
        btn->setEnabled(false);
    }

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setEnabled(false);

    auto* ctrlRow = new QHBoxLayout;
    ctrlRow->addWidget(m_btnFirst);
    ctrlRow->addWidget(m_btnPrev);
    ctrlRow->addWidget(m_btnNext);
    ctrlRow->addWidget(m_btnLast);
    ctrlRow->addStretch();
    ctrlRow->addWidget(m_positionLabel);
    ctrlRow->addStretch();
    ctrlRow->addWidget(m_btnAuto);

    auto* boardBox = new QVBoxLayout;
    boardBox->addWidget(m_headerLabel);
    boardBox->addWidget(m_boardView, 1);
    boardBox->addWidget(m_moveLabel);
    boardBox->addWidget(m_slider);
    boardBox->addLayout(ctrlRow);

    // ── Assemble ──────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    auto* leftW    = new QWidget; leftW->setLayout(new QVBoxLayout);
    leftW->layout()->addWidget(gameGroup);
    auto* rightW   = new QWidget; rightW->setLayout(new QVBoxLayout);
    rightW->layout()->addWidget(moveGroup);
    auto* centreW  = new QWidget; centreW->setLayout(boardBox);

    splitter->addWidget(leftW);
    splitter->addWidget(centreW);
    splitter->addWidget(rightW);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 5);
    splitter->setStretchFactor(2, 2);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);

    // ── Connections ───────────────────────────────────────────────
    connect(m_gameList, &QListWidget::currentRowChanged,
            this, &ReplayDialog::onGameSelected);
    connect(m_btnFirst, &QPushButton::clicked, this, &ReplayDialog::onFirst);
    connect(m_btnPrev,  &QPushButton::clicked, this, &ReplayDialog::onPrev);
    connect(m_btnNext,  &QPushButton::clicked, this, &ReplayDialog::onNext);
    connect(m_btnLast,  &QPushButton::clicked, this, &ReplayDialog::onLast);
    connect(m_btnAuto,  &QPushButton::clicked, this, &ReplayDialog::onAutoPlay);
    connect(m_slider,   &QSlider::valueChanged, this, &ReplayDialog::onSliderMoved);
    connect(m_moveList, &QListWidget::currentRowChanged,
            this, [this](int row) {
                if (row >= 0 && row != m_replayIndex) {
                    // DSA: Tree traversal — jump to arbitrary node
                    rebuildBoardTo(row);
                    m_replayIndex = row;
                    updateUI();
                }
            });

    if (!m_history.empty())
        m_gameList->setCurrentRow(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game selection
// ─────────────────────────────────────────────────────────────────────────────

void ReplayDialog::onGameSelected(int row)
{
    if (row < 0 || row >= (int)m_history.size()) return;
    m_autoTimer->stop();
    m_autoPlaying = false;
    m_btnAuto->setText("▶▶ Auto");
    loadGame(m_history[row]);
}

void ReplayDialog::loadGame(const GameRecord& rec)
{
    m_currentGame = rec;
    m_allMoves    = rec.moves;         // DSA: Vector — copy
    m_totalMoves  = (int)m_allMoves.size();
    m_replayIndex = -1;

    // ── DSA: Queue — fill replay queue with all moves (FIFO)
    while (!m_replayQueue.empty()) m_replayQueue.pop();
    for (const auto& mv : m_allMoves)
        m_replayQueue.push(mv);

    // Reset scratch board to start position
    m_scratchBoard->reset();
    m_boardView->setBoard(m_scratchBoard);

    // Setup slider
    m_slider->setRange(-1, m_totalMoves - 1);
    m_slider->setValue(-1);
    m_slider->setEnabled(m_totalMoves > 0);

    // Header label
    m_headerLabel->setText(formatHeader(rec));

    refreshMoveList();
    updateUI();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Navigation — DSA: Tree traversal (cursor advances/retreats)
// ─────────────────────────────────────────────────────────────────────────────

void ReplayDialog::onFirst()
{
    // Tree traversal: jump to root
    m_scratchBoard->reset();
    m_boardView->setBoard(m_scratchBoard);
    m_replayIndex = -1;

    // Refill queue from start
    while (!m_replayQueue.empty()) m_replayQueue.pop();
    for (const auto& mv : m_allMoves)
        m_replayQueue.push(mv);

    updateUI();
}

void ReplayDialog::onPrev()
{
    if (m_replayIndex < 0) return;
    // Tree traversal: step back — replay from root to index-1
    --m_replayIndex;
    rebuildBoardTo(m_replayIndex);
    updateUI();
}

void ReplayDialog::onNext()
{
    if (m_replayIndex >= m_totalMoves - 1) return;

    // DSA: Queue — pop one move from front and apply it
    if (!m_replayQueue.empty()) {
        std::string uci = m_replayQueue.front();
        m_replayQueue.pop();
        ++m_replayIndex;
        applyMoveAt(m_replayIndex);
        updateUI();
    }
}

void ReplayDialog::onLast()
{
    // Tree traversal: jump to leaf node
    rebuildBoardTo(m_totalMoves - 1);
    m_replayIndex = m_totalMoves - 1;
    while (!m_replayQueue.empty()) m_replayQueue.pop();
    updateUI();
}

void ReplayDialog::onAutoPlay()
{
    if (m_allMoves.empty()) return;
    m_autoPlaying = !m_autoPlaying;
    if (m_autoPlaying) {
        m_btnAuto->setText("⏸ Pause");
        m_autoTimer->start();
    } else {
        m_btnAuto->setText("▶▶ Auto");
        m_autoTimer->stop();
    }
}

void ReplayDialog::onAutoTick()
{
    // DSA: Queue — auto-play pops one move per timer tick
    if (m_replayIndex >= m_totalMoves - 1) {
        m_autoPlaying = false;
        m_btnAuto->setText("▶▶ Auto");
        m_autoTimer->stop();
        return;
    }
    onNext();
}

void ReplayDialog::onSliderMoved(int value)
{
    if (value == m_replayIndex) return;
    // DSA: Tree traversal — random access jump
    rebuildBoardTo(value);
    m_replayIndex = value;

    // Rebuild queue from current position forward
    while (!m_replayQueue.empty()) m_replayQueue.pop();
    for (int i = m_replayIndex + 1; i < m_totalMoves; ++i)
        m_replayQueue.push(m_allMoves[i]);

    updateUI();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Board helpers
// ─────────────────────────────────────────────────────────────────────────────

void ReplayDialog::applyMoveAt(int index)
{
    if (index < 0 || index >= m_totalMoves) return;
    const std::string& uci = m_allMoves[index];
    if (uci.size() < 4) return;

    int fc = uci[0]-'a', fr = uci[1]-'1';
    int tc = uci[2]-'a', tr = uci[3]-'1';
    PieceType promo = NONE_PIECE;
    if (uci.size() == 5) {
        char p = uci[4];
        if (p=='q') promo=QUEEN; else if(p=='r') promo=ROOK;
        else if(p=='b') promo=BISHOP; else if(p=='n') promo=KNIGHT;
    }

    MoveGenerator gen(*m_scratchBoard);
    auto legal = gen.generateLegalMoves(m_scratchBoard->sideToMove);
    for (const Move& mv : legal) {
        if (mv.from == Square(fr,fc) && mv.to == Square(tr,tc) &&
            (promo == NONE_PIECE || mv.promotion == promo)) {
            m_scratchBoard->makeMove(mv);
            break;
        }
    }
    m_boardView->setBoard(m_scratchBoard);
}

void ReplayDialog::rebuildBoardTo(int index)
{
    // DSA: Tree traversal — replay from root node to target node
    m_scratchBoard->reset();
    for (int i = 0; i <= index && i < m_totalMoves; ++i)
        applyMoveAt(i);
    m_boardView->setBoard(m_scratchBoard);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI helpers
// ─────────────────────────────────────────────────────────────────────────────

void ReplayDialog::updateUI()
{
    bool hasGame  = m_totalMoves > 0;
    bool hasPrev  = hasGame && m_replayIndex >= 0;
    bool hasNext  = hasGame && m_replayIndex < m_totalMoves - 1;

    m_btnFirst->setEnabled(hasPrev);
    m_btnPrev ->setEnabled(hasPrev);
    m_btnNext ->setEnabled(hasNext);
    m_btnLast ->setEnabled(hasNext);
    m_btnAuto ->setEnabled(hasGame);

    // Sync slider (block signal to avoid loop)
    m_slider->blockSignals(true);
    m_slider->setValue(m_replayIndex);
    m_slider->blockSignals(false);

    // Position label
    int displayed = m_replayIndex + 1;
    m_positionLabel->setText(QString("Move %1 / %2")
        .arg(displayed > 0 ? displayed : 0)
        .arg(m_totalMoves));

    // Move label
    if (m_replayIndex >= 0 && m_replayIndex < m_totalMoves) {
        m_moveLabel->setText("Last: " + moveLabel(m_replayIndex,
                                                    m_allMoves[m_replayIndex]));
    } else {
        m_moveLabel->setText("Start position");
    }

    // Highlight current move in list
    m_moveList->blockSignals(true);
    m_moveList->setCurrentRow(m_replayIndex);
    m_moveList->blockSignals(false);
    if (m_replayIndex >= 0)
        m_moveList->scrollToItem(m_moveList->item(m_replayIndex));
}

void ReplayDialog::refreshMoveList()
{
    m_moveList->clear();
    // DSA: Vector — iterate to populate move list widget
    for (int i = 0; i < (int)m_allMoves.size(); ++i) {
        m_moveList->addItem(moveLabel(i, m_allMoves[i]));
    }
}

QString ReplayDialog::moveLabel(int idx, const std::string& uci) const
{
    int moveNum  = idx / 2 + 1;
    bool isWhite = (idx % 2 == 0);
    return QString("%1%2. %3")
        .arg(moveNum)
        .arg(isWhite ? "." : "..")
        .arg(QString::fromStdString(uci));
}

QString ReplayDialog::formatHeader(const GameRecord& rec) const
{
    QString winner;
    if (rec.result == "Draw")
        winner = "Draw by " + rec.resultReason;
    else
        winner = rec.result + " wins by " + rec.resultReason;

    return QString("%1  |  %2  |  %3 moves  |  %4")
        .arg(rec.dateTime.left(16).replace('T',' '))
        .arg(rec.mode)
        .arg(rec.totalMoves)
        .arg(winner);
}
