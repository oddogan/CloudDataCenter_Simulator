#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "IPlacementStrategy.h"

class ILPStrategy : public IPlacementStrategy
{
public:
    ILPStrategy();
    ~ILPStrategy() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    std::vector<PhysicalMachine *> m_chosenMachines;
    std::vector<PhysicalMachine *> m_turnedOffMachines;
    size_t m_chosenMachineCount;
    void ChooseMachines(std::vector<PhysicalMachine> &machines, const std::vector<VirtualMachine *> &requests, const std::vector<VirtualMachine *> &migrations);
    double CalculatePowerOnCost(PhysicalMachine &machine);

    double m_migrationCost;
    double m_Tau;
    double m_extraMachineCoefficient;
    int m_maximumRequestsInPM;

    QDoubleSpinBox *m_migrationCostSpin;
    QDoubleSpinBox *m_TauSpin;
    QDoubleSpinBox *m_extraMachineCoefficientSpin;
    QSpinBox *m_maximumRequestsInPMSpin;

    QWidget *m_configWidget;
};