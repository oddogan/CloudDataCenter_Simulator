#include "SimulatorController.h"
#include <QDebug>
#include <chrono>
#include <thread>

#include "strategies/StrategyFactory.h"
#include "data/Resources.h"

SimulatorController::SimulatorController(QObject *parent)
    : QObject(parent), m_dataCenter(nullptr), m_eventQueue(nullptr), m_engine(nullptr), m_trace(nullptr), m_initialized(false), m_stop(false)
{
}

SimulatorController::~SimulatorController()
{
    stopSimulation(); // ensure threads are stopped

    // Cleanup
    if (m_trace)
    {
        m_trace->stop();
        delete m_trace;
        m_trace = nullptr;
    }
    if (m_engine)
    {
        m_engine->stop();
        delete m_engine;
        m_engine = nullptr;
    }
    if (m_dataCenter)
    {
        delete m_dataCenter;
        m_dataCenter = nullptr;
    }
    if (m_eventQueue)
    {
        delete m_eventQueue;
        m_eventQueue = nullptr;
    }
}

void SimulatorController::initialize(const QString &traceFile)
{
    stopSimulation(); // in case we are re-initializing

    if (m_dataCenter)
        delete m_dataCenter;
    if (m_eventQueue)
        delete m_eventQueue;
    if (m_engine)
        delete m_engine;
    if (m_trace)
    {
        m_trace->stop();
        delete m_trace;
    }

    m_stop = false;

    // Create an initial strategy
    IConfigurableStrategy *initStrat = StrategyFactory::create("FirstFitDecreasing");
    m_dataCenter = new DataCenter(initStrat, 3);

    // Add some PMs
    for (int i = 0; i < 500; ++i)
    {
        m_dataCenter->addPhysicalMachine(PhysicalMachine(i, Resources(48, 1536, 35000, 50000, 40000)));
    }

    // Create event queue
    m_eventQueue = new ConcurrentEventQueue();

    // Create simulation engine
    m_engine = new SimulationEngine(*m_dataCenter, *m_eventQueue);

    // Create trace reader
    m_trace = new TraceReader(traceFile.toStdString(), *m_eventQueue);

    m_initialized = true;
}

void SimulatorController::startSimulation()
{
    if (!m_initialized)
    {
        qWarning() << "Simulator not initialized. Call initialize() first.";
        return;
    }

    // Start parser
    m_trace->start();

    // Start simulation
    m_engine->start();
}

void SimulatorController::stopSimulation()
{
    m_stop = true;
    if (m_trace)
    {
        m_trace->stop();
    }
    if (m_engine)
    {
        m_engine->stop();
    }
}

std::tuple<double, Resources> SimulatorController::getUsageSnapshot()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_dataCenter)
        return {currentSimTime(), Resources()};

    Resources util;
    int count = 0;

    auto &pms = m_dataCenter->getPhysicalMachines();
    for (auto &pm : pms)
    {
        if (pm.isTurnedOn())
        {
            util += pm.getUtilization();
            count++;
        }
    }
    double simTime = currentSimTime();
    util /= std::max(1, count);
    return {simTime, util};
}

void SimulatorController::setBundleSize(int bSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_dataCenter)
    {
        m_dataCenter->setBundleSize(bSize);
    }
}

void SimulatorController::setStrategy(IConfigurableStrategy *strategy)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_dataCenter)
    {
        m_dataCenter->setPlacementStrategy(strategy);
    }
}

double SimulatorController::currentSimTime()
{
    if (!m_engine)
        return 0.0;
    return m_engine->currentTime();
}

void SimulatorController::pushEvent(const std::shared_ptr<IEvent> &evt)
{
    if (m_eventQueue)
    {
        m_eventQueue->push(evt);
    }
}