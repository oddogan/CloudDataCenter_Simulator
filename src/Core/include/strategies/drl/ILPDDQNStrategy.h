#pragma once

#include "../IPlacementStrategy.h"
#include "DDQNAgent.h"
#include <ilcplex/ilocplex.h>
#include <QWidget>

class DataCenter;

class ILPDDQNStrategy : public IPlacementStrategy
{
public:
    ILPDDQNStrategy();
    ~ILPDDQNStrategy() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;

    void setDataCenter(DataCenter *dc) { m_dataCenter = dc; }
    void updateAgent();

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QString name() const override;

private:
    DataCenter *m_dataCenter;
    std::vector<double> ComputeState();
    std::vector<double> m_lastState;
    int m_lastActionIdx;
    double m_lastReward;
    bool m_lastFeasibility;

    std::vector<std::tuple<double, double, double, double, double>> m_actions;

    // DQN agent
    DDQNAgent *m_agent;
    double m_gap;

    // ILP
    std::vector<PhysicalMachine *> m_chosenMachines;
    std::vector<PhysicalMachine *> m_turnedOffMachines;
    size_t m_chosenMachineCount;
    void ChooseMachines(std::vector<PhysicalMachine> &machines, const std::vector<VirtualMachine *> &requests, const std::vector<VirtualMachine *> &migrations);
    double CalculatePowerOnCost(PhysicalMachine &machine);

    double m_Mu;    // Migration cost
    double m_Tau;   // Target Utilization After Migration
    double m_Beta;  // Expected Utilization Scaler for New Requests
    double m_Gamma; // Expected Utilization Scaler for Migrations
    double m_MST;   // Migration Start Threshold
    double m_extraMachineCoefficient;
    int m_maximumRequestsInPM;

    QWidget *m_configWidget{nullptr};
};