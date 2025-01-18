#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <memory>
#include "concurrent/ConcurrentEventQueue.h"

class TraceReader
{
public:
    TraceReader(const std::string &filename, ConcurrentEventQueue &queue);
    ~TraceReader();

    void start();
    void stop();

private:
    void parsingLoop();

    std::string m_filename;
    ConcurrentEventQueue &m_queue;
    std::thread m_thread;
    std::atomic<bool> m_stop;
};