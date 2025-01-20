#include "logging/LogManager.h"
#include <iostream>
#include <chrono>
#include <ctime>

LogManager &LogManager::instance()
{
    static LogManager instance;
    return instance;
}

LogManager::LogManager() : m_currentLogFile("")
{
    m_categoriesEnabled[LogCategory::PLACEMENT] = false;
    m_categoriesEnabled[LogCategory::VM_ARRIVAL] = false;
    m_categoriesEnabled[LogCategory::VM_DEPARTURE] = false;
    m_categoriesEnabled[LogCategory::VM_MIGRATION] = false;
    m_categoriesEnabled[LogCategory::VM_UTIL_UPDATE] = false;
    m_categoriesEnabled[LogCategory::TRACE] = false;
    m_categoriesEnabled[LogCategory::DEBUG] = false;
}

LogManager::~LogManager()
{
    if (m_logFile.is_open())
    {
        m_logFile.close();
    }
}

void LogManager::setLogFile(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open())
    {
        m_logFile.close();
    }
    m_currentLogFile = filename;
    if (!filename.empty())
    {
        m_logFile.open(filename, std::ios::out | std::ios::app);
        if (!m_logFile.is_open())
        {
            std::cerr << "[LogManager] Error opening log file " << filename << std::endl;
        }
    }
}

void LogManager::setCategoryEnabled(LogCategory cat, bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_categoriesEnabled[cat] = enabled;
}

bool LogManager::isCategoryEnabled(LogCategory cat) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_categoriesEnabled.find(cat);
    if (it != m_categoriesEnabled.end())
    {
        return it->second;
    }
    return false;
}

std::string LogManager::getLogFile() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentLogFile;
}

void LogManager::log(LogCategory cat, const std::string &msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // If not enabled, don't log
    auto it = m_categoriesEnabled.find(cat);
    if (it == m_categoriesEnabled.end() || !it->second)
    {
        return;
    }

    // If no file open, fallback to stdout
    if (!m_logFile.is_open())
    {
        std::cerr << "[" << categoryName(cat) << "] " << msg << std::endl;
        return;
    }

    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);

    // Format
    m_logFile << "[" << std::ctime(&timeT);
    m_logFile.seekp(-2, std::ios_base::cur); // remove newline from ctime
    m_logFile << " - " << categoryName(cat) << "] " << msg << std::endl;
    m_logFile.flush();
}