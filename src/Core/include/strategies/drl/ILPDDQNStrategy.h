#pragma once

#include "IDQNStrategy.h"

class ILPDDQNStrategy : public IDQNStrategy
{
public:
    ILPDDQNStrategy();
    ~ILPDDQNStrategy() override;
    QString name() const override;
};