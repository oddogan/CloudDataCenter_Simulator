#pragma once

#include "data/Resources.h"

// A lightweight "snapshot" for ephemeral usage in the strategies.
struct MachineState
{
    int id;
    bool isTurnedOn;
    double powerOnCost;
    double cpuCost;
    Resources total; // total capacity
    Resources used;  // current usage

    // Just a convenience function
    bool canHost(const Resources &req) const
    {
        return ::canHost(req, total - used);
    }
};

inline void allocateEphemeral(MachineState &ms, const Resources &req)
{
    ms.used += req;
}