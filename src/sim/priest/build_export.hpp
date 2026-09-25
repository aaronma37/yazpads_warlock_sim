#pragma once
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <fstream>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include "priest_sim.hpp"
#include "parallel_runner.hpp"
#include "optimizer.hpp"
#include "spec_presets.hpp"
#include "src/sim/common/zip_writer.hpp"

namespace priest {
namespace build_export {

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

inline void append_tree_points(std::ostringstream& json,
                               const std::array<TalentNodeDef, 18>& nodes,
                               const DisciplineTalents& t) {
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
                               const std::array<TalentNodeDef, 17>& nodes,
                               const HolyTalents& t) {
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
                               const std::array<TalentNodeDef, 18>& nodes,
                               const ShadowTalents& t) {
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

inline std::string export_build_json(const PriestSimulator& sim,
                                    const BatchSimResult* last_result = nullptr,
                                    int iterations = 0,
                                    int thread_count = 0) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"format\": \"priest-build/1\",\n";
    json << "  \"class\": \"Priest\",\n";
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

    json << "  \"stats_mode\": \"" << (sim.use_raw_stats ? "raw" : "gear") << "\",\n";
    if (sim.use_raw_stats) {
        const sim::Stats& s = sim.raw_stats;
        json << "  \"raw_stats\": {\n";
        json << "    \"stamina\": " << json_double(s.stamina) << ", ";
        json << "\"intellect\": " << json_double(s.intellect) << ", ";
        json << "\"spirit\": " << json_double(s.spirit) << ",\n";
        json << "    \"spell_power\": " << json_double(s.spell_power) << ", ";
        json << "\"shadow_power\": " << json_double(s.shadow_power) << ", ";
        json << "\"holy_power\": " << json_double(s.holy_power) << ",\n";
        json << "    \"spell_hit_percent\": " << json_double(s.spell_hit_percent) << ", ";
        json << "\"spell_crit_percent\": " << json_double(s.spell_crit_percent) << ", ";
        json << "\"spell_haste_percent\": " << json_double(s.spell_haste_percent) << ", ";
        json << "\"mp5\": " << json_double(s.mp5) << "\n";
        json << "  },\n";
    } else {
        json << "  \"gear\": {\n";
        json << "    \"loadout_name\": \"" << json_escape(sim.gear.name) << "\",\n";
        json << "    \"slots\": {\n";
        for (size_t i = 0; i < static_cast<size_t>(sim::Slot::COUNT); ++i) {
            sim::Slot slot = static_cast<sim::Slot>(i);
            const sim::Item& item = sim.gear.get(slot);
            json << "      \"" << sim::slot_to_name(slot) << "\": \"" << json_escape(item.name) << "\"";
            json << (i + 1 < static_cast<size_t>(sim::Slot::COUNT) ? ",\n" : "\n");
        }
        json << "    }\n";
        json << "  },\n";
    }

    json << "  \"talents\": {\n";
    json << "    \"discipline\": { ";
    append_tree_points(json, get_disc_nodes(), sim.talents.disc);
    json << " },\n";
    json << "    \"holy\": { ";
    append_tree_points(json, get_holy_nodes(), sim.talents.holy);
    json << " },\n";
    json << "    \"shadow\": { ";
    append_tree_points(json, get_shadow_nodes(), sim.talents.shadow);
    json << " }\n";
    json << "  },\n";

    json << "  \"buffs\": {\n    ";
    {
        bool first = true;
        const sim::BuffConfig& b = sim.buffs;
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
    }
    json << "\n  },\n";

    {
        const PolicyConfig& p = sim.policy;
        json << "  \"policy\": {\n";
        json << "    \"rotation\": \"" << rotation_choice_to_string(p.rotation) << "\",\n";
        json << "    \"maintain_swp\": " << (p.maintain_swp ? "true" : "false") << ",\n";
        json << "    \"cast_mind_blast\": " << (p.cast_mind_blast ? "true" : "false") << ",\n";
        json << "    \"cast_sw_death\": " << (p.cast_sw_death ? "true" : "false") << ",\n";
        json << "    \"execute_sw_death_only\": " << (p.execute_sw_death_only ? "true" : "false") << ",\n";
        json << "    \"cast_devouring_plague\": " << (p.cast_devouring_plague ? "true" : "false") << ",\n";
        json << "    \"cast_vampiric_embrace\": " << (p.cast_vampiric_embrace ? "true" : "false") << ",\n";
        json << "    \"cast_holy_fire\": " << (p.cast_holy_fire ? "true" : "false") << ",\n";
        json << "    \"cast_penance\": " << (p.cast_penance ? "true" : "false") << ",\n";
        json << "    \"use_inner_focus\": " << (p.use_inner_focus ? "true" : "false") << ",\n";
        json << "    \"use_power_infusion\": " << (p.use_power_infusion ? "true" : "false") << ",\n";
        json << "    \"mana_potion_threshold\": " << json_double(p.mana_potion_threshold) << ",\n";
        json << "    \"demonic_rune_threshold\": " << json_double(p.demonic_rune_threshold) << "\n";
        json << "  },\n";
    }

    {
        const MechanicsConfig& m = sim.mechanics;
        json << "  \"mechanics\": {\n";
        json << "    \"shadow_weaving_personal\": " << (m.shadow_weaving_personal ? "true" : "false") << ", ";
        json << "\"snapshot_dots\": " << (m.snapshot_dots ? "true" : "false") << ", ";
        json << "\"spell_batching\": " << (m.spell_batching ? "true" : "false") << ",\n";
        json << "    \"batch_window_ms\": " << json_double(m.batch_window_ms) << ", ";
        json << "\"debuff_limit\": " << m.debuff_limit << ", ";
        json << "\"enforce_debuff_slots\": " << (m.enforce_debuff_slots ? "true" : "false") << ",\n";
        json << "    \"base_hit_vs_boss\": " << json_double(m.base_hit_vs_boss) << ", ";
        json << "\"max_spell_hit\": " << json_double(m.max_spell_hit) << ", ";
        json << "\"base_spell_crit_multiplier\": " << json_double(m.base_spell_crit_multiplier) << ",\n";
        json << "    \"base_gcd\": " << json_double(m.base_gcd) << ", ";
        json << "\"haste_affects_gcd\": " << (m.haste_affects_gcd ? "true" : "false") << ", ";
        json << "\"allow_mind_flay_clipping\": " << (m.allow_mind_flay_clipping ? "true" : "false") << "\n";
        json << "  },\n";
    }

    {
        const sim::TargetConfig& t = sim.target_config;
        json << "  \"target\": {\n";
        json << "    \"target_count\": " << t.target_count << ",\n";
        json << "    \"level\": " << t.level << ",\n";
        json << "    \"creature_type\": \"" << sim::creature_type_to_string(t.creature_type) << "\",\n";
        json << "    \"base_shadow_resistance\": " << json_double(t.base_shadow_resistance) << ",\n";
        json << "    \"base_fire_resistance\": " << json_double(t.base_fire_resistance) << ",\n";
        json << "    \"is_beast\": " << (t.is_beast ? "true" : "false") << "\n";
        json << "  }";
    }

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
        json << "      \"p5\": " << json_double(r.p5_dps) << ", ";
        json << "\"p50\": " << json_double(r.p50_dps) << ", ";
        json << "\"p95\": " << json_double(r.p95_dps) << "\n";
        json << "    },\n";
        json << "    \"crit_percent\": " << json_double(r.crit_percent) << ",\n";
        json << "    \"miss_percent\": " << json_double(r.miss_percent) << ",\n";
        json << "    \"mean_mana_spent\": " << json_double(r.mean_mana_spent) << ",\n";
        json << "    \"mean_mana_gained\": " << json_double(r.mean_mana_gained) << ",\n";
        json << "    \"damage_breakdown\": {\n";
        json << "      \"pct_sw_pain\": " << json_double(r.pct_sw_pain) << ",\n";
        json << "      \"pct_mind_flay\": " << json_double(r.pct_mind_flay) << ",\n";
        json << "      \"pct_mind_blast\": " << json_double(r.pct_mind_blast) << ",\n";
        json << "      \"pct_sw_death\": " << json_double(r.pct_sw_death) << ",\n";
        json << "      \"pct_devouring_plague\": " << json_double(r.pct_devouring_plague) << ",\n";
        json << "      \"pct_smite\": " << json_double(r.pct_smite) << ",\n";
        json << "      \"pct_holy_fire\": " << json_double(r.pct_holy_fire) << ",\n";
        json << "      \"pct_penance\": " << json_double(r.pct_penance) << ",\n";
        json << "      \"pct_holy_nova\": " << json_double(r.pct_holy_nova) << ",\n";
        json << "      \"pct_starshards\": " << json_double(r.pct_starshards) << ",\n";
        json << "      \"pct_chastise\": " << json_double(r.pct_chastise) << ",\n";
        json << "      \"pct_shadowguard\": " << json_double(r.pct_shadowguard) << ",\n";
        json << "      \"pct_touch_of_the_grave\": " << json_double(r.pct_touch_of_the_grave) << "\n";
        json << "    }\n";
        json << "  }\n";
    } else {
        json << "\n";
    }

    json << "}\n";
    return json.str();
}

inline std::string export_candidate_json(const CandidateResult& cand, const sim::TargetConfig& target = sim::TargetConfig{}) {
    PriestSimulator sim;
    sim.race = cand.race;
    sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, cand.race);
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
    csv << "Rank,Spec Name,Race,Mean DPS,Min DPS,Max DPS,StdDev\n";
    for (const auto& r : results) {
        csv << r.rank << ",\"" << json_escape(r.name) << "\"," << race_to_string(r.race) << ","
            << r.mean_dps << "," << r.min_dps << "," << r.max_dps << "," << r.std_dev_dps << "\n";
    }
    return csv.str();
}

inline std::string export_specs_batch_json(const std::vector<CandidateResult>& results,
                                          const PriestSimulator* base_sim = nullptr) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"format\": \"priest-specs-batch/1\",\n";
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
        sim::TargetConfig target = base_sim ? base_sim->target_config : sim::TargetConfig{};
        std::string full_cfg = export_candidate_json(r, target);
        json << "      \"full_configuration\": ";
        json << full_cfg;
        json << (i + 1 < results.size() ? "    },\n" : "    }\n");
    }
    json << "  ]\n";
    json << "}\n";
    return json.str();
}

inline sim::ZipArchive create_specs_batch_zip(const std::vector<CandidateResult>& results,
                                              const PriestSimulator* base_sim = nullptr) {
    sim::ZipArchive zip;

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        char rank_prefix[16];
        std::snprintf(rank_prefix, sizeof(rank_prefix), "%02zu_", i + 1);
        std::string filename = std::string(rank_prefix) + sanitize_filename(r.name) + ".json";

        sim::TargetConfig target = base_sim ? base_sim->target_config : sim::TargetConfig{};
        std::string spec_json = export_candidate_json(r, target);
        zip.add_file(filename, spec_json);
    }

    std::string batch_json = export_specs_batch_json(results, base_sim);
    zip.add_file("manifest.json", batch_json);

    std::string batch_csv = export_specs_batch_csv(results);
    zip.add_file("leaderboard.csv", batch_csv);

    return zip;
}

} // namespace build_export
} // namespace priest
