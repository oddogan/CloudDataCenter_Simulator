#include "strategies/AlphaBetaStrategy.h"
#include <QFormLayout>
#include <algorithm>
#include "strategies/MachineState.h"
#include "data/Resources.h"

AlphaBetaStrategy::AlphaBetaStrategy()
    : m_alpha(1.0), m_beta(2.0), m_alphaSpin(nullptr), m_betaSpin(nullptr), m_configWidget(nullptr)
{
}

AlphaBetaStrategy::~AlphaBetaStrategy()
{
    // If we created the config widget, Qt will delete it if it has a parent,
    // but to be safe:
    delete m_configWidget;
}

std::vector<PlacementDecision> AlphaBetaStrategy::run(
    const std::vector<VirtualMachine *> &vms,
    const std::vector<PhysicalMachine> &pms)
{
    // Build ephemeral
    std::vector<MachineState> localStates;
    localStates.reserve(pms.size());
    for (auto &pm : pms)
    {
        MachineState ms;
        ms.id = pm.getID();
        ms.total = pm.getTotal();
        ms.used = pm.getUsed();
        localStates.push_back(ms);
    }

    // Example: place VMs in descending order by (alpha*CPU + beta*RAM)
    std::vector<VirtualMachine *> sorted = vms;
    std::sort(sorted.begin(), sorted.end(), [&](auto *a, auto *b)
              {
        double costA = m_alpha * a->getUsage().cpu + m_beta * a->getUsage().ram;
        double costB = m_alpha * b->getUsage().cpu + m_beta * b->getUsage().ram;
        return costA > costB; });

    std::vector<PlacementDecision> results;
    results.reserve(vms.size());

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
                results.push_back({vm, ms.id});
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            results.push_back({vm, -1});
        }
    }
    return results;
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