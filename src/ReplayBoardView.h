#pragma once
#include <QWidget>
#include <QSvgRenderer>
#include <QMap>
#include <QString>
#include "board.h"
#include "chess.h"

// ─────────────────────────────────────────────────────────────────────────────
//  ReplayBoardView
//
//  Lightweight read-only board widget for ReplayDialog.
//  Does NOT require a ChessController — renders directly from a Board*.
// ─────────────────────────────────────────────────────────────────────────────
class ReplayBoardView : public QWidget
{
    Q_OBJECT
public:
    explicit ReplayBoardView(QWidget* parent = nullptr);

    void setBoard(const Board* b) { m_board = b; update(); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void loadPixmaps();
    QString pieceKey(const Piece& p) const;

    const Board*                    m_board = nullptr;
    QMap<QString, QSvgRenderer*>    m_svg;
    bool                            m_loaded = false;

    // Board colors
    static constexpr QColor LIGHT = QColor(0xC8, 0xA3, 0x75);
    static constexpr QColor DARK  = QColor(0x8B, 0x56, 0x2A);
};
