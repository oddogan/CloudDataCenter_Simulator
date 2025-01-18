#pragma once

#include <memory>
#include "IEvent.h"
#include "data/VirtualMachine.h"

class VMRequestEvent : public IEvent
{
public:
    VMRequestEvent(double time, std::unique_ptr<VirtualMachine> vm)
        : m_time(time), m_vm(std::move(vm))
    {
    }

    double getTime() const override { return m_time; }

    void accept(DataCenter &dataCenter, SimulationEngine &engine) override;

    // Move the VM out so DataCenter can own it
    std::unique_ptr<VirtualMachine> takeVM();

private:
    double m_time;
    std::unique_ptr<VirtualMachine> m_vm;
};