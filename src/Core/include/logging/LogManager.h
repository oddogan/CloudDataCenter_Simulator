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

inline const char *categoryName(LogCategory cat)
{
    switch (cat)
    {
    case LogCategory::PLACEMENT:
        return "PLACEMENT";
    case LogCategory::VM_ARRIVAL:
        return "VM_ARRIVAL";
    case LogCategory::VM_DEPARTURE:
        return "VM_DEPARTURE";
    case LogCategory::VM_MIGRATION:
        return "VM_MIGRATION";
    case LogCategory::VM_UTIL_UPDATE:
        return "VM_UTIL_UPDATE";
    case LogCategory::TRACE:
        return "TRACE";
    case LogCategory::DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

class LogManager
{
public:
    static LogManager &instance();

    // Non-copyable
    LogManager(const LogManager &) = delete;
    LogManager &operator=(const LogManager &) = delete;

    // Configure
    void setLogFile(const std::string &filename);
    void setCategoryEnabled(LogCategory cat, bool enabled);

    // For reading settings in the UI
    bool isCategoryEnabled(LogCategory cat) const;
    std::string getLogFile() const;

    // Log
    void log(LogCategory cat, const std::string &msg);

private:
    LogManager();
    ~LogManager();

    mutable std::mutex m_mutex;
    std::ofstream m_logFile;
    std::string m_currentLogFile;

    std::unordered_map<LogCategory, bool> m_categoriesEnabled;
};