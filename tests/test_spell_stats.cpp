#include "test_framework.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/parallel_runner.hpp"

using namespace warlock;

static WarlockSimulator make_spell_stats_sim() {
    WarlockSimulator sim;
    sim.race = Race::UNDEAD;
    sim.talents = Talents();
    sim.policy.pet = PetChoice::IMP;
    sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim.policy.corruption = DotPolicy::ALWAYS;
    sim.buffs.sacrifice_imp = false;
    sim.buffs.sacrifice_succubus = false;
    sim.mechanics.pet_mana_management = false;
    sim.use_raw_stats = true;
    sim.fight_duration = 30.0;
    return sim;
}

TEST_CASE(SpellStats, DirectSpellInvariants) {
    WarlockSimulator sim = make_spell_stats_sim();
    FastRNG rng(777);
    SimResult res = sim.run_single_simulation(rng);

    const SpellCombatStats& sb = res.spell_stats[static_cast<size_t>(SpellID::SHADOW_BOLT)];
    CHECK(sb.casts > 0);
    CHECK(sb.hits + sb.misses <= sb.casts); // completed casts resolve as hit or miss (in-flight cast at fight end does not land)
    CHECK(sb.hits + sb.misses > 0);
    CHECK(sb.crits <= sb.hits);
    CHECK(sb.damage > 0.0);
    CHECK_EQ(res.dmg_shadow_bolt, sb.damage); // per-spell damage matches the legacy bucket
}

TEST_CASE(SpellStats, TicksCountAsHits) {
    WarlockSimulator sim = make_spell_stats_sim();
    FastRNG rng(777);
    SimResult res = sim.run_single_simulation(rng);

    const SpellCombatStats& corr = res.spell_stats[static_cast<size_t>(SpellID::CORRUPTION)];
    CHECK(corr.casts > 0);
    CHECK(corr.hits > corr.casts); // 6 ticks per application count as hits
    CHECK(corr.crits <= corr.hits);
    CHECK_EQ(res.dmg_corruption, corr.damage);
}

TEST_CASE(SpellStats, PetFireboltInvariants) {
    WarlockSimulator sim = make_spell_stats_sim();
    FastRNG rng(777);
    SimResult res = sim.run_single_simulation(rng);

    const SpellCombatStats& fb = res.spell_stats[static_cast<size_t>(SpellID::PET_FIREBOLT)];
    CHECK(fb.casts > 0);
    CHECK_EQ(fb.hits + fb.misses, fb.casts);
    CHECK(fb.crits <= fb.hits);
    CHECK_EQ(res.dmg_pet_firebolt, fb.damage);
}

TEST_CASE(SpellStats, BatchAggregation) {
    WarlockSimulator sim = make_spell_stats_sim();
    BatchSimResult batch = ParallelSimRunner::run_batch(sim, 8, 2, nullptr, 0xC0FFEEULL);

    const BatchSpellStats& sb = batch.spell_stats[static_cast<size_t>(SpellID::SHADOW_BOLT)];
    CHECK(sb.mean_casts > 0.0);
    CHECK(sb.mean_hits > 0.0);
    CHECK(spell_avg_hit(sb) > 0.0);
    double crit = spell_crit_pct(sb);
    double miss = spell_miss_pct(sb);
    CHECK(crit >= 0.0 && crit <= 100.0);
    CHECK(miss >= 0.0 && miss <= 100.0);
    CHECK(sb.mean_hits + sb.mean_misses > 0.0);

    const BatchSpellStats& corr = batch.spell_stats[static_cast<size_t>(SpellID::CORRUPTION)];
    CHECK(corr.mean_hits > corr.mean_casts); // ticks survive aggregation
}
