#pragma once
#include <vector>
#include <array>
#include <string>
#include "des_engine.hpp"
#include "stats.hpp"
#include "talents.hpp"
#include "gear.hpp"
#include "buffs.hpp"
#include "policy.hpp"
#include "mechanics.hpp"
#include "spells.hpp"

namespace warlock {

struct TimelineEntry {
    double time = 0.0;
    double damage = 0.0;
    SpellID spell_id = SpellID::NONE;
    bool is_crit = false;
    bool is_miss = false;
    double mana = 0.0;
    int isb_charges = 0;
};

struct SpellCastLog {
    double time = 0.0;
    SpellID spell_id = SpellID::NONE;
    double damage = 0.0;
    bool is_crit = false;
    bool is_miss = false;
    double cast_time = 0.0;
    std::string tag; // "Opener", "Curse", "DoT", "Execute", "Proc", "Burst", "Filler", "Mana"
};

struct SimResult {
    double duration = 0.0;
    double total_damage = 0.0;
    double dps = 0.0;

    int total_casts = 0;
    int total_damage_events = 0;
    int total_damage_crits = 0;
    int crits = 0;
    int direct_spell_casts = 0;
    int direct_spell_crits = 0;
    int shadow_bolt_casts = 0;
    int shadow_bolt_hits = 0;
    int shadow_bolt_crits = 0;
    int misses = 0;
    int partial_resists = 0;

    int life_taps = 0;
    double mana_spent = 0.0;
    double mana_gained = 0.0;

    int isb_procs = 0;
    int isb_consumed = 0;
    double isb_uptime_percent = 0.0;

    int nightfall_procs = 0;

    // Damage breakdown by spell
    double dmg_shadow_bolt = 0.0;
    double dmg_corruption = 0.0;
    double dmg_curse = 0.0; // Total curse damage
    double dmg_agony = 0.0;
    double dmg_doom = 0.0;
    double dmg_siphon_life = 0.0;
    double dmg_immolate = 0.0;
    double dmg_shadowburn = 0.0;
    double dmg_conflagrate = 0.0;
    double dmg_incinerate = 0.0;
    double dmg_soul_fire = 0.0;
    double dmg_drain_hope = 0.0;
    double dmg_drain_life = 0.0;
    double dmg_drain_soul = 0.0;
    double dmg_searing_pain = 0.0;
    double dmg_pet = 0.0; // Total pet damage
    double dmg_pet_imp = 0.0;
    double dmg_pet_succubus = 0.0;
    double dmg_pet_melee = 0.0;
    double dmg_pet_lash_of_pain = 0.0;
    double dmg_pet_firebolt = 0.0;
    double dmg_demonic_brand = 0.0;

    std::vector<TimelineEntry> timeline;
    std::vector<SpellCastLog> cast_sequence;
};

class WarlockSimulator {
public:
    Race race = Race::HUMAN;
    BaseAttributes base_attrs;
    GearLoadout gear;
    Talents talents;
    BuffConfig buffs;
    PolicyConfig policy;
    MechanicsConfig mechanics;
    TargetConfig target_config;

    // Stat input mode: false = use equipped gear loadout; true = use direct numeric raw_stats
    bool use_raw_stats = true;
    Stats raw_stats;

    double fight_duration = 120.0;
    bool record_timeline = false;

    WarlockSimulator();

    // Runs a single deterministic or stochastic DES iteration
    SimResult run_single_simulation(FastRNG& rng);

    // Helper calculation methods
    double calculate_hit_chance(School school) const;
    double calculate_crit_chance(School school, const Stats& stats) const;
    double calculate_partial_resist_multiplier(School school, double target_resistance, FastRNG& rng) const;
};

} // namespace warlock
