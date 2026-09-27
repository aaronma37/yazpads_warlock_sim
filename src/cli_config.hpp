#pragma once

// Small dependency-free JSON reader for the headless comparison CLI.  It is
// intentionally limited to configuration values (objects, arrays, strings,
// numbers, booleans and null); it is not intended to replace a general JSON
// library in the simulator.

#include <cctype>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/sim/warlock_sim.hpp"
#include "src/sim/spec_presets.hpp"

namespace warlock::cli_config {

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Object, Array } type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::map<std::string, JsonValue> object;
    std::vector<JsonValue> array;

    const JsonValue* get(const std::string& key) const {
        if (type != Type::Object) return nullptr;
        auto it = object.find(key);
        return it == object.end() ? nullptr : &it->second;
    }
};

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    JsonValue parse() {
        skip_space();
        JsonValue value = parse_value();
        skip_space();
        if (pos_ != text_.size()) fail("trailing characters");
        return value;
    }

private:
    const std::string& text_;
    size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error("JSON error at byte " + std::to_string(pos_) + ": " + message);
    }

    void skip_space() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }

    bool consume(char c) {
        skip_space();
        if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; }
        return false;
    }

    void expect(char c) {
        if (!consume(c)) fail(std::string("expected '") + c + "'");
    }

    JsonValue parse_value() {
        skip_space();
        if (pos_ >= text_.size()) fail("unexpected end of input");
        switch (text_[pos_]) {
            case '{': return parse_object();
            case '[': return parse_array();
            case '"': { JsonValue v; v.type = JsonValue::Type::String; v.string = parse_string(); return v; }
            case 't': return parse_literal("true", JsonValue::Type::Bool, true);
            case 'f': return parse_literal("false", JsonValue::Type::Bool, false);
            case 'n': return parse_literal("null", JsonValue::Type::Null, false);
            default:
                if (text_[pos_] == '-' || std::isdigit(static_cast<unsigned char>(text_[pos_]))) return parse_number();
                fail("unexpected value");
        }
    }

    JsonValue parse_literal(const char* literal, JsonValue::Type type, bool boolean) {
        size_t length = std::strlen(literal);
        if (text_.compare(pos_, length, literal) != 0) fail("invalid literal");
        pos_ += length;
        JsonValue v; v.type = type; v.boolean = boolean; return v;
    }

    JsonValue parse_number() {
        size_t start = pos_;
        if (text_[pos_] == '-') ++pos_;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        if (pos_ < text_.size() && text_[pos_] == '.') { ++pos_; while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_; }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_; if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        }
        JsonValue v; v.type = JsonValue::Type::Number;
        try { v.number = std::stod(text_.substr(start, pos_ - start)); }
        catch (...) { fail("invalid number"); }
        return v;
    }

    std::string parse_string() {
        expect('"');
        std::string out;
        while (pos_ < text_.size()) {
            char c = text_[pos_++];
            if (c == '"') return out;
            if (c != '\\') { out += c; continue; }
            if (pos_ >= text_.size()) fail("unfinished escape");
            char e = text_[pos_++];
            switch (e) {
                case '"': out += '"'; break; case '\\': out += '\\'; break;
                case '/': out += '/'; break; case 'b': out += '\b'; break;
                case 'f': out += '\f'; break; case 'n': out += '\n'; break;
                case 'r': out += '\r'; break; case 't': out += '\t'; break;
                case 'u':
                    // Configuration identifiers are ASCII; preserve a simple
                    // unicode escape as '?' rather than mis-decoding UTF-16.
                    for (int i = 0; i < 4; ++i) { if (pos_ >= text_.size() || !std::isxdigit(static_cast<unsigned char>(text_[pos_]))) fail("invalid unicode escape"); ++pos_; }
                    out += '?'; break;
                default: fail("invalid escape");
            }
        }
        fail("unterminated string");
    }

    JsonValue parse_object() {
        JsonValue v; v.type = JsonValue::Type::Object; expect('{'); skip_space();
        if (consume('}')) return v;
        for (;;) {
            skip_space(); if (pos_ >= text_.size() || text_[pos_] != '"') fail("object key must be a string");
            std::string key = parse_string(); expect(':'); v.object.emplace(std::move(key), parse_value());
            if (consume('}')) return v; expect(',');
        }
    }

    JsonValue parse_array() {
        JsonValue v; v.type = JsonValue::Type::Array; expect('['); skip_space();
        if (consume(']')) return v;
        for (;;) {
            v.array.push_back(parse_value());
            if (consume(']')) return v; expect(',');
        }
    }
};

inline const JsonValue* field(const JsonValue& object, const char* key) { return object.get(key); }

inline double number(const JsonValue& v, const char* key) {
    if (v.type != JsonValue::Type::Number) throw std::runtime_error(std::string("config field '") + key + "' must be a number");
    return v.number;
}
inline bool boolean(const JsonValue& v, const char* key) {
    if (v.type != JsonValue::Type::Bool) throw std::runtime_error(std::string("config field '") + key + "' must be boolean");
    return v.boolean;
}
inline std::string string(const JsonValue& v, const char* key) {
    if (v.type != JsonValue::Type::String) throw std::runtime_error(std::string("config field '") + key + "' must be string");
    return v.string;
}
inline void set_number(const JsonValue* object, const char* key, double& target) { if (object && field(*object, key)) target = number(*field(*object, key), key); }
inline void set_int(const JsonValue* object, const char* key, int& target) { if (object && field(*object, key)) target = static_cast<int>(number(*field(*object, key), key)); }
inline void set_bool(const JsonValue* object, const char* key, bool& target) { if (object && field(*object, key)) target = boolean(*field(*object, key), key); }

template <size_t N, typename Tree>
inline void apply_talent_tree(const JsonValue* object, const std::array<TalentNodeDef, N>& nodes, Tree& tree, const char* tree_name) {
    if (!object) return;
    if (object->type != JsonValue::Type::Object) throw std::runtime_error(std::string("talent tree '") + tree_name + "' must be an object");
    for (const auto& [id, value] : object->object) {
        bool found = false;
        for (size_t i = 0; i < N; ++i) {
            if (id == nodes[i].id) {
                int points = static_cast<int>(number(value, id.c_str()));
                if (points < 0 || points > nodes[i].max_points) throw std::runtime_error("invalid points for talent: " + id);
                tree.get_points_by_index(i) = points;
                found = true;
                break;
            }
        }
        if (!found) throw std::runtime_error("unknown " + std::string(tree_name) + " talent: " + id);
    }
}

inline RotationChoice parse_rotation(const std::string& value) {
    static const std::map<std::string, RotationChoice> values = {
        {"shadow_destro", RotationChoice::SHADOW_DESTRO}, {"shadow_destro_2", RotationChoice::SHADOW_DESTRO_2},
        {"fire_destro", RotationChoice::FIRE_DESTRO}, {"dp_shadow", RotationChoice::DP_AF_SHADOW},
        {"dp_fire", RotationChoice::DP_RUIN_FIRE}, {"deep_affliction", RotationChoice::DEEP_AFFLICTION_SB},
        {"sm_ruin", RotationChoice::SM_RUIN}, {"pure_shadow_bolt", RotationChoice::PURE_SHADOW_BOLT},
        {"fire_destro_no_corruption", RotationChoice::FIRE_DESTRO_NO_CORRUPTION},
        {"shadow_and_flame_fire", RotationChoice::SHADOW_AND_FLAME_FIRE_BANE},
        {"dp_shadow_brand", RotationChoice::DP_AF_SHADOW_BRAND}
    };
    auto it = values.find(value); if (it == values.end()) throw std::runtime_error("unknown rotation: " + value); return it->second;
}

inline CurseChoice parse_curse(const std::string& value) {
    if (value == "none") return CurseChoice::NONE; if (value == "bane_of_agony" || value == "coa") return CurseChoice::BANE_OF_AGONY; if (value == "bane_of_doom" || value == "cod") return CurseChoice::BANE_OF_DOOM;
    throw std::runtime_error("unknown curse: " + value);
}
inline PetChoice parse_pet(const std::string& value) {
    if (value == "none") return PetChoice::NONE; if (value == "imp") return PetChoice::IMP; if (value == "succubus") return PetChoice::SUCCUBUS; throw std::runtime_error("unknown pet: " + value);
}

inline Race parse_race(const std::string& value) {
    if (value == "human") return Race::HUMAN; if (value == "gnome") return Race::GNOME;
    if (value == "orc") return Race::ORC; if (value == "troll") return Race::TROLL;
    if (value == "undead") return Race::UNDEAD;
    throw std::runtime_error("unknown race: " + value);
}

inline void apply_config(const JsonValue& root, WarlockSimulator& sim, int& iterations, int& threads, uint64_t& seed) {
    if (root.type != JsonValue::Type::Object) throw std::runtime_error("configuration root must be an object");
    if (const auto* v = field(root, "class")) if (string(*v, "class") != "Warlock" && string(*v, "class") != "warlock") throw std::runtime_error("config class must be Warlock");

    const JsonValue* simulation = field(root, "simulation");
    set_int(simulation, "iterations", iterations); set_int(simulation, "threads", threads);
    if (simulation && field(*simulation, "seed")) seed = static_cast<uint64_t>(number(*field(*simulation, "seed"), "seed"));

    const JsonValue* fight = field(root, "fight");
    if (fight) { set_number(fight, "duration", sim.fight_duration); set_number(fight, "duration_s", sim.fight_duration); set_number(fight, "duration_variance", sim.duration_variance); set_number(fight, "duration_variance_s", sim.duration_variance); set_bool(fight, "randomize_duration", sim.randomize_duration); }
    if (const auto* v = field(root, "race")) { sim.race = parse_race(string(*v, "race")); sim.base_attrs = get_base_attributes_for_race(sim.race); }

    if (const auto* v = field(root, "spec")) {
        const auto* preset = find_spec_preset(string(*v, "spec"));
        if (!preset) throw std::runtime_error("unknown spec preset: " + string(*v, "spec"));
        apply_spec_preset(sim, *preset);
    }
    const JsonValue* build = field(root, "build");
    if (build && field(*build, "spec")) {
        const auto* preset = find_spec_preset(string(*field(*build, "spec"), "build.spec"));
        if (!preset) throw std::runtime_error("unknown build spec preset");
        apply_spec_preset(sim, *preset);
    }
    const JsonValue* talents = build ? field(*build, "talents") : field(root, "talents");
    if (talents) {
        sim.talents = {};
        apply_talent_tree(field(*talents, "affliction"), FOREVER_AFFLICTION_NODES, sim.talents.aff, "affliction");
        apply_talent_tree(field(*talents, "demonology"), FOREVER_DEMONOLOGY_NODES, sim.talents.demo, "demonology");
        apply_talent_tree(field(*talents, "destruction"), FOREVER_DESTRUCTION_NODES, sim.talents.destro, "destruction");
    }
    if (const auto* gear = field(root, "gear")) {
        if (const auto* v = field(*gear, "preset")) {
            std::string name = string(*v, "gear.preset");
            if (name == "preraid") sim.gear = GearLoadout::create_preraid_bis();
            else if (name == "p3") sim.gear = GearLoadout::create_phase3_bis();
            else if (name == "p5") sim.gear = GearLoadout::create_phase5_bis();
            else if (name == "p6") sim.gear = GearLoadout::create_phase6_bis();
            else throw std::runtime_error("unknown gear preset: " + name);
        }
    }
    bool final_attributes = false;
    if (const auto* mode = field(root, "stats_mode")) {
        std::string mode_name = string(*mode, "stats_mode");
        sim.use_raw_stats = mode_name == "raw" || mode_name == "final";
        final_attributes = mode_name == "final";
    }
    if (const auto* raw = field(root, "raw_stats")) {
        sim.use_raw_stats = true;
        set_number(raw, "stamina", sim.raw_stats.stamina); set_number(raw, "intellect", sim.raw_stats.intellect); set_number(raw, "spirit", sim.raw_stats.spirit);
        if (final_attributes) {
            sim.raw_stats.stamina -= sim.base_attrs.stamina;
            sim.raw_stats.intellect -= sim.base_attrs.intellect;
            sim.raw_stats.spirit -= sim.base_attrs.spirit;
        }
        set_number(raw, "spell_power", sim.raw_stats.spell_power); set_number(raw, "shadow_power", sim.raw_stats.shadow_power); set_number(raw, "fire_power", sim.raw_stats.fire_power);
        set_number(raw, "spell_hit_percent", sim.raw_stats.spell_hit_percent); set_number(raw, "spell_crit_percent", sim.raw_stats.spell_crit_percent); set_number(raw, "spell_haste_percent", sim.raw_stats.spell_haste_percent); set_number(raw, "mp5", sim.raw_stats.mp5);
        if (final_attributes) {
            sim.raw_stats.spell_crit_percent -= sim.base_attrs.base_spell_crit + (sim.base_attrs.intellect + sim.raw_stats.intellect) / 60.6;
        }
    }

    const auto apply_bools = [](const JsonValue* object, const std::map<std::string, bool*>& fields) {
        if (!object) return; for (const auto& [key, target] : fields) if (const auto* v = field(*object, key.c_str())) *target = boolean(*v, key.c_str());
    };
    apply_bools(field(root, "buffs"), {
        {"arcane_intellect", &sim.buffs.arcane_intellect}, {"blessing_of_kings", &sim.buffs.blessing_of_kings}, {"blessing_of_wisdom", &sim.buffs.blessing_of_wisdom}, {"mark_of_the_wild", &sim.buffs.mark_of_the_wild}, {"judgement_of_wisdom", &sim.buffs.judgement_of_wisdom},
        {"flask_of_supreme_power", &sim.buffs.flask_of_supreme_power}, {"flask_of_distilled_wisdom", &sim.buffs.flask_of_distilled_wisdom}, {"flask_of_the_titans", &sim.buffs.flask_of_the_titans}, {"greater_arcane_elixir", &sim.buffs.greater_arcane_elixir}, {"elixir_of_shadow_power", &sim.buffs.elixir_of_shadow_power}, {"elixir_of_greater_firepower", &sim.buffs.elixir_of_greater_firepower}, {"elixir_of_the_owl", &sim.buffs.elixir_of_the_owl}, {"elixir_of_the_sages", &sim.buffs.elixir_of_the_sages}, {"mageblood_elixir", &sim.buffs.mageblood_elixir}, {"greater_mageblood_elixir", &sim.buffs.greater_mageblood_elixir}, {"brilliant_wizard_oil", &sim.buffs.brilliant_wizard_oil}, {"use_mana_potions", &sim.buffs.use_mana_potions}, {"use_demonic_runes", &sim.buffs.use_demonic_runes}, {"rallying_cry", &sim.buffs.rallying_cry}, {"songflower", &sim.buffs.songflower}, {"spirit_of_zandalar", &sim.buffs.spirit_of_zandalar}, {"warchiefs_blessing", &sim.buffs.warchiefs_blessing}, {"sayges_fortune", &sim.buffs.sayges_fortune},
        {"curse_of_shadows", &sim.buffs.curse_of_shadows}, {"curse_of_elements", &sim.buffs.curse_of_elements}, {"shadow_weaving", &sim.buffs.shadow_weaving}, {"nightfall_axe", &sim.buffs.nightfall_axe}, {"sacrifice_imp", &sim.buffs.sacrifice_imp}, {"sacrifice_succubus", &sim.buffs.sacrifice_succubus}
    });

    const JsonValue* policy = build ? (field(*build, "policy") ? field(*build, "policy") : field(root, "policy")) : field(root, "policy");
    if (policy) {
        if (const auto* v = field(*policy, "rotation")) sim.policy.rotation = parse_rotation(string(*v, "policy.rotation"));
        if (const auto* v = field(*policy, "curse")) sim.policy.curse = parse_curse(string(*v, "policy.curse"));
        if (const auto* v = field(*policy, "pet")) sim.policy.pet = parse_pet(string(*v, "policy.pet"));
        set_bool(policy, "maintain_immolate", sim.policy.maintain_immolate); set_number(policy, "life_tap_threshold_pct", sim.policy.life_tap_threshold_pct);
        set_bool(policy, "use_trinkets_on_cooldown", sim.policy.use_trinkets_on_cooldown); set_bool(policy, "cast_nightfall_procs", sim.policy.cast_nightfall_procs); set_bool(policy, "use_conflagrate", sim.policy.use_conflagrate); set_bool(policy, "use_incinerate", sim.policy.use_incinerate); set_bool(policy, "use_decimation_soul_fire", sim.policy.use_decimation_soul_fire); set_bool(policy, "channel_drain_hope", sim.policy.channel_drain_hope); set_bool(policy, "multi_dot_corruption", sim.policy.multi_dot_corruption); set_bool(policy, "auto_bane_of_havoc", sim.policy.auto_bane_of_havoc);
    }
    if (build) {
        if (const auto* v = field(*build, "rotation")) {
            if (v->type == JsonValue::Type::String) sim.policy.rotation = parse_rotation(string(*v, "build.rotation"));
            else if (v->type == JsonValue::Type::Array) sim.policy.rotation = RotationChoice::DP_AF_SHADOW_BRAND;
            else throw std::runtime_error("build.rotation must be a string or action array");
        }
        if (const auto* v = field(*build, "pet")) sim.policy.pet = parse_pet(string(*v, "build.pet"));
        if (const auto* v = field(*build, "sacrifice")) {
            std::string sacrifice = string(*v, "build.sacrifice");
            sim.buffs.sacrifice_imp = sacrifice == "imp";
            sim.buffs.sacrifice_succubus = sacrifice == "succubus";
            if (sacrifice != "none" && sacrifice != "imp" && sacrifice != "succubus") throw std::runtime_error("unknown build sacrifice: " + sacrifice);
        }
    }

    const JsonValue* mechanics = field(root, "mechanics");
    if (mechanics) {
        set_bool(mechanics, "snapshot_dots", sim.mechanics.snapshot_dots); set_bool(mechanics, "spell_batching", sim.mechanics.spell_batching); set_number(mechanics, "batch_window_ms", sim.mechanics.batch_window_ms);
        set_int(mechanics, "debuff_limit", sim.mechanics.debuff_limit); set_bool(mechanics, "enforce_debuff_slots", sim.mechanics.enforce_debuff_slots); set_bool(mechanics, "personal_shadow_weaving", sim.mechanics.personal_shadow_weaving);
        set_bool(mechanics, "partial_resists_enabled", sim.mechanics.partial_resists_enabled); set_number(mechanics, "base_hit_vs_boss", sim.mechanics.base_hit_vs_boss); set_number(mechanics, "max_spell_hit", sim.mechanics.max_spell_hit); set_number(mechanics, "base_spell_crit_multiplier", sim.mechanics.base_spell_crit_multiplier);
        set_bool(mechanics, "projectile_travel_time", sim.mechanics.projectile_travel_time); set_number(mechanics, "default_boss_distance_yards", sim.mechanics.default_boss_distance_yards); set_number(mechanics, "projectile_speed_yards_per_sec", sim.mechanics.projectile_speed_yards_per_sec); set_bool(mechanics, "imp_firebolt_modern_scaling", sim.mechanics.imp_firebolt_modern_scaling);
    }
    const JsonValue* target = field(root, "target");
    if (target) { set_int(target, "target_count", sim.target_config.target_count); set_int(target, "level", sim.target_config.level); set_number(target, "base_shadow_resistance", sim.target_config.base_shadow_resistance); set_number(target, "base_fire_resistance", sim.target_config.base_fire_resistance); set_bool(target, "is_beast", sim.target_config.is_beast); set_bool(target, "curse_of_shadows", sim.target_config.curse_of_shadows); set_bool(target, "curse_of_elements", sim.target_config.curse_of_elements); set_bool(target, "shadow_weaving", sim.target_config.shadow_weaving); set_bool(target, "stormstrike", sim.target_config.stormstrike); set_bool(target, "nightfall_axe_proc", sim.target_config.nightfall_axe_proc); }
}

inline void load_file(const std::string& path, WarlockSimulator& sim, int& iterations, int& threads, uint64_t& seed) {
    std::ifstream input(path); if (!input.is_open()) throw std::runtime_error("cannot open config file: " + path);
    std::ostringstream contents; contents << input.rdbuf();
    apply_config(Parser(contents.str()).parse(), sim, iterations, threads, seed);
}

} // namespace warlock::cli_config
