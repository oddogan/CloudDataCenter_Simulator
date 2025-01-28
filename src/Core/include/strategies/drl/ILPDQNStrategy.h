#pragma once

#include "../IPlacementStrategy.h"
#include "DQNAgent.h"
#include <ilcplex/ilocplex.h>
#include <QWidget>

class ILPDQNStrategy : public IPlacementStrategy
{
public:
    ILPDQNStrategy();
    ~ILPDQNStrategy() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    // Discrete combos for (tau, migrationCost)
    static std::vector<std::pair<double, double>> s_actions;

    // DQN agent
    DQNAgent m_agent;
    double m_gap;

    // ILP
    std::vector<PhysicalMachine *> m_chosenMachines;
    std::vector<PhysicalMachine *> m_turnedOffMachines;
    size_t m_chosenMachineCount;
    void ChooseMachines(std::vector<PhysicalMachine> &machines, const std::vector<VirtualMachine *> &requests, const std::vector<VirtualMachine *> &migrations);
    double CalculatePowerOnCost(PhysicalMachine &machine);

    double m_migrationCost;
    double m_Tau;
    double m_extraMachineCoefficient;
    int m_maximumRequestsInPM;

    QWidget *m_configWidget;
};