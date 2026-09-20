#include "optimizer.hpp"
#include "spec_presets.hpp"
#include "talent_graph.hpp"
#include <random>
#include <algorithm>

namespace priest {

std::vector<CandidateResult> Optimizer::optimize_talents(
    const PriestSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback,
    bool compare_all_races
) {
    const auto& presets = standard_spec_presets();

    std::vector<sim::Race> races;
    if (compare_all_races) {
        races = {
            sim::Race::HUMAN,
            sim::Race::DWARF,
            sim::Race::NIGHT_ELF,
            sim::Race::UNDEAD,
            sim::Race::TROLL
        };
    } else {
        races = { base_sim.race };
    }

    struct CandidateDef {
        std::string name;
        std::string spec_name;
        Talents talents;
        RotationChoice rotation;
        sim::Race race;
    };

    std::vector<CandidateDef> defs;
    defs.reserve(presets.size() * races.size());

    for (const auto& r : races) {
        for (const auto& p : presets) {
            std::string label = std::string(p.display_name);
            if (compare_all_races) {
                label += " [" + std::string(sim::race_to_string(r)) + "]";
            }
            defs.push_back({label, p.display_name, p.make_talents(), p.rotation, r});
        }
    }

    std::vector<CandidateResult> results;
    results.reserve(defs.size());

    const size_t total = defs.size();
    for (size_t i = 0; i < total; ++i) {
        const auto& d = defs[i];
        if (callback) {
            callback(static_cast<float>(i) / static_cast<float>(total), d.name);
        }

        PriestSimulator cand_sim = base_sim;
        cand_sim.race = d.race;
        cand_sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, d.race);
        cand_sim.talents = d.talents;
        cand_sim.policy.rotation = d.rotation;
        cand_sim.record_timeline = true;

        BatchSimResult batch = ParallelSimRunner::run_batch(cand_sim, iterations_per_candidate);

        CandidateResult res;
        res.name = d.name;
        res.category = "Standard Presets";
        res.race = d.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.sw_procs = batch.mean_sw_weaving_procs;
        res.mean_mana_spent = batch.mean_mana_spent;
        res.mean_mana_gained = batch.mean_mana_gained;
        res.talents = cand_sim.talents;
        res.gear = cand_sim.gear;
        res.buffs = cand_sim.buffs;
        res.policy = cand_sim.policy;
        res.mechanics = cand_sim.mechanics;
        res.use_raw_stats = cand_sim.use_raw_stats;
        res.raw_stats = cand_sim.raw_stats;
        res.batch = batch;

        results.push_back(res);
    }

    if (callback) {
        callback(1.0f, "Completed Benchmark");
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    return results;
}

std::vector<CandidateResult> Optimizer::optimize_genetic_ai(
    const PriestSimulator& base_sim,
    int population_size,
    int generations,
    int screening_sims,
    int final_sims,
    bool seed_with_presets,
    bool optimize_race,
    std::function<void(float progress, const std::string& current_name)> callback,
    std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback,
    const std::atomic<bool>* should_stop
) {
    const auto& graph = TalentGraph::get();
    std::mt19937_64 rng(42);

    const std::vector<sim::Race> allowed_races = {
        sim::Race::HUMAN,
        sim::Race::DWARF,
        sim::Race::NIGHT_ELF,
        sim::Race::UNDEAD,
        sim::Race::TROLL
    };

    struct Genome {
        std::vector<int> genes;
        sim::Race race = sim::Race::HUMAN;
        RotationChoice rotation = RotationChoice::SHADOW_PRIEST;
        double fitness = 0.0;
        BatchSimResult batch;
    };

    auto repair_genome = [&](std::vector<int>& v) {
        // Enforce max points and validity
        int total = graph.count_total_points(v);
        // If over 51, subtract from highest rows down
        while (total > 51) {
            for (int r = (int)v.size() - 1; r >= 0 && total > 51; --r) {
                if (v[r] > 0) {
                    v[r]--;
                    total--;
                }
            }
        }
        // If under 51, add to valid available nodes
        std::uniform_int_distribution<size_t> dist(0, v.size() - 1);
        int attempts = 0;
        while (total < 51 && attempts < 200) {
            attempts++;
            size_t idx = dist(rng);
            if (v[idx] < graph.nodes()[idx].max_rank) {
                v[idx]++;
                if (graph.is_valid(v, 51)) {
                    total++;
                } else {
                    v[idx]--;
                }
            }
        }
    };

    std::vector<Genome> pop;
    pop.reserve(population_size);

    // Seed presets
    if (seed_with_presets) {
        for (const auto& p : standard_spec_presets()) {
            Genome g;
            g.genes = graph.to_vector(p.make_talents());
            g.race = base_sim.race;
            g.rotation = p.rotation;
            pop.push_back(g);
        }
    }

    // Fill remainder with variations
    while (pop.size() < static_cast<size_t>(population_size)) {
        Genome g;
        // Start from standard shadow preset and perturb
        g.genes = graph.to_vector(Talents::create_forever_shadow());
        g.race = optimize_race ? allowed_races[rng() % allowed_races.size()] : base_sim.race;
        // Mutate a few points
        for (int m = 0; m < 5; ++m) {
            size_t idx = rng() % g.genes.size();
            if (g.genes[idx] > 0) g.genes[idx]--;
        }
        repair_genome(g.genes);
        // Choose rotation based on shadowform / shadow talents
        Talents t = graph.to_talents(g.genes);
        g.rotation = (t.shadow.shadowform > 0 || t.shadow.mind_flay > 0) ? RotationChoice::SHADOW_PRIEST : RotationChoice::SMITE_PRIEST;
        pop.push_back(g);
    }

    // Evolution loop
    for (int gen = 0; gen < generations; ++gen) {
        if (should_stop && should_stop->load()) break;

        float progress = static_cast<float>(gen) / static_cast<float>(generations);
        if (callback) {
            callback(progress, "Generation " + std::to_string(gen + 1) + " / " + std::to_string(generations));
        }

        // Evaluate fitness
        for (auto& ind : pop) {
            if (ind.fitness > 0.0) continue; // Already evaluated
            PriestSimulator sim = base_sim;
            sim.race = ind.race;
            sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, ind.race);
            sim.talents = graph.to_talents(ind.genes);
            sim.policy.rotation = ind.rotation;

            ind.batch = ParallelSimRunner::run_batch(sim, screening_sims);
            ind.fitness = ind.batch.mean_dps;
        }

        // Sort descending
        std::sort(pop.begin(), pop.end(), [](const Genome& a, const Genome& b) {
            return a.fitness > b.fitness;
        });

        // Periodic elite callback
        if (generation_callback && (gen % 5 == 0 || gen == generations - 1)) {
            std::vector<CandidateResult> elites;
            for (size_t i = 0; i < std::min((size_t)5, pop.size()); ++i) {
                CandidateResult c;
                c.rank = static_cast<int>(i + 1);
                c.name = "Gen " + std::to_string(gen + 1) + " Elite #" + std::to_string(i + 1);
                c.category = "Genetic AI";
                c.race = pop[i].race;
                c.mean_dps = pop[i].fitness;
                c.std_dev_dps = pop[i].batch.std_dev_dps;
                c.min_dps = pop[i].batch.min_dps;
                c.max_dps = pop[i].batch.max_dps;
                c.sw_procs = pop[i].batch.mean_sw_weaving_procs;
                c.mean_mana_spent = pop[i].batch.mean_mana_spent;
                c.mean_mana_gained = pop[i].batch.mean_mana_gained;
                c.talents = graph.to_talents(pop[i].genes);
                c.batch = pop[i].batch;
                elites.push_back(c);
            }
            generation_callback(elites);
        }

        if (gen == generations - 1) break;

        // Reproduce: Keep top 20% elites
        size_t elite_count = std::max((size_t)2, pop.size() / 5);
        std::vector<Genome> next_pop;
        next_pop.reserve(population_size);
        for (size_t i = 0; i < elite_count; ++i) {
            next_pop.push_back(pop[i]);
        }

        // Generate offspring
        while (next_pop.size() < static_cast<size_t>(population_size)) {
            // Tournament selection
            size_t p1 = rng() % elite_count;
            size_t p2 = rng() % pop.size();
            const auto& parent1 = pop[p1];
            const auto& parent2 = pop[p2];

            Genome child;
            child.genes = parent1.genes;
            // Crossover
            size_t xover_pt = rng() % child.genes.size();
            for (size_t k = xover_pt; k < child.genes.size(); ++k) {
                child.genes[k] = parent2.genes[k];
            }
            // Mutation
            if (rng() % 100 < 40) {
                size_t m_idx = rng() % child.genes.size();
                if (child.genes[m_idx] > 0 && rng() % 2 == 0) child.genes[m_idx]--;
                else child.genes[m_idx]++;
            }
            repair_genome(child.genes);

            child.race = (optimize_race && rng() % 2 == 0) ? parent2.race : parent1.race;
            Talents t = graph.to_talents(child.genes);
            child.rotation = (t.shadow.shadowform > 0 || t.shadow.mind_flay > 0) ? RotationChoice::SHADOW_PRIEST : RotationChoice::SMITE_PRIEST;
            child.fitness = 0.0;
            next_pop.push_back(child);
        }

        pop = std::move(next_pop);
    }

    // Final evaluation on top performers with high precision
    size_t final_eval_count = std::min((size_t)10, pop.size());
    std::vector<CandidateResult> final_results;
    final_results.reserve(final_eval_count);

    if (callback) {
        callback(0.95f, "Final Precision Evaluation...");
    }

    for (size_t i = 0; i < final_eval_count; ++i) {
        const auto& ind = pop[i];
        PriestSimulator sim = base_sim;
        sim.race = ind.race;
        sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, ind.race);
        sim.talents = graph.to_talents(ind.genes);
        sim.policy.rotation = ind.rotation;
        sim.record_timeline = true;

        BatchSimResult batch = ParallelSimRunner::run_batch(sim, final_sims);

        CandidateResult res;
        res.name = "AI Evolved Build #" + std::to_string(i + 1);
        res.category = "Genetic AI";
        res.race = ind.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.sw_procs = batch.mean_sw_weaving_procs;
        res.mean_mana_spent = batch.mean_mana_spent;
        res.mean_mana_gained = batch.mean_mana_gained;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.buffs = sim.buffs;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.use_raw_stats = sim.use_raw_stats;
        res.raw_stats = sim.raw_stats;
        res.batch = batch;

        final_results.push_back(res);
    }

    std::sort(final_results.begin(), final_results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < final_results.size(); ++i) {
        final_results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) {
        callback(1.0f, "Completed Genetic AI Optimization");
    }

    return final_results;
}

} // namespace priest
