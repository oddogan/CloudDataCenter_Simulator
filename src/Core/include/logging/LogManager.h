#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <sstream>

enum class LogCategory
{
    PLACEMENT,
    VM_ARRIVAL,
    VM_DEPARTURE,
    VM_MIGRATION,
    VM_UTIL_UPDATE,
    TRACE,
    DEBUG
};

struct LogCategoryInfo
{
    LogCategory cat;
    std::string displayName; // For GUI e.g. "VM Arrival"
};

class LogManager
{
public:
    static LogManager &instance();

    // Non-copyable
    LogManager(const LogManager &) = delete;
    LogManager &operator=(const LogManager &) = delete;

    // Returns a static list of all categories with display names
    std::vector<LogCategoryInfo> getAllCategories() const;
    std::string getCategoryDisplayName(LogCategory cat) const;

    // Configure
    void setLogFile(const std::string &filename);
    void setCategoryEnabled(LogCategory cat, bool enabled);
    void setLogToConsole(bool enabled);

    // For reading settings in the UI
    bool isCategoryEnabled(LogCategory cat) const;
    std::string getLogFile() const;
    bool getLogToConsole() const;

    // Log
    void log(LogCategory cat, const std::string &msg);

private:
    LogManager();
    ~LogManager();

    mutable std::mutex m_mutex;
    std::ofstream m_logFile;
    std::string m_currentLogFile;
    bool m_logToConsole;

    std::unordered_map<LogCategory, bool> m_categoriesEnabled;
};