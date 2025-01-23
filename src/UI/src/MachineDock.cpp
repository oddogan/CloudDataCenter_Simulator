#include "MachineDock.h"

MachineDock::MachineDock(ISimulationStatus *status, QWidget *parent)
{
    m_status = status;

    setWindowTitle("Machines");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    createUI();

    connect(&m_timer, &QTimer::timeout, this, &MachineDock::onTimer);
    m_timer.start(100);
}

MachineDock::~MachineDock()
{
    for (auto panel : m_panels)
    {
        delete panel;
    }
}

void MachineDock::createUI()
{
    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    m_container = new QWidget(scrollArea);
    m_layout = new QVBoxLayout(m_container);

    m_container->setLayout(m_layout);
    scrollArea->setWidget(m_container);

    setWidget(scrollArea);

    rebuildPanels();
}

void MachineDock::rebuildPanels()
{
    for (auto panel : m_panels)
    {
        delete panel;
    }
    m_panels.clear();

    if (!m_status)
        return;

    auto infos = m_status->getMachineUsageInfo();
    for (const auto &info : infos)
    {
        auto panel = new MachinePanel(m_container);
        panel->setMachineInfo(info);
        m_layout->addWidget(panel);
        m_panels.push_back(panel);
    }

    m_layout->addStretch(1);
}

void MachineDock::updatePanels()
{
    if (!m_status)
        return;

    auto infos = m_status->getMachineUsageInfo();

    // if the number of machines changed, rebuild
    if (infos.size() != m_panels.size())
    {
        rebuildPanels();
        return;
    }

    for (size_t i = 0; i < infos.size(); ++i)
    {
        m_panels[i]->setMachineInfo(infos[i]);
    }
}

void MachineDock::onTimer()
{
    updatePanels();
}