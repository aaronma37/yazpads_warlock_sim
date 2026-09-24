#include "warlock_sim.hpp"
#include "viper_oracle.hpp"
#include <algorithm>
#include <cmath>

namespace warlock {

WarlockSimulator::WarlockSimulator() {
    race = Race::HUMAN;
    base_attrs = get_base_attributes_for_race(race);
    gear = GearLoadout::create_preraid_bis();
    talents = Talents::create_forever_shadow_destro();
    use_raw_stats = true;
    raw_stats = gear.calculate_stats();
    raw_stats.spell_power += raw_stats.shadow_power;
    raw_stats.shadow_power = 0.0;
}

double WarlockSimulator::calculate_hit_chance(School school) const {
    double base_hit = mechanics.base_hit_vs_boss; // 0.83 (83% for lvl 63 boss)
    int delta = target_config.level - 60;
    if (delta <= 0) {
        base_hit = 0.96 + std::min(0.03, -delta * 0.01);
    } else if (delta == 1) {
        base_hit = 0.95;
    } else if (delta == 2) {
        base_hit = 0.94;
    } else if (delta == 3) {
        base_hit = mechanics.base_hit_vs_boss; // 0.83 (83%)
    } else {
        base_hit = std::max(0.01, mechanics.base_hit_vs_boss - (delta - 3) * 0.11);
    }

    double hit = base_hit;
    Stats current_stats = use_raw_stats ? raw_stats : gear.calculate_stats();
    hit += current_stats.spell_hit_percent * 0.01;

    // In WoW Forever: Suppression grants +1% hit to all spells per point
    if (talents.aff.suppression > 0) {
        hit += talents.aff.suppression * 0.01;
    }

    if (hit > mechanics.max_spell_hit) {
        hit = mechanics.max_spell_hit; // 99% cap
    }
    return hit;
}

double WarlockSimulator::calculate_crit_chance(School school, const Stats& stats) const {
    double crit = stats.total_spell_crit(base_attrs.base_spell_crit);

    // Human Sword Specialization (+2% spell/ability crit when wielding 1H sword)
    bool is_sword = false;
    if (!use_raw_stats) {
        const Item& mh = gear.get(Slot::MAIN_HAND);
        if (mh.name.find("Mageblade") != std::string::npos ||
            mh.name.find("Sword") != std::string::npos ||
            mh.name.find("Blade") != std::string::npos ||
            mh.icon.find("sword") != std::string::npos ||
            mh.icon.find("Sword") != std::string::npos) {
            is_sword = true;
        }
    } else {
        is_sword = true; // Default to active in direct stat mode
    }
    if (race == Race::HUMAN && is_sword) {
        crit += 2.0;
    }

    // Orc Axe Specialization (+1% spell/ability crit when wielding axe)
    bool is_axe = false;
    if (!use_raw_stats) {
        const Item& mh = gear.get(Slot::MAIN_HAND);
        if (mh.name.find("Axe") != std::string::npos || mh.icon.find("axe") != std::string::npos) {
            is_axe = true;
        }
    }
    if (race == Race::ORC && is_axe) {
        crit += 1.0;
    }

    // Malevolence (Afflic Row 4 Col 1): +1% crit chance to Shadow spells per point
    if (school == School::SHADOW && talents.aff.malevolence > 0) {
        crit += talents.aff.malevolence * 1.0;
    }

    return crit * 0.01; // As fraction
}

double WarlockSimulator::calculate_partial_resist_multiplier(School school, double target_resistance, FastRNG& rng) const {
    if (!mechanics.partial_resists_enabled || target_resistance <= 0.0) {
        return 1.0;
    }

    // Classic 4-roll partial resist model
    // Innate resistance ~ 24 on boss if not cursed
    double avg_resist = (target_resistance / (60.0 * 5.0)) * 0.75;
    if (avg_resist <= 0.0) return 1.0;

    double roll = rng.next_double();
    if (roll < 0.76) return 1.0;
    if (roll < 0.95) return 0.75;
    if (roll < 0.99) return 0.50;
    return 0.25;
}

SimResult WarlockSimulator::run_single_simulation(FastRNG& rng) {
    SimResult result;

    // Resolve fight duration for this iteration (fixed vs randomized)
    double effective_duration = fight_duration;
    if (randomize_duration && duration_variance > 0.0) {
        double min_d = std::max(5.0, fight_duration - duration_variance);
        double max_d = fight_duration + duration_variance;
        effective_duration = rng.range(min_d, max_d);
    }
    result.duration = effective_duration;

    // Race-specific base attributes
    base_attrs = get_base_attributes_for_race(race);

    // Compute base stats from gear & buffs (or raw manual stats)
    Stats stats = use_raw_stats ? raw_stats : gear.calculate_stats();

    // Demonic Sacrifice buffs require the Demonic Sacrifice talent (or Demonic Pact)
    BuffConfig active_buffs = buffs;
    if (talents.demo.demonic_sacrifice == 0 && talents.demo.demonic_pact == 0) {
        active_buffs.sacrifice_imp = false;
        active_buffs.sacrifice_succubus = false;
    }
    active_buffs.apply_to_stats(stats, base_attrs, true, mechanics.personal_shadow_weaving); // true = WoW Forever mechanics

    // Demonic Embrace (+3% Total Stamina per point, up to +15%)
    if (talents.demo.demonic_embrace > 0) {
        double stam_bonus_mult = 1.0 + talents.demo.demonic_embrace * 0.03;
        stats.stamina *= stam_bonus_mult;
        stats.max_health = base_attrs.base_health + stats.stamina * 10.0;
        if (active_buffs.flask_of_the_titans) stats.max_health += 1200.0;
    }

    // Gnome Expansive Mind (+5% Mana)
    if (race == Race::GNOME) {
        stats.max_mana *= 1.05;
    }

    // Fel Vitality (+5% Max Mana per point)
    if (talents.demo.fel_vitality > 0) {
        stats.max_mana *= (1.0 + talents.demo.fel_vitality * 0.05);
    }

    // Active Pet Determination:
    // In WoW Forever, Demonic Pact (Demo Capstone) allows Demonic Sacrifice to persist
    // while summoning a DIFFERENT demon pet.
    PetChoice active_pet = policy.pet;
    if (active_buffs.sacrifice_succubus || active_buffs.sacrifice_imp) {
        if (talents.demo.demonic_pact > 0) {
            if (active_buffs.sacrifice_imp && policy.pet == PetChoice::IMP) {
                active_pet = PetChoice::NONE;
            } else if (active_buffs.sacrifice_succubus && policy.pet == PetChoice::SUCCUBUS) {
                active_pet = PetChoice::NONE;
            } else {
                active_pet = policy.pet;
            }
        } else {
            active_pet = PetChoice::NONE;
        }
    }

    // Blood Pact Rank 5: Increases party members' Stamina by 54 whenever Imp is active
    if (active_pet == PetChoice::IMP) {
        double blood_pact_stamina = 54.0;
        if (talents.demo.demonic_embrace > 0) {
            blood_pact_stamina *= (1.0 + talents.demo.demonic_embrace * 0.03);
        }
        stats.stamina += blood_pact_stamina;
        stats.max_health += blood_pact_stamina * 10.0;
    }

    // Demonic Knowledge (Demo Row 5 Col 3): +33% of level per point to spell damage (+60 SP at 3/3 at level 60)
    if (active_pet != PetChoice::NONE && talents.demo.demonic_knowledge > 0) {
        stats.spell_power += 20.0 * talents.demo.demonic_knowledge;
    }

    // Soul Link (Demo Row 5 Col 2): +3% damage dealt by master and demon
    if (active_pet != PetChoice::NONE && talents.demo.soul_link > 0) {
        stats.all_damage_multiplier *= 1.03;
    }

    // Apply talent multipliers
    double shadow_multiplier = stats.shadow_multiplier;
    if (talents.aff.shadow_mastery > 0) {
        shadow_multiplier *= (1.0 + talents.aff.shadow_mastery * 0.01); // 1% per pt in Forever
    }
    double fire_multiplier = stats.fire_multiplier;

    // Master Demonologist (Forever: Succubus = +2%/pt Shadow, Imp = +2%/pt Fire)
    if (active_pet == PetChoice::SUCCUBUS && talents.demo.master_demonologist > 0) {
        shadow_multiplier *= (1.0 + talents.demo.master_demonologist * 0.02);
    } else if (active_pet == PetChoice::IMP && talents.demo.master_demonologist > 0) {
        fire_multiplier *= (1.0 + talents.demo.master_demonologist * 0.02);
    }

    // Agonizing Flames (Destro Row 4 Col 2): +3% / +7% / +10% damage to ALL Destruction spells
    double agonizing_flames_bonus = (talents.destro.agonizing_flames == 1) ? 0.03 :
                                   ((talents.destro.agonizing_flames == 2) ? 0.07 :
                                   ((talents.destro.agonizing_flames == 3) ? 0.10 : 0.0));
    double destro_spell_mult = 1.0 + agonizing_flames_bonus;

    // Cataclysm (Destro Row 2 Col 2): -3% / -6% / -10% Mana cost to Destruction spells
    double cataclysm_mana_mult = (talents.destro.cataclysm == 1) ? (1.0 - 0.03) :
                                ((talents.destro.cataclysm == 2) ? (1.0 - 0.06) :
                                ((talents.destro.cataclysm == 3) ? (1.0 - 0.10) : 1.0));

    // Fire and Brimstone (Destro Row 5 Col 3): +8% / +17% / +25% Conflagrate crit chance
    double fnb_crit_bonus = (talents.destro.fire_and_brimstone == 1) ? 0.08 :
                           ((talents.destro.fire_and_brimstone == 2) ? 0.17 :
                           ((talents.destro.fire_and_brimstone == 3) ? 0.25 : 0.0));

    // Malediction (Afflic Row 2 Col 1): +1% periodic damage per point (+5% at 5/5)
    double malediction_mult = 1.0 + talents.aff.malediction * 0.01;

    // Ruin (Destro Row 3 Col 2): +20% crit damage bonus per point (+100% bonus at 5/5 -> 2.0x total)
    double destro_crit_mult = 1.0 + 0.50 * (1.0 + talents.destro.ruin * 0.20);
    double base_crit_mult = mechanics.base_spell_crit_multiplier; // 1.50

    // Simulation runtime state
    double current_time = 0.0;
    double player_mana = stats.max_mana;
    double player_health = stats.max_health;

    double gcd_ready_time = 0.0;
    double cast_finish_time = 0.0;
    bool is_casting = false;
    SpellID current_casting_spell = SpellID::NONE;

    // Cooldowns
    double potion_cd_ready = 0.0;
    double rune_cd_ready = 0.0;
    double shadowburn_cd_ready = 0.0;
    double conflagrate_cd_ready = 0.0;
    double soul_fire_cd_ready = 0.0;
    double drain_hope_cd_ready = 0.0;
    double trinket_cd_ready = 0.0;
    double trinket_expire_time = 0.0;
    double racial_cd_ready = 0.0;
    double racial_expire_time = 0.0;
    double amplify_curse_cd_ready = 0.0;
    int eureka_charges = 0;
    double doom_tick_time = 0.0;

    auto get_current_sp = [&](School school, double t) -> double {
        double sp = (school == School::FIRE) ? stats.effective_fire_power() : stats.effective_shadow_power();
        if (t < trinket_expire_time) sp += 175.0;
        if (race == Race::ORC && t < racial_expire_time) sp += stats.spell_power * 0.10; // Blood Fury
        return sp;
    };

    auto get_haste_mult = [&](double t) -> double {
        double haste = stats.spell_haste_percent;
        if (race == Race::TROLL && t < racial_expire_time) haste += 10.0; // Berserking
        return 1.0 / (1.0 + haste * 0.01);
    };

    // Buffs and procs
    bool shadow_trance_active = false;
    double shadow_trance_expire = 0.0;
    double shadow_and_flame_shadow_expire = 0.0; // Conflagrate grants +10% Shadow for 20s
    double shadow_and_flame_fire_expire = 0.0;   // Shadowburn grants +10% Fire for 20s
    double drain_hope_channel_end = 0.0;         // +10% Shadow DoT damage during channel
    int demonic_brand_charges = 0;               // Pet attacks consuming Demonic Brand
    double demonic_brand_expire = 0.0;
    double decimation_buff_expire = 0.0;         // 10s Soul Fire cast time reduction buff from SB/Searing Pain on <35% HP

    // Target state
    TargetConfig target = target_config;
    target.is_beast = (target.creature_type == CreatureType::BEAST) || target.is_beast;
    if (buffs.curse_of_shadows) {
        target.current_shadow_resistance = std::max(0.0, target.base_shadow_resistance - 75.0);
        target.curse_of_shadows = true;
    }
    if (buffs.curse_of_elements) {
        target.current_fire_resistance = std::max(0.0, target.base_fire_resistance - 75.0);
        target.curse_of_elements = true;
    }
    if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) {
        target.shadow_weaving = true;
    }

    // Pet Mana tracking
    double pet_max_mana = 0.0;
    if (active_pet == PetChoice::IMP) {
        pet_max_mana = mechanics.imp_base_mana * (1.0 + 0.05 * talents.demo.fel_vitality);
    } else if (active_pet == PetChoice::SUCCUBUS) {
        pet_max_mana = mechanics.succubus_base_mana * (1.0 + 0.05 * talents.demo.fel_vitality);
    }
    double pet_mana = pet_max_mana;

    // Active DoTs tracking & Multi-Target state
    constexpr int MAX_TARGETS = 5;
    int num_targets = std::clamp(target_config.target_count, 1, MAX_TARGETS);

    struct ActiveDot {
        bool active = false;
        double expire_time = 0.0;
        int ticks_remaining = 0;
        double tick_interval = 3.0;
        double tick_damage = 0.0;
        double tick_multiplier = 1.0;
        bool amplified = false;
    };

    struct TargetCombatState {
        ActiveDot dot_corruption;
        ActiveDot dot_agony;
        ActiveDot dot_immolate;
        ActiveDot dot_siphon_life;
        bool has_bane_of_havoc = false;
        double bane_of_havoc_expire = 0.0;
    };
    TargetCombatState target_states[MAX_TARGETS];

    // Reference aliases for target 0 (primary target) for single-target transparent compatibility
    ActiveDot& dot_corruption = target_states[0].dot_corruption;
    ActiveDot& dot_agony = target_states[0].dot_agony;
    ActiveDot& dot_immolate = target_states[0].dot_immolate;
    ActiveDot& dot_siphon_life = target_states[0].dot_siphon_life;

    // Bane of Havoc cleave applicator
    auto apply_havoc_cleave = [&](double raw_damage, int source_target_idx) {
        if (num_targets >= 2 && target_states[1].has_bane_of_havoc && current_time < target_states[1].bane_of_havoc_expire && source_target_idx != 1) {
            double havoc_dmg = raw_damage * 0.15;
            result.dmg_bane_of_havoc += havoc_dmg;
            result.dmg_curse += havoc_dmg;
            result.total_damage += havoc_dmg;
            result.record_spell_hit(SpellID::BANE_OF_HAVOC, havoc_dmg, false);
        }
    };

    // Setup FastEventQueue
    FastEventQueue<256> queue;
    queue.push(effective_duration, EventType::SIMULATION_END);
    queue.push(5.0, EventType::MANA_REGEN_TICK);

    // Initial Pet actions
    if (active_pet == PetChoice::SUCCUBUS) {
        queue.push(1.0, EventType::PET_MELEE_SWING);
        queue.push(0.5, EventType::PET_CAST_FINISH);
    } else if (active_pet == PetChoice::IMP) {
        queue.push(0.3, EventType::PET_CAST_FINISH);
    }

    if (record_timeline) {
        result.timeline.push_back({0.0, 0.0, SpellID::NONE, false, false, player_mana, 0});
    }

    // Helper to get active school multiplier with procs
    auto get_current_shadow_multiplier = [&](double now) {
        double mult = shadow_multiplier;
        if (now < shadow_and_flame_shadow_expire && talents.destro.shadow_and_flame > 0) {
            mult *= (1.0 + talents.destro.shadow_and_flame * 0.02); // up to +10%
        }
        return mult;
    };

    auto get_current_fire_multiplier = [&](double now) {
        double mult = fire_multiplier;
        if (now < shadow_and_flame_fire_expire && talents.destro.shadow_and_flame > 0) {
            mult *= (1.0 + talents.destro.shadow_and_flame * 0.02); // up to +10%
        }
        return mult;
    };

    auto apply_touch_of_the_grave = [&](double now) {
        if (race == Race::UNDEAD && rng.chance(0.10)) {
            double grave_dmg = 0.05 * stats.max_health * get_current_shadow_multiplier(now);
            result.total_damage += grave_dmg;
            result.dmg_touch_of_the_grave += grave_dmg;
            result.touch_of_the_grave_procs++;
            result.total_damage_events++;
            result.record_spell_hit(SpellID::TOUCH_OF_THE_GRAVE, grave_dmg, false);
            player_health = std::min(stats.max_health, player_health + grave_dmg);
        }
    };

    // Decision maker using Rule-Based Action Priority List (APL)
    std::vector<PriorityRule> priority_rules = policy.get_priority_rules(talents, race);
    RotationChoice eff_rotation = policy.rotation;
    size_t decision_step_count = 0;

    auto decide_next_action = [&](double now) {
        if (is_casting || now < gcd_ready_time) return;

        bool execute_phase = (now / effective_duration) >= 0.65; // Target <35% HP

        // 1. Off-GCD Cooldown checks: Mana Potions & Demonic Runes
        if (buffs.use_mana_potions && now >= potion_cd_ready && (stats.max_mana - player_mana) >= 1800.0) {
            double mana_gain = rng.range(1400.0, 2200.0);
            player_mana = std::min(stats.max_mana, player_mana + mana_gain);
            result.mana_gained += mana_gain;
            potion_cd_ready = now + 120.0;
            if (record_timeline) {
                result.timeline.push_back({now, 0.0, SpellID::POTION_MANA, false, false, player_mana, target.isb_charges});
            }
        }
        if (buffs.use_demonic_runes && now >= rune_cd_ready && (stats.max_mana - player_mana) >= 1200.0 && player_health > 1500.0) {
            double mana_gain = rng.range(900.0, 1500.0);
            player_mana = std::min(stats.max_mana, player_mana + mana_gain);
            player_health -= mana_gain;
            result.mana_gained += mana_gain;
            rune_cd_ready = now + 120.0;
            if (record_timeline) {
                result.timeline.push_back({now, 0.0, SpellID::DEMONIC_RUNE, false, false, player_mana, target.isb_charges});
            }
        }

        // 2. Off-GCD Trinket on-use
        if (policy.use_trinkets_on_cooldown && now >= trinket_cd_ready) {
            const Item& t1 = gear.get(Slot::TRINKET1);
            const Item& t2 = gear.get(Slot::TRINKET2);
            if (t1.has_on_use) {
                trinket_expire_time = now + t1.on_use_duration;
                trinket_cd_ready = now + t1.on_use_cooldown;
                queue.push(trinket_expire_time, EventType::BUFF_EXPIRE, static_cast<uint8_t>(SpellID::TRINKET_USE));
            } else if (t2.has_on_use) {
                trinket_expire_time = now + t2.on_use_duration;
                trinket_cd_ready = now + t2.on_use_cooldown;
                queue.push(trinket_expire_time, EventType::BUFF_EXPIRE, static_cast<uint8_t>(SpellID::TRINKET_USE));
            }
        }

        // 2b. Racial active cooldowns
        bool racial_ready = (now >= racial_cd_ready);
        bool should_trigger_racial = false;

        if (racial_ready) {
            double racial_cd = (race == Race::TROLL) ? 180.0 : 120.0;
            bool doom_active = (doom_tick_time > now);
            double time_to_doom = doom_active ? (doom_tick_time - now) : 999.0;
            bool will_cast_doom = false;
            for (const auto& r : priority_rules) {
                if (r.action == PriorityAction::CURSE_OF_DOOM) {
                    will_cast_doom = true;
                    break;
                }
            }

            if ((race == Race::GNOME || race == Race::ORC) && (doom_active || will_cast_doom || policy.racial_policy == RacialPolicy::ALIGN_DOOM)) {
                double doom_window = (race == Race::ORC) ? 14.0 : 6.0;
                if (doom_active && (fight_duration - now >= time_to_doom)) {
                    // Pop before the Doom damage tick
                    if (time_to_doom <= doom_window && time_to_doom >= 0.0) {
                        should_trigger_racial = true;
                    }
                } else if (!doom_active && will_cast_doom && (fight_duration - now >= 60.0)) {
                    // Hold racial to align with the upcoming Doom tick
                    should_trigger_racial = false;
                } else if (policy.racial_policy == RacialPolicy::ON_COOLDOWN) {
                    should_trigger_racial = true;
                } else if (policy.racial_policy == RacialPolicy::EXECUTE_ONLY || policy.racial_policy == RacialPolicy::ALIGN_DOOM) {
                    should_trigger_racial = execute_phase;
                } else if (policy.racial_policy == RacialPolicy::ALIGN_EXECUTE) {
                    double execute_start = fight_duration * 0.65;
                    if (now < 1.0 && (execute_start >= racial_cd)) {
                        should_trigger_racial = true;
                    } else {
                        should_trigger_racial = execute_phase;
                    }
                }
            } else if (policy.racial_policy == RacialPolicy::ON_COOLDOWN) {
                should_trigger_racial = true;
            } else if (policy.racial_policy == RacialPolicy::EXECUTE_ONLY || policy.racial_policy == RacialPolicy::ALIGN_DOOM) {
                should_trigger_racial = execute_phase;
            } else if (policy.racial_policy == RacialPolicy::ALIGN_EXECUTE) {
                double execute_start = fight_duration * 0.65;
                if (now < 1.0 && (execute_start >= racial_cd)) {
                    should_trigger_racial = true;
                } else {
                    should_trigger_racial = execute_phase;
                }
            }
        }

        if (should_trigger_racial) {
            if (race == Race::ORC) {
                racial_expire_time = now + 15.0;
                racial_cd_ready = now + 120.0;
                if (record_timeline) {
                    bool doom_active = (doom_tick_time > now);
                    double time_to_doom = doom_active ? (doom_tick_time - now) : 999.0;
                    std::string note = (time_to_doom <= 14.0 && time_to_doom >= 0.0) ? "Blood Fury (Aligned with Bane of Doom)" : (execute_phase ? "Blood Fury (Execute Phase)" : "Blood Fury (+10% SP for 15s)");
                    result.cast_sequence.push_back({now, SpellID::RACIAL_BLOOD_FURY, 0.0, false, false, 0.0, note});
                }
            } else if (race == Race::TROLL) {
                racial_expire_time = now + 10.0;
                racial_cd_ready = now + 180.0;
                if (record_timeline) {
                    result.cast_sequence.push_back({now, SpellID::RACIAL_BERSERKING, 0.0, false, false, 0.0, execute_phase ? "Berserking (Execute Phase)" : "Racial Cooldown"});
                }
            } else if (race == Race::GNOME) {
                eureka_charges = 3;
                racial_cd_ready = now + 120.0;
                if (record_timeline) {
                    bool doom_active = (doom_tick_time > now);
                    double time_to_doom = doom_active ? (doom_tick_time - now) : 999.0;
                    std::string note = (time_to_doom <= 6.0 && time_to_doom >= 0.0) ? "Eureka! (Aligned with Bane of Doom)" : (execute_phase ? "Eureka! (Execute -50% Mana, +10% Dmg)" : "Eureka! (-50% Mana, +10% Dmg 3 casts)");
                    result.cast_sequence.push_back({now, SpellID::RACIAL_EUREKA, 0.0, false, false, 0.0, note});
                }
            }
        }

        // 3. Multi-Target Openers & Upkeep (Bane of Havoc & Multi-DoT Corruption)
        if (num_targets >= 2 && talents.destro.bane_of_havoc > 0 && policy.auto_bane_of_havoc) {
            if (!target_states[1].has_bane_of_havoc || now >= target_states[1].bane_of_havoc_expire) {
                double havoc_mana = 150.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                if (player_mana >= havoc_mana) {
                    player_mana -= havoc_mana;
                    result.mana_spent += havoc_mana;
                    result.total_casts++;
                    result.record_spell_cast(SpellID::BANE_OF_HAVOC);
                    if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                    if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                        target_states[1].has_bane_of_havoc = true;
                        target_states[1].bane_of_havoc_expire = now + 300.0;
                    } else {
                        result.misses++;
                        result.record_spell_miss(SpellID::BANE_OF_HAVOC);
                    }

                    gcd_ready_time = now + mechanics.base_gcd;
                    queue.push(gcd_ready_time, EventType::GCD_READY);
                    if (record_timeline) {
                        result.timeline.push_back({now, 0.0, SpellID::BANE_OF_HAVOC, false, false, player_mana, target.isb_charges});
                        result.cast_sequence.push_back({now, SpellID::BANE_OF_HAVOC, 0.0, false, false, 0.0, "Bane of Havoc (T2)"});
                    }
                    return;
                }
            }
        }

        if (num_targets >= 2 && policy.multi_dot_corruption && policy.corruption != DotPolicy::NEVER &&
            eff_rotation != RotationChoice::PURE_SHADOW_BOLT &&
            eff_rotation != RotationChoice::DP_AF_SHADOW_NO_CORRUPTION &&
            eff_rotation != RotationChoice::FIRE_DESTRO_NO_CORRUPTION) {
            for (int t = 1; t < num_targets; ++t) {
                if ((!target_states[t].dot_corruption.active || now >= target_states[t].dot_corruption.expire_time) &&
                    (fight_duration - now >= 8.0)) {
                    double corr_mana = 340.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    if (player_mana >= corr_mana) {
                        double cast_time = std::max(0.0, (2.0 - 0.4 * talents.aff.improved_corruption) * get_haste_mult(now));
                        if (cast_time == 0.0) {
                            player_mana -= corr_mana;
                            result.mana_spent += corr_mana;
                            result.total_casts++;
                            result.record_spell_cast(SpellID::CORRUPTION);
                            bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                            if (eureka_active) eureka_charges--;

                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                target_states[t].dot_corruption.active = true;
                                target_states[t].dot_corruption.expire_time = now + 18.0;
                                target_states[t].dot_corruption.ticks_remaining = 6;
                                target_states[t].dot_corruption.tick_interval = 3.0;
                                double sp = get_current_sp(School::SHADOW, now);
                                target_states[t].dot_corruption.tick_damage = 73.0 + (sp * mechanics.corruption_sp_coefficient * 0.20);
                                target_states[t].dot_corruption.tick_multiplier = get_current_shadow_multiplier(now) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult * (eureka_active ? 1.10 : 1.0);
                                queue.push(now + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION), 0, static_cast<uint32_t>(t));
                            } else {
                                result.misses++;
                                result.record_spell_miss(SpellID::CORRUPTION);
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.timeline.push_back({now, 0.0, SpellID::CORRUPTION, false, false, player_mana, target.isb_charges});
                                result.cast_sequence.push_back({now, SpellID::CORRUPTION, 0.0, false, false, 0.0, "Multi-DoT Corruption"});
                            }
                            return;
                        } else {
                            is_casting = true;
                            current_casting_spell = SpellID::CORRUPTION;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::CORRUPTION), 0, static_cast<uint32_t>(t));
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::CORRUPTION, 0.0, false, false, cast_time, "Multi-DoT Corruption"});
                            }
                            return;
                        }
                    }
                }
            }
        }

        // Helper to extract normalized state observation for ML / VIPER
        auto get_current_observation = [&]() -> sim::SimObservation {
            sim::SimObservation obs;
            obs.player_mana_pct = static_cast<float>(std::clamp(player_mana / std::max(1.0, stats.max_mana), 0.0, 1.0));
            obs.player_hp_pct = static_cast<float>(std::clamp(player_health / std::max(1.0, stats.max_health), 0.0, 1.0));
            obs.fight_progress_pct = static_cast<float>(std::clamp(now / std::max(0.1, effective_duration), 0.0, 1.0));
            obs.time_remaining_sec = static_cast<float>(std::max(0.0, effective_duration - now));
            obs.target_hp_pct = static_cast<float>(std::clamp(1.0 - (now / std::max(0.1, effective_duration)), 0.0, 1.0));
            
            obs.num_targets = static_cast<float>(num_targets);
            obs.target2_has_havoc = (num_targets >= 2 && target_states[1].has_bane_of_havoc && now < target_states[1].bane_of_havoc_expire) ? 1.0f : 0.0f;
            
            obs.nightfall_proc_active = (now < shadow_trance_expire) ? 1.0f : 0.0f;
            obs.decimation_rem_sec = static_cast<float>(std::max(0.0, decimation_buff_expire - now));
            obs.shadow_and_flame_rem_sec = static_cast<float>(std::max(0.0, shadow_and_flame_fire_expire - now));
            obs.trinket_rem_sec = static_cast<float>(std::max(0.0, trinket_expire_time - now));
            obs.racial_rem_sec = static_cast<float>(std::max(0.0, racial_expire_time - now));
            obs.eureka_charges = static_cast<float>(eureka_charges);
            
            obs.dot_corruption_rem_sec = (dot_corruption.active && now < dot_corruption.expire_time)
                ? static_cast<float>(dot_corruption.expire_time - now) : 0.0f;
            obs.dot_agony_rem_sec = (dot_agony.active && now < dot_agony.expire_time)
                ? static_cast<float>(dot_agony.expire_time - now) : 0.0f;
            obs.dot_doom_rem_sec = (doom_tick_time > now) ? static_cast<float>(doom_tick_time - now) : 0.0f;
            obs.dot_immolate_rem_sec = (dot_immolate.active && now < dot_immolate.expire_time)
                ? static_cast<float>(dot_immolate.expire_time - now) : 0.0f;
            obs.dot_siphon_life_rem_sec = (dot_siphon_life.active && now < dot_siphon_life.expire_time)
                ? static_cast<float>(dot_siphon_life.expire_time - now) : 0.0f;
            obs.dot_wrack_rem_sec = (now < drain_hope_channel_end)
                ? static_cast<float>(drain_hope_channel_end - now) : 0.0f;
            bool isb_active = (target.isb_expire_time > now && (target.isb_charges > 0 || target.isb_charges == -1));
            obs.isb_charges_rem = isb_active ? (target.isb_charges == -1 ? 4.0f : static_cast<float>(target.isb_charges)) : 0.0f;
            
            obs.cd_conflagrate_sec = static_cast<float>(std::max(0.0, conflagrate_cd_ready - now));
            obs.cd_shadowburn_sec = static_cast<float>(std::max(0.0, shadowburn_cd_ready - now));
            obs.cd_curse_of_doom_sec = static_cast<float>(std::max(0.0, doom_tick_time - now));
            obs.cd_amplify_curse_sec = static_cast<float>(std::max(0.0, amplify_curse_cd_ready - now));
            obs.cd_racial_sec = static_cast<float>(std::max(0.0, racial_cd_ready - now));
            obs.cd_potion_sec = static_cast<float>(std::max(0.0, potion_cd_ready - now));
            obs.cd_demonic_rune_sec = static_cast<float>(std::max(0.0, rune_cd_ready - now));
            
            return obs;
        };

        auto log_viper_sample = [&](PriorityAction act, const std::string& name) {
            result.action_history.push_back(act);
            decision_step_count++;
            if (record_viper_samples) {
                sim::VIPERStep step;
                step.state = get_current_observation();
                step.oracle_action = static_cast<uint8_t>(act);
                step.action_name = name;
                step.sample_weight = 1.0f;
                viper_dataset.add_sample(step);
            }
        };

        // 4. Action Selection: Forced Rollout Prefix vs Live Online Greedy Oracle vs Sequential Rule-Based Priority Evaluation
        bool is_forced = (decision_step_count < forced_action_prefix.size());
        std::vector<PriorityRule> rules_to_evaluate;
        
        bool is_oracle = (use_oracle_execution_policy || policy.use_oracle_execution_policy);
        std::vector<PriorityRule> dynamic_oracle_rules;
        if (is_oracle && !is_forced) {
            sim::SimObservation cur_obs = get_current_observation();
            auto candidates = VIPEROracle::get_candidate_actions();
            std::vector<std::pair<double, PriorityAction>> scored;
            scored.reserve(candidates.size());
            for (const auto& [act, _] : candidates) {
                if (act == PriorityAction::RACIAL_EUREKA ||
                    act == PriorityAction::RACIAL_BLOOD_FURY ||
                    act == PriorityAction::RACIAL_BERSERKING ||
                    act == PriorityAction::AMPLIFY_CURSE ||
                    act == PriorityAction::BANE_OF_HAVOC) {
                    continue; // Handled off-GCD
                }
                if (VIPEROracle::is_action_legal(act, cur_obs, talents)) {
                    double q = VIPEROracle::estimate_local_q_value(cur_obs, act, talents);
                    scored.push_back({q, act});
                }
            }
            std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
                return a.first > b.first;
            });
            for (const auto& [q_val, act] : scored) {
                if (q_val <= -900.0) continue;
                PriorityRule r;
                r.action = act;
                r.spell_id = VIPEROracle::get_spell_id(act);
                r.name = VIPEROracle::get_action_name(act);
                r.enabled = true;
                r.use_custom_thresholds = false;
                dynamic_oracle_rules.push_back(r);
            }
        }

        if (is_forced) {
            PriorityAction forced_act = forced_action_prefix[decision_step_count];
            PriorityRule r;
            r.action = forced_act;
            r.spell_id = VIPEROracle::get_spell_id(forced_act);
            r.name = VIPEROracle::get_action_name(forced_act);
            r.enabled = true;
            r.use_custom_thresholds = false;
            rules_to_evaluate.push_back(r);

            // Append fallback priority rules in case forced action cannot be cast (e.g. cooldown / resource constraint)
            for (const auto& fallback_rule : (is_oracle ? dynamic_oracle_rules : priority_rules)) {
                rules_to_evaluate.push_back(fallback_rule);
            }
        } else {
            rules_to_evaluate = is_oracle ? dynamic_oracle_rules : priority_rules;
        }

        for (const auto& rule : rules_to_evaluate) {
            if (!rule.enabled) continue;
            bool is_rule_forced = (is_forced && !rules_to_evaluate.empty() && &rule == &rules_to_evaluate[0]);

            // Parameterized Continuous Predicates (Learned from VIPER CART Decision Tree & MCTS / Custom APL)
            if (rule.use_custom_thresholds && !is_rule_forced) {
                if (rule.check_mana || rule.max_mana_pct < 0.999f || rule.min_mana_pct > 0.001f) {
                    float cur_mana_pct = static_cast<float>(player_mana / std::max(1.0, stats.max_mana));
                    if (cur_mana_pct > rule.max_mana_pct || cur_mana_pct < rule.min_mana_pct) continue;
                }
                if (rule.check_target_hp || rule.max_target_hp_pct < 0.999f || rule.min_target_hp_pct > 0.001f) {
                    float cur_target_hp = static_cast<float>(std::clamp(1.0 - (now / std::max(0.1, effective_duration)), 0.0, 1.0));
                    if (cur_target_hp > rule.max_target_hp_pct || cur_target_hp < rule.min_target_hp_pct) continue;
                }
                if (rule.check_fight_time || rule.min_time_remaining > 0.0f || rule.max_time_remaining < 9000.0f) {
                    float cur_time_rem = static_cast<float>(std::max(0.0, effective_duration - now));
                    if (cur_time_rem < rule.min_time_remaining || cur_time_rem > rule.max_time_remaining) continue;
                }
                if (rule.check_isb_debuff || rule.require_isb_active || rule.min_isb_rem_sec > 0.0f) {
                    float isb_rem = static_cast<float>(std::max(0.0, target.isb_expire_time - now));
                    bool isb_active = (target.isb_expire_time > now && isb_rem >= rule.min_isb_rem_sec && (target.isb_charges > 0 || target.isb_charges == -1));
                    if (!isb_active) continue;
                }
                if (rule.check_shadow_trance) {
                    if (!shadow_trance_active || (now >= shadow_trance_expire && shadow_trance_expire > 0.0)) continue;
                }
                if (rule.check_decimation) {
                    bool decim_active = (talents.demo.decimation > 0 && now < decimation_buff_expire);
                    if (rule.require_decimation_active && !decim_active) continue;
                    if (!rule.require_decimation_active && decim_active) continue;
                }
                if (rule.check_demonic_brand) {
                    bool brand_active = (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && now < demonic_brand_expire);
                    if (rule.require_demonic_brand_missing && brand_active) continue;
                    if (!rule.require_demonic_brand_missing && !brand_active) continue;
                }
                if (rule.check_doom_debuff) {
                    bool doom_active = (doom_tick_time > now);
                    if (rule.require_doom_missing && doom_active) continue;
                    if (!rule.require_doom_missing && !doom_active) continue;
                }
            }

            switch (rule.action) {
                case PriorityAction::RACIAL_EUREKA:
                case PriorityAction::RACIAL_BLOOD_FURY:
                case PriorityAction::RACIAL_BERSERKING:
                case PriorityAction::AMPLIFY_CURSE:
                case PriorityAction::BANE_OF_HAVOC:
                    // Handled above in off-GCD check or during Curse cast / Multi-target upkeep
                    break;

                case PriorityAction::LIFE_TAP: {
                    double mana_pct = (player_mana / stats.max_mana) * 100.0;
                    bool gnome_last_charge_tap = (race == Race::GNOME && eureka_charges == 1 && mana_pct < 70.0);
                    bool should_tap = is_rule_forced
                        ? (player_mana < stats.max_mana - 10.0)
                        : (is_oracle
                            ? (mana_pct <= 50.0)
                            : (rule.use_custom_thresholds 
                                ? (mana_pct <= static_cast<double>(rule.max_mana_pct * 100.0f) || gnome_last_charge_tap)
                                : (mana_pct <= policy.life_tap_threshold_pct || gnome_last_charge_tap)));

                    if (should_tap) {
                        log_viper_sample(rule.action, "Life Tap");
                        double health_cost = 430.0;
                        double mana_gained = (health_cost + 1.0 * stats.spirit) * (1.0 + 0.10 * talents.aff.improved_life_tap);
                        player_mana = std::min(stats.max_mana, player_mana + mana_gained);
                        player_health = std::max(1.0, player_health - health_cost);
                        result.life_taps++;
                        result.mana_gained += mana_gained;

                        // Demonic Energies: Pet gains 50%/100% of Mana gained from Life Tap
                        if (talents.demo.demonic_energies > 0 && active_pet != PetChoice::NONE) {
                            double pet_gain = mana_gained * (0.50 * talents.demo.demonic_energies);
                            pet_mana = std::min(pet_max_mana, pet_mana + pet_gain);
                        }

                        gcd_ready_time = now + mechanics.base_gcd;
                        queue.push(gcd_ready_time, EventType::GCD_READY);

                        if (record_timeline) {
                            result.timeline.push_back({now, 0.0, SpellID::LIFE_TAP, false, false, player_mana, target.isb_charges});
                            std::string note = gnome_last_charge_tap ? "Life Tap (Gnome Eureka Prep <70% Mana)" : "Mana Tap";
                            result.cast_sequence.push_back({now, SpellID::LIFE_TAP, 0.0, false, false, 0.0, note});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::CURSE_OF_AGONY: {
                    bool doom_active = (doom_tick_time > now);
                    bool should_cast_agony = !doom_active && (!dot_agony.active || (rule.use_custom_thresholds && (dot_agony.expire_time - now) <= rule.max_dot_rem_sec));
                    if (should_cast_agony) {
                        double mana_cost = 215.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Curse of Agony");
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.record_spell_cast(SpellID::CURSE_OF_AGONY);
                            apply_touch_of_the_grave(now);

                            bool is_amplified = false;
                            if (talents.aff.amplify_curse > 0 && now >= amplify_curse_cd_ready) {
                                amplify_curse_cd_ready = now + 180.0;
                                is_amplified = true;
                                result.record_spell_cast(SpellID::AMPLIFY_CURSE);
                                if (record_timeline) {
                                    result.cast_sequence.push_back({now, SpellID::AMPLIFY_CURSE, 0.0, false, false, 0.0, "Amplify Curse"});
                                }
                            }

                            bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                            if (eureka_active) eureka_charges--;

                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                dot_agony.active = true;
                                dot_agony.expire_time = now + 24.0;
                                dot_agony.ticks_remaining = 12;
                                dot_agony.tick_interval = 2.0;
                                dot_agony.amplified = is_amplified;
                                doom_tick_time = 0.0;
                                double sp = get_current_sp(School::SHADOW, now);
                                dot_agony.tick_damage = (552.0 / 12.0) + (sp * 1.596 / 12.0);
                                dot_agony.tick_multiplier = get_current_shadow_multiplier(now) * (1.0 + talents.aff.improved_bane_of_agony * 0.05) * malediction_mult * (eureka_active ? 1.10 : 1.0);
                                queue.push(now + 2.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CURSE_OF_AGONY));
                            } else {
                                result.misses++;
                                result.record_spell_miss(SpellID::CURSE_OF_AGONY);
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.timeline.push_back({now, 0.0, SpellID::CURSE_OF_AGONY, false, false, player_mana, target.isb_charges});
                                result.cast_sequence.push_back({now, SpellID::CURSE_OF_AGONY, 0.0, false, false, 0.0, (now < 1.0) ? "Opener DoT" : "Curse DoT"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::CURSE_OF_DOOM: {
                    double time_left = effective_duration - now;
                    bool should_cast_doom = !dot_agony.active && (is_oracle 
                        ? (time_left >= 60.0)
                        : (rule.use_custom_thresholds 
                            ? (time_left >= static_cast<double>(rule.min_time_remaining)) 
                            : (time_left >= 60.0 || policy.curse == CurseChoice::CURSE_OF_DOOM)));

                    if (should_cast_doom) {
                        double mana_cost = 300.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Bane of Doom");
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.record_spell_cast(SpellID::CURSE_OF_DOOM);
                            apply_touch_of_the_grave(now);
                            bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                            if (eureka_active) eureka_charges--;
                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                dot_agony.active = true;
                                dot_agony.expire_time = now + 60.0;
                                dot_agony.ticks_remaining = 1;
                                dot_agony.tick_interval = 60.0;
                                dot_agony.tick_multiplier = eureka_active ? 1.10 : 1.0;
                                doom_tick_time = now + 60.0;
                                queue.push(now + 60.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CURSE_OF_DOOM));
                            } else {
                                result.misses++;
                                result.record_spell_miss(SpellID::CURSE_OF_DOOM);
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.timeline.push_back({now, 0.0, SpellID::CURSE_OF_DOOM, false, false, player_mana, target.isb_charges});
                                result.cast_sequence.push_back({now, SpellID::CURSE_OF_DOOM, 0.0, false, false, 0.0, (now < 1.0) ? "Opener Curse" : "Bane of Doom"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::NIGHTFALL_SHADOW_BOLT: {
                    if (shadow_trance_active && policy.cast_nightfall_procs) {
                        shadow_trance_active = false;
                        double mana_cost = 380.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Nightfall Shadow Bolt");
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.shadow_bolt_casts++;
                            result.record_spell_cast(SpellID::SHADOW_BOLT);
                            apply_touch_of_the_grave(now);

                            uint16_t eureka_flag = (race == Race::GNOME && eureka_charges > 0) ? 1 : 0;
                            if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                            double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                            queue.push(now + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SHADOW_BOLT), eureka_flag);

                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.timeline.push_back({now, 0.0, SpellID::SHADOW_BOLT, false, false, player_mana, target.isb_charges});
                                result.cast_sequence.push_back({now, SpellID::SHADOW_BOLT, 0.0, false, false, 0.0, "Nightfall Instant"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DECIMATION_SEARING_PAIN: {
                    bool is_decim_target = rule.use_custom_thresholds 
                        ? ((1.0 - (now / effective_duration)) <= static_cast<double>(rule.max_target_hp_pct))
                        : execute_phase;

                    if (is_decim_target && talents.demo.decimation > 0 && now >= decimation_buff_expire) {
                        double sp_mana = 168.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= sp_mana) {
                            log_viper_sample(rule.action, "Decimation Searing Pain");
                            double cast_time = std::max(1.0, 1.5 * get_haste_mult(now));
                            is_casting = true;
                            current_casting_spell = SpellID::SEARING_PAIN;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SEARING_PAIN));
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SEARING_PAIN, 0.0, false, false, cast_time, "Decimation Proc"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DEMONIC_BRAND_SEARING_PAIN: {
                    if (talents.demo.demonic_brand > 0 && (demonic_brand_charges == 0 || now >= demonic_brand_expire)) {
                        double sp_mana = 168.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= sp_mana) {
                            log_viper_sample(rule.action, "Demonic Brand Searing Pain");
                            double cast_time = std::max(1.0, 1.5 * get_haste_mult(now));
                            is_casting = true;
                            current_casting_spell = SpellID::SEARING_PAIN;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SEARING_PAIN));
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SEARING_PAIN, 0.0, false, false, cast_time, "Demonic Brand"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DECIMATION_SOUL_FIRE: {
                    bool is_decim_target = rule.use_custom_thresholds 
                        ? ((1.0 - (now / effective_duration)) <= static_cast<double>(rule.max_target_hp_pct))
                        : execute_phase;

                    if (is_decim_target && talents.demo.decimation > 0 && now < decimation_buff_expire && now >= soul_fire_cd_ready) {
                        double sf_mana = 335.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= sf_mana) {
                            log_viper_sample(rule.action, "Decimation Soul Fire");
                            double cast_time = std::max(0.5, (6.0 - 0.4 * talents.destro.bane) * (1.0 - 0.20 * talents.demo.decimation) * get_haste_mult(now));
                            is_casting = true;
                            current_casting_spell = SpellID::SOUL_FIRE;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SOUL_FIRE));
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SOUL_FIRE, 0.0, false, false, cast_time, "Decimation Execute"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::SIPHON_LIFE: {
                    bool should_cast_sl = talents.aff.siphon_life > 0 && (!dot_siphon_life.active || (rule.use_custom_thresholds && (dot_siphon_life.expire_time - now) <= rule.max_dot_rem_sec));
                    if (should_cast_sl) {
                        double mana_cost = 365.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Siphon Life");
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.record_spell_cast(SpellID::SIPHON_LIFE);
                            apply_touch_of_the_grave(now);

                            bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                            if (eureka_active) eureka_charges--;

                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                dot_siphon_life.active = true;
                                dot_siphon_life.expire_time = now + 30.0;
                                dot_siphon_life.ticks_remaining = 10;
                                dot_siphon_life.tick_interval = 3.0;
                                double sp = get_current_sp(School::SHADOW, now);
                                dot_siphon_life.tick_damage = 41.0 + (0.05 * sp);
                                dot_siphon_life.tick_multiplier = get_current_shadow_multiplier(now) * malediction_mult * (eureka_active ? 1.10 : 1.0);
                                queue.push(now + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::SIPHON_LIFE));
                            } else {
                                result.misses++;
                                result.record_spell_miss(SpellID::SIPHON_LIFE);
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.timeline.push_back({now, 0.0, SpellID::SIPHON_LIFE, false, false, player_mana, target.isb_charges});
                                result.cast_sequence.push_back({now, SpellID::SIPHON_LIFE, 0.0, false, false, 0.0, (now < 3.0) ? "Opener DoT" : "DoT Refresh"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DRAIN_HOPE: {
                    if (talents.aff.drain_hope > 0 && now >= drain_hope_channel_end) {
                        double dh_mana = 240.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= dh_mana) {
                            log_viper_sample(rule.action, "Drain Hope");
                            player_mana -= dh_mana;
                            result.mana_spent += dh_mana;
                            result.total_casts++;
                            result.record_spell_cast(SpellID::DRAIN_HOPE);
                            apply_touch_of_the_grave(now);
                            drain_hope_channel_end = now + 6.0;
                            drain_hope_cd_ready = now + 6.0;
                            if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                            if (mechanics.instant_drain_hope) {
                                gcd_ready_time = now + mechanics.base_gcd;
                                for (int i = 1; i <= 6; ++i) {
                                    queue.push(now + i * 1.0, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_HOPE));
                                }
                                queue.push(gcd_ready_time, EventType::GCD_READY);
                                if (record_timeline) {
                                    result.timeline.push_back({now, 0.0, SpellID::DRAIN_HOPE, false, false, player_mana, target.isb_charges});
                                    result.cast_sequence.push_back({now, SpellID::DRAIN_HOPE, 0.0, false, false, 0.0, "Instant DoT"});
                                }
                                return;
                            } else {
                                double haste = get_haste_mult(now);
                                double total_channel_time = 6.0 * haste;
                                double tick_interval = 1.0 * haste;
                                drain_hope_channel_end = now + total_channel_time;
                                drain_hope_cd_ready = now + total_channel_time;

                                gcd_ready_time = now + std::max(mechanics.base_gcd, total_channel_time);

                                for (int i = 1; i <= 6; ++i) {
                                    queue.push(now + i * tick_interval, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_HOPE));
                                }
                                queue.push(gcd_ready_time, EventType::GCD_READY);
                                if (record_timeline) {
                                    result.timeline.push_back({now, 0.0, SpellID::DRAIN_HOPE, false, false, player_mana, target.isb_charges});
                                    result.cast_sequence.push_back({now, SpellID::DRAIN_HOPE, 0.0, false, false, total_channel_time, "Channel DoT"});
                                }
                                return;
                            }
                        }
                    }
                    break;
                }

                case PriorityAction::IMMOLATE: {
                    bool should_cast_immo = !dot_immolate.active || (rule.use_custom_thresholds && (dot_immolate.expire_time - now) <= rule.max_dot_rem_sec);
                    if (should_cast_immo) {
                        double mana_cost = 380.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Immolate");
                            double cast_time = std::max(1.0, (2.0 - 0.1 * talents.destro.bane) * get_haste_mult(now)); // 1.5s with 5/5 Bane
                            is_casting = true;
                            current_casting_spell = SpellID::IMMOLATE;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::IMMOLATE));
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::IMMOLATE, 0.0, false, false, cast_time, (now < 5.0) ? "Opener DoT" : "DoT Refresh"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::CONFLAGRATE: {
                    if (talents.destro.conflagrate > 0 && dot_immolate.active && now >= conflagrate_cd_ready) {
                        double mana_cost = 265.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Conflagrate");
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.direct_spell_casts++;
                            result.record_spell_cast(SpellID::CONFLAGRATE);
                            apply_touch_of_the_grave(now);
                            conflagrate_cd_ready = now + 10.0;

                            bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                            if (eureka_active) eureka_charges--;

                            bool is_crit = false;
                            double dmg = 0.0;
                            if (rng.chance(calculate_hit_chance(School::FIRE))) {
                                result.total_damage_events++;
                                double sp = get_current_sp(School::FIRE, now);
                                dmg = rng.range(306.0, 374.0) + (1.5 / 3.5) * sp;
                                dmg *= get_current_fire_multiplier(now) * stats.all_damage_multiplier * destro_spell_mult;

                                double crit_chance = calculate_crit_chance(School::FIRE, stats) + fnb_crit_bonus;
                                is_crit = rng.chance(crit_chance);
                                if (is_crit) {
                                    dmg *= destro_crit_mult;
                                    result.direct_spell_crits++;
                                    result.total_damage_crits++;
                                    result.crits++;
                                }

                                dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);
                                if (race == Race::GNOME && eureka_active) { dmg *= 1.10; }
                                if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                                result.dmg_conflagrate += dmg;
                                result.record_spell_hit(SpellID::CONFLAGRATE, dmg, is_crit);
                                result.total_damage += dmg;
                                if (record_timeline) {
                                    result.timeline.push_back({now, dmg, SpellID::CONFLAGRATE, is_crit, false, player_mana, target.isb_charges});
                                }
                                apply_havoc_cleave(dmg, 0);

                                if (talents.destro.shadow_and_flame > 0) {
                                    shadow_and_flame_shadow_expire = now + 20.0;
                                }
                            } else {
                                result.misses++;
                                result.record_spell_miss(SpellID::CONFLAGRATE);
                            }

                            bool consume_immolate = true;
                            if (talents.destro.shadow_and_flame > 0) {
                                if (rng.chance(talents.destro.shadow_and_flame * 0.20)) {
                                    consume_immolate = false;
                                }
                            }
                            if (consume_immolate) {
                                dot_immolate.active = false;
                                dot_immolate.ticks_remaining = 0;
                            }

                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::CONFLAGRATE, dmg, is_crit, (dmg == 0.0), 0.0, "Burst"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::SHADOWBURN:
                case PriorityAction::SHADOWBURN_ISB: {
                    bool sb_cond = true;
                    if (!is_oracle && !policy.use_custom_apl) {
                        if (eff_rotation == RotationChoice::FIRE_DESTRO || eff_rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2) {
                            sb_cond = (talents.destro.shadow_and_flame > 0 && now >= shadow_and_flame_fire_expire) || (policy.shadowburn == ShadowburnPolicy::ON_COOLDOWN);
                        } else if (policy.shadowburn == ShadowburnPolicy::EXECUTE_ONLY) {
                            sb_cond = execute_phase;
                        }
                    }

                    if (talents.destro.shadowburn > 0 && sb_cond && now >= shadowburn_cd_ready) {
                        double mana_cost = 365.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Shadowburn");
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.direct_spell_casts++;
                            result.record_spell_cast(SpellID::SHADOWBURN);
                            apply_touch_of_the_grave(now);
                            shadowburn_cd_ready = now + 8.0;

                            bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                            if (eureka_active) eureka_charges--;

                            bool crit = false;
                            double dmg = 0.0;
                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                result.total_damage_events++;
                                double sp = get_current_sp(School::SHADOW, now);
                                dmg = rng.range(238.0, 266.0) + (1.5 / 3.5) * sp;
                                dmg *= get_current_shadow_multiplier(now) * stats.all_damage_multiplier * destro_spell_mult;

                                if (target.consume_isb_charge(now)) {
                                    dmg *= (1.0 + target.isb_bonus);
                                    result.isb_consumed++;
                                }

                                crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                                if (crit) {
                                    dmg *= destro_crit_mult;
                                    result.direct_spell_crits++;
                                    result.total_damage_crits++;
                                    result.crits++;
                                }

                                dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                                if (race == Race::GNOME && eureka_active) { dmg *= 1.10; }
                                if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                                result.dmg_shadowburn += dmg;
                                result.record_spell_hit(SpellID::SHADOWBURN, dmg, crit);
                                result.total_damage += dmg;
                                if (record_timeline) {
                                    result.timeline.push_back({now, dmg, SpellID::SHADOWBURN, crit, false, player_mana, target.isb_charges});
                                }
                                apply_havoc_cleave(dmg, 0);

                                if (talents.destro.shadow_and_flame > 0) {
                                    shadow_and_flame_fire_expire = now + 20.0;
                                }
                            } else {
                                result.misses++;
                                result.record_spell_miss(SpellID::SHADOWBURN);
                            }

                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SHADOWBURN, dmg, crit, (dmg == 0.0), 0.0, "Burst Cooldown"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::CORRUPTION: {
                    bool should_cast_corr = !dot_corruption.active || (rule.use_custom_thresholds && (dot_corruption.expire_time - now) <= rule.max_dot_rem_sec);
                    if (should_cast_corr) {
                        double mana_cost = 340.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= mana_cost) {
                            log_viper_sample(rule.action, "Corruption");
                            double cast_time = std::max(0.0, (2.0 - 0.4 * talents.aff.improved_corruption) * get_haste_mult(now));
                            if (cast_time == 0.0) {
                                player_mana -= mana_cost;
                                result.mana_spent += mana_cost;
                                result.total_casts++;
                                result.record_spell_cast(SpellID::CORRUPTION);
                                apply_touch_of_the_grave(now);
                                bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                                if (eureka_active) eureka_charges--;

                                if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                    dot_corruption.active = true;
                                    dot_corruption.expire_time = now + 18.0;
                                    dot_corruption.ticks_remaining = 6;
                                    dot_corruption.tick_interval = 3.0;
                                    double sp = get_current_sp(School::SHADOW, now);
                                    dot_corruption.tick_damage = 73.0 + (sp * mechanics.corruption_sp_coefficient * 0.20);
                                    dot_corruption.tick_multiplier = get_current_shadow_multiplier(now) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult * (eureka_active ? 1.10 : 1.0);
                                    queue.push(now + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION));
                                } else {
                                    result.misses++;
                                    result.record_spell_miss(SpellID::CORRUPTION);
                                }
                                gcd_ready_time = now + mechanics.base_gcd;
                                queue.push(gcd_ready_time, EventType::GCD_READY);
                                if (record_timeline) {
                                    result.timeline.push_back({now, 0.0, SpellID::CORRUPTION, false, false, player_mana, target.isb_charges});
                                    result.cast_sequence.push_back({now, SpellID::CORRUPTION, 0.0, false, false, 0.0, (now < 3.0) ? "Opener DoT" : "DoT Refresh"});
                                }
                                return;
                            } else {
                                is_casting = true;
                                current_casting_spell = SpellID::CORRUPTION;
                                cast_finish_time = now + cast_time;
                                queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::CORRUPTION));
                                gcd_ready_time = now + mechanics.base_gcd;
                                queue.push(gcd_ready_time, EventType::GCD_READY);
                                if (record_timeline) {
                                    result.cast_sequence.push_back({now, SpellID::CORRUPTION, 0.0, false, false, cast_time, (now < 3.0) ? "Opener DoT" : "DoT Refresh"});
                                }
                                return;
                            }
                        }
                    }
                    break;
                }


                case PriorityAction::SEARING_PAIN_FILLER: {
                    double sp_mana = 168.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    if (player_mana >= sp_mana) {
                        log_viper_sample(rule.action, "Searing Pain");
                        double cast_time = std::max(1.0, 1.5 * get_haste_mult(now));
                        is_casting = true;
                        current_casting_spell = SpellID::SEARING_PAIN;
                        cast_finish_time = now + cast_time;
                        queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SEARING_PAIN));
                        gcd_ready_time = now + mechanics.base_gcd;
                        queue.push(gcd_ready_time, EventType::GCD_READY);
                        if (record_timeline) {
                            result.cast_sequence.push_back({now, SpellID::SEARING_PAIN, 0.0, false, false, cast_time, "Primary Fire Filler"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::INCINERATE_FILLER: {
                    if (talents.destro.incinerate > 0) {
                        double inc_mana = 325.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                        if (player_mana >= inc_mana) {
                            log_viper_sample(rule.action, "Incinerate");
                            double cast_time = std::max(1.0, (2.5 - 0.1 * talents.destro.bane) * get_haste_mult(now)); // 2.0s with 5/5 Bane
                            is_casting = true;
                            current_casting_spell = SpellID::INCINERATE;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::INCINERATE));
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::INCINERATE, 0.0, false, false, cast_time, "Primary Filler"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DRAIN_LIFE_FILLER: {
                    double dl_mana = 300.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    if (player_mana >= dl_mana) {
                        log_viper_sample(rule.action, "Drain Life");
                        player_mana -= dl_mana;
                        result.mana_spent += dl_mana;
                        result.total_casts++;
                        result.record_spell_cast(SpellID::DRAIN_LIFE);
                        apply_touch_of_the_grave(now);
                        if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                        double haste = get_haste_mult(now);
                        double total_channel_time = 5.0 * haste;
                        double tick_interval = 1.0 * haste;

                        gcd_ready_time = now + std::max(mechanics.base_gcd, total_channel_time);

                        for (int i = 1; i <= 5; ++i) {
                            queue.push(now + i * tick_interval, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_LIFE));
                        }
                        queue.push(gcd_ready_time, EventType::GCD_READY);
                        if (record_timeline) {
                            result.timeline.push_back({now, 0.0, SpellID::DRAIN_LIFE, false, false, player_mana, target.isb_charges});
                            result.cast_sequence.push_back({now, SpellID::DRAIN_LIFE, 0.0, false, false, total_channel_time, "Drain Life (Filler)"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::DRAIN_SOUL_FILLER: {
                    double ds_mana = 290.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    if (player_mana >= ds_mana) {
                        log_viper_sample(rule.action, "Drain Soul");
                        player_mana -= ds_mana;
                        result.mana_spent += ds_mana;
                        result.total_casts++;
                        result.record_spell_cast(SpellID::DRAIN_SOUL);
                        apply_touch_of_the_grave(now);
                        if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                        double haste = get_haste_mult(now);
                        double total_channel_time = 15.0 * haste;
                        double tick_interval = 3.0 * haste;

                        gcd_ready_time = now + std::max(mechanics.base_gcd, total_channel_time);

                        for (int i = 1; i <= 5; ++i) {
                            queue.push(now + i * tick_interval, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_SOUL));
                        }
                        queue.push(gcd_ready_time, EventType::GCD_READY);
                        if (record_timeline) {
                            result.timeline.push_back({now, 0.0, SpellID::DRAIN_SOUL, false, false, player_mana, target.isb_charges});
                            result.cast_sequence.push_back({now, SpellID::DRAIN_SOUL, 0.0, false, false, total_channel_time, "Drain Soul (Filler)"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::SHADOW_BOLT_FILLER: {
                    double sb_mana = 380.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    if (player_mana >= sb_mana) {
                        log_viper_sample(rule.action, "Shadow Bolt");
                        double cast_time = std::max(1.0, (3.0 - 0.1 * talents.destro.bane) * get_haste_mult(now)); // 2.5s with 5/5 Bane
                        is_casting = true;
                        current_casting_spell = SpellID::SHADOW_BOLT;
                        cast_finish_time = now + cast_time;
                        queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
                        gcd_ready_time = now + mechanics.base_gcd;
                        queue.push(gcd_ready_time, EventType::GCD_READY);
                        if (record_timeline) {
                            result.cast_sequence.push_back({now, SpellID::SHADOW_BOLT, 0.0, false, false, cast_time, "Primary Filler"});
                        }
                        return;
                    }
                    break;
                }
            }
        }

        // Emergency Resource Fallback: Life Tap when out of mana for filler
        double health_cost = 430.0;
        double mana_gained = (health_cost + 1.0 * stats.spirit) * (1.0 + 0.10 * talents.aff.improved_life_tap);
        player_mana = std::min(stats.max_mana, player_mana + mana_gained);
        player_health -= health_cost;
        result.life_taps++;
        result.mana_gained += mana_gained;

        // Demonic Energies: Pet gains 50%/100% of Mana gained from Life Tap
        if (talents.demo.demonic_energies > 0 && active_pet != PetChoice::NONE) {
            double pet_gain = mana_gained * (0.50 * talents.demo.demonic_energies);
            pet_mana = std::min(pet_max_mana, pet_mana + pet_gain);
        }

        gcd_ready_time = now + mechanics.base_gcd;
        queue.push(gcd_ready_time, EventType::GCD_READY);

        if (record_timeline) {
            result.timeline.push_back({now, 0.0, SpellID::LIFE_TAP, false, false, player_mana, target.isb_charges});
            result.cast_sequence.push_back({now, SpellID::LIFE_TAP, 0.0, false, false, 0.0, "Mana Tap"});
        }
    };


    // Trigger initial action at t = 0
    decide_next_action(0.0);

    // Main Discrete Event Simulation loop
    while (!queue.empty()) {
        Event ev = queue.pop();
        current_time = ev.time;
        target.update_isb_uptime(current_time);

        if (ev.type == EventType::SIMULATION_END) {
            break;
        }

        switch (ev.type) {
            case EventType::CAST_FINISH: {
                is_casting = false;
                result.total_casts++;

                if (ev.spell_id == static_cast<uint8_t>(SpellID::SHADOW_BOLT)) {
                    result.shadow_bolt_casts++;
                    result.direct_spell_casts++;
                    result.record_spell_cast(SpellID::SHADOW_BOLT);
                    apply_touch_of_the_grave(current_time);
                    double sb_mana = 380.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    player_mana -= sb_mana;
                    result.mana_spent += sb_mana;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, 0.0, SpellID::SHADOW_BOLT, false, false, player_mana, target.isb_charges});
                    }
                    uint16_t eureka_flag = (race == Race::GNOME && eureka_charges > 0) ? 1 : 0;
                    if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                    if ((current_time / fight_duration) >= 0.65 && talents.demo.decimation > 0) {
                        decimation_buff_expire = current_time + 10.0;
                    }

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SHADOW_BOLT), eureka_flag);
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SEARING_PAIN)) {
                    result.direct_spell_casts++;
                    result.record_spell_cast(SpellID::SEARING_PAIN);
                    apply_touch_of_the_grave(current_time);
                    double sp_mana = 168.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    player_mana -= sp_mana;
                    result.mana_spent += sp_mana;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, 0.0, SpellID::SEARING_PAIN, false, false, player_mana, target.isb_charges});
                    }
                    uint16_t eureka_flag = (race == Race::GNOME && eureka_charges > 0) ? 1 : 0;
                    if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                    if ((current_time / fight_duration) >= 0.65 && talents.demo.decimation > 0) {
                        decimation_buff_expire = current_time + 10.0;
                    }

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SEARING_PAIN), eureka_flag);
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::INCINERATE)) {
                    result.direct_spell_casts++;
                    result.record_spell_cast(SpellID::INCINERATE);
                    apply_touch_of_the_grave(current_time);
                    double inc_mana = 355.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    player_mana -= inc_mana;
                    result.mana_spent += inc_mana;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, 0.0, SpellID::INCINERATE, false, false, player_mana, target.isb_charges});
                    }
                    uint16_t eureka_flag = (race == Race::GNOME && eureka_charges > 0) ? 1 : 0;
                    if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::INCINERATE), eureka_flag);
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SOUL_FIRE)) {
                    result.direct_spell_casts++;
                    result.record_spell_cast(SpellID::SOUL_FIRE);
                    apply_touch_of_the_grave(current_time);
                    double sf_mana = 335.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    player_mana -= sf_mana;
                    result.mana_spent += sf_mana;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, 0.0, SpellID::SOUL_FIRE, false, false, player_mana, target.isb_charges});
                    }
                    soul_fire_cd_ready = current_time + (60.0 * (1.0 - 0.45 * talents.demo.decimation));
                    uint16_t eureka_flag = (race == Race::GNOME && eureka_charges > 0) ? 1 : 0;
                    if (race == Race::GNOME && eureka_charges > 0) eureka_charges--;

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SOUL_FIRE), eureka_flag);
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::IMMOLATE)) {
                    double imm_mana = 380.0 * cataclysm_mana_mult * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    player_mana -= imm_mana;
                    result.mana_spent += imm_mana;
                    result.direct_spell_casts++;
                    result.record_spell_cast(SpellID::IMMOLATE);
                    apply_touch_of_the_grave(current_time);
                    bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                    if (eureka_active) eureka_charges--;

                    if (rng.chance(calculate_hit_chance(School::FIRE))) {
                        result.total_damage_events++;
                        double sp = get_current_sp(School::FIRE, current_time);
                        // Aftermath (Destro Row 2 Col 3): +10% initial Immolate damage per point (+50% at 5/5)
                        double aftermath_mult = 1.0 + talents.destro.aftermath * 0.10;
                        double dmg = (158.0 * aftermath_mult) + 0.20 * sp;
                        dmg *= get_current_fire_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;
                        bool crit = rng.chance(calculate_crit_chance(School::FIRE, stats));
                        if (crit) {
                            dmg *= destro_crit_mult;
                            result.direct_spell_crits++;
                            result.total_damage_crits++;
                            result.crits++;
                        }
                        dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                        if (race == Race::GNOME && eureka_active) { dmg *= 1.10; }
                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }

                        result.dmg_immolate += dmg;
                        result.record_spell_hit(SpellID::IMMOLATE, dmg, crit);
                        result.total_damage += dmg;
                        apply_havoc_cleave(dmg, 0);

                        if (record_timeline) {
                            result.timeline.push_back({current_time, dmg, SpellID::IMMOLATE, crit, false, player_mana, target.isb_charges});
                        }

                        dot_immolate.active = true;
                        dot_immolate.expire_time = current_time + 15.0;
                        dot_immolate.ticks_remaining = 5;
                        dot_immolate.tick_interval = 3.0;
                        dot_immolate.tick_damage = 55.0 + 0.13 * sp;
                        dot_immolate.tick_multiplier = get_current_fire_multiplier(current_time) * destro_spell_mult * malediction_mult * (eureka_active ? 1.10 : 1.0);
                        queue.push(current_time + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::IMMOLATE));
                    } else {
                        result.misses++;
                        result.record_spell_miss(SpellID::IMMOLATE);
                        if (record_timeline) {
                            result.timeline.push_back({current_time, 0.0, SpellID::IMMOLATE, false, true, player_mana, target.isb_charges});
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::CORRUPTION)) {
                    double corr_mana = 340.0 * (race == Race::GNOME && eureka_charges > 0 ? 0.5 : 1.0);
                    player_mana -= corr_mana;
                    result.mana_spent += corr_mana;
                    result.record_spell_cast(SpellID::CORRUPTION);
                    apply_touch_of_the_grave(current_time);
                    if (record_timeline) {
                        result.timeline.push_back({current_time, 0.0, SpellID::CORRUPTION, false, false, player_mana, target.isb_charges});
                    }
                    bool eureka_active = (race == Race::GNOME && eureka_charges > 0);
                    if (eureka_active) eureka_charges--;

                    uint32_t t_idx = ev.user_data;
                    if (t_idx >= static_cast<uint32_t>(num_targets)) t_idx = 0;
                    ActiveDot& cur_corr = target_states[t_idx].dot_corruption;

                    if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                        cur_corr.active = true;
                        cur_corr.expire_time = current_time + 18.0;
                        cur_corr.ticks_remaining = 6;
                        cur_corr.tick_interval = 3.0;
                        double sp = get_current_sp(School::SHADOW, current_time);
                        cur_corr.tick_damage = 73.0 + (sp * mechanics.corruption_sp_coefficient * 0.20);
                        cur_corr.tick_multiplier = get_current_shadow_multiplier(current_time) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult * (eureka_active ? 1.10 : 1.0);
                        queue.push(current_time + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION), 0, t_idx);
                    } else {
                        result.misses++;
                        result.record_spell_miss(SpellID::CORRUPTION);
                    }
                }

                decide_next_action(current_time);
                break;
            }

            case EventType::SPELL_IMPACT: {
                if (ev.spell_id == static_cast<uint8_t>(SpellID::SHADOW_BOLT)) {
                    if (!rng.chance(calculate_hit_chance(School::SHADOW))) {
                        result.misses++;
                        result.record_spell_miss(SpellID::SHADOW_BOLT);
                        if (record_timeline) {
                            result.timeline.push_back({current_time, 0.0, SpellID::SHADOW_BOLT, false, true, player_mana, target.isb_charges});
                            for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                                if (it->spell_id == SpellID::SHADOW_BOLT && it->damage == 0.0 && !it->is_miss) {
                                    it->is_miss = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }

                    result.shadow_bolt_hits++;
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double base_dmg = rng.range(253.0, 283.0);
                    double dmg = base_dmg + (3.0 / 3.5) * sp;

                    // Decimation bonus: +3% per point on Shadow Bolt when boss <35% HP
                    bool execute_phase = (current_time / fight_duration) >= 0.65;
                    if (execute_phase && talents.demo.decimation > 0) {
                        dmg *= (1.0 + talents.demo.decimation * 0.03);
                    }

                    // ISB check
                    if (target.consume_isb_charge(current_time)) {
                        dmg *= (1.0 + target.isb_bonus);
                        result.isb_consumed++;
                    }

                    dmg *= get_current_shadow_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;

                    // Crit roll
                    bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                    if (is_crit) {
                        result.shadow_bolt_crits++;
                        result.direct_spell_crits++;
                        result.total_damage_crits++;
                        result.crits++;
                        dmg *= destro_crit_mult;
                        // Improved Shadow Bolt talent: Apply 12s buff (charges if Classic, unlimited if Forever)
                        if (talents.destro.improved_shadow_bolt > 0) {
                            target.apply_isb(current_time, talents.destro.improved_shadow_bolt, mechanics.isb_has_charges);
                            result.isb_procs++;
                        }
                    }

                    // Partial resist roll
                    double resist_mult = calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                    if (resist_mult < 1.0) result.partial_resists++;
                    dmg *= resist_mult;

                    if (race == Race::GNOME && ev.sub_id == 1) { dmg *= 1.10; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }

                    // Judgement of Wisdom
                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_shadow_bolt += dmg;
                    result.record_spell_hit(SpellID::SHADOW_BOLT, dmg, is_crit);
                    result.total_damage += dmg;
                    apply_havoc_cleave(dmg, 0);

                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::SHADOW_BOLT, is_crit, false, player_mana, target.isb_charges});
                        for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                            if (it->spell_id == SpellID::SHADOW_BOLT && it->damage == 0.0 && !it->is_miss) {
                                it->damage = dmg;
                                it->is_crit = is_crit;
                                break;
                            }
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::INCINERATE)) {
                    if (!rng.chance(calculate_hit_chance(School::FIRE))) {
                        result.misses++;
                        result.record_spell_miss(SpellID::INCINERATE);
                        if (record_timeline) {
                            result.timeline.push_back({current_time, 0.0, SpellID::INCINERATE, false, true, player_mana, target.isb_charges});
                            for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                                if (it->spell_id == SpellID::INCINERATE && it->damage == 0.0 && !it->is_miss) {
                                    it->is_miss = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }

                    result.total_damage_events++;
                    double sp = get_current_sp(School::FIRE, current_time);
                    double base_dmg = rng.range(201.0, 233.0);
                    double dmg = base_dmg + (2.5 / 3.5) * sp;

                    // +25% bonus damage if Immolate is active on target
                    if (dot_immolate.active) {
                        dmg *= 1.25;
                    }

                    dmg *= get_current_fire_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;

                    bool is_crit = rng.chance(calculate_crit_chance(School::FIRE, stats));
                    if (is_crit) {
                        dmg *= destro_crit_mult;
                        result.direct_spell_crits++;
                        result.total_damage_crits++;
                        result.crits++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                    if (race == Race::GNOME && ev.sub_id == 1) { dmg *= 1.10; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }

                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_incinerate += dmg;
                    result.record_spell_hit(SpellID::INCINERATE, dmg, is_crit);
                    result.total_damage += dmg;
                    apply_havoc_cleave(dmg, 0);

                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::INCINERATE, is_crit, false, player_mana, target.isb_charges});
                        for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                            if (it->spell_id == SpellID::INCINERATE && it->damage == 0.0 && !it->is_miss) {
                                it->damage = dmg;
                                it->is_crit = is_crit;
                                break;
                            }
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SEARING_PAIN)) {
                    if (!rng.chance(calculate_hit_chance(School::FIRE))) {
                        result.misses++;
                        result.record_spell_miss(SpellID::SEARING_PAIN);
                        if (record_timeline) {
                            result.timeline.push_back({current_time, 0.0, SpellID::SEARING_PAIN, false, true, player_mana, target.isb_charges});
                            for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                                if (it->spell_id == SpellID::SEARING_PAIN && it->damage == 0.0 && !it->is_miss) {
                                    it->is_miss = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }

                    result.total_damage_events++;
                    double sp = get_current_sp(School::FIRE, current_time);
                    double base_dmg = rng.range(108.0, 127.0);
                    double dmg = base_dmg + (1.5 / 3.5) * sp;

                    // Decimation bonus: +3% per point on Searing Pain when boss <35% HP
                    bool execute_phase = (current_time / fight_duration) >= 0.65;
                    if (execute_phase && talents.demo.decimation > 0) {
                        dmg *= (1.0 + talents.demo.decimation * 0.03);
                    }

                    dmg *= get_current_fire_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;

                    // Agonizing Flames: +3% / +7% / +10% crit chance on Searing Pain
                    double crit_chance = calculate_crit_chance(School::FIRE, stats) + agonizing_flames_bonus;
                    bool is_crit = rng.chance(crit_chance);
                    if (is_crit) {
                        dmg *= destro_crit_mult;
                        result.direct_spell_crits++;
                        result.total_damage_crits++;
                        result.crits++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                    // Demonic Brand: Brands the target for 10s (pet's next 2/4/6 attacks deal bonus damage)
                    if (talents.demo.demonic_brand > 0) {
                        demonic_brand_charges = talents.demo.demonic_brand * 2;
                        demonic_brand_expire = current_time + 10.0;
                    }

                    if (race == Race::GNOME && ev.sub_id == 1) { dmg *= 1.10; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }

                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_searing_pain += dmg;
                    result.record_spell_hit(SpellID::SEARING_PAIN, dmg, is_crit);
                    result.total_damage += dmg;
                    apply_havoc_cleave(dmg, 0);

                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::SEARING_PAIN, is_crit, false, player_mana, target.isb_charges});
                        for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                            if (it->spell_id == SpellID::SEARING_PAIN && it->damage == 0.0 && !it->is_miss) {
                                it->damage = dmg;
                                it->is_crit = is_crit;
                                break;
                            }
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SOUL_FIRE)) {
                    if (!rng.chance(calculate_hit_chance(School::FIRE))) {
                        result.misses++;
                        result.record_spell_miss(SpellID::SOUL_FIRE);
                        if (record_timeline) {
                            result.timeline.push_back({current_time, 0.0, SpellID::SOUL_FIRE, false, true, player_mana, target.isb_charges});
                            for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                                if (it->spell_id == SpellID::SOUL_FIRE && it->damage == 0.0 && !it->is_miss) {
                                    it->is_miss = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }

                    result.total_damage_events++;
                    double sp = get_current_sp(School::FIRE, current_time);
                    double base_dmg = rng.range(383.0, 479.0);
                    double dmg = base_dmg + 1.0 * sp;
                    dmg *= get_current_fire_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;

                    bool is_crit = rng.chance(calculate_crit_chance(School::FIRE, stats));
                    if (is_crit) {
                        dmg *= destro_crit_mult;
                        result.direct_spell_crits++;
                        result.total_damage_crits++;
                        result.crits++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                    if (race == Race::GNOME && ev.sub_id == 1) { dmg *= 1.10; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }

                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_soul_fire += dmg;
                    result.record_spell_hit(SpellID::SOUL_FIRE, dmg, is_crit);
                    result.total_damage += dmg;
                    apply_havoc_cleave(dmg, 0);

                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::SOUL_FIRE, is_crit, false, player_mana, target.isb_charges});
                        for (auto it = result.cast_sequence.rbegin(); it != result.cast_sequence.rend(); ++it) {
                            if (it->spell_id == SpellID::SOUL_FIRE && it->damage == 0.0 && !it->is_miss) {
                                it->damage = dmg;
                                it->is_crit = is_crit;
                                break;
                            }
                        }
                    }
                }
                break;
            }

            case EventType::CHANNEL_TICK: {
                // Determine active Affliction effects count on target for Soul Siphon
                // (Corruption, Bane of Agony, Siphon Life, Curse of Shadows/Elements/Doom if applicable, etc.)
                int aff_effects_count = (dot_corruption.active ? 1 : 0) + 
                                       (dot_agony.active ? 1 : 0) + 
                                       (dot_siphon_life.active ? 1 : 0);
                // Soul Siphon: +4% / +8% / +12% per active Affliction effect, up to 3 effects (max +12% / +24% / +36%)
                double soul_siphon_bonus_per_effect = talents.aff.soul_siphon * 0.04;
                double soul_siphon_mult = 1.0 + std::min(3, aff_effects_count) * soul_siphon_bonus_per_effect;

                // Improved Drains: +7% / +13% / +20% flat damage / healing
                double imp_drains_mult = 1.0;
                if (talents.aff.improved_drains == 1) imp_drains_mult = 1.07;
                else if (talents.aff.improved_drains == 2) imp_drains_mult = 1.13;
                else if (talents.aff.improved_drains >= 3) imp_drains_mult = 1.20;

                if (ev.spell_id == static_cast<uint8_t>(SpellID::DRAIN_HOPE)) {
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double dmg = (212.0 / 6.0) + ((1.0 / 6.0) * sp);
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * imp_drains_mult * soul_siphon_mult;

                    // Baseline DoT Crit + Pandemic bonus (Affliction)
                    bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                    if (is_crit) {
                        result.total_damage_crits++;
                        result.crits++;
                        double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                        dmg *= pand_crit_mult;
                    }

                    if ((!mechanics.isb_has_charges || mechanics.isb_all_shadow_sources) && target.consume_isb_charge(current_time)) {
                        dmg *= (1.0 + target.isb_bonus);
                        result.isb_consumed++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    result.dmg_drain_hope += dmg;
                    result.record_spell_hit(SpellID::DRAIN_HOPE, dmg, is_crit);
                    result.total_damage += dmg;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::DRAIN_HOPE, is_crit, false, player_mana, target.isb_charges});
                    }
                    apply_havoc_cleave(dmg, 0);

                    // Nightfall proc check on Wrack ticks (2% per pt = 4% at 2/2)
                    if (mechanics.nightfall_enabled && talents.aff.nightfall > 0) {
                        double p = talents.aff.nightfall * 0.02;
                        if (rng.chance(p)) {
                            shadow_trance_active = true;
                            shadow_trance_expire = current_time + 10.0;
                            result.nightfall_procs++;
                            queue.push(shadow_trance_expire, EventType::BUFF_EXPIRE, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::DRAIN_LIFE)) {
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double dmg = 71.0 + (0.10 * sp);
                    dmg *= imp_drains_mult * soul_siphon_mult;

                    // Wrack amplification (+10% to other Shadow DoTs/drains)
                    double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                    // Baseline DoT Crit + Pandemic bonus (Affliction)
                    bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                    if (is_crit) {
                        result.total_damage_crits++;
                        result.crits++;
                        double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                        dmg *= pand_crit_mult;
                    }

                    if ((!mechanics.isb_has_charges || mechanics.isb_all_shadow_sources) && target.consume_isb_charge(current_time)) {
                        dmg *= (1.0 + target.isb_bonus);
                        result.isb_consumed++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    result.dmg_drain_life += dmg;
                    result.record_spell_hit(SpellID::DRAIN_LIFE, dmg, is_crit);
                    result.total_damage += dmg;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::DRAIN_LIFE, is_crit, false, player_mana, target.isb_charges});
                    }
                    apply_havoc_cleave(dmg, 0);

                    // Health restored from Drain Life
                    double heal = dmg;
                    player_health = std::min(stats.max_health, player_health + heal);

                    // Nightfall proc check on Drain Life ticks (2% per pt = 4% at 2/2)
                    if (mechanics.nightfall_enabled && talents.aff.nightfall > 0) {
                        double p = talents.aff.nightfall * 0.02;
                        if (rng.chance(p)) {
                            shadow_trance_active = true;
                            shadow_trance_expire = current_time + 10.0;
                            result.nightfall_procs++;
                            queue.push(shadow_trance_expire, EventType::BUFF_EXPIRE, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::DRAIN_SOUL)) {
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double dmg = 91.0 + (0.20 * sp);
                    dmg *= imp_drains_mult * soul_siphon_mult;

                    // Wrack amplification (+10% to other Shadow DoTs/drains)
                    double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                    // Baseline DoT Crit + Pandemic bonus (Affliction)
                    bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                    if (is_crit) {
                        result.total_damage_crits++;
                        result.crits++;
                        double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                        dmg *= pand_crit_mult;
                    }

                    if ((!mechanics.isb_has_charges || mechanics.isb_all_shadow_sources) && target.consume_isb_charge(current_time)) {
                        dmg *= (1.0 + target.isb_bonus);
                        result.isb_consumed++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    result.dmg_drain_soul += dmg;
                    result.record_spell_hit(SpellID::DRAIN_SOUL, dmg, is_crit);
                    result.total_damage += dmg;
                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::DRAIN_SOUL, is_crit, false, player_mana, target.isb_charges});
                    }
                    apply_havoc_cleave(dmg, 0);

                    // Nightfall proc check on Drain Soul ticks (2% per pt = 4% at 2/2)
                    if (mechanics.nightfall_enabled && talents.aff.nightfall > 0) {
                        double p = talents.aff.nightfall * 0.02;
                        if (rng.chance(p)) {
                            shadow_trance_active = true;
                            shadow_trance_expire = current_time + 10.0;
                            result.nightfall_procs++;
                            queue.push(shadow_trance_expire, EventType::BUFF_EXPIRE, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
                        }
                    }
                }
                break;
            }

            case EventType::DOT_TICK: {
                uint32_t t_idx = ev.user_data;
                if (t_idx >= static_cast<uint32_t>(num_targets)) t_idx = 0;

                if (ev.spell_id == static_cast<uint8_t>(SpellID::CORRUPTION)) {
                    ActiveDot& cur_corr = target_states[t_idx].dot_corruption;
                    if (cur_corr.active && cur_corr.ticks_remaining > 0) {
                        cur_corr.ticks_remaining--;
                        result.total_damage_events++;
                        double dmg = cur_corr.tick_damage;

                        // Dynamic DoT scaling (No snapshotting in Forever)
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::SHADOW, current_time);
                            dmg = 73.0 + (sp * mechanics.corruption_sp_coefficient * 0.20);
                        }

                        // Wrack amplification (+10% to other Shadow DoTs)
                        double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                        dmg *= get_current_shadow_multiplier(current_time) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                        // Baseline DoT Crit + Pandemic bonus (Affliction)
                        bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                        if (is_crit) {
                            result.total_damage_crits++;
                            result.crits++;
                            double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                            dmg *= pand_crit_mult;
                        }

                        if (target.consume_isb_charge(current_time)) {
                            dmg *= (1.0 + target.isb_bonus);
                            result.isb_consumed++;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_corruption += dmg;
                        result.record_spell_hit(SpellID::CORRUPTION, dmg, is_crit);
                        result.total_damage += dmg;
                        if (record_timeline) {
                            result.timeline.push_back({current_time, dmg, SpellID::CORRUPTION, is_crit, false, player_mana, target.isb_charges});
                        }
                        apply_havoc_cleave(dmg, t_idx);

                        // Nightfall proc check (2% per point = 4% at 2/2)
                        if (mechanics.nightfall_enabled && talents.aff.nightfall > 0) {
                            double p = talents.aff.nightfall * 0.02;
                            if (rng.chance(p)) {
                                shadow_trance_active = true;
                                shadow_trance_expire = current_time + 10.0;
                                result.nightfall_procs++;
                                queue.push(shadow_trance_expire, EventType::BUFF_EXPIRE, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
                            }
                        }

                        if (cur_corr.ticks_remaining > 0 && current_time + cur_corr.tick_interval <= cur_corr.expire_time) {
                            queue.push(current_time + cur_corr.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION), 0, t_idx);
                        } else {
                            cur_corr.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::CURSE_OF_AGONY)) {
                    ActiveDot& cur_agony = target_states[t_idx].dot_agony;
                    if (cur_agony.active && cur_agony.ticks_remaining > 0) {
                        cur_agony.ticks_remaining--;
                        result.total_damage_events++;
                        int tick_index = 12 - cur_agony.ticks_remaining;
                        double ramp = (tick_index <= 4) ? 0.50 : (tick_index <= 8 ? 1.0 : 1.50);

                        double base_tick_portion = (552.0 / 12.0);
                        if (cur_agony.amplified) {
                            base_tick_portion *= 1.50;
                        }
                        double sp_tick_portion = 0.0;
                        if (mechanics.snapshot_dots) {
                            // cur_agony.tick_damage stored (552.0 / 12.0) + (sp * 1.596 / 12.0)
                            sp_tick_portion = cur_agony.tick_damage - (552.0 / 12.0);
                        } else {
                            double sp = get_current_sp(School::SHADOW, current_time);
                            sp_tick_portion = (sp * 1.596 / 12.0);
                        }
                        double total_tick_before_ramp = base_tick_portion + sp_tick_portion;

                        double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                        double dmg = total_tick_before_ramp * ramp * get_current_shadow_multiplier(current_time) * (1.0 + talents.aff.improved_bane_of_agony * 0.05) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                        // Baseline DoT Crit + Pandemic bonus (Affliction)
                        bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                        if (is_crit) {
                            result.total_damage_crits++;
                            result.crits++;
                            double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                            dmg *= pand_crit_mult;
                        }

                        if (target.consume_isb_charge(current_time)) {
                            dmg *= (1.0 + target.isb_bonus);
                            result.isb_consumed++;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_agony += dmg;
                        result.dmg_curse += dmg;
                        result.record_spell_hit(SpellID::CURSE_OF_AGONY, dmg, is_crit);
                        result.total_damage += dmg;
                        if (record_timeline) {
                            result.timeline.push_back({current_time, dmg, SpellID::CURSE_OF_AGONY, is_crit, false, player_mana, target.isb_charges});
                        }
                        apply_havoc_cleave(dmg, t_idx);

                        if (cur_agony.ticks_remaining > 0) {
                            queue.push(current_time + cur_agony.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CURSE_OF_AGONY), 0, t_idx);
                        } else {
                            cur_agony.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SIPHON_LIFE)) {
                    ActiveDot& cur_sl = target_states[t_idx].dot_siphon_life;
                    if (cur_sl.active && cur_sl.ticks_remaining > 0) {
                        cur_sl.ticks_remaining--;
                        result.total_damage_events++;
                        double dmg = cur_sl.tick_damage;
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::SHADOW, current_time);
                            dmg = 41.0 + (0.05 * sp);
                        }
                        double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                        dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                        // Baseline DoT Crit + Pandemic bonus (Affliction)
                        bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                        if (is_crit) {
                            result.total_damage_crits++;
                            result.crits++;
                            double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                            dmg *= pand_crit_mult;
                        }

                        if (target.consume_isb_charge(current_time)) {
                            dmg *= (1.0 + target.isb_bonus);
                            result.isb_consumed++;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_siphon_life += dmg;
                        result.record_spell_hit(SpellID::SIPHON_LIFE, dmg, is_crit);
                        result.total_damage += dmg;
                        if (record_timeline) {
                            result.timeline.push_back({current_time, dmg, SpellID::SIPHON_LIFE, is_crit, false, player_mana, target.isb_charges});
                        }
                        apply_havoc_cleave(dmg, t_idx);
                        player_health = std::min(stats.max_health, player_health + dmg);

                        if (cur_sl.ticks_remaining > 0) {
                            queue.push(current_time + cur_sl.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::SIPHON_LIFE), 0, t_idx);
                        } else {
                            cur_sl.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::IMMOLATE)) {
                    ActiveDot& cur_imm = target_states[t_idx].dot_immolate;
                    if (cur_imm.active && cur_imm.ticks_remaining > 0) {
                        cur_imm.ticks_remaining--;
                        result.total_damage_events++;
                        double dmg = cur_imm.tick_damage;
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::FIRE, current_time);
                            dmg = 55.0 + 0.13 * sp;
                        }
                        dmg *= get_current_fire_multiplier(current_time) * destro_spell_mult * malediction_mult * stats.all_damage_multiplier;

                        // Baseline DoT Crit + Ruin bonus (Destruction)
                        bool is_crit = rng.chance(calculate_crit_chance(School::FIRE, stats));
                        if (is_crit) {
                            result.total_damage_crits++;
                            result.crits++;
                            dmg *= destro_crit_mult;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_immolate += dmg;
                        result.record_spell_hit(SpellID::IMMOLATE, dmg, is_crit);
                        result.total_damage += dmg;
                        if (record_timeline) {
                            result.timeline.push_back({current_time, dmg, SpellID::IMMOLATE, is_crit, false, player_mana, target.isb_charges});
                        }
                        apply_havoc_cleave(dmg, t_idx);

                        if (cur_imm.ticks_remaining > 0) {
                            queue.push(current_time + cur_imm.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::IMMOLATE), 0, t_idx);
                        } else {
                            cur_imm.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::CURSE_OF_DOOM)) {
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double dmg = 1742.0 + 4.0 * sp;
                    double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;
                    bool is_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                    if (is_crit) {
                        result.total_damage_crits++;
                        result.crits++;
                        double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                        dmg *= pand_crit_mult;
                    }

                    if (target.consume_isb_charge(current_time)) {
                        dmg *= (1.0 + target.isb_bonus);
                        result.isb_consumed++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                    if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    result.dmg_doom += dmg;
                    result.dmg_curse += dmg;
                    result.record_spell_hit(SpellID::CURSE_OF_DOOM, dmg, is_crit);
                    result.total_damage += dmg;
                    apply_havoc_cleave(dmg, t_idx);
                    target_states[t_idx].dot_agony.active = false;
                    doom_tick_time = 0.0;

                    if (record_timeline) {
                        result.timeline.push_back({current_time, dmg, SpellID::CURSE_OF_DOOM, is_crit, false, player_mana, target.isb_charges});
                        bool eureka_buffed = (race == Race::GNOME && eureka_charges > 0);
                        bool orc_buffed = (race == Race::ORC && current_time < racial_expire_time);
                        std::string tick_note = eureka_buffed ? "Doom Tick (+10% Eureka!)" : (orc_buffed ? "Doom Tick (+10% Blood Fury)" : "Doom Tick");
                        result.cast_sequence.push_back({current_time, SpellID::CURSE_OF_DOOM, dmg, is_crit, false, 0.0, tick_note});
                    }
                }
                break;
            }

            case EventType::GCD_READY: {
                decide_next_action(current_time);
                break;
            }

            case EventType::BUFF_EXPIRE: {
                if (ev.spell_id == static_cast<uint8_t>(SpellID::SHADOW_BOLT)) {
                    if (current_time >= shadow_trance_expire) {
                        shadow_trance_active = false;
                    }
                }
                break;
            }

            case EventType::MANA_REGEN_TICK: {
                if (stats.mp5 > 0.0) {
                    player_mana = std::min(stats.max_mana, player_mana + stats.mp5);
                    result.mana_gained += stats.mp5;
                }
                if (mechanics.pet_mana_management && active_pet != PetChoice::NONE) {
                    double pet_mp5 = mechanics.pet_base_mp5;
                    if (buffs.blessing_of_wisdom) pet_mp5 += 30.0;
                    if (buffs.warchiefs_blessing) pet_mp5 += 10.0;
                    pet_mana = std::min(pet_max_mana, pet_mana + pet_mp5);
                }
                queue.push(current_time + 5.0, EventType::MANA_REGEN_TICK);
                break;
            }

            case EventType::PET_MELEE_SWING: {
                if (active_pet == PetChoice::SUCCUBUS) {
                    result.record_spell_cast(SpellID::PET_MELEE);
                    if (rng.chance(0.95)) {
                        double master_sp = get_current_sp(School::SHADOW, current_time);
                        double bonus_ap = mechanics.pet_scaling ? (mechanics.pet_ap_ratio * master_sp) : 0.0;
                        double base_swing = rng.range(145.0, 195.0) + (bonus_ap / 14.0) * 2.0;

                        // Unholy Power in Forever: +2% per point (+10% at 5/5)
                        base_swing *= (1.0 + talents.demo.unholy_power * 0.02);

                        double armor_mult = 0.86;
                        double swing_dmg = base_swing * armor_mult;

                        bool melee_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                        if (melee_crit) {
                            swing_dmg *= 2.0;
                        }

                        // Judgement of Wisdom proc on pet melee hit (50% chance to restore 59 mana)
                        if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                            pet_mana = std::min(pet_max_mana, pet_mana + 59.0);
                        }

                        // Demonic Brand proc (Succubus deals bonus Shadow damage)
                        double brand_dmg = 0.0;
                        if (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && current_time < demonic_brand_expire) {
                            demonic_brand_charges--;
                            brand_dmg = rng.range(65.0, 68.0);
                            brand_dmg *= (1.0 + talents.demo.unholy_power * 0.02);
                            if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) brand_dmg *= 1.15;
                            if (buffs.curse_of_shadows) brand_dmg *= 1.10;
                            brand_dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                            result.dmg_demonic_brand += brand_dmg;
                        }

                        result.dmg_pet_melee += swing_dmg;
                        result.record_spell_hit(SpellID::PET_MELEE, swing_dmg, melee_crit);
                        result.dmg_pet_succubus += (swing_dmg + brand_dmg);
                        result.dmg_pet += (swing_dmg + brand_dmg);
                        result.total_damage += (swing_dmg + brand_dmg);
                        apply_havoc_cleave(swing_dmg + brand_dmg, 0);
                    } else {
                        result.record_spell_miss(SpellID::PET_MELEE);
                    }

                    if (current_time + 2.0 < fight_duration) {
                        queue.push(current_time + 2.0, EventType::PET_MELEE_SWING);
                    }
                }
                break;
            }

            case EventType::PET_CAST_FINISH: {
                if (active_pet == PetChoice::SUCCUBUS) {
                    double lop_cost = mechanics.succubus_lop_cost;
                    bool can_cast = !mechanics.pet_mana_management || (pet_mana >= lop_cost);

                    if (can_cast) {
                        if (mechanics.pet_mana_management) {
                            pet_mana -= lop_cost;
                        }
                        result.record_spell_cast(SpellID::PET_LASH_OF_PAIN);

                        // Lash of Pain: 50 shadow damage + pet SP scaling
                        if (rng.chance(0.83)) {
                            double master_sp = get_current_sp(School::SHADOW, current_time);
                            double pet_sp = mechanics.pet_scaling ? (mechanics.pet_sp_ratio * master_sp) : 0.0;
                            double base_lop = 50.0 + (1.5 / 3.5) * pet_sp; // Pet SP inheritance

                            // Unholy Power (+2%/pt) and Improved Sayaad (+10%/pt)
                            base_lop *= (1.0 + talents.demo.unholy_power * 0.02);
                            base_lop *= (1.0 + talents.demo.improved_sayaad * 0.10);

                            if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) base_lop *= 1.15;
                            if (buffs.curse_of_shadows) base_lop *= 1.10;

                            bool lop_crit = rng.chance(calculate_crit_chance(School::SHADOW, stats));
                            if (lop_crit) {
                                base_lop *= 1.5;
                            }

                            base_lop *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);

                            // Judgement of Wisdom proc on pet spell hit
                            if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                                pet_mana = std::min(pet_max_mana, pet_mana + 59.0);
                            }

                            // Demonic Brand proc (Succubus deals bonus Shadow damage)
                            double brand_dmg = 0.0;
                            if (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && current_time < demonic_brand_expire) {
                                demonic_brand_charges--;
                                brand_dmg = rng.range(65.0, 68.0);
                                brand_dmg *= (1.0 + talents.demo.unholy_power * 0.02);
                                if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) brand_dmg *= 1.15;
                                if (buffs.curse_of_shadows) brand_dmg *= 1.10;
                                brand_dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                                result.dmg_demonic_brand += brand_dmg;
                            }

                            result.dmg_pet_lash_of_pain += base_lop;
                            result.record_spell_hit(SpellID::PET_LASH_OF_PAIN, base_lop, lop_crit);
                            result.dmg_pet_succubus += (base_lop + brand_dmg);
                            result.dmg_pet += (base_lop + brand_dmg);
                            result.total_damage += (base_lop + brand_dmg);
                            apply_havoc_cleave(base_lop + brand_dmg, 0);
                        } else {
                            result.record_spell_miss(SpellID::PET_LASH_OF_PAIN);
                        }
                    }

                    // Improved Sayaad reduces Lash of Pain cooldown by 1.0s per rank (12s -> 9s at 3/3)
                    double lop_cd = std::max(3.0, 12.0 - 1.0 * talents.demo.improved_sayaad);
                    if (!can_cast) {
                        // If OOM, retry sooner (every 1.5s)
                        lop_cd = 1.5;
                    }
                    if (current_time + lop_cd < fight_duration) {
                        queue.push(current_time + lop_cd, EventType::PET_CAST_FINISH);
                    }
                } else if (active_pet == PetChoice::IMP) {
                    double fb_cost = mechanics.imp_firebolt_cost;
                    bool can_cast = !mechanics.pet_mana_management || (pet_mana >= fb_cost);

                    if (can_cast) {
                        if (mechanics.pet_mana_management) {
                            pet_mana -= fb_cost;
                        }
                        result.record_spell_cast(SpellID::PET_FIREBOLT);

                        // Imp Firebolt: Modern (44 base + pet SP, 2.0s cast) vs Classic (85-98 + pet SP, 1.5s cast)
                        if (rng.chance(0.83)) {
                            double master_sp = get_current_sp(School::FIRE, current_time);
                            double base_fb = 0.0;
                            double pet_sp = mechanics.pet_scaling ? (mechanics.pet_sp_ratio * master_sp) : 0.0;
                            if (mechanics.imp_firebolt_modern_scaling) {
                                // Modern Firebolt (Rank 7): 44 base fire damage + pet SP scaling (10 SP = 1 Pet SP at 2.0/3.5 coeff)
                                base_fb = 44.0 + (2.0 / 3.5) * pet_sp;
                            } else {
                                // Classic Firebolt (Rank 7): 85 - 98 fire damage + pet SP scaling (10 SP = 1 Pet SP at 1.5/3.5 coeff)
                                base_fb = rng.range(85.0, 98.0) + (1.5 / 3.5) * pet_sp;
                            }

                            base_fb *= (1.0 + talents.demo.unholy_power * 0.02);
                            base_fb *= (1.0 + talents.demo.improved_imp * 0.10);

                            if (buffs.curse_of_elements) base_fb *= 1.10;

                            bool fb_crit = rng.chance(calculate_crit_chance(School::FIRE, stats));
                            if (fb_crit) {
                                base_fb *= 1.5;
                            }

                            base_fb *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                            // Judgement of Wisdom proc on pet spell hit
                            if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                                pet_mana = std::min(pet_max_mana, pet_mana + 59.0);
                            }

                            // Demonic Brand proc (Imp deals bonus Fire damage)
                            double brand_dmg = 0.0;
                            if (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && current_time < demonic_brand_expire) {
                                demonic_brand_charges--;
                                brand_dmg = rng.range(65.0, 68.0);
                                brand_dmg *= (1.0 + talents.demo.unholy_power * 0.02);
                                if (buffs.curse_of_elements) brand_dmg *= 1.10;
                                brand_dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);
                                result.dmg_demonic_brand += brand_dmg;
                            }

                            result.dmg_pet_firebolt += base_fb;
                            result.record_spell_hit(SpellID::PET_FIREBOLT, base_fb, fb_crit);
                            result.dmg_pet_imp += (base_fb + brand_dmg);
                            result.dmg_pet += (base_fb + brand_dmg);
                            result.total_damage += (base_fb + brand_dmg);
                            apply_havoc_cleave(base_fb + brand_dmg, 0);
                        } else {
                            result.record_spell_miss(SpellID::PET_FIREBOLT);
                        }
                    }

                    // Imp Firebolt cast interval: 2.0s for Modern, 1.5s for Classic
                    double fb_interval = mechanics.imp_firebolt_modern_scaling ? 2.0 : 1.5;
                    if (!can_cast) {
                        // If OOM, retry on 1.0s intervals
                        fb_interval = 1.0;
                    }
                    if (current_time + fb_interval < effective_duration) {
                        queue.push(current_time + fb_interval, EventType::PET_CAST_FINISH);
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    // Finalize metrics
    target.update_isb_uptime(effective_duration);
    result.dps = result.total_damage / effective_duration;
    result.isb_uptime_percent = (target.total_isb_uptime / effective_duration) * 100.0;

    return result;
}

} // namespace warlock
