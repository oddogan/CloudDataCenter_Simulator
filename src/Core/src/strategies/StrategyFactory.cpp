#include "strategies/StrategyFactory.h"
#include "strategies/FirstFitDecreasing.h"
#include "strategies/BestFitDecreasing.h"
#include "strategies/AlphaBetaStrategy.h"

IPlacementStrategy *StrategyFactory::create(const QString &name)
{
    if (name == "FirstFitDecreasing")
    {
        return new FirstFitDecreasing();
    }
    else if (name == "BestFitDecreasing")
    {
        return new BestFitDecreasing();
    }
    else if (name == "AlphaBetaStrategy")
    {
        return new AlphaBetaStrategy();
    }
    // fallback
    return new FirstFitDecreasing();
}

QStringList StrategyFactory::availableStrategies()
{
    return {"FirstFitDecreasing", "BestFitDecreasing", "AlphaBetaStrategy"};
}