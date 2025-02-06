#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <deque>
#include <random>
#include <algorithm>
#include <chrono>
#include "IDQNAgent.h"

/**
 * A minimal DQNAgent class
 */
class DQNAgent : public IDQNAgent
{
public:
    // constructor
    DQNAgent(size_t stateDim, size_t actionCount, double lr = 1e-4, size_t replayCapacity = 100000,
             size_t batchSize = 32, double gamma = 0.99)
    {
        m_stateDim = stateDim;
        m_actionCount = actionCount;
        m_replayCapacity = replayCapacity;
        m_batchSize = batchSize;
        m_learningRate = lr;
        m_gamma = gamma;
        m_epsilon = 1.0;
        m_epsilonMin = 0.01;
        m_epsilonDecay = 1e-4;

        // create QNet
        m_qNet = QNet(stateDim, actionCount);
        m_optimizer = std::make_unique<torch::optim::Adam>(
            m_qNet->parameters(),
            torch::optim::AdamOptions(m_learningRate));

        // random seed
        m_rng = std::mt19937(std::random_device{}());
    }

    virtual void update() override
    {
        if (m_replay.size() < m_batchSize)
        {
            return; // not enough data
        }

        // sample a batch
        std::vector<size_t> indices(m_replay.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), m_rng);

        size_t batch = std::min(m_batchSize, m_replay.size());
        std::vector<Transition> batchData(batch);
        for (size_t i = 0; i < batch; i++)
        {
            batchData[i] = m_replay[indices[i]];
        }

        // build Tensors
        auto stateTensor = torch::zeros({(long)batch, (long)m_stateDim});
        auto nextTensor = torch::zeros({(long)batch, (long)m_stateDim});
        auto actionTensor = torch::zeros({(long)batch}, torch::kInt64);
        auto rewardTensor = torch::zeros({(long)batch});
        auto doneTensor = torch::zeros({(long)batch}, torch::kInt64);

        for (size_t i = 0; i < batch; i++)
        {
            // copy state
            for (size_t d = 0; d < m_stateDim; d++)
            {
                stateTensor[i][d] = (float)batchData[i].state[d];
                nextTensor[i][d] = (float)batchData[i].nextState[d];
            }
            actionTensor[i] = batchData[i].action;
            rewardTensor[i] = (float)batchData[i].reward;
            doneTensor[i] = batchData[i].done ? 1 : 0;
        }

        // forward pass on states
        auto qvals = m_qNet->forward(stateTensor);

        // forward pass on next states
        auto nextQ = m_qNet->forward(nextTensor);

        // build target Q
        auto targetQ = qvals.clone();

        for (size_t i = 0; i < batch; i++)
        {
            double target = rewardTensor[i].item<float>();
            int done = doneTensor[i].item<int>();
            if (done == 0)
            {
                // not done => add gamma * max next Q
                // basic DQN approach
                auto row = nextQ[i]; // shape [actionCount]
                double maxNext = row.max().item<float>();
                target += m_gamma * maxNext;
            }
            int a = actionTensor[i].item<int>();
            targetQ[i][a] = target;
        }

        // compute loss
        auto loss = torch::mse_loss(qvals, targetQ.detach());

        // step
        m_optimizer->zero_grad();
        loss.backward();
        m_optimizer->step();

        // optionally decay epsilon
        if (m_epsilon > m_epsilonMin)
        {
            m_epsilon -= m_epsilonDecay;
            if (m_epsilon < m_epsilonMin)
            {
                m_epsilon = m_epsilonMin;
            }
        }
    }

    virtual int selectAction(const std::vector<double> &state) override
    {
        double r = (double)rand() / RAND_MAX;
        if (r < m_epsilon)
        {
            // random
            std::uniform_int_distribution<int> dist(0, m_actionCount - 1);
            return dist(m_rng);
        }
        else
        {
            // greedy from Q
            torch::Tensor s = torch::from_blob(
                                  const_cast<double *>(state.data()),
                                  {(long)state.size()})
                                  .clone()
                                  .unsqueeze(0)
                                  .to(torch::kFloat); // shape [1, stateDim]

            torch::Tensor qvals = m_qNet->forward(s); // [1, actionCount]
            auto maxIdx = qvals.argmax(1);            // shape [1]
            int action = maxIdx.item<int>();
            return action;
        }
    }

private:
    QNet m_qNet;
    std::unique_ptr<torch::optim::Optimizer> m_optimizer;
};