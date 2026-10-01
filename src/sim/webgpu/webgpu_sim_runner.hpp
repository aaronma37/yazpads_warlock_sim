#pragma once
#include "combat_types.hpp"
#include "native_gpu.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/parallel_runner.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace warlock {

enum class SimEngineType {
    CPU_DES = 0,
    WEBGPU_COMPUTE = 1
};

enum class WebGPUSchedulerMode {
    NEXT_EVENT = 0,
    FIXED_1MS = 1000,
    FIXED_10MS = 10000,
    FIXED_50MS = 50000
};

class WebGPUSimRunner {
public:
    static std::vector<BatchSimResult> run_batch_candidates(
        const std::vector<WarlockSimulator>& sims,
        int iterations,
        uint32_t step_us = 0,
        uint32_t seed = 0
    ) {
        if (sims.empty()) return {};
        if (iterations <= 0) iterations = 10000;
        if (iterations > 1000000) iterations = 1000000;

        std::vector<Config> configs;
        configs.reserve(sims.size());
        for (const auto& sim : sims) configs.push_back(build_webgpu_config(sim));

        std::vector<State> states;
        double elapsed_seconds = 0.0;
        std::string gpu_error;
        const bool gpu_used = run_native_webgpu(configs, static_cast<uint32_t>(iterations), seed, step_us,
                                                states, elapsed_seconds, gpu_error);
        if (!gpu_used) {
            states.resize(sims.size() * static_cast<size_t>(iterations));
            const auto cpu_start = std::chrono::high_resolution_clock::now();
            simulate_batch(configs.data(), static_cast<uint32_t>(configs.size()),
                           static_cast<uint32_t>(iterations), seed, step_us, states.data());
            elapsed_seconds = std::chrono::duration<double>(
                std::chrono::high_resolution_clock::now() - cpu_start).count();
        }

        std::vector<BatchSimResult> results;
        results.reserve(sims.size());
        const double per_candidate_seconds = elapsed_seconds / static_cast<double>(sims.size());
        for (size_t i = 0; i < sims.size(); ++i) {
            const auto begin = states.begin() + static_cast<std::ptrdiff_t>(i * iterations);
            const std::vector<State> candidate_states(begin, begin + iterations);
            auto result = process_states_to_result(configs[i], candidate_states, per_candidate_seconds);
            result.gpu_used = gpu_used;
            results.push_back(std::move(result));
        }
        return results;
    }

    static Config build_webgpu_config(const WarlockSimulator& sim) {
        Stats player_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
        BaseAttributes base_attrs = get_base_attributes_for_race(sim.race);
        // Match run_single_simulation: sacrifice buffs are inactive unless the
        // character has Demonic Sacrifice or Demonic Pact.
        BuffConfig active_buffs = sim.buffs;
        if (sim.talents.demo.demonic_sacrifice == 0 && sim.talents.demo.demonic_pact == 0) {
            active_buffs.sacrifice_imp = false;
            active_buffs.sacrifice_succubus = false;
        }
        active_buffs.apply_to_stats(player_stats, base_attrs, true, sim.mechanics.personal_shadow_weaving);

        PetChoice active_pet = sim.policy.pet;
        if (active_buffs.sacrifice_imp || active_buffs.sacrifice_succubus) {
            if (sim.talents.demo.demonic_pact > 0) {
                if ((active_buffs.sacrifice_imp && sim.policy.pet == PetChoice::IMP) ||
                    (active_buffs.sacrifice_succubus && sim.policy.pet == PetChoice::SUCCUBUS)) {
                    active_pet = PetChoice::NONE;
                }
            } else {
                active_pet = PetChoice::NONE;
            }
        }

        // Demonic Knowledge is applied to player spell power in the CPU model;
        // pet spell inheritance also receives its separate flat pet-SP bonus below.
        if (active_pet != PetChoice::NONE && sim.talents.demo.demonic_knowledge > 0) {
            player_stats.spell_power += 20.0 * sim.talents.demo.demonic_knowledge;
        }

        Config c{};
        c.duration = static_cast<float>(sim.fight_duration);

        // Gnome Expansive Mind (+5% Mana)
        if (sim.race == Race::GNOME) {
            player_stats.max_mana *= 1.05;
        }

        // Fel Vitality (+5% Max Mana per point)
        if (sim.talents.demo.fel_vitality > 0) {
            player_stats.max_mana *= (1.0 + sim.talents.demo.fel_vitality * 0.05);
        }

        // Consumables: Mana Potions (+1800 mana / 120s = 75 MP5) and Demonic Runes (+1200 mana / 120s = 50 MP5)
        float pot_mp5 = 0.0f;
        if (sim.buffs.use_mana_potions) pot_mp5 += 75.0f;
        if (sim.buffs.use_demonic_runes) pot_mp5 += 50.0f;
        c.mp5 = static_cast<float>(player_stats.mp5) + pot_mp5;

        // Trinket SP is applied while its modeled on-use window is active,
        // matching the CPU simulator's event-time spell-power lookup.
        const Item* sp_trinket = nullptr;
        if (sim.policy.use_trinkets_on_cooldown) {
            const Item& t1 = sim.gear.get(Slot::TRINKET1);
            const Item& t2 = sim.gear.get(Slot::TRINKET2);
            if (t1.has_on_use && t1.on_use_cooldown > 0.0) {
                sp_trinket = &t1;
            } else if (t2.has_on_use && t2.on_use_cooldown > 0.0) {
                sp_trinket = &t2;
            }
        }
        // Dynamic Racial configuration
        if (sim.race == Race::ORC) {
            c.racialType = 1.0f; // ORC_BLOOD_FURY (+10% base SP for 15s)
            c.racialPolicy = static_cast<float>(sim.policy.racial_policy);
            c.racialDuration = 15.0f;
            c.racialCooldown = 120.0f;
            c.racialBonus = static_cast<float>(player_stats.spell_power * 0.10);
        } else if (sim.race == Race::TROLL) {
            c.racialType = 2.0f; // TROLL_BERSERKING (+10% Haste for 10s on 180s CD)
            c.racialPolicy = static_cast<float>(sim.policy.racial_policy);
            c.racialDuration = 10.0f;
            c.racialCooldown = 180.0f;
            c.racialBonus = 10.0f; // +10% Haste
            c.racialPad0 = (sim.target_config.is_beast || sim.target_config.creature_type == CreatureType::BEAST) ? 1.05f : 1.0f;
        } else if (sim.race == Race::GNOME) {
            c.racialType = 3.0f; // GNOME_EUREKA (3 charges, -10% mana, +10% dmg on 120s CD)
            c.racialPolicy = static_cast<float>(sim.policy.racial_policy);
            c.racialDuration = 3.0f; // 3 charges
            c.racialCooldown = 120.0f;
            c.racialBonus = 0.10f; // +10% damage
        }

        c.maxMana = static_cast<float>(player_stats.max_mana);
        c.spellPower = static_cast<float>(player_stats.effective_shadow_power());
        c.shadowPower = static_cast<float>(player_stats.effective_shadow_power());
        c.firePower = static_cast<float>(player_stats.effective_fire_power());
        if (sp_trinket) {
            c.trinketBonus = 175.0f;
            c.trinketDuration = static_cast<float>(sp_trinket->on_use_duration);
            c.trinketCooldown = static_cast<float>(sp_trinket->on_use_cooldown);
            c.trinketEnabled = 1.0f;
        }

        // Base cast times without static racial haste approximation
        float base_cast = (sim.talents.destro.bane > 0) ? std::max(1.0f, 3.0f - sim.talents.destro.bane * 0.1f) : 3.0f;
        float haste_mult = 1.0f + static_cast<float>(player_stats.spell_haste_percent) / 100.0f;
        c.castTime = base_cast / haste_mult;
        c.gcd = static_cast<float>(sim.mechanics.base_gcd);


        // Match WarlockSimulator::calculate_hit_chance for the configured target level.
        double base_hit = sim.mechanics.base_hit_vs_boss;
        const int level_delta = sim.target_config.level - 60;
        if (level_delta <= 0) base_hit = 0.96 + std::min(0.03, -level_delta * 0.01);
        else if (level_delta == 1) base_hit = 0.95;
        else if (level_delta == 2) base_hit = 0.94;
        else if (level_delta > 3) base_hit = std::max(0.01, sim.mechanics.base_hit_vs_boss - (level_delta - 3) * 0.11);
        c.hit = static_cast<float>(std::min(sim.mechanics.max_spell_hit,
            base_hit + player_stats.spell_hit_percent * 0.01 + sim.talents.aff.suppression * 0.01));

        const Item& main_hand = sim.gear.get(Slot::MAIN_HAND);
        const bool sword = sim.use_raw_stats || main_hand.name.find("Mageblade") != std::string::npos ||
            main_hand.name.find("Sword") != std::string::npos || main_hand.name.find("Blade") != std::string::npos ||
            main_hand.icon.find("sword") != std::string::npos || main_hand.icon.find("Sword") != std::string::npos;
        const bool axe = !sim.use_raw_stats && (main_hand.name.find("Axe") != std::string::npos ||
            main_hand.icon.find("axe") != std::string::npos);
        auto spell_crit = [&](School school) {
            double crit = player_stats.total_spell_crit(base_attrs.base_spell_crit);
            if (sim.race == Race::HUMAN && sword) crit += 2.0;
            if (sim.race == Race::ORC && axe) crit += 1.0;
            if (school == School::SHADOW) crit += sim.talents.aff.malevolence;
            return static_cast<float>(std::clamp(crit * 0.01, 0.0, 1.0));
        };
        c.crit = spell_crit(School::SHADOW);
        c.fireCrit = spell_crit(School::FIRE);

        // Match the CPU's separate destruction and Pandemic critical multipliers.
        c.boltCrit = static_cast<float>(1.0 + 0.50 * (1.0 + sim.talents.destro.ruin * 0.20));
        c.dotCrit = static_cast<float>(1.0 + 0.50 * (1.0 + sim.talents.aff.pandemic * 0.33333333));

        // Priority Rules & APL Translation:
        const auto rotation_rules = sim.policy.get_priority_rules(sim.talents, sim.race);
        auto dot_refresh_window = [&](PriorityAction action) {
            const auto it = std::find_if(rotation_rules.begin(), rotation_rules.end(), [action](const PriorityRule& rule) {
                return rule.enabled && rule.action == action && rule.use_custom_thresholds && rule.check_dot_refresh;
            });
            return it == rotation_rules.end() ? 0.0f : it->max_dot_rem_sec;
        };

        bool has_corr_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::CORRUPTION; });
        bool has_agony_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::CURSE_OF_AGONY; });
        bool has_doom_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::CURSE_OF_DOOM; });
        bool has_siphon_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::SIPHON_LIFE; });
        bool has_immolate_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::IMMOLATE; });
        bool has_nightfall_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::NIGHTFALL_SHADOW_BOLT; });
        bool has_demonic_brand_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::DEMONIC_BRAND_SEARING_PAIN; });
        bool has_wrack_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::WRACK; });
        bool has_incinerate_filler_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::INCINERATE_FILLER; });
        bool has_searing_filler_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::SEARING_PAIN_FILLER; });
        bool has_decimation_searing_rule = std::any_of(rotation_rules.begin(), rotation_rules.end(),
            [](const PriorityRule& r) { return r.enabled && r.action == PriorityAction::DECIMATION_SEARING_PAIN; });

        // Match WarlockSimulator::run_single_simulation: Rank 6 Life Tap costs
        // 430 health and returns 430 + Spirit mana, modified by Improved Life Tap.
        float tap_talent_mult = 1.0f + static_cast<float>(sim.talents.aff.improved_life_tap) * 0.10f;
        c.tapGain = (430.0f + static_cast<float>(player_stats.spirit)) * tap_talent_mult;

        float tap_threshold = static_cast<float>(sim.policy.life_tap_threshold_pct / 100.0);
        for (const auto& r : rotation_rules) {
            if (r.enabled && r.action == PriorityAction::LIFE_TAP) {
                if (r.use_custom_thresholds && r.check_mana) {
                    tap_threshold = r.max_mana_pct;
                }
                break;
            }
        }
        if (tap_threshold > 1.0f || tap_threshold <= 0.0f) tap_threshold = 0.30f;
        c.tapThreshold = tap_threshold;

        c.corruptionCastTime = std::max(0.0f, 2.0f - 0.4f * static_cast<float>(sim.talents.aff.improved_corruption)) / haste_mult;
        c.nightfall = (sim.mechanics.nightfall_enabled && sim.talents.aff.nightfall > 0 && has_nightfall_rule)
            ? (static_cast<float>(sim.talents.aff.nightfall) * 0.02f) : 0.0f;

        // Mana costs (with Cataclysm reduction if present)
        const int cataclysm_rank = sim.talents.destro.cataclysm;
        float cata_cost_mult = cataclysm_rank == 1 ? 0.97f :
            (cataclysm_rank == 2 ? 0.94f : (cataclysm_rank == 3 ? 0.90f : 1.0f));
        c.boltCost = (sim.mechanics.use_book_spell_ranks ? 380.0f : 370.0f) * cata_cost_mult;
        c.corruptionCost = has_corr_rule ? ((sim.mechanics.use_book_spell_ranks ? 340.0f : 290.0f) * cata_cost_mult) : 0.0f;
        c.agonyCost = has_agony_rule ? (215.0f * cata_cost_mult) : 0.0f;
        c.doomCost = has_doom_rule ? (300.0f * cata_cost_mult) : 0.0f;
        c.siphonCost = (has_siphon_rule && sim.talents.aff.siphon_life > 0) ? (365.0f * cata_cost_mult) : 0.0f;
        c.immolateCost = has_immolate_rule ?
            ((sim.mechanics.use_book_spell_ranks ? 380.0f : 370.0f) * cata_cost_mult) : 0.0f;
        c.conflagCost = (sim.talents.destro.conflagrate > 0) ? (265.0f * cata_cost_mult) : 0.0f;
        c.shadowburnCost = (sim.talents.destro.shadowburn > 0) ? (365.0f * cata_cost_mult) : 0.0f;

        // Shadow Bolt base damage: 246-274 (253-283 with AQ book)
        c.boltMin = sim.mechanics.use_book_spell_ranks ? 253.0f : 246.0f;
        c.boltMax = sim.mechanics.use_book_spell_ranks ? 283.0f : 274.0f;

        // Corruption base damage per tick (57 standard, 73 with AQ book)
        c.dotBase = sim.mechanics.use_book_spell_ranks ? 73.0f : 57.0f;

        bool sac_imp = active_buffs.sacrifice_imp && (active_pet != PetChoice::IMP);
        bool sac_succ = active_buffs.sacrifice_succubus && (active_pet != PetChoice::SUCCUBUS);

        // All-damage multipliers (Soul Link: +3%)
        float all_damage_mult = static_cast<float>(player_stats.all_damage_multiplier);
        if (active_pet != PetChoice::NONE && sim.talents.demo.soul_link > 0) {
            all_damage_mult *= 1.03f;
        }

        // Multipliers from stats (buffs, consumables, world buffs, sacrifice) + talents
        float shadow_mult_base = static_cast<float>(player_stats.shadow_multiplier) * all_damage_mult;
        if (active_pet == PetChoice::SUCCUBUS && sim.talents.demo.master_demonologist > 0) {
            shadow_mult_base *= (1.0f + static_cast<float>(sim.talents.demo.master_demonologist) * 0.02f);
        }
        const float shadow_mastery_bonus = static_cast<float>(sim.talents.aff.shadow_mastery) * 0.01f;
        const float dot_mult = shadow_mult_base * (1.0f + shadow_mastery_bonus);
        c.dotMultiplier = dot_mult;
        const float agonizing_flames_bonus = sim.talents.destro.agonizing_flames == 1 ? 0.03f :
            (sim.talents.destro.agonizing_flames == 2 ? 0.07f :
            (sim.talents.destro.agonizing_flames == 3 ? 0.10f : 0.0f));
        c.shadowBoltMultiplier = shadow_mult_base *
            (1.0f + shadow_mastery_bonus + agonizing_flames_bonus);

        c.ampCurse = sim.talents.aff.amplify_curse > 0 ? 1.0f : 0.0f;

        // Improved Shadow Bolt (ISB)
        if (sim.talents.destro.improved_shadow_bolt > 0) {
            c.isbBonus = static_cast<float>(sim.talents.destro.improved_shadow_bolt) * 0.04f;
            c.isbCharges = sim.mechanics.isb_has_charges ? 4.0f : 0.0f;
            c.isbAllShadow = (!sim.mechanics.isb_has_charges || sim.mechanics.isb_all_shadow_sources) ? 1.0f : 0.0f;
        } else {
            c.isbBonus = 0.0f;
            c.isbCharges = 0.0f;
            c.isbAllShadow = 0.0f;
        }

        // Curse of Agony / Bane of Agony
        c.agonyBase = 552.0f / 12.0f;
        const float malediction_bonus = static_cast<float>(sim.talents.aff.malediction) * 0.01f;
        c.agonyMultiplier = shadow_mult_base * (1.0f + shadow_mastery_bonus + malediction_bonus + static_cast<float>(sim.talents.aff.improved_bane_of_agony) * 0.05f);

        // Curse of Doom / Bane of Doom
        c.doomBase = 1742.0f;
        c.doomMultiplier = shadow_mult_base * (1.0f + shadow_mastery_bonus + malediction_bonus);

        // Siphon Life
        c.siphonBase = 41.0f;
        c.siphonMultiplier = shadow_mult_base * (1.0f + shadow_mastery_bonus + malediction_bonus);
        c.corruptionMultiplier = shadow_mult_base * (1.0f + shadow_mastery_bonus + malediction_bonus + static_cast<float>(sim.talents.aff.improved_corruption) * 0.02f);
        c.corruptionSpCoeff = static_cast<float>(sim.mechanics.corruption_sp_coefficient / 6.0);
        const float improved_drains_bonus = sim.talents.aff.improved_drains == 1 ? 0.07f :
            (sim.talents.aff.improved_drains == 2 ? 0.13f : (sim.talents.aff.improved_drains >= 3 ? 0.20f : 0.0f));
        c.wrackCost = (has_wrack_rule && sim.talents.aff.drain_hope > 0) ? 240.0f : 0.0f;
        c.wrackBase = 216.0f / 6.0f;
        c.wrackSpCoeff = 0.858f / 6.0f;
        c.wrackMultiplier = shadow_mult_base * (1.0f + shadow_mastery_bonus + malediction_bonus + improved_drains_bonus);
        c.wrackSoulSiphonPerEffect = static_cast<float>(sim.talents.aff.soul_siphon) * 0.04f;
        c.wrackChannelTime = 6.0f / haste_mult;
        c.wrackTickInterval = 1.0f / haste_mult;

        // Immolate
        float imm_cast = (sim.talents.destro.bane > 0) ? std::max(1.0f, 2.0f - sim.talents.destro.bane * 0.1f) : 2.0f;
        c.immolateCastTime = imm_cast / haste_mult;
        float fire_mult = static_cast<float>(player_stats.fire_multiplier) * all_damage_mult;
        if (active_pet == PetChoice::IMP && sim.talents.demo.master_demonologist > 0) {
            fire_mult *= (1.0f + static_cast<float>(sim.talents.demo.master_demonologist) * 0.02f);
        }
        c.immolateMultiplier = fire_mult * (1.0f + agonizing_flames_bonus + malediction_bonus);
        c.immolateDirectMultiplier = fire_mult *
            (1.0f + agonizing_flames_bonus + static_cast<float>(sim.talents.destro.aftermath) * 0.10f);
        // Match the rank selected by the CPU simulator. Standard Immolate is
        // 157.5 direct / 52 per tick; the book rank is 158 / 55.
        c.immolateMin = sim.mechanics.use_book_spell_ranks ? 158.0f : 157.5f;
        c.immolateMax = c.immolateMin;
        c.immolateDotBase = sim.mechanics.use_book_spell_ranks ? 55.0f : 52.0f;

        // Conflagrate & Shadowburn
        c.conflagMin = 306.0f;
        c.conflagMax = 374.0f;
        c.conflagMultiplier = fire_mult * (1.0f + agonizing_flames_bonus);
        c.fnbCritBonus = static_cast<float>(sim.talents.destro.fire_and_brimstone == 1 ? 0.08 : (sim.talents.destro.fire_and_brimstone == 2 ? 0.17 : (sim.talents.destro.fire_and_brimstone == 3 ? 0.25 : 0.0)));
        c.sburnMin = 259.0f;
        c.sburnMax = 289.0f;
        c.shadowburnMultiplier = shadow_mult_base * (1.0f + shadow_mastery_bonus + agonizing_flames_bonus);
        c.snfShadowBonus = static_cast<float>(sim.talents.destro.shadow_and_flame) * 0.02f;
        c.snfFireBonus = static_cast<float>(sim.talents.destro.shadow_and_flame) * 0.02f;
        c.snfConflagSaveChance = static_cast<float>(sim.talents.destro.shadow_and_flame) * 0.20f;

        // Pets & Pet Scaling
        c.petType = 0.0f;
        c.petCastInterval = 2.0f;
        c.petBaseMin = 0.0f;
        c.petBaseMax = 0.0f;
        c.petSpRatio = 0.0f;
        c.petMultiplier = 1.0f;
        c.petLopBase = 0.0f;
        c.petLopSpRatio = 0.0f;
        c.petLopCd = 12.0f;
        c.petMeleeBase = 0.0f;
        c.petApRatio = 0.0f;
        c.petSpellHit = c.hit;
        c.petSpellCrit = c.crit;
        c.petMeleeMissPct = 8.0f;
        c.petMeleeDodgePct = 6.5f;
        c.petGlancePct = 40.0f;
        c.petGlanceMultiplier = 0.65f;
        c.petArmorMultiplier = 1.0f;
        c.petMeleeMultiplier = 1.0f;
        c.petLopMultiplier = 1.0f;
        c.petLopCost = static_cast<float>(sim.mechanics.succubus_lop_cost);
        c.petManaManagement = sim.mechanics.pet_mana_management ? 1.0f : 0.0f;
        c.petManaMax = static_cast<float>(sim.mechanics.succubus_base_mana * (1.0 + 0.05 * sim.talents.demo.fel_vitality));
        c.petMp5 = static_cast<float>(sim.mechanics.pet_base_mp5 + (sim.buffs.blessing_of_wisdom ? 30.0 : 0.0) + (sim.buffs.warchiefs_blessing ? 10.0 : 0.0));
        c.petFlatSP = static_cast<float>(20.0 * sim.talents.demo.demonic_knowledge * (1.5 / 3.5));
        c.petMeleeCritPct = 0.0f;
        c.petSpellPiercing = sim.mechanics.spell_piercing_below_zero ? 1.0f : 0.0f;
        c.petPiercingBonus = static_cast<float>(sim.mechanics.spell_piercing_bonus_per_point);
        c.targetResist = static_cast<float>(sim.target_config.current_shadow_resistance);
        if (sim.buffs.curse_of_shadows || sim.buffs.curse_of_elements) {
            c.targetResist = static_cast<float>(std::max(0.0, sim.target_config.base_shadow_resistance - 75.0));
        }
        c.spellPen = static_cast<float>(player_stats.spell_penetration);
        c.partialResistEnabled = sim.mechanics.partial_resists_enabled ? 1.0f : 0.0f;
        c.executeBonus = static_cast<float>(sim.talents.demo.decimation) * 0.03f;
        c.soulFireCost = 335.0f * cata_cost_mult;
        c.soulFireCastTime = std::max(0.5f, (6.0f - 0.4f * sim.talents.destro.bane) *
            (1.0f - 0.20f * sim.talents.demo.decimation) / haste_mult);
        c.soulFireCooldown = 60.0f * (1.0f - 0.45f * sim.talents.demo.decimation);
        c.soulFireMultiplier = fire_mult * (1.0f + agonizing_flames_bonus);
        c.searingCost = (has_searing_filler_rule || has_decimation_searing_rule || has_demonic_brand_rule)
            ? 168.0f * cata_cost_mult : 0.0f;
        c.searingCastTime = std::max(1.0f, 1.5f / haste_mult);
        c.searingMultiplier = fire_mult * (1.0f + agonizing_flames_bonus);
        c.searingCritBonus = agonizing_flames_bonus;
        c.demonicBrandRank = static_cast<float>(sim.talents.demo.demonic_brand);
        c.demonicBrandMultiplier = (1.0f + static_cast<float>(sim.talents.demo.unholy_power) * 0.02f) *
            (1.0f + static_cast<float>(sim.talents.demo.master_demonologist) * 0.02f);
        if (sim.talents.demo.soul_link > 0) c.demonicBrandMultiplier *= 1.03f;
        if (sim.buffs.curse_of_elements) c.demonicBrandMultiplier *= 1.10f;
        c.agonyRefreshSec = dot_refresh_window(PriorityAction::CURSE_OF_AGONY);
        c.corruptionRefreshSec = dot_refresh_window(PriorityAction::CORRUPTION);
        c.siphonRefreshSec = dot_refresh_window(PriorityAction::SIPHON_LIFE);
        c.immolateRefreshSec = dot_refresh_window(PriorityAction::IMMOLATE);
        c.doomMinTimeRemaining = 60.0f;
        for (const auto& rule : rotation_rules) {
            if (rule.enabled && rule.action == PriorityAction::CURSE_OF_DOOM) {
                if (rule.use_custom_thresholds && rule.check_fight_time) {
                    c.doomMinTimeRemaining = rule.min_time_remaining;
                }
                break;
            }
        }
        c.touchOfTheGraveDamage = (sim.race == Race::UNDEAD) ? static_cast<float>(0.05 * player_stats.max_health) : 0.0f;
        c.petTapGain = (active_pet != PetChoice::NONE && sim.talents.demo.demonic_energies > 0) ? (c.tapGain * 0.50f * static_cast<float>(sim.talents.demo.demonic_energies)) : 0.0f;

        if (active_pet == PetChoice::IMP) {
            c.petType = 1.0f;
            c.petManaMax = static_cast<float>(sim.mechanics.imp_base_mana * (1.0 + 0.05 * sim.talents.demo.fel_vitality));
            c.petLopCost = static_cast<float>(sim.mechanics.imp_firebolt_cost);
            c.petCastInterval = sim.mechanics.imp_firebolt_modern_scaling ? 2.0f : 1.5f;
            if (sim.mechanics.imp_firebolt_modern_scaling) {
                c.petBaseMin = 44.0f;
                c.petBaseMax = 44.0f;
                c.petSpRatio = sim.mechanics.pet_scaling ? (static_cast<float>(sim.mechanics.pet_sp_ratio) * (2.0f / 3.5f)) : 0.0f;
                c.petFlatSP = static_cast<float>(20.0 * sim.talents.demo.demonic_knowledge * (2.0 / 3.5));
            } else {
                c.petBaseMin = 85.0f;
                c.petBaseMax = 98.0f;
                c.petSpRatio = sim.mechanics.pet_scaling ? (static_cast<float>(sim.mechanics.pet_sp_ratio) * (1.5f / 3.5f)) : 0.0f;
                c.petFlatSP = static_cast<float>(20.0 * sim.talents.demo.demonic_knowledge * (1.5 / 3.5));
            }
            float imp_mult = (1.0f + static_cast<float>(sim.talents.demo.unholy_power) * 0.02f + static_cast<float>(sim.talents.demo.improved_imp) * 0.10f);
            imp_mult *= (1.0f + static_cast<float>(sim.talents.demo.master_demonologist) * 0.02f);
            if (sim.talents.demo.soul_link > 0) imp_mult *= 1.03f;
            if (sim.buffs.curse_of_elements) imp_mult *= 1.10f;
            c.petMultiplier = imp_mult;
        } else if (active_pet == PetChoice::SUCCUBUS) {
            c.petType = 2.0f;
            c.petCastInterval = 2.0f; // Melee swing every 2s
            c.petMeleeBase = 101.0f;
            c.petApRatio = sim.mechanics.pet_scaling ? (static_cast<float>(sim.mechanics.pet_ap_ratio) * 2.0f / 14.0f) : 0.0f;
            c.petLopBase = 50.0f;
            c.petLopSpRatio = sim.mechanics.pet_scaling ? (static_cast<float>(sim.mechanics.pet_sp_ratio) * (1.5f / 3.5f)) : 0.0f;
            c.petLopCd = 12.0f;
            c.petSpellHit = c.hit;
            c.petSpellCrit = c.crit;
            c.petMeleeMissPct = static_cast<float>(std::max(0.0, 8.0 - std::max(0.0, player_stats.spell_hit_percent - 84.0)));
            c.petMeleeCritPct = static_cast<float>(std::max(0.0, 7.52 + player_stats.total_spell_crit(base_attrs.base_spell_crit) - 4.8));

            const double armor = std::max(0.0, sim.target_config.boss_armor
                - (sim.target_config.sunder_armor ? 2250.0 : 0.0)
                - (sim.target_config.faerie_fire ? 505.0 : 0.0));
            c.petArmorMultiplier = static_cast<float>(1.0 - armor / (armor + 400.0 + 85.0 * 60.0));
            c.petMeleeMultiplier = 1.0f + static_cast<float>(sim.talents.demo.unholy_power) * 0.02f;
            if (sim.talents.demo.soul_link > 0) c.petMeleeMultiplier *= 1.03f;

            float lop_mult = 1.0f + static_cast<float>(sim.talents.demo.unholy_power) * 0.02f + static_cast<float>(sim.talents.demo.improved_sayaad) * 0.10f;
            lop_mult *= (1.0f + static_cast<float>(sim.talents.demo.master_demonologist) * 0.02f);
            if (sim.talents.demo.soul_link > 0) lop_mult *= 1.03f;
            if (sim.buffs.shadow_weaving && !sim.mechanics.personal_shadow_weaving) lop_mult *= 1.15f;
            if (sim.buffs.curse_of_shadows || sim.buffs.curse_of_elements) lop_mult *= 1.10f;
            c.petLopMultiplier = lop_mult;
            c.petMultiplier = lop_mult;
        }

        // Action Priority List rule ingestion
        float* rule_ptrs[16] = {
            &c.rule0, &c.rule1, &c.rule2, &c.rule3,
            &c.rule4, &c.rule5, &c.rule6, &c.rule7,
            &c.rule8, &c.rule9, &c.rule10, &c.rule11,
            &c.rule12, &c.rule13, &c.rule14, &c.rule15
        };
        for (int i = 0; i < 16; ++i) *rule_ptrs[i] = 255.0f;

        size_t r_count = 0;
        for (const auto& r : rotation_rules) {
            if (!r.enabled) continue;
            if (r_count >= 16) break;
            *rule_ptrs[r_count++] = static_cast<float>(static_cast<uint8_t>(r.action));
        }
        c.ruleCount = static_cast<float>(r_count);

        // Filler spell configuration
        const auto rotation = sim.policy.rotation;
        const bool fire_filler_rotation =
            has_incinerate_filler_rule ||
            rotation == RotationChoice::FIRE_DESTRO ||
            rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION ||
            rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2 ||
            rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE;
        if (fire_filler_rotation && sim.talents.destro.incinerate > 0) {
            c.fillerType = 1.0f;
            c.castTime = std::max(1.0f, 2.5f - 0.1f * sim.talents.destro.bane) / haste_mult;
            c.boltCost = 325.0f * cata_cost_mult;
            c.boltMin = 201.0f;
            c.boltMax = 233.0f;
            c.shadowBoltMultiplier = fire_mult * (1.0f + agonizing_flames_bonus);
        } else {
            c.fillerType = has_searing_filler_rule ? 2.0f : 0.0f;
            if (has_searing_filler_rule) c.castTime = c.searingCastTime;
        }

        return c;
    }

    static BatchSimResult process_states_to_result(const Config& config, const std::vector<State>& states, double elapsed_seconds) {
        BatchSimResult result;
        int count = static_cast<int>(states.size());
        if (count <= 0) return result;

        result.total_iterations = count;
        result.total_sim_time_seconds = elapsed_seconds;
        result.iterations_per_second = (elapsed_seconds > 0.0) ? (count / elapsed_seconds) : 0.0;

        std::vector<double> dps_list;
        dps_list.reserve(count);

        double sum_dps = 0.0;
        double sum_dps_sq = 0.0;
        double sum_bolt_dmg = 0.0;
        double sum_dot_dmg = 0.0;
        double sum_agony_dmg = 0.0;
        double sum_doom_dmg = 0.0;
        double sum_siphon_dmg = 0.0;
        double sum_immolate_dmg = 0.0;
        double sum_incinerate_dmg = 0.0;
        double sum_conflag_dmg = 0.0;
        double sum_sburn_dmg = 0.0;
        double sum_soul_fire_dmg = 0.0;
        double sum_wrack_dmg = 0.0;
        double sum_pet_dmg = 0.0;

        double sum_bolts = 0.0;
        double sum_dots = 0.0;
        double sum_ticks = 0.0;
        double sum_taps = 0.0;
        double sum_procs = 0.0;
        double sum_misses = 0.0;
        double sum_crits = 0.0;
        double sum_corruption_crits = 0.0;
        double sum_corruption_ticks = 0.0;
        double sum_agonies = 0.0;
        double sum_dooms = 0.0;
        double sum_doom_hits = 0.0;
        double sum_doom_crits = 0.0;
        double sum_wracks = 0.0;
        double sum_wrack_hits = 0.0;
        double sum_wrack_crits = 0.0;
        double sum_siphons = 0.0;
        double sum_immolates = 0.0;
        double sum_immolate_direct_hits = 0.0;
        double sum_immolate_dot_hits = 0.0;
        double sum_immolate_misses = 0.0;
        double sum_conflagrates = 0.0;
        double sum_shadowburns = 0.0;
        double sum_soul_fires = 0.0, sum_soul_fire_hits = 0.0, sum_soul_fire_misses = 0.0, sum_soul_fire_crits = 0.0;
        double sum_incinerates = 0.0, sum_incinerate_hits = 0.0;
        double sum_incinerate_misses = 0.0, sum_incinerate_crits = 0.0;
        double sum_searing_damage = 0.0, sum_searing_casts = 0.0, sum_searing_hits = 0.0, sum_searing_misses = 0.0, sum_searing_crits = 0.0;
        double sum_agony_hits = 0.0, sum_agony_crits = 0.0;
        double sum_siphon_hits = 0.0, sum_siphon_crits = 0.0;
        double sum_totg_procs = 0.0, sum_totg_damage = 0.0;
        double sum_pet_casts = 0.0, sum_pet_hits = 0.0, sum_pet_crits = 0.0;
        double sum_demonic_brand_damage = 0.0;
        double sum_pet_melee_casts = 0.0, sum_pet_melee_hits = 0.0, sum_pet_melee_damage = 0.0;
        double sum_pet_lop_casts = 0.0, sum_pet_lop_hits = 0.0, sum_pet_lop_damage = 0.0;
        double sum_pet_melee_crits = 0.0, sum_pet_lop_crits = 0.0;
        double sum_isb_uptime_us = 0.0;

        for (int i = 0; i < count; ++i) {
            const auto& s = states[i];
            double dmg = static_cast<double>(s.boltDamage) + static_cast<double>(s.dotDamage) +
                         static_cast<double>(s.agonyDamage) + static_cast<double>(s.doomDamage) +
                         static_cast<double>(s.siphonDamage) + static_cast<double>(s.immolateDamage) +
                         static_cast<double>(s.incinerateDamage) +
                         static_cast<double>(s.conflagrateDamage) +
                         static_cast<double>(s.shadowburnDamage) +
                         static_cast<double>(s.soulFireDamage) +
                         static_cast<double>(s.searingDamage) +
                         static_cast<double>(s.wrackDamage) +
                         static_cast<double>(s.petDamage) +
                         static_cast<double>(s.totgDamage);
            double dps = dmg / static_cast<double>(config.duration);
            dps_list.push_back(dps);

            sum_dps += dps;
            sum_dps_sq += dps * dps;
            sum_bolt_dmg += s.boltDamage;
            sum_dot_dmg += s.dotDamage;
            sum_agony_dmg += s.agonyDamage;
            sum_doom_dmg += s.doomDamage;
            sum_siphon_dmg += s.siphonDamage;
            sum_immolate_dmg += s.immolateDamage;
            sum_incinerate_dmg += s.incinerateDamage;
            sum_conflag_dmg += s.conflagrateDamage;
            sum_sburn_dmg += s.shadowburnDamage;
            sum_soul_fire_dmg += s.soulFireDamage;
            sum_wrack_dmg += s.wrackDamage;
            sum_searing_damage += s.searingDamage;
            sum_pet_dmg += s.petDamage;
            sum_demonic_brand_damage += s.demonicBrandDamage;
            sum_totg_damage += s.totgDamage;
            sum_totg_procs += s.totgProcs;

            sum_bolts += s.bolts;
            sum_dots += s.dots;
            sum_ticks += s.ticks;
            sum_taps += s.taps;
            sum_procs += s.procs;
            sum_misses += s.misses;
            sum_crits += s.crits;
            sum_corruption_crits += s.corruptionCrits;
            sum_corruption_ticks += s.corruptionTicks;
            sum_agonies += s.agonies;
            sum_agony_hits += s.agonyHits;
            sum_agony_crits += s.agonyCrits;
            sum_dooms += s.dooms;
            sum_doom_hits += s.doomHits;
            sum_doom_crits += s.doomCrits;
            sum_wracks += s.wracks;
            sum_wrack_hits += s.wrackHits;
            sum_wrack_crits += s.wrackCrits;
            sum_siphons += s.siphons;
            sum_siphon_hits += s.siphonHits;
            sum_siphon_crits += s.siphonCrits;
            sum_immolates += s.immolates;
            sum_immolate_direct_hits += s.immolateDirectHits;
            sum_immolate_dot_hits += s.immolateDotHits;
            sum_immolate_misses += s.immolateMisses;
            sum_conflagrates += s.conflagrates;
            sum_shadowburns += s.shadowburns;
            sum_soul_fires += s.soulFires;
            sum_soul_fire_hits += s.soulFireHits;
            sum_soul_fire_misses += s.soulFireMisses;
            sum_soul_fire_crits += s.soulFireCrits;
            sum_incinerates += s.incinerates;
            sum_incinerate_hits += s.incinerateHits;
            sum_incinerate_misses += s.incinerateMisses;
            sum_incinerate_crits += s.incinerateCrits;
            sum_searing_casts += s.searingCasts;
            sum_searing_hits += s.searingHits;
            sum_searing_misses += s.searingMisses;
            sum_searing_crits += s.searingCrits;
            sum_pet_casts += s.petCasts;
            sum_pet_hits += s.petHits;
            sum_pet_crits += s.petCrits;
            sum_pet_melee_casts += s.petMeleeCasts;
            sum_pet_melee_hits += s.petMeleeHits;
            sum_pet_melee_damage += s.petMeleeDamage;
            sum_pet_lop_casts += s.petLopCasts;
            sum_pet_lop_hits += s.petLopHits;
            sum_pet_lop_damage += s.petLopDamage;
            sum_pet_melee_crits += s.petMeleeCrits;
            sum_pet_lop_crits += s.petLopCrits;
            sum_isb_uptime_us += static_cast<double>(s.isbUptimeUs);
        }

        result.mean_dps = sum_dps / count;
        double variance = (count > 1) ? ((sum_dps_sq - (sum_dps * sum_dps) / count) / (count - 1)) : 0.0;
        result.std_dev_dps = (variance > 0.0) ? std::sqrt(variance) : 0.0;

        std::sort(dps_list.begin(), dps_list.end());
        result.min_dps = dps_list.front();
        result.max_dps = dps_list.back();

        auto get_pct = [&](double pct) {
            size_t idx = static_cast<size_t>(std::clamp(pct * (count - 1), 0.0, static_cast<double>(count - 1)));
            return dps_list[idx];
        };

        result.p1_dps = get_pct(0.01);
        result.p5_dps = get_pct(0.05);
        result.p25_dps = get_pct(0.25);
        result.p50_dps = get_pct(0.50);
        result.p75_dps = get_pct(0.75);
        result.p95_dps = get_pct(0.95);
        result.p99_dps = get_pct(0.99);

        double duration_us = static_cast<double>(config.duration) * 1000000.0;
        if (duration_us > 0.0) {
            result.mean_isb_uptime = ((sum_isb_uptime_us / count) / duration_us) * 100.0;
        } else {
            result.mean_isb_uptime = 0.0;
        }

        result.mean_shadow_bolts = sum_bolts / count;
        result.mean_life_taps = sum_taps / count;
        result.mean_crits = sum_crits / count;

        double total_casts = sum_bolts + sum_dots + sum_agonies + sum_dooms + sum_siphons + sum_wracks + sum_immolates + sum_incinerates + sum_conflagrates + sum_shadowburns + sum_soul_fires + sum_searing_casts;
        result.miss_percent = (total_casts > 0.0) ? (sum_misses / total_casts) * 100.0 : 0.0;
        double total_damage_events = (sum_bolts * config.hit) + sum_ticks + sum_agony_hits + sum_siphon_hits + sum_totg_procs + sum_incinerate_hits + sum_searing_hits + (sum_immolates * config.hit) + (sum_conflagrates * config.hit) + (sum_shadowburns * config.hit) + sum_soul_fire_hits;
        result.crit_percent = (total_damage_events > 0.0) ? (sum_crits / total_damage_events) * 100.0 : 0.0;


        double total_damage = sum_bolt_dmg + sum_dot_dmg + sum_agony_dmg + sum_doom_dmg + sum_siphon_dmg + sum_wrack_dmg + sum_immolate_dmg + sum_incinerate_dmg + sum_conflag_dmg + sum_sburn_dmg + sum_soul_fire_dmg + sum_searing_damage + sum_pet_dmg + sum_totg_damage;
        result.pct_shadow_bolt = (total_damage > 0.0) ? (sum_bolt_dmg / total_damage) * 100.0 : 0.0;
        result.pct_soul_fire = (total_damage > 0.0) ? (sum_soul_fire_dmg / total_damage) * 100.0 : 0.0;
        result.pct_corruption = (total_damage > 0.0) ? (sum_dot_dmg / total_damage) * 100.0 : 0.0;
        result.pct_agony = (total_damage > 0.0) ? (sum_agony_dmg / total_damage) * 100.0 : 0.0;
        result.pct_doom = (total_damage > 0.0) ? (sum_doom_dmg / total_damage) * 100.0 : 0.0;
        result.pct_curse = result.pct_agony + result.pct_doom;
        result.pct_siphon_life = (total_damage > 0.0) ? (sum_siphon_dmg / total_damage) * 100.0 : 0.0;
        result.pct_drain_hope = (total_damage > 0.0) ? (sum_wrack_dmg / total_damage) * 100.0 : 0.0;
        result.pct_immolate = (total_damage > 0.0) ? (sum_immolate_dmg / total_damage) * 100.0 : 0.0;
        result.pct_incinerate = (total_damage > 0.0) ? (sum_incinerate_dmg / total_damage) * 100.0 : 0.0;
        result.pct_searing_pain = (total_damage > 0.0) ? (sum_searing_damage / total_damage) * 100.0 : 0.0;
        result.pct_conflagrate = (total_damage > 0.0) ? (sum_conflag_dmg / total_damage) * 100.0 : 0.0;
        result.pct_shadowburn = (total_damage > 0.0) ? (sum_sburn_dmg / total_damage) * 100.0 : 0.0;
        result.pct_demonic_brand = (total_damage > 0.0) ? (sum_demonic_brand_damage / total_damage) * 100.0 : 0.0;
        result.pct_touch_of_the_grave = (total_damage > 0.0) ? (sum_totg_damage / total_damage) * 100.0 : 0.0;
        result.pct_pet = (total_damage > 0.0) ? (sum_pet_dmg / total_damage) * 100.0 : 0.0;
        result.mean_pet_dps = (config.duration > 0.0f) ? (sum_pet_dmg / count) / config.duration : 0.0;

        if (config.petType == 1.0f) {
            result.pct_pet_imp = result.pct_pet;
            result.pct_pet_firebolt = (total_damage > 0.0) ? ((sum_pet_dmg - sum_demonic_brand_damage) / total_damage) * 100.0 : 0.0;
            auto& fb_stats = result.spell_stats[static_cast<size_t>(SpellID::PET_FIREBOLT)];
            fb_stats.mean_casts = sum_pet_casts / count;
            fb_stats.mean_hits = sum_pet_hits / count;
            fb_stats.mean_misses = (sum_pet_casts - sum_pet_hits) / count;
            fb_stats.mean_crits = sum_pet_crits / count;
            fb_stats.mean_damage = (sum_pet_dmg - sum_demonic_brand_damage) / count;
        } else if (config.petType == 2.0f) {
            result.pct_pet_succubus = result.pct_pet;
            result.pct_pet_melee = (total_damage > 0.0) ? sum_pet_melee_damage / total_damage * 100.0 : 0.0;
            result.pct_pet_lash_of_pain = (total_damage > 0.0) ? sum_pet_lop_damage / total_damage * 100.0 : 0.0;
            auto& melee_stats = result.spell_stats[static_cast<size_t>(SpellID::PET_MELEE)];
            melee_stats.mean_casts = sum_pet_melee_casts / count;
            melee_stats.mean_hits = sum_pet_melee_hits / count;
            melee_stats.mean_crits = sum_pet_melee_crits / count;
            melee_stats.mean_misses = std::max(0.0, sum_pet_melee_casts - sum_pet_melee_hits) / count;
            melee_stats.mean_damage = sum_pet_melee_damage / count;
            auto& lop_stats = result.spell_stats[static_cast<size_t>(SpellID::PET_LASH_OF_PAIN)];
            lop_stats.mean_casts = sum_pet_lop_casts / count;
            lop_stats.mean_hits = sum_pet_lop_hits / count;
            lop_stats.mean_crits = sum_pet_lop_crits / count;
            lop_stats.mean_misses = std::max(0.0, sum_pet_lop_casts - sum_pet_lop_hits) / count;
            lop_stats.mean_damage = sum_pet_lop_damage / count;
        }

        // Populate Spell Stats
        auto& sb_stats = result.spell_stats[static_cast<size_t>(SpellID::SHADOW_BOLT)];
        sb_stats.mean_casts = sum_bolts / count;
        sb_stats.mean_misses = (sum_bolts * (1.0 - config.hit)) / count;
        sb_stats.mean_hits = (sum_bolts * config.hit) / count;
        sb_stats.mean_damage = sum_bolt_dmg / count;
        sb_stats.mean_crits = (sum_bolts * config.hit * config.crit) / count;

        auto& corr_stats = result.spell_stats[static_cast<size_t>(SpellID::CORRUPTION)];
        corr_stats.mean_casts = sum_dots / count;
        corr_stats.mean_hits = sum_corruption_ticks / count;
        corr_stats.mean_misses = (sum_dots * (1.0 - config.hit)) / count;
        corr_stats.mean_crits = sum_corruption_crits / count;
        corr_stats.mean_damage = sum_dot_dmg / count;

        auto& agony_stats = result.spell_stats[static_cast<size_t>(SpellID::CURSE_OF_AGONY)];
        agony_stats.mean_casts = sum_agonies / count;
        agony_stats.mean_hits = sum_agony_hits / count;
        agony_stats.mean_misses = (sum_agonies * (1.0 - config.hit)) / count;
        agony_stats.mean_crits = sum_agony_crits / count;
        agony_stats.mean_damage = sum_agony_dmg / count;

        auto& doom_stats = result.spell_stats[static_cast<size_t>(SpellID::CURSE_OF_DOOM)];
        doom_stats.mean_casts = sum_dooms / count;
        doom_stats.mean_hits = sum_doom_hits / count;
        doom_stats.mean_misses = (sum_dooms * (1.0 - config.hit)) / count;
        doom_stats.mean_crits = sum_doom_crits / count;
        doom_stats.mean_damage = sum_doom_dmg / count;

        auto& siphon_stats = result.spell_stats[static_cast<size_t>(SpellID::SIPHON_LIFE)];
        siphon_stats.mean_casts = sum_siphons / count;
        siphon_stats.mean_hits = sum_siphon_hits / count;
        siphon_stats.mean_misses = (sum_siphons * (1.0 - config.hit)) / count;
        siphon_stats.mean_crits = sum_siphon_crits / count;
        siphon_stats.mean_damage = sum_siphon_dmg / count;

        auto& wrack_stats = result.spell_stats[static_cast<size_t>(SpellID::DRAIN_HOPE)];
        wrack_stats.mean_casts = sum_wracks / count;
        wrack_stats.mean_hits = sum_wrack_hits / count;
        wrack_stats.mean_crits = sum_wrack_crits / count;
        wrack_stats.mean_damage = sum_wrack_dmg / count;

        auto& imm_stats = result.spell_stats[static_cast<size_t>(SpellID::IMMOLATE)];
        imm_stats.mean_casts = sum_immolates / count;
        imm_stats.mean_hits = (sum_immolate_direct_hits + sum_immolate_dot_hits) / count;
        imm_stats.mean_misses = sum_immolate_misses / count;
        imm_stats.mean_crits = imm_stats.mean_hits * config.fireCrit;
        imm_stats.mean_damage = sum_immolate_dmg / count;

        auto& incin_stats = result.spell_stats[static_cast<size_t>(SpellID::INCINERATE)];
        incin_stats.mean_casts = sum_incinerates / count;
        incin_stats.mean_hits = sum_incinerate_hits / count;
        incin_stats.mean_misses = sum_incinerate_misses / count;
        incin_stats.mean_crits = sum_incinerate_crits / count;
        incin_stats.mean_damage = sum_incinerate_dmg / count;

        auto& searing_stats = result.spell_stats[static_cast<size_t>(SpellID::SEARING_PAIN)];
        searing_stats.mean_casts = sum_searing_casts / count;
        searing_stats.mean_hits = sum_searing_hits / count;
        searing_stats.mean_misses = sum_searing_misses / count;
        searing_stats.mean_crits = sum_searing_crits / count;
        searing_stats.mean_damage = sum_searing_damage / count;

        auto& conflag_stats = result.spell_stats[static_cast<size_t>(SpellID::CONFLAGRATE)];
        conflag_stats.mean_casts = sum_conflagrates / count;
        conflag_stats.mean_hits = (sum_conflagrates * config.hit) / count;
        conflag_stats.mean_misses = (sum_conflagrates * (1.0 - config.hit)) / count;
        conflag_stats.mean_crits = conflag_stats.mean_hits * std::min(1.0f, config.fireCrit + config.fnbCritBonus);
        conflag_stats.mean_damage = sum_conflag_dmg / count;

        auto& sburn_stats = result.spell_stats[static_cast<size_t>(SpellID::SHADOWBURN)];
        sburn_stats.mean_casts = sum_shadowburns / count;
        sburn_stats.mean_hits = (sum_shadowburns * config.hit) / count;
        sburn_stats.mean_misses = (sum_shadowburns * (1.0 - config.hit)) / count;
        sburn_stats.mean_crits = sburn_stats.mean_hits * config.crit;
        sburn_stats.mean_damage = sum_sburn_dmg / count;

        auto& soul_fire_stats = result.spell_stats[static_cast<size_t>(SpellID::SOUL_FIRE)];
        soul_fire_stats.mean_casts = sum_soul_fires / count;
        soul_fire_stats.mean_hits = sum_soul_fire_hits / count;
        soul_fire_stats.mean_misses = sum_soul_fire_misses / count;
        soul_fire_stats.mean_crits = sum_soul_fire_crits / count;
        soul_fire_stats.mean_damage = sum_soul_fire_dmg / count;

        auto& tap_stats = result.spell_stats[static_cast<size_t>(SpellID::LIFE_TAP)];
        tap_stats.mean_casts = sum_taps / count;

        auto& totg_stats = result.spell_stats[static_cast<size_t>(SpellID::TOUCH_OF_THE_GRAVE)];
        totg_stats.mean_hits = sum_totg_procs / count;
        totg_stats.mean_damage = sum_totg_damage / count;

        // Build 20 Histogram Bins
        constexpr int kNumBins = 20;
        result.histogram.resize(kNumBins);
        double dps_range = std::max(1.0, result.max_dps - result.min_dps);
        double bin_width = dps_range / kNumBins;

        for (int b = 0; b < kNumBins; ++b) {
            result.histogram[b].min_dps = result.min_dps + b * bin_width;
            result.histogram[b].max_dps = result.histogram[b].min_dps + bin_width;
            result.histogram[b].count = 0;
        }

        for (double dps : dps_list) {
            int bin_idx = static_cast<int>((dps - result.min_dps) / bin_width);
            if (bin_idx < 0) bin_idx = 0;
            if (bin_idx >= kNumBins) bin_idx = kNumBins - 1;
            result.histogram[bin_idx].count++;
        }

        return result;
    }

    static BatchSimResult run_batch(
        const WarlockSimulator& sim,
        int iterations = 10000,
        uint32_t step_us = 0,
        uint32_t seed = 42
    );
};

} // namespace warlock
