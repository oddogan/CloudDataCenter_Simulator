#pragma once

#include <atomic>
#include <thread>
#include "DataCenter.h"
#include "concurrent/ConcurrentEventQueue.h"

class SimulationEngine
{
public:
    SimulationEngine(DataCenter &dc, ConcurrentEventQueue &queue);
    ~SimulationEngine();

    void start();
    void stop();

    double currentTime() const { return m_currentTime; }

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