#pragma once
#include <vector>
#include <string>
#include <functional>
#include <array>
#include "optimizer.hpp"
#include "talent_graph.hpp"
#include "surrogate_model.hpp"

namespace warlock {

struct GeneticOptimizerConfig {
    int population_size = 50;
    int generations = 400;
    int screening_sims = 400;
    int final_sims = 2500;
    int offspring_pool_size = 150;
    int simulated_offspring_per_gen = 30;
    int num_threads = 0;                    // 0 = Auto (std::thread::hardware_concurrency)
    double mutation_rate = 0.45;
    double crossover_rate = 0.70;
    double initial_exploration_rate = 0.50; // High exploration in early generations
    double min_exploration_rate = 0.15;     // Annealed down to avoid premature convergence
    bool seed_with_presets = true;
    bool optimize_race = true;              // Evolve and discover the optimal race for each build
    bool return_diverse_peaks = true;       // Use MAP-Elites quality-diversity archive to return distinct spec champions

    // User-specified constraints (up to 3 required talents, locked race, locked rotation, locked pet/DS mode)
    std::vector<int> required_talent_indices; // Node indices (0..51) that must be maxed
    int forced_race = -1;                     // -1 = unconstrained/evolve; 0..4 = locked to Race enum
    int forced_rotation = -1;                 // -1 = auto/unconstrained; 0..N = locked to RotationChoice
    int forced_pet_mode = -1;                 // -1 = auto/unconstrained; 0..6 = locked to PetConstraint
};

struct GeneticOptimizationSummary {
    std::vector<CandidateResult> top_candidates;
    std::vector<CandidateResult> diverse_peaks;
    std::vector<double> generation_best_dps;
    std::vector<double> generation_mean_dps;
    SurrogateModel trained_surrogate;
    int total_evaluations = 0;
};

class GeneticOptimizerSession {
public:
    GeneticOptimizerSession();
    ~GeneticOptimizerSession();
    GeneticOptimizerSession(GeneticOptimizerSession&&) noexcept;
    GeneticOptimizerSession& operator=(GeneticOptimizerSession&&) noexcept;

    void start(const WarlockSimulator& base_sim, const GeneticOptimizerConfig& config);
    bool step(std::vector<CandidateResult>& current_elites, float& progress, std::string& status);
    std::vector<CandidateResult> finish(std::function<void(float, const std::string&)> callback = nullptr);
    void stop();

    bool is_running() const;
    bool is_finished() const;
    float progress() const;
    const std::string& current_status() const;
    const std::vector<CandidateResult>& get_elites() const;
    const GeneticOptimizationSummary& get_summary() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class GeneticOptimizer {
public:
    static GeneticOptimizationSummary run(
        const WarlockSimulator& base_sim,
        const GeneticOptimizerConfig& config = GeneticOptimizerConfig(),
        std::function<void(float progress, const std::string& current_name)> callback = nullptr,
        std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback = nullptr,
        const std::atomic<bool>* should_stop = nullptr
    );
};

} // namespace warlock
