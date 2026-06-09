#pragma once
#include <QDialog>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include "chess.h"

// ─────────────────────────────────────────────────────────────────────────────
//  PromotionDialog
//
//  Modal dialog for pawn promotion — user selects Queen/Rook/Bishop/Knight.
// ─────────────────────────────────────────────────────────────────────────────

class PromotionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PromotionDialog(Color color, QWidget* parent = nullptr)
        : QDialog(parent), m_selected(QUEEN)
    {
        setWindowTitle("Pawn Promotion");
        setModal(true);
        setFixedSize(340, 100);

        auto* layout = new QVBoxLayout(this);
        auto* label  = new QLabel("Choose promotion piece:", this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);

        auto* btnRow = new QHBoxLayout();
        layout->addLayout(btnRow);

        struct Choice { PieceType type; QString symbol; };
        const Choice choices[] = {
            {QUEEN,  color == WHITE ? "♕" : "♛"},
            {ROOK,   color == WHITE ? "♖" : "♜"},
            {BISHOP, color == WHITE ? "♗" : "♝"},
            {KNIGHT, color == WHITE ? "♘" : "♞"},
        };

        for (const auto& ch : choices) {
            auto* btn = new QPushButton(ch.symbol + "\n" +
                            QString(ch.type == QUEEN  ? "Queen"  :
                                    ch.type == ROOK   ? "Rook"   :
                                    ch.type == BISHOP ? "Bishop" : "Knight"),
                            this);
            btn->setFixedSize(72, 60);
            btn->setFont(QFont("Segoe UI Symbol", 20));
            PieceType t = ch.type;
            connect(btn, &QPushButton::clicked, this, [this, t]{
                m_selected = t;
                accept();
            });
            btnRow->addWidget(btn);
        }
    }

    PieceType selectedPiece() const { return m_selected; }

private:
    PieceType m_selected;
};
