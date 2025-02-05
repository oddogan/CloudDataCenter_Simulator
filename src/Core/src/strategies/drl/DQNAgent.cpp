#include "strategies/drl/DQNAgent.h"
#include <algorithm>
#include <cstdlib>

DQNAgent::QNetImpl::QNetImpl(int inDim, int outDim)
    : fc1(register_module("fc1", torch::nn::Linear(inDim, 64))),
      fc2(register_module("fc2", torch::nn::Linear(64, outDim)))
{
}

torch::Tensor DQNAgent::QNetImpl::forward(torch::Tensor &x)
{
    auto h = torch::relu(fc1->forward(x));
    return fc2->forward(h); // shape: [batchSize, outDim]
}

DQNAgent::DQNAgent(int stateDim, int actionCount, double lr, size_t batchSize, double epsilon)
    : m_qNet(QNet(stateDim, actionCount)),
      m_optimizer(m_qNet->parameters(), torch::optim::AdamOptions(lr)),
      m_capacity(10000),
      m_rng(std::random_device()()),
      m_stateDim(stateDim),
      m_actionCount(actionCount),
      m_batchSize(batchSize),
      m_epsilon(0.1)
{
}

DQNAgent::~DQNAgent()
{
}

int DQNAgent::selectAction(const std::vector<double> &state)
{
    if (std::uniform_real_distribution<double>(0, 1)(m_rng) < m_epsilon)
    {
        return randomAction();
    }

    torch::NoGradGuard noGrad;
    auto stateTensor = torch::from_blob(const_cast<double *>(state.data()), {1, m_stateDim});
    auto qValues = m_qNet->forward(stateTensor).squeeze(0);
    return argmaxQ(qValues);
}

int DQNAgent::randomAction()
{
    return std::uniform_int_distribution<int>(0, m_actionCount - 1)(m_rng);
}

int DQNAgent::argmaxQ(torch::Tensor qvals)
{
    // find the index of the max q value
    auto maxQ = qvals.argmax();
    return maxQ.item<int>();
}

void DQNAgent::storeTransition(const Transition &transition)
{
    if (m_replay.size() > m_capacity)
    {
        m_replay.pop_front();
    }
    m_replay.push_back(transition);
}

void DQNAgent::update()
{
    if (m_replay.size() < m_batchSize)
    {
        return;
    }

    std::vector<size_t> indices(m_replay.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), m_rng);

    auto batch = std::min<size_t>(m_batchSize, m_replay.size());
    auto replayBatch = std::vector<Transition>(batch);
    for (size_t i = 0; i < batch; ++i)
    {
        replayBatch[i] = m_replay[indices[i]];
    }

    auto stateBatch = torch::empty({(long long)batch, m_stateDim});
    auto nextStateBatch = torch::empty({(long long)batch, m_stateDim});
    auto actionBatch = torch::empty({(long long)batch}, torch::kInt64);
    auto rewardBatch = torch::empty({(long long)batch});
    auto doneBatch = torch::empty({(long long)batch}, torch::kBool);

    for (size_t i = 0; i < batch; ++i)
    {
        stateBatch[i] = torch::from_blob(const_cast<double *>(replayBatch[i].state.data()), {m_stateDim});
        nextStateBatch[i] = torch::from_blob(const_cast<double *>(replayBatch[i].nextState.data()), {m_stateDim});
        actionBatch[i] = replayBatch[i].action;
        rewardBatch[i] = replayBatch[i].reward;
        doneBatch[i] = replayBatch[i].done ? 0 : 1;
    }

    auto qValues = m_qNet->forward(stateBatch);
    auto nextQValues = m_qNet->forward(nextStateBatch);
    auto targetQValues = qValues.clone();

    for (size_t i = 0; i < batch; ++i)
    {
        auto target = rewardBatch[i];
        if (!doneBatch[i].item<bool>())
        {
            target += 0.99 * nextQValues[i].max().item<double>();
        }
        targetQValues[i][actionBatch[i].item<int>()] = target;
    }

    auto loss = torch::mse_loss(qValues, targetQValues);
    m_optimizer.zero_grad();
    loss.backward();
    m_optimizer.step();
}