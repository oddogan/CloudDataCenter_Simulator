#pragma once

#include "../IPlacementStrategy.h"
#include <vector>
#include <random>
#include <limits>
#include <algorithm>
#include <Eigen/Dense>

class PAPSOStrategy : public IPlacementStrategy
{
public:
    PAPSOStrategy(double w1 = 0.4, double w2 = 0.9,
                  int swarmSize = 60, int maxIterations = 100,
                  double inertiaMin = 0.4, double inertiaMax = 0.9,
                  double c1 = 2.05, double c2 = 2.05);

    virtual Results run(const std::vector<VirtualMachine *> &newRequests,
                        const std::vector<VirtualMachine *> &toMigrate,
                        const std::vector<PhysicalMachine> &machines) override;

    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    // Internal representation of a PSO particle
    struct Particle
    {
        std::vector<int> pos;     // Current VM->PM index mapping
        std::vector<double> vel;  // PSO velocity for each VM dimension
        std::vector<int> bestPos; // Personal best mapping seen so far
        double bestVal;           // Personal best fitness value
    };

    double m_w1, m_w2;
    int m_swarmSize;
    int m_maxIterations;
    double m_inertiaMin, m_inertiaMax;
    double m_c1, m_c2;

    double m_utilThreshold{0.8};

    std::mt19937 m_rng;
    std::uniform_real_distribution<double> m_uniDist;

    double randomDouble(double lo, double hi)
    {
        return std::uniform_real_distribution<double>(lo, hi)(m_rng);
    }

    // GUI
    QWidget *m_configWidget{nullptr};
};