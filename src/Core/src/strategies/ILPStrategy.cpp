#include <ilcplex/ilocplex.h>
#include <QFormLayout>
#include "strategies/ILPStrategy.h"
#include "logging/LogManager.h"

ILPStrategy::ILPStrategy() : m_Mu(250), m_Tau(0.75), m_Beta(1.0), m_Gamma(1.0), m_MST(1.0), m_extraMachineCoefficient(5.0), m_maximumRequestsInPM(100e3)
{
    m_chosenMachines.resize(1e3, nullptr);
    m_chosenMachineCount = 0;
    m_turnedOffMachines.resize(1e3, nullptr);
}

ILPStrategy::~ILPStrategy()
{
}

Results ILPStrategy::run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines)
{
    Results results;
    results.placementDecision.reserve(newRequests.size());
    results.migrationDecision.reserve(toMigrate.size());

    ChooseMachines(const_cast<std::vector<PhysicalMachine> &>(machines), newRequests, toMigrate);

    int I = m_chosenMachineCount;
    int J = newRequests.size();
    int nMig = toMigrate.size();

    LogManager::instance().log(LogCategory::DEBUG, "ILPStrategy: Running ILP with " + std::to_string(I) + " PMs, " + std::to_string(J) + " new requests, and " + std::to_string(nMig) + " migration requests");

    // CPLEX model
    IloEnv env;
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

        // Calculation of CpuUtilPer for eaxh PM for Cost 3

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
            double nCPUUtilization = floor(((m_chosenMachines[i]->getFreeResources().cpu * -1.0) / m_chosenMachines[i]->getTotal().cpu) * 100.0 + 100);

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
                cost += x_newcomers[j][i] * additionalCost * m_Beta;
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
                cost += x_migrations[j][i] * additionalCost * m_Gamma;
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

        // cout << "*********The CPU target after Allcation is:  " << Tau * stpPM_Array[violatedPM].nTotalCPU << endl;
        model.add(remainingCPU <= m_Tau * totalCPUCapacity); // Constraint to keep remaining load within threshold
        remainingCPU.end();

        //  SOLVE THE ILP

        IloCplex cplex(model);
        cplex.setParam(IloCplex::Param::TimeLimit, 60.0);
        // cplex.setParam(IloCplex::Param::Parallel, IloCplex::Parallel_Mode::Opportunistic);
        cplex.setOut(env.getNullStream());
        cplex.solve();

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

    env.end();

    return results;
}

double ILPStrategy::getMigrationThreshold()
{
    return m_MST;
}

void ILPStrategy::ChooseMachines(std::vector<PhysicalMachine> &machines, const std::vector<VirtualMachine *> &requests, const std::vector<VirtualMachine *> &migrations)
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

    LogManager::instance().log(LogCategory::DEBUG, "ILPStrategy: Chose " + std::to_string(m_chosenMachineCount) + " PMs for ILP. " + std::to_string(numExtraPMsToInclude) + " extra PMs included");
}

double ILPStrategy::CalculatePowerOnCost(PhysicalMachine &machine)
{
    return machine.getPowerOnCost() + machine.getPowerConsumptionCPU() * 4.0 + machine.getPowerConsumptionFPGA() * 2.0;
}

QWidget *ILPStrategy::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);
        auto layout = new QFormLayout(m_configWidget);

        m_MuSpin = new QDoubleSpinBox(m_configWidget);
        m_MuSpin->setMinimum(0.0);
        m_MuSpin->setMaximum(1000.0);
        m_MuSpin->setSingleStep(1.0);
        m_MuSpin->setValue(m_Mu);
        layout->addRow("Mu (Migration Cost):", m_MuSpin);

        m_TauSpin = new QDoubleSpinBox(m_configWidget);
        m_TauSpin->setMinimum(0.0);
        m_TauSpin->setMaximum(1.0);
        m_TauSpin->setSingleStep(0.01);
        m_TauSpin->setValue(m_Tau);
        layout->addRow("Tau (Target Utilization after Migration):", m_TauSpin);

        m_BetaSpin = new QDoubleSpinBox(m_configWidget);
        m_BetaSpin->setMinimum(0.0);
        m_BetaSpin->setMaximum(100.0);
        m_BetaSpin->setSingleStep(0.01);
        m_BetaSpin->setValue(m_Beta);
        layout->addRow("Beta (Expected Utilization Scaler for Newcomers):", m_BetaSpin);

        m_GammaSpin = new QDoubleSpinBox(m_configWidget);
        m_GammaSpin->setMinimum(0.0);
        m_GammaSpin->setMaximum(100.0);
        m_GammaSpin->setSingleStep(0.01);
        m_GammaSpin->setValue(m_Gamma);
        layout->addRow("Gamma (Expected Utilization Scaler for Migrations):", m_GammaSpin);

        m_MSTSpin = new QDoubleSpinBox(m_configWidget);
        m_MSTSpin->setMinimum(0.0);
        m_MSTSpin->setMaximum(1.0);
        m_MSTSpin->setSingleStep(0.01);
        m_MSTSpin->setValue(m_MST);
        layout->addRow("MST (Migration Start Threshold):", m_MSTSpin);

        m_extraMachineCoefficientSpin = new QDoubleSpinBox(m_configWidget);
        m_extraMachineCoefficientSpin->setMinimum(0.0);
        m_extraMachineCoefficientSpin->setMaximum(10.0);
        m_extraMachineCoefficientSpin->setSingleStep(0.1);
        m_extraMachineCoefficientSpin->setValue(m_extraMachineCoefficient);
        layout->addRow("Extra Machine Coefficient:", m_extraMachineCoefficientSpin);

        m_maximumRequestsInPMSpin = new QSpinBox(m_configWidget);
        m_maximumRequestsInPMSpin->setMinimum(1);
        m_maximumRequestsInPMSpin->setMaximum(2e5);
        m_maximumRequestsInPMSpin->setValue(m_maximumRequestsInPM);
        layout->addRow("Maximum Requests in PM:", m_maximumRequestsInPMSpin);

        m_configWidget->setLayout(layout);
    }

    return m_configWidget;
}

void ILPStrategy::applyConfigFromUI()
{
    m_Mu = m_MuSpin->value();
    m_Tau = m_TauSpin->value();
    m_Beta = m_BetaSpin->value();
    m_Gamma = m_GammaSpin->value();
    m_MST = m_MSTSpin->value();
    m_extraMachineCoefficient = m_extraMachineCoefficientSpin->value();
    m_maximumRequestsInPM = m_maximumRequestsInPMSpin->value();
}

QString ILPStrategy::name() const
{
    return "ILP Strategy";
}