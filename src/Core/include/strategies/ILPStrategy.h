#pragma once
#include "IPlacementStrategy.h"

class ILPStrategy : public IPlacementStrategy
{
public:
    ILPStrategy();
    ~ILPStrategy() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    virtual QWidget *createStatusWidget(QWidget *parent = nullptr) override;
    QString name() const override;

protected:
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

    double m_lastCost;
    bool m_lastFeasibility;

    size_t m_bundleSize;

    QDoubleSpinBox *m_MuSpin{nullptr};
    QDoubleSpinBox *m_TauSpin{nullptr};
    QDoubleSpinBox *m_BetaSpin{nullptr};
    QDoubleSpinBox *m_GammaSpin{nullptr};
    QDoubleSpinBox *m_MSTSpin{nullptr};
    QDoubleSpinBox *m_extraMachineCoefficientSpin{nullptr};
    QSpinBox *m_maximumRequestsInPMSpin{nullptr};
    QWidget *m_configWidget{nullptr};
    QWidget *m_statusWidget{nullptr};
};