#include "optimizer.hpp"
#include "spec_presets.hpp"
#include "genetic_optimizer.hpp"
#include <algorithm>

namespace warlock {

StatWeights Optimizer::calculate_candidate_stat_weights(
    const WarlockSimulator& candidate_sim,
    int iterations_per_sample
) {
    StatWeights weights;
    weights.valid = true;

    // Get baseline stats
    Stats base_stats = candidate_sim.use_raw_stats ? candidate_sim.raw_stats : candidate_sim.gear.calculate_stats();

    // Use Common Random Numbers (CRN) with identical seed across all stat perturb runs
    // This isolates pure deterministic gradient without stochastic variance / Monte-Carlo noise
    const uint64_t CRN_SEED = 0x9e3779b97f4a7c15ULL;
    int samples = std::max(1500, iterations_per_sample);

    // 0. Base run
    WarlockSimulator sim_base = candidate_sim;
    sim_base.use_raw_stats = true;
    sim_base.raw_stats = base_stats;
    double base_dps = ParallelSimRunner::run_batch(sim_base, samples, 0, nullptr, CRN_SEED).mean_dps;

    // 1. +40 Spell Power sample
    {
        WarlockSimulator sim_sp = sim_base;
        sim_sp.raw_stats.spell_power += 40.0;
        double sp_dps = ParallelSimRunner::run_batch(sim_sp, samples, 0, nullptr, CRN_SEED).mean_dps;
        weights.dps_per_sp = std::max(0.0, (sp_dps - base_dps) / 40.0);
    }

    // 2. +3% Spell Hit sample
    {
        WarlockSimulator sim_hit = sim_base;
        sim_hit.raw_stats.spell_hit_percent += 3.0;
        double hit_dps = ParallelSimRunner::run_batch(sim_hit, samples, 0, nullptr, CRN_SEED).mean_dps;
        weights.dps_per_hit = std::max(0.0, (hit_dps - base_dps) / 3.0);
    }

    // 3. +3% Spell Crit sample
    {
        WarlockSimulator sim_crit = sim_base;
        sim_crit.raw_stats.spell_crit_percent += 3.0;
        double crit_dps = ParallelSimRunner::run_batch(sim_crit, samples, 0, nullptr, CRN_SEED).mean_dps;
        weights.dps_per_crit = std::max(0.0, (crit_dps - base_dps) / 3.0);
    }

    // 4. +3% Spell Haste sample
    {
        WarlockSimulator sim_haste = sim_base;
        sim_haste.raw_stats.spell_haste_percent += 3.0;
        double haste_dps = ParallelSimRunner::run_batch(sim_haste, samples, 0, nullptr, CRN_SEED).mean_dps;
        weights.dps_per_haste = std::max(0.0, (haste_dps - base_dps) / 3.0);
    }

    // 5. +30 Intellect sample
    {
        WarlockSimulator sim_int = sim_base;
        sim_int.raw_stats.intellect += 30.0;
        double int_dps = ParallelSimRunner::run_batch(sim_int, samples, 0, nullptr, CRN_SEED).mean_dps;
        weights.dps_per_int = std::max(0.0, (int_dps - base_dps) / 30.0);
    }

    return weights;
}

std::vector<CandidateResult> Optimizer::optimize_talents(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback,
    bool compare_all_races,
    bool calculate_stat_weights
) {
    struct Candidate {
        std::string name;
        Talents talents;
        RotationChoice rotation;
        PetChoice pet;
        bool sac_succubus;
        bool sac_imp;
        bool maintain_immolate;
    };

    // Spec names, talents, and policies come from the central registry in
    // spec_presets.hpp so a rename there propagates to every consumer.
    std::vector<Candidate> candidates;
    candidates.reserve(standard_spec_presets().size());
    for (const auto& preset : standard_spec_presets()) {
        candidates.push_back({preset.display_name, preset.make_talents(),
                              preset.rotation, preset.pet,
                              preset.sac_succubus, preset.sac_imp,
                              preset.maintain_immolate});
    }

    std::vector<Race> races_to_test = compare_all_races
        ? std::vector<Race>{Race::UNDEAD, Race::ORC, Race::TROLL, Race::HUMAN, Race::GNOME}
        : std::vector<Race>{base_sim.race};

    std::vector<CandidateResult> results;
    int total = static_cast<int>(candidates.size() * races_to_test.size());
    int current_idx = 0;

    for (const auto& cand : candidates) {
        for (Race r : races_to_test) {
            std::string run_name = cand.name;
            if (compare_all_races) {
                run_name += " (" + std::string(race_to_string(r)) + ")";
            }
            if (callback) callback(static_cast<float>(current_idx) / total, run_name);
            current_idx++;

            WarlockSimulator sim = base_sim;
            sim.race = r;
            sim.base_attrs = get_base_attributes_for_race(r);
            sim.talents = cand.talents;
            sim.buffs.sacrifice_succubus = cand.sac_succubus;
            sim.buffs.sacrifice_imp = cand.sac_imp;
            sim.policy.pet = cand.pet;
            sim.policy.rotation = cand.rotation;
            sim.policy.maintain_immolate = cand.maintain_immolate;

            BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

            CandidateResult res;
            res.name = cand.name;
            if (compare_all_races) {
                res.name += std::string(" (") + race_to_string(r) + ")";
            }
            res.race = r;
            res.category = "Talents";
            res.mean_dps = batch.mean_dps;
            res.std_dev_dps = batch.std_dev_dps;
            res.min_dps = batch.min_dps;
            res.max_dps = batch.max_dps;
            res.isb_uptime = batch.mean_isb_uptime;
            res.talents = sim.talents;
            res.gear = sim.gear;
            res.buffs = sim.buffs;
            res.policy = sim.policy;
            res.mechanics = sim.mechanics;
            res.batch = batch;

            if (calculate_stat_weights) {
                if (callback) callback(static_cast<float>(current_idx - 1) / total, run_name + " (Stat Weights)");
                res.stat_weights = calculate_candidate_stat_weights(sim, std::max(1000, iterations_per_candidate / 2));
            }

            results.push_back(res);
        }
    }

    // Sort descending by Mean DPS
    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::optimize_gear(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback
) {
    struct GearCandidate {
        std::string name;
        GearLoadout gear;
    };

    std::vector<GearCandidate> candidates = {
        {"Pre-Raid BiS", GearLoadout::create_preraid_bis()},
        {"Phase 3/4 BiS (BWL / Bloodvine)", GearLoadout::create_phase3_bis()},
        {"Phase 5 BiS (AQ40)", GearLoadout::create_phase5_bis()},
        {"Phase 6 BiS (Naxxramas)", GearLoadout::create_phase6_bis()}
    };

    // Add Trinket variations on Phase 3 BiS
    {
        GearLoadout g = GearLoadout::create_phase3_bis();
        if (auto t = ItemDatabase::find_by_id(1203)) g.equip(Slot::TRINKET2, *t); // Briarwood Reed
        candidates.push_back({"Phase 3 BiS (Nelth Tear + Briarwood)", g});
    }
    {
        GearLoadout g = GearLoadout::create_phase6_bis();
        if (auto t = ItemDatabase::find_by_id(1201)) g.equip(Slot::TRINKET2, *t); // TOEP
        candidates.push_back({"Phase 6 BiS (Mark of Champion + TOEP)", g});
    }

    std::vector<CandidateResult> results;
    int total = static_cast<int>(candidates.size());

    for (int i = 0; i < total; ++i) {
        if (callback) callback(static_cast<float>(i) / total, candidates[i].name);

        WarlockSimulator sim = base_sim;
        sim.gear = candidates[i].gear;

        BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

        CandidateResult res;
        res.name = candidates[i].name;
        res.category = "Gear";
        res.race = sim.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.isb_uptime = batch.mean_isb_uptime;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.buffs = sim.buffs;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.batch = batch;

        results.push_back(res);
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::optimize_policy(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback
) {
    struct PolicyCandidate {
        std::string name;
        PolicyConfig policy;
    };

    std::vector<PolicyCandidate> candidates;

    // Rotation Presets comparison
    for (uint8_t r = 0; r <= 7; ++r) {
        PolicyConfig p = base_sim.policy;
        p.rotation = static_cast<RotationChoice>(r);
        candidates.push_back({"Rotation: " + std::string(rotation_choice_to_string(p.rotation)), p});
    }

    // Grid search over Life Tap threshold: 10%, 20%, 30%, 40%, 50%
    for (double tap_pct : {10.0, 20.0, 30.0, 40.0, 50.0}) {
        PolicyConfig p = base_sim.policy;
        p.life_tap_threshold_pct = tap_pct;
        candidates.push_back({"Life Tap @ < " + std::to_string(static_cast<int>(tap_pct)) + "% Mana", p});
    }

    // Curse / Bane comparisons
    {
        PolicyConfig p = base_sim.policy;
        p.curse = CurseChoice::BANE_OF_AGONY;
        candidates.push_back({"Bane: Bane of Agony (Solo/Affliction DPS)", p});
    }
    {
        PolicyConfig p = base_sim.policy;
        p.curse = CurseChoice::CURSE_OF_DOOM;
        candidates.push_back({"Curse: Curse of Doom (60s Burst)", p});
    }
    {
        PolicyConfig p = base_sim.policy;
        p.curse = CurseChoice::NONE;
        candidates.push_back({"Bane/Curse: None", p});
    }

    // Corruption comparisons
    {
        PolicyConfig p = base_sim.policy;
        p.corruption = DotPolicy::ALWAYS;
        candidates.push_back({"Corruption: Always Maintain", p});
    }
    {
        PolicyConfig p = base_sim.policy;
        p.corruption = DotPolicy::NEVER;
        candidates.push_back({"Corruption: Never (Shadow Bolt Spam Only)", p});
    }

    // Shadowburn on CD
    {
        PolicyConfig p = base_sim.policy;
        p.shadowburn = ShadowburnPolicy::ON_COOLDOWN;
        candidates.push_back({"Shadowburn: On Cooldown (8s)", p});
    }

    std::vector<CandidateResult> results;
    int total = static_cast<int>(candidates.size());

    for (int i = 0; i < total; ++i) {
        if (callback) callback(static_cast<float>(i) / total, candidates[i].name);

        WarlockSimulator sim = base_sim;
        sim.policy = candidates[i].policy;

        BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

        CandidateResult res;
        res.name = candidates[i].name;
        res.category = "Policy";
        res.race = sim.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.isb_uptime = batch.mean_isb_uptime;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.batch = batch;

        results.push_back(res);
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::evaluate_snapshotting_impact(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback
) {
    std::vector<CandidateResult> results;

    // Test with Snapshotting ON vs OFF for SM/AF (DoT heavy)
    {
        if (callback) callback(0.0f, "SM/AF - Snapshotting ON (Classic)");
        WarlockSimulator sim_on = base_sim;
        sim_on.talents = Talents::create_sm_af();
        sim_on.mechanics.snapshot_dots = true;
        BatchSimResult b_on = ParallelSimRunner::run_batch(sim_on, iterations_per_candidate);

        CandidateResult res_on;
        res_on.name = "SM/AF [Snapshotting ON (Classic)]";
        res_on.category = "Snapshotting";
        res_on.mean_dps = b_on.mean_dps;
        res_on.std_dev_dps = b_on.std_dev_dps;
        res_on.min_dps = b_on.min_dps;
        res_on.max_dps = b_on.max_dps;
        res_on.isb_uptime = b_on.mean_isb_uptime;
        res_on.talents = sim_on.talents;
        res_on.gear = sim_on.gear;
        res_on.policy = sim_on.policy;
        res_on.mechanics = sim_on.mechanics;
        results.push_back(res_on);

        if (callback) callback(0.25f, "SM/AF - Snapshotting OFF (Dynamic/Modern)");
        WarlockSimulator sim_off = base_sim;
        sim_off.talents = Talents::create_sm_af();
        sim_off.mechanics.snapshot_dots = false;
        BatchSimResult b_off = ParallelSimRunner::run_batch(sim_off, iterations_per_candidate);

        CandidateResult res_off;
        res_off.name = "SM/AF [Snapshotting OFF (Dynamic/Modern)]";
        res_off.category = "Snapshotting";
        res_off.mean_dps = b_off.mean_dps;
        res_off.std_dev_dps = b_off.std_dev_dps;
        res_off.min_dps = b_off.min_dps;
        res_off.max_dps = b_off.max_dps;
        res_off.isb_uptime = b_off.mean_isb_uptime;
        res_off.talents = sim_off.talents;
        res_off.gear = sim_off.gear;
        res_off.policy = sim_off.policy;
        res_off.mechanics = sim_off.mechanics;
        results.push_back(res_off);
    }

    // Test with Snapshotting ON vs OFF for DS/Ruin
    {
        if (callback) callback(0.50f, "DS/Ruin - Snapshotting ON (Classic)");
        WarlockSimulator sim_on = base_sim;
        sim_on.talents = Talents::create_ds_ruin();
        sim_on.buffs.sacrifice_succubus = true;
        sim_on.mechanics.snapshot_dots = true;
        BatchSimResult b_on = ParallelSimRunner::run_batch(sim_on, iterations_per_candidate);

        CandidateResult res_on;
        res_on.name = "DS/Ruin [Snapshotting ON (Classic)]";
        res_on.category = "Snapshotting";
        res_on.mean_dps = b_on.mean_dps;
        res_on.std_dev_dps = b_on.std_dev_dps;
        res_on.min_dps = b_on.min_dps;
        res_on.max_dps = b_on.max_dps;
        res_on.isb_uptime = b_on.mean_isb_uptime;
        res_on.talents = sim_on.talents;
        res_on.gear = sim_on.gear;
        res_on.policy = sim_on.policy;
        res_on.mechanics = sim_on.mechanics;
        results.push_back(res_on);

        if (callback) callback(0.75f, "DS/Ruin - Snapshotting OFF (Dynamic/Modern)");
        WarlockSimulator sim_off = base_sim;
        sim_off.talents = Talents::create_ds_ruin();
        sim_off.buffs.sacrifice_succubus = true;
        sim_off.mechanics.snapshot_dots = false;
        BatchSimResult b_off = ParallelSimRunner::run_batch(sim_off, iterations_per_candidate);

        CandidateResult res_off;
        res_off.name = "DS/Ruin [Snapshotting OFF (Dynamic/Modern)]";
        res_off.category = "Snapshotting";
        res_off.mean_dps = b_off.mean_dps;
        res_off.std_dev_dps = b_off.std_dev_dps;
        res_off.min_dps = b_off.min_dps;
        res_off.max_dps = b_off.max_dps;
        res_off.isb_uptime = b_off.mean_isb_uptime;
        res_off.talents = sim_off.talents;
        res_off.gear = sim_off.gear;
        res_off.policy = sim_off.policy;
        res_off.mechanics = sim_off.mechanics;
        results.push_back(res_off);
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::explore_combinatorial_talents(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback,
    bool compare_all_races,
    bool calculate_stat_weights
) {
    // Generate diverse valid 51-point distributions across Affliction / Demonology / Destruction
    struct TalentPointSplit {
        std::string name;
        int aff;
        int demo;
        int destro;
        bool sac_succubus;
        bool sac_imp;
    };

    std::vector<TalentPointSplit> splits = {
        {"5/11/35 DS/AF DS-Imp", 5, 11, 35, false, true},
        {"19/11/21 NF/DS/Ruin DS-Imp", 19, 11, 21, false, true},
        {"9/11/31 Fire Destro+Suppression DS-Succ", 9, 11, 31, true, false},
        {"2/31/18 DP/AF Shadow DS-Imp", 2, 31, 18, false, true},
        {"0/31/20 DP/AF Fire DS-Succ", 0, 31, 20, true, false},
        {"40/11/0 Deep Affliction DS-Imp", 40, 11, 0, false, true},
        {"32/0/19 SM/AF", 32, 0, 19, false, false},
        {"8/12/31 Shadow and Flame Fire 2", 8, 12, 31, false, false},
        {"20/0/31 Aff/Destro Conflagrate", 20, 0, 31, false, false},
        {"0/31/20 Decimation Execute", 0, 31, 20, false, false},
        {"11/20/20 Triple Tree", 11, 20, 20, false, false}
    };

    auto build_talents_from_split = [](int A, int D, int X, bool is_fire) {
        Talents t;
        // Affliction allocation
        int rem_a = A;
        t.aff.improved_life_tap = std::min(2, rem_a); rem_a -= t.aff.improved_life_tap;
        t.aff.suppression = std::min(5, rem_a); rem_a -= t.aff.suppression;
        t.aff.improved_corruption = std::min(5, rem_a); rem_a -= t.aff.improved_corruption;
        t.aff.malediction = std::min(5, rem_a); rem_a -= t.aff.malediction;
        t.aff.improved_bane_of_agony = std::min(2, rem_a); rem_a -= t.aff.improved_bane_of_agony;
        t.aff.pandemic = std::min(3, rem_a); rem_a -= t.aff.pandemic;
        t.aff.nightfall = std::min(2, rem_a); rem_a -= t.aff.nightfall;
        t.aff.shadow_mastery = std::min(5, rem_a); rem_a -= t.aff.shadow_mastery;
        if (rem_a >= 1) { t.aff.drain_hope = 1; rem_a -= 1; }

        // Demonology allocation
        int rem_d = D;
        t.demo.demonic_embrace = std::min(5, rem_d); rem_d -= t.demo.demonic_embrace;
        t.demo.improved_imp = std::min(3, rem_d); rem_d -= t.demo.improved_imp;
        t.demo.unholy_power = std::min(5, rem_d); rem_d -= t.demo.unholy_power;
        t.demo.fel_vitality = std::min(3, rem_d); rem_d -= t.demo.fel_vitality;
        t.demo.improved_sayaad = std::min(3, rem_d); rem_d -= t.demo.improved_sayaad;
        if (rem_d >= 1) { t.demo.demonic_sacrifice = 1; rem_d -= 1; }
        t.demo.decimation = std::min(2, rem_d); rem_d -= t.demo.decimation;
        t.demo.demonic_knowledge = std::min(3, rem_d); rem_d -= t.demo.demonic_knowledge;
        t.demo.master_demonologist = std::min(5, rem_d); rem_d -= t.demo.master_demonologist;
        if (rem_d >= 1) { t.demo.demonic_pact = 1; rem_d -= 1; }

        // Destruction allocation
        int rem_x = X;
        if (!is_fire) {
            t.destro.improved_shadow_bolt = std::min(5, rem_x); rem_x -= t.destro.improved_shadow_bolt;
        } else {
            t.destro.destructive_reach = std::min(2, rem_x); rem_x -= t.destro.destructive_reach;
        }
        t.destro.bane = std::min(5, rem_x); rem_x -= t.destro.bane;
        t.destro.cataclysm = std::min(3, rem_x); rem_x -= t.destro.cataclysm;
        t.destro.aftermath = std::min(5, rem_x); rem_x -= t.destro.aftermath;
        t.destro.ruin = std::min(5, rem_x); rem_x -= t.destro.ruin;
        if (rem_x >= 1) { t.destro.shadowburn = 1; rem_x -= 1; }
        t.destro.agonizing_flames = std::min(3, rem_x); rem_x -= t.destro.agonizing_flames;
        if (rem_x >= 1) { t.destro.conflagrate = 1; rem_x -= 1; }
        if (rem_x >= 1) { t.destro.bane_of_havoc = 1; rem_x -= 1; }
        t.destro.fire_and_brimstone = std::min(3, rem_x); rem_x -= t.destro.fire_and_brimstone;
        t.destro.shadow_and_flame = std::min(5, rem_x); rem_x -= t.destro.shadow_and_flame;
        if (rem_x >= 1) { t.destro.incinerate = 1; rem_x -= 1; }
        if (rem_x > 0 && is_fire && t.destro.improved_shadow_bolt == 0) {
            t.destro.improved_shadow_bolt = std::min(5, rem_x); rem_x -= t.destro.improved_shadow_bolt;
        }
        return t;
    };

    std::vector<Race> races_to_test = compare_all_races
        ? std::vector<Race>{Race::UNDEAD, Race::ORC, Race::TROLL, Race::HUMAN, Race::GNOME}
        : std::vector<Race>{base_sim.race};

    std::vector<CandidateResult> results;
    int total = static_cast<int>(splits.size() * races_to_test.size());
    int current_idx = 0;

    for (const auto& split : splits) {
        for (Race r : races_to_test) {
            std::string run_name = split.name;
            if (compare_all_races) {
                run_name += std::string(" [") + race_to_string(r) + "]";
            }
            if (callback) callback(static_cast<float>(current_idx++) / total, run_name);

            WarlockSimulator sim = base_sim;
            sim.race = r;
            sim.base_attrs = get_base_attributes_for_race(r);
            sim.talents = build_talents_from_split(split.aff, split.demo, split.destro, split.sac_succubus);
            sim.buffs.sacrifice_succubus = split.sac_succubus;
            sim.buffs.sacrifice_imp = split.sac_imp;
            if (split.sac_succubus && sim.talents.demo.demonic_pact > 0) {
                sim.policy.maintain_immolate = true;
                sim.policy.rotation = RotationChoice::DP_RUIN_FIRE;
                sim.policy.pet = PetChoice::IMP;
            } else if (split.sac_succubus) {
                sim.policy.maintain_immolate = true;
                sim.policy.rotation = RotationChoice::FIRE_DESTRO;
                sim.policy.pet = PetChoice::NONE;
            } else if (split.sac_imp && sim.talents.demo.demonic_pact > 0) {
                sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else if (split.sac_imp) {
                sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
                sim.policy.pet = PetChoice::NONE;
            } else if (split.aff >= 40) {
                sim.policy.rotation = RotationChoice::DEEP_AFFLICTION;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else if (split.aff >= 25 && split.destro >= 20) {
                sim.policy.rotation = RotationChoice::SM_RUIN;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else if (split.demo >= 30) {
                sim.policy.rotation = RotationChoice::DEMONOLOGY_EXECUTE;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else {
                sim.policy.rotation = RotationChoice::AFFLICTION_HYBRID_DOTS;
                sim.policy.pet = PetChoice::SUCCUBUS;
            }

            BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

            CandidateResult res;
            res.name = split.name;
            if (compare_all_races) {
                res.name += std::string(" (") + race_to_string(r) + ")";
            }
            res.race = r;
            res.category = "Combinatorial Talents";
            res.mean_dps = batch.mean_dps;
            res.std_dev_dps = batch.std_dev_dps;
            res.min_dps = batch.min_dps;
            res.max_dps = batch.max_dps;
            res.isb_uptime = batch.mean_isb_uptime;
            res.talents = sim.talents;
            res.gear = sim.gear;
            res.buffs = sim.buffs;
            res.policy = sim.policy;
            res.mechanics = sim.mechanics;
            res.batch = batch;

            if (calculate_stat_weights) {
                if (callback) callback(static_cast<float>(current_idx - 1) / total, run_name + " (Stat Weights)");
                res.stat_weights = calculate_candidate_stat_weights(sim, std::max(1000, iterations_per_candidate / 2));
            }

            results.push_back(res);
        }
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::optimize_genetic_ai(
    const WarlockSimulator& base_sim,
    int population_size,
    int generations,
    int screening_sims,
    int final_sims,
    bool seed_with_presets,
    bool optimize_race,
    double mutation_rate,
    double initial_exploration,
    double min_exploration,
    const std::vector<int>& required_talents,
    int forced_race,
    int forced_rotation,
    std::function<void(float progress, const std::string& current_name)> callback,
    std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback,
    const std::atomic<bool>* should_stop
) {
    GeneticOptimizerConfig cfg;
    cfg.population_size = population_size;
    cfg.generations = generations;
    cfg.screening_sims = screening_sims;
    cfg.final_sims = final_sims;
    cfg.seed_with_presets = seed_with_presets;
    cfg.optimize_race = optimize_race;
    cfg.mutation_rate = mutation_rate;
    cfg.initial_exploration_rate = initial_exploration;
    cfg.min_exploration_rate = min_exploration;
    cfg.required_talent_indices = required_talents;
    cfg.forced_race = forced_race;
    cfg.forced_rotation = forced_rotation;
    cfg.offspring_pool_size = std::max(100, population_size * 3);
    cfg.simulated_offspring_per_gen = std::max(20, population_size / 2);

    auto summary = GeneticOptimizer::run(base_sim, cfg, callback, generation_callback, should_stop);
    return summary.top_candidates;
}

std::vector<CandidateResult> Optimizer::compare_consumable_tiers(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback
) {
    struct ConsumeTier {
        std::string name;
        BuffConfig buffs;
    };

    // Tier 0: Naked (Only AI, Mark of the Wild, Kings, Wisdom - No consumables or world buffs)
    BuffConfig t0 = base_sim.buffs;
    t0.flask_of_supreme_power = false;
    t0.greater_arcane_elixir = false;
    t0.elixir_of_shadow_power = false;
    t0.brilliant_wizard_oil = false;
    t0.use_mana_potions = false;
    t0.use_demonic_runes = false;
    t0.rallying_cry = false;
    t0.songflower = false;
    t0.spirit_of_zandalar = false;
    t0.warchiefs_blessing = false;
    t0.sayges_fortune = false;

    // Tier 1: Basic Dungeon Consumes (Shadow Power + Mana Pots)
    BuffConfig t1 = t0;
    t1.elixir_of_shadow_power = true;
    t1.use_mana_potions = true;
    t1.use_demonic_runes = true;

    // Tier 2: Full Raid Consumes (Flask + Elixirs + Oils + Pots/Runes)
    BuffConfig t2 = t1;
    t2.flask_of_supreme_power = true;
    t2.greater_arcane_elixir = true;
    t2.brilliant_wizard_oil = true;

    // Tier 3: Full Raid Consumes + World Buffs (Dragonslayer + Songflower + Hakkar + DMF)
    BuffConfig t3 = t2;
    t3.rallying_cry = true;
    t3.songflower = true;
    t3.spirit_of_zandalar = true;
    t3.warchiefs_blessing = true;
    t3.sayges_fortune = true;

    std::vector<ConsumeTier> tiers = {
        {"Tier 0: Naked (Raid Buffs Only, 0 Consumes)", t0},
        {"Tier 1: Basic Consumes (Elixir of Shadow Power + Pots)", t1},
        {"Tier 2: Full Raid Consumes (Flask, Oils, Elixirs, Pots)", t2},
        {"Tier 3: Full Consumes + World Buffs (Onyxia, Songflower, DMF)", t3}
    };

    std::vector<CandidateResult> results;
    int total = static_cast<int>(tiers.size());

    for (int i = 0; i < total; ++i) {
        if (callback) callback(static_cast<float>(i) / total, tiers[i].name);

        WarlockSimulator sim = base_sim;
        sim.buffs = tiers[i].buffs;

        BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

        CandidateResult res;
        res.name = tiers[i].name;
        res.category = "Consumables";
        res.race = sim.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.isb_uptime = batch.mean_isb_uptime;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.buffs = sim.buffs;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.batch = batch;
        results.push_back(res);
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::compare_stat_values(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback
) {
    // Stat Sensitivity & Equivalence Point (EP) Testing
    Stats baseline_stats = base_sim.use_raw_stats ? base_sim.raw_stats : base_sim.gear.calculate_stats();

    struct StatTest {
        std::string name;
        Stats stats;
    };

    std::vector<StatTest> tests;

    // 1. Baseline
    tests.push_back({"Baseline Stats", baseline_stats});

    // 2. +10 Spell Power
    {
        Stats s = baseline_stats;
        s.spell_power += 10.0;
        tests.push_back({"+10 Spell Power (All Schools)", s});
    }

    // 3. +25 Spell Power
    {
        Stats s = baseline_stats;
        s.spell_power += 25.0;
        tests.push_back({"+25 Spell Power (All Schools)", s});
    }

    // 4. +1.0% Spell Hit
    {
        Stats s = baseline_stats;
        s.spell_hit_percent += 1.0;
        tests.push_back({"+1.0% Spell Hit Chance", s});
    }

    // 5. +2.0% Spell Hit
    {
        Stats s = baseline_stats;
        s.spell_hit_percent += 2.0;
        tests.push_back({"+2.0% Spell Hit Chance", s});
    }

    // 6. +1.0% Spell Crit
    {
        Stats s = baseline_stats;
        s.spell_crit_percent += 1.0;
        tests.push_back({"+1.0% Spell Crit Chance", s});
    }

    // 7. +2.0% Spell Crit
    {
        Stats s = baseline_stats;
        s.spell_crit_percent += 2.0;
        tests.push_back({"+2.0% Spell Crit Chance", s});
    }

    // 8. +15 Intellect (~0.25% Crit + Mana)
    {
        Stats s = baseline_stats;
        s.intellect += 15.0;
        tests.push_back({"+15 Intellect", s});
    }

    // 9. +20 MP5
    {
        Stats s = baseline_stats;
        s.mp5 += 20.0;
        tests.push_back({"+20 Mana Regen (MP5)", s});
    }

    std::vector<CandidateResult> results;
    int total = static_cast<int>(tests.size());

    for (int i = 0; i < total; ++i) {
        if (callback) callback(static_cast<float>(i) / total, tests[i].name);

        WarlockSimulator sim = base_sim;
        sim.use_raw_stats = true;
        sim.raw_stats = tests[i].stats;

        BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

        CandidateResult res;
        res.name = tests[i].name;
        res.category = "Stat Values (EP)";
        res.race = sim.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.isb_uptime = batch.mean_isb_uptime;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.buffs = sim.buffs;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.use_raw_stats = true;
        res.raw_stats = sim.raw_stats;
        res.batch = batch;
        results.push_back(res);
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

std::vector<CandidateResult> Optimizer::perturb_preset(
    const WarlockSimulator& base_sim,
    int iterations_per_candidate,
    std::function<void(float progress, const std::string& current_name)> callback
) {
    struct PerturbCandidate {
        std::string name;
        Talents talents;
        PolicyConfig policy;
        BuffConfig buffs;
    };

    std::vector<PerturbCandidate> candidates;

    // 1. Baseline: Exact current setup
    candidates.push_back({"[Baseline] Current Active Preset", base_sim.talents, base_sim.policy, base_sim.buffs});

    // 2. Rotational Perturbations
    // A. Corruption policy
    {
        auto p = base_sim.policy;
        if (p.corruption == DotPolicy::ALWAYS) {
            p.corruption = DotPolicy::NEVER;
            candidates.push_back({"[APL] Drop Corruption (Pure Filler Spam)", base_sim.talents, p, base_sim.buffs});
        } else {
            p.corruption = DotPolicy::ALWAYS;
            candidates.push_back({"[APL] Maintain Corruption (Pandemic DoT)", base_sim.talents, p, base_sim.buffs});
        }
    }

    // B. Immolate policy
    {
        auto p = base_sim.policy;
        p.maintain_immolate = !p.maintain_immolate;
        std::string label = p.maintain_immolate ? "[APL] Maintain Immolate" : "[APL] Drop Immolate";
        candidates.push_back({label, base_sim.talents, p, base_sim.buffs});
    }

    // C. Shadowburn policy
    if (base_sim.talents.destro.shadowburn > 0) {
        auto p1 = base_sim.policy;
        p1.shadowburn = ShadowburnPolicy::ON_COOLDOWN;
        candidates.push_back({"[APL] Shadowburn on Cooldown", base_sim.talents, p1, base_sim.buffs});

        auto p2 = base_sim.policy;
        p2.shadowburn = ShadowburnPolicy::EXECUTE_ONLY;
        candidates.push_back({"[APL] Shadowburn Execute Only (<20%)", base_sim.talents, p2, base_sim.buffs});

        auto p3 = base_sim.policy;
        p3.shadowburn = ShadowburnPolicy::NEVER;
        candidates.push_back({"[APL] Never Cast Shadowburn", base_sim.talents, p3, base_sim.buffs});
    }

    // D. Curse / Bane choices
    {
        auto p_agony = base_sim.policy;
        p_agony.curse = CurseChoice::BANE_OF_AGONY;
        candidates.push_back({"[APL] Bane: Bane of Agony (Pandemic Crits)", base_sim.talents, p_agony, base_sim.buffs});

        auto p_doom = base_sim.policy;
        p_doom.curse = CurseChoice::CURSE_OF_DOOM;
        candidates.push_back({"[APL] Curse: Curse of Doom (1-min Burst)", base_sim.talents, p_doom, base_sim.buffs});

        auto p_none = base_sim.policy;
        p_none.curse = CurseChoice::NONE;
        candidates.push_back({"[APL] Bane/Curse: None", base_sim.talents, p_none, base_sim.buffs});
    }

    // E. Decimation Soul Fire Execute (<35% HP)
    if (base_sim.talents.demo.decimation > 0) {
        auto p = base_sim.policy;
        p.use_decimation_soul_fire = !p.use_decimation_soul_fire;
        std::string label = p.use_decimation_soul_fire ? "[APL] Decimation: Soul Fire on Execute" : "[APL] Decimation: Skip Soul Fire (Cast SB)";
        candidates.push_back({label, base_sim.talents, p, base_sim.buffs});
    }

    // F. Drain Hope Channeled
    if (base_sim.talents.aff.drain_hope > 0) {
        auto p = base_sim.policy;
        p.channel_drain_hope = !p.channel_drain_hope;
        std::string label = p.channel_drain_hope ? "[APL] Channel Drain Hope on CD" : "[APL] Skip Drain Hope (Shadow Bolt Filler)";
        candidates.push_back({label, base_sim.talents, p, base_sim.buffs});
    }

    // G. Pet / Sacrifice Variations
    {
        auto p_succ = base_sim.policy;
        p_succ.pet = PetChoice::SUCCUBUS;
        auto b_succ = base_sim.buffs;
        b_succ.sacrifice_imp = false;
        b_succ.sacrifice_succubus = false;
        candidates.push_back({"[Pet] Active Succubus (Melee + Lash of Pain)", base_sim.talents, p_succ, b_succ});

        auto p_imp = base_sim.policy;
        p_imp.pet = PetChoice::IMP;
        candidates.push_back({"[Pet] Active Imp (Firebolt Support)", base_sim.talents, p_imp, b_succ});

        if (base_sim.talents.demo.demonic_sacrifice > 0) {
            auto p_sac_imp = base_sim.policy;
            p_sac_imp.pet = PetChoice::NONE;
            auto b_sac_imp = base_sim.buffs;
            b_sac_imp.sacrifice_imp = true;
            b_sac_imp.sacrifice_succubus = false;
            candidates.push_back({"[Pet] Sacrificed Imp (+15% Shadow)", base_sim.talents, p_sac_imp, b_sac_imp});

            auto p_sac_succ = base_sim.policy;
            p_sac_succ.pet = PetChoice::NONE;
            auto b_sac_succ = base_sim.buffs;
            b_sac_succ.sacrifice_imp = false;
            b_sac_succ.sacrifice_succubus = true;
            candidates.push_back({"[Pet] Sacrificed Succubus (+15% Fire)", base_sim.talents, p_sac_succ, b_sac_succ});
        }
    }

    // 3. Targeted Talent Perturbations
    // Check if Affliction/Destruction hybrid (e.g. SM/AF style)
    if (base_sim.talents.aff.total_points() >= 25 && base_sim.talents.destro.ruin > 0) {
        candidates.push_back({"[Talents] SM/AF: 2/5 SM + 3/3 Agonizing Flames (29/0/22)", Talents::create_forever_sm_ruin_max_flames(), base_sim.policy, base_sim.buffs});
        candidates.push_back({"[Talents] SM/AF: 3/5 SM + 2/3 Agonizing Flames (30/0/21)", Talents::create_forever_sm_ruin(), base_sim.policy, base_sim.buffs});
        candidates.push_back({"[Talents] SM/AF: 5/5 SM + 0/3 Agonizing Flames (32/0/19)", Talents::create_forever_sm_ruin_pure(), base_sim.policy, base_sim.buffs});
        
        Talents t_reach = Talents::create_forever_sm_ruin_max_flames();
        t_reach.destro.cataclysm = 1;
        t_reach.destro.destructive_reach = 2;
        candidates.push_back({"[Talents] SM/AF: +20% Destructive Reach (Shift from Cata)", t_reach, base_sim.policy, base_sim.buffs});
    }

    // Check if Demonology build (Demonic Pact vs MD Ruin vs DS Ruin)
    if (base_sim.talents.demo.total_points() >= 20) {
        auto b_dp = base_sim.buffs;
        b_dp.sacrifice_imp = true;
        auto p_dp = base_sim.policy;
        p_dp.pet = PetChoice::SUCCUBUS;
        p_dp.rotation = RotationChoice::DP_AF_SHADOW;
        candidates.push_back({"[Talents] DP/AF Shadow (2/31/18 - Sac Imp + Succubus)", Talents::create_forever_dp_af_shadow(), p_dp, b_dp});

        auto b_md = base_sim.buffs;
        b_md.sacrifice_imp = false;
        candidates.push_back({"[Talents] MD / Ruin (0/31/20 - 5/5 MD + 5/5 Ruin)", Talents::create_forever_md_ruin(), p_dp, b_md});

        auto b_ds = base_sim.buffs;
        b_ds.sacrifice_imp = true;
        auto p_ds = base_sim.policy;
        p_ds.pet = PetChoice::NONE;
        candidates.push_back({"[Talents] DS/AF (5/11/35 - Sac Imp + Ruin)", Talents::create_forever_ds_af(), p_ds, b_ds});
    }

    // Check if Destruction / Fire build
    if (base_sim.talents.destro.total_points() >= 30) {
        auto b_fire = base_sim.buffs;
        b_fire.sacrifice_succubus = true;
        auto p_fire = base_sim.policy;
        p_fire.rotation = RotationChoice::FIRE_DESTRO;
        p_fire.maintain_immolate = true;
        p_fire.pet = PetChoice::NONE;
        candidates.push_back({"[Talents] Fire Destro: Incinerate + Sac Succubus (5/11/35)", Talents::create_forever_fire_destro(), p_fire, b_fire});

        auto b_shadow = base_sim.buffs;
        b_shadow.sacrifice_imp = true;
        auto p_shadow = base_sim.policy;
        p_shadow.rotation = RotationChoice::SHADOW_DESTRO;
        p_shadow.pet = PetChoice::NONE;
        candidates.push_back({"[Talents] DS/AF: Conflag Weave + Ruin (5/11/35)", Talents::create_forever_ds_af(), p_shadow, b_shadow});
    }

    std::vector<CandidateResult> results;
    int total = static_cast<int>(candidates.size());

    for (int i = 0; i < total; ++i) {
        if (callback) callback(static_cast<float>(i) / total, candidates[i].name);

        WarlockSimulator sim = base_sim;
        sim.talents = candidates[i].talents;
        sim.policy = candidates[i].policy;
        sim.buffs = candidates[i].buffs;

        BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations_per_candidate);

        CandidateResult res;
        res.name = candidates[i].name;
        res.category = "Perturbations";
        res.race = sim.race;
        res.mean_dps = batch.mean_dps;
        res.std_dev_dps = batch.std_dev_dps;
        res.min_dps = batch.min_dps;
        res.max_dps = batch.max_dps;
        res.isb_uptime = batch.mean_isb_uptime;
        res.talents = sim.talents;
        res.gear = sim.gear;
        res.buffs = sim.buffs;
        res.policy = sim.policy;
        res.mechanics = sim.mechanics;
        res.batch = batch;
        results.push_back(res);
    }

    std::sort(results.begin(), results.end(), [](const CandidateResult& a, const CandidateResult& b) {
        return a.mean_dps > b.mean_dps;
    });

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].rank = static_cast<int>(i + 1);
    }

    if (callback) callback(1.0f, "Completed");
    return results;
}

} // namespace warlock
