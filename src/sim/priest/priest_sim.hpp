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
    std::string tag;                  // Role, trigger or notes
    std::string event_type = "Cast";  // "Cast", "Hit", "Tick"
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
                bool is_channel = (entry.spell_id == SpellID::MIND_FLAY || entry.spell_id == SpellID::PENANCE || entry.spell_id == SpellID::STARSHARDS);
                bool is_dot = (entry.spell_id == SpellID::SHADOW_WORD_PAIN || entry.spell_id == SpellID::DEVOURING_PLAGUE ||
                               entry.spell_id == SpellID::HOLY_FIRE);
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
