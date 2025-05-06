#include "strategies/FirstFitDecreasing.h"
#include <algorithm>
#include "strategies/MachineState.h"
#include "data/Resources.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

FirstFitDecreasing::FirstFitDecreasing()
    : m_configWidget(nullptr)
{
}

FirstFitDecreasing::~FirstFitDecreasing()
{
}

Results FirstFitDecreasing::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
{
    Results results;

    // Build ephemeral MachineState from each real PM
    std::vector<MachineState> localStates;
    localStates.reserve(machines.size());
    for (auto &pm : machines)
    {
        MachineState ms;
        ms.id = pm.getID();
        ms.total = pm.getTotal(); // reading pm's lock internally
        ms.used = pm.getReservedUsages();
        localStates.push_back(ms);
    }

    // 1) Handle newRequests
    // Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sortedNew = newRequests;
    std::sort(sortedNew.begin(), sortedNew.end(), [](auto *a, auto *b)
              { return a->getTotalRequestedResources().cpu > b->getTotalRequestedResources().cpu; });

    // Reserve space for results
    results.placementDecision.reserve(newRequests.size());

    // For each VM in descending order, do a "First Fit"
    for (auto *vm : sortedNew)
    {
        Resources need = vm->getTotalRequestedResources();
        bool placed = false;

        for (auto &ms : localStates)
        {
            if (ms.canHost(need))
            {
                ms.used += need; // ephemeral allocation
                results.placementDecision.push_back({vm, ms.id});
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            // no fit found
            results.placementDecision.push_back({vm, -1});
        }
    }

    // 2) Handle toMigrate
    // Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sortedMig = toMigrate;
    std::sort(sortedMig.begin(), sortedMig.end(), [](auto *a, auto *b)
              { return a->getTotalRequestedResources().cpu > b->getTotalRequestedResources().cpu; });

    // Reserve space for results
    results.migrationDecision.reserve(toMigrate.size());

    // For each VM in descending order, do a "First Fit"
    for (auto *vm : sortedMig)
    {
        Resources need = vm->getTotalRequestedResources();
        bool placed = false;

        for (auto &ms : localStates)
        {
            if (ms.canHost(need))
            {
                ms.used += need; // ephemeral allocation
                results.migrationDecision.push_back({vm, ms.id});
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            // no fit found
            results.migrationDecision.push_back({vm, -1});
        }
    }

    return results;
}

double FirstFitDecreasing::getMigrationThreshold()
{
    return 1.0;
}

size_t FirstFitDecreasing::getBundleSize()
{
    return 10;
}

QWidget *FirstFitDecreasing::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);
        auto layout = new QVBoxLayout(m_configWidget);

        QLabel *label = new QLabel("No configuration for FirstFitDecreasing", m_configWidget);
        layout->addWidget(label);

        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

void FirstFitDecreasing::applyConfigFromUI()
{
    // No parameters to update
}

QString FirstFitDecreasing::name() const
{
    return "FirstFitDecreasing";
}