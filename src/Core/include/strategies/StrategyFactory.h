#pragma once

#include <QString>
#include <QStringList>
#include "IPlacementStrategy.h"

struct StrategyInfo
{
    QString name; // e.g. "FirstFitDecreasing"
    // we might store a description, etc.
};

class StrategyFactory
{
public:
    // Create a strategy by name
    static IPlacementStrategy *create(const QString &name);

    // Return a list of known strategies
    static std::vector<StrategyInfo> availableStrategies();
};