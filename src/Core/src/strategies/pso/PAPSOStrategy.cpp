#include "strategies/pso/PAPSOStrategy.h"

PAPSOStrategy::PAPSOStrategy(int swarmSize, int maxIters,
                             double wMin, double wMax,
                             double c1, double c2,
                             double wActive, double wOverload,
                             double utilThresh)
    : m_swarmSize(swarmSize), m_maxIters(maxIters), m_wMin(wMin), m_wMax(wMax), m_c1(c1), m_c2(c2), m_wActive(wActive), m_wOverload(wOverload), m_utilThreshold(utilThresh), m_rng(std::random_device{}()), m_uniDist(0.0, 1.0)
{
}

Results PAPSOStrategy::run(const std::vector<VirtualMachine *> &newRequests,
                           const std::vector<VirtualMachine *> &toMigrate,
                           const std::vector<PhysicalMachine> &machines)
{
}

double PAPSOStrategy::getMigrationThreshold()
{
}

size_t PAPSOStrategy::getBundleSize()
{
}

QWidget *PAPSOStrategy::createConfigWidget(QWidget *parent)
{
}

void PAPSOStrategy::applyConfigFromUI()
{
}

QString PAPSOStrategy::name() const
{
    return "PAPSO";
}
