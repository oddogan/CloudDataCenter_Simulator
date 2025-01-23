#pragma once

#include <atomic>
#include <thread>
#include "DataCenter.h"
#include "concurrent/ConcurrentEventQueue.h"
#include "ISimulationStatus.h"
#include "ISimulationConfiguration.h"

class SimulationEngine : public ISimulationStatus, public ISimulationConfiguration
{
public:
    SimulationEngine(DataCenter &dc, ConcurrentEventQueue &queue);
    ~SimulationEngine();

    void start();
    void stop();

    double currentTime() const { return m_currentTime; }

    // ISimulationStatus
    double getCurrentTime() const override { return m_currentTime; }
    bool isRunning() const override { return !m_stop; }
    size_t getEventCount() const override { return m_queue.pushedCount(); }
    size_t getProcessedEventCount() const override { return m_queue.poppedCount(); }
    size_t getRemainingEventCount() const override { return m_queue.size(); }
    size_t getMachineCount() const override { return m_dataCenter.getPhysicalMachines().size(); }
    std::vector<MachineUsageInfo> getMachineUsageInfo() const override { return m_dataCenter.getMachineUsageInfo(); }
    std::string getCurrentStrategy() const override { return m_dataCenter.getPlacementStrategy()->name().toStdString(); }
    size_t getCurrentBundleSize() const override { return m_dataCenter.getBundleSize(); }
    ResourceUtilizations getResourceUtilizations() const override;

    // ISimulationConfiguration
    void setBundleSize(size_t size) override { m_dataCenter.setBundleSize(size); }
    void setPlacementStrategy(IPlacementStrategy *strategy) override { m_dataCenter.setPlacementStrategy(strategy); }
    size_t getBundleSize() const override { return m_dataCenter.getBundleSize(); }
    IPlacementStrategy *getPlacementStrategy() const override { return m_dataCenter.getPlacementStrategy(); }

    // In case we want external producers to push
    void pushEvent(const std::shared_ptr<IEvent> &evt);

private:
    void runLoop();

    DataCenter &m_dataCenter;
    ConcurrentEventQueue &m_queue;

    std::atomic<bool> m_stop;
    std::thread m_thread;
    double m_currentTime;
};