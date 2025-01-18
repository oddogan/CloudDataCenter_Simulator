#include "strategies/BestFitDecreasing.h"
#include <algorithm>
#include "strategies/MachineState.h"
#include "data/Resources.h"

std::vector<PlacementDecision> BestFitDecreasing::run(
    const std::vector<VirtualMachine *> &vms,
    const std::vector<PhysicalMachine> &machines)
{
    // Build ephemeral
    std::vector<MachineState> localStates;
    localStates.reserve(machines.size());
    for (auto &pm : machines)
    {
        MachineState ms;
        ms.id = pm.getID();
        ms.total = pm.getTotal();
        ms.used = pm.getUsed();
        localStates.push_back(ms);
    }

    // Sort VMs by descending CPU usage
    std::vector<VirtualMachine *> sorted = vms;
    std::sort(sorted.begin(), sorted.end(), [](auto *a, auto *b)
              { return a->getTotalRequestedResources().cpu > b->getTotalRequestedResources().cpu; });

    std::vector<PlacementDecision> results;
    results.reserve(vms.size());

    for (auto *vm : sorted)
    {
        Resources need = vm->getTotalRequestedResources();
        int bestIdx = -1;
        double bestLeftCPU = 1e9;

        // best fit
        for (int i = 0; i < (int)localStates.size(); i++)
        {
            auto &ms = localStates[i];
            if (ms.canHost(need))
            {
                double leftover = (ms.total.cpu - ms.used.cpu) - need.cpu;
                if (leftover < bestLeftCPU)
                {
                    bestLeftCPU = leftover;
                    bestIdx = i;
                }
            }
        }
        if (bestIdx >= 0)
        {
            localStates[bestIdx].used += need;
            results.push_back({vm, localStates[bestIdx].id});
        }
        else
        {
            results.push_back({vm, -1});
        }
    }

    return results;
}