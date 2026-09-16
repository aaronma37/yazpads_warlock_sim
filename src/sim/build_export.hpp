#pragma once
#include <cstdio>
#include <string>
#include <sstream>
#include "warlock_sim.hpp"

namespace warlock {
namespace build_export {

// Serializes the full preset build (race, gear or raw stats, talents,
// buffs, pet, rotation policy, mechanics, target, fight duration) to JSON
// so it can be copied to the clipboard, shared, and pasted back for edits.
inline std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

inline std::string json_double(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.6g", v);
    return buf;
}

inline const char* dot_policy_to_string_local(DotPolicy d) {
    switch (d) {
        case DotPolicy::ALWAYS: return "always";
        case DotPolicy::ONLY_WITH_DEBUFF_SLOT: return "only_with_debuff_slot";
        default: return "never";
    }
}

inline const char* shadowburn_policy_to_string_local(ShadowburnPolicy s) {
    switch (s) {
        case ShadowburnPolicy::ON_COOLDOWN: return "on_cooldown";
        case ShadowburnPolicy::EXECUTE_ONLY: return "execute_only";
        default: return "never";
    }
}

// Only nonzero talents are exported; a missing key means 0 points.
inline void append_tree_points(std::ostringstream& json,
                               const std::array<TalentNodeDef, 17>& nodes,
                               const AfflictionTalents& t) {
    bool first = true;
    for (size_t i = 0; i < nodes.size(); ++i) {
        int pts = t.get_points_by_index(i);
        if (pts == 0) continue;
        if (!first) json << ", ";
        first = false;
        json << "\"" << nodes[i].id << "\": " << pts;
    }
}

inline void append_tree_points(std::ostringstream& json,
                               const std::array<TalentNodeDef, 19>& nodes,
                               const DemonologyTalents& t) {
    bool first = true;
    for (size_t i = 0; i < nodes.size(); ++i) {
        int pts = t.get_points_by_index(i);
        if (pts == 0) continue;
        if (!first) json << ", ";
        first = false;
        json << "\"" << nodes[i].id << "\": " << pts;
    }
}

inline void append_tree_points(std::ostringstream& json,
                               const std::array<TalentNodeDef, 16>& nodes,
                               const DestructionTalents& t) {
    bool first = true;
    for (size_t i = 0; i < nodes.size(); ++i) {
        int pts = t.get_points_by_index(i);
        if (pts == 0) continue;
        if (!first) json << ", ";
        first = false;
        json << "\"" << nodes[i].id << "\": " << pts;
    }
}

inline void append_bool(std::ostringstream& json, const char* key, bool value, bool& first) {
    if (!first) json << ",\n    ";
    first = false;
    json << "\"" << key << "\": " << (value ? "true" : "false");
}

inline std::string export_build_json(const WarlockSimulator& sim) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"format\": \"warlock-build/1\",\n";
    json << "  \"race\": \"" << race_to_string(sim.race) << "\",\n";
    json << "  \"fight_duration\": " << json_double(sim.fight_duration) << ",\n";

    // Stats source: equipped gear or direct raw stats.
    json << "  \"stats_mode\": \"" << (sim.use_raw_stats ? "raw" : "gear") << "\",\n";
    if (sim.use_raw_stats) {
        const Stats& s = sim.raw_stats;
        json << "  \"raw_stats\": {\n";
        json << "    \"stamina\": " << json_double(s.stamina) << ", ";
        json << "\"intellect\": " << json_double(s.intellect) << ", ";
        json << "\"spirit\": " << json_double(s.spirit) << ",\n";
        json << "    \"spell_power\": " << json_double(s.spell_power) << ", ";
        json << "\"shadow_power\": " << json_double(s.shadow_power) << ", ";
        json << "\"fire_power\": " << json_double(s.fire_power) << ",\n";
        json << "    \"spell_hit_percent\": " << json_double(s.spell_hit_percent) << ", ";
        json << "\"spell_crit_percent\": " << json_double(s.spell_crit_percent) << ", ";
        json << "\"spell_haste_percent\": " << json_double(s.spell_haste_percent) << ", ";
        json << "\"mp5\": " << json_double(s.mp5) << "\n";
        json << "  },\n";
    }
    json << "  \"gear\": {\n";
    json << "    \"loadout_name\": \"" << json_escape(sim.gear.name) << "\",\n";
    json << "    \"slots\": {\n";
    for (size_t i = 0; i < static_cast<size_t>(Slot::COUNT); ++i) {
        Slot slot = static_cast<Slot>(i);
        const Item& item = sim.gear.get(slot);
        json << "      \"" << slot_to_name(slot) << "\": \"" << json_escape(item.name) << "\"";
        json << (i + 1 < static_cast<size_t>(Slot::COUNT) ? ",\n" : "\n");
    }
    json << "    }\n";
    json << "  },\n";

    // Talents (nonzero only; missing key = 0 points).
    json << "  \"talents\": {\n";
    json << "    \"affliction\": { ";
    append_tree_points(json, FOREVER_AFFLICTION_NODES, sim.talents.aff);
    json << " },\n";
    json << "    \"demonology\": { ";
    append_tree_points(json, FOREVER_DEMONOLOGY_NODES, sim.talents.demo);
    json << " },\n";
    json << "    \"destruction\": { ";
    append_tree_points(json, FOREVER_DESTRUCTION_NODES, sim.talents.destro);
    json << " }\n";
    json << "  },\n";

    // Buffs / consumables / debuffs / sacrifice.
    json << "  \"buffs\": {\n    ";
    {
        bool first = true;
        const BuffConfig& b = sim.buffs;
        append_bool(json, "arcane_intellect", b.arcane_intellect, first);
        append_bool(json, "blessing_of_kings", b.blessing_of_kings, first);
        append_bool(json, "blessing_of_wisdom", b.blessing_of_wisdom, first);
        append_bool(json, "mark_of_the_wild", b.mark_of_the_wild, first);
        append_bool(json, "judgement_of_wisdom", b.judgement_of_wisdom, first);
        append_bool(json, "flask_of_supreme_power", b.flask_of_supreme_power, first);
        append_bool(json, "greater_arcane_elixir", b.greater_arcane_elixir, first);
        append_bool(json, "elixir_of_shadow_power", b.elixir_of_shadow_power, first);
        append_bool(json, "elixir_of_greater_firepower", b.elixir_of_greater_firepower, first);
        append_bool(json, "brilliant_wizard_oil", b.brilliant_wizard_oil, first);
        append_bool(json, "use_mana_potions", b.use_mana_potions, first);
        append_bool(json, "use_demonic_runes", b.use_demonic_runes, first);
        append_bool(json, "rallying_cry", b.rallying_cry, first);
        append_bool(json, "songflower", b.songflower, first);
        append_bool(json, "spirit_of_zandalar", b.spirit_of_zandalar, first);
        append_bool(json, "warchiefs_blessing", b.warchiefs_blessing, first);
        append_bool(json, "sayges_fortune", b.sayges_fortune, first);
        append_bool(json, "curse_of_shadows", b.curse_of_shadows, first);
        append_bool(json, "curse_of_elements", b.curse_of_elements, first);
        append_bool(json, "shadow_weaving", b.shadow_weaving, first);
        append_bool(json, "nightfall_axe", b.nightfall_axe, first);
        append_bool(json, "sacrifice_imp", b.sacrifice_imp, first);
        append_bool(json, "sacrifice_succubus", b.sacrifice_succubus, first);
    }
    json << "\n  },\n";

    // Rotation policy + active pet.
    {
        const PolicyConfig& p = sim.policy;
        json << "  \"policy\": {\n";
        json << "    \"rotation\": \"" << rotation_choice_to_string(p.rotation) << "\",\n";
        json << "    \"curse\": \"" << curse_choice_to_string(p.curse) << "\",\n";
        json << "    \"corruption\": \"" << dot_policy_to_string_local(p.corruption) << "\",\n";
        json << "    \"maintain_immolate\": " << (p.maintain_immolate ? "true" : "false") << ",\n";
        json << "    \"shadowburn\": \"" << shadowburn_policy_to_string_local(p.shadowburn) << "\",\n";
        json << "    \"pet\": \"" << pet_choice_to_string(p.pet) << "\",\n";
        json << "    \"life_tap_threshold_pct\": " << json_double(p.life_tap_threshold_pct) << ",\n";
        json << "    \"use_trinkets_on_cooldown\": " << (p.use_trinkets_on_cooldown ? "true" : "false") << ",\n";
        json << "    \"cast_nightfall_procs\": " << (p.cast_nightfall_procs ? "true" : "false") << ",\n";
        json << "    \"use_conflagrate\": " << (p.use_conflagrate ? "true" : "false") << ",\n";
        json << "    \"use_incinerate\": " << (p.use_incinerate ? "true" : "false") << ",\n";
        json << "    \"use_decimation_soul_fire\": " << (p.use_decimation_soul_fire ? "true" : "false") << ",\n";
        json << "    \"channel_drain_hope\": " << (p.channel_drain_hope ? "true" : "false") << "\n";
        json << "  },\n";
    }

    // Mechanics toggles.
    {
        const MechanicsConfig& m = sim.mechanics;
        json << "  \"mechanics\": {\n";
        json << "    \"snapshot_dots\": " << (m.snapshot_dots ? "true" : "false") << ", ";
        json << "\"spell_batching\": " << (m.spell_batching ? "true" : "false") << ", ";
        json << "\"batch_window_ms\": " << json_double(m.batch_window_ms) << ",\n";
        json << "    \"debuff_limit\": " << m.debuff_limit << ", ";
        json << "\"enforce_debuff_slots\": " << (m.enforce_debuff_slots ? "true" : "false") << ", ";
        json << "\"personal_shadow_weaving\": " << (m.personal_shadow_weaving ? "true" : "false") << ",\n";
        json << "    \"partial_resists_enabled\": " << (m.partial_resists_enabled ? "true" : "false") << ", ";
        json << "\"isb_has_charges\": " << (m.isb_has_charges ? "true" : "false") << ", ";
        json << "\"isb_all_shadow_sources\": " << (m.isb_all_shadow_sources ? "true" : "false") << ",\n";
        json << "    \"base_hit_vs_boss\": " << json_double(m.base_hit_vs_boss) << ", ";
        json << "\"max_spell_hit\": " << json_double(m.max_spell_hit) << ", ";
        json << "\"base_spell_crit_multiplier\": " << json_double(m.base_spell_crit_multiplier) << ",\n";
        json << "    \"nightfall_enabled\": " << (m.nightfall_enabled ? "true" : "false") << ", ";
        json << "\"nightfall_proc_chance\": " << json_double(m.nightfall_proc_chance) << ",\n";
        json << "    \"projectile_travel_time\": " << (m.projectile_travel_time ? "true" : "false") << ", ";
        json << "\"default_boss_distance_yards\": " << json_double(m.default_boss_distance_yards) << ", ";
        json << "\"projectile_speed_yards_per_sec\": " << json_double(m.projectile_speed_yards_per_sec) << ",\n";
        json << "    \"base_gcd\": " << json_double(m.base_gcd) << ", ";
        json << "\"haste_affects_gcd\": " << (m.haste_affects_gcd ? "true" : "false") << ",\n";
        json << "    \"pet_scaling\": " << (m.pet_scaling ? "true" : "false") << ", ";
        json << "\"pet_sp_ratio\": " << json_double(m.pet_sp_ratio) << ", ";
        json << "\"pet_ap_ratio\": " << json_double(m.pet_ap_ratio) << ",\n";
        json << "    \"pet_mana_management\": " << (m.pet_mana_management ? "true" : "false") << ", ";
        json << "\"imp_base_mana\": " << json_double(m.imp_base_mana) << ", ";
        json << "\"succubus_base_mana\": " << json_double(m.succubus_base_mana) << ",\n";
        json << "    \"imp_firebolt_cost\": " << json_double(m.imp_firebolt_cost) << ", ";
        json << "\"succubus_lop_cost\": " << json_double(m.succubus_lop_cost) << ", ";
        json << "\"pet_base_mp5\": " << json_double(m.pet_base_mp5) << "\n";
        json << "  },\n";
    }

    // Target encounter.
    {
        const TargetConfig& t = sim.target_config;
        json << "  \"target\": {\n";
        json << "    \"level\": " << t.level << ",\n";
        json << "    \"creature_type\": \"" << creature_type_to_string(t.creature_type) << "\",\n";
        json << "    \"base_shadow_resistance\": " << json_double(t.base_shadow_resistance) << ",\n";
        json << "    \"base_fire_resistance\": " << json_double(t.base_fire_resistance) << ",\n";
        json << "    \"is_beast\": " << (t.is_beast ? "true" : "false") << "\n";
        json << "  }\n";
    }

    json << "}\n";
    return json.str();
}

} // namespace build_export
} // namespace warlock
