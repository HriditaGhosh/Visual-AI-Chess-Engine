#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "StatsManager.h"

class StatsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StatsDialog(StatsManager* stats, QWidget* parent = nullptr);

private slots:
    void onReset();
    void refresh();

private:
    void buildUI();

    StatsManager* m_stats;

    QLabel*       m_gamesLabel;
    QLabel*       m_winsLabel;
    QLabel*       m_lossesLabel;
    QLabel*       m_drawsLabel;
    QLabel*       m_hvhLabel;
    QProgressBar* m_winBar;
    QProgressBar* m_lossBar;
    QProgressBar* m_drawBar;
    QPushButton*  m_resetBtn;
};
