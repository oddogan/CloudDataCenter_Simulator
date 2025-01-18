#pragma once
#include "IConfigurableStrategy.h"

class BestFitDecreasing : public IConfigurableStrategy
{
public:
    BestFitDecreasing();
    ~BestFitDecreasing() override;

    std::vector<PlacementDecision> run(const std::vector<VirtualMachine *> &vms, const std::vector<PhysicalMachine> &machines) override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    QWidget *m_configWidget;
};