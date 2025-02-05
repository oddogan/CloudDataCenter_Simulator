#pragma once

#include <atomic>
#include <thread>
#include "DataCenter.h"
#include "StatisticsRecorder.h"
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
    void ConnectStatisticsRecorder(StatisticsRecorder &recorder);

    double currentTime() const { return m_currentTime; }

    // ISimulationStatus
    double getCurrentTime() const override { return m_currentTime; }
    bool isRunning() const override { return !m_stop; }
    size_t getEventCount() const override { return m_queue.pushedCount(); }
    size_t getProcessedEventCount() const override { return m_queue.poppedCount(); }
    size_t getRemainingEventCount() const override { return m_queue.size(); }
    size_t getMachineCount() const override { return m_dataCenter.getPhysicalMachines().size(); }
    size_t getTurnedOnMachineCount() const override { return m_dataCenter.getTurnedOnMachineCount(); }
    std::vector<MachineUsageInfo> getMachineUsageInfo() const override { return m_dataCenter.getMachineUsageInfo(); }
    std::string getCurrentStrategy() const override { return m_dataCenter.getPlacementStrategy()->name().toStdString(); }
    size_t getCurrentBundleSize() const override { return m_dataCenter.getBundleSize(); }
    ResourceUtilizations getResourceUtilizations() const override;
    double getAveragePowerConsumption() const override { return m_dataCenter.getAveragePowerConsumption(); }
    double getTotalPowerConsumption() const override { return m_dataCenter.getTotalPowerConsumption(); }

    // ISimulationConfiguration
    void setBundleSize(size_t size) override { m_dataCenter.setBundleSize(size); }
    void setPlacementStrategy(IPlacementStrategy *strategy) override { m_dataCenter.setPlacementStrategy(strategy); }
    size_t getBundleSize() const override { return m_dataCenter.getBundleSize(); }
    IPlacementStrategy *getPlacementStrategy() const override { return m_dataCenter.getPlacementStrategy(); }
    void setOutputFile(const std::string &filename) override;

    // In case we want external producers to push
    void pushEvent(const std::shared_ptr<IEvent> &evt);
    void removeEvents(std::function<bool(const std::shared_ptr<IEvent> &)> predicate);

private:
    void runLoop();

    DataCenter &m_dataCenter;
    ConcurrentEventQueue &m_queue;
    StatisticsRecorder *m_recorder;

    std::atomic<bool> m_stop;
    std::thread m_thread;
    double m_currentTime;
};