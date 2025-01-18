#include "events/ReconfigureStrategyEvent.h"
#include "DataCenter.h"
#include "SimulationEngine.h"

void ReconfigureStrategyEvent::accept(DataCenter &dc, SimulationEngine &engine)
{
    dc.handle(*this, engine);
}