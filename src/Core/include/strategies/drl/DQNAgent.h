#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <deque>
#include <random>
#include <algorithm>
#include <chrono>

/**
 * A single experience transition in the replay buffer.
 *  - state: a vector<double> for the environment state
 *  - action: an int for the chosen discrete action
 *  - reward: a double
 *  - nextState: a vector<double> for the next state
 *  - done: bool, true if this transition ends the episode
 */
struct Transition
{
    std::vector<double> state;
    int action;
    double reward;
    std::vector<double> nextState;
    bool done;
};

/**
 * A minimal DQNAgent class
 */
class DQNAgent
{
public:
    // constructor
    DQNAgent(int stateDim, int actionCount, double lr = 1e-4, size_t replayCapacity = 100000,
             size_t batchSize = 32, double gamma = 0.99)
        : m_stateDim(stateDim), m_actionCount(actionCount),
          m_replayCapacity(replayCapacity), m_batchSize(batchSize), m_gamma(gamma),
          m_epsilon(1.0), m_epsilonMin(0.01), m_epsilonDecay(1e-4)
    {
        // create QNet
        m_qNet = QNet(stateDim, actionCount);
        m_optimizer = std::make_unique<torch::optim::Adam>(
            m_qNet->parameters(),
            torch::optim::AdamOptions(lr));

        // random seed
        m_rng = std::mt19937(std::random_device{}());
    }

    size_t getBatchSize() const
    {
        return m_batchSize;
    }

    void setBatchSize(size_t batchSize)
    {
        m_batchSize = batchSize;
    }

    // select an action with epsilon-greedy
    int selectAction(const std::vector<double> &state)
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

    // store a transition in replay
    void storeTransition(const Transition &t)
    {
        if (m_replay.size() >= m_replayCapacity)
        {
            m_replay.pop_front();
        }
        m_replay.push_back(t);
    }

    // do one training step (sample from replay)
    void update()
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
        auto stateTensor = torch::zeros({(long)batch, m_stateDim});
        auto nextTensor = torch::zeros({(long)batch, m_stateDim});
        auto actionTensor = torch::zeros({(long)batch}, torch::kInt64);
        auto rewardTensor = torch::zeros({(long)batch});
        auto doneTensor = torch::zeros({(long)batch}, torch::kInt64);

        for (size_t i = 0; i < batch; i++)
        {
            // copy state
            for (int d = 0; d < m_stateDim; d++)
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

private:
    /**
     * QNet: a simple feedforward net with two linear layers.
     * inDim => hidden(64) => outDim
     */
    struct QNetImpl : torch::nn::Module
    {
        torch::nn::Linear fc1{nullptr}, fc2{nullptr};

        QNetImpl()
        {
        }

        QNetImpl(int inDim, int outDim)
        {
            fc1 = register_module("fc1", torch::nn::Linear(inDim, 64));
            fc2 = register_module("fc2", torch::nn::Linear(64, outDim));
        }

        torch::Tensor forward(const torch::Tensor &x)
        {
            // shape of x: [batch, inDim]
            auto h = torch::relu(fc1->forward(x));
            auto out = fc2->forward(h); // [batch, outDim]
            return out;
        }
    };
    TORCH_MODULE(QNet);

    int m_stateDim;
    int m_actionCount;
    size_t m_replayCapacity;
    size_t m_batchSize;
    double m_gamma;

    double m_epsilon;
    double m_epsilonMin;
    double m_epsilonDecay;

    // Q net
    QNet m_qNet;
    std::unique_ptr<torch::optim::Optimizer> m_optimizer;

    // replay
    std::deque<Transition> m_replay;

    // RNG
    std::mt19937 m_rng;
};