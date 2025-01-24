#include "StatusDock.h"

StatusDock::StatusDock(ISimulationStatus *status, QWidget *parent) : QDockWidget(parent), m_status(status), m_timeLabel(nullptr), m_eventCountLabel(nullptr), m_processedCountLabel(nullptr), m_remainingCountLabel(nullptr), m_machineCountLabel(nullptr), m_timer(nullptr), m_formLayout(nullptr)
{
    setWindowTitle("Status");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    createUI();

    connect(&m_timer, &QTimer::timeout, this, &StatusDock::onTimer);
    m_timer.start(100);
}

StatusDock::~StatusDock()
{
}

void StatusDock::createUI()
{
    auto container = new QWidget(this);
    m_formLayout = new QFormLayout(container);

    m_timeLabel = new QLabel("0", container);
    m_eventCountLabel = new QLabel("0", container);
    m_processedCountLabel = new QLabel("0", container);
    m_remainingCountLabel = new QLabel("0", container);
    m_machineCountLabel = new QLabel("0", container);
    m_turnedOnMachineCountLabel = new QLabel("0", container);
    m_currentStrategyLabel = new QLabel("None", container);
    m_currentBundleSizeLabel = new QLabel("0", container);

    m_formLayout->addRow("Time:", m_timeLabel);
    m_formLayout->addRow("Event count:", m_eventCountLabel);
    m_formLayout->addRow("Processed count:", m_processedCountLabel);
    m_formLayout->addRow("Remaining count:", m_remainingCountLabel);
    m_formLayout->addRow("Machine count:", m_machineCountLabel);
    m_formLayout->addRow("Turned on machine count:", m_turnedOnMachineCountLabel);
    m_formLayout->addRow("Current strategy:", m_currentStrategyLabel);
    m_formLayout->addRow("Current bundle size:", m_currentBundleSizeLabel);

    container->setLayout(m_formLayout);
    setWidget(container);

    updateStatus();
}

void StatusDock::updateStatus()
{
    if (!m_status)
        return;

    m_timeLabel->setText(QString::number(m_status->getCurrentTime()));
    m_eventCountLabel->setText(QString::number(m_status->getEventCount()));
    m_processedCountLabel->setText(QString::number(m_status->getProcessedEventCount()));
    m_remainingCountLabel->setText(QString::number(m_status->getRemainingEventCount()));
    m_machineCountLabel->setText(QString::number(m_status->getMachineCount()));
    m_turnedOnMachineCountLabel->setText(QString::number(m_status->getTurnedOnMachineCount()));
    m_currentStrategyLabel->setText(QString::fromStdString(m_status->getCurrentStrategy()));
    m_currentBundleSizeLabel->setText(QString::number(m_status->getCurrentBundleSize()));
}

void StatusDock::onTimer()
{
    updateStatus();
}