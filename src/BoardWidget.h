#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QTimer>
#include <vector>
#include <unordered_map>

#include "chess.h"
#include "ChessController.h"
#include "AnimationManager.h"
#include "ThemeManager.h"

// ─────────────────────────────────────────────────────────────────────────────
//  BoardWidget
//
//  Renders the 8×8 chess board and handles all user input:
//    • Click-to-move  (two-click: select → destination)
//    • Drag-and-drop  (press, drag, release)
//    • Move highlighting, legal move dots, last-move shading
//    • Piece animations (animated glide via AnimationManager)
//    • King-in-check red glow
//
//  DSA: Event-Driven Programming
//    – Every user gesture (mouse press, move, release) generates a Qt
//      event handled by overridden event methods.
//    – The board never polls; it reacts only to events.
//
//  DSA: Signal & Slot
//    – Emits moveRequested() → ChessController::tryMove()
//    – Receives boardChanged(), moveAnimationRequested() from controller
//    – Receives frameReady() from AnimationManager → schedules repaint
// ─────────────────────────────────────────────────────────────────────────────

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(ChessController* ctrl, QWidget* parent = nullptr);

    void setFlipped(bool f);
    bool isFlipped() const { return m_flipped; }

    // ── Replay / read-only support ────────────────────────────────
    void setInteractive(bool on) { m_interactive = on; }
    void setBoard(const Board* b) { m_replayBoard = b; update(); }

    QSize sizeHint() const override { return {560, 560}; }
    QSize minimumSizeHint() const override { return {320, 320}; }

signals:
    void moveRequested(Square from, Square to, PieceType promotion);
    void squareClicked(Square sq);

public slots:
    void onBoardChanged();
    void onMoveAnimationRequested(Square from, Square to);
    void onThemeChanged(ThemeManager::Theme t);

protected:
    // ── Qt event overrides (Event-Driven Programming) ────────────
    void paintEvent(QPaintEvent* event)       override;
    void mousePressEvent(QMouseEvent* event)  override;
    void mouseMoveEvent(QMouseEvent* event)   override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event)     override;

    // Drag-and-drop
    void dragEnterEvent(QDragEnterEvent*)     override;
    void dragMoveEvent(QDragMoveEvent*)       override;
    void dropEvent(QDropEvent*)               override;

private:
    // ── Layout helpers ───────────────────────────────────────────
    int   squareSize()     const;
    int   boardOffset()    const;  // centering offset
    QRect squareRect(int row, int col) const;
    QRect squareRect(Square sq)        const;
    Square squareAt(QPoint p)          const;
    QPoint squareCenter(Square sq)     const;

    // ── Rendering ────────────────────────────────────────────────
    void drawBoard(QPainter& p);
    void drawCoordinates(QPainter& p);
    void drawPieces(QPainter& p);
    void drawHighlights(QPainter& p);
    void drawLegalMoveDots(QPainter& p);
    void drawLastMoveHighlight(QPainter& p);
    void drawCheckHighlight(QPainter& p);
    void drawAnimatedPiece(QPainter& p);
    void drawDraggedPiece(QPainter& p);
    void drawThinkingIndicator(QPainter& p);

    // ── Piece pixmaps ────────────────────────────────────────────
    void loadPiecePixmaps();
    const QPixmap& piecePixmap(Piece p) const;

    // ── Click-to-move state machine ───────────────────────────────
    void handleSquareClick(Square sq);
    void clearSelection();
    bool needsPromotion(Square from, Square to) const;
    PieceType askPromotion();

    // ── Drag state ───────────────────────────────────────────────
    bool    m_dragging      = false;
    Square  m_dragFrom      = {};
    QPoint  m_dragPos;
    QPixmap m_dragPixmap;

    // ── Click-move state ─────────────────────────────────────────
    bool    m_hasSelection  = false;
    Square  m_selected      = {};
    std::vector<Move> m_legalFromSelected;

    // ── Last-move tracking ────────────────────────────────────────
    Square  m_lastFrom      = {};
    Square  m_lastTo        = {};
    bool    m_hasLastMove   = false;

    // ── Refs & managers ───────────────────────────────────────────
    ChessController*  m_ctrl;
    bool              m_interactive  = true;
    const Board*      m_replayBoard  = nullptr; // non-null in replay mode
    AnimationManager* m_anim;
    bool              m_flipped = false;

    // ── Animation tracking ────────────────────────────────────────
    Square  m_animFrom      = {};
    Square  m_animTo        = {};
    Piece   m_animPiece;
    bool    m_animating     = false;

    // ── Piece image cache ─────────────────────────────────────────
    // Key = Piece::encode() (4 bits: color|type)
    std::unordered_map<int, QPixmap> m_pixmaps;

    // ── Thinking dots animation ────────────────────────────────────
    QTimer  m_thinkTimer;
    int     m_thinkFrame = 0;
};
