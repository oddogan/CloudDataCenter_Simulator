#include "strategies/BestFitDecreasing.h"
#include <algorithm>
#include "strategies/MachineState.h"
#include "data/Resources.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

BestFitDecreasing::BestFitDecreasing()
    : m_configWidget(nullptr)
{
}

BestFitDecreasing::~BestFitDecreasing()
{
}

Results BestFitDecreasing::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
{
    Results results;

    // Build ephemeral
    std::vector<MachineState> localStates;
    localStates.reserve(machines.size());
    for (auto &pm : machines)
    {
        MachineState ms;
        ms.id = pm.getID();
        ms.total = pm.getTotal();
        ms.used = pm.getUsed();
        localStates.push_back(ms);
    }

    // 1) Handle newRequests
    // Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sortedNew = newRequests;
    std::sort(sortedNew.begin(), sortedNew.end(), [](auto *a, auto *b)
              { return a->getUsage().cpu > b->getUsage().cpu; });

    // Reserve space for results
    results.placementDecision.reserve(newRequests.size());

    // For each VM in descending order, do a "Best Fit"
    for (auto *vm : sortedNew)
    {
        Resources need = vm->getTotalRequestedResources();
        int bestIdx = -1;
        double bestLeftCPU = 1e9;

        // best fit
        for (int i = 0; i < (int)localStates.size(); i++)
        {
            auto &ms = localStates[i];
            if (ms.canHost(need))
            {
                double leftover = (ms.total.cpu - ms.used.cpu) - need.cpu;
                if (leftover < bestLeftCPU)
                {
                    bestLeftCPU = leftover;
                    bestIdx = i;
                }
            }
        }
        if (bestIdx >= 0)
        {
            localStates[bestIdx].used += need;
            results.placementDecision.push_back({vm, localStates[bestIdx].id});
        }
        else
        {
            results.placementDecision.push_back({vm, -1});
        }
    }

    // 2) Handle toMigrate
    // Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sortedMig = toMigrate;
    std::sort(sortedMig.begin(), sortedMig.end(), [](auto *a, auto *b)
              { return a->getUsage().cpu > b->getUsage().cpu; });

    // Reserve space for results
    results.migrationDecision.reserve(toMigrate.size());

    // For each VM in descending order, do a "Best Fit"
    for (auto *vm : sortedNew)
    {
        Resources need = vm->getTotalRequestedResources();
        int bestIdx = -1;
        double bestLeftCPU = 1e9;

        // best fit
        for (int i = 0; i < (int)localStates.size(); i++)
        {
            auto &ms = localStates[i];
            if (ms.canHost(need))
            {
                double leftover = (ms.total.cpu - ms.used.cpu) - need.cpu;
                if (leftover < bestLeftCPU)
                {
                    bestLeftCPU = leftover;
                    bestIdx = i;
                }
            }
        }
        if (bestIdx >= 0)
        {
            localStates[bestIdx].used += need;
            results.migrationDecision.push_back({vm, localStates[bestIdx].id});
        }
        else
        {
            results.migrationDecision.push_back({vm, -1});
        }
    }

    return results;
}

double BestFitDecreasing::getMigrationThreshold()
{
    return 0.0;
}

QWidget *BestFitDecreasing::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);
        auto layout = new QVBoxLayout(m_configWidget);
        QLabel *label = new QLabel("No configuration for BestFitDecreasing", m_configWidget);
        layout->addWidget(label);
        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

void BestFitDecreasing::applyConfigFromUI()
{
    // No parameters
}

QString BestFitDecreasing::name() const
{
    return "BestFitDecreasing";
}