#pragma once

#include <vector>
#include <mutex>
#include "VirtualMachine.h"
#include "Resources.h"

struct MachineUsageInfo
{
    int machineId;
    Resources used;
    Resources total;
};

class PhysicalMachine
{
public:
    PhysicalMachine(unsigned int id, const Resources &totalResources)
        : m_ID(id), m_totalResources(totalResources), m_usedResources(0, 0, 0, 0, 0)
    {
    }

    int getID() const { return m_ID; }

    bool canHost(const Resources &request) const
    {
        return ::canHost(request, m_totalResources - m_usedResources);
    }

    void allocate(const Resources &request)
    {
        m_usedResources += request;
    }
    void free(const Resources &request)
    {
        m_usedResources -= request;
    }

    void turnOff() { m_turnedOn = false; }
    void turnOn() { m_turnedOn = true; }
    bool isTurnedOn() const { return m_turnedOn; }

    Resources getFreeResources() const
    {
        return m_totalResources - m_usedResources;
    }

    Resources getTotal() const { return m_totalResources; }
    Resources getUsed() const { return m_usedResources; }
    Resources getUtilization() const
    {
        if (m_totalResources == Resources(0, 0, 0, 0, 0))
            return Resources(0, 0, 0, 0, 0);
        return m_usedResources / m_totalResources * 100;
    }
    bool isOvercommitted() const
    {
        auto free = getFreeResources();
        return (free.cpu < 0 || free.ram < 0 || free.disk < 0 || free.bandwidth < 0 || free.fpga < 0);
    }

    void addVM(VirtualMachine *vm)
    {
        if (!isTurnedOn())
            turnOn();

        m_virtualMachines.push_back(vm);
        allocate(vm->getUsage());
    }

    void removeVM(int vmID)
    {
        auto it = std::find_if(m_virtualMachines.begin(), m_virtualMachines.end(), [vmID](VirtualMachine *vm)
                               { return vm->getID() == vmID; });
        if (it != m_virtualMachines.end())
        {
            free((*it)->getUsage());
            m_virtualMachines.erase(it);
        }

        if (m_virtualMachines.empty())
            turnOff();
    }

    VirtualMachine *findVM(int vmID) const
    {
        for (auto *vm : m_virtualMachines)
        {
            if (vm->getID() == vmID)
                return vm;
        }
        return nullptr;
    }

    const std::vector<VirtualMachine *> &getVirtualMachines() const { return m_virtualMachines; }

    MachineUsageInfo getUsageInfo() const
    {
        return {m_ID, m_usedResources, m_totalResources};
    }

private:
    int m_ID;
    bool m_turnedOn{false};
    Resources m_totalResources;
    Resources m_usedResources;

    std::vector<VirtualMachine *> m_virtualMachines;
};