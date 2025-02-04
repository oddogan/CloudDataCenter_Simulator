#include "strategies/drl/ILPDQNStrategy.h"
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "logging/LogManager.h"
#include "DataCenter.h"

ILPDQNStrategy::ILPDQNStrategy() : m_gap(0.01), m_Mu(250), m_Tau(0.75), m_Beta(1.0), m_Gamma(1.0), m_MST(1.0), m_extraMachineCoefficient(5.0), m_maximumRequestsInPM(100e3)
{
    m_chosenMachines.resize(1e3, nullptr);
    m_chosenMachineCount = 0;
    m_turnedOffMachines.resize(1e3, nullptr);

    // Define the value sets
    std::vector<double> mu = {1.0, 0.95, 0.9};
    std::vector<double> tau = {1.0, 0.95, 0.90, 0.85, 0.8, 0.75};
    std::vector<std::tuple<double, double>> beta_gamma = {
        {1.0, 1.0}, {0.5, 0.5}, {1.0, -1.0}, {0.5, -1.0}, {0.8, -1.0}, {0.8, 0.8}, {0.85, -1.0}};
    std::vector<double> mst = {200, 250, 300};

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

    m_agent = new DQNAgent(10, m_actions.size(), 1e-3);
}

ILPDQNStrategy::~ILPDQNStrategy()
{
}

Results ILPDQNStrategy::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
{
    Results results;
    results.placementDecision.reserve(newRequests.size());
    results.migrationDecision.reserve(toMigrate.size());

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

    // Log the selected action in an aligned way for each parameter

    ChooseMachines(const_cast<std::vector<PhysicalMachine> &>(machines), newRequests, toMigrate);

    int I = m_chosenMachineCount;
    int J = newRequests.size();
    int nMig = toMigrate.size();

    LogManager::instance().log(LogCategory::DEBUG, "ILPDQNStrategy: Running ILP with " + std::to_string(I) + " PMs, " + std::to_string(J) + " new requests, and " + std::to_string(nMig) + " migration requests");

    // CPLEX model
    IloEnv env;

    double solutionCost = std::numeric_limits<double>::max();
    bool feasible = false;

    try
    {
        IloModel model(env);

        // Decision variables

        IloArray<IloBoolVarArray> x_newcomers(env, J); // Newcomer Requests to assign
        for (int j = 0; j < J; ++j)
        {
            x_newcomers[j] = IloBoolVarArray(env, I);
        }

        IloArray<IloBoolVarArray> x_migrations(env, nMig); // Migration Requests to assign
        for (int j = 0; j < nMig; ++j)
        {
            x_migrations[j] = IloBoolVarArray(env, I);
        }

        IloBoolVarArray y(env, I); // PM activation status
        for (int i = 0; i < I; ++i)
        {
            y[i] = IloBoolVar(env); // PM is turned on or not
        }

        IloBoolVarArray migrate(env, nMig); // Indicates if migration request is actually migrated
        for (int j = 0; j < nMig; ++j)
        {
            for (int i = 0; i < I; ++i)
            {
                x_migrations[j][i] = IloBoolVar(env);
            }
            migrate[j] = IloBoolVar(env);
        }

        // COST FUNCTION

        IloExpr cost(env);
        // Cost 1: Turning on a PM cost
        for (int i = 0; i < I; ++i)
        {
            cost += y[i] * (m_chosenMachines[i]->isTurnedOn() ? 1 : 100);
        }

        // Cost 2: Adding migration costs
        for (int j = 0; j < nMig; ++j)
        {
            cost += migrate[j] * m_Mu; // Additional cost for each migration
        }

        // Cost 3: Adding a dynamic cost depends on to PMs utilization for newcomer requests
        for (int i = 0; i < I; ++i)
        {
            // Calculate the CPU utilization dynamically (assuming current assignments are part of the model)
            double nCPUUtilization = m_chosenMachines[i]->getUtilization().cpu;

            for (int j = 0; j < J; ++j)
            {
                double additionalCost = 0;
                if (nCPUUtilization < 45)
                {
                    additionalCost = m_chosenMachines[i]->getPowerConsumptionCPU() * (300 - 4 * nCPUUtilization) * newRequests[j]->getUsage().cpu;
                }
                else
                {
                    additionalCost = m_chosenMachines[i]->getPowerConsumptionCPU() * (4 * nCPUUtilization - 60) * newRequests[j]->getUsage().cpu;
                }

                if (m_Beta < 0)
                {
                    additionalCost *= (newRequests[j]->getUsage().cpu / newRequests[j]->getTotalRequestedResources().cpu);
                }
                else
                {
                    additionalCost *= m_Beta;
                }

                cost += x_newcomers[j][i] * additionalCost;
            }
        }

        // Cost 4: Adding a dynamic cost depends on to PMs utilization for migration requests
        for (int i = 0; i < I; ++i)
        {
            // Calculate the CPU utilization dynamically (assuming current assignments are part of the model)
            double nCPUUtilization = floor(((m_chosenMachines[i]->getFreeResources().cpu * -1.0) / m_chosenMachines[i]->getTotal().cpu) * 100.0 + 100);

            for (int j = 0; j < nMig; ++j) // NOTE: Changed here (J to nMig)
            {
                double additionalCost = 0;
                if (nCPUUtilization < 45)
                {
                    additionalCost = m_chosenMachines[i]->getPowerConsumptionCPU() * (300 - 4 * nCPUUtilization) * toMigrate[j]->getUsage().cpu;
                }
                else
                {
                    additionalCost = m_chosenMachines[i]->getPowerConsumptionCPU() * (4 * nCPUUtilization - 60) * toMigrate[j]->getUsage().cpu;
                }

                if (m_Gamma < 0)
                {
                    additionalCost *= (toMigrate[j]->getUsage().cpu / toMigrate[j]->getTotalRequestedResources().cpu);
                }
                else
                {
                    additionalCost *= m_Gamma;
                }

                cost += x_migrations[j][i] * additionalCost;
            }
        }

        model.add(IloMinimize(env, cost));

        // CONSTRAINTS
        // Constraint 1: Each request can only be assigned to one PM
        for (int j = 0; j < J; ++j)
        {
            IloExpr sum(env);
            for (int i = 0; i < I; ++i)
            {
                sum += x_newcomers[j][i];
            }
            model.add(sum == 1); // NOTE: Changed here (== to <=)
            // model.add(IloRange(env, 1, sum, 1, "AllocatedToOnePM"));
            sum.end();
        }

        // Constraint 2: Resource constraints using A1, A2, A3, A4, A5 matrices
        for (int i = 0; i < I; ++i)
        {
            IloExpr sumCPU(env);
            IloExpr sumMemory(env);
            IloExpr sumDisk(env);
            IloExpr sumBW(env);
            IloExpr sumFPGA(env);

            for (int j = 0; j < J; ++j)
            {
                Resources requestedResources = newRequests[j]->getUsage();
                sumCPU += x_newcomers[j][i] * requestedResources.cpu;
                sumMemory += x_newcomers[j][i] * requestedResources.ram;
                sumDisk += x_newcomers[j][i] * requestedResources.disk;
                sumBW += x_newcomers[j][i] * requestedResources.bandwidth;
                sumFPGA += x_newcomers[j][i] * requestedResources.fpga;
            }

            for (int k = 0; k < nMig; ++k)
            {
                Resources requestedResources = toMigrate[k]->getUsage();
                sumCPU += x_migrations[k][i] * requestedResources.cpu;
                sumMemory += x_migrations[k][i] * requestedResources.ram;
                sumDisk += x_migrations[k][i] * requestedResources.disk;
                sumBW += x_migrations[k][i] * requestedResources.bandwidth;
                sumFPGA += x_migrations[k][i] * requestedResources.fpga;
            }

            auto freeResources = m_chosenMachines[i]->getFreeResources();
            model.add(sumCPU <= std::max(0.0, freeResources.cpu));
            // cout << "sumCPU: " << sumCPU << "and A1[" << i << "][0] is " << A1[i][0] <<endl;
            model.add(sumMemory <= freeResources.ram);
            model.add(sumDisk <= freeResources.disk);
            model.add(sumBW <= freeResources.bandwidth);
            model.add(sumFPGA <= freeResources.fpga);

            sumCPU.end();
            sumMemory.end();
            sumDisk.end();
            sumBW.end();
            sumFPGA.end();
        }

        // Constraint 3: Link PM activation to request assignment using the Big-M method
        for (int i = 0; i < I; ++i)
        {
            IloExpr sum(env);
            // Sum up assignment variables for newcomer requests
            for (int j = 0; j < J; ++j)
            {
                sum += x_newcomers[j][i];
            }
            // Sum up assignment variables for migration requests
            for (int j = 0; j < nMig; ++j)
            {
                sum += x_migrations[j][i];
            }
            // Ensure the PM is turned on if it has any requests assigned to it
            model.add(sum <= m_maximumRequestsInPM * y[i]); // If any request is assigned, PM must be on
            sum.end();
        }

        // Constraint 4: for each migration request should be migrated to 1 PM or not migrate
        for (int j = 0; j < nMig; ++j)
        {
            IloExpr sumMigrationAssignments(env);
            for (int i = 0; i < I; ++i)
            {
                sumMigrationAssignments += x_migrations[j][i];
            }
            model.add(sumMigrationAssignments == migrate[j]); // Linking migration decision to the assignment of PMs
            sumMigrationAssignments.end();
        }

        // Constraint 5: It is the constraint about how much load we need to migrate according to the TAM value defined in my thesis
        IloExpr remainingCPU(env);
        for (int j = 0; j < nMig; ++j)
        {
            remainingCPU += (1 - migrate[j]) * ceil(toMigrate[j]->getUsage().cpu); // CPU load of non-migrated requests
        }
        double totalCPUCapacity = m_chosenMachines[0]->getTotal().cpu; // Assuming PMq is defined and nTotalCPU is its total CPU capacity
        // TODO: violatedPM

        model.add(remainingCPU <= m_Tau * totalCPUCapacity); // Constraint to keep remaining load within threshold
        remainingCPU.end();

        //  SOLVE THE ILP
        IloCplex cplex(model);
        cplex.setParam(IloCplex::Param::TimeLimit, 60.0);
        // cplex.setParam(IloCplex::Param::Parallel, IloCplex::Parallel_Mode::Opportunistic);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, m_gap);
        cplex.setOut(env.getNullStream());
        bool ok = cplex.solve();

        if (ok)
        {
            solutionCost = cplex.getObjValue();
            feasible = true;
        }

        // Output results
        for (int j = 0; j < J; ++j)
        {
            bool assigned = false;
            for (int i = 0; i < I; ++i)
            {
                if (cplex.getValue(x_newcomers[j][i]) > 0.5)
                {
                    results.placementDecision.push_back({newRequests[j], m_chosenMachines[i]->getID()});
                    assigned = true;
                    break;
                }
            }
            if (!assigned)
            {
                results.placementDecision.push_back({newRequests[j], -1});
            }
        }

        for (int j = 0; j < nMig; ++j)
        {
            if (cplex.getValue(migrate[j]) > 0.5)
            {
                for (int i = 0; i < I; ++i)
                {
                    if (cplex.getValue(x_migrations[j][i]) > 0.5)
                    {
                        results.migrationDecision.push_back({toMigrate[j], m_chosenMachines[i]->getID()});
                    }
                }
            }
        }

        cplex.clear();
    }
    catch (IloException &e)
    {
        throw std::runtime_error("ILP Error: " + std::string(e.getMessage()));
    }

    // Save DQN info
    m_lastReward = feasible ? -solutionCost : -1000.0;
    m_lastState = state;
    m_lastActionIdx = aidx;
    m_lastFeasibility = feasible;

    env.end();

    return results;
}

double ILPDQNStrategy::getMigrationThreshold()
{
    return m_MST;
}

void ILPDQNStrategy::ChooseMachines(std::vector<PhysicalMachine> &machines, const std::vector<VirtualMachine *> &requests, const std::vector<VirtualMachine *> &migrations)
{
    m_chosenMachineCount = 0;

    for (auto &machine : machines)
    {
        if (machine.isTurnedOn())
        {
            m_chosenMachines[m_chosenMachineCount++] = &machine;
        }
    }

    size_t numExtraPMsToInclude = std::min(machines.size() - m_chosenMachineCount, static_cast<size_t>(m_extraMachineCoefficient * (requests.size() + migrations.size())));

    // Find numExtraPMsToInclude turned off machines with the lowest power consumption and add them to the chosenMachines vector
    size_t turnedOffMachineCount = 0;
    for (auto &machine : machines)
    {
        if (!machine.isTurnedOn())
        {
            m_turnedOffMachines[turnedOffMachineCount++] = &machine;
        }
    }

    std::sort(m_turnedOffMachines.begin(), m_turnedOffMachines.begin() + turnedOffMachineCount, [this](PhysicalMachine *a, PhysicalMachine *b)
              { return CalculatePowerOnCost(*a) < CalculatePowerOnCost(*b); });

    for (size_t i = 0; i < numExtraPMsToInclude; ++i)
    {
        m_chosenMachines[m_chosenMachineCount++] = m_turnedOffMachines[i];
    }

    LogManager::instance().log(LogCategory::DEBUG, "ILPDQNStrategy: Chose " + std::to_string(m_chosenMachineCount) + " PMs for ILP. " + std::to_string(numExtraPMsToInclude) + " extra PMs included");
}

double ILPDQNStrategy::CalculatePowerOnCost(PhysicalMachine &machine)
{
    return machine.getPowerOnCost() + machine.getPowerConsumptionCPU() * 4.0 + machine.getPowerConsumptionFPGA() * 2.0;
}

void ILPDQNStrategy::updateAgent()
{
    // Update DQN
    auto nextState = ComputeState();

    // Store transition
    m_agent->storeTransition({m_lastState, m_lastActionIdx, m_lastReward, nextState, !m_lastFeasibility});

    // Do Q-learning update
    m_agent->update();
}

QWidget *ILPDQNStrategy::createConfigWidget(QWidget *parent)
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

        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

void ILPDQNStrategy::applyConfigFromUI()
{
    m_gap = m_configWidget->findChild<QDoubleSpinBox *>()->value();
    qDebug() << "ILPDQNStrategy: MIP Gap set to " << m_gap;
}

QString ILPDQNStrategy::name() const
{
    return "ILP + DQN Strategy";
}
std::vector<double> ILPDQNStrategy::ComputeState()
{
    std::vector<double> state(13, 0.0);

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

    return state;
}