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
    int generations = 20;
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

    // User-specified constraints (up to 3 required talents, locked race, locked rotation)
    std::vector<int> required_talent_indices; // Node indices (0..51) that must be maxed
    int forced_race = -1;                     // -1 = unconstrained/evolve; 0..4 = locked to Race enum
    int forced_rotation = -1;                 // -1 = auto/unconstrained; 0..N = locked to RotationChoice
};

struct GeneticOptimizationSummary {
    std::vector<CandidateResult> top_candidates;
    std::vector<CandidateResult> diverse_peaks;
    std::vector<double> generation_best_dps;
    std::vector<double> generation_mean_dps;
    SurrogateModel trained_surrogate;
    int total_evaluations = 0;
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
