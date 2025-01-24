#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>
#include "data/PhysicalMachine.h"
#include "data/VirtualMachine.h"
#include "events/VMRequestEvent.h"
#include "events/VMUtilUpdateEvent.h"
#include "events/VMDepartureEvent.h"
#include "events/MigrationCompleteEvent.h"
#include "strategies/StrategyFactory.h"
#include "logging/LogManager.h"

class SimulationEngine;

class DataCenter
{
public:
    DataCenter();
    ~DataCenter();

    // Handle each event type
    void handle(const VMRequestEvent &event, SimulationEngine &engine);
    void handle(const VMUtilUpdateEvent &event, SimulationEngine &engine);
    void handle(const VMDepartureEvent &event, SimulationEngine &engine);
    void handle(const MigrationCompleteEvent &event, SimulationEngine &engine);

    void addPhysicalMachine(const PhysicalMachine &pm);

    void setPlacementStrategy(IPlacementStrategy *strategy);
    IPlacementStrategy *getPlacementStrategy() const;

    // For bundling
    void setBundleSize(size_t size);
    size_t getBundleSize() const { return m_bundleSize; }

    // Thread-safe updates
    bool updateVM(int vmId, double utilization);
    bool removeVM(int vmId);

    const std::vector<PhysicalMachine> &getPhysicalMachines() const { return m_physicalMachines; }
    std::vector<MachineUsageInfo> getMachineUsageInfo() const;
    Resources getResourceUtilizations() const;
    size_t getTurnedOnMachineCount() const;

private:
    void runPlacement(SimulationEngine &engine);
    void scheduleMigration(SimulationEngine &engine, int vmID, int new_pmID, unsigned int numberOfMigrations);
    bool detectOvercommitment(int pmId, SimulationEngine &engine);
    double computeMigrationTime(VirtualMachine *vm, unsigned int numberOfMigrations) const;

    void placeVMonPM(VirtualMachine *vm, int pmId, SimulationEngine &engine);

    mutable std::mutex m_strategyMutex;
    IPlacementStrategy *m_strategy;

    // Bundling
    size_t m_bundleSize;
    mutable std::mutex m_bundleMutex;
    std::vector<VirtualMachine *> m_pendingNewRequests;
    std::vector<VirtualMachine *> m_pendingMigrations;

    // Physical machines
    std::vector<PhysicalMachine> m_physicalMachines;

    // VM index
    std::unordered_map<int, std::pair<int, VirtualMachine *>> m_vmIndex;
    mutable std::mutex m_vmIndexMutex;
};