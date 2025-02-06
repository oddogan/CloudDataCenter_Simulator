#include "strategies/drl/IDQNStrategy.h"
#include "DataCenter.h"
#include <QFormLayout>
#include "logging/LogManager.h"

IDQNStrategy::IDQNStrategy()
{
    // Define the value sets
    std::vector<double> mu = {100, 200, 250, 300};
    std::vector<double> tau = {1.0, 0.95, 0.90, 0.85, 0.8, 0.75};
    std::vector<std::tuple<double, double>> beta_gamma = {
        {{0.5, 0.5}, {0.6, 0.3}, {0.4, 0.6}, {0.7, 0.3}}};
    std::vector<double> mst = {0.95, 0.9, 0.85, 0.8};

    // Nested loops to generate all combinations
    for (double m : mu)
    {
        for (double t : tau)
        {
            for (const auto &[b, g] : beta_gamma)
            {
                for (double ms : mst)
                {
                    m_actions.emplace_back(m, t, b, g, ms);
                }
            }
        }
    }
}

IDQNStrategy::~IDQNStrategy()
{
    delete m_agent;
}

Results IDQNStrategy::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
{
    // Build a state vector
    auto state = ComputeState();

    // Pick action from DQN -> index in s_actions
    int aidx = m_agent->selectAction(state);
    auto [mu, tau, beta, gamma, mst] = m_actions[aidx];
    m_Mu = mu;
    m_Tau = tau;
    m_Beta = beta;
    m_Gamma = gamma;
    m_MST = mst;

    LogManager::instance().log(LogCategory::DEBUG, "IDQNStrategy: Selected action: Mu = " + std::to_string(m_Mu) + ", Tau = " + std::to_string(m_Tau) + ", Beta = " + std::to_string(m_Beta) + ", Gamma = " + std::to_string(m_Gamma) + ", MST = " + std::to_string(m_MST));

    Results res = ILPStrategy::run(newRequests, toMigrate, machines);

    m_lastReward = -m_lastCost;
    m_lastState = state;
    m_lastActionIdx = aidx;
    m_lastFeasibility = ILPStrategy::m_lastFeasibility;

    return res;
}

double IDQNStrategy::getMigrationThreshold()
{
    return m_MST;
}

void IDQNStrategy::updateAgent()
{
    // Update DQN
    auto nextState = ComputeState();

    // Store transition
    m_agent->storeTransition({m_lastState, m_lastActionIdx, m_lastReward, nextState, !m_lastFeasibility});

    // Do Q-learning update
    m_agent->update();
}

QWidget *IDQNStrategy::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);
        auto layout = new QFormLayout(m_configWidget);

        auto gapSpin = new QDoubleSpinBox(m_configWidget);
        gapSpin->setRange(0.00, 1.00);
        gapSpin->setSingleStep(0.001);
        gapSpin->setValue(m_gap);
        layout->addRow("MIP Gap:", gapSpin);
        QObject::connect(gapSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double val)
                         { m_gap = val; });

        auto batchSizeSpin = new QSpinBox(m_configWidget);
        batchSizeSpin->setRange(1, 1024);
        batchSizeSpin->setValue(m_agent->getBatchSize());
        layout->addRow("Batch Size:", batchSizeSpin);
        QObject::connect(batchSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                         [this](int val)
                         { m_agent->setBatchSize(val); });

        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

void IDQNStrategy::applyConfigFromUI()
{
    m_gap = m_configWidget->findChild<QDoubleSpinBox *>()->value();
    qDebug() << "ILPDQNStrategy: MIP Gap set to " << m_gap;
    m_agent->setBatchSize(m_configWidget->findChild<QSpinBox *>()->value());
    qDebug() << "ILPDQNStrategy: Batch Size set to " << m_agent->getBatchSize();
}

std::vector<double> IDQNStrategy::ComputeState()
{
    std::vector<double> state(20, 0.0);

    auto machines = m_dataCenter->getPhysicalMachines();

    // Compute the active VM count
    int activeVMs = 0;
    for (auto &machine : machines)
    {
        activeVMs += machine.getVirtualMachines().size();
    }
    state[0] = activeVMs;

    // Compute the active PM count
    int activePMs = 0;
    for (auto &machine : machines)
    {
        if (machine.isTurnedOn())
        {
            activePMs++;
        }
    }
    state[1] = activePMs;

    // Compute the average utilizations and standard deviations
    std::vector<double> cpuUtilizations;
    std::vector<double> ramUtilizations;
    std::vector<double> diskUtilizations;
    std::vector<double> bwUtilizations;

    for (auto &machine : machines)
    {
        if (machine.isTurnedOn())
        {
            cpuUtilizations.push_back(machine.getUtilization().cpu);
            ramUtilizations.push_back(machine.getUtilization().ram);
            diskUtilizations.push_back(machine.getUtilization().disk);
            bwUtilizations.push_back(machine.getUtilization().bandwidth);
        }
    }

    state[2] = cpuUtilizations.size() > 0 ? std::accumulate(cpuUtilizations.begin(), cpuUtilizations.end(), 0.0) / cpuUtilizations.size() : 0.0;

    state[3] = cpuUtilizations.size() > 0 ? std::sqrt(std::accumulate(cpuUtilizations.begin(), cpuUtilizations.end(), 0.0, [state](double acc, double val)
                                                                      { return acc + (val - state[2]) * (val - state[2]); }) /
                                                      cpuUtilizations.size())
                                          : 0.0;

    state[4] = ramUtilizations.size() > 0 ? std::accumulate(ramUtilizations.begin(), ramUtilizations.end(), 0.0) / ramUtilizations.size() : 0.0;

    state[5] = ramUtilizations.size() > 0 ? std::sqrt(std::accumulate(ramUtilizations.begin(), ramUtilizations.end(), 0.0, [state](double acc, double val)
                                                                      { return acc + (val - state[4]) * (val - state[4]); }) /
                                                      ramUtilizations.size())
                                          : 0.0;

    state[6] = diskUtilizations.size() > 0 ? std::accumulate(diskUtilizations.begin(), diskUtilizations.end(), 0.0) / diskUtilizations.size() : 0.0;

    state[7] = diskUtilizations.size() > 0 ? std::sqrt(std::accumulate(diskUtilizations.begin(), diskUtilizations.end(), 0.0, [state](double acc, double val)
                                                                       { return acc + (val - state[6]) * (val - state[6]); }) /
                                                       diskUtilizations.size())
                                           : 0.0;

    state[8] = bwUtilizations.size() > 0 ? std::accumulate(bwUtilizations.begin(), bwUtilizations.end(), 0.0) / bwUtilizations.size() : 0.0;

    state[9] = bwUtilizations.size() > 0 ? std::sqrt(std::accumulate(bwUtilizations.begin(), bwUtilizations.end(), 0.0, [state](double acc, double val)
                                                                     { return acc + (val - state[8]) * (val - state[8]); }) /
                                                     bwUtilizations.size())
                                         : 0.0;

    // Compute the bins for CPU utilizations in PMs with 20% increments
    std::vector<int> cpuBins(5, 0);
    for (auto &machine : machines)
    {
        if (machine.isTurnedOn())
        {
            int bin = std::min(4, static_cast<int>(machine.getUtilization().cpu / 20.0));
            cpuBins[bin]++;
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        state[10 + i] = cpuBins[i];
    }

    // Add the stats since last placement to the state
    state[15] = m_dataCenter->getNumberofSLAVsSinceLastPlacement();
    state[16] = m_dataCenter->getNumberofMigrationsSinceLastPlacement();
    state[17] = m_dataCenter->getNumberofNewRequestsSinceLastPlacement();

    // Add the total SLAVs to the state
    state[18] = m_dataCenter->getNumberOfSLAViolations();

    // Add the total power consumption of the PMs to the state
    state[19] = m_dataCenter->getTotalPowerConsumption();

    return state;
}
