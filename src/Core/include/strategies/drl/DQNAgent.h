#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <random>
#include <string>

// ================================
// 1. Q-network definition (deep net)
// ================================
struct QNetImpl : torch::nn::Module
{
    // Layers
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc4{nullptr};
    torch::nn::BatchNorm1d bn1{nullptr}, bn2{nullptr}, bn3{nullptr};
    torch::nn::Dropout drop1{nullptr}, drop2{nullptr}, drop3{nullptr};

    QNetImpl() {}

    QNetImpl(int inDim, int outDim)
    {
        // Linear layers
        fc1 = register_module("fc1", torch::nn::Linear(inDim, 256));
        fc2 = register_module("fc2", torch::nn::Linear(256, 128));
        fc3 = register_module("fc3", torch::nn::Linear(128, 64));
        fc4 = register_module("fc4", torch::nn::Linear(64, outDim));

        // BatchNorm layers
        bn1 = register_module("bn1", torch::nn::BatchNorm1d(256));
        bn2 = register_module("bn2", torch::nn::BatchNorm1d(128));
        bn3 = register_module("bn3", torch::nn::BatchNorm1d(64));

        // Dropout layers (p=0.2 as an example)
        drop1 = register_module("drop1", torch::nn::Dropout(0.2));
        drop2 = register_module("drop2", torch::nn::Dropout(0.2));
        drop3 = register_module("drop3", torch::nn::Dropout(0.2));
    }

    torch::Tensor forward(const torch::Tensor &x)
    {
        // x shape: [batch_size, inDim]
        auto x1 = fc1->forward(x);
        x1 = bn1->forward(x1);
        x1 = torch::relu(x1);
        x1 = drop1->forward(x1);

        auto x2 = fc2->forward(x1);
        x2 = bn2->forward(x2);
        x2 = torch::relu(x2);
        x2 = drop2->forward(x2);

        auto x3 = fc3->forward(x2);
        x3 = bn3->forward(x3);
        x3 = torch::relu(x3);
        x3 = drop3->forward(x3);

        // Final layer (no activation)
        auto out = fc4->forward(x3); // [batch_size, outDim]
        return out;
    }
};

TORCH_MODULE(QNet); // QNet is a "module holder" around QNetImpl

// ================================
// 2. Replay Buffer
// ================================
struct Transition
{
    torch::Tensor state;
    torch::Tensor next_state;
    int action;
    float reward;
    bool done;
};

class ReplayBuffer
{
public:
    ReplayBuffer(size_t capacity) : capacity(capacity)
    {
        buffer.reserve(capacity);
    }

    void push(const Transition &transition)
    {
        if (buffer.size() < capacity)
        {
            buffer.push_back(transition);
        }
        else
        {
            // Overwrite oldest (circular buffer)
            buffer[idx] = transition;
            idx = (idx + 1) % capacity;
        }
    }

    std::vector<Transition> sample(size_t batchSize)
    {
        // Make sure we have something to sample
        size_t currentSize = std::min(buffer.size(), capacity);
        if (batchSize > currentSize)
            batchSize = currentSize;

        std::uniform_int_distribution<size_t> distribution(0, currentSize - 1);

        std::vector<Transition> sampled;
        sampled.reserve(batchSize);
        for (size_t i = 0; i < batchSize; ++i)
        {
            size_t r = distribution(rng);
            sampled.push_back(buffer[r]);
        }
        return sampled;
    }

    size_t size() const
    {
        return buffer.size();
    }

private:
    std::vector<Transition> buffer;
    size_t capacity;
    size_t idx = 0;
    std::mt19937 rng{std::random_device{}()};
};

// ================================
// 3. DQNAgent class
// ================================
class DQNAgent
{
public:
    DQNAgent(int inDim,
             int outDim,
             float gamma = 0.99f,
             float lr = 1e-3f,
             size_t batchSize = 64,
             size_t bufferSize = 10000,
             float epsilonStart = 1.0f,
             float epsilonEnd = 0.01f,
             float epsilonDecay = 1e-4f,
             int targetUpdateInterval = 1000)
        : epsilon(epsilonStart),
          epsilonMin(epsilonEnd),
          epsilonDecay(epsilonDecay),
          inDim(inDim),
          outDim(outDim),
          gamma(gamma),
          batchSize(batchSize),
          targetUpdateInterval(targetUpdateInterval),
          device(torch::kMPS) // device = torch::kCUDA; // if you have GPU, kCPU
    {
        // Create networks
        qNet = QNet(inDim, outDim);
        targetNet = QNet(inDim, outDim);

        // Decide on device: CPU or CUDA
        qNet->to(device);
        targetNet->to(device);

        // Initialize targetNet with same weights
        std::stringstream stream;
        torch::save(qNet, stream);
        torch::load(targetNet, stream);

        // Create optimizer
        optimizer = std::make_unique<torch::optim::Adam>(qNet->parameters(), lr);

        // Create replay buffer
        replayBuffer = std::make_unique<ReplayBuffer>(bufferSize);
    }

    // Store transition in replay buffer
    void storeTransition(const std::vector<double> &state,
                         int action,
                         float reward,
                         const std::vector<double> &nextState,
                         bool done)
    {
        torch::Tensor stateTensor = torch::from_blob(
                                        const_cast<double *>(state.data()),
                                        {(long)state.size()})
                                        .clone()
                                        .unsqueeze(0)
                                        .to(torch::kFloat); // shape [1, stateDim]

        torch::Tensor nextStateTensor = torch::from_blob(
                                            const_cast<double *>(nextState.data()),
                                            {(long)nextState.size()})
                                            .clone()
                                            .unsqueeze(0)
                                            .to(torch::kFloat); // shape [1, stateDim]

        Transition t{
            stateTensor.to(device),
            nextStateTensor.to(device),
            action,
            reward,
            done};
        replayBuffer->push(t);
    }

    // Epsilon-greedy action selection
    int selectAction(const std::vector<double> &state)
    {
        torch::Tensor stateTensor = torch::from_blob(
                                        const_cast<double *>(state.data()),
                                        {(long)state.size()})
                                        .clone()
                                        .unsqueeze(0)
                                        .to(torch::kFloat); // shape [1, stateDim]

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float sample = dist(rng);

        if (sample < epsilon)
        {
            // random
            std::uniform_int_distribution<int> actionDist(0, outDim - 1);
            return actionDist(rng);
        }
        else
        {
            // greedy
            qNet->eval(); // inference mode
            auto stateOnDevice = stateTensor.to(device);
            auto qValues = qNet->forward(stateOnDevice); // [1, outDim]
            auto maxQValIndex = qValues.argmax(1);       // [1]
            int action = maxQValIndex.item<int>();
            return action;
        }
    }

    // One training update (sample from buffer, compute loss, backprop)
    void update()
    {
        if (replayBuffer->size() < batchSize)
            return; // not enough data

        // Sample a mini-batch
        auto transitions = replayBuffer->sample(batchSize);

        // Collect batch data
        std::vector<torch::Tensor> states, nextStates;
        std::vector<int> actions;
        std::vector<float> rewards;
        std::vector<char> dones;

        states.reserve(batchSize);
        nextStates.reserve(batchSize);
        actions.reserve(batchSize);
        rewards.reserve(batchSize);
        dones.reserve(batchSize);

        for (auto &t : transitions)
        {
            states.push_back(t.state);
            nextStates.push_back(t.next_state);
            actions.push_back(t.action);
            rewards.push_back(t.reward);
            dones.push_back(t.done);
        }

        // Stack into Tensors
        auto stateBatch = torch::cat(states, 0).view({(int)batchSize, inDim}).to(device);
        auto nextStateBatch = torch::cat(nextStates, 0).view({(int)batchSize, inDim}).to(device);

        auto actionTensor = torch::from_blob(actions.data(), {(long)batchSize}, torch::kInt32).to(device);
        auto rewardTensor = torch::from_blob(rewards.data(), {(long)batchSize}, torch::kFloat).to(device);
        auto doneTensor = torch::from_blob(dones.data(), {(long)batchSize}, torch::kBool).to(device);

        // Online Q
        qNet->train();                            // training mode
        auto qValues = qNet->forward(stateBatch); // [batchSize, outDim]

        // Gather Q for chosen actions
        auto actionLong = actionTensor.to(torch::kLong).unsqueeze(1); // [batchSize,1]
        auto qChosen = qValues.gather(1, actionLong).squeeze(1);      // [batchSize]

        // Target: r + gamma * max Q'(s', a') if not done
        targetNet->eval();
        auto nextQValues = targetNet->forward(nextStateBatch); // [batchSize, outDim]
        auto maxNextQ = std::get<0>(nextQValues.max(1));       // [batchSize]

        auto mask = (doneTensor == false).to(torch::kFloat32);
        auto target = rewardTensor + gamma * maxNextQ * mask;

        // Loss = MSE( Q_chosen, target )
        auto loss = torch::nn::functional::mse_loss(qChosen, target.detach());

        // Backprop
        optimizer->zero_grad();
        loss.backward();
        optimizer->step();

        // Epsilon decay
        if (epsilon > epsilonMin)
        {
            epsilon -= epsilonDecay;
            if (epsilon < epsilonMin)
                epsilon = epsilonMin;
        }

        // Update target net every so often
        steps++;
        if (steps % targetUpdateInterval == 0)
        {
            syncTargetNet();
        }
    }

    // Save the learned model (both online and target net, plus optimizer if you want)
    void saveModel(const std::string &baseFilename)
    {
        // Save QNet
        torch::save(qNet, baseFilename + "/_online.pt");
        // Save target net
        torch::save(targetNet, baseFilename + "/_target.pt");
        // Optionally save optimizer state
        torch::save(*optimizer, baseFilename + "/_optim.pt");
        std::cout << "Model saved to " << baseFilename << "/_online.pt _target.pt _optim.pt\n";
    }

    // Load the model (and optionally the optimizer)
    void loadModel(const std::string &baseFilename)
    {
        // Load QNet
        torch::load(qNet, baseFilename + "/_online.pt");
        // Load target net
        torch::load(targetNet, baseFilename + "/_target.pt");
        // Optionally load optimizer
        try
        {
            torch::load(*optimizer, baseFilename + "/_optim.pt");
        }
        catch (...)
        {
            std::cerr << "Warning: Could not load optimizer state.\n";
        }
        // Sync target net if needed
        syncTargetNet();
        std::cout << "Model loaded from " << baseFilename << "/_online.pt _target.pt _optim.pt\n";
    }

    size_t getBatchSize() const
    {
        return batchSize;
    }

    void setBatchSize(size_t newSize)
    {
        batchSize = newSize;
    }

private:
    // Copy qNet weights to targetNet
    void syncTargetNet()
    {
        std::stringstream stream;
        torch::save(qNet, stream);
        torch::load(targetNet, stream);
    }

    // Example placeholders for a simulator integration:
    torch::Tensor getInitialSimState()
    {
        // Return e.g. a [1, inDim] random state
        return torch::rand({1, inDim}).to(device);
    }

    torch::Tensor stepSimulator(const torch::Tensor &state, int action, float &reward, bool &done)
    {
        // This is pseudo-code. Replace with your simulator logic.
        // For demonstration, random nextState, random reward
        auto nextState = torch::rand({1, inDim}).to(device);
        reward = -1.0;                                   // example
        done = (torch::rand({1}).item<float>() > 0.95f); // example
        return nextState;
    }

public:
    float epsilon;    // exploration rate
    float epsilonMin; // minimal exploration
    float epsilonDecay;

private:
    int inDim;
    int outDim;
    float gamma;
    size_t batchSize;
    int targetUpdateInterval;
    long steps = 0;

    // Networks
    QNet qNet;      // online Q
    QNet targetNet; // target Q

    // Replay buffer
    std::unique_ptr<ReplayBuffer> replayBuffer;

    // Optimizer
    std::unique_ptr<torch::optim::Adam> optimizer;

    // Device and RNG
    torch::Device device;
    std::mt19937 rng{std::random_device{}()};
};