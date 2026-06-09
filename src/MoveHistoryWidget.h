#pragma once
#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "chess.h"

// ─────────────────────────────────────────────────────────────────────────────
//  MoveHistoryWidget
//
//  Displays the move list in algebraic notation.
//  Connected to ChessController::moveApplied via Signal & Slot.
// ─────────────────────────────────────────────────────────────────────────────

class MoveHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MoveHistoryWidget(QWidget* parent = nullptr);

public slots:
    void onMoveApplied(Move m, bool /*capture*/, bool isCheck,
                       bool isCheckmate, bool /*castle*/,
                       bool /*promo*/);
    void onUndoPerformed();
    void onNewGame();

private:
    QListWidget* m_list;
    QLabel*      m_header;
    int          m_moveNumber = 1;
    bool         m_whiteNext  = true;
    QString      m_pendingWhite;

    QString formatMove(const Move& m, bool isCheck, bool isCheckmate) const;
};
