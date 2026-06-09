#include "StatsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFont>
#include <QMessageBox>
#include <QFrame>

StatsDialog::StatsDialog(StatsManager* stats, QWidget* parent)
    : QDialog(parent), m_stats(stats)
{
    setWindowTitle("Statistics");
    setMinimumWidth(420);
    setModal(true);
    buildUI();
    connect(m_stats, &StatsManager::statsUpdated, this, &StatsDialog::refresh);
}

void StatsDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // ── Title ─────────────────────────────────────────────────────
    auto* titleLabel = new QLabel("♟  Chess Statistics");
    QFont f = titleLabel->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 4);
    titleLabel->setFont(f);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // ── Games Played ──────────────────────────────────────────────
    m_gamesLabel = new QLabel;
    m_gamesLabel->setAlignment(Qt::AlignCenter);
    QFont gf = m_gamesLabel->font();
    gf.setPointSize(gf.pointSize() + 2);
    m_gamesLabel->setFont(gf);
    mainLayout->addWidget(m_gamesLabel);

    // ── Win/Loss/Draw bars ────────────────────────────────────────
    auto* barGroup = new QGroupBox("Win / Loss / Draw  (vs AI)");
    auto* barGrid  = new QGridLayout(barGroup);

    m_winsLabel   = new QLabel("Wins");
    m_lossesLabel = new QLabel("Losses");
    m_drawsLabel  = new QLabel("Draws");

    m_winBar  = new QProgressBar;
    m_lossBar = new QProgressBar;
    m_drawBar = new QProgressBar;

    m_winBar ->setTextVisible(true);
    m_lossBar->setTextVisible(true);
    m_drawBar->setTextVisible(true);
    m_winBar ->setStyleSheet("QProgressBar::chunk { background: #4CAF50; }");
    m_lossBar->setStyleSheet("QProgressBar::chunk { background: #F44336; }");
    m_drawBar->setStyleSheet("QProgressBar::chunk { background: #9E9E9E; }");

    barGrid->addWidget(m_winsLabel,   0, 0);
    barGrid->addWidget(m_winBar,      0, 1);
    barGrid->addWidget(m_lossesLabel, 1, 0);
    barGrid->addWidget(m_lossBar,     1, 1);
    barGrid->addWidget(m_drawsLabel,  2, 0);
    barGrid->addWidget(m_drawBar,     2, 1);
    mainLayout->addWidget(barGroup);

    // ── HvH section ───────────────────────────────────────────────
    auto* hvhGroup = new QGroupBox("Human vs Human");
    auto* hvhLayout = new QVBoxLayout(hvhGroup);
    m_hvhLabel = new QLabel;
    m_hvhLabel->setAlignment(Qt::AlignCenter);
    hvhLayout->addWidget(m_hvhLabel);
    mainLayout->addWidget(hvhGroup);

    // ── Separator ─────────────────────────────────────────────────
    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // ── Buttons ───────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout;
    m_resetBtn   = new QPushButton("Reset Statistics");
    m_resetBtn->setStyleSheet("QPushButton { color: #F44336; }");
    auto* closeBtn = new QPushButton("Close");
    btnRow->addWidget(m_resetBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    mainLayout->addLayout(btnRow);

    connect(m_resetBtn, &QPushButton::clicked, this, &StatsDialog::onReset);
    connect(closeBtn,   &QPushButton::clicked, this, &QDialog::accept);

    refresh();
}

void StatsDialog::refresh()
{
    int total = m_stats->gamesPlayed();
    m_gamesLabel->setText(QString("Games Played: <b>%1</b>").arg(total));

    int wins   = m_stats->humanWins();
    int losses = m_stats->humanLosses();
    int draws  = m_stats->draws();
    int aiGames = wins + losses + draws;

    m_winBar ->setMaximum(qMax(aiGames, 1));
    m_lossBar->setMaximum(qMax(aiGames, 1));
    m_drawBar->setMaximum(qMax(aiGames, 1));

    m_winBar ->setValue(wins);
    m_lossBar->setValue(losses);
    m_drawBar->setValue(draws);

    m_winBar ->setFormat(QString("%1 wins (%2%)")
        .arg(wins).arg(aiGames > 0 ? wins*100/aiGames : 0));
    m_lossBar->setFormat(QString("%1 losses (%2%)")
        .arg(losses).arg(aiGames > 0 ? losses*100/aiGames : 0));
    m_drawBar->setFormat(QString("%1 draws (%2%)")
        .arg(draws).arg(aiGames > 0 ? draws*100/aiGames : 0));

    m_hvhLabel->setText(
        QString("White wins: <b>%1</b>    Black wins: <b>%2</b>")
        .arg(m_stats->hvhWhiteWins())
        .arg(m_stats->hvhBlackWins()));
}

void StatsDialog::onReset()
{
    auto ans = QMessageBox::question(this, "Reset Stats",
        "Reset all statistics and game history?",
        QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes)
        m_stats->resetStats();
}
