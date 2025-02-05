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
    PhysicalMachine(unsigned int id, const Resources &totalResources, double perCoreBasePowerConsumption, double powerConsumptionCPU, double powerConsumptionFPGA)
        : m_ID(id), m_totalResources(totalResources), m_usedResources(0, 0, 0, 0, 0), m_powerOnCost(perCoreBasePowerConsumption * totalResources.cpu), m_powerConsumptionCPU(powerConsumptionCPU), m_powerConsumptionFPGA(powerConsumptionFPGA)
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

    void turnOff()
    {
        if (isMigrating())
            throw std::runtime_error("Cannot turn off PM while migrating");

        m_turnedOn = false;
    }

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

    bool isOvercommitted(double threshold_normalized) const
    {
        auto threshold = threshold_normalized * 100;
        auto used = getUtilization();
        return used.cpu > threshold || used.ram > threshold || used.disk > threshold || used.bandwidth > threshold || used.fpga > threshold;
    }

    double getPowerOnCost() const { return m_powerOnCost; }
    double getPowerConsumptionCPU() const { return m_powerConsumptionCPU; }
    double getPowerConsumptionFPGA() const { return m_powerConsumptionFPGA; }

    double getPowerConsumption() const
    {
        if (!isTurnedOn())
            return 0.0;

        return m_powerOnCost + m_powerConsumptionCPU * m_usedResources.cpu + m_powerConsumptionFPGA * m_usedResources.fpga;
    }

    void addVM(VirtualMachine *vm)
    {
        if (!isTurnedOn())
            turnOn();

        m_virtualMachines.push_back(vm);
        allocate(vm->getUsage());
        vm->setPMID(m_ID);
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
        else
        {
            throw std::runtime_error("VM not found in removeVM");
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

    void startMigration()
    {
        if (!isTurnedOn())
            throw std::runtime_error("Cannot start migration on turned off PM");
        m_ongoingMigrationCount++;
    }
    void endMigration()
    {
        if (!isTurnedOn())
            throw std::runtime_error("Cannot end migration on turned off PM");
        m_ongoingMigrationCount--;
    }
    bool isMigrating() const { return m_ongoingMigrationCount > 0; }

private:
    int m_ID;
    bool m_turnedOn{false};
    Resources m_totalResources;
    Resources m_usedResources;
    double m_powerOnCost;
    double m_powerConsumptionCPU;
    double m_powerConsumptionFPGA;
    int m_ongoingMigrationCount{0};

    std::vector<VirtualMachine *> m_virtualMachines;
};