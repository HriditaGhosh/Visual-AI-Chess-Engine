#include "BoardWidget.h"
#include "PromotionDialog.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDrag>
#include <QMimeData>
#include <QFont>
#include <QFontMetrics>
#include <QFile>
#include <QSvgRenderer>
#include <cmath>
#include <algorithm>

// ── Construction ─────────────────────────────────────────────────────────────

BoardWidget::BoardWidget(ChessController* ctrl, QWidget* parent)
    : QWidget(parent), m_ctrl(ctrl)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    // Animated piece
    m_animPiece = Piece();

    // DSA: Signal & Slot — AnimationManager ticks → repaint
    m_anim = new AnimationManager(this);
    connect(m_anim, &AnimationManager::frameReady,
            this,   [this]{ update(); });
    connect(m_anim, &AnimationManager::animationFinished,
            this,   [this]{
                m_animating = false;
                update();
            });

    // Thinking spinner
    m_thinkTimer.setInterval(400);
    connect(&m_thinkTimer, &QTimer::timeout,
            this, [this]{ m_thinkFrame = (m_thinkFrame + 1) % 4; update(); });

    connect(m_ctrl, &ChessController::aiThinkingStarted,
            this,   [this]{ m_thinkTimer.start(); update(); });
    connect(m_ctrl, &ChessController::aiThinkingFinished,
            this,   [this](SearchResult){ m_thinkTimer.stop(); update(); });

    // DSA: Signal & Slot — theme broadcasts trigger board repaint
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &BoardWidget::onThemeChanged);

    loadPiecePixmaps();
}

// ── Layout ───────────────────────────────────────────────────────────────────

int BoardWidget::squareSize() const
{
    return std::min(width(), height()) / 8;
}

int BoardWidget::boardOffset() const
{
    int sq = squareSize();
    return (std::min(width(), height()) - sq * 8) / 2;
}

QRect BoardWidget::squareRect(int row, int col) const
{
    int sq  = squareSize();
    int off = boardOffset();
    int displayRow = m_flipped ? row       : (7 - row);
    int displayCol = m_flipped ? (7 - col) : col;
    return QRect(off + displayCol * sq, off + displayRow * sq, sq, sq);
}

QRect BoardWidget::squareRect(Square sq) const
{
    return squareRect(sq.row, sq.col);
}

Square BoardWidget::squareAt(QPoint p) const
{
    int sq  = squareSize();
    int off = boardOffset();
    int dc = (p.x() - off) / sq;
    int dr = (p.y() - off) / sq;
    if (dc < 0 || dc > 7 || dr < 0 || dr > 7) return Square(-1, -1);
    int col = m_flipped ? (7 - dc) : dc;
    int row = m_flipped ? dr       : (7 - dr);
    return Square(row, col);
}

QPoint BoardWidget::squareCenter(Square sq) const
{
    return squareRect(sq).center();
}

// ── Public slots ─────────────────────────────────────────────────────────────

void BoardWidget::onBoardChanged()
{
    update();
}

void BoardWidget::onMoveAnimationRequested(Square from, Square to)
{
    // DSA: Animation queue — store animation target; anim manager queues frames
    Piece p = m_ctrl->board().at(to);  // piece already moved on the board
    m_animFrom  = from;
    m_animTo    = to;
    m_animPiece = p;
    m_animating = true;

    QPointF startPx = squareCenter(from);
    QPointF endPx   = squareCenter(to);
    int dist = static_cast<int>(
        std::hypot(endPx.x() - startPx.x(), endPx.y() - startPx.y()));
    int duration = std::clamp(dist * 2, 80, 300);

    m_anim->animate(startPx, endPx, duration, [this]{ m_animating = false; });
}

void BoardWidget::onThemeChanged(ThemeManager::Theme)
{
    update();
}

void BoardWidget::setFlipped(bool f)
{
    m_flipped = f;
    update();
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void BoardWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    drawBoard(p);
    drawLastMoveHighlight(p);
    drawCheckHighlight(p);
    drawHighlights(p);
    drawLegalMoveDots(p);
    drawPieces(p);
    drawAnimatedPiece(p);
    drawDraggedPiece(p);
    drawCoordinates(p);

    if (m_ctrl->isAIThinking())
        drawThinkingIndicator(p);
}

void BoardWidget::drawBoard(QPainter& p)
{
    ThemeManager* tm = ThemeManager::instance();
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            bool light = (r + c) % 2 == 0;
            p.fillRect(squareRect(r, c),
                       light ? tm->lightSquare() : tm->darkSquare());
        }
    }
}

void BoardWidget::drawCoordinates(QPainter& p)
{
    ThemeManager* tm = ThemeManager::instance();
    int sq  = squareSize();
    int off = boardOffset();
    QFont font("Segoe UI", std::max(8, sq / 7), QFont::Bold);
    p.setFont(font);

    for (int i = 0; i < 8; ++i) {
        bool lightBg = i % 2 == 0;
        p.setPen(lightBg ? tm->darkSquare() : tm->lightSquare());

        // Rank numbers (left edge)
        int rank = m_flipped ? (i + 1) : (8 - i);
        QRect rankRect(off, off + i * sq, sq / 4, sq / 4);
        p.drawText(rankRect.adjusted(3, 2, 0, 0), Qt::AlignLeft | Qt::AlignTop,
                   QString::number(rank));

        // File letters (bottom edge)
        char fileCh = m_flipped ? ('h' - i) : ('a' + i);
        QRect fileRect(off + i * sq, off + 8 * sq - sq / 4, sq, sq / 4);
        p.drawText(fileRect.adjusted(0, 0, -3, -2), Qt::AlignRight | Qt::AlignBottom,
                   QString(fileCh));
    }
}

void BoardWidget::drawHighlights(QPainter& p)
{
    if (!m_hasSelection) return;
    ThemeManager* tm = ThemeManager::instance();

    // Selected square glow
    p.fillRect(squareRect(m_selected), tm->highlightColor());
}

void BoardWidget::drawLegalMoveDots(QPainter& p)
{
    if (!m_hasSelection) return;
    ThemeManager* tm = ThemeManager::instance();
    int sq = squareSize();

    for (const Move& m : m_legalFromSelected) {
        QRect r = squareRect(m.to);
        bool capture = !m_ctrl->board().at(m.to).empty() || m.isEnPassant;

        if (capture) {
            // Ring around capture square
            p.setPen(QPen(tm->legalMoveColor(), sq * 0.08));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(r.adjusted(sq * 0.05, sq * 0.05, -sq * 0.05, -sq * 0.05));
        } else {
            // Filled dot for quiet move
            p.setBrush(tm->legalMoveColor());
            p.setPen(Qt::NoPen);
            int dotR = sq / 6;
            p.drawEllipse(r.center(), dotR, dotR);
        }
    }
}

void BoardWidget::drawLastMoveHighlight(QPainter& p)
{
    if (!m_hasLastMove) return;
    ThemeManager* tm = ThemeManager::instance();
    p.fillRect(squareRect(m_lastFrom), tm->lastMoveColor());
    p.fillRect(squareRect(m_lastTo),   tm->lastMoveColor());
}

void BoardWidget::drawCheckHighlight(QPainter& p)
{
    GameStatus st = m_ctrl->status();
    if (st != CHECK && st != CHECKMATE) return;

    ThemeManager* tm = ThemeManager::instance();
    Color side = m_ctrl->board().sideToMove;

    // Find king square by scanning the board
    Square kingSq(-1, -1);
    for (int r = 0; r < 8 && !kingSq.valid(); ++r)
        for (int c = 0; c < 8 && !kingSq.valid(); ++c) {
            const Piece& pc = m_ctrl->board().at(r, c);
            if (pc.type == KING && pc.color == side)
                kingSq = Square(r, c);
        }

    if (!kingSq.valid()) return;

    // Radial gradient red glow
    QRect r = squareRect(kingSq);
    QRadialGradient grad(r.center(), r.width() / 2);
    grad.setColorAt(0.0, tm->checkColor());
    grad.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(r, grad);
}

void BoardWidget::drawPieces(QPainter& p)
{
    int sq = squareSize();
    int pad = sq / 10;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Square sqPos(r, c);

            // Skip the square being dragged from
            if (m_dragging && sqPos == m_dragFrom) continue;

            // Skip animated piece destination (we'll draw it flying)
            if (m_animating && sqPos == m_animTo) continue;

            const Piece& pc = (m_replayBoard ? *m_replayBoard : m_ctrl->board()).at(r, c);
            if (pc.empty()) continue;

            QRect rect = squareRect(r, c).adjusted(pad, pad, -pad, -pad);
            p.drawPixmap(rect, piecePixmap(pc));
        }
    }
}

void BoardWidget::drawAnimatedPiece(QPainter& p)
{
    if (!m_animating || !m_anim->isAnimating()) return;

    int sq  = squareSize();
    int pad = sq / 10;
    QPointF pos = m_anim->currentPos();

    QRect rect(pos.x() - (sq - pad * 2) / 2,
               pos.y() - (sq - pad * 2) / 2,
               sq - pad * 2, sq - pad * 2);

    // Slight scale-up during flight
    float prog = m_anim->progress();
    float scale = 1.0f + 0.12f * std::sin(prog * M_PI);
    QSize scaledSize(rect.width() * scale, rect.height() * scale);
    QPoint center = rect.center();
    QRect scaledRect(center.x() - scaledSize.width() / 2,
                     center.y() - scaledSize.height() / 2,
                     scaledSize.width(), scaledSize.height());

    if (!m_animPiece.empty())
        p.drawPixmap(scaledRect, piecePixmap(m_animPiece));
}

void BoardWidget::drawDraggedPiece(QPainter& p)
{
    if (!m_dragging || m_dragPixmap.isNull()) return;
    int sq = squareSize();
    int pad = sq / 10;
    QRect rect(m_dragPos.x() - sq / 2 + pad,
               m_dragPos.y() - sq / 2 + pad,
               sq - pad * 2, sq - pad * 2);

    // Drop shadow
    p.save();
    p.setOpacity(0.25);
    p.fillRect(rect.translated(4, 6), Qt::black);
    p.setOpacity(1.0);
    p.drawPixmap(rect, m_dragPixmap);
    p.restore();
}

void BoardWidget::drawThinkingIndicator(QPainter& p)
{
    ThemeManager* tm = ThemeManager::instance();
    int off = boardOffset();
    int total = squareSize() * 8;
    int dotR = 6;
    int dotCount = 4;
    int spacing = 20;
    int startX = off + total / 2 - ((dotCount - 1) * spacing) / 2;
    int y = off + total + 18;

    for (int i = 0; i < dotCount; ++i) {
        float alpha = (i == m_thinkFrame) ? 1.0f : 0.3f;
        QColor c(0x6F, 0x9B, 0xD6, static_cast<int>(255 * alpha));
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(startX + i * spacing, y), dotR, dotR);
    }
}

// ── Mouse events  (Event-Driven Programming) ─────────────────────────────────

void BoardWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_interactive) return;
    if (event->button() != Qt::LeftButton) return;
    if (m_ctrl->isAIThinking())            return;

    Square sq = squareAt(event->pos());
    if (!sq.valid()) return;

    const Piece& pc = m_ctrl->board().at(sq);
    bool ownPiece = (!pc.empty() && pc.color == m_ctrl->board().sideToMove);

    if (ownPiece) {
        // Start drag
        m_dragging  = true;
        m_dragFrom  = sq;
        m_dragPos   = event->pos();
        m_dragPixmap = piecePixmap(pc);

        // Also handle as click selection
        if (!m_hasSelection || m_selected != sq) {
            m_hasSelection = true;
            m_selected     = sq;
            m_legalFromSelected = m_ctrl->movesFrom(sq);
        }
        update();
    } else if (m_hasSelection) {
        // Try move via click
        handleSquareClick(sq);
    }
}

void BoardWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        m_dragPos = event->pos();
        update();
    }
}

void BoardWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    if (m_dragging) {
        Square toSq = squareAt(event->pos());
        m_dragging = false;

        if (toSq.valid() && toSq != m_dragFrom) {
            // Drag-drop move
            if (needsPromotion(m_dragFrom, toSq)) {
                PieceType promo = askPromotion();
                if (promo != NONE_PIECE) {
                    m_lastFrom = m_dragFrom;
                    m_lastTo   = toSq;
                    m_hasLastMove = true;
                    clearSelection();
                    emit moveRequested(m_dragFrom, toSq, promo);
                }
            } else {
                m_lastFrom = m_dragFrom;
                m_lastTo   = toSq;
                m_hasLastMove = true;
                clearSelection();
                emit moveRequested(m_dragFrom, toSq, NONE_PIECE);
            }
        } else {
            update();
        }
    }
}

// ── Click-to-move logic ───────────────────────────────────────────────────────

void BoardWidget::handleSquareClick(Square sq)
{
    if (!m_hasSelection) {
        const Piece& pc = m_ctrl->board().at(sq);
        if (!pc.empty() && pc.color == m_ctrl->board().sideToMove) {
            m_hasSelection = true;
            m_selected     = sq;
            m_legalFromSelected = m_ctrl->movesFrom(sq);
        }
        update();
        return;
    }

    // Already have a selection — try to move
    if (sq == m_selected) {
        clearSelection();
        update();
        return;
    }

    // Check if clicked on own piece — switch selection
    const Piece& pc = m_ctrl->board().at(sq);
    if (!pc.empty() && pc.color == m_ctrl->board().sideToMove) {
        m_selected = sq;
        m_legalFromSelected = m_ctrl->movesFrom(sq);
        update();
        return;
    }

    // Try move
    if (needsPromotion(m_selected, sq)) {
        PieceType promo = askPromotion();
        if (promo == NONE_PIECE) { clearSelection(); update(); return; }
        m_lastFrom = m_selected;
        m_lastTo   = sq;
        m_hasLastMove = true;
        Square from = m_selected;
        clearSelection();
        emit moveRequested(from, sq, promo);
    } else {
        m_lastFrom = m_selected;
        m_lastTo   = sq;
        m_hasLastMove = true;
        Square from = m_selected;
        clearSelection();
        emit moveRequested(from, sq, NONE_PIECE);
    }
}

void BoardWidget::clearSelection()
{
    m_hasSelection = false;
    m_selected     = Square(-1, -1);
    m_legalFromSelected.clear();
}

bool BoardWidget::needsPromotion(Square from, Square to) const
{
    const Piece& pc = m_ctrl->board().at(from);
    if (pc.type != PAWN) return false;
    return (pc.color == WHITE && to.row == 7) ||
           (pc.color == BLACK && to.row == 0);
}

PieceType BoardWidget::askPromotion()
{
    PromotionDialog dlg(m_ctrl->board().sideToMove, this);
    if (dlg.exec() == QDialog::Accepted)
        return dlg.selectedPiece();
    return NONE_PIECE;
}

// ── Drag-and-drop overrides ───────────────────────────────────────────────────

void BoardWidget::dragEnterEvent(QDragEnterEvent* e) { e->acceptProposedAction(); }
void BoardWidget::dragMoveEvent(QDragMoveEvent* e)   { e->acceptProposedAction(); }
void BoardWidget::dropEvent(QDropEvent*)              {}

// ── Resize ────────────────────────────────────────────────────────────────────

void BoardWidget::resizeEvent(QResizeEvent*)
{
    update();
}

// ── Piece pixmaps ─────────────────────────────────────────────────────────────

void BoardWidget::loadPiecePixmaps()
{
    // Load SVG icons from Qt resources → QPixmap (128×128)
    // Falls back to Unicode glyph rendering if SVG not available
    struct PieceInfo {
        Color color; PieceType type;
        QString svgKey;   // resource alias e.g. "wK"
        QString symbol;   // fallback Unicode glyph
    };
    const PieceInfo pieces[] = {
        {WHITE, KING,   "wK", "♔"}, {WHITE, QUEEN,  "wQ", "♕"},
        {WHITE, ROOK,   "wR", "♖"}, {WHITE, BISHOP, "wB", "♗"},
        {WHITE, KNIGHT, "wN", "♘"}, {WHITE, PAWN,   "wP", "♙"},
        {BLACK, KING,   "bK", "♚"}, {BLACK, QUEEN,  "bQ", "♛"},
        {BLACK, ROOK,   "bR", "♜"}, {BLACK, BISHOP, "bB", "♝"},
        {BLACK, KNIGHT, "bN", "♞"}, {BLACK, PAWN,   "bP", "♟"},
    };

    for (const auto& pi : pieces) {
        int key = Piece(pi.type, pi.color).encode();

        // ── Try SVG resource first ─────────────────────────────
        QString resourcePath = ":/icons/" + pi.svgKey + ".svg";
        QPixmap pix(128, 128);
        pix.fill(Qt::transparent);

        // Use QSvgRenderer for correct SVG rendering
        bool svgLoaded = false;
        if (QFile::exists(resourcePath)) {
            QSvgRenderer renderer(resourcePath);
            if (renderer.isValid()) {
                pix.fill(Qt::transparent);
                QPainter svgPainter(&pix);
                renderer.render(&svgPainter);
                svgLoaded = true;
            }
        }

        // ── Fallback: Unicode glyph rendered with QPainter ─────
        if (!svgLoaded) {
            QPainter painter(&pix);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);

            QFont font("Segoe UI Symbol", 80);
            font.setStyleStrategy(QFont::PreferAntialias);
            painter.setFont(font);

            QRect rect(0, 0, 128, 128);

            // Outline pass (contrast against any square color)
            QColor outlineColor = (pi.color == WHITE)
                                  ? QColor(0x40, 0x30, 0x20)
                                  : QColor(0xFF, 0xFF, 0xFF, 160);
            painter.setPen(outlineColor);
            for (int dx = -2; dx <= 2; dx += 2)
                for (int dy = -2; dy <= 2; dy += 2)
                    painter.drawText(rect.translated(dx, dy),
                                     Qt::AlignCenter, pi.symbol);

            // Main fill
            painter.setPen(pi.color == WHITE
                           ? QColor(0xFF, 0xFF, 0xFF)
                           : QColor(0x18, 0x18, 0x18));
            painter.drawText(rect, Qt::AlignCenter, pi.symbol);
        }

        m_pixmaps[key] = pix;
    }
}

const QPixmap& BoardWidget::piecePixmap(Piece p) const
{
    static QPixmap empty;
    auto it = m_pixmaps.find(p.encode());
    if (it == m_pixmaps.end()) return empty;
    return it->second;
}
