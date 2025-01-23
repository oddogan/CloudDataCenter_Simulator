#pragma once

#include <cstddef>

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
};