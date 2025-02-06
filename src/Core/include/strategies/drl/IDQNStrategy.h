#pragma once

#include "../ILPStrategy.h"
#include "IDQNAgent.h"
#include <ilcplex/ilocplex.h>
#include <QWidget>

class DataCenter;

class IDQNStrategy : public ILPStrategy
{
public:
    IDQNStrategy();
    virtual ~IDQNStrategy() override;

    virtual Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;

    void setDataCenter(DataCenter *dc) { m_dataCenter = dc; }
    void updateAgent();

    virtual QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    virtual void applyConfigFromUI() override;

protected:
    DataCenter *m_dataCenter;
    std::vector<double> ComputeState();
    std::vector<double> m_lastState;
    int m_lastActionIdx;
    double m_lastReward;
    bool m_lastFeasibility;
    size_t m_stateSize{20};

    std::vector<std::tuple<double, double, double, double, double>> m_actions;

    // DQN agent
    IDQNAgent *m_agent;
    double m_gap;

    QWidget *m_configWidget{nullptr};
};