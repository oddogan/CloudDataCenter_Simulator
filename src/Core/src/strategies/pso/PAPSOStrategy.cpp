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

            loads[pm] += vms[i]->getTotalRequestedResources();
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

        /*
        for (int pm = 0; pm < (int)pms.size(); ++pm)
        {
            const double penalty = 0;
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
        result += newMachines * 0;
        */

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
                             double c1, double c2,
                             double maxVelocity)
    : m_w1(w1), m_w2(w2),
      m_swarmSize(swarmSize), m_maxIterations(maxIterations),
      m_inertiaMin(inertiaMin), m_inertiaMax(inertiaMax),
      m_c1(c1), m_c2(c2),
      m_maxVelocity(maxVelocity)
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
    opt.setThreads(0);   // single-threaded
    opt.setVerbosity(0); // silent
    opt.setPhiParticles(m_c1);
    opt.setPhiGlobal(m_c2);
    opt.setInertiaWeightStrategy(pso::LinearDecrease<double>(m_inertiaMin, m_inertiaMax));
    opt.setMinFunctionChange(-1);
    opt.setMinParticleChange(-1);
    opt.setMaxVelocity(m_maxVelocity);

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
    return 10;
}

QWidget *PAPSOStrategy::createConfigWidget(QWidget *parent)
{
    if (!m_configWidget)
    {
        m_configWidget = new QWidget(parent);
        auto layout = new QFormLayout(m_configWidget);

        m_w1Spin = new QDoubleSpinBox(m_configWidget);
        m_w1Spin->setRange(0.0, 1.0);
        m_w1Spin->setValue(m_w1);
        layout->addRow("W1", m_w1Spin);

        m_w2Spin = new QDoubleSpinBox(m_configWidget);
        m_w2Spin->setRange(0.0, 1.0);
        m_w2Spin->setValue(m_w2);
        layout->addRow("W2", m_w2Spin);

        m_swarmSizeSpin = new QSpinBox(m_configWidget);
        m_swarmSizeSpin->setRange(1, 1000);
        m_swarmSizeSpin->setValue(m_swarmSize);
        layout->addRow("Swarm Size", m_swarmSizeSpin);

        m_maxIterationsSpin = new QSpinBox(m_configWidget);
        m_maxIterationsSpin->setRange(1, 1000);
        m_maxIterationsSpin->setValue(m_maxIterations);
        layout->addRow("Max Iterations", m_maxIterationsSpin);

        m_inertiaMinSpin = new QDoubleSpinBox(m_configWidget);
        m_inertiaMinSpin->setRange(0.0, 1.0);
        m_inertiaMinSpin->setValue(m_inertiaMin);
        layout->addRow("Inertia Min", m_inertiaMinSpin);

        m_inertiaMaxSpin = new QDoubleSpinBox(m_configWidget);
        m_inertiaMaxSpin->setRange(0.0, 1.0);
        m_inertiaMaxSpin->setValue(m_inertiaMax);
        layout->addRow("Inertia Max", m_inertiaMaxSpin);

        m_c1Spin = new QDoubleSpinBox(m_configWidget);
        m_c1Spin->setRange(0.0, 10.0);
        m_c1Spin->setValue(m_c1);
        layout->addRow("Personal Bias", m_c1Spin);

        m_c2Spin = new QDoubleSpinBox(m_configWidget);
        m_c2Spin->setRange(0.0, 10.0);
        m_c2Spin->setValue(m_c2);
        layout->addRow("Social Bias", m_c2Spin);

        m_utilThresholdSpin = new QDoubleSpinBox(m_configWidget);
        m_utilThresholdSpin->setRange(0.0, 1.0);
        m_utilThresholdSpin->setValue(m_utilThreshold);
        layout->addRow("Utilization Threshold", m_utilThresholdSpin);

        m_maxVelocitySpin = new QDoubleSpinBox(m_configWidget);
        m_maxVelocitySpin->setRange(0.0, 100.0);
        m_maxVelocitySpin->setValue(m_maxVelocity);
        layout->addRow("Max Velocity", m_maxVelocitySpin);

        m_configWidget->setLayout(layout);
    }
    return m_configWidget;
}

void PAPSOStrategy::applyConfigFromUI()
{
    if (m_w1Spin && m_w2Spin && m_swarmSizeSpin && m_maxIterationsSpin &&
        m_inertiaMinSpin && m_inertiaMaxSpin && m_c1Spin && m_c2Spin &&
        m_utilThresholdSpin && m_maxVelocitySpin)
    {
        m_w1 = m_w1Spin->value();
        m_w2 = m_w2Spin->value();
        m_swarmSize = m_swarmSizeSpin->value();
        m_maxIterations = m_maxIterationsSpin->value();
        m_inertiaMin = m_inertiaMinSpin->value();
        m_inertiaMax = m_inertiaMaxSpin->value();
        m_c1 = m_c1Spin->value();
        m_c2 = m_c2Spin->value();
        m_utilThreshold = m_utilThresholdSpin->value();
        m_maxVelocity = m_maxVelocitySpin->value();
    }
}

QString PAPSOStrategy::name() const
{
    return "PAPSO";
}

QWidget *PAPSOStrategy::createStatusWidget(QWidget *parent)
{
    if (!m_statusWidget)
    {
        m_statusWidget = new QWidget(parent);
        auto layout = new QFormLayout(m_statusWidget);

        auto w1Label = new QLabel("W1: " + QString::number(m_w1), m_statusWidget);
        layout->addRow(w1Label);

        auto w2Label = new QLabel("W2: " + QString::number(m_w2), m_statusWidget);
        layout->addRow(w2Label);

        auto swarmSizeLabel = new QLabel("Swarm Size: " + QString::number(m_swarmSize), m_statusWidget);
        layout->addRow(swarmSizeLabel);

        auto maxIterationsLabel = new QLabel("Max Iterations: " + QString::number(m_maxIterations), m_statusWidget);
        layout->addRow(maxIterationsLabel);

        auto inertiaMinLabel = new QLabel("Inertia Min: " + QString::number(m_inertiaMin), m_statusWidget);
        layout->addRow(inertiaMinLabel);

        auto inertiaMaxLabel = new QLabel("Inertia Max: " + QString::number(m_inertiaMax), m_statusWidget);
        layout->addRow(inertiaMaxLabel);

        auto c1Label = new QLabel("Personal Bias (C1): " + QString::number(m_c1), m_statusWidget);
        layout->addRow(c1Label);

        auto c2Label = new QLabel("Social Bias (C2): " + QString::number(m_c2), m_statusWidget);
        layout->addRow(c2Label);

        auto utilThresholdLabel = new QLabel("Utilization Threshold: " + QString::number(m_utilThreshold), m_statusWidget);
        layout->addRow(utilThresholdLabel);

        auto maxVelocityLabel = new QLabel("Max Velocity: " + QString::number(m_maxVelocity), m_statusWidget);
        layout->addRow(maxVelocityLabel);

        m_statusWidget->setLayout(layout);
    }
    return m_statusWidget;
}