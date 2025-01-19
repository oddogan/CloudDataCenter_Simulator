#include "DataCenter.h"
#include "SimulationEngine.h"
#include "strategies/FirstFitDecreasing.h"
#include "strategies/BestFitDecreasing.h"
#include <algorithm>
#include <iostream>

DataCenter::DataCenter(IPlacementStrategy *initStrat, size_t bundleSize)
    : m_strategy(initStrat), m_bundleSize(bundleSize)
{
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
    removeVM(event.getVmId());

    std::cout << "[DataCenter] VM " << event.getVmId() << " departed" << std::endl;
}

void DataCenter::handle(const MigrationCompleteEvent &event, SimulationEngine &engine)
{
    int vmId = event.getVmId();
    int oldPmId = event.getOldPmId();
    int newPmId = event.getNewPmId();

    auto vm = m_vmIndex[vmId].second;
    if (!vm)
    {
        std::cerr << "[WARN] VM " << vmId << " not found" << std::endl;
        // throw std::runtime_error("VM not found for migration completion");
        return;
    }

    auto &oldPM = m_physicalMachines[oldPmId];
    auto &newPM = m_physicalMachines[newPmId];

    oldPM.removeVM(vmId);
    newPM.addVM(vm);

    // update index
    {
        std::lock_guard<std::mutex> lock(m_vmIndexMutex);
        m_vmIndex[vmId].first = newPmId;
    }

    vm->setMigrating(false);

    if (detectOvercommitment(newPmId, engine))
    {
        runPlacement(engine);
    }
}

void DataCenter::runPlacement(SimulationEngine &engine)
{
    if (!m_strategy)
        return;

    Results decisions;
    {
        std::lock_guard<std::mutex> lock(m_strategyMutex);
        decisions = m_strategy->run(m_pendingNewRequests, m_pendingMigrations, m_physicalMachines);
    }

    m_pendingNewRequests.clear();
    m_pendingMigrations.clear();

    // Handle new requests
    for (auto &pd : decisions.placementDecision)
    {
        if (pd.pmId < 0)
        {
            std::cerr << "[WARN] No fit for VM " << pd.vm->getID() << std::endl;
            throw std::runtime_error("No fit for VM");
        }
        else
        {
            std::cout << "[DataCenter] Placing VM " << pd.vm->getID() << " on PM " << pd.pmId << std::endl;
            placeVMonPM(pd.vm, pd.pmId, engine);
        }
    }

    // Handle migrations
    for (auto &pd : decisions.migrationDecision)
    {
        if (pd.pmId < 0)
        {
            std::cerr << "[WARN] No migration fit for VM " << pd.vm->getID() << std::endl;
            throw std::runtime_error("No migration fit for VM");
        }
        else if (pd.pmId == m_vmIndex[pd.vm->getID()].first)
        {
            std::cerr << "[WARN] Migration to same PM for VM " << pd.vm->getID() << std::endl;
        }
        else
        {
            std::cout << "[DataCenter] Scheduling migration VM " << pd.vm->getID() << " from PM " << m_vmIndex[pd.vm->getID()].first << " to PM " << pd.pmId << std::endl;
            scheduleMigration(engine, pd.vm->getID(), pd.pmId);
        }
    }
}

void DataCenter::scheduleMigration(SimulationEngine &engine, int vmID, int new_pmID)
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

    // create migration event
    vm->setMigrating(true);
    double dT = computeMigrationTime(vm);
    double t = engine.currentTime() + dT;
    auto evt = std::make_shared<MigrationCompleteEvent>(t, vmID, old_pmID, new_pmID);
    engine.pushEvent(evt);
}

bool DataCenter::detectOvercommitment(int pmId, SimulationEngine &engine)
{
    auto pmIt = std::find_if(m_physicalMachines.begin(), m_physicalMachines.end(),
                             [pmId](auto &pm)
                             { return pm.getID() == pmId; });
    if (pmIt == m_physicalMachines.end())
    {
        return false; // unknown PM
    }

    if (pmIt->isOvercommitted())
    {
        // Overcommitted.
        // This is just an example policy:

        std::vector<VirtualMachine *> vmsOnPM = pmIt->getVirtualMachines();
        // suppose cloneVMList() gives a snapshot of pointers. Or we can store them ourselves.

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

double DataCenter::computeMigrationTime(VirtualMachine *vm) const
{
    auto usage = vm->getUsage();
    return (usage.disk / usage.bandwidth);
}

bool DataCenter::updateVM(int vmId, double utilization)
{
    // find pm
    std::lock_guard<std::mutex> lock(m_vmIndexMutex);
    auto it = m_vmIndex.find(vmId);
    if (it == m_vmIndex.end())
    {
        return false;
    }
    int pmId = it->second.first;
    VirtualMachine *vmPtr = it->second.second;

    Resources oldUsage = vmPtr->getUsage();
    vmPtr->setUtilization(utilization);

    PhysicalMachine *pm = findPM(pmId);
    pm->free(oldUsage);
    // std::cout << "[DataCenter] Updating VM " << vmId << " on PM " << pmId << " - new usage: " << vmPtr->getUsage() << " - available: " << pm->getFreeResources() << std::endl;
    if (!pm->canHost(vmPtr->getUsage()))
    {
        // std::cerr << "[WARN] new usage doesn't fit on pm" << pmId << std::endl;
    }
    pm->allocate(vmPtr->getUsage());
    return true;
}

bool DataCenter::removeVM(int vmId)
{
    std::lock_guard<std::mutex> lock(m_vmIndexMutex);
    auto it = m_vmIndex.find(vmId);
    if (it == m_vmIndex.end())
    {
        return false;
    }
    int pmId = it->second.first;
    VirtualMachine *vmPtr = it->second.second;

    // find pm
    PhysicalMachine *pm = findPM(pmId);
    pm->removeVM(vmId);

    m_vmIndex.erase(it);
    delete vmPtr;
    return true;
}

void DataCenter::placeVMonPM(VirtualMachine *vm, int pmId, SimulationEngine &engine)
{
    Resources usage = vm->getUsage();
    PhysicalMachine *pm = findPM(pmId);
    if (!pm->canHost(usage))
    {
        std::runtime_error("PM " + std::to_string(pm->getID()) + "cannot host VM" + std::to_string(vm->getID()));
    }
    pm->addVM(vm);

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

PhysicalMachine *DataCenter::findPM(int pmID) const
{
    auto it = std::find_if(m_physicalMachines.begin(), m_physicalMachines.end(),
                           [pmID](const PhysicalMachine &pm)
                           { return pm.getID() == pmID; });

    if (it == m_physicalMachines.end())
    {
        throw std::runtime_error("PM " + std::to_string(pmID) + "not found");
    }

    return const_cast<PhysicalMachine *>(&(*it));
}

void DataCenter::printUsageSummary() const
{
    for (auto &pm : m_physicalMachines)
    {
        auto freeRes = pm.getFreeResources();
        std::cout << "PM" << pm.getID() << ": freeCPU =" << freeRes.cpu
                  << " freeRAM =" << freeRes.ram
                  << " freeDisk =" << freeRes.disk
                  << " freeBW =" << freeRes.bandwidth
                  << " freeFPGA =" << freeRes.fpga
                  << std::endl;
    }
}