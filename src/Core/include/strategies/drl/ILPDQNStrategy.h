#pragma once

#include "IDQNStrategy.h"

class ILPDQNStrategy : public IDQNStrategy
{
public:
    ILPDQNStrategy();
    ~ILPDQNStrategy() override;
    QString name() const override;
};