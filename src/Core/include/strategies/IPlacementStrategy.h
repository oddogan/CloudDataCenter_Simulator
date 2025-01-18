#pragma once

#include <vector>
#include "data/PhysicalMachine.h"
#include "data/VirtualMachine.h"

// TODO: Check it
struct PlacementDecision
{
    VirtualMachine *vm;
    int pmId;
};

class IPlacementStrategy
{
public:
    virtual ~IPlacementStrategy() = default;

    // Decide how to place a batch of VMs
    virtual std::vector<PlacementDecision> run(const std::vector<VirtualMachine *> &vms, const std::vector<PhysicalMachine> &pms) = 0;
};