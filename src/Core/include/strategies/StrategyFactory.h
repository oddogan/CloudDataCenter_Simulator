#pragma once

#include <QString>
#include <QStringList>
#include "IConfigurableStrategy.h"

class StrategyFactory
{
public:
    static IConfigurableStrategy *create(const QString &name);

    // List all known strategy names for the UI
    static QStringList availableStrategies();
};