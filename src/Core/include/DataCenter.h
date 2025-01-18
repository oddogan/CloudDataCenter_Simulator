#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>
#include "data/PhysicalMachine.h"
#include "data/VirtualMachine.h"
#include "events/VMRequestEvent.h"
#include "events/VMUtilUpdateEvent.h"
#include "events/VMDepartureEvent.h"
#include "strategies/IConfigurableStrategy.h"

class SimulationEngine;

class DataCenter
{
public:
    DataCenter(IConfigurableStrategy *strategy, size_t bundleSize = 1);
    ~DataCenter();

    // Handle each event type
    void handle(const VMRequestEvent &event, SimulationEngine &engine);
    void handle(const VMUtilUpdateEvent &event, SimulationEngine &engine);
    void handle(const VMDepartureEvent &event, SimulationEngine &engine);

    void addPhysicalMachine(const PhysicalMachine &pm);

    // For final usage
    void printUsageSummary() const;

    // Called by the simulation or externally
    void setPlacementStrategy(IConfigurableStrategy *strategy);

    // For bundling
    void setBundleSize(size_t size);
    void placeBundledVMs(SimulationEngine &engine);

    // Thread-safe updates
    bool updateVM(int vmId, double utilization);
    bool removeVM(int vmId);

    const std::vector<PhysicalMachine> &getPhysicalMachines() const { return m_physicalMachines; }

private:
    void placeVMonPM(VirtualMachine *vm, int pmId, SimulationEngine &engine);
    PhysicalMachine *findPM(int pmID) const;

    mutable std::mutex m_strategyMutex;
    IConfigurableStrategy *m_strategy;

    // Bundling
    size_t m_bundleSize;
    std::vector<VirtualMachine *> m_bundle;
    mutable std::mutex m_bundleMutex;

    // Physical machines
    std::vector<PhysicalMachine> m_physicalMachines;

    // VM index
    std::unordered_map<int, std::pair<int, VirtualMachine *>> m_vmIndex;
    mutable std::mutex m_vmIndexMutex;
};