#include "MoveHistoryWidget.h"

MoveHistoryWidget::MoveHistoryWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_header = new QLabel("Move History", this);
    m_header->setAlignment(Qt::AlignCenter);
    QFont hf = m_header->font();
    hf.setPointSize(11);
    hf.setBold(true);
    m_header->setFont(hf);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    m_list->setFont(QFont("Courier New", 10));
    m_list->setMaximumWidth(220);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);

    layout->addWidget(m_header);
    layout->addWidget(m_list);
}

void MoveHistoryWidget::onNewGame()
{
    m_list->clear();
    m_moveNumber = 1;
    m_whiteNext  = true;
    m_pendingWhite.clear();
}

void MoveHistoryWidget::onMoveApplied(Move m, bool, bool isCheck,
                                       bool isCheckmate, bool, bool)
{
    QString notation = formatMove(m, isCheck, isCheckmate);

    if (m_whiteNext) {
        m_pendingWhite = notation;
        m_whiteNext    = false;
    } else {
        // Complete the row: "1. e4   e5"
        QString row = QString("%1. %2  %3")
                          .arg(m_moveNumber, 3)
                          .arg(m_pendingWhite, -6)
                          .arg(notation);
        m_list->addItem(row);
        m_list->scrollToBottom();
        m_pendingWhite.clear();
        ++m_moveNumber;
        m_whiteNext = true;
    }
}

void MoveHistoryWidget::onUndoPerformed()
{
    // Remove last displayed row (which could be a half-completed row)
    if (!m_whiteNext) {
        // White's move was pending but black hadn't moved
        m_pendingWhite.clear();
        m_whiteNext = true;
    } else if (m_list->count() > 0) {
        delete m_list->takeItem(m_list->count() - 1);
        --m_moveNumber;
        m_whiteNext = false;
        // We'd need to re-show pending white; simplification: just decrement
    }
}

QString MoveHistoryWidget::formatMove(const Move& m, bool isCheck,
                                      bool isCheckmate) const
{
    // Simplified algebraic (UCI format + check/mate annotations)
    QString s = QString::fromStdString(m.toString());
    if (isCheckmate)   s += "#";
    else if (isCheck)  s += "+";
    return s;
}
