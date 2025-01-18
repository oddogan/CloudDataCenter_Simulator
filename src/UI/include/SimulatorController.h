#pragma once

#include <QObject>
#include <QTimer>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>

#include "DataCenter.h"
#include "SimulationEngine.h"
#include "TraceReader.h"
#include "concurrent/ConcurrentEventQueue.h"

// This class manages:
// 1) The DataCenter
// 2) The SimulationEngine
// 3) The Parser Thread
// And provides thread-safe access to usage data for the UI
class SimulatorController : public QObject
{
    Q_OBJECT

public:
    SimulatorController(QObject *parent = nullptr);
    ~SimulatorController();

    // Load trace & create data center, engine, parser
    void initialize(const QString &traceFile);

    // Start / stop parser + simulation
    void startSimulation();
    void stopSimulation();

    // Real-time usage snapshot
    // returns (time, ResourceUtilizations)
    std::tuple<double, Resources> getUsageSnapshot();

    // Set bundling at runtime
    void setBundleSize(int bSize);

    // Set strategy at runtime
    void setStrategy(IConfigurableStrategy *strategy);

    // Current simulation time
    double currentSimTime();

    // Possibly push events from GUI
    void pushEvent(const std::shared_ptr<IEvent> &evt);

signals:
    void simulationFinished();

private:
    // Owned objects
    DataCenter *m_dataCenter;
    ConcurrentEventQueue *m_eventQueue;
    SimulationEngine *m_engine;
    TraceReader *m_trace;

    // Because we can re-init multiple times
    bool m_initialized;
    std::atomic<bool> m_stop;
    std::mutex m_mutex; // protect data center usage

    std::thread m_parserThread; // if we do manual thread for parser
};