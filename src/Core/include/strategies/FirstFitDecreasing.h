#pragma once
#include "IPlacementStrategy.h"

class FirstFitDecreasing : public IPlacementStrategy
{
public:
    std::vector<PlacementDecision> run(const std::vector<VirtualMachine *> &vms, const std::vector<PhysicalMachine> &machines) override;
};