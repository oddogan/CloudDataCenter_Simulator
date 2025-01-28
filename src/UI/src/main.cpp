#include <QApplication>
#include "Core/include/logging/LogManager.h"
#include "Core/include/DataCenter.h"
#include "Core/include/SimulationEngine.h"
#include "Core/include/TraceReader.h"
#include "Core/include/concurrent/ConcurrentEventQueue.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Create the event queue
    ConcurrentEventQueue queue;

    // Start the trace reader
    TraceReader reader(queue);

    // Start the data center
    DataCenter dc;

    // Start the simulation engine
    SimulationEngine engine(dc, queue);

    // TODO: Set up the data center

    // Add the physical machines to the data center
    for (int i = 0; i < 500; ++i)
    {
        dc.addPhysicalMachine(PhysicalMachine(i, Resources(40, 1024, 24576, 40000, 100), 10, 10, 1));
    }

    // Run in CLI mode if arguments are provided
    if (argc > 1)
    {
        // Read the trace file
        reader.readTraceFile(argv[1]);

        // Start the simulation engine
        engine.start();

        // Wait for all traces to be processed
        while (reader.isRunning())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Stop the simulation engine
        engine.stop();
    }
    else
    {
        // Start the Qt application
        QApplication a(argc, argv);
        MainWindow w(reader, engine);
        w.show();
        return a.exec();
    }
}