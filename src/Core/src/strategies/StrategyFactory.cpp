#include "strategies/StrategyFactory.h"
#include "strategies/FirstFitDecreasing.h"
#include "strategies/BestFitDecreasing.h"
#include "strategies/AlphaBetaStrategy.h"

std::vector<StrategyInfo> StrategyFactory::availableStrategies()
{
    std::vector<StrategyInfo> list;
    list.push_back({"FirstFitDecreasing"});
    list.push_back({"BestFitDecreasing"});
    list.push_back({"AlphaBetaStrategy"});
    // etc.
    return list;
}

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
    else
    {
        throw std::runtime_error("Unknown strategy: " + name.toStdString());
    }
}