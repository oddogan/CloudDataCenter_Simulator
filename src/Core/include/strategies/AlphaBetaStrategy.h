#pragma once

#include "IConfigurableStrategy.h"
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>

class AlphaBetaStrategy : public IConfigurableStrategy
{
public:
    AlphaBetaStrategy();
    ~AlphaBetaStrategy() override;

    std::vector<PlacementDecision> run(
        const std::vector<VirtualMachine *> &vms,
        const std::vector<PhysicalMachine> &pms) override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    double m_alpha;
    double m_beta;

    QDoubleSpinBox *m_alphaSpin;
    QDoubleSpinBox *m_betaSpin;
    QWidget *m_configWidget;
};