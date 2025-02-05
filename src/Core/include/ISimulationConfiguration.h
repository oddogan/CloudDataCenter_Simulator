#pragma once

#include <cstddef>
#include <string>
#include "strategies/IPlacementStrategy.h"

class ISimulationConfiguration
{
public:
    virtual ~ISimulationConfiguration() = default;

    virtual void setBundleSize(size_t size) = 0;
    virtual void setPlacementStrategy(IPlacementStrategy *strategy) = 0;
    virtual void setOutputFile(const std::string &filename) = 0;

    virtual size_t getBundleSize() const = 0;
    virtual IPlacementStrategy *getPlacementStrategy() const = 0;
};