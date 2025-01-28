#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QFormLayout>
#include "ISimulationStatus.h"

class StatusDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit StatusDock(ISimulationStatus *status, QWidget *parent = nullptr);
    ~StatusDock();

private:
    void onTimer();

private:
    ISimulationStatus *m_status;
    QLabel *m_timeLabel;
    QLabel *m_eventCountLabel;
    QLabel *m_processedCountLabel;
    QLabel *m_remainingCountLabel;
    QLabel *m_machineCountLabel;
    QLabel *m_turnedOnMachineCountLabel;
    QLabel *m_currentStrategyLabel;
    QLabel *m_currentBundleSizeLabel;
    QTimer m_timer;
    QFormLayout *m_formLayout;

    void createUI();
    void updateStatus();
};