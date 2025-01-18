#pragma once

#include "IEvent.h"

class VMDepartureEvent : public IEvent
{
public:
    VMDepartureEvent(double time, int vmId)
        : m_time(time), m_vmId(vmId)
    {
    }

    double getTime() const override { return m_time; }
    void accept(DataCenter &dc, SimulationEngine &engine) override;

    int getVmId() const { return m_vmId; }

private:
    double m_time;
    int m_vmId;
};