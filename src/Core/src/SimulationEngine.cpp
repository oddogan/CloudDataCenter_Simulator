#include "SimulationEngine.h"
#include "events/IEvent.h"
#include <iostream>

SimulationEngine::SimulationEngine(DataCenter &dc, ConcurrentEventQueue &q)
    : m_dataCenter(dc), m_queue(q), m_stop(false), m_currentTime(0.0)
{
}

SimulationEngine::~SimulationEngine()
{
    stop();
}

void SimulationEngine::start()
{
    m_stop = false;
    m_thread = std::thread(&SimulationEngine::runLoop, this);
}

void SimulationEngine::stop()
{
    m_stop = true;
    m_queue.terminate();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

ResourceUtilizations SimulationEngine::getResourceUtilizations() const
{
    ResourceUtilizations result;
    result.utilizations = m_dataCenter.getResourceUtilizations();
    result.time = m_currentTime;
    return result;
}

void SimulationEngine::pushEvent(const std::shared_ptr<IEvent> &evt)
{
    m_queue.push(evt);
}

void SimulationEngine::runLoop()
{
    while (!m_stop)
    {
        std::shared_ptr<IEvent> evt;
        if (!m_queue.pop(evt))
        {
            // If queue is empty or terminated
            if (m_stop)
                break;
            continue;
        }
        double t = evt->getTime();

        if (t < m_currentTime)
        {
            LogManager::instance().log(LogCategory::DEBUG, "Event from the past: " + std::to_string(t) + " < " + std::to_string(m_currentTime));
            throw std::runtime_error("Event from the past");
            continue;
        }

        m_currentTime = t;

        // Single-thread event processing
        evt->accept(m_dataCenter, *this);
    }
    LogManager::instance().log(LogCategory::DEBUG, "SimulationEngine thread stopped");
}