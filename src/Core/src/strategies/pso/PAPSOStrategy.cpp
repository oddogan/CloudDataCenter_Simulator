#include "strategies/pso/PAPSOStrategy.h"
#include "logging/LogManager.h"
#include "pso-cpp/psocpp.h"

struct PAPSOObjective
{
    // these will be set before PSO runs
    static const std::vector<VirtualMachine *> *vmsPtr;
    static const std::vector<PhysicalMachine> *pmsPtr;
    static double w1, w2;
    static double utilThreshold;

    PAPSOObjective() = default;

    template <typename Derived>
    double operator()(const Eigen::MatrixBase<Derived> &x) const
    {
        const auto &vms = *vmsPtr;
        const auto &pms = *pmsPtr;
        const int numPMs = int(pms.size());
        const int numVMs = int(vms.size());

        std::vector<Resources> loads(numPMs);
        std::vector<Resources> totals(numPMs);
        for (int i = 0; i < (int)pms.size(); ++i)
        {
            loads[i] = pms[i].getUsed();
            totals[i] = pms[i].getTotal();
        }

        size_t numTurnedOnInitially = 0;
        for (auto &pm : pms)
        {
            if (pm.isTurnedOn())
                numTurnedOnInitially++;
        }

        for (int i = 0; i < numVMs; ++i)
        {
            int pm = int(std::round(x(i)));
            pm = std::clamp(pm, 0, numPMs - 1);

            loads[pm] += vms[i]->getUsage();
        }

        // count active vs overloaded
        int activeCount = 0, overloadedCount = 0;
        for (int pm = 0; pm < numPMs; ++pm)
        {
            bool isActive = loads[pm].cpu > 0.0;
            Resources utilization = loads[pm] / pms[pm].getTotal();
            bool isOverloaded = (utilization.cpu > utilThreshold || utilization.ram > utilThreshold || utilization.disk > utilThreshold || utilization.bandwidth > utilThreshold || utilization.fpga > utilThreshold);
            if (isActive)
                activeCount++;
            if (isOverloaded)
                overloadedCount++;
        }

        double fracActive = double(activeCount) / numPMs;
        double fracOverload = double(overloadedCount) / numPMs;

        double result = w1 * fracActive + w2 * fracOverload;

        for (int pm = 0; pm < (int)pms.size(); ++pm)
        {
            const double penalty = 1e10;
            if (loads[pm].cpu > totals[pm].cpu)
            {
                result += (penalty * (loads[pm].cpu - totals[pm].cpu) / totals[pm].cpu);
            }
            if (loads[pm].ram > totals[pm].ram)
            {
                result += (penalty * (loads[pm].ram - totals[pm].ram) / totals[pm].ram);
            }
            if (loads[pm].disk > totals[pm].disk)
            {
                result += (penalty * (loads[pm].disk - totals[pm].disk) / totals[pm].disk);
            }
            if (loads[pm].bandwidth > totals[pm].bandwidth)
            {
                result += (penalty * (loads[pm].bandwidth - totals[pm].bandwidth) / totals[pm].bandwidth);
            }
            if (loads[pm].fpga > totals[pm].fpga)
            {
                result += (penalty * (loads[pm].fpga - totals[pm].fpga) / totals[pm].fpga);
            }
        }

        size_t newMachines = activeCount - numTurnedOnInitially;
        result += newMachines * 1e5;

        return result;
    }
};

// Definition of static members
const std::vector<VirtualMachine *> *PAPSOObjective::vmsPtr = nullptr;
const std::vector<PhysicalMachine> *PAPSOObjective::pmsPtr = nullptr;
double PAPSOObjective::w1 = 0.5;
double PAPSOObjective::w2 = 0.5;
double PAPSOObjective::utilThreshold = 0.5;

PAPSOStrategy::PAPSOStrategy(double w1, double w2,
                             int swarmSize, int maxIterations,
                             double inertiaMin, double inertiaMax,
                             double c1, double c2)
    : m_w1(w1), m_w2(w2),
      m_swarmSize(swarmSize), m_maxIterations(maxIterations),
      m_inertiaMin(inertiaMin), m_inertiaMax(inertiaMax),
      m_c1(c1), m_c2(c2)
{
}

Results PAPSOStrategy::run(const std::vector<VirtualMachine *> &newRequests,
                           const std::vector<VirtualMachine *> &toMigrate,
                           const std::vector<PhysicalMachine> &machines)
{
    Results result;

    // 1) Combine all VMs
    std::vector<VirtualMachine *> allVMs;
    allVMs.reserve(newRequests.size() + toMigrate.size());
    allVMs.insert(allVMs.end(), newRequests.begin(), newRequests.end());
    allVMs.insert(allVMs.end(), toMigrate.begin(), toMigrate.end());

    const int numVMs = int(allVMs.size());
    const int numPMs = int(machines.size());
    if (numVMs == 0 || numPMs == 0)
        return result;

    // 2) Prepare the static pointers & weights for the functor
    PAPSOObjective::vmsPtr = &allVMs;
    PAPSOObjective::pmsPtr = &machines;
    PAPSOObjective::w1 = m_w1;
    PAPSOObjective::w2 = m_w2;
    PAPSOObjective::utilThreshold = m_utilThreshold;

    // 3) Build bounds: each of the numVMs dims in [0, numPMs-1]
    Eigen::MatrixXd bounds(2, numVMs);
    bounds.row(0).setZero();               // lower = 0
    bounds.row(1).setConstant(numPMs - 1); // upper = numPMs-1

    // 4) Instantiate PSO optimizer
    //    Template: <T, Functor, InertiaStrategy>
    using Inertia = pso::LinearDecrease<double>;
    pso::ParticleSwarmOptimization<double, PAPSOObjective, Inertia> opt;

    // 5) Configure PSO stopping criteria & performance
    opt.setMaxIterations(m_maxIterations);
    opt.setThreads(1);   // single-threaded
    opt.setVerbosity(0); // silent
    opt.setPhiParticles(m_c1);
    opt.setPhiGlobal(m_c2);
    opt.setInertiaWeightStrategy(pso::LinearDecrease<double>(m_inertiaMin, m_inertiaMax));
    opt.setMinFunctionChange(-1);
    opt.setMinParticleChange(-1);

    // 6) Run PSO
    auto psoResult = opt.minimize(bounds, m_swarmSize);

    // 7) Decode best solution: round each dim to int PM index
    std::vector<int> assignment(numVMs);
    for (int i = 0; i < numVMs; ++i)
    {
        int pm = int(std::round(psoResult.xval(i)));
        assignment[i] = std::clamp(pm, 0, numPMs - 1);
    }

    // 8) Fill Results
    for (size_t i = 0; i < newRequests.size(); ++i)
    {
        result.placementDecision.push_back({newRequests[i], assignment[i]});
    }
    for (size_t j = 0; j < toMigrate.size(); ++j)
    {
        result.migrationDecision.push_back({toMigrate[j],
                                            assignment[int(newRequests.size()) + int(j)]});
    }

    return result;
}

double PAPSOStrategy::getMigrationThreshold()
{
    return m_utilThreshold;
}

size_t PAPSOStrategy::getBundleSize()
{
    return 20;
}

QWidget *PAPSOStrategy::createConfigWidget(QWidget *parent)
{
    return m_configWidget;
}

void PAPSOStrategy::applyConfigFromUI()
{
}

QString PAPSOStrategy::name() const
{
    return "PAPSO";
}