#pragma once
#include "IPlacementStrategy.h"

class FirstFitDecreasing : public IPlacementStrategy
{
public:
    FirstFitDecreasing();
    ~FirstFitDecreasing() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QWidget *createStatusWidget(QWidget *parent = nullptr) override;
    QString name() const override;

private:
    QWidget *m_configWidget{nullptr};
    QWidget *m_statusWidget{nullptr};
};