#pragma once

#include <deque>
#include <random>
#include <torch/torch.h>

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

class IDQNAgent
{
public:
    virtual ~IDQNAgent() = default;

    // do one training step (sample from replay)
    virtual void update() = 0;

    // Select an action given a state
    virtual int selectAction(const std::vector<double> &state) = 0;

    // store a transition in replay
    void storeTransition(const Transition &t)
    {
        if (m_replay.size() >= m_replayCapacity)
        {
            m_replay.pop_front();
        }
        m_replay.push_back(t);
    }

    // Batch size operations
    size_t getBatchSize() const { return m_batchSize; }
    void setBatchSize(size_t batchSize) { m_batchSize = batchSize; }

    // Epsilon operations
    double getEpsilon() const { return m_epsilon; }

protected:
    size_t m_stateDim;
    size_t m_actionCount;
    size_t m_replayCapacity;
    size_t m_batchSize;

    double m_learningRate;
    double m_gamma;
    double m_epsilon;
    double m_epsilonMin;
    double m_epsilonDecay;

    std::deque<Transition> m_replay;

    std::mt19937 m_rng;

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
};