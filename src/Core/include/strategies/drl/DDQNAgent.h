#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <deque>
#include <random>
#include <algorithm>
#include <chrono>
#include <numeric>

/**
 * A single experience transition in the replay buffer.
 */
struct TransitionDDQN
{
    std::vector<double> state;
    int action;
    double reward;
    std::vector<double> nextState;
    bool done;
};

/**
 * QNet: a small feedforward net with two linear layers (64 hidden units).
 * We'll use the same architecture for both policyNet and targetNet.
 */
struct QNetImpl : torch::nn::Module
{
    torch::nn::Linear fc1{nullptr}, fc2{nullptr};

    QNetImpl() {}

    QNetImpl(int inDim, int outDim)
    {
        fc1 = register_module("fc1", torch::nn::Linear(inDim, 64));
        fc2 = register_module("fc2", torch::nn::Linear(64, outDim));
    }

    torch::Tensor forward(const torch::Tensor &x)
    {
        // shape [batch, inDim]
        auto h = torch::relu(fc1->forward(x));
        auto out = fc2->forward(h); // shape [batch, outDim]
        return out;
    }
};
TORCH_MODULE(QNet);

/**
 * DoubleDQNAgent:
 *  - We have a policy net for action selection
 *  - We have a target net for stable Q-value references
 *  - We do Double DQN update:
 *    target = r + gamma * Q_target(s', argmax_a Q_policy(s',a))
 *      if not done
 */
class DDQNAgent
{
public:
    DDQNAgent(int stateDim, int actionCount, double lr = 1e-4, size_t replayCapacity = 10000,
              size_t batchSize = 32, double gamma = 0.99,
              double epsilonStart = 1.0, double epsilonMin = 0.01, double epsilonDecay = 1e-5)
        : m_stateDim(stateDim), m_actionCount(actionCount),
          m_replayCapacity(replayCapacity), m_batchSize(batchSize), m_gamma(gamma),
          m_epsilon(epsilonStart), m_epsilonMin(epsilonMin), m_epsilonDecay(epsilonDecay)
    {
        // create policy net
        m_policyNet = QNet(stateDim, actionCount);
        // create target net with same architecture
        m_targetNet = QNet(stateDim, actionCount);

        // copy weights from policy to target initially
        std::stringstream stream;
        torch::save(m_policyNet, stream);
        torch::load(m_targetNet, stream);

        // set up optimizer for policy net
        m_optimizer = std::make_unique<torch::optim::Adam>(
            m_policyNet->parameters(),
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

    // pick an action with epsilon-greedy
    int selectAction(const std::vector<double> &state)
    {
        double r = double(rand()) / RAND_MAX;
        if (r < m_epsilon)
        {
            // random
            std::uniform_int_distribution<int> dist(0, m_actionCount - 1);
            return dist(m_rng);
        }
        else
        {
            // greedy from policy net
            auto sTen = torch::from_blob(
                            const_cast<double *>(state.data()),
                            {(long)state.size()})
                            .clone()
                            .unsqueeze(0)
                            .to(torch::kFloat); // shape [1, stateDim]

            // Q-values from policy net
            auto qvals = m_policyNet->forward(sTen); // [1, actionCount]
            auto maxIdx = qvals.argmax(1);           // shape [1]
            int action = maxIdx.item<int>();
            return action;
        }
    }

    // store a transition in replay
    void storeTransition(const TransitionDDQN &t)
    {
        if (m_replay.size() >= m_replayCapacity)
        {
            m_replay.pop_front();
        }
        m_replay.push_back(t);
    }

    // do one training step
    void update()
    {
        if (m_replay.size() < m_batchSize)
            return;

        // sample a batch
        std::vector<size_t> indices(m_replay.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), m_rng);

        size_t batch = std::min(m_batchSize, m_replay.size());
        std::vector<TransitionDDQN> batchData(batch);
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
            for (int d = 0; d < m_stateDim; d++)
            {
                stateTensor[i][d] = float(batchData[i].state[d]);
                nextTensor[i][d] = float(batchData[i].nextState[d]);
            }
            actionTensor[i] = batchData[i].action;
            rewardTensor[i] = float(batchData[i].reward);
            doneTensor[i] = batchData[i].done ? 1 : 0;
        }

        // policy net for Q(s)
        auto qVals = m_policyNet->forward(stateTensor); // [batch, actionCount]

        // policy net for next state => for action selection
        auto nextQ_policy = m_policyNet->forward(nextTensor); // [batch, actionCount]
        // pick best action index from policy net
        auto nextActions = nextQ_policy.argmax(1); // [batch]

        // target net for next state => for Q value of that best action
        auto nextQ_target = m_targetNet->forward(nextTensor); // [batch, actionCount]

        // We'll build the target Q-values from qVals
        auto targetQ = qVals.clone(); // we will modify only the chosen actions

        // for each sample in batch
        for (size_t i = 0; i < batch; i++)
        {
            float reward = rewardTensor[i].item<float>();
            int done = doneTensor[i].item<int>();
            int a = actionTensor[i].item<int>();

            double y = reward;
            if (done == 0)
            { // not terminal
                // double dqn => pick best action from policy, evaluate from target
                int nextA = nextActions[i].item<int>();
                double val = nextQ_target[i][nextA].item<double>();
                y += m_gamma * val;
            }

            // set the target for that (state,action)
            targetQ[i][a] = y;
        }

        // MSE loss
        auto loss = torch::mse_loss(qVals, targetQ.detach());

        // backprop
        m_optimizer->zero_grad();
        loss.backward();
        m_optimizer->step();

        // epsilon decay
        if (m_epsilon > m_epsilonMin)
        {
            m_epsilon -= m_epsilonDecay;
            if (m_epsilon < m_epsilonMin)
            {
                m_epsilon = m_epsilonMin;
            }
        }

        // occasionally update target net
        m_targetUpdateCount++;
        if (m_targetUpdateCount % 1000 == 0)
        {
            // copy policy net to target net
            std::stringstream stream;
            torch::save(m_policyNet, stream);
            torch::load(m_targetNet, stream);
            std::cout << "[Agent] Updated target net at step " << m_targetUpdateCount << "\n";
        }
    }

private:
    int m_stateDim;
    int m_actionCount;
    size_t m_replayCapacity;
    size_t m_batchSize;
    double m_gamma;

    double m_epsilon;
    double m_epsilonMin;
    double m_epsilonDecay;

    // nets
    QNet m_policyNet;
    QNet m_targetNet;

    std::unique_ptr<torch::optim::Optimizer> m_optimizer;

    // replay
    std::deque<TransitionDDQN> m_replay;

    std::mt19937 m_rng;

    // track how many updates to know when to copy to target
    size_t m_targetUpdateCount{0};
};