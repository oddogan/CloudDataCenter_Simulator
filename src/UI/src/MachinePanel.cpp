#include <QHBoxLayout>
#include <QDebug>
#include "MachinePanel.h"

MachinePanel::MachinePanel(QWidget *parent) : QWidget(parent)
{
    createUI();
}

MachinePanel::~MachinePanel()
{
}

void MachinePanel::createUI()
{
    auto layout = new QVBoxLayout(this);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setText("PM #?");
    layout->addWidget(m_titleLabel);

    m_cpuBar = new QProgressBar(this);
    m_cpuBar->setFormat("CPU: %v/%m");
    m_cpuBar->setTextVisible(true);
    layout->addWidget(m_cpuBar);

    m_ramBar = new QProgressBar(this);
    m_ramBar->setFormat("RAM: %v/%m");
    m_ramBar->setTextVisible(true);
    layout->addWidget(m_ramBar);

    m_diskBar = new QProgressBar(this);
    m_diskBar->setFormat("Disk: %v/%m");
    m_diskBar->setTextVisible(true);
    layout->addWidget(m_diskBar);

    m_fpgaBar = new QProgressBar(this);
    m_fpgaBar->setFormat("FPGA: %v/%m");
    m_fpgaBar->setTextVisible(true);
    layout->addWidget(m_fpgaBar);
}

void MachinePanel::setMachineInfo(const MachineUsageInfo &info)
{
    m_titleLabel->setText("PM #" + QString::number(info.machineId));

    m_cpuBar->setRange(0, info.total.cpu);
    m_cpuBar->setValue(info.used.cpu);
    m_ramBar->setRange(0, info.total.ram);
    m_ramBar->setValue(info.used.ram);
    m_diskBar->setRange(0, info.total.disk);
    m_diskBar->setValue(info.used.disk);
    m_fpgaBar->setRange(0, info.total.fpga);
    m_fpgaBar->setValue(info.used.fpga);
}