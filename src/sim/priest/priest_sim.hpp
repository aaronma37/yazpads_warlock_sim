#pragma once
#include <vector>
#include <array>
#include <string>
#include "src/sim/common/des_engine.hpp"
#include "src/sim/common/stats.hpp"
#include "src/sim/common/gear.hpp"
#include "src/sim/common/buffs.hpp"
#include "src/sim/common/spell_types.hpp"
#include "src/sim/common/mana_regen.hpp"
#include "talents.hpp"
#include "policy.hpp"
#include "mechanics.hpp"
#include "spells.hpp"

namespace priest {

struct TimelineEntry {
    double time = 0.0;
    double damage = 0.0;
    SpellID spell_id = SpellID::NONE;
    bool is_crit = false;
    bool is_miss = false;
    double mana = 0.0;
    int shadow_weaving_stacks = 0;
};

struct SpellCastLog {
    double time = 0.0;
    SpellID spell_id = SpellID::NONE;
    double damage = 0.0;
    bool is_crit = false;
    bool is_miss = false;
    double cast_time = 0.0;
    std::string tag;
};

struct SimResult {
    double duration = 0.0;
    double total_damage = 0.0;
    double dps = 0.0;

    std::array<sim::SpellCombatStats, static_cast<size_t>(SpellID::COUNT)> spell_stats{};

    void record_spell_cast(SpellID id) {
        spell_stats[static_cast<size_t>(id)].casts++;
        total_casts++;
    }
    void record_spell_hit(SpellID id, double dmg, bool crit) {
        sim::SpellCombatStats& s = spell_stats[static_cast<size_t>(id)];
        s.hits++;
        s.damage += dmg;
        total_damage_events++;
        if (crit) {
            s.crits++;
            total_damage_crits++;
        }
    }
    void record_spell_miss(SpellID id) {
        spell_stats[static_cast<size_t>(id)].misses++;
        misses++;
    }

    int total_casts = 0;
    int total_damage_events = 0;
    int total_damage_crits = 0;
    int misses = 0;
    int partial_resists = 0;

    double mana_spent = 0.0;
    double mana_gained = 0.0;
    int shadow_weaving_procs = 0;
    double mean_shadow_weaving_stacks = 0.0;

    // Damage breakdown by spell
    double dmg_sw_pain = 0.0;
    double dmg_mind_flay = 0.0;
    double dmg_mind_blast = 0.0;
    double dmg_sw_death = 0.0;
    double dmg_devouring_plague = 0.0;
    double dmg_smite = 0.0;
    double dmg_holy_fire = 0.0;
    double dmg_penance = 0.0;
    double dmg_holy_nova = 0.0;
    double dmg_starshards = 0.0;
    double dmg_chastise = 0.0;
    double dmg_shadowguard = 0.0;
    double dmg_touch_of_the_grave = 0.0;
    int touch_of_the_grave_procs = 0;

    double healing_vampiric_embrace = 0.0;
    double healing_devouring_plague = 0.0;
    double self_damage_sw_death = 0.0;
    int clearcast_holy_nova_procs = 0;

    std::vector<TimelineEntry> timeline;
    std::vector<SpellCastLog> cast_sequence;
};

class PriestSimulator {
public:
    sim::Race race = sim::Race::HUMAN;
    sim::BaseAttributes base_attrs;
    sim::GearLoadout gear;
    Talents talents;
    sim::BuffConfig buffs;
    PolicyConfig policy;
    MechanicsConfig mechanics;
    sim::TargetConfig target_config;

    bool use_raw_stats = true;
    sim::Stats raw_stats;

    double fight_duration = 180.0;
    bool randomize_duration = false;
    double duration_variance = 30.0;
    bool record_timeline = false;

    PriestSimulator();

    // Runs a single deterministic or stochastic DES iteration
    SimResult run_single_simulation(sim::FastRNG& rng);

    // Calculation helpers
    double calculate_hit_chance(sim::School school) const;
    double calculate_crit_chance(sim::School school, const sim::Stats& stats) const;
    double calculate_partial_resist_multiplier(sim::School school, double target_resistance, sim::FastRNG& rng) const;
};

} // namespace priest
