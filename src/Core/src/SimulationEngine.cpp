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
            std::cerr << "[SimulationEngine] Event from the past: " << t << " < " << m_currentTime << std::endl;
            throw std::runtime_error("Event from the past");
            continue;
        }

        m_currentTime = t;

        // Single-thread event processing
        evt->accept(m_dataCenter, *this);
    }
    std::cout << "[SimulationEngine] Thread stopped.\n";
}