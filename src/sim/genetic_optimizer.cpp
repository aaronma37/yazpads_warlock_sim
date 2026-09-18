#include "genetic_optimizer.hpp"
#include "spec_presets.hpp"
#include "parallel_runner.hpp"
#include <random>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace warlock {

namespace {

struct Individual {
    std::array<int, TOTAL_TALENT_NODES> talents;
    PetChoice pet = PetChoice::NONE;
    bool sac_imp = false;
    bool sac_succubus = false;
    RotationChoice rotation = RotationChoice::SHADOW_DESTRO;
    bool maintain_immolate = true;
    CurseChoice curse = CurseChoice::BANE_OF_AGONY;
    ShadowburnPolicy shadowburn = ShadowburnPolicy::ON_COOLDOWN;
    bool use_decimation_soul_fire = false;
    bool channel_drain_hope = false;
    Race race = Race::UNDEAD;

    double fitness = 0.0;
    BatchSimResult batch;
    bool evaluated = false;
};

// MAP-Elites Quality-Diversity Grid Parameters (Zero Human Bias)
constexpr size_t MAP_U_BINS = 8;    // Affliction vs Destruction point bias [-51, +51]
constexpr size_t MAP_V_BINS = 8;    // Demonology depth [0, 51]
constexpr size_t MAP_PET_BINS = 4;  // 0: Sac-Imp, 1: Sac-Succ, 2: Active Pet, 3: None/Other

struct MapElitesCell {
    Individual elite;
    bool occupied = false;
    double max_fitness = -1e9;
};

inline size_t get_pet_bin(const Individual& ind) {
    if (ind.sac_imp) return 0;
    if (ind.sac_succubus) return 1;
    if (ind.pet != PetChoice::NONE) return 2;
    return 3;
}

inline void get_map_elites_coord(const Individual& ind, size_t& u_bin, size_t& v_bin, size_t& pet_bin) {
    const auto& graph = TalentGraph::get();
    int a = graph.count_tree_points(ind.talents, 0);
    int d = graph.count_tree_points(ind.talents, 1);
    int x = graph.count_tree_points(ind.talents, 2);

    // Coordinate 1: Aff vs Destro balance [-51, +51] -> [0.0, 1.0)
    double u = static_cast<double>(x - a + 51) / 102.0;
    u = std::clamp(u, 0.0, 0.9999);
    u_bin = static_cast<size_t>(u * MAP_U_BINS);

    // Coordinate 2: Demo depth [0, 51] -> [0.0, 1.0)
    double v = static_cast<double>(d) / 51.0;
    v = std::clamp(v, 0.0, 0.9999);
    v_bin = static_cast<size_t>(v * MAP_V_BINS);

    // Coordinate 3: Pet / Sacrifice mode
    pet_bin = get_pet_bin(ind);
}

// Auto-align pet and rotation policies to match the talent profile
void adapt_policies_to_talents(Individual& ind, FastRNG& rng) {
    const auto& graph = TalentGraph::get();
    int aff_pts = graph.count_tree_points(ind.talents, 0);
    int demo_pts = graph.count_tree_points(ind.talents, 1);
    int destro_pts = graph.count_tree_points(ind.talents, 2);

    bool has_ds = (ind.talents[17 + 9] > 0);        // Demonic Sacrifice
    bool has_dp = (ind.talents[17 + 18] > 0);       // Demonic Pact
    bool has_decimate = (ind.talents[17 + 11] > 0); // Decimation
    bool has_ruin = (ind.talents[36 + 6] > 0);      // Ruin
    bool has_incin = (ind.talents[36 + 15] > 0);    // Incinerate
    bool has_sm = (ind.talents[15] > 0);            // Shadow Mastery
    bool has_wrack = (ind.talents[16] > 0);         // Wrack / Drain Hope
    bool has_siphon = (ind.talents[13] > 0);        // Siphon Life
    bool has_conflag = (ind.talents[36 + 10] > 0);  // Conflagrate
    bool has_sburn = (ind.talents[36 + 7] > 0);     // Shadowburn

    ind.shadowburn = has_sburn ? ShadowburnPolicy::ON_COOLDOWN : ShadowburnPolicy::NEVER;
    ind.use_decimation_soul_fire = has_decimate;
    ind.channel_drain_hope = has_wrack;

    // Pet & Sacrifice setup
    if (has_dp) {
        // Demonic Pact: Sac Imp + Active Succubus
        ind.sac_imp = true;
        ind.sac_succubus = false;
        ind.pet = PetChoice::SUCCUBUS;
    } else if (has_ds) {
        if (destro_pts >= 30 && has_incin && (rng.next_u64() % 2 == 0)) {
            // Fire Destro with Sac-Succubus (+15% Fire)
            ind.sac_succubus = true;
            ind.sac_imp = false;
            ind.pet = PetChoice::NONE;
        } else {
            // Shadow with Sac-Imp (+15% Shadow)
            ind.sac_imp = true;
            ind.sac_succubus = false;
            ind.pet = PetChoice::NONE;
        }
    } else {
        ind.sac_imp = false;
        ind.sac_succubus = false;
        if (demo_pts >= 15 || ind.talents[17 + 17] > 0) {
            ind.pet = (rng.next_u64() % 2 == 0) ? PetChoice::SUCCUBUS : PetChoice::IMP;
        } else {
            ind.pet = PetChoice::IMP;
        }
    }

    // Rotation setup
    if (has_incin || (destro_pts >= 30 && ind.sac_succubus)) {
        ind.maintain_immolate = true;
        if (has_conflag && ind.talents[36 + 14] > 0) {
            ind.rotation = (rng.next_u64() % 2 == 0) ? RotationChoice::FIRE_DESTRO : RotationChoice::SHADOW_AND_FLAME_FIRE_2;
        } else {
            ind.rotation = RotationChoice::FIRE_DESTRO;
        }
    } else if (has_wrack || aff_pts >= 35) {
        ind.maintain_immolate = false;
        ind.rotation = has_siphon ? RotationChoice::DEEP_AFFLICTION_SB : RotationChoice::DEEP_AFFLICTION_SB_NO_SL;
    } else if (has_dp || (demo_pts >= 25 && !has_ds)) {
        ind.maintain_immolate = false;
        ind.rotation = RotationChoice::DP_AF_SHADOW;
    } else if (has_sm && has_ruin) {
        ind.maintain_immolate = false;
        ind.rotation = RotationChoice::SM_RUIN;
    } else {
        // Standard Destro / Hybrid
        ind.maintain_immolate = has_conflag;
        ind.rotation = RotationChoice::SHADOW_DESTRO;
    }
}

// Convert Individual to WarlockSimulator
WarlockSimulator individual_to_sim(const WarlockSimulator& base_sim, const Individual& ind) {
    WarlockSimulator sim = base_sim;
    sim.race = ind.race;
    sim.base_attrs = get_base_attributes_for_race(ind.race);
    sim.talents = TalentGraph::get().to_talents(ind.talents);

    bool has_ds = (sim.talents.demo.demonic_sacrifice > 0);
    bool has_dp = (sim.talents.demo.demonic_pact > 0);
    sim.buffs.sacrifice_imp = (has_ds || has_dp) && ind.sac_imp;
    sim.buffs.sacrifice_succubus = (has_ds || has_dp) && ind.sac_succubus;

    sim.policy.pet = ind.pet;
    sim.policy.rotation = ind.rotation;
    sim.policy.maintain_immolate = ind.maintain_immolate;
    sim.policy.curse = ind.curse;
    sim.policy.shadowburn = ind.shadowburn;
    sim.policy.use_decimation_soul_fire = ind.use_decimation_soul_fire;
    sim.policy.channel_drain_hope = ind.channel_drain_hope;
    return sim;
}

inline std::vector<int> get_effective_required_talents(const GeneticOptimizerConfig& config) {
    std::vector<int> reqs = config.required_talent_indices;
    if (config.forced_pet_mode == static_cast<int>(PetConstraint::SAC_IMP) ||
        config.forced_pet_mode == static_cast<int>(PetConstraint::SAC_SUCCUBUS)) {
        reqs.push_back(AFFLICTION_NODE_COUNT + 9); // Demonic Sacrifice (R3C2 Demo)
    } else if (config.forced_pet_mode == static_cast<int>(PetConstraint::DEMONIC_PACT_IMP_SUCC) ||
               config.forced_pet_mode == static_cast<int>(PetConstraint::DEMONIC_PACT_SUCC_IMP)) {
        reqs.push_back(AFFLICTION_NODE_COUNT + 18); // Demonic Pact (R7C2 Demo)
    }
    return reqs;
}

// Enforce talent constraints, locked race, locked rotation, and locked pet/DS mode
void enforce_constraints(Individual& ind, const GeneticOptimizerConfig& config, FastRNG& rng) {
    const auto& graph = TalentGraph::get();

    // 0. Enforce required talents (including implicit talent requirements from Pet / DS constraints)
    auto all_req_talents = get_effective_required_talents(config);
    if (!all_req_talents.empty()) {
        for (int req_idx : all_req_talents) {
            if (req_idx < 0 || req_idx >= static_cast<int>(TOTAL_TALENT_NODES)) continue;
            const auto& target_node = graph.node(req_idx);

            // If prerequisite exists, max it
            if (target_node.req_global_idx >= 0) {
                int p_idx = target_node.req_global_idx;
                ind.talents[p_idx] = graph.node(p_idx).max_points;
            }

            // Ensure sufficient points in preceding rows of the tree
            int tree = target_node.tree_idx;
            int required_below = 5 * (target_node.row - 1);

            int cur_below = 0;
            for (size_t k = 0; k < TOTAL_TALENT_NODES; ++k) {
                if (graph.node(k).tree_idx == tree && graph.node(k).row < target_node.row) {
                    cur_below += ind.talents[k];
                }
            }

            while (cur_below < required_below) {
                std::vector<size_t> row_cands;
                for (size_t k = 0; k < TOTAL_TALENT_NODES; ++k) {
                    if (graph.node(k).tree_idx == tree && graph.node(k).row < target_node.row) {
                        if (ind.talents[k] < graph.node(k).max_points) {
                            row_cands.push_back(k);
                        }
                    }
                }
                if (row_cands.empty()) break;
                size_t pick = row_cands[rng.next_u64() % row_cands.size()];
                ind.talents[pick]++;
                cur_below++;
            }

            // Max out the target talent
            ind.talents[req_idx] = target_node.max_points;
        }

        // Repair any other tree inconsistencies and balance to 51 points
        // While repairing, protect required talents from being cleared or donated
        int current = graph.count_total_points(ind.talents);
        while (current > 51) {
            auto donors = graph.get_valid_donors(ind.talents);
            std::vector<size_t> filtered_donors;
            for (size_t d : donors) {
                bool is_req = false;
                for (int req : all_req_talents) {
                    if (static_cast<int>(d) == req) { is_req = true; break; }
                }
                if (!is_req) filtered_donors.push_back(d);
            }
            if (filtered_donors.empty()) break;
            size_t pick = filtered_donors[rng.next_u64() % filtered_donors.size()];
            ind.talents[pick]--;
            current--;
        }

        while (current < 51) {
            auto receivers = graph.get_valid_receivers(ind.talents);
            if (receivers.empty()) break;
            size_t pick = receivers[rng.next_u64() % receivers.size()];
            ind.talents[pick]++;
            current++;
        }
    }

    // 1. Enforce Demonic Sacrifice / Pet configuration
    if (config.forced_pet_mode >= 0) {
        PetConstraint pc = static_cast<PetConstraint>(config.forced_pet_mode);
        switch (pc) {
            case PetConstraint::ACTIVE_IMP:
                ind.sac_imp = false;
                ind.sac_succubus = false;
                ind.pet = PetChoice::IMP;
                break;
            case PetConstraint::ACTIVE_SUCCUBUS:
                ind.sac_imp = false;
                ind.sac_succubus = false;
                ind.pet = PetChoice::SUCCUBUS;
                break;
            case PetConstraint::SAC_IMP:
                ind.sac_imp = true;
                ind.sac_succubus = false;
                ind.pet = PetChoice::NONE;
                break;
            case PetConstraint::SAC_SUCCUBUS:
                ind.sac_imp = false;
                ind.sac_succubus = true;
                ind.pet = PetChoice::NONE;
                break;
            case PetConstraint::DEMONIC_PACT_IMP_SUCC:
                ind.sac_imp = true;
                ind.sac_succubus = false;
                ind.pet = PetChoice::SUCCUBUS;
                break;
            case PetConstraint::DEMONIC_PACT_SUCC_IMP:
                ind.sac_imp = false;
                ind.sac_succubus = true;
                ind.pet = PetChoice::IMP;
                break;
            case PetConstraint::NO_PET:
                ind.sac_imp = false;
                ind.sac_succubus = false;
                ind.pet = PetChoice::NONE;
                break;
            default:
                break;
        }
    } else {
        bool has_ds = (ind.talents[AFFLICTION_NODE_COUNT + 9] > 0);
        bool has_dp = (ind.talents[AFFLICTION_NODE_COUNT + 18] > 0);

        if (!has_ds && !has_dp) {
            if (ind.sac_imp || ind.sac_succubus) {
                ind.sac_imp = false;
                ind.sac_succubus = false;
                if (ind.pet == PetChoice::NONE) {
                    int demo_pts = graph.count_tree_points(ind.talents, 1);
                    if (demo_pts >= 15 || ind.talents[AFFLICTION_NODE_COUNT + 17] > 0) {
                        ind.pet = (rng.next_u64() % 2 == 0) ? PetChoice::SUCCUBUS : PetChoice::IMP;
                    } else {
                        ind.pet = PetChoice::IMP;
                    }
                }
            }
        } else if (!has_dp && (ind.sac_imp || ind.sac_succubus)) {
            ind.pet = PetChoice::NONE;
        }
    }

    // 2. Lock race if requested
    if (config.forced_race >= 0) {
        ind.race = static_cast<Race>(config.forced_race);
    }

    // 3. Lock rotation if requested
    if (config.forced_rotation >= 0) {
        ind.rotation = static_cast<RotationChoice>(config.forced_rotation);
    }
}

// Guided 1-point swap mutation using surrogate weights
void guided_point_swap(std::array<int, TOTAL_TALENT_NODES>& talents, const SurrogateModel& surrogate, FastRNG& rng, double exploration_rate, const std::vector<int>& required_talents = {}) {
    const auto& graph = TalentGraph::get();
    auto raw_donors = graph.get_valid_donors(talents);
    std::vector<size_t> donors;
    for (size_t d : raw_donors) {
        bool is_req = false;
        for (int req : required_talents) {
            if (static_cast<int>(d) == req) { is_req = true; break; }
        }
        if (!is_req) donors.push_back(d);
    }

    auto receivers = graph.get_valid_receivers(talents);

    if (donors.empty() || receivers.empty()) return;

    size_t donor_idx = donors[0];
    size_t receiver_idx = receivers[0];

    bool explore = (rng.next_double() < exploration_rate) || !surrogate.is_trained();

    if (explore) {
        donor_idx = donors[rng.next_u64() % donors.size()];
        receiver_idx = receivers[rng.next_u64() % receivers.size()];
    } else {
        // Softmax-weighted donor selection (prefer dropping low-weight talents)
        std::vector<double> donor_probs(donors.size());
        double min_w = 1e9, max_w = -1e9;
        for (size_t i = 0; i < donors.size(); ++i) {
            double w = surrogate.get_talent_weight(donors[i]);
            min_w = std::min(min_w, w);
            max_w = std::max(max_w, w);
        }
        double sum_d = 0.0;
        for (size_t i = 0; i < donors.size(); ++i) {
            double w = surrogate.get_talent_weight(donors[i]);
            donor_probs[i] = std::exp(-(w - min_w) / 50.0);
            sum_d += donor_probs[i];
        }
        double r_d = rng.next_double() * sum_d;
        double accum_d = 0.0;
        for (size_t i = 0; i < donors.size(); ++i) {
            accum_d += donor_probs[i];
            if (accum_d >= r_d) {
                donor_idx = donors[i];
                break;
            }
        }

        // Softmax-weighted receiver selection (prefer adding high-weight talents)
        std::vector<double> rec_probs(receivers.size());
        double sum_r = 0.0;
        for (size_t i = 0; i < receivers.size(); ++i) {
            double w = surrogate.get_talent_weight(receivers[i]);
            rec_probs[i] = std::exp((w - min_w) / 50.0);
            sum_r += rec_probs[i];
        }
        double r_r = rng.next_double() * sum_r;
        double accum_r = 0.0;
        for (size_t i = 0; i < receivers.size(); ++i) {
            accum_r += rec_probs[i];
            if (accum_r >= r_r) {
                receiver_idx = receivers[i];
                break;
            }
        }
    }

    talents[donor_idx]--;
    talents[receiver_idx]++;
    graph.repair(talents, rng);
}

// Crossover two individuals
Individual crossover_individuals(const Individual& p1, const Individual& p2, FastRNG& rng) {
    const auto& graph = TalentGraph::get();
    Individual child;

    int method = rng.next_u64() % 3;
    if (method == 0) {
        // Tree-swap crossover: take Tree 0/1 from p1, Tree 2 from p2
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            if (graph.node(i).tree_idx < 2) {
                child.talents[i] = p1.talents[i];
            } else {
                child.talents[i] = p2.talents[i];
            }
        }
    } else if (method == 1) {
        // Uniform node crossover
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            child.talents[i] = (rng.next_u64() % 2 == 0) ? p1.talents[i] : p2.talents[i];
        }
    } else {
        // Blended points
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            child.talents[i] = (p1.talents[i] + p2.talents[i]) / 2;
        }
    }

    graph.repair(child.talents, rng);

    // Inherit policy & race from the fitter parent
    const Individual& dom = (p1.fitness >= p2.fitness) ? p1 : p2;
    child.race = dom.race;
    child.pet = dom.pet;
    child.sac_imp = dom.sac_imp;
    child.sac_succubus = dom.sac_succubus;
    child.rotation = dom.rotation;
    child.maintain_immolate = dom.maintain_immolate;
    child.curse = dom.curse;
    child.shadowburn = dom.shadowburn;
    child.use_decimation_soul_fire = dom.use_decimation_soul_fire;
    child.channel_drain_hope = dom.channel_drain_hope;

    adapt_policies_to_talents(child, rng);
    return child;
}

std::string format_build_name(const Individual& ind) {
    const auto& graph = TalentGraph::get();
    int a = graph.count_tree_points(ind.talents, 0);
    int d = graph.count_tree_points(ind.talents, 1);
    int x = graph.count_tree_points(ind.talents, 2);

    Talents t = graph.to_talents(ind.talents);

    // Identify key prominent talents across trees
    bool has_dh   = (t.aff.drain_hope > 0);
    bool has_sm   = (t.aff.shadow_mastery > 0);
    bool has_sl   = (t.aff.siphon_life > 0);
    bool has_nf   = (t.aff.nightfall > 0);

    bool has_dp   = (t.demo.demonic_pact > 0);
    bool has_md   = (t.demo.master_demonologist > 0);
    bool has_soul = (t.demo.soul_link > 0);
    bool has_deci = (t.demo.decimation > 0);
    bool has_ds   = (t.demo.demonic_sacrifice > 0);

    bool has_inc  = (t.destro.incinerate > 0);
    bool has_sf   = (t.destro.shadow_and_flame > 0);
    bool has_conflag = (t.destro.conflagrate > 0);
    bool has_ruin = (t.destro.ruin > 0);
    bool has_sburn= (t.destro.shadowburn > 0);

    std::vector<std::string> tags;

    // Affliction tags
    if (has_dh) tags.push_back("Drain Hope");
    else if (has_sm) tags.push_back("SM");
    else if (has_sl) tags.push_back("SL");
    else if (has_nf) tags.push_back("NF");
    else if (a >= 20) tags.push_back("Aff");

    // Demonology tags
    if (has_dp) tags.push_back("DP");
    else if (has_md) tags.push_back("MD");
    else if (has_soul) tags.push_back("Soul Link");
    else if (has_ds && has_deci) tags.push_back("DS/Deci");
    else if (has_ds) tags.push_back("DS");
    else if (has_deci) tags.push_back("Deci");
    else if (d >= 20) tags.push_back("Demo");

    // Destruction tags
    if (has_inc) tags.push_back("Incin");
    else if (has_sf) tags.push_back("S&F");
    else if (has_conflag) tags.push_back("Conflag");
    else if (has_ruin) tags.push_back("Ruin");
    else if (has_sburn) tags.push_back("Sburn");
    else if (x >= 20) tags.push_back("Destro");

    std::ostringstream ss;
    ss << a << "/" << d << "/" << x << " ";

    if (tags.empty()) {
        // Fallback to highest point tree
        if (x >= a && x >= d) ss << "Destro";
        else if (a >= d && a >= x) ss << "Aff";
        else ss << "Demo";
    } else {
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i > 0) ss << "/";
            ss << tags[i];
        }
    }

    return ss.str();
}

} // anonymous namespace

GeneticOptimizationSummary GeneticOptimizer::run(
    const WarlockSimulator& base_sim,
    const GeneticOptimizerConfig& config,
    std::function<void(float progress, const std::string& current_name)> callback,
    std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback,
    const std::atomic<bool>* should_stop
) {
    GeneticOptimizationSummary summary;
    FastRNG rng(0xDEADBEEF4242ULL);
    const auto& graph = TalentGraph::get();

    std::vector<Individual> population;
    population.reserve(config.population_size);

    // MAP-Elites Grid (256 niches across Aff/Destro balance, Demo depth, and Pet mode)
    std::array<std::array<std::array<MapElitesCell, MAP_PET_BINS>, MAP_V_BINS>, MAP_U_BINS> map_grid{};

    auto add_to_map_elites = [&](const Individual& ind) {
        size_t u, v, p;
        get_map_elites_coord(ind, u, v, p);
        auto& cell = map_grid[u][v][p];
        if (!cell.occupied || ind.fitness > cell.max_fitness) {
            cell.occupied = true;
            cell.max_fitness = ind.fitness;
            cell.elite = ind;
        }
    };

    auto get_occupied_elites = [&]() -> std::vector<Individual> {
        std::vector<Individual> elites;
        for (size_t u = 0; u < MAP_U_BINS; ++u) {
            for (size_t v = 0; v < MAP_V_BINS; ++v) {
                for (size_t p = 0; p < MAP_PET_BINS; ++p) {
                    if (map_grid[u][v][p].occupied) {
                        elites.push_back(map_grid[u][v][p].elite);
                    }
                }
            }
        }
        return elites;
    };

    // Helper to format live candidate result list from current individuals
    auto make_candidate_results = [&](const std::vector<Individual>& inds) -> std::vector<CandidateResult> {
        std::vector<CandidateResult> res_list;
        res_list.reserve(inds.size());
        for (size_t i = 0; i < inds.size(); ++i) {
            const auto& ind = inds[i];
            int a = graph.count_tree_points(ind.talents, 0);
            int d = graph.count_tree_points(ind.talents, 1);
            int x = graph.count_tree_points(ind.talents, 2);

            std::string cat = "Hybrid Peak";
            if (x >= 25 && x >= a && x >= d) cat = "Destruction Peak";
            else if (a >= 25 && a >= x && a >= d) cat = "Affliction Peak";
            else if (d >= 25 && d >= a && d >= x) cat = "Demonology Peak";

            CandidateResult res;
            res.rank = static_cast<int>(i + 1);
            res.name = format_build_name(ind);
            res.category = cat;
            res.race = ind.race;
            res.mean_dps = ind.fitness;
            res.std_dev_dps = ind.batch.std_dev_dps;
            res.min_dps = ind.batch.min_dps;
            res.max_dps = ind.batch.max_dps;
            res.isb_uptime = ind.batch.mean_isb_uptime;
            res.talents = graph.to_talents(ind.talents);
            res.gear = base_sim.gear;
            res.use_raw_stats = base_sim.use_raw_stats;
            res.raw_stats = base_sim.raw_stats;
            res.buffs = base_sim.buffs;
            res.buffs.sacrifice_imp = ind.sac_imp;
            res.buffs.sacrifice_succubus = ind.sac_succubus;
            res.policy = base_sim.policy;
            res.policy.pet = ind.pet;
            res.policy.rotation = ind.rotation;
            res.policy.maintain_immolate = ind.maintain_immolate;
            res.policy.curse = ind.curse;
            res.policy.shadowburn = ind.shadowburn;
            res.policy.use_decimation_soul_fire = ind.use_decimation_soul_fire;
            res.policy.channel_drain_hope = ind.channel_drain_hope;
            res.mechanics = base_sim.mechanics;
            res.batch = ind.batch;
            res_list.push_back(res);
        }
        return res_list;
    };

    // 1. Initialize population
    if (config.seed_with_presets) {
        for (const auto& p : standard_spec_presets()) {
            Individual ind;
            ind.talents = graph.to_vector(p.make_talents());
            ind.pet = p.pet;
            ind.sac_imp = p.sac_imp;
            ind.sac_succubus = p.sac_succubus;
            ind.rotation = p.rotation;
            ind.maintain_immolate = p.maintain_immolate;
            ind.race = config.optimize_race ? static_cast<Race>(rng.next_u64() % 5) : base_sim.race;
            enforce_constraints(ind, config, rng);
            population.push_back(ind);
            if (population.size() >= static_cast<size_t>(config.population_size / 2)) break;
        }
    }

    // Fill remainder with random valid builds
    while (population.size() < static_cast<size_t>(config.population_size)) {
        Individual ind;
        ind.talents = graph.generate_random_valid(rng);
        ind.race = config.optimize_race ? static_cast<Race>(rng.next_u64() % 5) : base_sim.race;
        adapt_policies_to_talents(ind, rng);
        enforce_constraints(ind, config, rng);
        population.push_back(ind);
    }

    // 2. Evaluate Generation 0
    int total_steps = config.generations + 1;
    for (size_t i = 0; i < population.size(); ++i) {
        if (should_stop && should_stop->load()) break;
        if (!population[i].evaluated) {
            WarlockSimulator sim = individual_to_sim(base_sim, population[i]);
            population[i].batch = ParallelSimRunner::run_batch(sim, config.screening_sims, config.num_threads);
            population[i].fitness = population[i].batch.mean_dps;
            population[i].evaluated = true;
            summary.total_evaluations++;

            summary.trained_surrogate.add_sample({
                population[i].talents,
                population[i].pet,
                population[i].sac_imp,
                population[i].sac_succubus,
                population[i].rotation,
                population[i].maintain_immolate,
                population[i].curse,
                population[i].race,
                population[i].fitness
            });

            add_to_map_elites(population[i]);
        }
    }

    std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
        return a.fitness > b.fitness;
    });

    summary.trained_surrogate.train();

    if (generation_callback) {
        generation_callback(make_candidate_results(population));
    }

    // Main Evolutionary Loop (MAP-Elites Quality-Diversity with Exploration Cooling)
    for (int gen = 1; gen <= config.generations; ++gen) {
        if (should_stop && should_stop->load()) break;

        // Annealed exploration schedule: starts high to prevent premature local convergence, then narrows
        double t_prog = static_cast<double>(gen - 1) / std::max(1, config.generations - 1);
        double cur_exploration_rate = config.initial_exploration_rate * (1.0 - t_prog) + config.min_exploration_rate * t_prog;

        float progress = static_cast<float>(gen) / static_cast<float>(total_steps);
        if (callback) {
            std::ostringstream ss;
            ss << "Gen " << gen << "/" << config.generations << " [Best: " << std::fixed << std::setprecision(1) << population[0].fitness << " DPS]";
            callback(progress, ss.str());
        }

        auto occupied_elites = get_occupied_elites();
        if (occupied_elites.empty()) occupied_elites = population;

        // Generate candidate offspring pool
        std::vector<Individual> candidate_pool;
        candidate_pool.reserve(config.offspring_pool_size);

        for (int i = 0; i < config.offspring_pool_size; ++i) {
            // Quality-Diversity Parent Selection: Sample from occupied niches across the entire map
            size_t p1_idx = rng.next_u64() % occupied_elites.size();
            size_t p2_idx = rng.next_u64() % occupied_elites.size();

            Individual offspring;
            if (rng.next_double() < config.crossover_rate) {
                offspring = crossover_individuals(occupied_elites[p1_idx], occupied_elites[p2_idx], rng);
            } else {
                offspring = occupied_elites[p1_idx];
            }

            // Guided point swap mutations with annealing exploration
            if (rng.next_double() < config.mutation_rate) {
                int swaps = 1 + (rng.next_u64() % 3);
                auto effective_reqs = get_effective_required_talents(config);
                for (int s = 0; s < swaps; ++s) {
                    guided_point_swap(offspring.talents, summary.trained_surrogate, rng, cur_exploration_rate, effective_reqs);
                }
            }

            // Race mutation (if race optimization enabled and not locked)
            if (config.forced_race < 0 && config.optimize_race && rng.next_double() < 0.20) {
                offspring.race = static_cast<Race>(rng.next_u64() % 5);
            }

            // Macro-jump / Rotation mutation
            if (rng.next_double() < 0.20) {
                adapt_policies_to_talents(offspring, rng);
            }

            enforce_constraints(offspring, config, rng);
            candidate_pool.push_back(offspring);
        }

        // Diversity injection: 6 random immigrants
        for (int imm = 0; imm < 6; ++imm) {
            Individual immigrant;
            immigrant.talents = graph.generate_random_valid(rng);
            immigrant.race = (config.forced_race >= 0) ? static_cast<Race>(config.forced_race) : (config.optimize_race ? static_cast<Race>(rng.next_u64() % 5) : base_sim.race);
            adapt_policies_to_talents(immigrant, rng);
            enforce_constraints(immigrant, config, rng);
            candidate_pool.push_back(immigrant);
        }

        // Surrogate Pre-Screening: Score candidate offspring using regression model
        struct ScoredCandidate {
            size_t idx;
            double predicted_dps;
        };
        std::vector<ScoredCandidate> scored;
        scored.reserve(candidate_pool.size());

        for (size_t i = 0; i < candidate_pool.size(); ++i) {
            double pred = summary.trained_surrogate.predict(
                candidate_pool[i].talents,
                candidate_pool[i].pet,
                candidate_pool[i].sac_imp,
                candidate_pool[i].sac_succubus,
                candidate_pool[i].rotation,
                candidate_pool[i].maintain_immolate,
                candidate_pool[i].curse,
                candidate_pool[i].race
            );
            scored.push_back({i, pred});
        }

        std::sort(scored.begin(), scored.end(), [](const ScoredCandidate& a, const ScoredCandidate& b) {
            return a.predicted_dps > b.predicted_dps;
        });

        // Niche-fair offspring selection: Allocate simulation budget evenly across top surrogate predictions + exploratory candidates
        int num_to_sim = std::min(config.simulated_offspring_per_gen, static_cast<int>(candidate_pool.size()));
        int num_top = static_cast<int>(num_to_sim * 0.70);
        int num_explore = num_to_sim - num_top;

        std::vector<size_t> chosen_indices;
        for (int i = 0; i < num_top && i < static_cast<int>(scored.size()); ++i) {
            chosen_indices.push_back(scored[i].idx);
        }
        for (int i = 0; i < num_explore; ++i) {
            size_t r_idx = rng.next_u64() % candidate_pool.size();
            chosen_indices.push_back(r_idx);
        }

        std::vector<Individual> evaluated_offspring;
        evaluated_offspring.reserve(chosen_indices.size());

        for (size_t cand_idx : chosen_indices) {
            if (should_stop && should_stop->load()) break;
            Individual ind = candidate_pool[cand_idx];

            WarlockSimulator sim = individual_to_sim(base_sim, ind);
            ind.batch = ParallelSimRunner::run_batch(sim, config.screening_sims, config.num_threads);
            ind.fitness = ind.batch.mean_dps;
            ind.evaluated = true;
            summary.total_evaluations++;

            summary.trained_surrogate.add_sample({
                ind.talents,
                ind.pet,
                ind.sac_imp,
                ind.sac_succubus,
                ind.rotation,
                ind.maintain_immolate,
                ind.curse,
                ind.race,
                ind.fitness
            });

            add_to_map_elites(ind);
            evaluated_offspring.push_back(ind);
        }

        // Elitism: Merge top parents + evaluated offspring, keep top population_size
        std::vector<Individual> next_pop;
        next_pop.insert(next_pop.end(), population.begin(), population.begin() + std::min((size_t)5, population.size()));
        next_pop.insert(next_pop.end(), evaluated_offspring.begin(), evaluated_offspring.end());

        std::sort(next_pop.begin(), next_pop.end(), [](const Individual& a, const Individual& b) {
            return a.fitness > b.fitness;
        });

        if (next_pop.size() > static_cast<size_t>(config.population_size)) {
            next_pop.resize(config.population_size);
        }
        population = std::move(next_pop);

        // Update tracking
        double mean_dps = 0.0;
        for (const auto& ind : population) mean_dps += ind.fitness;
        mean_dps /= population.size();

        summary.generation_best_dps.push_back(population[0].fitness);
        summary.generation_mean_dps.push_back(mean_dps);

        // Retrain the surrogate with updated dataset
        summary.trained_surrogate.train();

        // Publish live generation elites to UI
        if (generation_callback) {
            auto live_elites = get_occupied_elites();
            std::sort(live_elites.begin(), live_elites.end(), [](const Individual& a, const Individual& b) {
                return a.fitness > b.fitness;
            });
            if (live_elites.size() > 20) live_elites.resize(20);
            generation_callback(make_candidate_results(live_elites));
        }
    }

    // Final Stage: High-precision evaluation of Stratified Quality-Diversity Spectrum
    if (callback) callback(0.95f, "Finalizing & Simulating Diverse Spec Champions (High Precision)...");

    // Extract all MAP-Elites champions
    auto all_map_elites = get_occupied_elites();

    // Group occupied cells into distinct macro-archetype niches based on continuous talent distributions
    enum class MacroArchetype {
        DESTRUCTION_HEAVY,
        AFFLICTION_HEAVY,
        DEMONOLOGY_HEAVY,
        HYBRID_SPECS
    };

    struct TypedElite {
        Individual ind;
        MacroArchetype archetype;
        std::string archetype_name;
    };

    std::vector<TypedElite> categorized;
    categorized.reserve(all_map_elites.size());

    for (const auto& ind : all_map_elites) {
        int a = graph.count_tree_points(ind.talents, 0);
        int d = graph.count_tree_points(ind.talents, 1);
        int x = graph.count_tree_points(ind.talents, 2);

        TypedElite te;
        te.ind = ind;

        if (x >= 25 && x >= a && x >= d) {
            te.archetype = MacroArchetype::DESTRUCTION_HEAVY;
            te.archetype_name = "Destruction Peak";
        } else if (a >= 25 && a >= x && a >= d) {
            te.archetype = MacroArchetype::AFFLICTION_HEAVY;
            te.archetype_name = "Affliction Peak";
        } else if (d >= 25 && d >= a && d >= x) {
            te.archetype = MacroArchetype::DEMONOLOGY_HEAVY;
            te.archetype_name = "Demonology Peak";
        } else {
            te.archetype = MacroArchetype::HYBRID_SPECS;
            te.archetype_name = "Hybrid Peak";
        }
        categorized.push_back(te);
    }

    // Sort within each archetype by screening fitness
    std::sort(categorized.begin(), categorized.end(), [](const TypedElite& a, const TypedElite& b) {
        return a.ind.fitness > b.ind.fitness;
    });

    // Helper lambda to check talent/policy equality
    auto is_same_build = [](const Individual& a, const Individual& b) {
        return a.talents == b.talents && a.rotation == b.rotation && a.pet == b.pet &&
               a.sac_imp == b.sac_imp && a.sac_succubus == b.sac_succubus && a.race == b.race;
    };

    // Quotas: Pick top 5 Destro, top 5 Affliction, top 5 Demo, top 5 Hybrid
    std::vector<TypedElite> diverse_pool;
    std::unordered_map<MacroArchetype, int> archetype_counts;
    const int max_per_archetype = 5;

    // Pass 1: Select up to max_per_archetype from each category
    for (const auto& te : categorized) {
        if (archetype_counts[te.archetype] >= max_per_archetype) continue;
        bool dup = false;
        for (const auto& d : diverse_pool) {
            if (is_same_build(te.ind, d.ind)) { dup = true; break; }
        }
        if (!dup) {
            diverse_pool.push_back(te);
            archetype_counts[te.archetype]++;
        }
    }

    // Pass 2: If we have fewer than 20 builds total, fill remaining slots with highest overall fitness
    if (diverse_pool.size() < 20) {
        for (const auto& te : categorized) {
            bool dup = false;
            for (const auto& d : diverse_pool) {
                if (is_same_build(te.ind, d.ind)) { dup = true; break; }
            }
            if (!dup) {
                diverse_pool.push_back(te);
                if (diverse_pool.size() >= 20) break;
            }
        }
    }

    // High precision simulation for all chosen diverse champions
    for (size_t i = 0; i < diverse_pool.size(); ++i) {
        if (should_stop && should_stop->load()) break;
        WarlockSimulator sim = individual_to_sim(base_sim, diverse_pool[i].ind);
        BatchSimResult batch = ParallelSimRunner::run_batch(sim, config.final_sims, config.num_threads);

        CandidateResult res;
        res.name = format_build_name(diverse_pool[i].ind);
        res.category = diverse_pool[i].archetype_name;
        res.race = sim.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.isb_uptime = batch.mean_isb_uptime;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.use_raw_stats = sim.use_raw_stats;
        res.raw_stats = sim.raw_stats;
        res.buffs = sim.buffs;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.batch = batch;

        summary.diverse_peaks.push_back(res);
    }

    // Sort final results by high-precision simulated mean DPS
    std::sort(summary.diverse_peaks.begin(), summary.diverse_peaks.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < summary.diverse_peaks.size(); ++i) {
        summary.diverse_peaks[i].rank = static_cast<int>(i + 1);
    }

    summary.top_candidates = summary.diverse_peaks;

    if (generation_callback && !summary.diverse_peaks.empty()) {
        generation_callback(summary.diverse_peaks);
    }

    if (callback) callback(1.0f, "Completed");
    return summary;
}

} // namespace warlock
