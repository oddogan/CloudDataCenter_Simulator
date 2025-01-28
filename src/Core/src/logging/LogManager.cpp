#include "logging/LogManager.h"
#include <iostream>
#include <chrono>
#include <ctime>

// We'll define a static array for category info
static const LogCategoryInfo s_categories[] = {
    {LogCategory::PLACEMENT, "VM Placement"},
    {LogCategory::VM_ARRIVAL, "VM Arrival"},
    {LogCategory::VM_DEPARTURE, "VM Departure"},
    {LogCategory::VM_MIGRATION, "Migration"},
    {LogCategory::VM_UTIL_UPDATE, "VM Util Update"},
    {LogCategory::TRACE, "Trace Info"},
    {LogCategory::STRATEGY, "Strategy Info"},
    {LogCategory::DEBUG, "Debug"},
    {LogCategory::WARNING, "Warning"}};

LogManager &LogManager::instance()
{
    static LogManager instance;
    return instance;
}

LogManager::LogManager() : m_currentLogFile(""), m_logToConsole(true)
{
    // default all to false
    for (auto &info : s_categories)
    {
        m_categoriesEnabled[info.cat] = false;
    }
}

LogManager::~LogManager()
{
    if (m_logFile.is_open())
    {
        m_logFile.close();
    }
}

// Return a vector of categories
std::vector<LogCategoryInfo> LogManager::getAllCategories() const
{
    std::vector<LogCategoryInfo> result;
    for (auto &c : s_categories)
    {
        result.push_back(c);
    }
    return result;
}

std::string LogManager::getCategoryDisplayName(LogCategory cat) const
{
    for (auto &c : s_categories)
    {
        if (c.cat == cat)
        {
            return c.displayName;
        }
    }
    return "Unknown";
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

void LogManager::setLogToConsole(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logToConsole = enabled;
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

bool LogManager::getLogToConsole() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logToConsole;
}

void LogManager::log(LogCategory cat, const std::string &msg)
{
    // check if enabled
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_categoriesEnabled.find(cat);
    if (it == m_categoriesEnabled.end() || !it->second)
    {
        return; // not enabled
    }

    // timestamp
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    // e.g. "Mon Nov 13 14:15:32 2023"
    // We'll remove the trailing newline
    std::string ts = std::ctime(&timeT);
    if (!ts.empty() && ts.back() == '\n')
        ts.pop_back();

    // find displayName
    std::string catName = getCategoryDisplayName(cat);
    std::string logMessage = "[" + ts + " - " + catName + "] " + msg + "\n";

    if (m_logToConsole)
    {
        std::cout << logMessage;
    }

    if (m_logFile.is_open())
    {
        m_logFile << logMessage;
        m_logFile.flush();
    }
}