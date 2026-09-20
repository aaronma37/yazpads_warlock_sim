#include "priest_sim.hpp"
#include <algorithm>
#include <cmath>

namespace priest {

PriestSimulator::PriestSimulator() {
    base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, race);
    talents = Talents::create_forever_shadow();
    gear = sim::GearLoadout::create_phase3_bis();
    raw_stats = gear.calculate_stats();
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
    if (!mechanics.partial_resists_enabled || target_resistance <= 0.0) {
        return 1.0;
    }
    double r = target_resistance;
    if (school == sim::School::SHADOW && target_config.curse_of_shadows) r = std::max(0.0, r - 75.0);
    if (r <= 0.0) return 1.0;

    double avg_resist = r / (r + 400.0);
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

    // Mental Strength talent: +3% total Intellect per point (up to +15%)
    if (talents.disc.mental_strength > 0) {
        stats.intellect *= (1.0 + talents.disc.mental_strength * 0.03);
        stats.max_mana = base_attrs.base_mana + stats.intellect * 15.0;
    }

    // Spiritual Guidance talent: +1.6% of Spirit as Spell Power per point (up to +8% at 5/5)
    if (talents.holy.spiritual_guidance > 0) {
        double bonus_sp = stats.spirit * (talents.holy.spiritual_guidance * 0.016);
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

    // Searing Light talent (+2.5% holy damage per point)
    if (talents.holy.searing_light > 0) {
        stats.holy_multiplier *= (1.0 + talents.holy.searing_light * 0.025);
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

    // Buffs on self
    bool inner_focus_active = false;
    double power_infusion_expires = 0.0;

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

    // Active Channel (Mind Flay)
    struct ActiveChannel {
        bool active = false;
        int ticks_done = 0;
        int total_ticks = 3;
        double next_tick = 0.0;
        double end_time = 0.0;
        double tick_damage = 0.0;
    };
    ActiveChannel channel_mf;

    // Event Queue
    sim::FastEventQueue<256> queue;
    queue.push(0.0, sim::EventType::GCD_READY);
    queue.push(2.0, sim::EventType::MANA_REGEN_TICK);
    queue.push(duration, sim::EventType::SIMULATION_END);

    // Helper: apply spell damage
    auto deal_damage = [&](SpellID spell, double raw_dmg, bool is_crit, sim::School school) {
        double resist_mult = calculate_partial_resist_multiplier(school, shadow_res, rng);
        double final_dmg = raw_dmg * resist_mult;

        if (final_dmg > 0.0) {
            result.total_damage += final_dmg;
            result.record_spell_hit(spell, final_dmg, is_crit);
            result.total_damage_events++;
            if (is_crit) result.total_damage_crits++;

            switch (spell) {
                case SpellID::SHADOW_WORD_PAIN:  result.dmg_sw_pain += final_dmg; break;
                case SpellID::MIND_FLAY:         result.dmg_mind_flay += final_dmg; break;
                case SpellID::MIND_BLAST:        result.dmg_mind_blast += final_dmg; break;
                case SpellID::SHADOW_WORD_DEATH: result.dmg_sw_death += final_dmg; break;
                case SpellID::DEVOURING_PLAGUE:  result.dmg_devouring_plague += final_dmg; break;
                case SpellID::SMITE:             result.dmg_smite += final_dmg; break;
                case SpellID::HOLY_FIRE:         result.dmg_holy_fire += final_dmg; break;
                case SpellID::PENANCE:           result.dmg_penance += final_dmg; break;
                default: break;
            }

            // Shadow Weaving Stack Application (100% chance per shadow damage impact/tick)
            if (school == sim::School::SHADOW && talents.shadow.shadow_weaving > 0) {
                if (shadow_weaving_stacks < mechanics.max_shadow_weaving_stacks) {
                    shadow_weaving_stacks++;
                    result.shadow_weaving_procs++;
                }
                shadow_weaving_expires = current_time + 15.0;
            }

            if (record_timeline) {
                result.timeline.push_back({current_time, final_dmg, spell, is_crit, false, current_mana, shadow_weaving_stacks});
            }
        }
    };

    // Calculate effective spell multipliers
    auto get_current_spell_multiplier = [&](sim::School school, SpellID spell) -> double {
        double mult = (school == sim::School::SHADOW) ? stats.shadow_multiplier : stats.holy_multiplier;
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
        if (spell == SpellID::SHADOW_WORD_PAIN || spell == SpellID::SHADOW_WORD_DEATH || spell == SpellID::DEVOURING_PLAGUE) {
            mult *= (1.0 + talents.disc.twin_disciplines * 0.01);
        }

        // Improved Mind Flay (+10% per point = +20% at 2/2)
        if (spell == SpellID::MIND_FLAY && talents.shadow.improved_mind_flay > 0) {
            mult *= (1.0 + talents.shadow.improved_mind_flay * 0.10);
        }

        return mult;
    };

    // Action decider: picks the next action when free
    auto decide_next_action = [&]() {
        if (current_time < gcd_ready_time || current_casting_spell != SpellID::NONE) return;

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

        // Shadow Rotation
        if (policy.rotation == RotationChoice::SHADOW_PRIEST) {
            // 1. Shadow Word: Pain maintenance
            if (policy.maintain_swp && (!dot_swp.active || current_time >= dot_swp.expire_time - 0.1)) {
                auto swp_def = SpellBook::shadow_word_pain_rank8();
                double cost = swp_def.mana_cost;
                if (in_shadowform) cost *= (1.0 - mechanics.shadowform_mana_cost_reduction);
                if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }

                if (current_mana >= cost) {
                    current_mana -= cost;
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::SHADOW_WORD_PAIN);
                    result.total_casts++;

                    // Hit roll
                    if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                        dot_swp.active = true;
                        int extra_duration = talents.shadow.improved_shadow_word_pain * 3; // +3s / +6s
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

                    gcd_ready_time = current_time + mechanics.base_gcd;
                    queue.push(gcd_ready_time, sim::EventType::GCD_READY);
                    return;
                }
            }

            // 2. Mind Blast on cooldown
            double mb_cd = 8.0 - (talents.shadow.improved_mind_blast * 0.5); // Down to 5.5s
            if (policy.cast_mind_blast && current_time >= cd_mind_blast_ready) {
                auto mb_def = SpellBook::mind_blast_rank9();
                double cost = mb_def.mana_cost;
                if (in_shadowform) cost *= (1.0 - mechanics.shadowform_mana_cost_reduction);
                if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }

                if (current_mana >= cost) {
                    current_casting_spell = SpellID::MIND_BLAST;
                    cast_finish_time = current_time + mb_def.base_cast_time;
                    gcd_ready_time = current_time + std::max(mechanics.base_gcd, mb_def.base_cast_time);

                    queue.push(cast_finish_time, sim::EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::MIND_BLAST));
                    queue.push(gcd_ready_time, sim::EventType::GCD_READY);
                    cd_mind_blast_ready = current_time + mb_cd;
                    return;
                }
            }

            // 3. Shadow Word: Death
            bool execute_ok = !policy.execute_sw_death_only || ((duration - current_time) / duration <= 0.20);
            if (policy.cast_sw_death && execute_ok && current_time >= cd_sw_death_ready) {
                auto swd_def = SpellBook::shadow_word_death_rank4();
                double cost = swd_def.mana_cost;
                if (in_shadowform) cost *= (1.0 - mechanics.shadowform_mana_cost_reduction);
                if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }

                if (current_mana >= cost) {
                    current_mana -= cost;
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::SHADOW_WORD_DEATH);
                    result.total_casts++;
                    cd_sw_death_ready = current_time + swd_def.cooldown;

                    if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                        double crit_p = calculate_crit_chance(sim::School::SHADOW, stats);
                        if ((duration - current_time) / duration <= 0.20) {
                            crit_p += talents.shadow.early_demise * 0.15; // +15% / +30% execute crit
                        }
                        bool is_crit = rng.chance(crit_p);
                        double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::SHADOW_WORD_DEATH);
                        double dmg = rng.range(swd_def.min_dmg, swd_def.max_dmg) + stats.effective_shadow_power() * swd_def.direct_coefficient;
                        dmg *= mult;
                        if (is_crit) dmg *= stats.shadow_crit_bonus_multiplier;
                        deal_damage(SpellID::SHADOW_WORD_DEATH, dmg, is_crit, sim::School::SHADOW);
                    } else {
                        result.record_spell_miss(SpellID::SHADOW_WORD_DEATH);
                        result.misses++;
                    }

                    gcd_ready_time = current_time + mechanics.base_gcd;
                    queue.push(gcd_ready_time, sim::EventType::GCD_READY);
                    return;
                }
            }

            // 4. Mind Flay filler (3-second channeled spell)
            if (talents.shadow.mind_flay > 0) {
                auto mf_def = SpellBook::mind_flay_rank6();
                double cost = mf_def.mana_cost;
                if (in_shadowform) cost *= (1.0 - mechanics.shadowform_mana_cost_reduction);

                if (current_mana >= cost) {
                    current_mana -= cost;
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::MIND_FLAY);
                    result.total_casts++;

                    if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                        channel_mf.active = true;
                        channel_mf.ticks_done = 0;
                        channel_mf.total_ticks = 3;
                        channel_mf.next_tick = current_time + 1.0;
                        channel_mf.end_time = current_time + 3.0;

                        double mult = get_current_spell_multiplier(sim::School::SHADOW, SpellID::MIND_FLAY);
                        double sp = stats.effective_shadow_power();
                        channel_mf.tick_damage = (mf_def.dot_base_dmg_per_tick + sp * mf_def.dot_coeff_per_tick) * mult;

                        queue.push(channel_mf.next_tick, sim::EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::MIND_FLAY));
                    } else {
                        result.record_spell_miss(SpellID::MIND_FLAY);
                        result.misses++;
                    }

                    gcd_ready_time = current_time + mechanics.base_gcd;
                    queue.push(gcd_ready_time, sim::EventType::GCD_READY);
                    return;
                }
            }
        } else {
            // Smite / Holy DPS Rotation
            auto smite_def = SpellBook::smite_rank8();
            double cast_time = smite_def.base_cast_time - (talents.holy.divine_fury * 0.1); // 2.5s down to 2.0s
            double cost = smite_def.mana_cost;
            if (inner_focus_active) { cost = 0.0; inner_focus_active = false; }

            if (current_mana >= cost) {
                current_casting_spell = SpellID::SMITE;
                cast_finish_time = current_time + cast_time;
                gcd_ready_time = current_time + std::max(mechanics.base_gcd, cast_time);

                queue.push(cast_finish_time, sim::EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SMITE));
                queue.push(gcd_ready_time, sim::EventType::GCD_READY);
                return;
            }
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
                    double cost = mb.mana_cost;
                    if (in_shadowform) cost *= (1.0 - mechanics.shadowform_mana_cost_reduction);
                    current_mana = std::max(0.0, current_mana - cost);
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::MIND_BLAST);
                    result.total_casts++;

                    if (rng.chance(calculate_hit_chance(sim::School::SHADOW))) {
                        bool is_crit = rng.chance(calculate_crit_chance(sim::School::SHADOW, stats));
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
                    double cost = sm.mana_cost;
                    current_mana = std::max(0.0, current_mana - cost);
                    result.mana_spent += cost;
                    fsr_tracker.on_mana_spent(current_time);

                    result.record_spell_cast(SpellID::SMITE);
                    result.total_casts++;

                    if (rng.chance(calculate_hit_chance(sim::School::HOLY))) {
                        bool is_crit = rng.chance(calculate_crit_chance(sim::School::HOLY, stats));
                        double mult = get_current_spell_multiplier(sim::School::HOLY, SpellID::SMITE);
                        double dmg = rng.range(sm.min_dmg, sm.max_dmg) + stats.effective_holy_power() * sm.direct_coefficient;
                        dmg *= mult;
                        if (is_crit) dmg *= stats.holy_crit_bonus_multiplier;
                        deal_damage(SpellID::SMITE, dmg, is_crit, sim::School::HOLY);
                    } else {
                        result.record_spell_miss(SpellID::SMITE);
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
                }
                break;
            }

            case sim::EventType::CHANNEL_TICK: {
                SpellID spell = static_cast<SpellID>(ev.spell_id);
                if (spell == SpellID::MIND_FLAY && channel_mf.active) {
                    channel_mf.ticks_done++;
                    deal_damage(SpellID::MIND_FLAY, channel_mf.tick_damage, false, sim::School::SHADOW);

                    // Mind Flay clipping check: after tick 2, if Mind Blast or SW:P refresh is ready, clip channel
                    bool can_clip = policy.clip_mind_flay_for_mb && (channel_mf.ticks_done == 2) && (current_time >= cd_mind_blast_ready || !dot_swp.active);
                    if (channel_mf.ticks_done < channel_mf.total_ticks && !can_clip) {
                        channel_mf.next_tick = current_time + 1.0;
                        queue.push(channel_mf.next_tick, sim::EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::MIND_FLAY));
                    } else {
                        channel_mf.active = false;
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
