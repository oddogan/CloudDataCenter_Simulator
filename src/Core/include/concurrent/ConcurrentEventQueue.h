#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "events/IEvent.h"

#include <iostream>

struct EventComparator
{
    bool operator()(const std::shared_ptr<IEvent> &lhs, const std::shared_ptr<IEvent> &rhs) const
    {
        return lhs->getTime() >= rhs->getTime();
    }
};

class ConcurrentEventQueue
{
public:
    ConcurrentEventQueue() : m_terminate(false), m_pushCount(0), m_popCount(0) {}

    // Producer: push a new event
    void push(std::shared_ptr<IEvent> event)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(event);
            m_pushCount++;
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
        m_popCount++;

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

    // Returns the number of events in the queue
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    // Returns the number of events popped from the queue
    size_t poppedCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_popCount;
    }

    // Returns the number of events pushed to the queue
    size_t pushedCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pushCount;
    }

    // Remove events from the queue based on a predicate using move semantics
    void remove(std::function<bool(const std::shared_ptr<IEvent> &)> predicate)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::priority_queue<std::shared_ptr<IEvent>, std::vector<std::shared_ptr<IEvent>>, EventComparator> newQueue;
        while (!m_queue.empty())
        {
            auto evt = m_queue.top();
            m_queue.pop();
            if (!predicate(evt))
            {
                newQueue.push(evt);
            }
        }
        m_queue = std::move(newQueue);
    }

private:
    std::priority_queue<std::shared_ptr<IEvent>, std::vector<std::shared_ptr<IEvent>>, EventComparator> m_queue;
    bool m_terminate;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    size_t m_pushCount;
    size_t m_popCount;
};