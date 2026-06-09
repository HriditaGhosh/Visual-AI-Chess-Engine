#include "ReplayBoardView.h"
#include <QPainter>
#include <QFile>

ReplayBoardView::ReplayBoardView(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(320, 320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    loadPixmaps();
}

void ReplayBoardView::loadPixmaps()
{
    // Load SVG renderers for all 12 piece types
    static const char* keys[] = {
        "wK","wQ","wR","wB","wN","wP",
        "bK","bQ","bR","bB","bN","bP"
    };
    for (const char* key : keys) {
        QString path = QString(":/icons/%1.svg").arg(key);
        if (QFile::exists(path)) {
            auto* r = new QSvgRenderer(path, this);
            if (r->isValid()) m_svg[key] = r;
            else              delete r;
        }
    }
    m_loaded = true;
}

QString ReplayBoardView::pieceKey(const Piece& p) const
{
    if (p.empty()) return {};
    static const char* types[] = {"","P","N","B","R","Q","K"};
    return QString("%1%2")
        .arg(p.color == WHITE ? 'w' : 'b')
        .arg(types[p.type]);
}

void ReplayBoardView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int w   = width();
    int h   = height();
    int sq  = qMin(w, h) / 8;
    int pad = (w - sq * 8) / 2;
    int vpad= (h - sq * 8) / 2;

    // Draw squares
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            bool light = (r + c) % 2 == 0;
            painter.fillRect(
                pad + c * sq,
                vpad + (7 - r) * sq,
                sq, sq,
                light ? QColor(0xC8, 0xA3, 0x75) : QColor(0x8B, 0x56, 0x2A)
            );
        }
    }

    if (!m_board) return;

    // Draw pieces
    int piecepad = sq / 9;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = m_board->at(r, c);
            if (p.empty()) continue;

            QRect rect(
                pad  + c * sq + piecepad,
                vpad + (7 - r) * sq + piecepad,
                sq - 2 * piecepad,
                sq - 2 * piecepad
            );

            QString key = pieceKey(p);
            if (m_svg.contains(key)) {
                m_svg[key]->render(&painter, rect);
            } else {
                // Unicode fallback
                static const QString glyphs[] = {
                    "","♙","♘","♗","♖","♕","♔",
                    "","♟","♞","♝","♜","♛","♚"
                };
                int idx = (p.color == WHITE ? 0 : 7) + p.type;
                painter.setPen(p.color == WHITE ? Qt::white : Qt::black);
                QFont f = painter.font();
                f.setPixelSize(sq - 8);
                painter.setFont(f);
                painter.drawText(rect, Qt::AlignCenter, glyphs[idx]);
            }
        }
    }

    // Draw coordinates
    painter.setPen(QColor(255, 255, 255, 160));
    QFont cf = painter.font();
    cf.setPixelSize(qMax(8, sq / 5));
    painter.setFont(cf);
    for (int i = 0; i < 8; ++i) {
        // File labels (a-h)
        painter.drawText(
            QRect(pad + i * sq, vpad + 8 * sq - cf.pixelSize() - 2, sq, cf.pixelSize() + 2),
            Qt::AlignCenter,
            QString(QChar('a' + i))
        );
        // Rank labels (1-8)
        painter.drawText(
            QRect(pad - cf.pixelSize() - 2, vpad + (7 - i) * sq, cf.pixelSize() + 2, sq),
            Qt::AlignCenter,
            QString::number(i + 1)
        );
    }
}
