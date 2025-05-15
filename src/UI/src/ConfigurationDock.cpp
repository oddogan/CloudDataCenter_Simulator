#include "ConfigurationDock.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include "strategies/StrategyFactory.h"

ConfigurationDock::ConfigurationDock(ISimulationConfiguration *simulator, QWidget *parent)
    : QDockWidget(parent), m_simulator(simulator)
{
    setWindowTitle("Configuration");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    createUI();
}

ConfigurationDock::~ConfigurationDock()
{
    clearCurrentStrategy(true);
}

void ConfigurationDock::onStrategyResetClicked()
{
    clearCurrentStrategy(true);

    // Set the strategy combo box to select nothing
    m_strategyCombo->setCurrentIndex(-1);
}

void ConfigurationDock::createUI()
{
    m_container = new QWidget(this);
    m_formLayout = new QFormLayout(m_container);

    // Output file
    auto hboxOutputFile = new QHBoxLayout();

    auto outputFilePathLabel = new QLabel("Output file:", m_container);
    hboxOutputFile->addWidget(outputFilePathLabel);

    m_outputFilePathEdit = new QLineEdit(m_container);
    hboxOutputFile->addWidget(m_outputFilePathEdit);

    m_outputFileBrowseBtn = new QPushButton("Browse", m_container);
    connect(m_outputFileBrowseBtn, &QPushButton::clicked, this, &ConfigurationDock::onOutputFileBrowseClicked);
    hboxOutputFile->addWidget(m_outputFileBrowseBtn);

    m_outputFileApplyBtn = new QPushButton("Apply", m_container);
    connect(m_outputFileApplyBtn, &QPushButton::clicked, this, &ConfigurationDock::onOutputFileApplyClicked);
    hboxOutputFile->addWidget(m_outputFileApplyBtn);

    m_formLayout->addRow(hboxOutputFile);

    // Strategy
    m_strategyCombo = new QComboBox(m_container);
    loadStrategyList();
    connect(m_strategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigurationDock::onStrategyChanged);
    m_formLayout->addRow("Strategy:", m_strategyCombo);

    auto hbox = new QHBoxLayout();

    m_strategyApplyBtn = new QPushButton("Apply", m_container);
    connect(m_strategyApplyBtn, &QPushButton::clicked, this, &ConfigurationDock::onStrategyApplyClicked);
    hbox->addWidget(m_strategyApplyBtn);

    m_strategyResetBtn = new QPushButton("Reset", m_container);
    connect(m_strategyResetBtn, &QPushButton::clicked, this, &ConfigurationDock::onStrategyResetClicked);
    hbox->addWidget(m_strategyResetBtn);
    m_formLayout->addRow(hbox);

    // Get the current strategy from the simulator
    auto strategy = m_simulator->getPlacementStrategy();

    // Add its status widget
    if (strategy)
    {
        m_currentStrategyStatusWidget = strategy->createStatusWidget(m_container);
        if (m_currentStrategyStatusWidget)
        {
            m_formLayout->addRow(m_currentStrategyStatusWidget);
        }
    }

    m_container->setLayout(m_formLayout);
    setWidget(m_container);

    onStrategyResetClicked();
}

void ConfigurationDock::loadStrategyList()
{
    auto list = StrategyFactory::availableStrategies();
    for (auto &info : list)
    {
        m_strategyCombo->addItem(info.name);
    }
}

void ConfigurationDock::clearCurrentStrategy(bool deleteStrategy)
{
    if (m_currentStrategyWidget)
    {
        m_formLayout->removeRow(m_currentStrategyWidget);
        m_currentStrategyWidget = nullptr;
    }

    if (m_currentStrategy)
    {
        if (deleteStrategy)
            delete m_currentStrategy;

        m_currentStrategy = nullptr;
    }
}

void ConfigurationDock::onStrategyChanged(int index)
{
    if (index < 0 || index >= m_strategyCombo->count())
        return;

    // pick a new strategy name
    QString name = m_strategyCombo->itemText(index);
    // clear old
    clearCurrentStrategy(true);

    // create new
    m_currentStrategy = StrategyFactory::create(name);
    if (!m_currentStrategy)
    {
        qDebug() << "[ConfigurationDock] Could not create strategy" << name;
        return;
    }

    // embed its config widget
    QWidget *w = m_currentStrategy->createConfigWidget(m_container);
    if (w)
    {
        m_formLayout->addRow(w);
        m_currentStrategyWidget = w;
    }
    qDebug() << "[ConfigurationDock] Strategy changed to" << name;
}

void ConfigurationDock::onStrategyApplyClicked()
{
    if (!m_currentStrategy)
        return;

    // ask the strategy to apply
    m_currentStrategy->applyConfigFromUI();

    qDebug() << "[ConfigurationDock] Strategy apply done for" << m_currentStrategy->name();
    m_simulator->setPlacementStrategy(m_currentStrategy);

    // Update the strategy status widget with the new strategy's status widget, delete previous if still exists
    if (m_currentStrategyStatusWidget)
    {
        m_formLayout->removeRow(m_currentStrategyStatusWidget);
        m_currentStrategyStatusWidget = nullptr;
    }

    QWidget *statusWidget = m_currentStrategy->createStatusWidget(m_container);
    if (statusWidget)
    {
        m_formLayout->addRow(statusWidget);
        m_currentStrategyStatusWidget = statusWidget;
    }
    else
    {
        qDebug() << "[ConfigurationDock] No status widget for strategy" << m_currentStrategy->name();
    }

    // reset the combo box
    clearCurrentStrategy(false);
    m_strategyCombo->setCurrentIndex(-1);
}

void ConfigurationDock::onOutputFileBrowseClicked()
{
    QString path = QFileDialog::getSaveFileName(this, "Select Output File");
    if (!path.isEmpty())
    {
        m_outputFilePathEdit->setText(path);
    }
}

void ConfigurationDock::onOutputFileApplyClicked()
{
    if (!m_simulator)
        return;

    m_simulator->setOutputFile(m_outputFilePathEdit->text().toStdString());
    qDebug() << "[ConfigurationDock] Output file set to"
             << m_outputFilePathEdit->text();
}
