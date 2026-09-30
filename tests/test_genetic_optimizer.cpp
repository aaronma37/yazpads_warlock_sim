#include "test_framework.hpp"
#include "src/sim/talent_graph.hpp"
#include "src/sim/surrogate_model.hpp"
#include "src/sim/genetic_optimizer.hpp"
#include "src/sim/spec_presets.hpp"

using namespace warlock;

TEST_CASE(GeneticOptimizer, TalentGraphPresetsValidity) {
    const auto& graph = TalentGraph::get();
    for (const auto& preset : standard_spec_presets()) {
        Talents t = preset.make_talents();
        auto vec = graph.to_vector(t);
        bool valid = graph.is_valid(vec, 51);
        if (!valid) {
            std::cout << "Preset FAILED is_valid: " << preset.display_name << std::endl;
            for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                if (vec[i] > 0) {
                    std::cout << "  node " << i << " (" << graph.node(i).name << "): " << vec[i] 
                              << " (row " << graph.node(i).row << ", tree " << graph.node(i).tree_idx << ")" << std::endl;
                }
            }
        }
        CHECK(valid);
        Talents roundtrip = graph.to_talents(vec);
        CHECK_EQ(roundtrip.aff.total_points(), t.aff.total_points());
        CHECK_EQ(roundtrip.demo.total_points(), t.demo.total_points());
        CHECK_EQ(roundtrip.destro.total_points(), t.destro.total_points());
    }
}

TEST_CASE(GeneticOptimizer, TalentGraphRandomAndRepair) {
    const auto& graph = TalentGraph::get();
    FastRNG rng(42);

    for (int i = 0; i < 50; ++i) {
        auto vec = graph.generate_random_valid(rng, 51);
        CHECK(graph.is_valid(vec, 51));

        // Corrupt random positions
        vec[rng.next_u64() % TOTAL_TALENT_NODES] = 10; // rank beyond max
        vec[rng.next_u64() % TOTAL_TALENT_NODES] = -5; // negative rank
        graph.repair(vec, rng, 51);
        CHECK(graph.is_valid(vec, 51));
    }
}

TEST_CASE(GeneticOptimizer, SurrogateModelTraining) {
    SurrogateModel surrogate(1e-2);
    const auto& graph = TalentGraph::get();
    FastRNG rng(1337);

    // Generate synthetic training points
    for (int i = 0; i < 30; ++i) {
        auto vec = graph.generate_random_valid(rng, 51);
        double dps = 500.0 + 10.0 * vec[36 + 6] + 5.0 * vec[15]; // Ruin and Shadow Mastery boost
        surrogate.add_sample(SurrogateSample{
            vec,
            PetChoice::IMP,
            false,
            false,
            RotationChoice::SHADOW_DESTRO,
            true,
            CurseChoice::BANE_OF_AGONY,
            Race::UNDEAD,
            dps
        });
    }

    bool trained = surrogate.train();
    CHECK(trained);
    CHECK(surrogate.is_trained());

    auto test_vec = graph.generate_random_valid(rng, 51);
    double pred = surrogate.predict(test_vec, PetChoice::IMP, false, false, RotationChoice::SHADOW_DESTRO, true, CurseChoice::BANE_OF_AGONY);
    CHECK(pred > 0.0);
}

TEST_CASE(GeneticOptimizer, EndToEndSmoke) {
    WarlockSimulator sim;
    GeneticOptimizerConfig cfg;
    cfg.population_size = 10;
    cfg.generations = 3;
    cfg.screening_sims = 50;
    cfg.final_sims = 100;
    cfg.offspring_pool_size = 20;
    cfg.simulated_offspring_per_gen = 5;

    auto summary = GeneticOptimizer::run(sim, cfg, nullptr);
    CHECK(!summary.top_candidates.empty());
    CHECK(!summary.diverse_peaks.empty());
    CHECK(summary.top_candidates[0].mean_dps > 0.0);
    CHECK_EQ(summary.generation_best_dps.size(), 3);
}

TEST_CASE(GeneticOptimizer, EnforceConstraintsTest) {
    WarlockSimulator sim;
    GeneticOptimizerConfig cfg;
    cfg.population_size = 10;
    cfg.generations = 3;
    cfg.screening_sims = 50;
    cfg.final_sims = 100;
    cfg.offspring_pool_size = 20;
    cfg.simulated_offspring_per_gen = 5;

    // Constrain to Ruin (Destro node index 36 + 6 = 42) and Shadow Mastery (Aff node index 15)
    cfg.required_talent_indices = { 15, 42 };
    // Lock race to Troll
    cfg.forced_race = static_cast<int>(Race::TROLL);
    // Lock rotation to SM_RUIN
    cfg.forced_rotation = static_cast<int>(RotationChoice::SM_RUIN);

    auto summary = GeneticOptimizer::run(sim, cfg, nullptr);
    CHECK(!summary.top_candidates.empty());
    
    const auto& graph = TalentGraph::get();
    for (const auto& cand : summary.top_candidates) {
        CHECK_EQ(static_cast<int>(cand.race), static_cast<int>(Race::TROLL));
        CHECK_EQ(static_cast<int>(cand.policy.rotation), static_cast<int>(RotationChoice::SM_RUIN));
        auto vec = graph.to_vector(cand.talents);
        CHECK_EQ(vec[15], 5); // Shadow Mastery maxed
        CHECK_EQ(vec[42], 5); // Ruin maxed
        CHECK(graph.is_valid(vec, 51));
    }
}

TEST_CASE(GeneticOptimizer, DemonicSacrificeTalentRequirement) {
    // Tests that Genetic Optimizer never allows Sac-Imp or Sac-Succubus when Demonic Sacrifice is missing
    WarlockSimulator sim;
    GeneticOptimizerConfig cfg;
    cfg.population_size = 20;
    cfg.generations = 4;
    cfg.screening_sims = 50;
    cfg.final_sims = 100;
    cfg.offspring_pool_size = 30;
    cfg.simulated_offspring_per_gen = 10;
    cfg.seed_with_presets = true;

    auto summary = GeneticOptimizer::run(sim, cfg, nullptr);
    CHECK(!summary.top_candidates.empty());

    for (const auto& cand : summary.top_candidates) {
        bool has_ds = (cand.talents.demo.demonic_sacrifice > 0);
        bool has_dp = (cand.talents.demo.demonic_pact > 0);
        if (!has_ds && !has_dp) {
            CHECK_EQ(cand.buffs.sacrifice_imp, false);
            CHECK_EQ(cand.buffs.sacrifice_succubus, false);
        }
    }

    for (const auto& cand : summary.diverse_peaks) {
        bool has_ds = (cand.talents.demo.demonic_sacrifice > 0);
        bool has_dp = (cand.talents.demo.demonic_pact > 0);
        if (!has_ds && !has_dp) {
            CHECK_EQ(cand.buffs.sacrifice_imp, false);
            CHECK_EQ(cand.buffs.sacrifice_succubus, false);
        }
    }

    // Direct simulation check: Setting sacrifice_imp = true on non-DS warlock must not grant +15% Shadow
    FastRNG rng1(42), rng2(42);
    WarlockSimulator non_ds_sim;
    non_ds_sim.talents = Talents(); // 0 points in demo
    non_ds_sim.buffs.sacrifice_imp = true;
    non_ds_sim.use_raw_stats = true;
    non_ds_sim.raw_stats.max_mana = 5000.0;
    non_ds_sim.raw_stats.max_health = 4000.0;
    non_ds_sim.raw_stats.spell_power = 500.0;
    non_ds_sim.raw_stats.spell_hit_percent = 100.0;
    non_ds_sim.raw_stats.spell_crit_percent = 0.0;
    non_ds_sim.mechanics.partial_resists_enabled = false;
    non_ds_sim.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
    non_ds_sim.fight_duration = 5.0; // 1 SB cast (3.0s cast + 1.25s projectile travel)

    SimResult res_no_ds = non_ds_sim.run_single_simulation(rng1);

    WarlockSimulator clean_sim = non_ds_sim;
    clean_sim.buffs.sacrifice_imp = false;
    SimResult res_clean = clean_sim.run_single_simulation(rng2);

    CHECK(res_no_ds.dmg_shadow_bolt > 0.0);
    // Erroneous +15% shadow multiplier from sac_imp without DS talent must be blocked
    CHECK_NEAR(res_no_ds.dmg_shadow_bolt, res_clean.dmg_shadow_bolt, 0.0001);
}

TEST_CASE(GeneticOptimizer, ForcedPetConstraintCombinations) {
    WarlockSimulator sim;
    GeneticOptimizerConfig cfg;
    cfg.population_size = 15;
    cfg.generations = 3;
    cfg.screening_sims = 30;
    cfg.final_sims = 50;
    cfg.offspring_pool_size = 15;
    cfg.simulated_offspring_per_gen = 5;

    // 1. Force SAC_IMP
    cfg.forced_pet_mode = static_cast<int>(PetConstraint::SAC_IMP);
    auto summary_sac_imp = GeneticOptimizer::run(sim, cfg, nullptr);
    CHECK(!summary_sac_imp.top_candidates.empty());
    for (const auto& cand : summary_sac_imp.top_candidates) {
        CHECK_EQ(cand.buffs.sacrifice_imp, true);
        CHECK_EQ(cand.buffs.sacrifice_succubus, false);
        CHECK_EQ(static_cast<int>(cand.policy.pet), static_cast<int>(PetChoice::NONE));
        CHECK(cand.talents.demo.demonic_sacrifice > 0);
    }

    // 2. Force DEMONIC_PACT_IMP_SUCC
    cfg.forced_pet_mode = static_cast<int>(PetConstraint::DEMONIC_PACT_IMP_SUCC);
    auto summary_dp = GeneticOptimizer::run(sim, cfg, nullptr);
    CHECK(!summary_dp.top_candidates.empty());
    for (const auto& cand : summary_dp.top_candidates) {
        CHECK_EQ(cand.buffs.sacrifice_imp, true);
        CHECK_EQ(cand.buffs.sacrifice_succubus, false);
        CHECK_EQ(static_cast<int>(cand.policy.pet), static_cast<int>(PetChoice::SUCCUBUS));
        CHECK(cand.talents.demo.demonic_pact > 0);
    }

    // 3. Force ACTIVE_IMP
    cfg.forced_pet_mode = static_cast<int>(PetConstraint::ACTIVE_IMP);
    auto summary_imp = GeneticOptimizer::run(sim, cfg, nullptr);
    CHECK(!summary_imp.top_candidates.empty());
    for (const auto& cand : summary_imp.top_candidates) {
        CHECK_EQ(cand.buffs.sacrifice_imp, false);
        CHECK_EQ(cand.buffs.sacrifice_succubus, false);
        CHECK_EQ(static_cast<int>(cand.policy.pet), static_cast<int>(PetChoice::IMP));
    }
}

TEST_CASE(GeneticOptimizer, PolicyMutationExploresEveryRotationAndLegalPetMode) {
    FastRNG rng(7123);
    Talents talents;
    talents.demo.demonic_pact = 1;
    talents.demo.demonic_brand = 3;
    genetic_detail::SearchPolicy policy;
    bool seen[genetic_detail::rotation_count][9]{};
    for (int i = 0; i < 15000; ++i) {
        genetic_detail::mutate_policy(policy, talents, rng, 1.0);
        const size_t mode = static_cast<size_t>(policy.pet) * 3 + (policy.sac_imp ? 1 : policy.sac_succubus ? 2 : 0);
        seen[static_cast<size_t>(policy.rotation)][mode] = true;
    }
    for (size_t r = 0; r < genetic_detail::rotation_count; ++r) {
        // None, two sacrifices, two active pets, and both Pact pairings.
        for (size_t p : {0u, 1u, 2u, 3u, 4u, 6u, 8u}) CHECK(seen[r][p]);
    }
    talents = Talents{};
    for (int i = 0; i < 200; ++i) {
        genetic_detail::mutate_policy(policy, talents, rng, 1.0);
        CHECK(!policy.sac_imp && !policy.sac_succubus);
    }
    talents.demo.demonic_sacrifice = 1;
    for (int i = 0; i < 200; ++i) {
        genetic_detail::mutate_policy(policy, talents, rng, 1.0);
        CHECK(!(policy.sac_imp || policy.sac_succubus) || policy.pet == PetChoice::NONE);
    }
}

TEST_CASE(GeneticOptimizer, PolicyCrossoverCanCombineBrandWithEitherPetPairing) {
    FastRNG rng(331);
    genetic_detail::SearchPolicy a, b;
    a.rotation = RotationChoice::DP_AF_SHADOW_BRAND;
    a.pet = PetChoice::SUCCUBUS;
    a.sac_imp = true;
    b.rotation = RotationChoice::DP_RUIN_FIRE;
    b.pet = PetChoice::IMP;
    b.sac_succubus = true;
    bool found = false;
    for (int i = 0; i < 100; ++i) {
        auto child = genetic_detail::crossover_policy(a, b, rng);
        CHECK(child.pet == PetChoice::IMP ? child.sac_succubus && !child.sac_imp : child.sac_imp && !child.sac_succubus);
        if (child.rotation == a.rotation && child.pet == b.pet) found = true;
    }
    CHECK(found);
}

TEST_CASE(GeneticOptimizer, SurrogateDistinguishesRotationAndSacrificedPetCombinations) {
    const auto talents = TalentGraph::get().to_vector(Talents::create_forever_aff_dp_brand());
    SurrogateModel model;
    for (int i = 0; i < 8; ++i) {
        model.add_sample({talents, PetChoice::IMP, false, true, RotationChoice::DP_AF_SHADOW, false, CurseChoice::BANE_OF_AGONY, Race::UNDEAD, 500.0});
        model.add_sample({talents, PetChoice::IMP, false, true, RotationChoice::DP_AF_SHADOW_BRAND, false, CurseChoice::BANE_OF_AGONY, Race::UNDEAD, 800.0});
        model.add_sample({talents, PetChoice::NONE, false, true, RotationChoice::DP_AF_SHADOW_BRAND, false, CurseChoice::BANE_OF_AGONY, Race::UNDEAD, 600.0});
    }
    CHECK(model.train());
    const double brand = model.predict(talents, PetChoice::IMP, false, true, RotationChoice::DP_AF_SHADOW_BRAND, false, CurseChoice::BANE_OF_AGONY);
    CHECK(brand > model.predict(talents, PetChoice::IMP, false, true, RotationChoice::DP_AF_SHADOW, false, CurseChoice::BANE_OF_AGONY) + 250.0);
    CHECK(brand > model.predict(talents, PetChoice::NONE, false, true, RotationChoice::DP_AF_SHADOW_BRAND, false, CurseChoice::BANE_OF_AGONY) + 150.0);
}

TEST_CASE(GeneticOptimizer, PactOnlySearchExploresUnseededBrandImpBuilds) {
    WarlockSimulator sim;
    sim.fight_duration = 10.0;
    // An active custom/learned controller must not replace the candidates' rotations.
    sim.policy.use_custom_apl = true;
    sim.policy.custom_rules = sim.policy.get_priority_rules(sim.talents, sim.race);
    sim.policy.use_imitation_policy = true;
    sim.policy.use_oracle_execution_policy = true;
    sim.policy.use_gbdt_policy = true;
    sim.use_oracle_execution_policy = true;
    sim.use_gbdt_policy = true;
    GeneticOptimizerConfig cfg;
    cfg.population_size = 600;
    cfg.generations = 1;
    cfg.screening_sims = 1;
    cfg.final_sims = 1;
    cfg.num_threads = 1;
    cfg.seed_with_presets = false;
    cfg.required_talent_indices = {static_cast<int>(AFFLICTION_NODE_COUNT + 18)};
    cfg.offspring_pool_size = 30;
    cfg.simulated_offspring_per_gen = 10;
    GeneticOptimizerSession session;
    session.start(sim, cfg);
    bool rotations[genetic_detail::rotation_count]{};
    bool found_brand_imp = false;
    for (const auto& cand : session.get_elites()) {
        CHECK(cand.talents.demo.demonic_pact > 0);
        CHECK(TalentGraph::get().is_valid(TalentGraph::get().to_vector(cand.talents), 51));
        CHECK(!cand.policy.use_custom_apl && !cand.policy.use_imitation_policy);
        CHECK(!cand.policy.use_oracle_execution_policy && !cand.policy.use_gbdt_policy);
        rotations[static_cast<size_t>(cand.policy.rotation)] = true;
        if (cand.talents.demo.demonic_brand > 0 && cand.policy.rotation == RotationChoice::DP_AF_SHADOW_BRAND &&
            cand.policy.pet == PetChoice::IMP && cand.buffs.sacrifice_succubus) {
            found_brand_imp = true;
            bool uses_brand = false;
            for (const auto& rule : cand.policy.get_priority_rules(cand.talents, cand.race))
                if (rule.action == PriorityAction::DEMONIC_BRAND_SEARING_PAIN) uses_brand = true;
            CHECK(uses_brand);
        }
    }
    for (bool seen : rotations) CHECK(seen);
    CHECK(found_brand_imp);
    // Verify user locks survive reproduction as well as initialization.
    cfg.population_size = 12;
    cfg.forced_rotation = static_cast<int>(RotationChoice::DP_AF_SHADOW_BRAND);
    cfg.forced_pet_mode = static_cast<int>(PetConstraint::DEMONIC_PACT_SUCC_IMP);
    auto summary = GeneticOptimizer::run(sim, cfg);
    for (const auto& cand : summary.top_candidates) {
        CHECK(cand.policy.rotation == RotationChoice::DP_AF_SHADOW_BRAND);
        CHECK(cand.policy.pet == PetChoice::IMP);
        CHECK(cand.buffs.sacrifice_succubus && !cand.buffs.sacrifice_imp);
    }
}
