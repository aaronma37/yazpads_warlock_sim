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
#include "src/sim/common/sim_state_vector.hpp"

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
    std::string tag;                  // Role, trigger or notes
    std::string event_type = "Cast";  // "Cast", "Hit", "Tick"
};

// Per-spell combat counters for the damage breakdown views.
// casts = completed casts / DoT applications / channel casts / pet casts & swings.
// hits = damaging impacts + ticks (ticks count as hits). damage sums final damage.
struct SpellCombatStats {
    int casts = 0;
    int hits = 0;
    int crits = 0;
    int misses = 0;
    double damage = 0.0;
};

struct SimResult {
    double duration = 0.0;
    double total_damage = 0.0;
    double dps = 0.0;

    std::array<SpellCombatStats, static_cast<size_t>(SpellID::COUNT)> spell_stats;

    void record_spell_cast(SpellID id) { spell_stats[static_cast<size_t>(id)].casts++; }
    void record_spell_hit(SpellID id, double dmg, bool crit) {
        SpellCombatStats& s = spell_stats[static_cast<size_t>(id)];
        s.hits++;
        s.damage += dmg;
        if (crit) s.crits++;
    }
    void record_spell_miss(SpellID id) { spell_stats[static_cast<size_t>(id)].misses++; }

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
    double dmg_bane_of_havoc = 0.0;
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
    double dmg_touch_of_the_grave = 0.0;
    int touch_of_the_grave_procs = 0;

    std::vector<TimelineEntry> timeline;
    std::vector<SpellCastLog> cast_sequence;
    std::vector<PriorityAction> action_history;

    std::vector<SpellCastLog> get_combat_events() const {
        std::vector<SpellCastLog> events;
        for (const auto& c : cast_sequence) {
            SpellCastLog entry = c;
            entry.event_type = "Cast";
            entry.tag = "Cast";
            events.push_back(entry);
        }
        for (const auto& entry : timeline) {
            if (entry.damage > 0.0 || entry.is_miss) {
                bool is_channel = (entry.spell_id == SpellID::DRAIN_LIFE || entry.spell_id == SpellID::DRAIN_SOUL || entry.spell_id == SpellID::DRAIN_HOPE);
                bool is_dot = (entry.spell_id == SpellID::CORRUPTION || entry.spell_id == SpellID::CURSE_OF_AGONY ||
                               entry.spell_id == SpellID::SIPHON_LIFE || entry.spell_id == SpellID::CURSE_OF_DOOM ||
                               entry.spell_id == SpellID::IMMOLATE);
                std::string type_label;
                if (is_channel) {
                    type_label = "Channel Tick";
                } else if (is_dot) {
                    type_label = "DoT Tick";
                } else {
                    type_label = "Hit";
                }
                events.push_back({entry.time, entry.spell_id, entry.damage, entry.is_crit, entry.is_miss, 0.0, type_label, type_label});
            }
        }
        std::stable_sort(events.begin(), events.end(), [](const SpellCastLog& a, const SpellCastLog& b) {
            if (std::abs(a.time - b.time) > 1e-5) {
                return a.time < b.time;
            }
            if (a.event_type == "Cast" && b.event_type != "Cast") return true;
            if (a.event_type != "Cast" && b.event_type == "Cast") return false;
            return false;
        });
        return events;
    }

    std::vector<SpellCastLog> get_damage_sequence() const {
        return get_combat_events();
    }
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

    double fight_duration = 180.0;
    bool randomize_duration = false;     // When true, fight length varies uniformly per simulation: [fight_duration - variance, fight_duration + variance]
    double duration_variance = 30.0;     // Fight length spread in seconds (+/- seconds)
    bool record_timeline = false;
    bool record_viper_samples = false;
    bool use_oracle_execution_policy = false; // Live online greedy MCTS / Oracle controller
    std::vector<PriorityAction> forced_action_prefix; // Prefix of actions to force during rollouts
    sim::VIPERDataset viper_dataset;

    WarlockSimulator();

    // Runs a single deterministic or stochastic DES iteration
    SimResult run_single_simulation(FastRNG& rng);

    // Helper calculation methods
    double calculate_hit_chance(School school) const;
    double calculate_crit_chance(School school, const Stats& stats) const;
    double calculate_partial_resist_multiplier(School school, double target_resistance, FastRNG& rng) const;
};

} // namespace warlock
