#pragma once

#include "IPlacementStrategy.h"

class AlphaBetaStrategy : public IPlacementStrategy
{
public:
    AlphaBetaStrategy();
    ~AlphaBetaStrategy() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QWidget *createStatusWidget(QWidget *parent = nullptr) override;
    QString name() const override;

private:
    double m_alpha;
    double m_beta;

    QDoubleSpinBox *m_alphaSpin;
    QDoubleSpinBox *m_betaSpin;
    QWidget *m_configWidget{nullptr};
    QWidget *m_statusWidget{nullptr};
};