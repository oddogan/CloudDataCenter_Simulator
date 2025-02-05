#include "DataCenter.h"
#include "SimulationEngine.h"
#include "strategies/FirstFitDecreasing.h"
#include "strategies/BestFitDecreasing.h"
#include <algorithm>
#include <iostream>
#include "strategies/drl/ILPDQNStrategy.h"

DataCenter::DataCenter()
    : m_strategy(nullptr)
{
    // Default strategy
    setPlacementStrategy(StrategyFactory::create("ILPStrategy"));
    setBundleSize(10);
}

DataCenter::~DataCenter()
{
    // Clean up strategy
    if (m_strategy)
    {
        delete m_strategy;
        m_strategy = nullptr;
    }

    // Also free any leftover VMs in pending or m_vmIndex
    for (auto *vm : m_pendingNewRequests)
    {
        delete vm;
    }
    m_pendingNewRequests.clear();

    for (auto &kv : m_vmIndex)
    {
        // kv.second is (pmId, vmPtr)
        delete kv.second.second; // the VM
    }
    m_vmIndex.clear();
}

void DataCenter::setBundleSize(size_t newSize)
{
    std::lock_guard<std::mutex> lock(m_bundleMutex);
    m_bundleSize = newSize;
}

void DataCenter::setPlacementStrategy(IPlacementStrategy *strat)
{
    std::lock_guard<std::mutex> lock(m_strategyMutex);

    if (m_strategy)
        delete m_strategy;

    m_strategy = strat;
}

IPlacementStrategy *DataCenter::getPlacementStrategy() const
{
    std::lock_guard<std::mutex> lock(m_strategyMutex);
    return m_strategy;
}

void DataCenter::addPhysicalMachine(const PhysicalMachine &pm)
{
    m_physicalMachines.push_back(pm);
}

void DataCenter::handle(const VMRequestEvent &event, SimulationEngine &engine)
{
    auto vm = const_cast<VMRequestEvent &>(event).takeVM();
    VirtualMachine *rawVm = vm.release();

    // push to pending
    {
        std::lock_guard<std::mutex> lock(m_bundleMutex);
        m_NewRequestCountSinceLastPlacement++;
        m_pendingNewRequests.push_back(rawVm);
        if (m_pendingNewRequests.size() >= m_bundleSize)
        {
            runPlacement(engine);
        }
    }
}

void DataCenter::handle(const VMUtilUpdateEvent &event, SimulationEngine &engine)
{
    updateVM(event.getVmId(), event.getUtilization());

    if (detectOvercommitment(m_vmIndex[event.getVmId()].first, engine))
    {
        runPlacement(engine);
    }
}

void DataCenter::handle(const VMDepartureEvent &event, SimulationEngine &engine)
{
    // If VM was migrating at the time
    auto vm = m_vmIndex[event.getVmId()].second;
    if (vm->isMigrating())
    {
        // End the migrations in both old and new PMs
        int oldPmId = vm->getOldPMID();
        auto &oldPM = m_physicalMachines[oldPmId];
        oldPM.endMigration();
        oldPM.removeVM(event.getVmId());
        auto &newPM = m_physicalMachines[m_vmIndex[event.getVmId()].first];
        newPM.endMigration();

        LogManager::instance().log(LogCategory::VM_MIGRATION, "VM " + std::to_string(event.getVmId()) + " migration cancelled");
    }

    removeVM(event.getVmId());

    LogManager::instance().log(LogCategory::VM_DEPARTURE, "VM " + std::to_string(event.getVmId()) + " departed");
}

void DataCenter::handle(const MigrationCompleteEvent &event, SimulationEngine &engine)
{
    int vmId = event.getVmId();
    int oldPmId = event.getOldPmId();

    auto vm = m_vmIndex[vmId].second;
    if (!vm)
    {
        LogManager::instance().log(LogCategory::VM_MIGRATION, "VM " + std::to_string(vmId) + " departed before its migration completion");
        // throw std::runtime_error("VM not found for migration completion");
        return;
    }

    vm->setMigrating(false);

    auto &oldPM = m_physicalMachines[oldPmId];
    oldPM.endMigration();
    oldPM.removeVM(vmId);

    // End the migrations in both old and new PMs
    auto &newPM = m_physicalMachines[event.getNewPmId()];
    newPM.endMigration();

    m_MigrationCountSinceLastPlacement++;

    LogManager::instance().log(LogCategory::VM_MIGRATION, "VM " + std::to_string(vmId) + " migrated from PM " + std::to_string(oldPmId) + " to PM " + std::to_string(event.getNewPmId()));
}

void DataCenter::runPlacement(SimulationEngine &engine)
{
    if (!m_strategy)
        return;

    Results decisions;

    std::lock_guard<std::mutex> lock(m_strategyMutex);

    if (auto ilpdqn = dynamic_cast<ILPDQNStrategy *>(m_strategy))
    {
        ilpdqn->setDataCenter(this);
    }

    decisions = m_strategy->run(m_pendingNewRequests, m_pendingMigrations, m_physicalMachines);

    m_pendingNewRequests.clear();
    m_pendingMigrations.clear();

    m_SLAVcountSinceLastPlacement = 0;
    m_MigrationCountSinceLastPlacement = 0;
    m_NewRequestCountSinceLastPlacement = 0;

    // Handle new requests
    for (auto &pd : decisions.placementDecision)
    {
        if (pd.pmId < 0)
        {
            LogManager::instance().log(LogCategory::PLACEMENT, "No fit for VM " + std::to_string(pd.vm->getID()));
            throw std::runtime_error("No fit for VM " + std::to_string(pd.vm->getID()));
        }
        else
        {
            LogManager::instance().log(LogCategory::PLACEMENT, "VM " + std::to_string(pd.vm->getID()) + " placed on PM " + std::to_string(pd.pmId));
            placeVMonPM(pd.vm, pd.pmId, engine);
        }
    }

    // Handle migrations
    unsigned int numberOfMigrations = decisions.migrationDecision.size();
    for (auto &pd : decisions.migrationDecision)
    {
        if (pd.pmId < 0)
        {
            LogManager::instance().log(LogCategory::VM_MIGRATION, "No migration fit for VM " + std::to_string(pd.vm->getID()));
            // throw std::runtime_error("No migration fit for VM");
        }
        else if (pd.pmId == m_vmIndex[pd.vm->getID()].first)
        {
            LogManager::instance().log(LogCategory::VM_MIGRATION, "VM " + std::to_string(pd.vm->getID()) + " already on PM " + std::to_string(pd.pmId));
        }
        else
        {
            LogManager::instance().log(LogCategory::VM_MIGRATION, "VM " + std::to_string(pd.vm->getID()) + " migrating from PM " + std::to_string(m_vmIndex[pd.vm->getID()].first) + " to PM " + std::to_string(pd.pmId));
            scheduleMigration(engine, pd.vm->getID(), pd.pmId, numberOfMigrations);
        }
    }

    if (auto ilpdqn = dynamic_cast<ILPDQNStrategy *>(m_strategy))
    {
        ilpdqn->updateAgent();
    }
}

void DataCenter::scheduleMigration(SimulationEngine &engine, int vmID, int new_pmID, unsigned int numberOfMigrations)
{
    int old_pmID = m_vmIndex[vmID].first;
    if (old_pmID == new_pmID)
    {
        return;
    }

    auto vm = m_vmIndex[vmID].second;
    if (!vm)
    {
        std::cerr << "[WARN] VM " << vmID << " not found" << std::endl;
        throw std::runtime_error("VM not found for migration scheduling");
    }

    vm->setMigrating(true);

    auto &newPM = m_physicalMachines[new_pmID];
    newPM.addVM(vm);
    // update index
    {
        std::lock_guard<std::mutex> lock(m_vmIndexMutex);
        m_vmIndex[vmID].first = new_pmID;
    }

    // Start migrations on both old and new PM
    newPM.startMigration();
    auto &oldPM = m_physicalMachines[old_pmID];
    oldPM.startMigration();

    // create migration event
    double dT = computeMigrationTime(vm, numberOfMigrations);
    double t = engine.currentTime() + dT;
    auto evt = std::make_shared<MigrationCompleteEvent>(t, vmID, old_pmID, new_pmID);
    engine.pushEvent(evt);
}

bool DataCenter::detectOvercommitment(int pmId, SimulationEngine &engine)
{
    double MSThreshold = 0.8;
    {
        std::lock_guard<std::mutex> lock(m_strategyMutex);
        MSThreshold = m_strategy->getMigrationThreshold();
    }

    if (m_physicalMachines[pmId].isOvercommitted(MSThreshold))
    {
        if (m_physicalMachines[pmId].isMigrating())
        {
            return false; // already in migration
        }

        m_SLAVcount++;
        m_SLAVcountSinceLastPlacement++;

        const std::vector<VirtualMachine *> &vmsOnPM = m_physicalMachines[pmId].getVirtualMachines();

        // We do a while loop if we want
        for (auto *vm : vmsOnPM)
        {
            if (vm->isMigrating())
            {
                continue; // skip already in migration
            }

            m_pendingMigrations.push_back(vm);
        }

        return true;
    }
    return false;
}

double DataCenter::computeMigrationTime(VirtualMachine *vm, unsigned int numberOfMigrations) const
{
    auto usage = vm->getUsage();
    return (usage.disk / (usage.bandwidth / (1000 * numberOfMigrations)));
}

bool DataCenter::updateVM(int vmId, double utilization)
{
    // find pm
    std::lock_guard<std::mutex> lock(m_vmIndexMutex);
    auto it = m_vmIndex.find(vmId);
    if (it == m_vmIndex.end())
    {
        throw std::runtime_error("VM " + std::to_string(vmId) + " not found in updateVM");
    }
    int pmId = it->second.first;
    VirtualMachine *vmPtr = it->second.second;

    Resources oldUsage = vmPtr->getUsage();
    vmPtr->setUtilization(utilization);

    PhysicalMachine &pm = m_physicalMachines[pmId];
    pm.free(oldUsage);
    LogManager::instance().log(LogCategory::VM_UTIL_UPDATE, "VM " + std::to_string(vmId) + " updated on PM " + std::to_string(pmId) + " - new usage: " + "0" + " - available: " + "0"); // TODO: add actual values
    pm.allocate(vmPtr->getUsage());

    if (vmPtr->isMigrating())
    {
        // migration is in progress
        // we need to update the old PM as well
        int oldPMID = vmPtr->getOldPMID();
        PhysicalMachine &oldPM = m_physicalMachines[oldPMID];
        oldPM.free(oldUsage);
        oldPM.allocate(vmPtr->getUsage());
    }

    return true;
}

bool DataCenter::removeVM(int vmId)
{
    std::lock_guard<std::mutex> lock1(m_vmIndexMutex);
    auto it = m_vmIndex.find(vmId);
    if (it == m_vmIndex.end())
    {
        return false;
    }
    int pmId = it->second.first;
    VirtualMachine *vmPtr = it->second.second;

    PhysicalMachine &pm = m_physicalMachines[pmId];
    pm.removeVM(vmId);

    m_vmIndex.erase(it);
    delete vmPtr;
    return true;
}

std::vector<MachineUsageInfo> DataCenter::getMachineUsageInfo() const
{
    std::vector<MachineUsageInfo> result;
    for (const auto &pm : m_physicalMachines)
    {
        MachineUsageInfo info;
        info.machineId = pm.getID();
        info.total = pm.getTotal();
        info.used = pm.getUsed();
        result.push_back(info);
    }
    return result;
}

Resources DataCenter::getResourceUtilizations() const
{
    Resources result;

    // Result will have the used/total ratio for turned on machines
    Resources total;
    Resources used;
    size_t count = 0;
    for (const auto &pm : m_physicalMachines)
    {
        if (pm.isTurnedOn())
        {
            total += pm.getTotal();
            used += pm.getUsed();
            count++;
        }
    }

    if (count > 0)
    {
        result = used / total * 100.0;
    }

    return result;
}

size_t DataCenter::getTurnedOnMachineCount() const
{
    size_t count = 0;
    for (const auto &pm : m_physicalMachines)
    {
        if (pm.isTurnedOn())
        {
            count++;
        }
    }
    return count;
}

double DataCenter::getAveragePowerConsumption() const
{
    double total = 0.0;
    size_t count = 0;
    for (const auto &pm : m_physicalMachines)
    {
        if (pm.isTurnedOn())
        {
            total += pm.getPowerConsumption();
            count++;
        }
    }

    if (count > 0)
    {
        return total / count;
    }

    return 0.0;
}

double DataCenter::getTotalPowerConsumption() const
{
    double total = 0.0;
    for (const auto &pm : m_physicalMachines)
    {
        if (pm.isTurnedOn())
        {
            total += pm.getPowerConsumption();
        }
    }
    return total;
}

size_t DataCenter::getNumberOfSLAViolations() const
{
    return m_SLAVcount;
}

void DataCenter::placeVMonPM(VirtualMachine *vm, int pmId, SimulationEngine &engine)
{
    Resources usage = vm->getUsage();
    PhysicalMachine &pm = m_physicalMachines[pmId];
    if (!pm.canHost(usage))
    {
        std::runtime_error("PM " + std::to_string(pm.getID()) + "cannot host VM" + std::to_string(vm->getID()));
    }
    pm.addVM(vm);

    // insert in vmIndex
    {
        std::lock_guard<std::mutex> lock(m_vmIndexMutex);
        m_vmIndex[vm->getID()] = {pmId, vm};
    }
    vm->setPlaced(true);

    // schedule usage updates
    vm->setStartTime(engine.currentTime());
    for (auto &u : vm->getFutureUtilizations())
    {
        double t = engine.currentTime() + u.offset;
        // create VMUtilUpdateEvent
        auto evt = std::make_shared<VMUtilUpdateEvent>(t, vm->getID(), u.utilization);
        engine.pushEvent(evt);
    }

    double departureTime = vm->getStartTime() + vm->getDuration();
    auto devt = std::make_shared<VMDepartureEvent>(departureTime, vm->getID());
    engine.pushEvent(devt);
}