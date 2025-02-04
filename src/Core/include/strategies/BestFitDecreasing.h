#pragma once
#include "IPlacementStrategy.h"

class BestFitDecreasing : public IPlacementStrategy
{
public:
    BestFitDecreasing();
    ~BestFitDecreasing() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    QWidget *m_configWidget;
};