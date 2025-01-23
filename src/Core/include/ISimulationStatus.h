#pragma once

#include <cstddef>
#include <vector>
#include "data/PhysicalMachine.h"

struct ResourceUtilizations
{
    double time;
    Resources utilizations;
};

class ISimulationStatus
{
public:
    virtual ~ISimulationStatus() = default;

    virtual double getCurrentTime() const = 0;
    virtual bool isRunning() const = 0;
    virtual size_t getEventCount() const = 0;
    virtual size_t getProcessedEventCount() const = 0;
    virtual size_t getRemainingEventCount() const = 0;
    virtual size_t getMachineCount() const = 0;
    virtual std::vector<MachineUsageInfo> getMachineUsageInfo() const = 0;
    virtual std::string getCurrentStrategy() const = 0;
    virtual size_t getCurrentBundleSize() const = 0;
    virtual ResourceUtilizations getResourceUtilizations() const = 0;
};