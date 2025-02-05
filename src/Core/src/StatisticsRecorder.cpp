#include "StatisticsRecorder.h"

#include "SimulationEngine.h"

StatisticsRecorder::StatisticsRecorder(SimulationEngine &engine) : m_engine(&engine)
{
}

StatisticsRecorder::~StatisticsRecorder()
{
}

void StatisticsRecorder::setOutputFile(const std::string &filename)
{
    m_outputFile.open(filename, std::ios::out | std::ios::binary);

    if (!m_outputFile.is_open())
    {
        throw std::runtime_error("Could not open file " + filename);
    }
}

void StatisticsRecorder::flush()
{
    if (m_outputFile.is_open())
    {
        m_outputFile.flush();
    }
}

void StatisticsRecorder::recordStatistics()
{
    if (!m_outputFile.is_open())
    {
        throw std::runtime_error("Output file not set");
    }

    auto utilizations = m_engine->getResourceUtilizations();

    m_outputFile.write(reinterpret_cast<char *>(&utilizations.time), sizeof(double));
    m_outputFile.write(reinterpret_cast<char *>(&utilizations.utilizations.cpu), sizeof(double));
    m_outputFile.write(reinterpret_cast<char *>(&utilizations.utilizations.ram), sizeof(double));
    m_outputFile.write(reinterpret_cast<char *>(&utilizations.utilizations.disk), sizeof(double));
    m_outputFile.write(reinterpret_cast<char *>(&utilizations.utilizations.bandwidth), sizeof(double));
    m_outputFile.write(reinterpret_cast<char *>(&utilizations.utilizations.fpga), sizeof(double));

    size_t turnedOnMachineCount = m_engine->getTurnedOnMachineCount();
    m_outputFile.write(reinterpret_cast<char *>(&turnedOnMachineCount), sizeof(size_t));

    double averagePowerConsumption = m_engine->getAveragePowerConsumption();
    m_outputFile.write(reinterpret_cast<char *>(&averagePowerConsumption), sizeof(double));

    double totalPowerConsumption = m_engine->getTotalPowerConsumption();
    m_outputFile.write(reinterpret_cast<char *>(&totalPowerConsumption), sizeof(double));
}