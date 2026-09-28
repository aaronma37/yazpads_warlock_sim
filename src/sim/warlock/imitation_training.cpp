#include "imitation_training.hpp"
#include "viper_oracle.hpp"
#include "build_export.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <stdexcept>

namespace warlock {
namespace {
using Selection = WarlockSimulator::DecisionSelection;
using Controller = WarlockSimulator::DecisionController;
using Cancelled = sim::ParallelCancelled;
PriorityRule action_rule(PriorityAction action) {
    PriorityRule rule;
    rule.action = action;
    rule.spell_id = VIPEROracle::get_spell_id(action);
    rule.name = VIPEROracle::get_action_name(action);
    return rule;
}
Selection force_action(PriorityAction action, Selection fallback, const WarlockSimulator& simulator) {
    if (fallback.rules.empty()) fallback.rules = simulator.policy.get_priority_rules(simulator.talents, simulator.race);
    fallback.rules.insert(fallback.rules.begin(), action_rule(action));
    fallback.force_first = true;
    return fallback;
}
WarlockSimulator clean_simulator(WarlockSimulator simulator) {
    simulator.neural_decision = {};
    simulator.decision_controller = {};
    simulator.forced_action_prefix.clear();
    simulator.use_oracle_execution_policy = simulator.policy.use_oracle_execution_policy = false;
    simulator.use_gbdt_policy = simulator.policy.use_gbdt_policy = false;
    simulator.policy.use_imitation_policy = false;
    simulator.record_timeline = simulator.record_viper_samples = false;
    return simulator;
}
double mean(const std::vector<double>& values) {
    return values.empty() ? 0 : std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}
double standard_error(const std::vector<double>& values) {
    if (values.size() < 2) return 0;
    double average = mean(values), sum = 0;
    for (double value : values) sum += (value - average) * (value - average);
    return std::sqrt(sum / (values.size() * (values.size() - 1)));
}
void checked_write(const std::filesystem::path& path, const std::string& value) {
    std::ofstream output(path);
    output << value;
    if (!output) throw std::runtime_error("Cannot write " + path.string());
}
}

SearchResult search_imitation_actions(const WarlockSimulator& simulator, uint64_t episode_seed,
    const std::vector<Selection>& prefix, const sim::SimObservation& state, int rollouts,
    uint64_t future_seed, const Controller& continuation, const std::atomic<bool>& cancel,
    int search_depth, double exploration, sim::ParallelExecutor* executor) {
    if (rollouts < 2) throw std::runtime_error("Search requires at least two rollouts per action");
    if (search_depth < 1 || exploration < 0 || !std::isfinite(exploration)) throw std::runtime_error("Invalid MCTS parameters");
    SearchResult output;
    std::vector<SearchResult> branches(neural_actions.size());
    auto evaluate_action = [&](size_t index) {
        auto action = neural_actions[index];
        auto& branch = branches[index];
        if (!VIPEROracle::is_action_available_for_talents(action, simulator.talents)) return;
        std::vector<double> samples;
        // Open-loop UCT: descendant nodes represent attempted action sequences.
        // Stochastic outcomes share nodes; execution retains eligibility/fallback rules.
        struct UCTNode { int visits = 0; double total = 0; std::vector<std::pair<PriorityAction, size_t>> children; };
        std::vector<UCTNode> tree(1);
        bool executable = true;
        for (int trial = 0; trial < rollouts; ++trial) {
            if (cancel) throw Cancelled{};
            auto replay = clean_simulator(simulator);
            bool reached = false;
            bool expanded = false;
            size_t parent = 0;
            std::vector<size_t> visited{0};
            replay.decision_controller = [&](const sim::SimObservation& observation, size_t step, FastRNG& rng) {
                if (cancel) throw Cancelled{};
                if (step < prefix.size()) return prefix[step];
                Selection fallback = continuation ? continuation(observation, step, rng) : Selection{};
                if (step == prefix.size()) {
                    if (observation.to_array() != state.to_array())
                        throw std::runtime_error("Search replay did not reproduce the decision state");
                    reached = true;
                    // Every branch shares its complete past and trial's future RNG.
                    rng = FastRNG(future_seed + static_cast<uint64_t>(trial) * 7919);
                    return force_action(action, std::move(fallback), replay);
                }
                if (step < prefix.size() + static_cast<size_t>(search_depth) && !expanded) {
                    std::vector<PriorityAction> candidates;
                    auto add_candidate = [&](PriorityAction candidate) {
                        if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end() &&
                            VIPEROracle::is_action_available_for_talents(candidate, replay.talents) &&
                            VIPEROracle::is_action_legal(candidate, observation, replay.talents))
                            candidates.push_back(candidate);
                    };
                    for (const auto& rule : fallback.rules) add_candidate(rule.action);
                    for (auto candidate : neural_actions) add_candidate(candidate);
                    double best_score = -1e100;
                    size_t chosen = 0;
                    PriorityAction preferred = PriorityAction::SHADOW_BOLT_FILLER;
                    for (auto candidate : candidates) {
                        auto child = std::find_if(tree[parent].children.begin(), tree[parent].children.end(),
                            [&](const auto& item) { return item.first == candidate; });
                        if (child == tree[parent].children.end()) {
                            chosen = tree.size();
                            tree[parent].children.emplace_back(candidate, chosen);
                            tree.push_back({});
                            preferred = candidate;
                            expanded = true; // one expansion per trial, then rollout continuation
                            break;
                        }
                        const auto& node = tree[child->second];
                        const double score = node.visits == 0 ? 1e99 : node.total / node.visits +
                            exploration * std::sqrt(std::log(tree[parent].visits + 1.0) / node.visits);
                        if (score > best_score) { best_score = score; chosen = child->second; preferred = candidate; }
                    }
                    if (!candidates.empty()) {
                        parent = chosen;
                        visited.push_back(chosen);
                        return force_action(preferred, std::move(fallback), replay);
                    }
                }
                return fallback;
            };
            FastRNG rng(episode_seed);
            auto result = replay.run_single_simulation(rng);
            ++branch.replay_count;
            if (!reached || result.action_history.size() <= prefix.size() || result.action_history[prefix.size()] != action) {
                executable = false;
                break;
            }
            samples.push_back(result.dps);
            for (auto node : visited) {
                ++tree[node].visits;
                tree[node].total += result.dps / 1000.0;
            }
        }
        if (executable && !samples.empty())
            branch.values.push_back({action, mean(samples), standard_error(samples), static_cast<int>(samples.size())});
    };
    if (executor) executor->parallel_for(branches.size(), evaluate_action);
    else for (size_t i = 0; i < branches.size(); ++i) evaluate_action(i);
    for (const auto& branch : branches) {
        output.replay_count += branch.replay_count;
        output.values.insert(output.values.end(), branch.values.begin(), branch.values.end());
    }
    std::stable_sort(output.values.begin(), output.values.end(), [](auto& a, auto& b) { return a.dps > b.dps; });
    return output;
}

void SearchImitationPolicy::fit(const std::vector<sim::GBDTMultiActionQPolicy::QSample>& samples, const sim::GBDTConfig& config,
    sim::ParallelExecutor* executor, const std::atomic<bool>* cancel) {
    sim::GBDTMultiActionQPolicy native(config);
    native.fit(samples, executor, cancel);
    SearchImitationPolicy fitted;
    for (auto action : neural_actions) {
        const auto it = native.models().find(static_cast<uint8_t>(action));
        if (it == native.models().end()) continue;
        size_t index = fitted.classes.size();
        fitted.classes.push_back(action);
        fitted.trees.push_back({index, {{-1, -1, -1, 0, it->second.base_score()}}});
        for (const auto& source : it->second.trees()) {
            Tree tree;
            tree.class_index = index;
            for (const auto& node : source.nodes)
                tree.nodes.push_back({node.is_leaf ? -1 : static_cast<int>(node.feature_index), node.left_child,
                    node.right_child, node.threshold, node.leaf_value * config.learning_rate});
            fitted.trees.push_back(std::move(tree));
        }
    }
    if (fitted.classes.size() < 2) throw std::runtime_error("Search needs at least two executable actions to fit a policy");
    *this = std::move(fitted);
}

std::string SearchImitationPolicy::serialize() const {
    std::ostringstream output;
    output << std::setprecision(17) << "WARLOCK_GBDT_V1 " << sim::SimObservation::FEATURE_COUNT << ' '
        << classes.size() << ' ' << trees.size() << '\n';
    for (const auto& name : sim::SimObservation::feature_names()) output << name << ' ';
    output << '\n';
    for (auto action : classes) output << static_cast<int>(action) << ' ';
    output << '\n';
    for (const auto& tree : trees) {
        output << tree.class_index << ' ' << tree.nodes.size() << '\n';
        for (const auto& node : tree.nodes)
            output << node.feature << ' ' << node.left << ' ' << node.right << ' ' << node.threshold << ' ' << node.value << '\n';
    }
    return output.str();
}

void SearchImitationPolicy::save(const std::string& path) const { checked_write(path, serialize()); }

void SearchImitationPolicy::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open GBDT policy: " + path);
    std::ostringstream contents;
    contents << input.rdbuf();
    deserialize(contents.str());
}

void SearchImitationPolicy::deserialize(const std::string& text) {
    std::istringstream input(text);
    SearchImitationPolicy loaded;
    std::string magic, name;
    size_t features = 0, class_count = 0, tree_count = 0;
    input >> magic >> features >> class_count >> tree_count;
    if (magic != "WARLOCK_GBDT_V1" || features != sim::SimObservation::FEATURE_COUNT ||
        class_count < 2 || class_count > neural_actions.size() || tree_count > 100000 || tree_count == 0)
        throw std::runtime_error("Invalid GBDT policy header");
    for (const auto& expected : sim::SimObservation::feature_names()) {
        input >> name;
        if (name != expected) throw std::runtime_error("GBDT feature schema mismatch");
    }
    for (size_t c = 0; c < class_count; ++c) {
        int action = -1;
        input >> action;
        auto found = std::find_if(neural_actions.begin(), neural_actions.end(), [&](auto a) { return static_cast<int>(a) == action; });
        if (found == neural_actions.end() || std::find(loaded.classes.begin(), loaded.classes.end(), *found) != loaded.classes.end())
            throw std::runtime_error("Invalid GBDT action mapping");
        loaded.classes.push_back(*found);
    }
    for (size_t t = 0; t < tree_count; ++t) {
        Tree tree;
        size_t nodes = 0;
        input >> tree.class_index >> nodes;
        if (tree.class_index >= class_count || nodes == 0 || nodes > 65535) throw std::runtime_error("Invalid GBDT tree");
        tree.nodes.resize(nodes);
        for (auto& node : tree.nodes) input >> node.feature >> node.left >> node.right >> node.threshold >> node.value;
        for (size_t n = 0; n < nodes; ++n) {
            const auto& node = tree.nodes[n];
            if (!std::isfinite(node.threshold) || !std::isfinite(node.value) || node.feature < -1 ||
                node.feature >= static_cast<int>(features) || (node.feature >= 0 &&
                (node.left <= static_cast<int>(n) || node.right <= static_cast<int>(n) ||
                 node.left >= static_cast<int>(nodes) || node.right >= static_cast<int>(nodes))))
                throw std::runtime_error("Invalid GBDT node");
        }
        loaded.trees.push_back(std::move(tree));
    }
    if (!input) throw std::runtime_error("Truncated GBDT policy");
    *this = std::move(loaded);
}

std::vector<std::pair<PriorityAction, double>> SearchImitationPolicy::ranked_values(const sim::SimObservation& state) const {
    if (classes.empty()) return {};
    const auto features = state.to_array();
    std::vector<double> scores(classes.size());
    for (const auto& tree : trees) {
        int index = 0;
        while (tree.nodes[index].feature >= 0) {
            const auto& node = tree.nodes[index];
            index = features[node.feature] <= node.threshold ? node.left : node.right;
        }
        scores[tree.class_index] += tree.nodes[index].value;
    }
    std::vector<std::pair<PriorityAction, double>> values;
    for (size_t i = 0; i < classes.size(); ++i) values.emplace_back(classes[i], scores[i]);
    std::stable_sort(values.begin(), values.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    return values;
}

Selection SearchImitationPolicy::select(const sim::SimObservation& state) const {
    if (classes.empty()) return {};
    Selection selection;
    selection.force_all = true;
    for (const auto& [action, value] : ranked_values(state)) selection.rules.push_back(action_rule(action));
    // Unseen actions remain fallback options; simulator checks actual resources/cooldowns.
    for (auto action : neural_actions)
        if (std::find(classes.begin(), classes.end(), action) == classes.end()) selection.rules.push_back(action_rule(action));
    return selection;
}

void activate_imitation_policy(PolicyConfig& policy, std::shared_ptr<const SearchImitationPolicy> model,
    const std::string& name) {
    if (!model || model->classes.size() < 2 || model->trees.empty()) throw std::runtime_error("No fitted GBDT policy to activate");
    policy.imitation_policy = std::move(model);
    policy.imitation_policy_name = name;
    policy.use_imitation_policy = true;
    policy.use_gbdt_policy = policy.use_oracle_execution_policy = false;
}

void activate_imitation_policy(WarlockSimulator& simulator, std::shared_ptr<const SearchImitationPolicy> model,
    const std::string& name) {
    activate_imitation_policy(simulator.policy, std::move(model), name);
    simulator.use_gbdt_policy = simulator.use_oracle_execution_policy = false;
    simulator.neural_decision = {};
    simulator.decision_controller = {};
    simulator.forced_action_prefix.clear();
}

void load_imitation_policy(WarlockSimulator& simulator, const std::string& path) {
    std::filesystem::path file(path);
    if (std::filesystem::is_directory(file)) file /= "policy.gbdt";
    auto loaded = std::make_shared<SearchImitationPolicy>();
    loaded->load(file.string());
    activate_imitation_policy(simulator, std::move(loaded), file.string());
}

std::shared_ptr<const SearchImitationPolicy> ImitationTrainer::policy_snapshot() const {
    if (!has_policy()) throw std::runtime_error("No fitted GBDT policy to activate");
    return std::make_shared<const SearchImitationPolicy>(model);
}

ImitationTrainer::ImitationTrainer(const WarlockSimulator& simulator) : snapshot(clean_simulator(simulator)) {
    auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    directory = std::filesystem::temp_directory_path() / ("warlock-imitation-" + std::to_string(stamp));
    if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Cannot create imitation working directory");
    checked_write(directory / "build.json", build_export::export_build_json(snapshot));
}
ImitationTrainer::~ImitationTrainer() {
    std::error_code error;
    std::filesystem::remove_all(directory, error);
}

void ImitationTrainer::train(const ImitationConfig& config, const std::atomic<bool>& cancel,
    const std::function<void(const ImitationProgress&)>& callback) {
    if (config.threads < 0 || config.threads > 256 || config.rounds < 1 || config.episodes < 2 || config.rollouts < 2 || config.reference_rollouts < 2 ||
        config.search_depth < 1 || !std::isfinite(config.exploration) || config.exploration < 0 || config.sample_stride < 1 ||
        config.samples_per_episode < 1 || config.evaluation_episodes < 2 || config.trees < 1 ||
        config.max_depth < 1 || config.max_depth > 15 || !std::isfinite(config.min_child_weight) || config.min_child_weight < 0 ||
        !std::isfinite(config.l2_reg) || config.l2_reg < 0 ||
        !std::isfinite(config.learning_rate) || config.learning_rate <= 0 || config.learning_rate > 1 ||
        !std::isfinite(config.teacher_mix) || config.teacher_mix < 0 || config.teacher_mix > 1)
        throw std::runtime_error("Invalid imitation training configuration");
    const auto hardware_threads = std::max(1u, std::thread::hardware_concurrency());
    sim::ParallelExecutor executor(config.threads == 0 ? std::min(256u, hardware_threads) : static_cast<unsigned>(config.threads));
    std::ostringstream parameters;
    parameters << "{\"backend\":\"native_cpp_gbdt\",\"trees\":" << config.trees
        << ",\"max_depth\":" << config.max_depth << ",\"min_child_weight\":" << config.min_child_weight
        << ",\"l2_reg\":" << config.l2_reg << ",\"learning_rate\":" << config.learning_rate
        << ",\"seed\":" << config.seed << ",\"rounds\":" << config.rounds << ",\"episodes\":" << config.episodes
        << ",\"sample_stride\":" << config.sample_stride << ",\"samples_per_episode\":" << config.samples_per_episode
        << ",\"rollouts\":" << config.rollouts << ",\"evaluation_episodes\":" << config.evaluation_episodes
        << ",\"search_depth\":" << config.search_depth << ",\"exploration\":" << config.exploration
        << ",\"reference_rollouts\":" << config.reference_rollouts
        << ",\"teacher_mix\":" << config.teacher_mix
        << ",\"threads\":" << config.threads << ",\"resolved_threads\":" << executor.concurrency() << "}";
    checked_write(directory / "config.json", parameters.str());
    std::ofstream dataset(directory / "samples.csv");
    dataset << "episode,action,relative_q,weight";
    for (auto& name : sim::SimObservation::feature_names()) dataset << ',' << name;
    dataset << '\n' << std::setprecision(9);
    ImitationProgress progress;
    progress.threads = executor.concurrency();
    std::mutex progress_mutex;
    const auto run_started = std::chrono::steady_clock::now();
    auto phase_started = run_started, last_publish = run_started;
    int phase_replays = 0;
    struct Example { sim::SimObservation state; std::vector<SearchActionValue> values; double weight; int episode; };
    std::vector<Example> examples;
    auto publish_locked = [&](bool force) {
        const auto now = std::chrono::steady_clock::now();
        if (!force && now - last_publish < std::chrono::milliseconds(100)) return;
        last_publish = now;
        progress.elapsed_seconds = std::chrono::duration<double>(now - run_started).count();
        progress.phase_seconds = std::chrono::duration<double>(now - phase_started).count();
        progress.fights_per_second = progress.phase_seconds > 0 ? progress.episodes_done / progress.phase_seconds : 0;
        progress.search_replays_per_second = progress.phase_seconds > 0 ?
            (progress.search_replays - phase_replays) / progress.phase_seconds : 0;
        if (callback) callback(progress);
    };
    auto publish = [&] { std::lock_guard lock(progress_mutex); publish_locked(true); };
    auto report_work = [&](int labels, int replays, int fights) {
        std::lock_guard lock(progress_mutex);
        progress.samples += labels;
        progress.search_replays += replays;
        progress.episodes_done += fights;
        publish_locked(false);
    };
    auto begin_phase = [&](const std::string& phase) {
        std::lock_guard lock(progress_mutex);
        progress.phase = phase;
        progress.episodes_done = 0;
        phase_replays = progress.search_replays;
        phase_started = std::chrono::steady_clock::now();
        publish_locked(true);
    };
    auto learner = [&](const sim::SimObservation& obs, size_t, FastRNG&) { return model.select(obs); };
    try {
        for (int round = 0; round < config.rounds; ++round) {
            progress.round = round + 1;
            begin_phase("Collecting exact-state search labels");
            std::vector<std::vector<Example>> episode_examples(config.episodes);
            executor.parallel_for(config.episodes, [&](size_t episode_index) {
                const int episode = static_cast<int>(episode_index);
                if (cancel) throw Cancelled{};
                uint64_t seed = static_cast<uint64_t>(config.seed) + 1000003ULL * (round * config.episodes + episode + 1);
                FastRNG mixture_rng(seed ^ 0xabc987);
                std::vector<Selection> prefix;
                int collected = 0;
                auto sim = snapshot;
                sim.decision_controller = [&](const sim::SimObservation& obs, size_t step, FastRNG& rng) {
                    if (cancel) throw Cancelled{};
                    auto selection = learner(obs, step, rng);
                    if (step % config.sample_stride == 0 && collected < config.samples_per_episode) {
                        auto search = search_imitation_actions(snapshot, seed, prefix, obs, config.rollouts,
                            seed ^ (0x123456789ULL + step * 104729), learner, cancel, config.search_depth, config.exploration, &executor);
                        if (!search.values.empty()) {
                            auto& best = search.values.front();
                            double weight = 1;
                            if (search.values.size() > 1) {
                                auto& next = search.values[1];
                                double gap = best.dps - next.dps;
                                double uncertainty = std::hypot(best.stderr_dps, next.stderr_dps);
                                weight = std::clamp(gap / (1 + uncertainty), 0.1, 100.0);
                            }
                            int group = round * config.episodes + episode;
                            episode_examples[episode_index].push_back({obs, search.values, weight, group});
                            ++collected;
                            if (round == 0 || mixture_rng.next_double() < config.teacher_mix)
                                selection = force_action(best.action, std::move(selection), snapshot);
                        }
                        report_work(search.values.empty() ? 0 : 1, search.replay_count, 0);
                    }
                    prefix.push_back(selection);
                    return selection;
                };
                FastRNG rng(seed);
                sim.run_single_simulation(rng);
                report_work(0, 0, 1);
            });
            // Merge by episode/decision ID, independent of task completion order.
            for (auto& episode : episode_examples) {
                for (auto& example : episode) {
                    for (const auto& value : example.values) {
                        dataset << example.episode << ',' << static_cast<int>(value.action) << ','
                            << value.dps - example.values.front().dps << ',' << example.weight;
                        for (float feature : example.state.to_array()) dataset << ',' << feature;
                        dataset << '\n';
                    }
                    examples.push_back(std::move(example));
                }
            }
            const double collection_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - phase_started).count();
            progress.collection_seconds += collection_seconds;
            dataset.flush();
            if (!dataset) throw std::runtime_error("Cannot write imitation dataset");
            if (cancel) throw Cancelled{};
            begin_phase("Fitting native C++ GBDT to search action values");
            std::vector<sim::GBDTMultiActionQPolicy::QSample> training_samples;
            for (const auto& example : examples) {
                // Whole episodes are reserved for validation, rather than adjacent states.
                if (example.episode % 5 == 0) continue;
                for (const auto& value : example.values)
                    training_samples.push_back({example.state, static_cast<uint8_t>(value.action),
                        static_cast<float>(value.dps - example.values.front().dps), static_cast<float>(example.weight)});
            }
            sim::GBDTConfig tree_config;
            tree_config.num_trees = config.trees;
            tree_config.max_depth = config.max_depth;
            tree_config.learning_rate = config.learning_rate;
            tree_config.min_child_weight = config.min_child_weight;
            tree_config.l2_reg = config.l2_reg;
            model.fit(training_samples, tree_config, &executor, &cancel);
            model.save((directory / "policy.gbdt").string());
            const double fit_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - phase_started).count();
            progress.fit_seconds += fit_seconds;
            if (cancel) throw Cancelled{};
            begin_phase("Evaluating policy, APL and online search on held-out fights");
            ImitationRound metrics;
            metrics.round = round + 1;
            metrics.samples = progress.samples;
            size_t validation_count = 0, validation_correct = 0;
            for (const auto& example : examples) {
                if (example.episode % 5 != 0) continue;
                const auto selection = model.select(example.state);
                for (const auto& rule : selection.rules) {
                    if (std::any_of(example.values.begin(), example.values.end(), [&](const auto& v) { return v.action == rule.action; })) {
                        ++validation_count;
                        if (rule.action == example.values.front().action) ++validation_correct;
                        break;
                    }
                }
            }
            metrics.validation_accuracy_pct = validation_count ? 100.0 * validation_correct / validation_count : 0;
            std::vector<double> policy_values(config.evaluation_episodes), apl_values(config.evaluation_episodes),
                teacher_values(config.evaluation_episodes), paired_gaps(config.evaluation_episodes);
            struct EvaluationStats { size_t compared = 0, agreed = 0; double regret = 0; };
            std::vector<EvaluationStats> evaluation_stats(config.evaluation_episodes);
            executor.parallel_for(config.evaluation_episodes, [&](size_t ep) {
                if (cancel) throw Cancelled{};
                uint64_t seed = (static_cast<uint64_t>(config.seed) << 32) ^ (0xe7a10000ULL + ep * 65537ULL);
                auto baseline = snapshot;
                FastRNG apl_rng(seed);
                apl_values[ep] = baseline.run_single_simulation(apl_rng).dps;
                auto policy = snapshot;
                policy.decision_controller = learner;
                FastRNG policy_rng(seed);
                policy_values[ep] = policy.run_single_simulation(policy_rng).dps;
                auto teacher = snapshot;
                std::vector<Selection> prefix;
                teacher.decision_controller = [&](const sim::SimObservation& obs, size_t step, FastRNG& rng) {
                    if (cancel) throw Cancelled{};
                    auto selection = learner(obs, step, rng);
                    auto search = search_imitation_actions(snapshot, seed, prefix, obs, config.reference_rollouts,
                        seed ^ (0xbadcafeULL + step * 104729), learner, cancel, config.search_depth, config.exploration, &executor);
                    if (!search.values.empty()) {
                        auto predicted = std::find_if(selection.rules.begin(), selection.rules.end(), [&](const auto& rule) {
                            return std::any_of(search.values.begin(), search.values.end(), [&](auto& value) { return value.action == rule.action; });
                        });
                        if (predicted != selection.rules.end()) {
                            ++evaluation_stats[ep].compared;
                            if (predicted->action == search.values.front().action) ++evaluation_stats[ep].agreed;
                            for (auto& value : search.values) if (value.action == predicted->action)
                                evaluation_stats[ep].regret += search.values.front().dps - value.dps;
                        }
                        selection = force_action(search.values.front().action, std::move(selection), snapshot);
                    }
                    prefix.push_back(selection);
                    report_work(0, search.replay_count, 0);
                    return selection;
                };
                FastRNG teacher_rng(seed);
                teacher_values[ep] = teacher.run_single_simulation(teacher_rng).dps;
                paired_gaps[ep] = policy_values[ep] - teacher_values[ep];
                report_work(0, 0, 1);
            });
            const double evaluation_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - phase_started).count();
            progress.evaluation_seconds += evaluation_seconds;
            metrics.collection_seconds = collection_seconds;
            metrics.fit_seconds = fit_seconds;
            metrics.evaluation_seconds = evaluation_seconds;
            size_t compared = 0, agreed = 0;
            double regret = 0;
            for (const auto& stats : evaluation_stats) {
                compared += stats.compared; agreed += stats.agreed; regret += stats.regret;
            }
            metrics.policy_dps = mean(policy_values);
            metrics.apl_dps = mean(apl_values);
            metrics.teacher_dps = mean(teacher_values);
            metrics.policy_minus_teacher = mean(paired_gaps);
            // Conservative t approximation for small evaluation batches.
            double critical = config.evaluation_episodes == 2 ? 12.706 :
                config.evaluation_episodes <= 5 ? 4.303 : config.evaluation_episodes <= 10 ? 2.571 :
                config.evaluation_episodes <= 30 ? 2.262 : 2.042;
            metrics.gap_ci95 = critical * standard_error(paired_gaps);
            metrics.agreement_pct = compared ? 100.0 * agreed / compared : 0;
            metrics.regret_dps = compared ? regret / compared : 0;
            metrics.search_replays = progress.search_replays;
            progress.history.push_back(metrics);
            std::ostringstream report;
            report << "round,labels,search_replays,policy_dps,apl_dps,teacher_dps,policy_minus_teacher,gap_ci95,agreement_pct,regret_dps,validation_accuracy_pct,collection_seconds,fit_seconds,evaluation_seconds\n";
            for (const auto& row : progress.history)
                report << row.round << ',' << row.samples << ',' << row.search_replays << ',' << row.policy_dps << ','
                    << row.apl_dps << ',' << row.teacher_dps << ',' << row.policy_minus_teacher << ',' << row.gap_ci95 << ','
                    << row.agreement_pct << ',' << row.regret_dps << ',' << row.validation_accuracy_pct << ','
                    << row.collection_seconds << ',' << row.fit_seconds << ',' << row.evaluation_seconds << '\n';
            checked_write(directory / "metrics.csv", report.str());
            publish();
        }
        progress.phase = "Finished";
    } catch (const Cancelled&) {
        progress.phase = "Stopped; last completed model is available";
    }
    publish();
}

void ImitationTrainer::save_policy(const std::string& path) const {
    if (!has_policy()) throw std::runtime_error("No fitted GBDT policy to save");
    // Save a directory bundle: native inference, data and build provenance.
    if (!std::filesystem::create_directory(path)) throw std::runtime_error("Choose a new bundle directory; destination exists");
    for (const char* name : {"policy.gbdt", "config.json", "build.json", "samples.csv", "metrics.csv"})
        if (std::filesystem::exists(directory / name))
            std::filesystem::copy_file(directory / name, std::filesystem::path(path) / name);
    auto deployed = snapshot;
    activate_imitation_policy(deployed, policy_snapshot(), "Search imitation GBDT");
    checked_write(std::filesystem::path(path) / "deployment.json", build_export::export_build_json(deployed));
}
}
