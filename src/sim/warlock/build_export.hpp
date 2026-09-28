#pragma once
#include <cstdio>
#include <charconv>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <fstream>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include "warlock_sim.hpp"
#include "imitation_training.hpp"
#include "parallel_runner.hpp"
#include "optimizer.hpp"
#include "src/sim/common/zip_writer.hpp"

namespace warlock {
namespace build_export {

// Serializes the full preset build (race, gear or raw stats, talents,
// buffs, pet, rotation policy, mechanics, target, fight duration, and optional simulation results)
// to JSON so it can be copied to the clipboard, saved to file, or shared.
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
    // Shortest readable representation that round-trips to the same double.
    const auto result = std::to_chars(buf, buf + sizeof(buf), v);
    return std::string(buf, result.ptr);
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

inline void append_apl(std::ostringstream& json, const PolicyConfig& policy,
                       const Talents& talents, Race race) {
    const auto rules = policy.get_priority_rules(talents, race);
    json << "    \"use_custom_apl\": " << (policy.use_custom_apl ? "true" : "false") << ",\n";
    json << "    \"apl\": [\n";
    for (size_t i = 0; i < rules.size(); ++i) {
        const auto& rule = rules[i];
        json << "      {\n";
        json << "        \"action\": " << static_cast<unsigned int>(rule.action) << ",\n";
        json << "        \"spell_id\": " << static_cast<unsigned int>(rule.spell_id) << ",\n";
        json << "        \"name\": \"" << json_escape(rule.name) << "\",\n";
        json << "        \"condition\": \"" << json_escape(rule.condition_summary) << "\",\n";
        json << "        \"trigger\": \"" << json_escape(rule.trigger_condition) << "\",\n";
        json << "        \"enabled\": " << (rule.enabled ? "true" : "false") << ",\n";
        json << "        \"custom_thresholds\": " << (rule.use_custom_thresholds ? "true" : "false") << "\n";
        json << "      }" << (i + 1 < rules.size() ? "," : "") << "\n";
    }
    json << "    ]";
}

inline bool save_export_to_file(const std::string& filepath, const std::string& content) {
#if defined(__EMSCRIPTEN__)
    EM_ASM({
        var filename = UTF8ToString($0);
        var content = UTF8ToString($1);
        var blob = new Blob([content], {type: 'application/json'});
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }, filepath.c_str(), content.c_str());
    return true;
#else
    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << content;
    return true;
#endif
}

inline std::string export_build_json(const WarlockSimulator& sim,
                                    const BatchSimResult* last_result = nullptr,
                                    int iterations = 0,
                                    int thread_count = 0) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"format\": \"warlock-build/1\",\n";
    json << "  \"class\": \"Warlock\",\n";
    json << "  \"race\": \"" << race_to_string(sim.race) << "\",\n";
    json << "  \"fight_duration\": " << json_double(sim.fight_duration) << ",\n";
    json << "  \"duration_variance\": " << json_double(sim.duration_variance) << ",\n";
    json << "  \"randomize_duration\": " << (sim.randomize_duration ? "true" : "false") << ",\n";

    if (iterations > 0 || thread_count > 0) {
        json << "  \"sim_config\": {\n";
        json << "    \"iterations\": " << (iterations > 0 ? iterations : (last_result ? last_result->total_iterations : 10000)) << ",\n";
        json << "    \"worker_threads\": " << thread_count << ",\n";
        json << "    \"fight_duration\": " << json_double(sim.fight_duration) << ",\n";
        json << "    \"duration_variance\": " << json_double(sim.duration_variance) << ",\n";
        json << "    \"randomize_duration\": " << (sim.randomize_duration ? "true" : "false") << "\n";
        json << "  },\n";
    }

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
        json << "\"mp5\": " << json_double(s.mp5) << ", ";
        json << "\"spell_penetration\": " << json_double(s.spell_penetration) << "\n";
        json << "  },\n";
    } else {
        json << "  \"gear\": {\n";
        json << "    \"loadout_name\": \"" << json_escape(sim.gear.name) << "\",\n";
        json << "    \"extra_spell_power\": " << json_double(sim.gear.extra_spell_power) << ",\n";
        json << "    \"extra_shadow_power\": " << json_double(sim.gear.extra_shadow_power) << ",\n";
        json << "    \"extra_spell_hit\": " << json_double(sim.gear.extra_spell_hit) << ",\n";
        json << "    \"extra_spell_crit\": " << json_double(sim.gear.extra_spell_crit) << ",\n";
        json << "    \"extra_spell_penetration\": " << json_double(sim.gear.extra_spell_penetration) << ",\n";
        json << "    \"slots\": {\n";
        for (size_t i = 0; i < static_cast<size_t>(Slot::COUNT); ++i) {
            Slot slot = static_cast<Slot>(i);
            const Item& item = sim.gear.get(slot);
            json << "      \"" << slot_to_name(slot) << "\": \"" << json_escape(item.name) << "\"";
            json << (i + 1 < static_cast<size_t>(Slot::COUNT) ? ",\n" : "\n");
        }
        json << "    }\n";
        json << "  },\n";
    }

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
        append_bool(json, "flask_of_distilled_wisdom", b.flask_of_distilled_wisdom, first);
        append_bool(json, "flask_of_the_titans", b.flask_of_the_titans, first);
        append_bool(json, "greater_arcane_elixir", b.greater_arcane_elixir, first);
        append_bool(json, "elixir_of_shadow_power", b.elixir_of_shadow_power, first);
        append_bool(json, "elixir_of_greater_firepower", b.elixir_of_greater_firepower, first);
        append_bool(json, "elixir_of_the_owl", b.elixir_of_the_owl, first);
        append_bool(json, "elixir_of_the_sages", b.elixir_of_the_sages, first);
        append_bool(json, "mageblood_elixir", b.mageblood_elixir, first);
        append_bool(json, "greater_mageblood_elixir", b.greater_mageblood_elixir, first);
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
        json << "    \"trained_gbdt\": {\n";
        json << "      \"enabled\": " << (p.use_imitation_policy ? "true" : "false") << ",\n";
        json << "      \"name\": \"" << json_escape(p.imitation_policy_name) << "\",\n";
        json << "      \"model\": ";
        if (p.imitation_policy) json << "\"" << json_escape(p.imitation_policy->serialize()) << "\"\n";
        else json << "null\n";
        json << "    },\n";
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
        json << "    \"channel_drain_hope\": " << (p.channel_drain_hope ? "true" : "false") << ",\n";
        json << "    \"multi_dot_corruption\": " << (p.multi_dot_corruption ? "true" : "false") << ",\n";
        json << "    \"auto_bane_of_havoc\": " << (p.auto_bane_of_havoc ? "true" : "false") << ",\n";
        json << "    \"racial_policy\": \"" << racial_policy_to_string(p.racial_policy) << "\",\n";
        append_apl(json, p, sim.talents, sim.race);
        json << "  },\n";
    }

    // Mechanics toggles.
    {
        const MechanicsConfig& m = sim.mechanics;
        json << "  \"mechanics\": {\n";
        json << "    \"use_book_spell_ranks\": " << (m.use_book_spell_ranks ? "true" : "false") << ",\n";
        json << "    \"allow_rank2_shadow_bolt\": " << (m.allow_rank2_shadow_bolt ? "true" : "false") << ",\n";
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
        json << "\"haste_affects_gcd\": " << (m.haste_affects_gcd ? "true" : "false") << ", ";
        json << "\"instant_drain_hope\": " << (m.instant_drain_hope ? "true" : "false") << ",\n";
        json << "    \"corruption_sp_coefficient\": " << json_double(m.corruption_sp_coefficient) << ",\n";
        json << "    \"pet_scaling\": " << (m.pet_scaling ? "true" : "false") << ", ";
        json << "\"pet_sp_ratio\": " << json_double(m.pet_sp_ratio) << ", ";
        json << "\"pet_ap_ratio\": " << json_double(m.pet_ap_ratio) << ",\n";
        json << "    \"pet_mana_management\": " << (m.pet_mana_management ? "true" : "false") << ", ";
        json << "\"imp_base_mana\": " << json_double(m.imp_base_mana) << ", ";
        json << "\"succubus_base_mana\": " << json_double(m.succubus_base_mana) << ",\n";
        json << "    \"imp_firebolt_cost\": " << json_double(m.imp_firebolt_cost) << ", ";
        json << "\"succubus_lop_cost\": " << json_double(m.succubus_lop_cost) << ", ";
        json << "\"pet_base_mp5\": " << json_double(m.pet_base_mp5) << ",\n";
        json << "    \"imp_firebolt_modern_scaling\": " << (m.imp_firebolt_modern_scaling ? "true" : "false") << "\n";
        json << "  },\n";
    }

    // Target encounter.
    {
        const TargetConfig& t = sim.target_config;
        json << "  \"target\": {\n";
        json << "    \"target_count\": " << t.target_count << ",\n";
        json << "    \"level\": " << t.level << ",\n";
        json << "    \"creature_type\": \"" << creature_type_to_string(t.creature_type) << "\",\n";
        json << "    \"boss_armor\": " << json_double(t.boss_armor) << ",\n";
        json << "    \"sunder_armor\": " << (t.sunder_armor ? "true" : "false") << ",\n";
        json << "    \"faerie_fire\": " << (t.faerie_fire ? "true" : "false") << ",\n";
        json << "    \"base_shadow_resistance\": " << json_double(t.base_shadow_resistance) << ",\n";
        json << "    \"base_fire_resistance\": " << json_double(t.base_fire_resistance) << ",\n";
        json << "    \"is_beast\": " << (t.is_beast ? "true" : "false") << "\n";
        json << "  }";
    }

    // Simulation Results if available
    if (last_result && last_result->total_iterations > 0) {
        const BatchSimResult& r = *last_result;
        json << ",\n  \"simulation_results\": {\n";
        json << "    \"iterations\": " << r.total_iterations << ",\n";
        json << "    \"total_sim_time_seconds\": " << json_double(r.total_sim_time_seconds) << ",\n";
        json << "    \"iterations_per_second\": " << json_double(r.iterations_per_second) << ",\n";
        json << "    \"mean_dps\": " << json_double(r.mean_dps) << ",\n";
        json << "    \"min_dps\": " << json_double(r.min_dps) << ",\n";
        json << "    \"max_dps\": " << json_double(r.max_dps) << ",\n";
        json << "    \"std_dev_dps\": " << json_double(r.std_dev_dps) << ",\n";
        json << "    \"median_dps\": " << json_double(r.p50_dps) << ",\n";
        json << "    \"percentiles\": {\n";
        json << "      \"p1\": " << json_double(r.p1_dps) << ", ";
        json << "\"p5\": " << json_double(r.p5_dps) << ", ";
        json << "\"p25\": " << json_double(r.p25_dps) << ", ";
        json << "\"p50\": " << json_double(r.p50_dps) << ", ";
        json << "\"p75\": " << json_double(r.p75_dps) << ", ";
        json << "\"p95\": " << json_double(r.p95_dps) << ", ";
        json << "\"p99\": " << json_double(r.p99_dps) << "\n";
        json << "    },\n";
        json << "    \"crit_percent\": " << json_double(r.crit_percent) << ",\n";
        json << "    \"miss_percent\": " << json_double(r.miss_percent) << ",\n";
        json << "    \"mean_isb_uptime\": " << json_double(r.mean_isb_uptime) << ",\n";
        json << "    \"mean_shadow_bolts\": " << json_double(r.mean_shadow_bolts) << ",\n";
        json << "    \"mean_crits\": " << json_double(r.mean_crits) << ",\n";
        json << "    \"mean_life_taps\": " << json_double(r.mean_life_taps) << ",\n";
        json << "    \"mean_mana_spent\": " << json_double(r.mean_mana_spent) << ",\n";
        json << "    \"mean_pet_dps\": " << json_double(r.mean_pet_dps) << ",\n";
        json << "    \"damage_breakdown\": {\n";
        json << "      \"pct_shadow_bolt\": " << json_double(r.pct_shadow_bolt) << ",\n";
        json << "      \"pct_corruption\": " << json_double(r.pct_corruption) << ",\n";
        json << "      \"pct_curse\": " << json_double(r.pct_curse) << ",\n";
        json << "      \"pct_agony\": " << json_double(r.pct_agony) << ",\n";
        json << "      \"pct_doom\": " << json_double(r.pct_doom) << ",\n";
        json << "      \"pct_bane_of_havoc\": " << json_double(r.pct_bane_of_havoc) << ",\n";
        json << "      \"pct_siphon_life\": " << json_double(r.pct_siphon_life) << ",\n";
        json << "      \"pct_immolate\": " << json_double(r.pct_immolate) << ",\n";
        json << "      \"pct_shadowburn\": " << json_double(r.pct_shadowburn) << ",\n";
        json << "      \"pct_conflagrate\": " << json_double(r.pct_conflagrate) << ",\n";
        json << "      \"pct_incinerate\": " << json_double(r.pct_incinerate) << ",\n";
        json << "      \"pct_searing_pain\": " << json_double(r.pct_searing_pain) << ",\n";
        json << "      \"pct_soul_fire\": " << json_double(r.pct_soul_fire) << ",\n";
        json << "      \"pct_drain_hope\": " << json_double(r.pct_drain_hope) << ",\n";
        json << "      \"pct_drain_life\": " << json_double(r.pct_drain_life) << ",\n";
        json << "      \"pct_drain_soul\": " << json_double(r.pct_drain_soul) << ",\n";
        json << "      \"pct_pet\": " << json_double(r.pct_pet) << ",\n";
        json << "      \"pct_pet_imp\": " << json_double(r.pct_pet_imp) << ",\n";
        json << "      \"pct_pet_succubus\": " << json_double(r.pct_pet_succubus) << ",\n";
        json << "      \"pct_pet_melee\": " << json_double(r.pct_pet_melee) << ",\n";
        json << "      \"pct_pet_lash_of_pain\": " << json_double(r.pct_pet_lash_of_pain) << ",\n";
        json << "      \"pct_pet_firebolt\": " << json_double(r.pct_pet_firebolt) << ",\n";
        json << "      \"pct_demonic_brand\": " << json_double(r.pct_demonic_brand) << ",\n";
        json << "      \"pct_touch_of_the_grave\": " << json_double(r.pct_touch_of_the_grave) << "\n";
        json << "    }\n";
        json << "  }\n";
    } else {
        json << "\n";
    }

    json << "}\n";
    return json.str();
}

inline std::string export_candidate_json(const CandidateResult& cand, const TargetConfig& target = TargetConfig{}) {
    WarlockSimulator sim;
    sim.race = cand.race;
    sim.base_attrs = get_base_attributes_for_race(cand.race);
    sim.talents = cand.talents;
    sim.gear = cand.gear;
    sim.buffs = cand.buffs;
    sim.policy = cand.policy;
    sim.mechanics = cand.mechanics;
    sim.use_raw_stats = cand.use_raw_stats;
    sim.raw_stats = cand.raw_stats;
    sim.target_config = target;

    const BatchSimResult* batch_ptr = (cand.batch.total_iterations > 0) ? &cand.batch : nullptr;
    return export_build_json(sim, batch_ptr);
}

inline std::string sanitize_filename(const std::string& name) {
    std::string out;
    for (char c : name) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') {
            out += static_cast<char>(std::tolower(c));
        } else if (c == ' ' || c == '/' || c == '(' || c == ')' || c == '+') {
            if (!out.empty() && out.back() != '_') {
                out += '_';
            }
        }
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out.empty() ? "spec" : out;
}

inline std::string export_specs_batch_csv(const std::vector<CandidateResult>& results) {
    std::ostringstream csv;
    csv << "Rank,Spec Name,Race,Mean DPS,Min DPS,Max DPS,StdDev,ISB Uptime,DPS/SP,DPS/Hit,DPS/Crit,DPS/Haste,DPS/Int,DPS/Spirit\n";
    for (const auto& r : results) {
        csv << r.rank << ",\"" << json_escape(r.name) << "\"," << race_to_string(r.race) << ","
            << r.mean_dps << "," << r.min_dps << "," << r.max_dps << "," << r.std_dev_dps << ","
            << (r.isb_uptime * 100.0) << "%";
        if (r.stat_weights.valid) {
            csv << "," << r.stat_weights.dps_per_sp
                << "," << r.stat_weights.dps_per_hit
                << "," << r.stat_weights.dps_per_crit
                << "," << r.stat_weights.dps_per_haste
                << "," << r.stat_weights.dps_per_int
                << "," << r.stat_weights.dps_per_spirit;
        } else {
            csv << ",,,,,,";
        }
        csv << "\n";
    }
    return csv.str();
}

inline std::string export_specs_batch_json(const std::vector<CandidateResult>& results,
                                          const WarlockSimulator* base_sim = nullptr) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"format\": \"warlock-specs-batch/1\",\n";
    json << "  \"total_specs\": " << results.size() << ",\n";
    json << "  \"specs\": [\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        json << "    {\n";
        json << "      \"rank\": " << r.rank << ",\n";
        json << "      \"name\": \"" << json_escape(r.name) << "\",\n";
        json << "      \"race\": \"" << race_to_string(r.race) << "\",\n";
        json << "      \"mean_dps\": " << json_double(r.mean_dps) << ",\n";
        json << "      \"min_dps\": " << json_double(r.min_dps) << ",\n";
        json << "      \"max_dps\": " << json_double(r.max_dps) << ",\n";
        json << "      \"std_dev_dps\": " << json_double(r.std_dev_dps) << ",\n";
        json << "      \"isb_uptime\": " << json_double(r.isb_uptime) << ",\n";
        if (r.stat_weights.valid) {
            json << "      \"stat_weights\": {\n";
            json << "        \"dps_per_sp\": " << json_double(r.stat_weights.dps_per_sp) << ",\n";
            json << "        \"dps_per_hit\": " << json_double(r.stat_weights.dps_per_hit) << ",\n";
            json << "        \"dps_per_crit\": " << json_double(r.stat_weights.dps_per_crit) << ",\n";
            json << "        \"dps_per_haste\": " << json_double(r.stat_weights.dps_per_haste) << ",\n";
            json << "        \"dps_per_int\": " << json_double(r.stat_weights.dps_per_int) << ",\n";
            json << "        \"dps_per_spirit\": " << json_double(r.stat_weights.dps_per_spirit) << "\n";
            json << "      },\n";
        }
        TargetConfig target = base_sim ? base_sim->target_config : TargetConfig{};
        std::string full_cfg = export_candidate_json(r, target);
        // Indent full configuration
        std::istringstream cfg_stream(full_cfg);
        std::string line;
        json << "      \"full_configuration\": ";
        json << full_cfg;
        json << (i + 1 < results.size() ? "    },\n" : "    }\n");
    }
    json << "  ]\n";
    json << "}\n";
    return json.str();
}

inline sim::ZipArchive create_specs_batch_zip(const std::vector<CandidateResult>& results,
                                              const WarlockSimulator* base_sim = nullptr) {
    sim::ZipArchive zip;

    // 1. Add individual JSON files for each spec shown
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        char rank_prefix[16];
        std::snprintf(rank_prefix, sizeof(rank_prefix), "%02zu_", i + 1);
        std::string filename = std::string(rank_prefix) + sanitize_filename(r.name) + ".json";

        TargetConfig target = base_sim ? base_sim->target_config : TargetConfig{};
        std::string spec_json = export_candidate_json(r, target);
        zip.add_file(filename, spec_json);
    }

    // 2. Add summary manifest JSON
    std::string batch_json = export_specs_batch_json(results, base_sim);
    zip.add_file("manifest.json", batch_json);

    // 3. Add leaderboard CSV
    std::string batch_csv = export_specs_batch_csv(results);
    zip.add_file("leaderboard.csv", batch_csv);

    return zip;
}

} // namespace build_export
} // namespace warlock
