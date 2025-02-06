#include "strategies/drl/ILPDQNStrategy.h"
#include "logging/LogManager.h"
#include "strategies/drl/DQNAgent.h"

ILPDQNStrategy::ILPDQNStrategy()
{
    m_agent = new DQNAgent(20, m_actions.size(), 1e-4, 100000, 128, 0.99);
}

ILPDQNStrategy::~ILPDQNStrategy()
{
}

QString ILPDQNStrategy::name() const
{
    return "ILP + DQN Strategy";
}