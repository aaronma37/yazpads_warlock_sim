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
#include "src/sim/warlock/imitation_training.hpp"

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
    for (int i = 0; i <= static_cast<int>(RotationChoice::DP_AF_SHADOW_BRAND); ++i)
        if (value == rotation_choice_to_string(static_cast<RotationChoice>(i))) return static_cast<RotationChoice>(i);
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
    for (int i = 0; i <= 2; ++i)
        if (value == curse_choice_to_string(static_cast<CurseChoice>(i))) return static_cast<CurseChoice>(i);
    if (value == "none") return CurseChoice::NONE; if (value == "bane_of_agony" || value == "coa") return CurseChoice::BANE_OF_AGONY; if (value == "bane_of_doom" || value == "cod") return CurseChoice::BANE_OF_DOOM;
    throw std::runtime_error("unknown curse: " + value);
}
inline PetChoice parse_pet(const std::string& value) {
    for (int i = 0; i <= 2; ++i)
        if (value == pet_choice_to_string(static_cast<PetChoice>(i))) return static_cast<PetChoice>(i);
    if (value == "none") return PetChoice::NONE; if (value == "imp") return PetChoice::IMP; if (value == "succubus") return PetChoice::SUCCUBUS; throw std::runtime_error("unknown pet: " + value);
}

inline PriorityRule custom_rule(PriorityAction action, SpellID spell_id, const char* name) {
    PriorityRule rule;
    rule.action = action;
    rule.spell_id = spell_id;
    rule.name = name;
    rule.condition_summary = "APL action";
    rule.trigger_condition = "Trigger according to the canonical comparison APL.";
    rule.rule_explanation = "Loaded from the canonical comparison rotation action list.";
    rule.use_custom_thresholds = true;
    return rule;
}

inline void apply_rotation_array(const JsonValue& rotation, PolicyConfig& policy) {
    if (rotation.type != JsonValue::Type::Array) throw std::runtime_error("build.rotation must be a string or action array");
    policy.custom_rules.clear();
    policy.use_custom_apl = true;

    for (const auto& value : rotation.array) {
        const std::string action = string(value, "build.rotation[]");
        if (action == "curse_of_elements") {
            // CoE is an encounter debuff in the C++ model, configured through
            // buffs/target. The comparison config marks it external, so it is
            // intentionally not a player GCD action here.
            continue;
        }
        if (action == "bane") {
            auto doom = custom_rule(PriorityAction::CURSE_OF_DOOM, SpellID::CURSE_OF_DOOM, "Bane of Doom");
            doom.check_doom_debuff = true;
            doom.require_doom_missing = true;
            doom.check_fight_time = true;
            doom.min_time_remaining = 60.0f;
            policy.custom_rules.push_back(doom);

            auto agony = custom_rule(PriorityAction::CURSE_OF_AGONY, SpellID::CURSE_OF_AGONY, "Bane of Agony");
            agony.check_doom_debuff = true;
            agony.require_doom_missing = true;
            agony.check_dot_refresh = true;
            agony.check_fight_time = true;
            agony.min_time_remaining = 12.0f;
            agony.max_dot_rem_sec = 0.0f;
            policy.custom_rules.push_back(agony);
        } else if (action == "searingPainBrand" || action == "searing_pain_brand") {
            auto rule = custom_rule(PriorityAction::DEMONIC_BRAND_SEARING_PAIN, SpellID::DEMONIC_BRAND, "Demonic Brand (Searing Pain)");
            rule.check_demonic_brand = true;
            rule.require_demonic_brand_missing = true;
            policy.custom_rules.push_back(rule);
        } else if (action == "immolate") {
            auto rule = custom_rule(PriorityAction::IMMOLATE, SpellID::IMMOLATE, "Immolate");
            rule.check_dot_refresh = true;
            rule.max_dot_rem_sec = 0.0f;
            policy.custom_rules.push_back(rule);
        } else if (action == "corruption") {
            auto rule = custom_rule(PriorityAction::CORRUPTION, SpellID::CORRUPTION, "Corruption");
            rule.check_dot_refresh = true;
            rule.max_dot_rem_sec = 0.0f;
            policy.custom_rules.push_back(rule);
        } else if (action == "shadowBolt" || action == "shadow_bolt") {
            policy.custom_rules.push_back(custom_rule(PriorityAction::SHADOW_BOLT_FILLER, SpellID::SHADOW_BOLT, "Shadow Bolt"));
        } else {
            throw std::runtime_error("unknown build.rotation action: " + action);
        }
    }
    if (policy.custom_rules.empty()) throw std::runtime_error("build.rotation action array produced no C++ APL actions");
}

inline Race parse_race(const std::string& value) {
    for (auto race : {Race::HUMAN, Race::GNOME, Race::ORC, Race::TROLL, Race::UNDEAD})
        if (value == race_to_string(race)) return race;
    if (value == "human") return Race::HUMAN; if (value == "gnome") return Race::GNOME;
    if (value == "orc") return Race::ORC; if (value == "troll") return Race::TROLL;
    if (value == "undead") return Race::UNDEAD;
    throw std::runtime_error("unknown race: " + value);
}

inline void apply_config(const JsonValue& root, WarlockSimulator& sim, int& iterations, int& threads, uint64_t& seed,
    const std::filesystem::path& config_directory = {}) {
    if (root.type != JsonValue::Type::Object) throw std::runtime_error("configuration root must be an object");
    if (const auto* v = field(root, "class")) if (string(*v, "class") != "Warlock" && string(*v, "class") != "warlock") throw std::runtime_error("config class must be Warlock");

    const JsonValue* simulation = field(root, "simulation");
    set_int(simulation, "iterations", iterations); set_int(simulation, "threads", threads);
    if (simulation && field(*simulation, "seed")) seed = static_cast<uint64_t>(number(*field(*simulation, "seed"), "seed"));
    const JsonValue* exported_simulation = field(root, "sim_config");
    set_int(exported_simulation, "iterations", iterations);
    set_int(exported_simulation, "worker_threads", threads);

    const JsonValue* fight = field(root, "fight");
    if (fight) { set_number(fight, "duration", sim.fight_duration); set_number(fight, "duration_s", sim.fight_duration); set_number(fight, "duration_variance", sim.duration_variance); set_number(fight, "duration_variance_s", sim.duration_variance); set_bool(fight, "randomize_duration", sim.randomize_duration); }
    set_number(&root, "fight_duration", sim.fight_duration);
    set_number(&root, "duration_variance", sim.duration_variance);
    set_bool(&root, "randomize_duration", sim.randomize_duration);
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
        if (const auto* slots = field(*gear, "slots")) {
            sim.gear.items = {};
            for (size_t i = 0; i < static_cast<size_t>(Slot::COUNT); ++i) {
                auto slot = static_cast<Slot>(i);
                const auto* value = field(*slots, slot_to_name(slot));
                if (!value) continue;
                const auto name = string(*value, "gear.slots item");
                if (name.empty() || name == Item{}.name) continue;
                const auto* item = ItemDatabase::find_by_name(name);
                if (!item) throw std::runtime_error("Unknown equipped item: " + name);
                sim.gear.equip(slot, *item);
            }
        }
        if (const auto* name = field(*gear, "loadout_name")) sim.gear.name = string(*name, "gear.loadout_name");
        set_number(gear, "extra_spell_power", sim.gear.extra_spell_power);
        set_number(gear, "extra_shadow_power", sim.gear.extra_shadow_power);
        set_number(gear, "extra_spell_hit", sim.gear.extra_spell_hit);
        set_number(gear, "extra_spell_crit", sim.gear.extra_spell_crit);
        set_number(gear, "extra_spell_penetration", sim.gear.extra_spell_penetration);
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
        set_number(raw, "spell_penetration", sim.raw_stats.spell_penetration);
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
        if (const auto* value = field(*policy, "corruption")) {
            const auto name = string(*value, "policy.corruption");
            if (name == "always") sim.policy.corruption = DotPolicy::ALWAYS;
            else if (name == "never") sim.policy.corruption = DotPolicy::NEVER;
            else if (name == "only_with_debuff_slot") sim.policy.corruption = DotPolicy::ONLY_WITH_DEBUFF_SLOT;
            else throw std::runtime_error("Unknown corruption policy: " + name);
        }
        if (const auto* value = field(*policy, "shadowburn")) {
            const auto name = string(*value, "policy.shadowburn");
            if (name == "on_cooldown") sim.policy.shadowburn = ShadowburnPolicy::ON_COOLDOWN;
            else if (name == "execute_only") sim.policy.shadowburn = ShadowburnPolicy::EXECUTE_ONLY;
            else if (name == "never") sim.policy.shadowburn = ShadowburnPolicy::NEVER;
            else throw std::runtime_error("Unknown shadowburn policy: " + name);
        }
        if (const auto* value = field(*policy, "racial_policy")) {
            const auto name = string(*value, "policy.racial_policy");
            bool matched = false;
            for (int i = 0; i <= 3; ++i) if (name == racial_policy_to_string(static_cast<RacialPolicy>(i))) {
                sim.policy.racial_policy = static_cast<RacialPolicy>(i); matched = true; break;
            }
            if (!matched) throw std::runtime_error("Unknown racial policy: " + name);
        }
        if (const auto* trained = field(*policy, "trained_gbdt")) {
            if (trained->type != JsonValue::Type::Object) throw std::runtime_error("policy.trained_gbdt must be an object");
            bool enabled = true;
            set_bool(trained, "enabled", enabled);
            std::string name = "Configured GBDT";
            if (const auto* value = field(*trained, "name")) name = string(*value, "trained_gbdt.name");
            auto loaded = std::make_shared<SearchImitationPolicy>();
            if (const auto* value = field(*trained, "model"); value && value->type != JsonValue::Type::Null)
                loaded->deserialize(string(*value, "trained_gbdt.model"));
            else if (const auto* value = field(*trained, "path")) {
                auto file = std::filesystem::path(string(*value, "trained_gbdt.path"));
                if (file.is_relative()) file = config_directory / file;
                if (std::filesystem::is_directory(file)) file /= "policy.gbdt";
                loaded->load(file.string());
            }
            if (loaded->classes.empty()) {
                if (enabled) throw std::runtime_error("Enabled trained_gbdt requires model or path");
                sim.policy.imitation_policy.reset();
                sim.policy.imitation_policy_name = name;
                sim.policy.use_imitation_policy = false;
            } else {
                activate_imitation_policy(sim, std::move(loaded), name);
                sim.policy.use_imitation_policy = enabled;
            }
        }
    }
    if (build) {
        if (const auto* v = field(*build, "rotation")) {
            if (v->type == JsonValue::Type::String) sim.policy.rotation = parse_rotation(string(*v, "build.rotation"));
            else if (v->type == JsonValue::Type::Array) apply_rotation_array(*v, sim.policy);
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
        set_bool(mechanics, "use_book_spell_ranks", sim.mechanics.use_book_spell_ranks);
        set_bool(mechanics, "allow_rank2_shadow_bolt", sim.mechanics.allow_rank2_shadow_bolt);
        set_bool(mechanics, "snapshot_dots", sim.mechanics.snapshot_dots); set_bool(mechanics, "spell_batching", sim.mechanics.spell_batching); set_number(mechanics, "batch_window_ms", sim.mechanics.batch_window_ms);
        set_int(mechanics, "debuff_limit", sim.mechanics.debuff_limit); set_bool(mechanics, "enforce_debuff_slots", sim.mechanics.enforce_debuff_slots); set_bool(mechanics, "personal_shadow_weaving", sim.mechanics.personal_shadow_weaving);
        set_bool(mechanics, "partial_resists_enabled", sim.mechanics.partial_resists_enabled); set_number(mechanics, "base_hit_vs_boss", sim.mechanics.base_hit_vs_boss); set_number(mechanics, "max_spell_hit", sim.mechanics.max_spell_hit); set_number(mechanics, "base_spell_crit_multiplier", sim.mechanics.base_spell_crit_multiplier); set_number(mechanics, "corruption_sp_coefficient", sim.mechanics.corruption_sp_coefficient);
        set_bool(mechanics, "projectile_travel_time", sim.mechanics.projectile_travel_time); set_number(mechanics, "default_boss_distance_yards", sim.mechanics.default_boss_distance_yards); set_number(mechanics, "projectile_speed_yards_per_sec", sim.mechanics.projectile_speed_yards_per_sec); set_bool(mechanics, "imp_firebolt_modern_scaling", sim.mechanics.imp_firebolt_modern_scaling);
        set_bool(mechanics, "isb_has_charges", sim.mechanics.isb_has_charges); set_bool(mechanics, "isb_all_shadow_sources", sim.mechanics.isb_all_shadow_sources);
        set_bool(mechanics, "nightfall_enabled", sim.mechanics.nightfall_enabled); set_number(mechanics, "nightfall_proc_chance", sim.mechanics.nightfall_proc_chance);
        set_number(mechanics, "base_gcd", sim.mechanics.base_gcd); set_bool(mechanics, "haste_affects_gcd", sim.mechanics.haste_affects_gcd);
        set_bool(mechanics, "instant_drain_hope", sim.mechanics.instant_drain_hope); set_bool(mechanics, "pet_scaling", sim.mechanics.pet_scaling);
        set_number(mechanics, "pet_sp_ratio", sim.mechanics.pet_sp_ratio); set_number(mechanics, "pet_ap_ratio", sim.mechanics.pet_ap_ratio);
        set_bool(mechanics, "pet_mana_management", sim.mechanics.pet_mana_management); set_number(mechanics, "imp_base_mana", sim.mechanics.imp_base_mana);
        set_number(mechanics, "succubus_base_mana", sim.mechanics.succubus_base_mana); set_number(mechanics, "imp_firebolt_cost", sim.mechanics.imp_firebolt_cost);
        set_number(mechanics, "succubus_lop_cost", sim.mechanics.succubus_lop_cost); set_number(mechanics, "pet_base_mp5", sim.mechanics.pet_base_mp5);
    }
    const JsonValue* target = field(root, "target");
    if (target) { set_int(target, "target_count", sim.target_config.target_count); set_int(target, "level", sim.target_config.level); set_number(target, "boss_armor", sim.target_config.boss_armor); set_bool(target, "sunder_armor", sim.target_config.sunder_armor); set_bool(target, "faerie_fire", sim.target_config.faerie_fire); set_number(target, "base_shadow_resistance", sim.target_config.base_shadow_resistance); set_number(target, "base_fire_resistance", sim.target_config.base_fire_resistance); set_bool(target, "is_beast", sim.target_config.is_beast); set_bool(target, "curse_of_shadows", sim.target_config.curse_of_shadows); set_bool(target, "curse_of_elements", sim.target_config.curse_of_elements); set_bool(target, "shadow_weaving", sim.target_config.shadow_weaving); set_bool(target, "stormstrike", sim.target_config.stormstrike); set_bool(target, "nightfall_axe_proc", sim.target_config.nightfall_axe_proc); }
    if (target && field(*target, "creature_type")) {
        const auto name = string(*field(*target, "creature_type"), "target.creature_type");
        bool matched = false;
        for (int i = 0; i <= 8; ++i) if (name == creature_type_to_string(static_cast<CreatureType>(i))) {
            sim.target_config.creature_type = static_cast<CreatureType>(i); matched = true; break;
        }
        if (!matched) throw std::runtime_error("Unknown creature type: " + name);
    }
}

inline void load_file(const std::string& path, WarlockSimulator& sim, int& iterations, int& threads, uint64_t& seed) {
    std::ifstream input(path); if (!input.is_open()) throw std::runtime_error("cannot open config file: " + path);
    std::ostringstream contents; contents << input.rdbuf();
    apply_config(Parser(contents.str()).parse(), sim, iterations, threads, seed, std::filesystem::path(path).parent_path());
}

} // namespace warlock::cli_config
