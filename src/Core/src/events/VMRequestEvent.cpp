#include "events/VMRequestEvent.h"
#include "DataCenter.h"
#include "SimulationEngine.h"

void VMRequestEvent::accept(DataCenter &dataCenter, SimulationEngine &engine)
{
    dataCenter.handle(*this, engine);
}

std::unique_ptr<VirtualMachine> VMRequestEvent::takeVM()
{
    return std::move(m_vm);
}