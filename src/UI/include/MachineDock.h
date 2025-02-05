#pragma once

#include <QDockWidget>
#include <QScrollArea>
#include <QWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <vector>
#include "MachinePanel.h"
#include "Core/include/ISimulationStatus.h"

class MachineDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit MachineDock(ISimulationStatus *status, QWidget *parent = nullptr);
    ~MachineDock();

private:
    void onTimer();

private:
    ISimulationStatus *m_status;

    QWidget *m_container;
    QVBoxLayout *m_layout;
    QTimer m_timer;

    std::vector<MachinePanel *> m_panels;

    void createUI();
    void rebuildPanels();
    void updatePanels();
};