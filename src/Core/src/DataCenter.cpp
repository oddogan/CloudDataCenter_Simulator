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
    for (auto *vm : m_bundle)
    {
        delete vm;
    }
    m_bundle.clear();

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
    if (m_strategy)
    {
        delete m_strategy;
    }
    m_strategy = strat;
}

void DataCenter::addPhysicalMachine(const PhysicalMachine &pm)
{
    m_physicalMachines.push_back(pm);
}

void DataCenter::handle(const VMRequestEvent &evt, SimulationEngine &engine)
{
    std::unique_ptr<VirtualMachine> uptr = const_cast<VMRequestEvent &>(evt).takeVM();
    VirtualMachine *rawVm = uptr.release();

    // push to pending
    {
        std::lock_guard<std::mutex> lock(m_bundleMutex);
        m_bundle.push_back(rawVm);
        if (m_bundle.size() >= m_bundleSize)
        {
            placeBundledVMs(engine);
        }
    }
}

void DataCenter::handle(const VMUtilUpdateEvent &evt, SimulationEngine &engine)
{
    updateVM(evt.getVmId(), evt.getUtilization());
}

void DataCenter::handle(const VMDepartureEvent &evt, SimulationEngine &engine)
{
    removeVM(evt.getVmId());
}

void DataCenter::handle(const ReconfigureStrategyEvent &evt, SimulationEngine &engine)
{
    std::string name = evt.getStrategyName();
    if (name == "FirstFitDecreasing")
    {
        setPlacementStrategy(new FirstFitDecreasing());
    }
    else if (name == "BestFitDecreasing")
    {
        setPlacementStrategy(new BestFitDecreasing());
    }
    else
    {
        std::cerr << "[DataCenter] Unknown strategy " << name
                  << ", fallback to FirstFit.\n";
        setPlacementStrategy(new FirstFitDecreasing());
    }
    std::cout << "[DataCenter] Strategy reconfigured to " << name << std::endl;
}

void DataCenter::placeBundledVMs(SimulationEngine &engine)
{
    if (!m_strategy)
        return;

    // local copy of pending
    std::vector<VirtualMachine *> copy;
    {
        if (m_bundle.empty())
            return;
        copy = m_bundle;
        m_bundle.clear();
    }

    auto decisions = m_strategy->run(copy, m_physicalMachines);

    // Apply them
    for (auto &dec : decisions)
    {
        if (dec.pmId < 0)
        {
            std::cerr << "[WARN] No fit for VM " << dec.vm->getID() << std::endl;
            throw std::runtime_error("No fit for VM");
        }
        else
        {
            std::cout << "[DataCenter] Placing VM " << dec.vm->getID() << " on PM " << dec.pmId << std::endl;
            placeVMonPM(dec.vm, dec.pmId, engine);
        }
    }
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
    std::cout << "[DataCenter] Updating VM " << vmId << " on PM " << pmId << " - new usage: " << vmPtr->getUsage() << " - available: " << pm->getFreeResources() << std::endl;
    if (!pm->canHost(vmPtr->getUsage()))
    {
        std::cerr << "[WARN] new usage doesn't fit on pm" << pmId << std::endl;
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