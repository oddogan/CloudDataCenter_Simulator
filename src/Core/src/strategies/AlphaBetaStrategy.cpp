#include "strategies/AlphaBetaStrategy.h"
#include <algorithm>
#include "strategies/MachineState.h"
#include "data/Resources.h"

AlphaBetaStrategy::AlphaBetaStrategy()
    : m_alpha(1.0), m_beta(2.0), m_alphaSpin(nullptr), m_betaSpin(nullptr), m_configWidget(nullptr)
{
}

AlphaBetaStrategy::~AlphaBetaStrategy()
{
}

Results AlphaBetaStrategy::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
{
    Results results;

    // Build ephemeral
    std::vector<MachineState> localStates;
    localStates.reserve(machines.size());
    for (auto &pm : machines)
    {
        MachineState ms;
        ms.id = pm.getID();
        ms.isTurnedOn = pm.isTurnedOn();
        ms.powerOnCost = pm.getPowerOnCost();
        ms.cpuCost = pm.getPowerConsumptionCPU();
        ms.total = pm.getTotal();
        ms.used = pm.getUsed();
        localStates.push_back(ms);
    }

    // Example: place VMs in descending order by (alpha*CPU + beta*RAM)
    std::vector<VirtualMachine *> sorted = newRequests;
    std::sort(sorted.begin(), sorted.end(), [&](auto *a, auto *b)
              {
        double costA = m_alpha * a->getUsage().cpu + m_beta * a->getUsage().ram;
        double costB = m_alpha * b->getUsage().cpu + m_beta * b->getUsage().ram;
        return costA > costB; });

    results.placementDecision.reserve(newRequests.size());

    // We'll do a naive "first fit" but using the sorted array
    for (auto *vm : sorted)
    {
        Resources need = vm->getTotalRequestedResources();
        bool placed = false;
        for (auto &ms : localStates)
        {
            if (ms.canHost(need))
            {
                ms.used += need; // ephemeral
                results.placementDecision.push_back({vm, ms.id});
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            results.placementDecision.push_back({vm, -1});
        }
    }
    return results;
}

double AlphaBetaStrategy::getMigrationThreshold()
{
    return 0.0;
}

size_t AlphaBetaStrategy::getBundleSize()
{
    return 10;
}

// Build the config widget with spin boxes for alpha & beta
QWidget *AlphaBetaStrategy::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);

        m_alphaSpin = new QDoubleSpinBox(m_configWidget);
        m_alphaSpin->setRange(0.0, 999.0);
        m_alphaSpin->setValue(m_alpha);

        m_betaSpin = new QDoubleSpinBox(m_configWidget);
        m_betaSpin->setRange(0.0, 999.0);
        m_betaSpin->setValue(m_beta);

        auto layout = new QFormLayout(m_configWidget);
        layout->addRow("Alpha", m_alphaSpin);
        layout->addRow("Beta", m_betaSpin);

        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

// Called by GUI when user hits "Apply" or something
void AlphaBetaStrategy::applyConfigFromUI()
{
    if (m_alphaSpin && m_betaSpin)
    {
        m_alpha = m_alphaSpin->value();
        m_beta = m_betaSpin->value();
    }
}

QString AlphaBetaStrategy::name() const
{
    return "AlphaBetaStrategy";
}

QWidget *AlphaBetaStrategy::createStatusWidget(QWidget *parent)
{
    if (!m_statusWidget)
    {
        m_statusWidget = new QWidget(parent);
        auto layout = new QFormLayout(m_statusWidget);
        auto label = new QLabel("Alpha: " + QString::number(m_alpha), m_statusWidget);
        layout->addRow(label);
        label = new QLabel("Beta: " + QString::number(m_beta), m_statusWidget);
        layout->addRow(label);
        m_statusWidget->setLayout(layout);
    }
    return m_statusWidget;
}