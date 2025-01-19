#pragma once

#include <vector>
#include <QWidget>
#include <QString>
#include "data/PhysicalMachine.h"
#include "data/VirtualMachine.h"

// TODO: Check it
struct PlacementDecision
{
    VirtualMachine *vm;
    int pmId;
};

struct Results
{
    std::vector<PlacementDecision> placementDecision;
    std::vector<PlacementDecision> migrationDecision;
};

class IPlacementStrategy
{
public:
    virtual ~IPlacementStrategy() = default;

    // Decide how to place a batch of VMs
    virtual Results run(const std::vector<VirtualMachine *> &newRequests, const std::vector<VirtualMachine *> &toMigrate, const std::vector<PhysicalMachine> &machines) = 0;

    // If you want a config widget:

    // Returns a QWidget (or a layout) containing input fields for user configuration
    // The DataCenter or GUI can embed this widget in a config panel.
    virtual QWidget *createConfigWidget(QWidget *parent = nullptr) = 0;

    // Called by the GUI once the user changes parameters in the config widget
    // Letting the strategy update its internal fields
    virtual void applyConfigFromUI() = 0;

    // Return a human-readable name for the strategy
    virtual QString name() const = 0;
};