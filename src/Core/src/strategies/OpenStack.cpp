#include "strategies/OpenStack.h"
#include "strategies/MachineState.h"

OpenStack::OpenStack()
{
}

OpenStack::~OpenStack()
{
}

Results OpenStack::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
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

    // 1) Handle newRequests
    // Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sortedNew = newRequests;
    std::sort(sortedNew.begin(), sortedNew.end(), [](auto *a, auto *b)
              { return a->getTotalRequestedResources().cpu > b->getTotalRequestedResources().cpu; });

    // Reserve space for results
    results.placementDecision.reserve(newRequests.size());

    // For each VM in descending order, do a "Best Fit"
    for (auto *vm : sortedNew)
    {
        Resources need = vm->getTotalRequestedResources();
        int bestIdx = -1;
        double bestPowerIncrease = std::numeric_limits<double>::max();

        // best fit
        for (int i = 0; i < (int)localStates.size(); i++)
        {
            auto &ms = localStates[i];
            if (ms.canHost(need))
            {
                if ((ms.total.cpu * (1 - m_ial) > (ms.total.cpu - ms.used.cpu - need.cpu)))
                {
                    continue; // skip if the PM is already too loaded
                }

                double powerIncrease = 0;
                if (!ms.isTurnedOn)
                    powerIncrease = ms.powerOnCost;

                powerIncrease += ms.cpuCost * need.cpu;

                if (powerIncrease < bestPowerIncrease)
                {
                    bestPowerIncrease = powerIncrease;
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
        Resources need = vm->getUsage();
        int bestIdx = -1;
        double bestPowerIncrease = std::numeric_limits<double>::max();

        // best fit
        for (int i = 0; i < (int)localStates.size(); i++)
        {
            auto &ms = localStates[i];
            if (ms.canHost(need))
            {
                if ((ms.total.cpu * (1 - m_ial) > (ms.total.cpu - ms.used.cpu - need.cpu)))
                {
                    continue; // skip if the PM is already too loaded
                }

                double powerIncrease = 0;
                if (!ms.isTurnedOn)
                    powerIncrease = ms.powerOnCost;

                powerIncrease += ms.cpuCost * need.cpu;

                if (powerIncrease < bestPowerIncrease)
                {
                    bestPowerIncrease = powerIncrease;
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
};

double OpenStack::getMigrationThreshold()
{
    return 1.0;
}

size_t OpenStack::getBundleSize()
{
    return 1;
}

QWidget *OpenStack::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);
        auto layout = new QFormLayout(m_configWidget);

        m_ialSpin = new QDoubleSpinBox(m_configWidget);
        m_ialSpin->setRange(0.0, 1.0);
        m_ialSpin->setValue(m_ial);
        layout->addRow("Initial Allocation Limit (IAL)", m_ialSpin);

        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

void OpenStack::applyConfigFromUI()
{
    if (m_ialSpin)
    {
        m_ial = m_ialSpin->value();
    }
}

QString OpenStack::name() const
{
    return "OpenStack";
}

QWidget *OpenStack::createStatusWidget(QWidget *parent)
{
    if (!m_statusWidget)
    {
        m_statusWidget = new QWidget(parent);
        auto layout = new QVBoxLayout(m_statusWidget);

        QLabel *label = new QLabel("No status for OpenStack", m_statusWidget);
        layout->addWidget(label);

        m_statusWidget->setLayout(layout);
    }
    return m_statusWidget;
}