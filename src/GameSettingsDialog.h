#pragma once
#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include "ChessController.h"

// ─────────────────────────────────────────────────────────────────────────────
//  GameSettingsDialog
//
//  New Game configuration: mode (HvH, HvAI, AIvH) + difficulty + side.
// ─────────────────────────────────────────────────────────────────────────────

class GameSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GameSettingsDialog(const GameConfig& current, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("New Game Settings");
        setModal(true);
        setMinimumWidth(300);

        auto* mainLayout = new QVBoxLayout(this);

        // ── Game mode ─────────────────────────────────────────────
        auto* modeGroup = new QGroupBox("Game Mode", this);
        auto* modeLayout = new QVBoxLayout(modeGroup);
        m_hvh   = new QRadioButton("Human vs Human",    this);
        m_hvai  = new QRadioButton("Human vs AI",       this);
        m_aivh  = new QRadioButton("AI vs Human",       this);
        modeLayout->addWidget(m_hvh);
        modeLayout->addWidget(m_hvai);
        modeLayout->addWidget(m_aivh);
        modeGroup->setLayout(modeLayout);
        mainLayout->addWidget(modeGroup);

        // Set initial
        if (current.mode == PlayerMode::HumanVsHuman)     m_hvh->setChecked(true);
        else if (current.mode == PlayerMode::HumanVsAI)   m_hvai->setChecked(true);
        else                                               m_aivh->setChecked(true);

        // ── Difficulty ────────────────────────────────────────────
        auto* diffGroup = new QGroupBox("AI Difficulty", this);
        auto* diffLayout = new QHBoxLayout(diffGroup);
        m_diffCombo = new QComboBox(this);
        m_diffCombo->addItem("Easy",   QVariant(int(EASY)));
        m_diffCombo->addItem("Medium", QVariant(int(MEDIUM)));
        m_diffCombo->addItem("Hard",   QVariant(int(HARD)));
        m_diffCombo->addItem("Expert", QVariant(int(EXPERT)));
        // Select current
        for (int i = 0; i < m_diffCombo->count(); ++i)
            if (m_diffCombo->itemData(i).toInt() == int(current.difficulty))
                m_diffCombo->setCurrentIndex(i);
        diffLayout->addWidget(new QLabel("Level:", this));
        diffLayout->addWidget(m_diffCombo);
        diffGroup->setLayout(diffLayout);
        mainLayout->addWidget(diffGroup);

        // ── Side ──────────────────────────────────────────────────
        auto* sideGroup = new QGroupBox("Human plays as", this);
        auto* sideLayout = new QHBoxLayout(sideGroup);
        m_playWhite = new QRadioButton("White", this);
        m_playBlack = new QRadioButton("Black", this);
        sideLayout->addWidget(m_playWhite);
        sideLayout->addWidget(m_playBlack);
        sideGroup->setLayout(sideLayout);
        mainLayout->addWidget(sideGroup);
        (current.humanColor == WHITE ? m_playWhite : m_playBlack)->setChecked(true);

        // ── Buttons ───────────────────────────────────────────────
        auto* btnRow = new QHBoxLayout();
        auto* ok     = new QPushButton("Start Game", this);
        auto* cancel = new QPushButton("Cancel",     this);
        ok->setDefault(true);
        btnRow->addStretch();
        btnRow->addWidget(cancel);
        btnRow->addWidget(ok);
        mainLayout->addLayout(btnRow);

        connect(ok,     &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    GameConfig result() const
    {
        GameConfig cfg;
        if      (m_hvh->isChecked())  cfg.mode = PlayerMode::HumanVsHuman;
        else if (m_hvai->isChecked()) cfg.mode = PlayerMode::HumanVsAI;
        else                          cfg.mode = PlayerMode::AIVsHuman;

        cfg.difficulty = static_cast<Difficulty>(m_diffCombo->currentData().toInt());
        cfg.humanColor = m_playWhite->isChecked() ? WHITE : BLACK;
        return cfg;
    }

private:
    QRadioButton* m_hvh;
    QRadioButton* m_hvai;
    QRadioButton* m_aivh;
    QComboBox*    m_diffCombo;
    QRadioButton* m_playWhite;
    QRadioButton* m_playBlack;
};
