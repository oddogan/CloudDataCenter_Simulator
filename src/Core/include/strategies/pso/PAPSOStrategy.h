#pragma once

#include "../IPlacementStrategy.h"
#include <vector>
#include <random>
#include <limits>
#include <algorithm>

class PAPSOStrategy : public IPlacementStrategy
{
public:
    PAPSOStrategy(int swarmSize = 30,
                  int maxIters = 100,
                  double wMin = 0.4,
                  double wMax = 0.9,
                  double c1 = 1.5,
                  double c2 = 1.5,
                  double wActive = 0.7,
                  double wOverload = 0.3,
                  double utilThresh = 0.8);

    virtual Results run(const std::vector<VirtualMachine *> &newRequests,
                        const std::vector<VirtualMachine *> &toMigrate,
                        const std::vector<PhysicalMachine> &machines) override;

    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    struct Particle
    {
        std::vector<int> pos;     // VM -> PM index
        std::vector<double> vel;  // Velocity
        std::vector<int> bestPos; // Personal best
        double bestVal;           // Personal best value
    };

    int m_swarmSize;
    int m_maxIters;
    double m_wMin, m_wMax;
    double m_c1, m_c2;
    double m_wActive, m_wOverload;
    double m_utilThreshold;

    std::mt19937 m_rng;
    std::uniform_real_distribution<double> m_uniDist;

    double randomDouble(double lo, double hi)
    {
        return std::uniform_real_distribution<double>(lo, hi)(m_rng);
    }

    // fitness = wActive*(#active/total) + wOverload*(#overloaded/total)
    double fitness(const std::vector<VirtualMachine *> &vms,
                   const std::vector<PhysicalMachine> &pms,
                   const std::vector<int> &mapping);
};