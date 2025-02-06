#include "strategies/drl/ILPDDQNStrategy.h"
#include "logging/LogManager.h"
#include "strategies/drl/DDQNAgent.h"

ILPDDQNStrategy::ILPDDQNStrategy()
{
    m_agent = new DDQNAgent(20, m_actions.size(), 1e-4, 100000, 128, 0.99);
}

ILPDDQNStrategy::~ILPDDQNStrategy()
{
}

QString ILPDDQNStrategy::name() const
{
    return "ILP + DDQN Strategy";
}