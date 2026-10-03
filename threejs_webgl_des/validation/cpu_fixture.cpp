// Authoritative Native C++ Oracle Adapter
// Treats src/sim/warlock/warlock_sim.cpp as the ground truth.
// Executes true spec presets and custom scenarios with full spell and pet breakdowns.

#include "src/sim/warlock/warlock_sim.hpp"
#include "src/sim/warlock/spec_presets.hpp"
#include "src/sim/warlock/parallel_runner.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <sstream>

using namespace warlock;

struct SpellEntry {
    SpellID id;
    std::string name;
};

static const std::vector<SpellEntry> ALL_SPELL_ENTRIES = {
    {SpellID::SHADOW_BOLT, "Shadow Bolt"},
    {SpellID::CORRUPTION, "Corruption"},
    {SpellID::CURSE_OF_AGONY, "Bane of Agony"},
    {SpellID::CURSE_OF_DOOM, "Curse of Doom"},
    {SpellID::IMMOLATE, "Immolate"},
    {SpellID::INCINERATE, "Incinerate"},
    {SpellID::SEARING_PAIN, "Searing Pain"},
    {SpellID::CONFLAGRATE, "Conflagrate"},
    {SpellID::SOUL_FIRE, "Soul Fire"},
    {SpellID::SHADOWBURN, "Shadowburn"},
    {SpellID::SIPHON_LIFE, "Siphon Life"},
    {SpellID::DRAIN_HOPE, "Wrack / Drain Hope"},
    {SpellID::DRAIN_SOUL, "Drain Soul"},
    {SpellID::DRAIN_LIFE, "Drain Life"},
    {SpellID::PET_FIREBOLT, "Imp Firebolt (Pet)"},
    {SpellID::PET_MELEE, "Succubus Melee (Pet)"},
    {SpellID::PET_LASH_OF_PAIN, "Succubus Lash of Pain (Pet)"},
    {SpellID::DEMONIC_BRAND, "Demonic Brand (Pet Proc)"}
};

void output_sim_result_json(const SimResult& r, uint64_t next_rng, double duration) {
    std::cout << "{\"total\":" << r.total_damage 
              << ",\"dps\":" << (r.total_damage / (duration > 0 ? duration : 180.0))
              << ",\"mana\":" << r.final_mana
              << ",\"spent\":" << r.mana_spent 
              << ",\"gained\":" << r.mana_gained
              << ",\"taps\":" << r.life_taps 
              << ",\"procs\":" << r.nightfall_procs
              << ",\"isbProcs\":" << r.isb_procs 
              << ",\"isbConsumed\":" << r.isb_consumed
              << ",\"petDamage\":" << r.dmg_pet
              << ",\"impDamage\":" << r.dmg_pet_imp
              << ",\"succubusDamage\":" << r.dmg_pet_succubus
              << ",\"demonicBrandDamage\":" << r.dmg_demonic_brand
              << ",\"nextRandom\":[" << uint32_t(next_rng) << ',' << uint32_t(next_rng >> 32) << ']';

    // Core 6 spells legacy indices (0..5) for baseline compatibility
    const SpellID core_spells[] = {
        SpellID::SHADOW_BOLT, SpellID::CORRUPTION, SpellID::CURSE_OF_AGONY,
        SpellID::IMMOLATE, SpellID::INCINERATE, SpellID::SEARING_PAIN
    };
    for (int i = 0; i < 6; ++i) {
        const auto& s = r.spell_stats[static_cast<size_t>(core_spells[i])];
        std::cout << ",\"damage" << i << "\":" << s.damage
                  << ",\"casts" << i << "\":" << s.casts 
                  << ",\"hits" << i << "\":" << s.hits
                  << ",\"crits" << i << "\":" << s.crits 
                  << ",\"misses" << i << "\":" << s.misses;
    }

    // Full comprehensive breakdown for all spells
    std::cout << ",\"spellBreakdown\":[";
    bool first_spell = true;
    for (const auto& entry : ALL_SPELL_ENTRIES) {
        const auto& s = r.spell_stats[static_cast<size_t>(entry.id)];
        double dmg = s.damage;

        if (dmg > 0 || s.casts > 0 || s.hits > 0) {
            if (!first_spell) std::cout << ',';
            first_spell = false;
            std::cout << "{\"id\":" << static_cast<int>(entry.id)
                      << ",\"name\":\"" << entry.name << "\""
                      << ",\"damage\":" << dmg
                      << ",\"dps\":" << (dmg / (duration > 0 ? duration : 180.0))
                      << ",\"casts\":" << s.casts
                      << ",\"hits\":" << s.hits
                      << ",\"crits\":" << s.crits
                      << ",\"misses\":" << s.misses
                      << ",\"avgHit\":" << (s.hits > 0 ? (dmg / s.hits) : 0.0)
                      << "}";
        }
    }
    std::cout << "]";

    // Damage trace for positive damage events
    std::cout << ",\"damageTrace\":[";
    bool comma = false;
    for (const auto& e : r.timeline) {
        if (e.damage > 0) {
            if (comma) std::cout << ',';
            comma = true;
            std::cout << '[' << e.time << ',' << int(e.spell_id) << ',' << e.damage << ',' << (e.is_crit ? 1 : 0) << ']';
        }
    }
    std::cout << "]}\n";
}

void run_preset(const std::string& preset_id, uint64_t seed, double duration) {
    const SpecPreset* preset = find_spec_preset(preset_id);
    if (!preset) {
        std::cerr << "{\"error\":\"Preset not found: " << preset_id << "\"}\n";
        return;
    }

    WarlockSimulator sim;
    sim.race = Race::HUMAN;
    sim.fight_duration = duration;
    sim.record_timeline = true;
    sim.buffs.curse_of_shadows = false;
    sim.buffs.curse_of_elements = false;
    sim.mechanics.isb_has_charges = false; // Default: uncapped 12s ISB aura
    sim.mechanics.pet_mana_management = true;
    sim.mechanics.pet_scaling = true;

    // Apply official spec preset
    apply_spec_preset(sim, *preset);

    // Standard baseline raid stats
    sim.use_raw_stats = true;
    sim.raw_stats = Stats{};
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 100.0;
    sim.raw_stats.stamina = 220.0;
    sim.raw_stats.spell_hit_percent = 12.0;
    sim.raw_stats.spell_crit_percent = 15.0;
    sim.raw_stats.mp5 = 20.0;

    FastRNG rng(seed);
    const auto r = sim.run_single_simulation(rng);
    const auto next = rng.next_u64();
    output_sim_result_json(r, next, duration);
}

void output_preset_info_json(const SpecPreset& p) {
    Talents talents = p.make_talents();
    std::string pid = p.id;
    std::string rot = "shadow";
    if (p.rotation == RotationChoice::FIRE_DESTRO ||
        p.rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION ||
        p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2 ||
        p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE ||
        p.rotation == RotationChoice::DP_RUIN_FIRE) {
        // DP_RUIN_FIRE uses the dedicated Searing Pain priority list in the
        // CPU policy (build_dp_fire_searing_rules), despite its broad "Fire"
        // preset label. Keep the browser APL on the same filler spell.
        if (p.rotation == RotationChoice::DP_RUIN_FIRE ||
            pid.find("searing") != std::string::npos) rot = "searing";
        else rot = "fire";
    }

    bool has_corr = (p.rotation != RotationChoice::FIRE_DESTRO_NO_CORRUPTION &&
                     p.rotation != RotationChoice::DP_AF_SHADOW_NO_CORRUPTION &&
                     p.rotation != RotationChoice::PURE_SHADOW_BOLT &&
                     rot != "fire" && rot != "searing");
    if (p.rotation == RotationChoice::FIRE_DESTRO || 
        p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2 || 
        p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE ||
        p.rotation == RotationChoice::DP_RUIN_FIRE) {
        has_corr = true;
    }

    bool has_agony = (p.rotation != RotationChoice::FIRE_DESTRO_NO_CORRUPTION &&
                      p.rotation != RotationChoice::PURE_SHADOW_BOLT &&
                      p.rotation != RotationChoice::DP_AF_SHADOW_NO_BANE &&
                      p.rotation != RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE_NO_BANE &&
                      rot != "searing");
    if (p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE || 
        p.rotation == RotationChoice::FIRE_DESTRO ||
        p.rotation == RotationChoice::DP_RUIN_FIRE) {
        has_agony = true;
    }

    bool has_imm = (p.rotation == RotationChoice::FIRE_DESTRO ||
                    p.rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION ||
                    p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2 ||
                    p.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE ||
                    p.rotation == RotationChoice::DP_RUIN_FIRE ||
                    p.rotation == RotationChoice::AFFLICTION_HYBRID_DOTS ||
                    ((p.rotation == RotationChoice::SHADOW_DESTRO || p.rotation == RotationChoice::SHADOW_DESTRO_2) && talents.destro.shadow_and_flame > 0 && talents.destro.conflagrate > 0) ||
                    (rot == "fire") || (rot == "searing"));

    bool conflag = (talents.destro.conflagrate > 0 && has_imm && rot != "bolt" && p.rotation != RotationChoice::DP_RUIN_FIRE);
    bool sb = (talents.destro.shadowburn > 0 && rot != "bolt" &&
               p.rotation != RotationChoice::DP_AF_SHADOW &&
               p.rotation != RotationChoice::DP_AF_SHADOW_BRAND &&
               p.rotation != RotationChoice::DP_RUIN_FIRE &&
               p.rotation != RotationChoice::DEEP_AFFLICTION &&
               p.rotation != RotationChoice::DEEP_AFFLICTION_SB);
    bool doom = has_agony;
    bool decimation = (talents.demo.decimation > 0);
    bool demonic_brand = (talents.demo.demonic_brand > 0);
    bool has_siphon = (talents.aff.siphon_life > 0 && (p.rotation == RotationChoice::DEEP_AFFLICTION || p.rotation == RotationChoice::DEEP_AFFLICTION_SB || p.rotation == RotationChoice::AFFLICTION_HYBRID_DOTS));
    bool has_drain_hope = (talents.aff.drain_hope > 0 && (p.rotation == RotationChoice::DEEP_AFFLICTION || p.rotation == RotationChoice::DEEP_AFFLICTION_SB || p.rotation == RotationChoice::DEEP_AFFLICTION_SB_NO_SL || p.rotation == RotationChoice::AFFLICTION_HYBRID_DOTS));
    double improved_drains_bonus = (talents.aff.improved_drains == 1) ? 0.07 : ((talents.aff.improved_drains == 2) ? 0.13 : ((talents.aff.improved_drains >= 3) ? 0.20 : 0.0));
    double soul_siphon_bonus = talents.aff.soul_siphon * 0.04;

    std::string pet = (p.pet == PetChoice::IMP) ? "imp" : ((p.pet == PetChoice::SUCCUBUS) ? "succubus" : "none");

    double sp = 500.0;
    if (p.pet != PetChoice::NONE && talents.demo.demonic_knowledge > 0) {
        sp += 20.0 * talents.demo.demonic_knowledge;
    }

    double hit = 12.0 + talents.aff.suppression * 1.0;
    double crit = 15.0;

    double af_bonus = (talents.destro.agonizing_flames == 1) ? 0.03 : 
                     ((talents.destro.agonizing_flames == 2) ? 0.07 : 
                     ((talents.destro.agonizing_flames == 3) ? 0.10 : 0.0));
    double aftermath_bonus = talents.destro.aftermath * 0.10;
    double malevolence = talents.aff.malevolence * 1.0;
    double malediction_bonus = talents.aff.malediction * 0.01;
    double shadow_mastery_bonus = talents.aff.shadow_mastery * 0.01;
    double improved_corruption_bonus = (talents.aff.improved_corruption >= 5 ? 0.10 : 0.02 * talents.aff.improved_corruption);

    // Base Shadow multiplier (Demonic Sacrifice of Imp in Forever = +15% Shadow)
    double s_mult = 1.0;
    if (p.sac_imp) s_mult *= 1.15;
    if (p.pet == PetChoice::SUCCUBUS && talents.demo.master_demonologist > 0) {
        s_mult *= (1.0 + talents.demo.master_demonologist * 0.02);
    }
    if (p.pet != PetChoice::NONE && talents.demo.soul_link > 0) {
        s_mult *= 1.03;
    }

    // Base Fire multiplier (Demonic Sacrifice of Succubus in Forever = +15% Fire)
    double f_mult = 1.0;
    if (p.sac_succubus) f_mult *= 1.15;
    if (p.pet == PetChoice::IMP && talents.demo.master_demonologist > 0) {
        f_mult *= (1.0 + talents.demo.master_demonologist * 0.02);
    }
    if (p.pet != PetChoice::NONE && talents.demo.soul_link > 0) {
        f_mult *= 1.03;
    }

    double snf_chance = (talents.destro.shadow_and_flame > 0 ? (talents.destro.shadow_and_flame * 0.20) : 0.0);
    double snf_bonus = (talents.destro.shadow_and_flame > 0 ? (talents.destro.shadow_and_flame * 0.02) : 0.0);
    double dot_crit = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
    double fnb_crit = (talents.destro.fire_and_brimstone == 1 ? 8.0 : (talents.destro.fire_and_brimstone == 2 ? 17.0 : (talents.destro.fire_and_brimstone == 3 ? 25.0 : 0.0)));
    double pet_mult = 1.0;
    if (p.pet == PetChoice::IMP) {
        pet_mult = (1.0 + talents.demo.improved_imp * 0.10) * (1.0 + talents.demo.unholy_power * 0.02);
    } else if (p.pet == PetChoice::SUCCUBUS) {
        pet_mult = (1.0 + talents.demo.unholy_power * 0.02);
    }
    double corr_mult = s_mult * (1.0 + shadow_mastery_bonus + malediction_bonus + improved_corruption_bonus);
    double brand_mult = (1.0 + talents.demo.unholy_power * 0.02) *
                        (1.0 + talents.demo.master_demonologist * 0.02) *
                        (talents.demo.soul_link > 0 ? 1.03 : 1.0);
    double pet_firebolt_mult = (1.0 + talents.demo.unholy_power * 0.02 + talents.demo.improved_imp * 0.10) *
                               (1.0 + talents.demo.master_demonologist * 0.02) *
                               (talents.demo.soul_link > 0 ? 1.03 : 1.0);
    double pet_melee_mult = (1.0 + talents.demo.unholy_power * 0.02) *
                            (talents.demo.soul_link > 0 ? 1.03 : 1.0);
    double pet_lash_mult = (1.0 + talents.demo.unholy_power * 0.02 + talents.demo.improved_sayaad * 0.10) *
                           (1.0 + talents.demo.master_demonologist * 0.02) *
                           (talents.demo.soul_link > 0 ? 1.03 : 1.0);

    std::cout << "  {\"id\":\"" << p.id << "\",\"name\":\"" << p.display_name << "\""
              << ",\"pet\":\"" << pet << "\""
              << ",\"sacImp\":" << (p.sac_imp ? "true" : "false")
              << ",\"sacSucc\":" << (p.sac_succubus ? "true" : "false")
              << ",\"rotation\":\"" << rot << "\""
              << ",\"spellPower\":" << sp
              << ",\"hit\":" << hit
              << ",\"crit\":" << crit
              << ",\"malevolence\":" << malevolence
              << ",\"afBonus\":" << af_bonus
              << ",\"aftermathBonus\":" << aftermath_bonus
              << ",\"resistance\":24.0"
              << ",\"maledictionBonus\":" << malediction_bonus
              << ",\"shadowMasteryBonus\":" << shadow_mastery_bonus
              << ",\"improvedCorruptionBonus\":" << improved_corruption_bonus
              << ",\"corruption\":" << (has_corr ? "true" : "false")
              << ",\"agony\":" << (has_agony ? "true" : "false")
              << ",\"immolate\":" << (has_imm ? "true" : "false")
              << ",\"conflagrate\":" << (conflag ? "true" : "false")
              << ",\"shadowburn\":" << (sb ? "true" : "false")
              << ",\"curseOfDoom\":" << (doom ? "true" : "false")
              << ",\"decimation\":" << (decimation ? "true" : "false")
              << ",\"demonicBrand\":" << (demonic_brand ? "true" : "false")
              << ",\"demonicBrandRank\":" << talents.demo.demonic_brand
              << ",\"baneRank\":" << talents.destro.bane
              << ",\"decimationRank\":" << talents.demo.decimation
              << ",\"brandMult\":" << brand_mult
              << ",\"petFireboltMult\":" << pet_firebolt_mult
              << ",\"petMeleeMult\":" << pet_melee_mult
              << ",\"petLashMult\":" << pet_lash_mult
              << ",\"demonicEnergies\":" << (talents.demo.demonic_energies * 1.0)
              << ",\"demonicKnowledge\":" << talents.demo.demonic_knowledge
              << ",\"siphonLife\":" << (has_siphon ? "true" : "false")
              << ",\"drainHope\":" << (has_drain_hope ? "true" : "false")
              << ",\"improvedDrainsBonus\":" << improved_drains_bonus
              << ",\"soulSiphonBonus\":" << soul_siphon_bonus
              << ",\"snfChance\":" << snf_chance
              << ",\"snfBonus\":" << snf_bonus
              << ",\"dotCrit\":" << dot_crit
              << ",\"fnbCrit\":" << fnb_crit
              << ",\"petMult\":" << pet_mult
              << ",\"corrMultiplier\":" << corr_mult
              << ",\"instantCorruption\":" << ((talents.aff.improved_corruption >= 5) ? "true" : "false")
              << ",\"nightfall\":" << ((talents.aff.nightfall > 0) ? "true" : "false")
              << ",\"isb\":" << ((talents.destro.improved_shadow_bolt > 0) ? "true" : "false")
              << ",\"isbBonus\":" << (talents.destro.improved_shadow_bolt * 0.04)
              << ",\"ruin\":" << ((talents.destro.ruin > 0) ? "true" : "false")
              << ",\"ruinRank\":" << talents.destro.ruin
              << ",\"improvedTap\":" << ((talents.aff.improved_life_tap > 0) ? "true" : "false")
              << ",\"tapBonus\":" << (talents.aff.improved_life_tap * 0.10)
              << ",\"nightfallChance\":" << (talents.aff.nightfall * 0.02)
              << ",\"masterDemo\":0"
              << ",\"shadowMultiplier\":" << s_mult
              << ",\"fireMultiplier\":" << f_mult
              << "}";
}

int main(int argc, char** argv) {
    std::cout << std::setprecision(17);

    if (argc >= 2 && std::string(argv[1]) == "--preset") {
        std::string pid = (argc >= 3) ? argv[2] : "ds_af";
        uint64_t s = (argc >= 4) ? std::stoull(argv[3]) : 42;
        double dur = (argc >= 5) ? std::stod(argv[4]) : 180.0;
        run_preset(pid, s, dur);
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--list-presets") {
        std::cout << "[\n";
        const auto& presets = standard_spec_presets();
        for (size_t i = 0; i < presets.size(); ++i) {
            output_preset_info_json(presets[i]);
            std::cout << (i + 1 < presets.size() ? "," : "") << "\n";
        }
        std::cout << "]\n";
        return 0;
    }

    // Interactive line-by-line stream mode
    std::string token;
    while (std::cin >> token) {
        if (token == "PRESET") {
            std::string pid;
            uint64_t s;
            double dur;
            std::cin >> pid >> s >> dur;
            run_preset(pid, s, dur);
            continue;
        }

        // Standard custom line format:
        // token is seed
        uint64_t seed = std::stoull(token);
        double duration, power, intellect, spirit, hit, crit, mp5, distance, resistance, penetration, threshold;
        std::string rotation, petChoice;
        int book, charges, partial, piercing, corruption, agony, immolate, instant, nightfall, isb, ruin, tap;
        int sacImp, sacSucc, masterDemo;
        double trinketSP, trinketDuration, trinketCD, shadowMult, fireMult;

        std::cin >> duration >> rotation >> power >> intellect >> spirit >> hit >> crit >> mp5
                 >> distance >> resistance >> penetration >> threshold >> book >> charges >> partial >> piercing
                 >> corruption >> agony >> immolate >> instant >> nightfall >> isb >> ruin >> tap
                 >> sacImp >> sacSucc >> masterDemo >> petChoice
                 >> trinketSP >> trinketDuration >> trinketCD >> shadowMult >> fireMult;

        WarlockSimulator sim;
        sim.race = Race::HUMAN;
        sim.fight_duration = duration;
        sim.record_timeline = true;
        sim.raw_stats = Stats{};
        sim.raw_stats.spell_power = power;
        sim.raw_stats.intellect = intellect;
        sim.raw_stats.spirit = spirit;
        sim.raw_stats.spell_hit_percent = hit;
        sim.raw_stats.spell_crit_percent = crit;
        sim.raw_stats.mp5 = mp5;
        sim.raw_stats.spell_penetration = penetration;
        sim.raw_stats.shadow_multiplier = shadowMult;
        sim.raw_stats.fire_multiplier = fireMult;
        if (masterDemo > 0) {
            if (petChoice == "succubus" || (petChoice == "none" && rotation != "fire" && rotation != "searing")) {
                sim.raw_stats.shadow_multiplier *= (1.0 + masterDemo * 0.02);
            }
            if (petChoice == "imp" || (petChoice == "none" && (rotation == "fire" || rotation == "searing"))) {
                sim.raw_stats.fire_multiplier *= (1.0 + masterDemo * 0.02);
            }
        }

        sim.buffs = BuffConfig{};
        sim.buffs.curse_of_shadows = false;
        sim.buffs.curse_of_elements = false;
        sim.buffs.sacrifice_imp = (sacImp != 0);
        sim.buffs.sacrifice_succubus = (sacSucc != 0);

        sim.talents = Talents{};
        sim.talents.destro.bane = 5;
        sim.talents.destro.ruin = ruin ? 5 : 0;
        sim.talents.destro.improved_shadow_bolt = isb ? 5 : 0;
        sim.talents.destro.incinerate = (rotation == "fire") ? 1 : 0;
        sim.talents.aff.improved_corruption = instant ? 5 : 0;
        sim.talents.aff.nightfall = nightfall ? 2 : 0;
        sim.talents.aff.improved_life_tap = tap ? 2 : 0;
        sim.talents.demo.demonic_sacrifice = (sacImp || sacSucc) ? 1 : 0;
        sim.talents.demo.master_demonologist = masterDemo;

        sim.mechanics.use_book_spell_ranks = book;
        sim.mechanics.isb_has_charges = (charges != 0);
        sim.mechanics.partial_resists_enabled = partial;
        sim.mechanics.spell_piercing_below_zero = piercing;
        sim.mechanics.default_boss_distance_yards = distance;
        sim.mechanics.pet_mana_management = true;
        sim.mechanics.pet_scaling = true;

        sim.target_config.base_shadow_resistance = sim.target_config.current_shadow_resistance = resistance;
        sim.target_config.base_fire_resistance = sim.target_config.current_fire_resistance = resistance;

        sim.policy = PolicyConfig{};
        sim.policy.pet = (petChoice == "imp") ? PetChoice::IMP : ((petChoice == "succubus") ? PetChoice::SUCCUBUS : PetChoice::NONE);

        if (trinketSP > 0.0) {
            sim.policy.use_trinkets_on_cooldown = true;
            Item tr;
            tr.has_on_use = true;
            tr.on_use_duration = trinketDuration;
            tr.on_use_cooldown = trinketCD;
            sim.gear.equip(Slot::TRINKET1, tr);
        } else {
            sim.policy.use_trinkets_on_cooldown = false;
        }

        sim.policy.life_tap_threshold_pct = threshold;
        sim.policy.use_custom_apl = true;
        auto rule = [&](PriorityAction action, SpellID id) {
            PriorityRule r; r.action = action; r.spell_id = id;
            sim.policy.custom_rules.push_back(r);
        };
        rule(PriorityAction::LIFE_TAP, SpellID::LIFE_TAP);
        rule(PriorityAction::NIGHTFALL_SHADOW_BOLT, SpellID::SHADOW_BOLT);
        if (rotation != "bolt") {
            if (immolate) rule(PriorityAction::IMMOLATE, SpellID::IMMOLATE);
            if (corruption) rule(PriorityAction::CORRUPTION, SpellID::CORRUPTION);
            if (agony) rule(PriorityAction::CURSE_OF_AGONY, SpellID::CURSE_OF_AGONY);
        }
        if (rotation == "fire") rule(PriorityAction::INCINERATE_FILLER, SpellID::INCINERATE);
        else if (rotation == "searing") rule(PriorityAction::SEARING_PAIN_FILLER, SpellID::SEARING_PAIN);
        else rule(PriorityAction::SHADOW_BOLT_FILLER, SpellID::SHADOW_BOLT);

        FastRNG rng(seed);
        const auto r = sim.run_single_simulation(rng);
        const auto next = rng.next_u64();
        output_sim_result_json(r, next, duration);
    }

    return 0;
}
