#pragma once

#include <QString>
#include <QStringList>
#include "IPlacementStrategy.h"

class StrategyFactory
{
public:
    static IPlacementStrategy *create(const QString &name);

    // List all known strategy names for the UI
    static QStringList availableStrategies();
};