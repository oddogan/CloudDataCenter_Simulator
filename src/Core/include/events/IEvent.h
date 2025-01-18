#pragma once

class DataCenter;
class SimulationEngine;

class IEvent
{
public:
    virtual ~IEvent() = default;

    // Return the time of this event
    virtual double getTime() const = 0;

    // The double-dispatch entry point
    virtual void accept(DataCenter &dataCenter, SimulationEngine &engine) = 0;
};