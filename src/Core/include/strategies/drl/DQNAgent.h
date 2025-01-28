#pragma once

/* State design

    [0]: # active VMs
    [1]: # active PMs
    [2]: Average CPU utilization across PMs
    [3]: Standard deviation of CPU utilization across PMs
    [4]: Average RAM utilization across PMs
    [5]: Standard deviation of RAM utilization across PMs
    [6]: Average disk utilization across PMs
    [7]: Standard deviation of disk utilization across PMs
    [8]: Average bandwidth utilization across PMs
    [9]: Standard deviation of bandwidth utilization across PMs
*/

#include <torch/torch.h>
#include <vector>
#include <deque>
#include <random>

// a single replay transition
struct Transition
{
    std::vector<double> state;
    int action;
    double reward;
    std::vector<double> nextState;
    bool done;
};

class DQNAgent
{
public:
    DQNAgent(int stateDim, int actionCount, double lr = 1e-3);
    ~DQNAgent();

    int selectAction(const std::vector<double> &state);
    void storeTransition(const Transition &transition);
    void update();

private:
    struct QNetImpl : torch::nn::Module
    {
        torch::nn::Linear fc1, fc2;
        QNetImpl(int inDim, int outDim);
        torch::Tensor forward(torch::Tensor &x);
    };
    TORCH_MODULE(QNet);

    QNet m_qNet;
    torch::optim::Adam m_optimizer;
    std::deque<Transition> m_replay;
    size_t m_capacity;
    std::mt19937 m_rng;

    int m_stateDim;
    int m_actionCount;
    double m_epsilon;

    int randomAction();
    int argmaxQ(torch::Tensor qvals);
};