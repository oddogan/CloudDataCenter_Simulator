#pragma once

#include <QWidget>
#include <QString>
#include "IPlacementStrategy.h"

class IConfigurableStrategy : public IPlacementStrategy
{
public:
    virtual ~IConfigurableStrategy() = default;

    // Returns a QWidget (or a layout) containing input fields for user configuration
    // The DataCenter or GUI can embed this widget in a config panel.
    virtual QWidget *createConfigWidget(QWidget *parent = nullptr) = 0;

    // Called by the GUI once the user changes parameters in the config widget
    // Letting the strategy update its internal fields
    virtual void applyConfigFromUI() = 0;

    // Return a human-readable name for the strategy
    virtual QString name() const = 0;
};