#include "strategies/FirstFitDecreasing.h"
#include <algorithm>
#include "strategies/MachineState.h"
#include "data/Resources.h"

std::vector<PlacementDecision> FirstFitDecreasing::run(const std::vector<VirtualMachine *> &vms, const std::vector<PhysicalMachine> &machines)
{
    // Step 1: Build ephemeral MachineState from each real PM
    std::vector<MachineState> localStates;
    localStates.reserve(machines.size());
    for (auto &pm : machines)
    {
        MachineState ms;
        ms.id = pm.getID();
        ms.total = pm.getTotal(); // reading pm's lock internally
        ms.used = pm.getUsed();
        localStates.push_back(ms);
    }

    // Step 2: Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sorted = vms;
    std::sort(sorted.begin(), sorted.end(), [](auto *a, auto *b)
              { return a->getUsage().cpu > b->getUsage().cpu; });

    std::vector<PlacementDecision> results;
    results.reserve(vms.size());

    // Step 3: For each VM in descending order, do a "First Fit"
    for (auto *vm : sorted)
    {
        Resources need = vm->getTotalRequestedResources();
        bool placed = false;

        for (auto &ms : localStates)
        {
            if (ms.canHost(need))
            {
                ms.used += need; // ephemeral allocation
                results.push_back({vm, ms.id});
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            // no fit found
            results.push_back({vm, -1});
        }
    }

    return results;
}