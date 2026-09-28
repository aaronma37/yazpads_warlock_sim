#include "ppo_training.hpp"
#include <condition_variable>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <rl_tools/operations/cpu.h>
#include <rl_tools/nn/optimizers/adam/instance/operations_generic.h>
#include <rl_tools/nn/layers/gru/operations_generic.h>
#include <rl_tools/rl/environments/environments.h>

namespace warlock::ppo_detail {
struct Transition {
    static constexpr size_t DIM = 24;
    NeuralObservation observation{};
    double damage = 0;
    double time = 0;
    bool done = false;
};

// Suspend the existing DES at decision boundaries without replaying prefixes or
// copying its event queue. One owned worker per environment; all exchanges are
// synchronized and destruction wakes and joins a suspended simulation.
class CombatSession {
    struct Stopped {};
    WarlockSimulator source;
    std::thread worker;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopping = false, ready = false, action_ready = false;
    Transition current;
    NeuralAction action{};
    std::exception_ptr error;
public:
    explicit CombatSession(const WarlockSimulator& sim) : source(sim) {}
    ~CombatSession() { stop(); }
    void stop() {
        { std::lock_guard lock(mutex); stopping = true; }
        cv.notify_all();
        if (worker.joinable()) worker.join();
    }
    Transition reset(unsigned seed) {
        stop();
        { std::lock_guard lock(mutex);
          stopping = ready = action_ready = false; current = {}; error = nullptr; }
        worker = std::thread([this, seed] {
            try {
                auto sim = source;
                sim.record_viper_samples = false;
                sim.record_timeline = false;
                sim.neural_decision = [this](const NeuralObservation& observation, double damage, double time) {
                    std::unique_lock lock(mutex);
                    current = {observation, damage, time, false};
                    ready = true;
                    cv.notify_all();
                    cv.wait(lock, [&] { return action_ready || stopping; });
                    if (stopping) throw Stopped{};
                    action_ready = false;
                    return action;
                };
                FastRNG rng(seed);
                const auto result = sim.run_single_simulation(rng);
                { std::lock_guard lock(mutex);
                  current.damage = result.total_damage; current.time = result.duration;
                  current.done = true; ready = true; }
            } catch (const Stopped&) {
                return;
            } catch (...) {
                std::lock_guard lock(mutex);
                error = std::current_exception(); ready = true;
            }
            cv.notify_all();
        });
        return wait();
    }
    Transition wait() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return ready; });
        if (error) std::rethrow_exception(error);
        return current;
    }
    Transition advance(const NeuralAction& value) {
        { std::lock_guard lock(mutex);
          action = value; ready = false; action_ready = true; }
        cv.notify_all();
        return wait();
    }
};

struct Environment : rl_tools::rl::environments::Environment<float, size_t> {
    using T = float;
    using TI = size_t;
    using State = Transition;
    struct Parameters {};
    struct Observation { static constexpr size_t DIM = 24; };
    using ObservationPrivileged = Observation;
    static constexpr size_t N_AGENTS = 1;
    static constexpr size_t ACTION_DIM = neural_actions.size();
    static constexpr size_t EPISODE_STEP_LIMIT = 4096;
    CombatSession* session;
};
}

// Register environment operations before including PPO's dependent templates.
namespace rl_tools {
using CombatEnvironment = warlock::ppo_detail::Environment;
template<class D> void malloc(D&, CombatEnvironment&) {}
template<class D> void free(D&, CombatEnvironment&) {}
template<class D> void init(D&, CombatEnvironment&) {}
template<class D, class R> void sample_initial_parameters(D&, const CombatEnvironment&, CombatEnvironment::Parameters&, R&) {}
template<class D, class R> void sample_initial_state(D& d, const CombatEnvironment& e, CombatEnvironment::Parameters&, CombatEnvironment::State& s, R& rng) {
    s = e.session->reset(random::uniform_int_distribution(d.random, 0u, 0xffffffffu, rng));
}
template<class D, class A, class R> float step(D&, const CombatEnvironment& e, CombatEnvironment::Parameters&, const CombatEnvironment::State& s, const Matrix<A>& a, CombatEnvironment::State& next, R&) {
    warlock::NeuralAction scores;
    for (size_t i = 0; i < scores.size(); ++i) {
        scores[i] = get(a, 0, i);
        if (!std::isfinite(scores[i])) throw std::runtime_error("PPO produced a non-finite action");
    }
    next = e.session->advance(scores);
    return static_cast<float>(next.time - s.time);
}
template<class D, class A, class R> float reward(D&, const CombatEnvironment&, CombatEnvironment::Parameters&, const CombatEnvironment::State& s, const Matrix<A>&, const CombatEnvironment::State& next, R&) {
    return static_cast<float>((next.damage - s.damage) / 1000.0);
}
template<class D, class R> bool terminated(D&, const CombatEnvironment&, const CombatEnvironment::Parameters&, const CombatEnvironment::State& s, R&) { return s.done; }
template<class D, class O, class R> void observe(D&, const CombatEnvironment&, const CombatEnvironment::Parameters&, const CombatEnvironment::State& s, CombatEnvironment::Observation, Matrix<O>& out, R&) {
    for (size_t i = 0; i < s.observation.size(); ++i) set(out, 0, i, s.observation[i]);
}
}

#include <rl_tools/rl/algorithms/ppo/loop/core/operations_generic.h>
// Checkpoint validation must report an error to the UI, never abort the app.
namespace rl_tools::utils {
inline bool assert_exit(devices::DefaultCPU&, bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
    return condition;
}
}
#define RL_TOOLS_PERSIST_BACKENDS_TAR_OPERATIONS_CPU_NOT_INCLUDE_GENERIC
#include <rl_tools/persist/backends/tar/operations_cpu.h>
#include <rl_tools/persist/backends/tar/operations_posix.h>
#include <rl_tools/nn/layers/dense/persist.h>
#include <rl_tools/nn/layers/standardize/persist.h>
#include <rl_tools/nn/layers/gru/persist.h>
#include <rl_tools/nn_models/mlp/persist.h>
#include <rl_tools/nn_models/mlp_unconditional_stddev/persist.h>
#include <rl_tools/nn_models/sequential/persist.h>

namespace warlock {
namespace rlt = rl_tools;
namespace ppo_detail {
using Device = rlt::devices::DefaultCPU;
using TypePolicy = rlt::numeric_types::Policy<float>;
using RNG = Device::SPEC::RANDOM::ENGINE<>;
struct Parameters : rlt::rl::algorithms::ppo::loop::core::DefaultParameters<TypePolicy, size_t, Environment> {
    static constexpr size_t N_ENVIRONMENTS = 4;
    static constexpr size_t ON_POLICY_RUNNER_STEPS_PER_ENV = 128;
    static constexpr size_t BATCH_SIZE = N_ENVIRONMENTS * ON_POLICY_RUNNER_STEPS_PER_ENV;
    static constexpr size_t ACTOR_HIDDEN_DIM = 32, CRITIC_HIDDEN_DIM = 32;
    static constexpr size_t ACTOR_NUM_LAYERS = 2, CRITIC_NUM_LAYERS = 2;
    static constexpr bool NORMALIZE_OBSERVATIONS = false;
    struct PPO_PARAMETERS : rlt::rl::algorithms::ppo::DefaultParameters<TypePolicy, size_t, BATCH_SIZE> {
        static constexpr size_t N_EPOCHS = 1;
        static constexpr bool SHUFFLE_EPOCH = false;
        static constexpr bool STATEFUL_ACTOR_AND_CRITIC = true;
        static constexpr bool TRUNCATE_ON_EACH_ITERATION = true;
        static constexpr float GAMMA = 0.99f, LAMBDA = 0.95f;
        static constexpr float ACTION_ENTROPY_COEFFICIENT = 0.001f;
    };
};
using Config = rlt::rl::algorithms::ppo::loop::core::Config<TypePolicy, size_t, RNG, Environment, Parameters,
    rlt::rl::algorithms::ppo::loop::core::ConfigApproximatorsGRU<true>::template Approximators, true>;
using Loop = Config::State<Config>;
using Actor = Config::NN::ACTOR_TYPE;
using ForwardActor = Actor::CHANGE_CAPABILITY<rlt::nn::capability::Forward<true>>;
using EvalActor = ForwardActor::CHANGE_BATCH_SIZE<size_t, 1>::CHANGE_SEQUENCE_LENGTH<size_t, 1>;
struct ActorWeights {
    Device& device;
    EvalActor actor;
    explicit ActorWeights(Device& d) : device(d) { rlt::malloc(device, actor); }
    ~ActorWeights() { rlt::free(device, actor); }
};
struct Inference {
    Device device;
    EvalActor actor;
    EvalActor::State<true> state;
    EvalActor::Buffer<true> buffer;
    RNG rng;
    rlt::Matrix<rlt::matrix::Specification<float, size_t, 1, 24, false>> input;
    rlt::Matrix<rlt::matrix::Specification<float, size_t, 1, neural_actions.size(), false>> output;
    Inference(Device& source_device, const Actor& source_actor, const RNG& source_rng) : rng(source_rng) {
        rlt::malloc(device, actor); rlt::malloc(device, state); rlt::malloc(device, buffer);
        rlt::copy(source_device, device, source_actor, actor);
        rlt::reset(device, actor, state, rng);
    }
    ~Inference() {
        rlt::free(device, buffer); rlt::free(device, state); rlt::free(device, actor);
    }
    NeuralAction next(const NeuralObservation& obs) {
        for (size_t i = 0; i < obs.size(); ++i) rlt::set(input, 0, i, obs[i]);
        auto in = rlt::to_tensor(device, input);
        auto out = rlt::to_tensor(device, output);
        // Inference shape is one timestep, but memory must persist for the
        // entire fight. The default GRU mode would reset after every step.
        rlt::Mode<rlt::nn::layers::gru::NoAutoResetMode<rlt::mode::Default<>>> mode;
        rlt::evaluate_step(device, actor, in, state, out, buffer, rng, mode);
        NeuralAction action;
        for (size_t i = 0; i < action.size(); ++i) {
            action[i] = rlt::get(output, 0, i);
            if (!std::isfinite(action[i])) throw std::runtime_error("Policy produced a non-finite action during evaluation");
        }
        return action;
    }
};
}

struct PPOTrainer::Impl {
    WarlockSimulator source;
    ppo_detail::Device device;
    ppo_detail::Loop loop;
    std::array<std::unique_ptr<ppo_detail::CombatSession>, ppo_detail::Parameters::N_ENVIRONMENTS> sessions;
    explicit Impl(const WarlockSimulator& sim, unsigned seed) : source(sim) {
        source.neural_decision = {};
        source.record_viper_samples = false;
        source.record_timeline = false;
        source.forced_action_prefix.clear();
        rlt::malloc(device, loop);
        for (size_t i = 0; i < sessions.size(); ++i) {
            sessions[i] = std::make_unique<ppo_detail::CombatSession>(source);
            rlt::get_ref(device, loop.envs, i).session = sessions[i].get();
            rlt::get_ref(device, loop.env_parameters, i) = {};
        }
        rlt::init(device, loop, seed);
    }
    ~Impl() {
        for (auto& session : sessions) session->stop();
        // Upstream core free currently accesses envs after releasing its tensor.
        // These environments own no allocations; release their tensors last.
        rlt::free(device, loop.ppo); rlt::free(device, loop.ppo_buffers);
        rlt::free(device, loop.on_policy_runner_dataset); rlt::free(device, loop.on_policy_runner);
        rlt::free(device, loop.actor_eval_buffers); rlt::free(device, loop.actor_buffers);
        rlt::free(device, loop.critic_buffers); rlt::free(device, loop.critic_buffers_gae);
        rlt::free(device, loop.actor_optimizer); rlt::free(device, loop.critic_optimizer);
        rlt::free(device, loop.observations_dense);
        rlt::free(device, loop.observation_normalizer); rlt::free(device, loop.observation_privileged_normalizer);
        rlt::free(device, loop.envs); rlt::free(device, loop.env_parameters);
    }
};
PPOTrainer::PPOTrainer(const WarlockSimulator& sim, unsigned seed) : impl(std::make_unique<Impl>(sim, seed)) {
    if (!std::isfinite(sim.fight_duration) || sim.fight_duration <= 0 || sim.fight_duration > 1800)
        throw std::invalid_argument("Training requires a fight duration between 0 and 1800 seconds");
}
PPOTrainer::~PPOTrainer() = default;
void PPOTrainer::train(int updates, const std::atomic<bool>& cancel, const std::function<void(PPOTrainingProgress)>& progress) {
    if (updates < 1 || updates > 100000) throw std::invalid_argument("Updates must be between 1 and 100000");
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < updates && !cancel.load(); ++i) {
        rlt::step(impl->device, impl->loop);
        // Upstream recurrent train() accepts one full-sequence epoch per call.
        // Repeat on the SAME rollout, retaining its old log probabilities, GAE
        // targets and reset masks. Each call recomputes recurrent activations.
        for (int epoch = 1; epoch < 4; ++epoch) {
            auto& s = impl->loop;
            rlt::train(impl->device, s.ppo, s.on_policy_runner_dataset,
                s.actor_optimizer, s.critic_optimizer, s.ppo_buffers,
                s.actor_buffers, s.critic_buffers, s.rng);
        }
        double reward = 0;
        for (size_t j = 0; j < ppo_detail::Parameters::BATCH_SIZE; ++j) {
            const float value = rlt::get(impl->loop.on_policy_runner_dataset.rewards, j, 0);
            if (!std::isfinite(value)) throw std::runtime_error("Non-finite rollout reward");
            reward += value;
        }
        if (progress) progress({i + 1, (i + 1) * static_cast<int>(ppo_detail::Parameters::BATCH_SIZE),
            reward / ppo_detail::Parameters::BATCH_SIZE,
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()});
    }
}

NeuralDecision PPOTrainer::make_policy() {
    auto inference = std::make_shared<ppo_detail::Inference>(impl->device, impl->loop.ppo.actor, impl->loop.rng);
    return [inference](const NeuralObservation& obs, double, double) { return inference->next(obs); };
}
PPOEvaluation PPOTrainer::evaluate(int episodes, unsigned seed) {
    if (episodes < 1 || episodes > 10000) throw std::invalid_argument("Invalid evaluation episode count");
    PPOEvaluation result;
    for (int episode = 0; episode < episodes; ++episode) {
        auto sim = impl->source;
        FastRNG baseline_rng(seed + episode);
        result.baseline_dps += sim.run_single_simulation(baseline_rng).dps;
        sim.neural_decision = make_policy();
        FastRNG policy_rng(seed + episode);
        result.policy_dps += sim.run_single_simulation(policy_rng).dps;
    }
    result.policy_dps /= episodes; result.baseline_dps /= episodes;
    return result;
}
void PPOTrainer::save_policy(const std::string& path) {
    ppo_detail::ActorWeights weights(impl->device);
    rlt::copy(impl->device, impl->device, impl->loop.ppo.actor, weights.actor);
    rlt::persist::backends::tar::Writer writer;
    rlt::persist::backends::tar::WriterGroup<rlt::persist::backends::tar::WriterGroupSpecification<size_t, decltype(writer)>> root{"", &writer};
    auto group = rlt::create_group(impl->device, root, "warlock_ppo_gru_v1");
    rlt::save(impl->device, weights.actor, group);
    rlt::persist::backends::tar::finalize(impl->device, writer);
    std::ofstream file(path, std::ios::binary);
    file.write(writer.buffer.data(), writer.buffer.size());
    file.close();
    if (!file) throw std::runtime_error("Could not save policy: " + path);
}
void PPOTrainer::load_policy(const std::string& path) {
    using Data = rlt::persist::backends::tar::PosixFileData<size_t>;
    const auto close_file = [](FILE* f) { fclose(f); };
    std::unique_ptr<FILE, decltype(close_file)> file(fopen(path.c_str(), "rb"), close_file);
    if (!file) throw std::runtime_error("Could not open policy: " + path);
    Data data;
    data.f = file.get();
    if (fseek(data.f, 0, SEEK_END) != 0 || ftell(data.f) < 0) throw std::runtime_error("Cannot read checkpoint size");
    data.size = static_cast<size_t>(ftell(data.f)); rewind(data.f);
    rlt::persist::backends::tar::ReaderGroup<rlt::persist::backends::tar::ReaderGroupSpecification<size_t, Data>> root;
    root.data = data;
    auto group = rlt::get_group(impl->device, root, "warlock_ppo_gru_v1");
    ppo_detail::ActorWeights weights(impl->device);
    if (!rlt::load(impl->device, weights.actor, group)) throw std::runtime_error("Invalid PPO GRU policy checkpoint");
    // Only publish a completely loaded actor; failures leave the old policy intact.
    if (rlt::is_nan(impl->device, weights.actor)) throw std::runtime_error("Checkpoint contains invalid weights");
    rlt::copy(impl->device, impl->device, weights.actor, impl->loop.ppo.actor);
}
}
