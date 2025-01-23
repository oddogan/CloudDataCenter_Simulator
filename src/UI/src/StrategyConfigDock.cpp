#include "StrategyConfigDock.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include "strategies/StrategyFactory.h"

StrategyConfigDock::StrategyConfigDock(QWidget *parent)
    : QDockWidget(parent), m_container(nullptr), m_formLayout(nullptr), m_strategyCombo(nullptr), m_applyBtn(nullptr), m_currentStrategy(nullptr), m_currentStrategyWidget(nullptr)
{
    setWindowTitle("Strategy Config");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    createUI();
}

StrategyConfigDock::~StrategyConfigDock()
{
    clearCurrentStrategy();
}

void StrategyConfigDock::createUI()
{
    m_container = new QWidget(this);
    m_formLayout = new QFormLayout(m_container);

    m_strategyCombo = new QComboBox(m_container);
    loadStrategyList();
    connect(m_strategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StrategyConfigDock::onStrategyChanged);
    m_formLayout->addRow("Strategy:", m_strategyCombo);

    m_applyBtn = new QPushButton("Apply", m_container);
    connect(m_applyBtn, &QPushButton::clicked, this, &StrategyConfigDock::onApplyClicked);
    m_formLayout->addRow(m_applyBtn);

    m_container->setLayout(m_formLayout);
    setWidget(m_container);

    // select first strategy if any
    if (m_strategyCombo->count() > 0)
    {
        onStrategyChanged(0);
    }
}

void StrategyConfigDock::loadStrategyList()
{
    auto list = StrategyFactory::availableStrategies();
    for (auto &info : list)
    {
        m_strategyCombo->addItem(info.name);
    }
}

void StrategyConfigDock::clearCurrentStrategy()
{
    if (m_currentStrategy)
    {
        delete m_currentStrategy;
        m_currentStrategy = nullptr;
    }

    if (m_currentStrategyWidget)
    {
        m_formLayout->removeWidget(m_currentStrategyWidget);
        m_currentStrategyWidget = nullptr;
    }
}

void StrategyConfigDock::onStrategyChanged(int index)
{
    // pick a new strategy name
    QString name = m_strategyCombo->itemText(index);
    // clear old
    clearCurrentStrategy();

    // create new
    m_currentStrategy = StrategyFactory::create(name);
    if (!m_currentStrategy)
    {
        qDebug() << "[StrategyConfigDock] Could not create strategy" << name;
        return;
    }

    // embed its config widget
    QWidget *w = m_currentStrategy->createConfigWidget(m_container);
    if (w)
    {
        m_formLayout->addRow(w);
        m_currentStrategyWidget = w;
    }
    qDebug() << "[StrategyConfigDock] Strategy changed to" << name;
}

void StrategyConfigDock::onApplyClicked()
{
    if (!m_currentStrategy)
        return;

    // ask the strategy to apply
    m_currentStrategy->applyConfigFromUI();

    // we might do dataCenter->setStrategy(m_currentStrategy);
    // but then we'd transfer ownership.
    // For demonstration, just logging:
    qDebug() << "[StrategyConfigDock] Strategy apply done for"
             << m_currentStrategy->name();
}