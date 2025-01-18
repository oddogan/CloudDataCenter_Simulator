#pragma once

#include <string>
#include "IEvent.h"

class ReconfigureStrategyEvent : public IEvent
{
public:
    ReconfigureStrategyEvent(double time, const std::string &name)
        : m_time(time), m_stratName(name)
    {
    }

    double getTime() const override { return m_time; }
    void accept(DataCenter &dc, SimulationEngine &engine) override;

    const std::string &getStrategyName() const { return m_stratName; }

private:
    double m_time;
    std::string m_stratName;
};