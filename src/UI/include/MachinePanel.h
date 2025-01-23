#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "data/PhysicalMachine.h"

class MachinePanel : public QWidget
{
    Q_OBJECT

public:
    explicit MachinePanel(QWidget *parent = nullptr);
    ~MachinePanel();

    void setMachineInfo(const MachineUsageInfo &info);

private:
    QLabel *m_titleLabel{nullptr};
    QProgressBar *m_cpuBar{nullptr};
    QProgressBar *m_ramBar{nullptr};
    QProgressBar *m_diskBar{nullptr};
    QProgressBar *m_fpgaBar{nullptr};

    void createUI();
};