#pragma once

#include "IEvent.h"
#include "data/Resources.h"

class VMUtilUpdateEvent : public IEvent
{
public:
    VMUtilUpdateEvent(double time, int vmId, double utilization)
        : m_time(time), m_vmId(vmId), m_utilization(utilization)
    {
    }

    double getTime() const override { return m_time; }
    void accept(DataCenter &dc, SimulationEngine &engine) override;

    int getVmId() const { return m_vmId; }
    double getUtilization() const { return m_utilization; }

private:
    double m_time;
    int m_vmId;
    double m_utilization;
};