#include "events/MigrationCompleteEvent.h"
#include "DataCenter.h"
#include "SimulationEngine.h"

MigrationCompleteEvent::MigrationCompleteEvent(double time, int vmId, int oldPmId, int newPmId)
    : m_time(time), m_vmId(vmId), m_oldPmId(oldPmId), m_newPmId(newPmId)
{
}

double MigrationCompleteEvent::getTime() const
{
    return m_time;
}

void MigrationCompleteEvent::accept(DataCenter &dc, SimulationEngine &engine)
{
    dc.handle(*this, engine);
}

int MigrationCompleteEvent::getVmId() const { return m_vmId; }
int MigrationCompleteEvent::getOldPmId() const { return m_oldPmId; }
int MigrationCompleteEvent::getNewPmId() const { return m_newPmId; }