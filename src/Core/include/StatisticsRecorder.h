#pragma once

#include <vector>
#include <fstream>

class SimulationEngine;

class StatisticsRecorder
{
public:
    StatisticsRecorder(SimulationEngine &engine);
    ~StatisticsRecorder();

    void recordStatistics();
    void setOutputFile(const std::string &filename);
    void flush();

private:
    SimulationEngine *m_engine;
    std::ofstream m_outputFile;
};