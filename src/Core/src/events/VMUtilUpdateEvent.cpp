#include "events/VMUtilUpdateEvent.h"
#include "DataCenter.h"
#include "SimulationEngine.h"

void VMUtilUpdateEvent::accept(DataCenter &dc, SimulationEngine &engine)
{
    dc.handle(*this, engine);
}