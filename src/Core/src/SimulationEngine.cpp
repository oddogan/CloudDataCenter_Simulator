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
    m_recorder->flush();
}

void SimulationEngine::ConnectStatisticsRecorder(StatisticsRecorder &recorder)
{
    m_recorder = &recorder;
}

ResourceUtilizations SimulationEngine::getResourceUtilizations() const
{
    ResourceUtilizations result;
    result.utilizations = m_dataCenter.getResourceUtilizations();
    result.time = m_currentTime;
    return result;
}

void SimulationEngine::setOutputFile(const std::string &filename)
{
    m_recorder->setOutputFile(filename);
}

void SimulationEngine::pushEvent(const std::shared_ptr<IEvent> &evt)
{
    m_queue.push(evt);
}

void SimulationEngine::removeEvents(std::function<bool(const std::shared_ptr<IEvent> &)> predicate)
{
    m_queue.remove(predicate);
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
            LogManager::instance().log(LogCategory::WARNING, "Event from the past: " + std::to_string(t) + " < " + std::to_string(m_currentTime));
            throw std::runtime_error("Event from the past");
            continue;
        }

        m_currentTime = t;

        // Single-thread event processing
        evt->accept(m_dataCenter, *this);

        if (m_recorder)
        {
            m_recorder->recordStatistics();
        }
    }
    qDebug() << "SimulationEngine: Stopped";
}