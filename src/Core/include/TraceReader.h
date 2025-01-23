#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <memory>
#include "concurrent/ConcurrentEventQueue.h"

struct TraceInfo
{
    std::string filename;
    std::thread parserThread;
};

class TraceReader
{
public:
    TraceReader(ConcurrentEventQueue &queue);
    ~TraceReader();

    void stop();
    void readTraceFile(const std::string &filename);
    bool isRunning() const { return m_traces.size() > 0; }

private:
    void parsingLoop(const std::string &filename);

    std::vector<TraceInfo> m_traces;
    ConcurrentEventQueue &m_queue;
    std::atomic<bool> m_stop;
};