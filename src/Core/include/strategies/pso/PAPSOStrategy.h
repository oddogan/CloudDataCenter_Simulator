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
    PAPSOStrategy(double w1 = 0.5, double w2 = 0.5,
                  int swarmSize = 60, int maxIterations = 100,
                  double inertiaMin = 0.4, double inertiaMax = 0.9,
                  double c1 = 2.05, double c2 = 2.05,
                  double maxVelocity = 10);

    virtual Results run(const std::vector<VirtualMachine *> &newRequests,
                        const std::vector<VirtualMachine *> &toMigrate,
                        const std::vector<PhysicalMachine> &machines) override;

    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QWidget *createStatusWidget(QWidget *parent = nullptr) override;
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
    double m_maxVelocity;

    double m_utilThreshold{0.8};

    std::mt19937 m_rng;
    std::uniform_real_distribution<double> m_uniDist;

    double randomDouble(double lo, double hi)
    {
        return std::uniform_real_distribution<double>(lo, hi)(m_rng);
    }

    // GUI
    QWidget *m_configWidget{nullptr};
    QWidget *m_statusWidget{nullptr};
    QDoubleSpinBox *m_w1Spin{nullptr};
    QDoubleSpinBox *m_w2Spin{nullptr};
    QSpinBox *m_swarmSizeSpin{nullptr};
    QSpinBox *m_maxIterationsSpin{nullptr};
    QDoubleSpinBox *m_inertiaMinSpin{nullptr};
    QDoubleSpinBox *m_inertiaMaxSpin{nullptr};
    QDoubleSpinBox *m_c1Spin{nullptr};
    QDoubleSpinBox *m_c2Spin{nullptr};
    QDoubleSpinBox *m_utilThresholdSpin{nullptr};
    QDoubleSpinBox *m_maxVelocitySpin{nullptr};
};