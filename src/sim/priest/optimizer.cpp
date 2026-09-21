#include "optimizer.hpp"
#include "spec_presets.hpp"
#include "talent_graph.hpp"
#include <random>
#include <algorithm>

namespace priest {

namespace {

std::string format_priest_build_name(const Talents& t) {
    int disc = t.disc.total_points();
    int holy = t.holy.total_points();
    int shadow = t.shadow.total_points();

    std::string nums = std::to_string(disc) + "/" + std::to_string(holy) + "/" + std::to_string(shadow);

    std::string tag;
    if (t.shadow.shadowform > 0) {
        tag = (t.disc.inner_focus > 0) ? "Shadow" : "Deep Shadow";
    } else if (t.disc.power_infusion > 0) {
        tag = (t.holy.searing_light > 0) ? "PI Smite" : "PI Disc";
    } else if (t.disc.penance > 0) {
        tag = "Penance";
    } else if (t.holy.searing_light > 0 || t.holy.holy_specialization > 0) {
        tag = "Smite DPS";
    } else if (shadow >= 20) {
        tag = "Shadow";
    } else if (holy >= 20) {
        tag = "Holy";
    } else {
        tag = "Discipline";
    }

    return nums + " " + tag;
}

} // anonymous namespace

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

struct PriestGeneticOptimizerSession::Impl {
    PriestSimulator base_sim;
    int population_size = 50;
    int generations = 400;
    int screening_sims = 400;
    int final_sims = 2500;
    bool seed_with_presets = false;
    bool optimize_race = true;
    double mutation_rate = 0.45;
    std::vector<int> req_talents;
    int forced_race = -1;
    int forced_rotation = -1;
    int thread_count = 0;

    std::mt19937_64 rng{42};
    struct Genome {
        std::vector<int> genes;
        sim::Race race = sim::Race::HUMAN;
        RotationChoice rotation = RotationChoice::SHADOW_PRIEST;
        double fitness = 0.0;
        BatchSimResult batch;
    };
    std::vector<Genome> pop;

    int gen = 0;
    bool is_running = false;
    bool is_finished = false;
    bool stop_requested = false;
    float current_progress = 0.0f;
    std::string current_status;
    std::vector<CandidateResult> live_elites;

    std::vector<CandidateResult> extract_elites(size_t count = 5) {
        const auto& graph = TalentGraph::get();
        std::vector<CandidateResult> elites;
        for (size_t i = 0; i < std::min(count, pop.size()); ++i) {
            CandidateResult c;
            c.rank = static_cast<int>(i + 1);
            c.talents = graph.to_talents(pop[i].genes);
            c.name = format_priest_build_name(c.talents);
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
        return elites;
    }

    void start(
        const PriestSimulator& sim,
        int pop_sz,
        int gens,
        int screen_sims,
        int fn_sims,
        bool seed_pre,
        bool opt_race,
        double mut_rate,
        const std::vector<int>& req_t,
        int f_race,
        int f_rot,
        int th_count
    ) {
        const auto& graph = TalentGraph::get();
        base_sim = sim;
        population_size = pop_sz;
        generations = gens;
        screening_sims = screen_sims;
        final_sims = fn_sims;
        seed_with_presets = seed_pre;
        optimize_race = opt_race;
        mutation_rate = mut_rate;
        req_talents = req_t;
        forced_race = f_race;
        forced_rotation = f_rot;
        thread_count = th_count;
        rng = std::mt19937_64(42);

        const std::vector<sim::Race> allowed_races = {
            sim::Race::HUMAN,
            sim::Race::DWARF,
            sim::Race::NIGHT_ELF,
            sim::Race::UNDEAD,
            sim::Race::TROLL
        };

        pop.clear();
        pop.reserve(population_size);

        if (seed_with_presets) {
            for (const auto& p : standard_spec_presets()) {
                Genome g;
                g.genes = graph.to_vector(p.make_talents());
                g.race = (forced_race >= 0) ? static_cast<sim::Race>(forced_race) : base_sim.race;
                g.rotation = (forced_rotation >= 0) ? static_cast<RotationChoice>(forced_rotation) : p.rotation;
                graph.repair(g.genes, rng, req_talents);
                pop.push_back(g);
            }
        }

        while (pop.size() < static_cast<size_t>(population_size)) {
            Genome g;
            if (pop.empty() || (rng() % 3 == 0)) {
                g.genes = graph.generate_random_valid(rng, 51, req_talents);
            } else {
                const auto& presets = standard_spec_presets();
                const auto& p = presets[rng() % presets.size()];
                g.genes = graph.to_vector(p.make_talents());
                for (int m = 0; m < 5; ++m) {
                    size_t idx = rng() % g.genes.size();
                    if (g.genes[idx] > 0 && (rng() % 2 == 0)) g.genes[idx]--;
                    else g.genes[idx]++;
                }
                graph.repair(g.genes, rng, req_talents);
            }

            if (forced_race >= 0) {
                g.race = static_cast<sim::Race>(forced_race);
            } else {
                g.race = optimize_race ? allowed_races[rng() % allowed_races.size()] : base_sim.race;
            }

            if (forced_rotation >= 0) {
                g.rotation = static_cast<RotationChoice>(forced_rotation);
            } else {
                Talents t = graph.to_talents(g.genes);
                g.rotation = (t.shadow.shadowform > 0 || t.shadow.mind_flay > 0) ? RotationChoice::SHADOW_PRIEST : RotationChoice::SMITE_PRIEST;
            }
            pop.push_back(g);
        }

        // Evaluate Gen 0
        for (auto& ind : pop) {
            if (ind.fitness > 0.0) continue;
            PriestSimulator s = base_sim;
            s.race = ind.race;
            s.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, ind.race);
            s.talents = graph.to_talents(ind.genes);
            s.policy.rotation = ind.rotation;

            ind.batch = ParallelSimRunner::run_batch(s, screening_sims, thread_count);
            ind.fitness = ind.batch.mean_dps;
        }

        std::sort(pop.begin(), pop.end(), [](const Genome& a, const Genome& b) {
            return a.fitness > b.fitness;
        });

        live_elites = extract_elites(10);
        gen = 0;
        is_running = true;
        is_finished = false;
        stop_requested = false;
        current_progress = 0.0f;
        current_status = "Generation 0 Initialized";
    }

    bool step(std::vector<CandidateResult>& elites_out, float& progress_out, std::string& status_out) {
        if (!is_running) return false;
        if (stop_requested || gen >= generations) {
            finish();
            elites_out = live_elites;
            progress_out = 1.0f;
            status_out = "Completed";
            return false;
        }

        const auto& graph = TalentGraph::get();
        gen++;

        current_progress = static_cast<float>(gen) / static_cast<float>(generations);
        current_status = "Generation " + std::to_string(gen) + " / " + std::to_string(generations) + " [Best: " + std::to_string(static_cast<int>(pop[0].fitness)) + " DPS]";

        // Evaluate fitness
        for (auto& ind : pop) {
            if (ind.fitness > 0.0) continue;
            PriestSimulator s = base_sim;
            s.race = ind.race;
            s.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, ind.race);
            s.talents = graph.to_talents(ind.genes);
            s.policy.rotation = ind.rotation;

            ind.batch = ParallelSimRunner::run_batch(s, screening_sims, thread_count);
            ind.fitness = ind.batch.mean_dps;
        }

        std::sort(pop.begin(), pop.end(), [](const Genome& a, const Genome& b) {
            return a.fitness > b.fitness;
        });

        live_elites = extract_elites(10);
        elites_out = live_elites;
        progress_out = current_progress;
        status_out = current_status;

        if (gen >= generations) {
            finish();
            elites_out = live_elites;
            progress_out = 1.0f;
            status_out = "Completed";
            return false;
        }

        // Reproduce: Keep top 20% elites
        size_t elite_count = std::max((size_t)2, pop.size() / 5);
        std::vector<Genome> next_pop;
        next_pop.reserve(population_size);
        for (size_t i = 0; i < elite_count; ++i) {
            next_pop.push_back(pop[i]);
        }

        // Generate offspring
        while (next_pop.size() < static_cast<size_t>(population_size)) {
            size_t p1 = rng() % elite_count;
            size_t p2 = rng() % pop.size();
            const auto& parent1 = pop[p1];
            const auto& parent2 = pop[p2];

            Genome child;
            child.genes = parent1.genes;
            size_t xover_pt = rng() % child.genes.size();
            for (size_t k = xover_pt; k < child.genes.size(); ++k) {
                child.genes[k] = parent2.genes[k];
            }
            int mut_chance = static_cast<int>(mutation_rate * 100.0);
            if (rng() % 100 < mut_chance) {
                size_t m_idx = rng() % child.genes.size();
                if (child.genes[m_idx] > 0 && rng() % 2 == 0) child.genes[m_idx]--;
                else child.genes[m_idx]++;
            }
            graph.repair(child.genes, rng, req_talents);

            if (forced_race >= 0) {
                child.race = static_cast<sim::Race>(forced_race);
            } else {
                child.race = (optimize_race && rng() % 2 == 0) ? parent2.race : parent1.race;
            }

            if (forced_rotation >= 0) {
                child.rotation = static_cast<RotationChoice>(forced_rotation);
            } else {
                Talents t = graph.to_talents(child.genes);
                child.rotation = (t.shadow.shadowform > 0 || t.shadow.mind_flay > 0) ? RotationChoice::SHADOW_PRIEST : RotationChoice::SMITE_PRIEST;
            }
            child.fitness = 0.0;
            next_pop.push_back(child);
        }

        pop = std::move(next_pop);
        return true;
    }

    std::vector<CandidateResult> finish(std::function<void(float, const std::string&)> cb = nullptr) {
        if (is_finished) return live_elites;
        if (cb) cb(0.95f, "Final Precision Evaluation...");

        const auto& graph = TalentGraph::get();
        size_t final_eval_count = std::min((size_t)10, pop.size());
        std::vector<CandidateResult> final_results;
        final_results.reserve(final_eval_count);

        for (size_t i = 0; i < final_eval_count; ++i) {
            if (stop_requested) break;
            const auto& ind = pop[i];
            PriestSimulator s = base_sim;
            s.race = ind.race;
            s.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, ind.race);
            s.talents = graph.to_talents(ind.genes);
            s.policy.rotation = ind.rotation;
            s.record_timeline = true;

            BatchSimResult batch = ParallelSimRunner::run_batch(s, final_sims, thread_count);

            CandidateResult res;
            res.name = format_priest_build_name(s.talents);
            res.category = "Genetic AI";
            res.race = ind.race;
            res.mean_dps = batch.mean_dps;
            res.std_dev_dps = batch.std_dev_dps;
            res.min_dps = batch.min_dps;
            res.max_dps = batch.max_dps;
            res.sw_procs = batch.mean_sw_weaving_procs;
            res.mean_mana_spent = batch.mean_mana_spent;
            res.mean_mana_gained = batch.mean_mana_gained;
            res.talents = s.talents;
            res.gear = s.gear;
            res.buffs = s.buffs;
            res.policy = s.policy;
            res.mechanics = s.mechanics;
            res.use_raw_stats = s.use_raw_stats;
            res.raw_stats = s.raw_stats;
            res.batch = batch;

            final_results.push_back(res);
        }

        std::sort(final_results.begin(), final_results.end(), [](const CandidateResult& a, const CandidateResult& b) {
            return a.mean_dps > b.mean_dps;
        });

        for (size_t i = 0; i < final_results.size(); ++i) {
            final_results[i].rank = static_cast<int>(i + 1);
        }

        live_elites = final_results;
        is_running = false;
        is_finished = true;
        current_progress = 1.0f;
        current_status = "Completed";
        if (cb) cb(1.0f, "Completed Genetic AI Optimization");
        return live_elites;
    }
};

PriestGeneticOptimizerSession::PriestGeneticOptimizerSession() : impl_(std::make_unique<Impl>()) {}
PriestGeneticOptimizerSession::~PriestGeneticOptimizerSession() = default;
PriestGeneticOptimizerSession::PriestGeneticOptimizerSession(PriestGeneticOptimizerSession&&) noexcept = default;
PriestGeneticOptimizerSession& PriestGeneticOptimizerSession::operator=(PriestGeneticOptimizerSession&&) noexcept = default;

void PriestGeneticOptimizerSession::start(
    const PriestSimulator& base_sim,
    int population_size,
    int generations,
    int screening_sims,
    int final_sims,
    bool seed_with_presets,
    bool optimize_race,
    double mutation_rate,
    const std::vector<int>& req_talents,
    int forced_race,
    int forced_rotation,
    int thread_count
) {
    impl_->start(base_sim, population_size, generations, screening_sims, final_sims, seed_with_presets, optimize_race, mutation_rate, req_talents, forced_race, forced_rotation, thread_count);
}
bool PriestGeneticOptimizerSession::step(std::vector<CandidateResult>& current_elites, float& progress, std::string& status) {
    return impl_->step(current_elites, progress, status);
}
std::vector<CandidateResult> PriestGeneticOptimizerSession::finish(std::function<void(float, const std::string&)> callback) {
    return impl_->finish(callback);
}
void PriestGeneticOptimizerSession::stop() {
    impl_->stop_requested = true;
}
bool PriestGeneticOptimizerSession::is_running() const { return impl_->is_running; }
bool PriestGeneticOptimizerSession::is_finished() const { return impl_->is_finished; }
float PriestGeneticOptimizerSession::progress() const { return impl_->current_progress; }
const std::string& PriestGeneticOptimizerSession::current_status() const { return impl_->current_status; }
const std::vector<CandidateResult>& PriestGeneticOptimizerSession::get_elites() const { return impl_->live_elites; }

std::vector<CandidateResult> Optimizer::optimize_genetic_ai(
    const PriestSimulator& base_sim,
    int population_size,
    int generations,
    int screening_sims,
    int final_sims,
    bool seed_with_presets,
    bool optimize_race,
    double mutation_rate,
    double initial_explore,
    double min_explore,
    const std::vector<int>& req_talents,
    int forced_race,
    int forced_rotation,
    int thread_count,
    std::function<void(float progress, const std::string& current_name)> callback,
    std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback,
    const std::atomic<bool>* should_stop
) {
    (void)initial_explore;
    (void)min_explore;
    PriestGeneticOptimizerSession session;
    session.start(base_sim, population_size, generations, screening_sims, final_sims, seed_with_presets, optimize_race, mutation_rate, req_talents, forced_race, forced_rotation, thread_count);
    if (generation_callback) generation_callback(session.get_elites());

    while (session.is_running()) {
        if (should_stop && should_stop->load()) {
            session.stop();
            break;
        }
        std::vector<CandidateResult> elites;
        float prog;
        std::string status;
        bool more = session.step(elites, prog, status);
        if (callback) callback(prog, status);
        if (generation_callback && !elites.empty()) generation_callback(elites);
        if (!more) break;
    }

    return session.finish(callback);
}

} // namespace priest
