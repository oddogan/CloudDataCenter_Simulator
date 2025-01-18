#include "events/VMDepartureEvent.h"
#include "DataCenter.h"
#include "SimulationEngine.h"

void VMDepartureEvent::accept(DataCenter &dc, SimulationEngine &engine)
{
    dc.handle(*this, engine);
}