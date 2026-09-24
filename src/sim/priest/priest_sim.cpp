#include "priest_sim.hpp"
#include <algorithm>
#include <cmath>

namespace priest {

PriestSimulator::PriestSimulator() {
    base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, race);
    talents = Talents::create_forever_shadow();
    gear = sim::GearLoadout::create_preraid_bis();
    use_raw_stats = true;
    raw_stats = gear.calculate_stats();
    raw_stats.spell_power += raw_stats.shadow_power;
    raw_stats.shadow_power = 0.0;
}

double PriestSimulator::calculate_hit_chance(sim::School school) const {
    double base_hit = mechanics.base_hit_vs_boss; // 83% for lvl 63 boss
    int delta = target_config.level - 60;
    if (delta <= 2) {
        base_hit = 0.96 - delta * 0.01;
    } else if (delta == 3) {
        base_hit = mechanics.base_hit_vs_boss;
    } else {
        base_hit = std::max(0.01, mechanics.base_hit_vs_boss - (delta - 3) * 0.11);
    }

    double extra_hit = (use_raw_stats ? raw_stats.spell_hit_percent : gear.calculate_stats().spell_hit_percent);
    if (school == sim::School::SHADOW) {
        extra_hit += talents.shadow.shadow_focus * 1.0; // Shadow Focus (+1% hit per point)
    } else if (school == sim::School::HOLY) {
        extra_hit += talents.disc.holy_precision * 6.0; // +6% per point (up to 18%)
    }

    double hit = base_hit + (extra_hit / 100.0);
    return std::min(mechanics.max_spell_hit, hit);
}

double PriestSimulator::calculate_crit_chance(sim::School school, const sim::Stats& stats) const {
    double crit = stats.total_spell_crit(base_attrs.base_spell_crit, 59.2); // Priest 59.2 Int per 1% Crit
    if (school == sim::School::HOLY) {
        crit += talents.holy.holy_specialization * 1.0;
    }
    return std::clamp(crit / 100.0, 0.0, 1.0);
}

double PriestSimulator::calculate_partial_resist_multiplier(sim::School school, double target_resistance, sim::FastRNG& rng) const {
    double r = target_resistance;
    if (school == sim::School::SHADOW && target_config.curse_of_shadows) r = std::max(0.0, r - 75.0);

    double effective_res = r - raw_stats.spell_penetration;

    if (effective_res < 0.0) {
        if (mechanics.spell_piercing_below_zero) {
            return 1.0 + (-effective_res) * mechanics.spell_piercing_bonus_per_point;
        }
        return 1.0;
    }

    if (!mechanics.partial_resists_enabled || effective_res <= 0.0) {
        return 1.0;
    }

    double avg_resist = effective_res / (effective_res + 400.0);
    double roll = rng.next_double();
    if (roll < avg_resist * 0.25) return 0.25;
    if (roll < avg_resist * 0.50) return 0.50;
    if (roll < avg_resist * 0.75) return 0.75;
    if (roll < avg_resist)        return 0.00;
    return 1.0;
}

SimResult PriestSimulator::run_single_simulation(sim::FastRNG& rng) {
    SimResult result;
    double duration = fight_duration;
    if (randomize_duration) {
        duration = rng.range(fight_duration - duration_variance, fight_duration + duration_variance);
        duration = std::max(10.0, duration);
    }
    result.duration = duration;

    // 1. Calculate active player combat stats
    sim::Stats stats = use_raw_stats ? raw_stats : gear.calculate_stats();
    sim::BuffConfig active_buffs = buffs;
    active_buffs.apply_to_stats(stats, base_attrs, true, mechanics.personal_shadow_weaving);

    // Racial attribute & passive bonuses
    if (race == sim::Race::HUMAN) {
        stats.spirit *= 1.05; // The Human Spirit (+5% Spirit)
    } else if (race == sim::Race::GNOME) {
        stats.intellect *= 1.05; // Expansive Mind (+5% Intellect)
    }
    if (target_config.creature_type == sim::CreatureType::BEAST && race == sim::Race::TROLL) {
        stats.all_damage_multiplier *= 1.05; // Beast Slaying (+5% damage vs beasts)
    }

    // Mental Strength talent: +3% total Intellect per point (up to +15%)
    if (talents.disc.mental_strength > 0) {
        stats.intellect *= (1.0 + talents.disc.mental_strength * 0.03);
        stats.max_mana = base_attrs.base_mana + stats.intellect * 15.0;
    }

    // Spiritual Guidance talent: 1/3/5/6/8% of Spirit as Spell Power per point (up to +8% at 5/5)
    if (talents.holy.spiritual_guidance > 0) {
        static constexpr double sg_table[6] = {0.0, 0.01, 0.03, 0.05, 0.06, 0.08};
        double bonus_sp = stats.spirit * sg_table[std::clamp(talents.holy.spiritual_guidance, 0, 5)];
        stats.spell_power += bonus_sp;
    }

    // Darkness talent (+2% shadow damage per point)
    if (talents.shadow.darkness > 0) {
        stats.shadow_multiplier *= (1.0 + talents.shadow.darkness * 0.02);
    }

    // Shadowform (+10% shadow damage, 2.0x crit multiplier)
    bool in_shadowform = mechanics.shadowform_enabled && (talents.shadow.shadowform > 0);
    if (in_shadowform) {
        stats.shadow_multiplier *= (1.0 + mechanics.shadowform_damage_bonus);
        stats.shadow_crit_bonus_multiplier = mechanics.shadowform_crit_bonus_multiplier;
    }

    // Searing Light talent: Rank 1 is +2%, Rank 2 is +5% Holy damage
    if (talents.holy.searing_light > 0) {
        static constexpr double sl_table[3] = {0.0, 0.02, 0.05};
        stats.holy_multiplier *= (1.0 + sl_table[std::clamp(talents.holy.searing_light, 0, 2)]);
    }

    // Target active resistances
    double shadow_res = target_config.current_shadow_resistance;
    if (target_config.curse_of_shadows) shadow_res = std::max(0.0, shadow_res - 75.0);

    // 2. State tracking
    double current_time = 0.0;
    double current_mana = stats.max_mana;
    double gcd_ready_time = 0.0;
    double cast_finish_time = 0.0;
    SpellID current_casting_spell = SpellID::NONE;

    sim::FiveSecondRuleTracker fsr_tracker;

    // Cooldowns
    double cd_mind_blast_ready = 0.0;
    double cd_sw_death_ready = 0.0;
    double cd_devouring_plague_ready = 0.0;
    double cd_inner_focus_ready = 0.0;
    double cd_power_infusion_ready = 0.0;
    double cd_potion_ready = 0.0;
    double cd_demonic_rune_ready = 0.0;
    double cd_penance_ready = 0.0;
    double cd_starshards_ready = 0.0;
    double cd_chastise_ready = 0.0;
    double cd_dark_sacrifice_ready = 0.0;
    double cd_berserking_ready = 0.0;

    // Buffs on self
    bool inner_focus_active = false;
    double power_infusion_expires = 0.0;
    double berserking_expires = 0.0;
    bool free_holy_nova_active = false;

    // Debuffs on target
    int shadow_weaving_stacks = 0;
    double shadow_weaving_expires = 0.0;

    // Active DoTs
    struct ActiveDot {
        bool active = false;
        double next_tick = 0.0;
        double expire_time = 0.0;
        double tick_damage = 0.0;
        int remaining_ticks = 0;
    };
    ActiveDot dot_swp;
    ActiveDot dot_dp;
    ActiveDot dot_holy_fire;

    // Active Channel (Mind Flay, Penance, or Starshards)
    struct ActiveChannel {
        bool active = false;
        SpellID spell = SpellID::NONE;
        int ticks_done = 0;
        int total_ticks = 0;
        double next_tick = 0.0;
        double end_time = 0.0;
        double tick_damage = 0.0;
        sim::School school = sim::School::SHADOW;
    };
    ActiveChannel channel;

    // Event Queue
    sim::FastEventQueue<256> queue;
    queue.push(0.0, sim::EventType::GCD_READY);
    queue.push(2.0, sim::EventType::MANA_REGEN_TICK);
    queue.push(duration, sim::EventType::SIMULATION_END);

    // Helper: calculate mana cost with all discounts
    auto calculate_spell_cost = [&](SpellID spell, double base_cost) -> double {
        if (inner_focus_active) return 0.0;
        if (spell == SpellID::HOLY_NOVA && free_holy_nova_active) return 0.0;

        static constexpr double ma_table[4] = {0.0, 0.03, 0.07, 0.10};
        static constexpr double dc_table[3] = {0.0, 0.25, 0.50};

        double cost = base_cost;
        bool is_shadow = (spell == SpellID::SHADOW_WORD_PAIN || spell == SpellID::MIND_FLAY ||
                          spell == SpellID::MIND_BLAST || spell == SpellID::SHADOW_WORD_DEATH ||
                          spell == SpellID::DEVOURING_PLAGUE);

        if (is_shadow && in_shadowform) {
            cost *= (1.0 - mechanics.shadowform_mana_cost_reduction);
        }

        if (spell == SpellID::DEVOURING_PLAGUE && talents.shadow.devouring_contagion > 0) {
            cost *= (1.0 - dc_table[std::clamp(talents.shadow.devouring_contagion, 0, 2)]);
        }

        bool is_instant_or_smite_hf = (spell == SpellID::SHADOW_WORD_PAIN || spell == SpellID::SHADOW_WORD_DEATH ||
                                       spell == SpellID::DEVOURING_PLAGUE || spell == SpellID::SMITE ||
                                       spell == SpellID::HOLY_FIRE || spell == SpellID::HOLY_NOVA ||
                                       spell == SpellID::CHASTISE);
        if (is_instant_or_smite_hf && talents.disc.mental_agility > 0) {
            cost *= (1.0 - ma_table[std::clamp(talents.disc.mental_agility, 0, 3)]);
        }

        if (spell == SpellID::PENANCE && talents.holy.improved_healing > 0) {
            cost *= (1.0 - talents.holy.improved_healing * 0.05);
        }

        return cost;
    };

    // Calculate effective spell multipliers
    auto get_current_spell_multiplier = [&](sim::School school, SpellID spell) -> double {
        double mult = (school == sim::School::SHADOW) ? stats.shadow_multiplier :
                      (school == sim::School::HOLY)   ? stats.holy_multiplier : 1.0;
        mult *= stats.all_damage_multiplier;

        // Shadow Weaving bonus (+2% per stack in Forever)
        if (school == sim::School::SHADOW && shadow_weaving_stacks > 0 && current_time <= shadow_weaving_expires) {
            mult *= (1.0 + shadow_weaving_stacks * mechanics.shadow_weaving_per_stack);
        }

        // Power Infusion (+20% spell damage)
        if (current_time < power_infusion_expires) {
            mult *= 1.20;
        }

        // Curse of Shadows (+10% shadow damage)
        if (school == sim::School::SHADOW && target_config.curse_of_shadows) {
            mult *= 1.10;
        }

        // Twin Disciplines talent: +1% per point to instant cast spells
        if (spell == SpellID::SHADOW_WORD_PAIN || spell == SpellID::SHADOW_WORD_DEATH ||
            spell == SpellID::DEVOURING_PLAGUE || spell == SpellID::HOLY_NOVA ||
            spell == SpellID::CHASTISE) {
            mult *= (1.0 + talents.disc.twin_disciplines * 0.01);
        }

        // Improved Mind Flay (+10% per point = +20% at 2/2)
        if (spell == SpellID::MIND_FLAY && talents.shadow.improved_mind_flay > 0) {
            mult *= (1.0 + talents.shadow.improved_mind_flay * 0.10);
        }

        // Power in Light (+2% to +10% to Smite and Penance when target has Holy Fire)
        if ((spell == SpellID::SMITE || spell == SpellID::PENANCE) && dot_holy_fire.active && talents.disc.power_in_light > 0) {
            mult *= (1.0 + talents.disc.power_in_light * mechanics.power_in_light_bonus_per_rank);
        }

        return mult;
    };

    // Helper: apply spell damage
    auto deal_damage = [&](SpellID spell, double raw_dmg, bool is_crit, sim::School school) {
        double resist_mult = calculate_partial_resist_multiplier(school, shadow_res, rng);
        double final_dmg = raw_dmg * resist_mult;

        if (final_dmg > 0.0) {
            result.total_damage += final_dmg;
            result.record_spell_hit(spell, final_dmg, is_crit);

            switch (spell) {
                case SpellID::SHADOW_WORD_PAIN:  result.dmg_sw_pain += final_dmg; break;
                case SpellID::MIND_FLAY:         result.dmg_mind_flay += final_dmg; break;
                case SpellID::MIND_BLAST:        result.dmg_mind_blast += final_dmg; break;
                case SpellID::SHADOW_WORD_DEATH: result.dmg_sw_death += final_dmg; break;
                case SpellID::DEVOURING_PLAGUE:  result.dmg_devouring_plague += final_dmg; break;
                case SpellID::SMITE:             result.dmg_smite += final_dmg; break;
                case SpellID::HOLY_FIRE:         result.dmg_holy_fire += final_dmg; break;
                case SpellID::PENANCE:           result.dmg_penance += final_dmg; break;
                case SpellID::HOLY_NOVA:         result.dmg_holy_nova += final_dmg; break;
                case SpellID::STARSHARDS:        result.dmg_starshards += final_dmg; break;
                case SpellID::CHASTISE:          result.dmg_chastise += final_dmg; break;
                case SpellID::SHADOWGUARD:       result.dmg_shadowguard += final_dmg; break;
                case SpellID::TOUCH_OF_THE_GRAVE: result.dmg_touch_of_the_grave += final_dmg; break;
                default: break;
            }

            // Devouring Plague self-healing (100% of damage dealt)
            if (spell == SpellID::DEVOURING_PLAGUE) {
                result.healing_devouring_plague += final_dmg;
            }

            // Vampiric Embrace party healing (20% of shadow spell damage dealt)
            if (school == sim::School::SHADOW && talents.shadow.vampiric_embrace > 0 && spell != SpellID::TOUCH_OF_THE_GRAVE) {
                result.healing_vampiric_embrace += final_dmg * mechanics.vampiric_embrace_healing_percent;
            }

            // Shadow Weaving Stack Application (33% / 67% / 100% chance per shadow damage impact/tick)
            if (school == sim::School::SHADOW && talents.shadow.shadow_weaving > 0 && spell != SpellID::TOUCH_OF_THE_GRAVE) {
                static constexpr double sw_chance[4] = {0.0, 0.33, 0.67, 1.00};
                if (rng.chance(sw_chance[std::clamp(talents.shadow.shadow_weaving, 0, 3)])) {
                    if (shadow_weaving_stacks < mechanics.max_shadow_weaving_stacks) {
                        shadow_weaving_stacks++;
                        result.shadow_weaving_procs++;
                    }
                    shadow_weaving_expires = current_time + 15.0;
                }
            }

            if (record_timeline) {
                result.timeline.push_back({current_time, final_dmg, spell, is_crit, false, current_mana, shadow_weaving_stacks});
            }
        }
    };

    double totg_cd_ready = 0.0;
    auto apply_touch_of_the_grave = [&]() {
        if (race == sim::Race::UNDEAD && current_time >= totg_cd_ready && rng.chance(0.10)) {
            totg_cd_ready = current_time + 1.0;
            double grave_raw = 0.05 * stats.max_health;
            double grave_res = calculate_partial_resist_multiplier(sim::School::SHADOW, shadow_res, rng);
            double grave_dmg = grave_raw * grave_res;
            if (grave_dmg > 0.0) {
                result.total_damage += grave_dmg;
                result.dmg_touch_of_the_grave += grave_dmg;
                result.touch_of_the_grave_procs++;
                result.record_spell_hit(SpellID::TOUCH_OF_THE_GRAVE, grave_dmg, false);
                if (record_timeline) {
                    result.timeline.push_back({current_time, grave_dmg, SpellID::TOUCH_OF_THE_GRAVE, false, false, current_mana, shadow_weaving_stacks});
                }
            }
        }
    };

    // Action decider: picks the next action when free
    auto decide_next_action = [&]() {
        if (current_time < gcd_ready_time || current_casting_spell != SpellID::NONE || channel.active) return;

        // Consumables check
        if (active_buffs.use_mana_potions && current_time >= cd_potion_ready && (current_mana / stats.max_mana) <= policy.mana_potion_threshold) {
            double pot = rng.range(1400.0, 2200.0);
            current_mana = std::min(stats.max_mana, current_mana + pot);
            result.mana_gained += pot;
            cd_potion_ready = current_time + 120.0;
        }
        if (active_buffs.use_demonic_runes && current_time >= cd_demonic_rune_ready && (current_mana / stats.max_mana) <= policy.demonic_rune_threshold) {
            double rune = rng.range(900.0, 1500.0);
            current_mana = std::min(stats.max_mana, current_mana + rune);
            result.mana_gained += rune;
            cd_demonic_rune_ready = current_time + 120.0;
        }

        // Dark Sacrifice (Undead Racial)
        if (race == sim::Race::UNDEAD && policy.use_dark_sacrifice && current_time >= cd_dark_sacrifice_ready && (current_mana / stats.max_mana) <= 0.60) {
            current_mana = std::min(stats.max_mana, current_mana + 1600.0);
            result.mana_gained += 1600.0;
            cd_dark_sacrifice_ready = current_time + 600.0; // 10 min CD
        }

        // Berserking (Troll Racial)
        if (race == sim::Race::TROLL && policy.use_berserking && current_time >= cd_berserking_ready) {
            berserking_expires = current_time + 10.0;
            cd_berserking_ready = current_time + 180.0; // 3 min CD
        }

        // Power Infusion
        if (policy.use_power_infusion && talents.disc.power_infusion > 0 && current_time >= cd_power_infusion_ready) {
            power_infusion_expires = current_time + 15.0;
            cd_power_infusion_ready = current_time + 180.0;
        }

        // Inner Focus
        if (policy.use_inner_focus && talents.disc.inner_focus > 0 && current_time >= cd_inner_focus_ready) {
            inner_focus_active = true;
            cd_inner_focus_ready = current_time + 180.0;
        }

        double cast_speed_mult = (current_time < berserking_expires) ? 1.10 : 1.0;

        // Lambda helpers for spell casting
        auto try_dp = [&]() -> bool {
            if (current_time < cd_devouring_plague_ready || dot_dp.active) return false;
            auto dp_def = SpellBook::devouring_plague_rank6();
            double cost = calculate_spell_cost(SpellID::DEVOURING_PLAGUE, dp_def.mana_cost);
            if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::DEVOURING_PLAGUE);
            result.total_casts++;
            apply_touch_of_the_grave();
            cd_devouring_plague_ready = current_time + dp_def.cooldown;

            if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                dot_dp.active = true;
                dot_dp.expire_time = current_time + dp_def.dot_duration;
                dot_dp.next_tick = current_time + dp_def.dot_tick_interval;
                dot_dp.remaining_ticks = dp_def.num_ticks;

                double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::DEVOURING_PLAGUE);
                double sp = stats.effective_shadow_power();
                dot_dp.tick_damage = (dp_def.dot_base_dmg_per_tick + sp * dp_def.dot_coeff_per_tick) * mult;

                queue.push(dot_dp.next_tick, sim::EventType::DOT_TICK, static_cast<uint8_t>(SpellID::DEVOURING_PLAGUE));
            } else {
                result.record_spell_miss(SpellID::DEVOURING_PLAGUE);
                result.misses++;
            }

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::DEVOURING_PLAGUE, 0.0, false, false, 0.0, "DoT Application"});
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_swp = [&]() -> bool {
            if (dot_swp.active && current_time < dot_swp.expire_time - 0.1) return false;
            auto swp_def = SpellBook::shadow_word_pain_rank8();
            double cost = calculate_spell_cost(SpellID::SHADOW_WORD_PAIN, swp_def.mana_cost);
            if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::SHADOW_WORD_PAIN);
            result.total_casts++;
            apply_touch_of_the_grave();

            if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                dot_swp.active = true;
                int extra_duration = talents.shadow.improved_shadow_word_pain * 3;
                double dur = swp_def.dot_duration + extra_duration;
                dot_swp.expire_time = current_time + dur;
                dot_swp.next_tick = current_time + swp_def.dot_tick_interval;
                dot_swp.remaining_ticks = static_cast<int>(dur / swp_def.dot_tick_interval);

                double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::SHADOW_WORD_PAIN);
                double sp = stats.effective_shadow_power();
                dot_swp.tick_damage = (swp_def.dot_base_dmg_per_tick + sp * swp_def.dot_coeff_per_tick) * mult;

                queue.push(dot_swp.next_tick, sim::EventType::DOT_TICK, static_cast<uint8_t>(SpellID::SHADOW_WORD_PAIN));
            } else {
                result.record_spell_miss(SpellID::SHADOW_WORD_PAIN);
                result.misses++;
            }

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::SHADOW_WORD_PAIN, 0.0, false, false, 0.0, "DoT Application"});
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_mb = [&]() -> bool {
            if (current_time < cd_mind_blast_ready) return false;
            auto mb_def = SpellBook::mind_blast_rank9();
            double cost = calculate_spell_cost(SpellID::MIND_BLAST, mb_def.mana_cost);
            if (inner_focus_active) { cost = 0.0; }
            if (current_mana < cost) return false;

            current_casting_spell = SpellID::MIND_BLAST;
            double cast_time = mb_def.base_cast_time / cast_speed_mult;
            cast_finish_time = current_time + cast_time;
            gcd_ready_time = current_time + std::max(mechanics.base_gcd, cast_time);

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::MIND_BLAST, 0.0, false, false, cast_time, "Burst"});
            }

            queue.push(cast_finish_time, sim::EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::MIND_BLAST));
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            double mb_cd = 8.0 - (talents.shadow.improved_mind_blast * 0.5);
            cd_mind_blast_ready = current_time + mb_cd;
            return true;
        };

        auto try_swd = [&](bool execute_only) -> bool {
            bool execute_ok = !execute_only || ((duration - current_time) / duration <= 0.20);
            if (!execute_ok || current_time < cd_sw_death_ready) return false;
            auto swd_def = SpellBook::shadow_word_death_rank4();
            double cost = calculate_spell_cost(SpellID::SHADOW_WORD_DEATH, swd_def.mana_cost);
            bool if_crit_bonus = inner_focus_active;
            if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::SHADOW_WORD_DEATH);
            result.total_casts++;
            apply_touch_of_the_grave();
            cd_sw_death_ready = current_time + swd_def.cooldown;

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::SHADOW_WORD_DEATH, 0.0, false, false, 0.0, "Execute / Burst"});
            }

            if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                double crit_p = calculate_crit_chance(sim::School::SHADOW, stats);
                if ((duration - current_time) / duration <= 0.20) {
                    crit_p += talents.shadow.early_demise * 0.15;
                }
                if (if_crit_bonus) {
                    crit_p += 0.25;
                }
                bool is_crit = rng.chance(crit_p);
                double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::SHADOW_WORD_DEATH);
                double dmg = rng.range(swd_def.min_dmg, swd_def.max_dmg) + stats.effective_shadow_power() * swd_def.direct_coefficient;
                dmg *= mult;
                if (is_crit) dmg *= stats.shadow_crit_bonus_multiplier;
                deal_damage(SpellID::SHADOW_WORD_DEATH, dmg, is_crit, sim::School::SHADOW);

                if (mechanics.sw_death_backlash) {
                    double backlash = stats.max_health * mechanics.sw_death_backlash_percent_max_hp;
                    result.self_damage_sw_death += backlash;
                }
            } else {
                result.record_spell_miss(SpellID::SHADOW_WORD_DEATH);
                result.misses++;
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_starshards = [&]() -> bool {
            if (race != sim::Race::NIGHT_ELF || current_time < cd_starshards_ready) return false;
            auto star_def = SpellBook::starshards_rank7();
            double cost = star_def.mana_cost;
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::STARSHARDS);
            result.total_casts++;
            apply_touch_of_the_grave();
            cd_starshards_ready = current_time + star_def.cooldown;

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::STARSHARDS, 0.0, false, false, star_def.dot_duration, "Channel"});
            }

            if (rng.chance(calculate_hit_chance(sim::School::ARCANE))) {
                channel.active = true;
                channel.spell = SpellID::STARSHARDS;
                channel.school = sim::School::ARCANE;
                channel.ticks_done = 0;
                channel.total_ticks = star_def.num_ticks;
                channel.next_tick = current_time + star_def.dot_tick_interval;
                channel.end_time = current_time + star_def.dot_duration;

                double mult = get_current_spell_multiplier(sim::School::ARCANE, SpellID::STARSHARDS);
                double sp = stats.effective_arcane_power();
                channel.tick_damage = (star_def.dot_base_dmg_per_tick + sp * star_def.dot_coeff_per_tick) * mult;

                queue.push(channel.next_tick, sim::EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::STARSHARDS));
            } else {
                result.record_spell_miss(SpellID::STARSHARDS);
                result.misses++;
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_mind_flay = [&]() -> bool {
            if (talents.shadow.mind_flay <= 0) return false;
            auto mf_def = SpellBook::mind_flay_rank6();
            double cost = calculate_spell_cost(SpellID::MIND_FLAY, mf_def.mana_cost);
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::MIND_FLAY);
            result.total_casts++;
            apply_touch_of_the_grave();

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::MIND_FLAY, 0.0, false, false, 3.0, "Channel"});
            }

            if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                channel.active = true;
                channel.spell = SpellID::MIND_FLAY;
                channel.school = sim::School::SHADOW;
                channel.ticks_done = 0;
                channel.total_ticks = 3;
                channel.next_tick = current_time + 1.0;
                channel.end_time = current_time + 3.0;

                double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::MIND_FLAY);
                double sp = stats.effective_shadow_power();
                channel.tick_damage = (mf_def.dot_base_dmg_per_tick + sp * mf_def.dot_coeff_per_tick) * mult;

                queue.push(channel.next_tick, sim::EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::MIND_FLAY));
            } else {
                result.record_spell_miss(SpellID::MIND_FLAY);
                result.misses++;
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_chastise = [&]() -> bool {
            if (race != sim::Race::DWARF || current_time < cd_chastise_ready) return false;
            auto ch_def = SpellBook::chastise_rank5();
            double cost = calculate_spell_cost(SpellID::CHASTISE, ch_def.mana_cost);
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::CHASTISE);
            result.total_casts++;
            apply_touch_of_the_grave();
            cd_chastise_ready = current_time + ch_def.cooldown;

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::CHASTISE, 0.0, false, false, 0.0, "Instant"});
            }

            if (rng.chance(calculate_hit_chance(sim::School::HOLY))) {
                bool is_crit = rng.chance(calculate_crit_chance(sim::School::HOLY, stats));
                double mult = get_current_spell_multiplier(sim::School::HOLY, SpellID::CHASTISE);
                double dmg = rng.range(ch_def.min_dmg, ch_def.max_dmg) + stats.effective_holy_power() * ch_def.direct_coefficient;
                dmg *= mult;
                if (is_crit) dmg *= stats.holy_crit_bonus_multiplier;
                deal_damage(SpellID::CHASTISE, dmg, is_crit, sim::School::HOLY);
            } else {
                result.record_spell_miss(SpellID::CHASTISE);
                result.misses++;
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_holy_nova = [&]() -> bool {
            if (!free_holy_nova_active || talents.holy.holy_nova <= 0) return false;
            auto nova_def = SpellBook::holy_nova_rank6();
            free_holy_nova_active = false;

            result.record_spell_cast(SpellID::HOLY_NOVA);
            result.total_casts++;
            apply_touch_of_the_grave();

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::HOLY_NOVA, 0.0, false, false, 0.0, "Instant AoE"});
            }

            if (rng.chance(calculate_hit_chance(sim::School::HOLY))) {
                bool is_crit = rng.chance(calculate_crit_chance(sim::School::HOLY, stats));
                double mult = get_current_spell_multiplier(sim::School::HOLY, SpellID::HOLY_NOVA);
                double dmg = rng.range(nova_def.min_dmg, nova_def.max_dmg) + stats.effective_holy_power() * nova_def.direct_coefficient;
                dmg *= mult;
                if (is_crit) dmg *= stats.holy_crit_bonus_multiplier;
                deal_damage(SpellID::HOLY_NOVA, dmg, is_crit, sim::School::HOLY);
            } else {
                result.record_spell_miss(SpellID::HOLY_NOVA);
                result.misses++;
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_holy_fire = [&]() -> bool {
            if (dot_holy_fire.active && current_time < dot_holy_fire.expire_time - 0.1) return false;
            auto hf_def = SpellBook::holy_fire_rank8();
            double cast_time = (hf_def.base_cast_time - (talents.holy.divine_fury * 0.1)) / cast_speed_mult;
            double cost = calculate_spell_cost(SpellID::HOLY_FIRE, hf_def.mana_cost);
            if (current_mana < cost) return false;

            current_casting_spell = SpellID::HOLY_FIRE;
            cast_finish_time = current_time + cast_time;
            gcd_ready_time = current_time + std::max(mechanics.base_gcd, cast_time);

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::HOLY_FIRE, 0.0, false, false, cast_time, "DoT / Direct"});
            }

            queue.push(cast_finish_time, sim::EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::HOLY_FIRE));
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_penance = [&]() -> bool {
            if (talents.disc.penance <= 0 || current_time < cd_penance_ready) return false;
            auto pen_def = SpellBook::penance_rank4();
            double cost = calculate_spell_cost(SpellID::PENANCE, pen_def.mana_cost);
            if (current_mana < cost) return false;

            current_mana -= cost;
            result.mana_spent += cost;
            fsr_tracker.on_mana_spent(current_time);

            result.record_spell_cast(SpellID::PENANCE);
            result.total_casts++;
            apply_touch_of_the_grave();
            cd_penance_ready = current_time + pen_def.cooldown;

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::PENANCE, 0.0, false, false, pen_def.dot_duration, "Channel"});
            }

            if (rng.chance(calculate_hit_chance(sim::School::HOLY))) {
                channel.active = true;
                channel.spell = SpellID::PENANCE;
                channel.school = sim::School::HOLY;
                channel.ticks_done = 0;
                channel.total_ticks = pen_def.num_ticks;
                channel.end_time = current_time + pen_def.dot_duration;

                double mult = get_current_spell_multiplier(sim::School::HOLY, SpellID::PENANCE);
                double sp = stats.effective_holy_power();
                channel.tick_damage = (pen_def.dot_base_dmg_per_tick + sp * pen_def.dot_coeff_per_tick) * mult;

                channel.ticks_done++;
                deal_damage(SpellID::PENANCE, channel.tick_damage, false, sim::School::HOLY);

                channel.next_tick = current_time + pen_def.dot_tick_interval;
                queue.push(channel.next_tick, sim::EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::PENANCE));
            } else {
                result.record_spell_miss(SpellID::PENANCE);
                result.misses++;
            }

            gcd_ready_time = current_time + mechanics.base_gcd;
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        auto try_smite = [&]() -> bool {
            auto smite_def = SpellBook::smite_rank8();
            double cast_time = (smite_def.base_cast_time - (talents.holy.divine_fury * 0.1)) / cast_speed_mult;
            double cost = calculate_spell_cost(SpellID::SMITE, smite_def.mana_cost);
            if (inner_focus_active) { cost = 0.0; }
            if (current_mana < cost) return false;

            current_casting_spell = SpellID::SMITE;
            cast_finish_time = current_time + cast_time;
            gcd_ready_time = current_time + std::max(mechanics.base_gcd, cast_time);

            if (record_timeline) {
                result.cast_sequence.push_back({current_time, SpellID::SMITE, 0.0, false, false, cast_time, "Filler"});
            }

            queue.push(cast_finish_time, sim::EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SMITE));
            queue.push(gcd_ready_time, sim::EventType::GCD_READY);
            return true;
        };

        // Execute rotation priority sequence
        switch (policy.rotation) {
            case RotationChoice::SHADOW_PRIEST:
                if (policy.cast_devouring_plague && try_dp()) return;
                if (policy.maintain_swp && try_swp()) return;
                if (policy.cast_mind_blast && try_mb()) return;
                if (policy.cast_sw_death && try_swd(policy.execute_sw_death_only)) return;
                if (policy.cast_starshards && try_starshards()) return;
                if (try_mind_flay()) return;
                if (try_smite()) return;
                break;

            case RotationChoice::SHADOW_NO_MB:
                if (policy.cast_devouring_plague && try_dp()) return;
                if (policy.maintain_swp && try_swp()) return;
                if (policy.cast_sw_death && try_swd(policy.execute_sw_death_only)) return;
                if (policy.cast_starshards && try_starshards()) return;
                if (try_mind_flay()) return;
                if (try_smite()) return;
                break;

            case RotationChoice::SHADOW_SWP_ONLY:
                if (policy.maintain_swp && try_swp()) return;
                if (policy.cast_sw_death && try_swd(true /* execute only < 20% HP */)) return;
                if (policy.cast_starshards && try_starshards()) return;
                if (try_mind_flay()) return;
                if (try_smite()) return;
                break;

            case RotationChoice::SMITE_PRIEST:
                if (policy.cast_chastise && try_chastise()) return;
                if (policy.cast_holy_nova_on_proc && try_holy_nova()) return;
                if (policy.cast_holy_fire && try_holy_fire()) return;
                if (policy.cast_penance && try_penance()) return;
                if (try_smite()) return;
                break;

            case RotationChoice::PURE_SMITE:
                if (policy.cast_chastise && try_chastise()) return;
                if (policy.cast_holy_nova_on_proc && try_holy_nova()) return;
                if (policy.cast_penance && try_penance()) return;
                if (try_smite()) return;
                break;

            case RotationChoice::HOLY_FIRE_WEAVING:
                if (policy.cast_chastise && try_chastise()) return;
                if (policy.cast_holy_nova_on_proc && try_holy_nova()) return;
                if (policy.cast_holy_fire && try_holy_fire()) return;
                if (policy.maintain_swp && try_swp()) return;
                if (policy.cast_penance && try_penance()) return;
                if (try_smite()) return;
                break;

            case RotationChoice::DISC_INQUISITOR:
                if (policy.cast_chastise && try_chastise()) return;
                if (policy.cast_holy_nova_on_proc && try_holy_nova()) return;
                if (policy.cast_holy_fire && try_holy_fire()) return;
                if (policy.maintain_swp && try_swp()) return;
                if (policy.cast_penance && try_penance()) return;
                if (policy.cast_sw_death && try_swd(policy.execute_sw_death_only)) return;
                if (try_smite()) return;
                break;

            default:
                if (try_smite()) return;
                break;
        }
    };

    // Main DES Loop
    while (!queue.empty()) {
        sim::Event ev = queue.pop();
        current_time = ev.time;
        if (current_time > duration) break;

        switch (ev.type) {
            case sim::EventType::CAST_FINISH: {
                SpellID spell = static_cast<SpellID>(ev.spell_id);
                current_casting_spell = SpellID::NONE;

                if (spell == SpellID::MIND_BLAST) {
                    auto mb = SpellBook::mind_blast_rank9();
                    double cost = calculate_spell_cost(SpellID::MIND_BLAST, mb.mana_cost);
                    bool if_crit_bonus = inner_focus_active;
                    if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }
                    current_mana = std::max(0.0, current_mana - cost);
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::MIND_BLAST);
                    result.total_casts++;
                    apply_touch_of_the_grave();

                    if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                        double crit_p = calculate_crit_chance(sim::School::SHADOW, stats) + (if_crit_bonus ? 0.25 : 0.0);
                        bool is_crit = rng.chance(crit_p);
                        double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::MIND_BLAST);
                        double dmg = rng.range(mb.min_dmg, mb.max_dmg) + stats.effective_shadow_power() * mb.direct_coefficient;
                        dmg *= mult;
                        if (is_crit) dmg *= stats.shadow_crit_bonus_multiplier;
                        deal_damage(SpellID::MIND_BLAST, dmg, is_crit, sim::School::SHADOW);
                    } else {
                        result.record_spell_miss(SpellID::MIND_BLAST);
                        result.misses++;
                    }
                } else if (spell == SpellID::SMITE) {
                    auto sm = SpellBook::smite_rank8();
                    double cost = calculate_spell_cost(SpellID::SMITE, sm.mana_cost);
                    bool if_crit_bonus = inner_focus_active;
                    if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }
                    current_mana = std::max(0.0, current_mana - cost);
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::SMITE);
                    result.total_casts++;
                    apply_touch_of_the_grave();

                    if (rng.chance(calculate_hit_chance(sim::School::HOLY))) {
                        double crit_p = calculate_crit_chance(sim::School::HOLY, stats) + (if_crit_bonus ? 0.25 : 0.0);
                        bool is_crit = rng.chance(crit_p);
                        double mult = get_current_spell_multiplier(sim::School::HOLY, SpellID::SMITE);
                        double dmg = rng.range(sm.min_dmg, sm.max_dmg) + stats.effective_holy_power() * sm.direct_coefficient;
                        dmg *= mult;
                        if (is_crit) dmg *= stats.holy_crit_bonus_multiplier;
                        deal_damage(SpellID::SMITE, dmg, is_crit, sim::School::HOLY);
                    } else {
                        result.record_spell_miss(SpellID::SMITE);
                        result.misses++;
                    }
                } else if (spell == SpellID::HOLY_FIRE) {
                    auto hf = SpellBook::holy_fire_rank8();
                    double cost = calculate_spell_cost(SpellID::HOLY_FIRE, hf.mana_cost);
                    bool if_crit_bonus = inner_focus_active;
                    if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }
                    current_mana = std::max(0.0, current_mana - cost);
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::HOLY_FIRE);
                    result.total_casts++;
                    apply_touch_of_the_grave();

                    if (rng.chance(calculate_hit_chance(sim::School::HOLY))) {
                        double crit_p = calculate_crit_chance(sim::School::HOLY, stats) + (if_crit_bonus ? 0.25 : 0.0);
                        bool is_crit = rng.chance(crit_p);
                        double mult = get_current_spell_multiplier(sim::School::HOLY, SpellID::HOLY_FIRE);
                        double dmg = rng.range(hf.min_dmg, hf.max_dmg) + stats.effective_holy_power() * hf.direct_coefficient;
                        dmg *= mult;
                        if (is_crit) dmg *= stats.holy_crit_bonus_multiplier;
                        deal_damage(SpellID::HOLY_FIRE, dmg, is_crit, sim::School::HOLY);

                        // Start Holy Fire DoT (10s duration, 5 ticks, 2s interval)
                        dot_holy_fire.active = true;
                        dot_holy_fire.expire_time = current_time + hf.dot_duration;
                        dot_holy_fire.next_tick = current_time + hf.dot_tick_interval;
                        dot_holy_fire.remaining_ticks = hf.num_ticks;
                        dot_holy_fire.tick_damage = (hf.dot_base_dmg_per_tick + stats.effective_holy_power() * hf.dot_coeff_per_tick) * mult;

                        queue.push(dot_holy_fire.next_tick, sim::EventType::DOT_TICK, static_cast<uint8_t>(SpellID::HOLY_FIRE));
                    } else {
                        result.record_spell_miss(SpellID::HOLY_FIRE);
                        result.misses++;
                    }
                }
                decide_next_action();
                break;
            }

            case sim::EventType::DOT_TICK: {
                SpellID spell = static_cast<SpellID>(ev.spell_id);
                if (spell == SpellID::SHADOW_WORD_PAIN && dot_swp.active) {
                    deal_damage(SpellID::SHADOW_WORD_PAIN, dot_swp.tick_damage, false, sim::School::SHADOW);
                    dot_swp.remaining_ticks--;
                    if (dot_swp.remaining_ticks > 0) {
                        dot_swp.next_tick = current_time + 3.0;
                        queue.push(dot_swp.next_tick, sim::EventType::DOT_TICK, static_cast<uint8_t>(SpellID::SHADOW_WORD_PAIN));
                    } else {
                        dot_swp.active = false;
                    }
                } else if (spell == SpellID::DEVOURING_PLAGUE && dot_dp.active) {
                    deal_damage(SpellID::DEVOURING_PLAGUE, dot_dp.tick_damage, false, sim::School::SHADOW);
                    dot_dp.remaining_ticks--;
                    if (dot_dp.remaining_ticks > 0) {
                        dot_dp.next_tick = current_time + 3.0;
                        queue.push(dot_dp.next_tick, sim::EventType::DOT_TICK, static_cast<uint8_t>(SpellID::DEVOURING_PLAGUE));
                    } else {
                        dot_dp.active = false;
                    }
                } else if (spell == SpellID::HOLY_FIRE && dot_holy_fire.active) {
                    deal_damage(SpellID::HOLY_FIRE, dot_holy_fire.tick_damage, false, sim::School::HOLY);
                    dot_holy_fire.remaining_ticks--;

                    // Clearcasting Holy Nova check (5% base + up to 10% from Searing Light)
                    if (talents.holy.holy_nova > 0) {
                        double nova_chance = mechanics.holy_nova_clearcast_proc_chance_base + (talents.holy.searing_light * mechanics.searing_light_proc_chance_per_rank);
                        if (rng.chance(nova_chance)) {
                            free_holy_nova_active = true;
                            result.clearcast_holy_nova_procs++;
                        }
                    }

                    if (dot_holy_fire.remaining_ticks > 0) {
                        dot_holy_fire.next_tick = current_time + 2.0;
                        queue.push(dot_holy_fire.next_tick, sim::EventType::DOT_TICK, static_cast<uint8_t>(SpellID::HOLY_FIRE));
                    } else {
                        dot_holy_fire.active = false;
                    }
                }
                break;
            }

            case sim::EventType::CHANNEL_TICK: {
                SpellID spell = static_cast<SpellID>(ev.spell_id);
                if (channel.active && channel.spell == spell) {
                    channel.ticks_done++;
                    deal_damage(spell, channel.tick_damage, false, channel.school);

                    // Mind Flay clipping check: after tick 2, if Mind Blast or SW:P refresh is ready, clip channel
                    bool mb_clip_ready = (policy.rotation == RotationChoice::SHADOW_PRIEST) && policy.cast_mind_blast && (current_time >= cd_mind_blast_ready);
                    bool swp_clip_ready = policy.maintain_swp && !dot_swp.active;
                    bool can_clip = (spell == SpellID::MIND_FLAY) && policy.clip_mind_flay_for_mb &&
                                    (channel.ticks_done == 2) && (mb_clip_ready || swp_clip_ready);
                    if (channel.ticks_done < channel.total_ticks && !can_clip) {
                        channel.next_tick = current_time + 1.0;
                        queue.push(channel.next_tick, sim::EventType::CHANNEL_TICK, static_cast<uint8_t>(spell));
                    } else {
                        channel.active = false;
                        decide_next_action();
                    }
                }
                break;
            }

            case sim::EventType::MANA_REGEN_TICK: {
                bool inside_5sr = fsr_tracker.is_inside_5sr(current_time);
                double med_ratio = (talents.disc.meditation / 3.0) * mechanics.meditation_casting_regen_ratio;
                double tick_mana = sim::ManaRegenCalculator::calculate_tick_mana(
                    stats.intellect, stats.spirit, stats.mp5, inside_5sr, med_ratio, sim::PlayerClass::PRIEST
                );
                current_mana = std::min(stats.max_mana, current_mana + tick_mana);
                result.mana_gained += tick_mana;

                queue.push(current_time + 2.0, sim::EventType::MANA_REGEN_TICK);
                break;
            }

            case sim::EventType::GCD_READY: {
                decide_next_action();
                break;
            }

            case sim::EventType::SIMULATION_END: {
                goto simulation_done;
            }

            default:
                break;
        }
    }

simulation_done:
    result.dps = (result.duration > 0.0) ? (result.total_damage / result.duration) : 0.0;
    return result;
}

} // namespace priest
