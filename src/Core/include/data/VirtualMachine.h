#pragma once

#include <vector>
#include "Resources.h"

struct UsageUpdate
{
    double offset; // time offset from the moment VM is placed
    double utilization;
};

class VirtualMachine
{
public:
    VirtualMachine(int id, const Resources &requestedResources, double duration)
        : m_ID(id), m_duration(duration), m_isPlaced(false), m_isMigrating(false), m_totalRequestedResources(requestedResources), m_currentUsage(0, 0, 0, 0, 0)
    {
    }

    int getID() const { return m_ID; }

    bool isPlaced() const { return m_isPlaced; }
    void setPlaced(bool isPlaced) { m_isPlaced = isPlaced; }

    bool isMigrating() const { return m_isMigrating; }

    void setMigrating(bool isMigrating)
    {
        m_isMigrating = isMigrating;
        if (isMigrating)
            m_oldPMID = m_currentPMID;
        else
            m_oldPMID = -1;
    }

    void setPMID(int pmID) { m_currentPMID = pmID; }

    int getOldPMID() const
    {
        if (!m_isMigrating)
        {
            throw std::runtime_error("VM is not migrating");
            return -1;
        }
        return m_oldPMID;
    }

    double getStartTime() const { return m_startTime; }
    void setStartTime(double startTime) { m_startTime = startTime; }
    double getDuration() const { return m_duration; }

    Resources getTotalRequestedResources() const { return m_totalRequestedResources; }
    Resources getUsage() const { return m_currentUsage; }
    void setUtilization(double utilization)
    {
        m_currentUsage = m_totalRequestedResources;
        m_currentUsage.cpu *= utilization;
    }
    void addFutureUtilization(double offset, double utilization_norm) { m_futureUsage.push_back({offset, utilization_norm}); }
    const std::vector<UsageUpdate> &getFutureUtilizations() const { return m_futureUsage; }

private:
    int m_ID;
    double m_startTime;
    double m_duration;
    bool m_isPlaced;
    bool m_isMigrating;
    int m_oldPMID;
    int m_currentPMID;

    Resources m_totalRequestedResources;
    Resources m_currentUsage;
    std::vector<UsageUpdate> m_futureUsage;
};