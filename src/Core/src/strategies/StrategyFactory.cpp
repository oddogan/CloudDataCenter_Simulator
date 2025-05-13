#include "strategies/StrategyFactory.h"
#include "strategies/FirstFitDecreasing.h"
#include "strategies/BestFitDecreasing.h"
#include "strategies/AlphaBetaStrategy.h"
#include "strategies/ILPStrategy.h"
#include "strategies/drl/ILPDQNStrategy.h"
#include "strategies/drl/ILPDDQNStrategy.h"

std::vector<StrategyInfo> StrategyFactory::availableStrategies()
{
    std::vector<StrategyInfo> list;
    list.push_back({"FirstFitDecreasing"});
    list.push_back({"BestFitDecreasing"});
    list.push_back({"AlphaBetaStrategy"});
    list.push_back({"ILPStrategy"});
    list.push_back({"ILP + DQN Strategy"});
    list.push_back({"ILP + DDQN Strategy"});
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
    else if (name == "ILPStrategy")
    {
        return new ILPStrategy();
    }
    else if (name == "ILP + DQN Strategy")
    {
        return new ILPDQNStrategy();
    }
    else if (name == "ILP + DDQN Strategy")
    {
        return new ILPDDQNStrategy();
    }
    else
    {
        throw std::runtime_error("Unknown strategy: " + name.toStdString());
    }
}