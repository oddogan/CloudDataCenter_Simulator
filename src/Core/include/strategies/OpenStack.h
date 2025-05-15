#pragma once

#include "IPlacementStrategy.h"
#include <vector>

class OpenStack : public IPlacementStrategy
{
public:
    OpenStack();
    ~OpenStack() override;

    Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) override;
    double getMigrationThreshold() override;
    size_t getBundleSize() override;

    QWidget *createConfigWidget(QWidget *parent = nullptr) override;
    void applyConfigFromUI() override;
    QWidget *createStatusWidget(QWidget *parent = nullptr) override;
    QString name() const override;

private:
    double m_ial{0.8};

    QWidget *m_configWidget{nullptr};
    QWidget *m_statusWidget{nullptr};
    QDoubleSpinBox *m_ialSpin{nullptr};
};