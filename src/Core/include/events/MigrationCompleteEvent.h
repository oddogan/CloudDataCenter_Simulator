#pragma once

#include "IEvent.h"

class DataCenter;
class SimulationEngine;

/**
 * MigrationCompleteEvent indicates that the migration
 * of a VM from oldPM to newPM has finished.
 */
class MigrationCompleteEvent : public IEvent
{
public:
    MigrationCompleteEvent(double time, int vmId, int oldPmId, int newPmId);

    double getTime() const override;
    void accept(DataCenter &dc, SimulationEngine &engine) override;

    int getVmId() const;
    int getOldPmId() const;
    int getNewPmId() const;

private:
    double m_time;
    int m_vmId;
    int m_oldPmId;
    int m_newPmId;
};