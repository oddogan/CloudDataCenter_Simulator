#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "events/IEvent.h"

struct EventComparator
{
    bool operator()(const std::shared_ptr<IEvent> &lhs, const std::shared_ptr<IEvent> &rhs) const
    {
        return lhs->getTime() > rhs->getTime();
    }
};

class ConcurrentEventQueue
{
public:
    ConcurrentEventQueue() : m_terminate(false) {}

    // Producer: push a new event
    void push(std::shared_ptr<IEvent> event)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(event);
        }
        m_cv.notify_one();
    }

    // Consumer: pop the earliest event (blocks if empty)
    // Returns false if terminated or queue is empty after terminate
    bool pop(std::shared_ptr<IEvent> &outEvent)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]
                  { return m_terminate || !m_queue.empty(); });

        if (m_terminate || m_queue.empty())
        {
            return false;
        }

        outEvent = m_queue.top();
        m_queue.pop();
        return true;
    }

    // Terminate the queue
    void terminate()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_terminate = true;
        }
        m_cv.notify_all();
    }

private:
    std::priority_queue<std::shared_ptr<IEvent>, std::vector<std::shared_ptr<IEvent>>, EventComparator> m_queue;
    bool m_terminate;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};