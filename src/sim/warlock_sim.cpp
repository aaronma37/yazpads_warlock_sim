#include "warlock_sim.hpp"
#include <algorithm>
#include <cmath>

namespace warlock {

WarlockSimulator::WarlockSimulator() {
    race = Race::GNOME;
    base_attrs = get_base_attributes_for_race(race);
    gear = GearLoadout::create_preraid_bis();
    talents = Talents::create_forever_shadow_destro();
    use_raw_stats = true;
    raw_stats = gear.calculate_stats();
    raw_stats.spell_power += raw_stats.shadow_power;
    raw_stats.shadow_power = 0.0;
}

double WarlockSimulator::calculate_hit_chance(School school) const {
    double hit = mechanics.base_hit_vs_boss; // 0.83 (83%)
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
    result.duration = fight_duration;

    // Race-specific base attributes
    base_attrs = get_base_attributes_for_race(race);

    // Compute base stats from gear & buffs (or raw manual stats)
    Stats stats = use_raw_stats ? raw_stats : gear.calculate_stats();
    buffs.apply_to_stats(stats, base_attrs, true, mechanics.personal_shadow_weaving); // true = WoW Forever mechanics

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
    if (buffs.sacrifice_succubus || buffs.sacrifice_imp) {
        if (talents.demo.demonic_pact > 0) {
            if (buffs.sacrifice_imp && policy.pet == PetChoice::IMP) {
                active_pet = PetChoice::NONE;
            } else if (buffs.sacrifice_succubus && policy.pet == PetChoice::SUCCUBUS) {
                active_pet = PetChoice::NONE;
            } else {
                active_pet = policy.pet;
            }
        } else {
            active_pet = PetChoice::NONE;
        }
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

    // Agonizing Flames (Destro Row 4 Col 2): +3% damage per point to ALL Destruction spells (+9% at 3/3)
    double destro_spell_mult = 1.0 + talents.destro.agonizing_flames * 0.03;

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
    int eureka_charges = 0;

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

    // Target state
    TargetConfig target = target_config;
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

    // Active DoTs tracking
    struct ActiveDot {
        bool active = false;
        double expire_time = 0.0;
        int ticks_remaining = 0;
        double tick_interval = 3.0;
        double tick_damage = 0.0;
        double tick_multiplier = 1.0;
    };
    ActiveDot dot_corruption;
    ActiveDot dot_agony;
    ActiveDot dot_immolate;
    ActiveDot dot_siphon_life;

    // Setup FastEventQueue
    FastEventQueue<256> queue;
    queue.push(fight_duration, EventType::SIMULATION_END);
    queue.push(5.0, EventType::MANA_REGEN_TICK);

    // Initial Pet actions
    if (active_pet == PetChoice::SUCCUBUS) {
        queue.push(1.0, EventType::PET_MELEE_SWING);
        queue.push(0.5, EventType::PET_CAST_FINISH);
    } else if (active_pet == PetChoice::IMP) {
        queue.push(0.3, EventType::PET_CAST_FINISH);
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

    // Decision maker using Rule-Based Action Priority List (APL)
    std::vector<PriorityRule> priority_rules = policy.get_priority_rules(talents, race);
    RotationChoice eff_rotation = policy.rotation;

    auto decide_next_action = [&](double now) {
        if (is_casting || now < gcd_ready_time) return;

        bool execute_phase = (now / fight_duration) >= 0.65; // Target <35% HP

        // 1. Off-GCD Cooldown checks: Mana Potions & Demonic Runes
        if (buffs.use_mana_potions && now >= potion_cd_ready && (stats.max_mana - player_mana) >= 1800.0) {
            double mana_gain = rng.range(1400.0, 2200.0);
            player_mana = std::min(stats.max_mana, player_mana + mana_gain);
            result.mana_gained += mana_gain;
            potion_cd_ready = now + 120.0;
        }
        if (buffs.use_demonic_runes && now >= rune_cd_ready && (stats.max_mana - player_mana) >= 1200.0 && player_health > 1500.0) {
            double mana_gain = rng.range(900.0, 1500.0);
            player_mana = std::min(stats.max_mana, player_mana + mana_gain);
            player_health -= mana_gain;
            result.mana_gained += mana_gain;
            rune_cd_ready = now + 120.0;
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
        if (now >= racial_cd_ready) {
            if (race == Race::ORC) {
                racial_expire_time = now + 15.0;
                racial_cd_ready = now + 120.0;
                if (record_timeline) {
                    result.cast_sequence.push_back({now, SpellID::RACIAL_BLOOD_FURY, 0.0, false, false, 0.0, "Racial Cooldown"});
                }
            } else if (race == Race::TROLL) {
                racial_expire_time = now + 10.0;
                racial_cd_ready = now + 180.0;
                if (record_timeline) {
                    result.cast_sequence.push_back({now, SpellID::RACIAL_BERSERKING, 0.0, false, false, 0.0, "Racial Cooldown"});
                }
            } else if (race == Race::GNOME) {
                eureka_charges = 3;
                racial_cd_ready = now + 120.0;
                if (record_timeline) {
                    result.cast_sequence.push_back({now, SpellID::RACIAL_EUREKA, 0.0, false, false, 0.0, "Racial Ability (+10% 3 casts)"});
                }
            }
        }

        // 3. Sequential Rule-Based Priority Evaluation
        for (const auto& rule : priority_rules) {
            if (!rule.enabled) continue;

            switch (rule.action) {
                case PriorityAction::RACIAL_EUREKA:
                case PriorityAction::RACIAL_BLOOD_FURY:
                case PriorityAction::RACIAL_BERSERKING:
                    // Handled above in off-GCD check
                    break;

                case PriorityAction::LIFE_TAP: {
                    double mana_pct = (player_mana / stats.max_mana) * 100.0;
                    if (mana_pct <= policy.life_tap_threshold_pct && player_health > 800.0) {
                        double base_tap = 580.0;
                        double extra_mana = (base_tap + 0.80 * stats.effective_shadow_power()) * (1.0 + 0.10 * talents.aff.improved_life_tap);
                        player_mana = std::min(stats.max_mana, player_mana + extra_mana);
                        player_health -= base_tap;
                        result.life_taps++;
                        result.mana_gained += extra_mana;

                        // Demonic Energies: Pet gains 50%/100% of Mana gained from Life Tap
                        if (talents.demo.demonic_energies > 0 && active_pet != PetChoice::NONE) {
                            double pet_gain = extra_mana * (0.50 * talents.demo.demonic_energies);
                            pet_mana = std::min(pet_max_mana, pet_mana + pet_gain);
                        }

                        gcd_ready_time = now + mechanics.base_gcd;
                        queue.push(gcd_ready_time, EventType::GCD_READY);

                        if (record_timeline) {
                            result.timeline.push_back({now, 0.0, SpellID::LIFE_TAP, false, false, player_mana, target.isb_charges});
                            result.cast_sequence.push_back({now, SpellID::LIFE_TAP, 0.0, false, false, 0.0, "Mana Tap"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::CURSE_OF_AGONY: {
                    if (!dot_agony.active) {
                        double mana_cost = 215.0;
                        if (player_mana >= mana_cost) {
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;

                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                dot_agony.active = true;
                                dot_agony.expire_time = now + 24.0;
                                dot_agony.ticks_remaining = 12;
                                dot_agony.tick_interval = 2.0;
                                double sp = get_current_sp(School::SHADOW, now);
                                dot_agony.tick_damage = (1044.0 / 12.0) + (sp / 12.0);
                                dot_agony.tick_multiplier = get_current_shadow_multiplier(now) * (1.0 + talents.aff.improved_bane_of_agony * 0.05) * malediction_mult;
                                queue.push(now + 2.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CURSE_OF_AGONY));
                            } else {
                                result.misses++;
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::CURSE_OF_AGONY, 0.0, false, false, 0.0, (now < 1.0) ? "Opener DoT" : "Curse DoT"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::CURSE_OF_DOOM: {
                    if (!dot_agony.active) {
                        double mana_cost = 300.0;
                        if (player_mana >= mana_cost) {
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                dot_agony.active = true;
                                dot_agony.expire_time = now + 60.0;
                                dot_agony.ticks_remaining = 1;
                                dot_agony.tick_interval = 60.0;
                                queue.push(now + 60.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CURSE_OF_DOOM));
                            } else {
                                result.misses++;
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::NIGHTFALL_SHADOW_BOLT: {
                    if (shadow_trance_active && policy.cast_nightfall_procs) {
                        shadow_trance_active = false;
                        double mana_cost = 380.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                        if (player_mana >= mana_cost) {
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.shadow_bolt_casts++;

                            double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                            queue.push(now + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SHADOW_BOLT));

                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SHADOW_BOLT, 0.0, false, false, 0.0, "Nightfall Instant"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DECIMATION_SOUL_FIRE: {
                    if (execute_phase && talents.demo.decimation > 0 && now >= soul_fire_cd_ready) {
                        double sf_mana = 335.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                        if (player_mana >= sf_mana) {
                            double cast_time = std::max(0.5, (4.0 - 0.4 * talents.destro.bane) * (1.0 - 0.20 * talents.demo.decimation) * get_haste_mult(now));
                            is_casting = true;
                            current_casting_spell = SpellID::SOUL_FIRE;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SOUL_FIRE));
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SOUL_FIRE, 0.0, false, false, cast_time, "Decimation Execute"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::SIPHON_LIFE: {
                    if (talents.aff.siphon_life > 0 && !dot_siphon_life.active) {
                        double mana_cost = 150.0;
                        if (player_mana >= mana_cost) {
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;

                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                dot_siphon_life.active = true;
                                dot_siphon_life.expire_time = now + 30.0;
                                dot_siphon_life.ticks_remaining = 10;
                                dot_siphon_life.tick_interval = 3.0;
                                double sp = get_current_sp(School::SHADOW, now);
                                dot_siphon_life.tick_damage = 15.0 + (0.10 * sp);
                                dot_siphon_life.tick_multiplier = get_current_shadow_multiplier(now) * malediction_mult;
                                queue.push(now + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::SIPHON_LIFE));
                            } else {
                                result.misses++;
                            }
                            gcd_ready_time = now + mechanics.base_gcd;
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::SIPHON_LIFE, 0.0, false, false, 0.0, (now < 3.0) ? "Opener DoT" : "DoT Refresh"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DRAIN_HOPE: {
                    if (talents.aff.drain_hope > 0 && now >= drain_hope_cd_ready) {
                        double dh_mana = 240.0;
                        if (player_mana >= dh_mana) {
                            player_mana -= dh_mana;
                            result.mana_spent += dh_mana;
                            result.total_casts++;
                            drain_hope_cd_ready = now + 20.0;
                            drain_hope_channel_end = now + 6.0;

                            // Soul Siphon talent increases drain tick rate (17/34/50% faster)
                            double drain_speed_mult = 1.0 + talents.aff.soul_siphon * (0.50 / 3.0);
                            double haste = get_haste_mult(now);
                            double total_channel_time = (6.0 / drain_speed_mult) * haste;
                            double tick_interval = (1.0 / drain_speed_mult) * haste;

                            gcd_ready_time = now + total_channel_time;

                            for (int i = 1; i <= 6; ++i) {
                                queue.push(now + i * tick_interval, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_HOPE));
                            }
                            queue.push(gcd_ready_time, EventType::GCD_READY);
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::DRAIN_HOPE, 0.0, false, false, total_channel_time, "Channel DoT"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::IMMOLATE: {
                    if (!dot_immolate.active) {
                        double mana_cost = 380.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                        if (player_mana >= mana_cost) {
                            double cast_time = std::max(1.0, (2.0 - 0.1 * talents.destro.bane) * get_haste_mult(now)); // 1.5s with 5/5 Bane
                            is_casting = true;
                            current_casting_spell = SpellID::IMMOLATE;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::IMMOLATE));
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
                        double mana_cost = 265.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                        if (player_mana >= mana_cost) {
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.direct_spell_casts++;
                            conflagrate_cd_ready = now + 10.0;

                            bool is_crit = false;
                            double dmg = 0.0;
                            if (rng.chance(calculate_hit_chance(School::FIRE))) {
                                result.total_damage_events++;
                                double sp = get_current_sp(School::FIRE, now);
                                dmg = rng.range(578.0, 704.0) + (1.5 / 3.5) * sp;
                                dmg *= get_current_fire_multiplier(now) * stats.all_damage_multiplier * destro_spell_mult;

                                double crit_chance = calculate_crit_chance(School::FIRE, stats) + (talents.destro.fire_and_brimstone * 0.08);
                                is_crit = rng.chance(crit_chance);
                                if (is_crit) {
                                    dmg *= destro_crit_mult;
                                    result.direct_spell_crits++;
                                    result.total_damage_crits++;
                                    result.crits++;
                                }

                                dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);
                                if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                                if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                                result.dmg_conflagrate += dmg;
                                result.total_damage += dmg;

                                if (talents.destro.shadow_and_flame > 0) {
                                    shadow_and_flame_shadow_expire = now + 20.0;
                                }
                            } else {
                                result.misses++;
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

                case PriorityAction::SHADOWBURN: {
                    bool sb_cond = true;
                    if (eff_rotation == RotationChoice::FIRE_DESTRO) {
                        sb_cond = (talents.destro.shadow_and_flame > 0 && now >= shadow_and_flame_fire_expire) || (policy.shadowburn == ShadowburnPolicy::ON_COOLDOWN);
                    } else if (policy.shadowburn == ShadowburnPolicy::EXECUTE_ONLY) {
                        sb_cond = execute_phase;
                    }

                    if (talents.destro.shadowburn > 0 && sb_cond && now >= shadowburn_cd_ready) {
                        double mana_cost = 365.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                        if (player_mana >= mana_cost) {
                            player_mana -= mana_cost;
                            result.mana_spent += mana_cost;
                            result.total_casts++;
                            result.direct_spell_casts++;
                            shadowburn_cd_ready = now + 8.0;

                            bool crit = false;
                            double dmg = 0.0;
                            if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                result.total_damage_events++;
                                double sp = get_current_sp(School::SHADOW, now);
                                dmg = rng.range(450.0, 502.0) + (1.5 / 3.5) * sp;
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
                                if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                                if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                                result.dmg_shadowburn += dmg;
                                result.total_damage += dmg;

                                if (talents.destro.shadow_and_flame > 0) {
                                    shadow_and_flame_fire_expire = now + 20.0;
                                }
                            } else {
                                result.misses++;
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
                    if (!dot_corruption.active) {
                        double mana_cost = 290.0;
                        if (player_mana >= mana_cost) {
                            double cast_time = std::max(0.0, (2.0 - 0.4 * talents.aff.improved_corruption) * get_haste_mult(now));
                            if (cast_time == 0.0) {
                                player_mana -= mana_cost;
                                result.mana_spent += mana_cost;
                                result.total_casts++;

                                if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                                    dot_corruption.active = true;
                                    dot_corruption.expire_time = now + 18.0;
                                    dot_corruption.ticks_remaining = 6;
                                    dot_corruption.tick_interval = 3.0;
                                    double sp = get_current_sp(School::SHADOW, now);
                                    dot_corruption.tick_damage = (822.0 / 6.0) + (sp / 6.0);
                                    dot_corruption.tick_multiplier = get_current_shadow_multiplier(now) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult;
                                    queue.push(now + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION));
                                } else {
                                    result.misses++;
                                }
                                gcd_ready_time = now + mechanics.base_gcd;
                                queue.push(gcd_ready_time, EventType::GCD_READY);
                                if (record_timeline) {
                                    result.cast_sequence.push_back({now, SpellID::CORRUPTION, 0.0, false, false, 0.0, (now < 3.0) ? "Opener DoT" : "DoT Refresh"});
                                }
                                return;
                            } else {
                                is_casting = true;
                                current_casting_spell = SpellID::CORRUPTION;
                                cast_finish_time = now + cast_time;
                                queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::CORRUPTION));
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
                    double sp_mana = 168.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    if (player_mana >= sp_mana) {
                        double cast_time = std::max(1.0, 1.5 * get_haste_mult(now));
                        is_casting = true;
                        current_casting_spell = SpellID::SEARING_PAIN;
                        cast_finish_time = now + cast_time;
                        queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SEARING_PAIN));
                        if (record_timeline) {
                            result.cast_sequence.push_back({now, SpellID::SEARING_PAIN, 0.0, false, false, cast_time, "Primary Fire Filler"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::INCINERATE_FILLER: {
                    if (talents.destro.incinerate > 0) {
                        double inc_mana = 355.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                        if (player_mana >= inc_mana) {
                            double cast_time = std::max(1.0, (2.5 - 0.1 * talents.destro.bane) * get_haste_mult(now)); // 2.0s with 5/5 Bane
                            is_casting = true;
                            current_casting_spell = SpellID::INCINERATE;
                            cast_finish_time = now + cast_time;
                            queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::INCINERATE));
                            if (record_timeline) {
                                result.cast_sequence.push_back({now, SpellID::INCINERATE, 0.0, false, false, cast_time, "Primary Filler"});
                            }
                            return;
                        }
                    }
                    break;
                }

                case PriorityAction::DRAIN_LIFE_FILLER: {
                    double dl_mana = 300.0;
                    if (player_mana >= dl_mana) {
                        player_mana -= dl_mana;
                        result.mana_spent += dl_mana;
                        result.total_casts++;

                        // Soul Siphon talent increases drain tick rate (17/34/50% faster)
                        double drain_speed_mult = 1.0 + talents.aff.soul_siphon * (0.50 / 3.0);
                        double haste = get_haste_mult(now);
                        double total_channel_time = (5.0 / drain_speed_mult) * haste;
                        double tick_interval = (1.0 / drain_speed_mult) * haste;

                        gcd_ready_time = now + total_channel_time;

                        for (int i = 1; i <= 5; ++i) {
                            queue.push(now + i * tick_interval, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_LIFE));
                        }
                        queue.push(gcd_ready_time, EventType::GCD_READY);
                        if (record_timeline) {
                            result.cast_sequence.push_back({now, SpellID::DRAIN_LIFE, 0.0, false, false, total_channel_time, "Drain Life (Filler)"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::DRAIN_SOUL_FILLER: {
                    double ds_mana = 290.0;
                    if (player_mana >= ds_mana) {
                        player_mana -= ds_mana;
                        result.mana_spent += ds_mana;
                        result.total_casts++;

                        // Soul Siphon talent increases drain tick rate (17/34/50% faster)
                        double drain_speed_mult = 1.0 + talents.aff.soul_siphon * (0.50 / 3.0);
                        double haste = get_haste_mult(now);
                        double total_channel_time = (15.0 / drain_speed_mult) * haste;
                        double tick_interval = (3.0 / drain_speed_mult) * haste;

                        gcd_ready_time = now + total_channel_time;

                        for (int i = 1; i <= 5; ++i) {
                            queue.push(now + i * tick_interval, EventType::CHANNEL_TICK, static_cast<uint8_t>(SpellID::DRAIN_SOUL));
                        }
                        queue.push(gcd_ready_time, EventType::GCD_READY);
                        if (record_timeline) {
                            result.cast_sequence.push_back({now, SpellID::DRAIN_SOUL, 0.0, false, false, total_channel_time, "Drain Soul (Filler)"});
                        }
                        return;
                    }
                    break;
                }

                case PriorityAction::SHADOW_BOLT_FILLER: {
                    double sb_mana = 380.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    if (player_mana >= sb_mana) {
                        double cast_time = std::max(1.0, (3.0 - 0.1 * talents.destro.bane) * get_haste_mult(now)); // 2.5s with 5/5 Bane
                        is_casting = true;
                        current_casting_spell = SpellID::SHADOW_BOLT;
                        cast_finish_time = now + cast_time;
                        queue.push(cast_finish_time, EventType::CAST_FINISH, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
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
        double base_tap = 580.0;
        double extra_mana = (base_tap + 0.80 * stats.effective_shadow_power()) * (1.0 + 0.10 * talents.aff.improved_life_tap);
        player_mana = std::min(stats.max_mana, player_mana + extra_mana);
        player_health -= base_tap;
        result.life_taps++;
        result.mana_gained += extra_mana;

        // Demonic Energies: Pet gains 50%/100% of Mana gained from Life Tap
        if (talents.demo.demonic_energies > 0 && active_pet != PetChoice::NONE) {
            double pet_gain = extra_mana * (0.50 * talents.demo.demonic_energies);
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
                    double sb_mana = 380.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    player_mana -= sb_mana;
                    result.mana_spent += sb_mana;

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SHADOW_BOLT));
                    gcd_ready_time = current_time;
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SEARING_PAIN)) {
                    result.direct_spell_casts++;
                    double sp_mana = 168.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    player_mana -= sp_mana;
                    result.mana_spent += sp_mana;

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SEARING_PAIN));
                    gcd_ready_time = current_time;
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::INCINERATE)) {
                    result.direct_spell_casts++;
                    double inc_mana = 355.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    player_mana -= inc_mana;
                    result.mana_spent += inc_mana;

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::INCINERATE));
                    gcd_ready_time = current_time;
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SOUL_FIRE)) {
                    result.direct_spell_casts++;
                    double sf_mana = 335.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    player_mana -= sf_mana;
                    result.mana_spent += sf_mana;
                    soul_fire_cd_ready = current_time + (60.0 * (1.0 - 0.45 * talents.demo.decimation));

                    double travel = mechanics.projectile_travel_time ? (mechanics.default_boss_distance_yards / mechanics.projectile_speed_yards_per_sec) : 0.0;
                    queue.push(current_time + travel, EventType::SPELL_IMPACT, static_cast<uint8_t>(SpellID::SOUL_FIRE));
                    gcd_ready_time = current_time;
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::IMMOLATE)) {
                    double imm_mana = 380.0 * (1.0 - 0.03 * talents.destro.cataclysm);
                    player_mana -= imm_mana;
                    result.mana_spent += imm_mana;
                    result.direct_spell_casts++;

                    if (rng.chance(calculate_hit_chance(School::FIRE))) {
                        result.total_damage_events++;
                        double sp = get_current_sp(School::FIRE, current_time);
                        // Aftermath (Destro Row 2 Col 3): +10% initial Immolate damage per point (+50% at 5/5)
                        double aftermath_mult = 1.0 + talents.destro.aftermath * 0.10;
                        double dmg = (rng.range(258.0, 306.0) * aftermath_mult) + 0.20 * sp;
                        dmg *= get_current_fire_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;
                        bool crit = rng.chance(calculate_crit_chance(School::FIRE, stats));
                        if (crit) {
                            dmg *= destro_crit_mult;
                            result.direct_spell_crits++;
                            result.total_damage_crits++;
                            result.crits++;
                        }
                        dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                        if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        if (race == Race::UNDEAD && rng.chance(0.15)) {
                            double grave_dmg = (rng.range(95.0, 115.0) + 0.10 * stats.effective_shadow_power()) * get_current_shadow_multiplier(current_time);
                            result.total_damage += grave_dmg;
                            player_health = std::min(stats.max_health, player_health + grave_dmg);
                        }

                        result.dmg_immolate += dmg;
                        result.total_damage += dmg;

                        dot_immolate.active = true;
                        dot_immolate.expire_time = current_time + 15.0;
                        dot_immolate.ticks_remaining = 5;
                        dot_immolate.tick_interval = 3.0;
                        dot_immolate.tick_damage = (485.0 / 5.0) + (0.65 / 5.0) * sp;
                        dot_immolate.tick_multiplier = get_current_fire_multiplier(current_time) * destro_spell_mult * malediction_mult;
                        queue.push(current_time + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::IMMOLATE));
                    } else {
                        result.misses++;
                    }
                    gcd_ready_time = current_time;
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::CORRUPTION)) {
                    double corr_mana = 290.0;
                    player_mana -= corr_mana;
                    result.mana_spent += corr_mana;

                    if (rng.chance(calculate_hit_chance(School::SHADOW))) {
                        dot_corruption.active = true;
                        dot_corruption.expire_time = current_time + 18.0;
                        dot_corruption.ticks_remaining = 6;
                        dot_corruption.tick_interval = 3.0;
                        double sp = get_current_sp(School::SHADOW, current_time);
                        dot_corruption.tick_damage = (822.0 / 6.0) + (sp / 6.0);
                        dot_corruption.tick_multiplier = get_current_shadow_multiplier(current_time) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult;
                        queue.push(current_time + 3.0, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION));
                    } else {
                        result.misses++;
                    }
                    gcd_ready_time = current_time;
                }


                decide_next_action(current_time);
                break;
            }

            case EventType::SPELL_IMPACT: {
                if (ev.spell_id == static_cast<uint8_t>(SpellID::SHADOW_BOLT)) {
                    if (!rng.chance(calculate_hit_chance(School::SHADOW))) {
                        result.misses++;
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
                    double base_dmg = rng.range(482.0, 538.0);
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

                    if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    if (race == Race::UNDEAD && rng.chance(0.15)) {
                        double grave_dmg = (rng.range(95.0, 115.0) + 0.10 * stats.effective_shadow_power()) * get_current_shadow_multiplier(current_time);
                        result.total_damage += grave_dmg;
                        player_health = std::min(stats.max_health, player_health + grave_dmg);
                    }

                    // Judgement of Wisdom
                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_shadow_bolt += dmg;
                    result.total_damage += dmg;

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
                    double base_dmg = rng.range(445.0, 515.0);
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

                    if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    if (race == Race::UNDEAD && rng.chance(0.15)) {
                        double grave_dmg = (rng.range(95.0, 115.0) + 0.10 * stats.effective_shadow_power()) * get_current_shadow_multiplier(current_time);
                        result.total_damage += grave_dmg;
                        player_health = std::min(stats.max_health, player_health + grave_dmg);
                    }

                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_incinerate += dmg;
                    result.total_damage += dmg;

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
                    double base_dmg = rng.range(204.0, 240.0);
                    double dmg = base_dmg + (1.5 / 3.5) * sp;

                    // Decimation bonus: +3% per point on Searing Pain when boss <35% HP
                    bool execute_phase = (current_time / fight_duration) >= 0.65;
                    if (execute_phase && talents.demo.decimation > 0) {
                        dmg *= (1.0 + talents.demo.decimation * 0.03);
                    }

                    dmg *= get_current_fire_multiplier(current_time) * stats.all_damage_multiplier * destro_spell_mult;

                    // Agonizing Flames: +3%/pt crit chance on Searing Pain
                    double crit_chance = calculate_crit_chance(School::FIRE, stats) + (talents.destro.agonizing_flames * 0.03);
                    bool is_crit = rng.chance(crit_chance);
                    if (is_crit) {
                        dmg *= destro_crit_mult;
                        result.direct_spell_crits++;
                        result.total_damage_crits++;
                        result.crits++;
                    }

                    dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                    // Demonic Brand: Brands the target for 10s (pet's next 2 attacks deal bonus damage)
                    if (talents.demo.demonic_brand > 0) {
                        demonic_brand_charges = 2;
                        demonic_brand_expire = current_time + 10.0;
                    }

                    if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    if (race == Race::UNDEAD && rng.chance(0.15)) {
                        double grave_dmg = (rng.range(95.0, 115.0) + 0.10 * stats.effective_shadow_power()) * get_current_shadow_multiplier(current_time);
                        result.total_damage += grave_dmg;
                        player_health = std::min(stats.max_health, player_health + grave_dmg);
                    }

                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_searing_pain += dmg;
                    result.total_damage += dmg;

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
                    double base_dmg = rng.range(715.0, 895.0);
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

                    if (race == Race::GNOME && eureka_charges > 0) { dmg *= 1.10; eureka_charges--; }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    if (race == Race::UNDEAD && rng.chance(0.15)) {
                        double grave_dmg = (rng.range(95.0, 115.0) + 0.10 * stats.effective_shadow_power()) * get_current_shadow_multiplier(current_time);
                        result.total_damage += grave_dmg;
                        player_health = std::min(stats.max_health, player_health + grave_dmg);
                    }

                    if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                        player_mana = std::min(stats.max_mana, player_mana + 59.0);
                        result.mana_gained += 59.0;
                    }

                    result.dmg_soul_fire += dmg;
                    result.total_damage += dmg;

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
                if (ev.spell_id == static_cast<uint8_t>(SpellID::DRAIN_HOPE)) {
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double dmg = 52.0 + (0.166667 * sp);
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier;

                    // Baseline DoT Crit + Pandemic bonus (Affliction)
                    if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
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
                    result.total_damage += dmg;

                    // Nightfall proc check on Drain Hope ticks (2% per pt = 4% at 2/2)
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

                    // Improved Drains talent (+2%/4%/6% per active Affliction effect on target)
                    int aff_count = (dot_corruption.active ? 1 : 0) + (dot_agony.active ? 1 : 0) + (dot_siphon_life.active ? 1 : 0);
                    double imp_drains_mult = 1.0 + (talents.aff.improved_drains * 0.02 * aff_count);
                    dmg *= imp_drains_mult;

                    // Drain Hope amplification (+10% to other Shadow DoTs/drains)
                    double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                    // Baseline DoT Crit + Pandemic bonus (Affliction)
                    if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
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
                    result.total_damage += dmg;

                    // Health restored from Drain Life (Soul Siphon reduces healing by 10/20/30%)
                    double heal = dmg * (1.0 - talents.aff.soul_siphon * 0.10);
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

                    // Improved Drains talent (+2%/4%/6% per active Affliction effect on target, tripled below 20% HP)
                    int aff_count = (dot_corruption.active ? 1 : 0) + (dot_agony.active ? 1 : 0) + (dot_siphon_life.active ? 1 : 0);
                    bool execute_20 = (current_time / fight_duration) >= 0.80; // Target <20% HP
                    double imp_drain_bonus_pct = talents.aff.improved_drains * 0.02 * (execute_20 ? 3.0 : 1.0);
                    double imp_drains_mult = 1.0 + (imp_drain_bonus_pct * aff_count);
                    dmg *= imp_drains_mult;

                    // Drain Hope amplification (+10% to other Shadow DoTs/drains)
                    double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                    // Baseline DoT Crit + Pandemic bonus (Affliction)
                    if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
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
                    result.total_damage += dmg;

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
                if (ev.spell_id == static_cast<uint8_t>(SpellID::CORRUPTION)) {
                    if (dot_corruption.active && dot_corruption.ticks_remaining > 0) {
                        dot_corruption.ticks_remaining--;
                        result.total_damage_events++;
                        double dmg = dot_corruption.tick_damage;

                        // Dynamic DoT scaling (No snapshotting in Forever)
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::SHADOW, current_time);
                            dmg = (822.0 / 6.0) + (sp / 6.0);
                        }

                        // Drain Hope amplification (+10% to other Shadow DoTs)
                        double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                        dmg *= get_current_shadow_multiplier(current_time) * (1.0 + talents.aff.improved_corruption * 0.02) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                        // Baseline DoT Crit + Pandemic bonus (Affliction)
                        if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
                            result.total_damage_crits++;
                            result.crits++;
                            double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                            dmg *= pand_crit_mult;
                        }

                        if (mechanics.isb_all_shadow_sources && target.consume_isb_charge(current_time)) {
                            dmg *= (1.0 + target.isb_bonus);
                            result.isb_consumed++;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_corruption += dmg;
                        result.total_damage += dmg;

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

                        if (dot_corruption.ticks_remaining > 0 && current_time + dot_corruption.tick_interval <= dot_corruption.expire_time) {
                            queue.push(current_time + dot_corruption.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CORRUPTION));
                        } else {
                            dot_corruption.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::CURSE_OF_AGONY)) {
                    if (dot_agony.active && dot_agony.ticks_remaining > 0) {
                        dot_agony.ticks_remaining--;
                        result.total_damage_events++;
                        int tick_index = 12 - dot_agony.ticks_remaining;
                        double ramp = (tick_index <= 4) ? 0.50 : (tick_index <= 8 ? 1.0 : 1.50);

                        double base_tick = dot_agony.tick_damage;
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::SHADOW, current_time);
                            base_tick = (1044.0 / 12.0) + (sp / 12.0);
                        }

                        double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                        double dmg = base_tick * ramp * get_current_shadow_multiplier(current_time) * (1.0 + talents.aff.improved_bane_of_agony * 0.05) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                        // Baseline DoT Crit + Pandemic bonus (Affliction)
                        if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
                            result.total_damage_crits++;
                            result.crits++;
                            double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                            dmg *= pand_crit_mult;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_agony += dmg;
                        result.dmg_curse += dmg;
                        result.total_damage += dmg;

                        if (dot_agony.ticks_remaining > 0) {
                            queue.push(current_time + dot_agony.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::CURSE_OF_AGONY));
                        } else {
                            dot_agony.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::SIPHON_LIFE)) {
                    if (dot_siphon_life.active && dot_siphon_life.ticks_remaining > 0) {
                        dot_siphon_life.ticks_remaining--;
                        result.total_damage_events++;
                        double dmg = dot_siphon_life.tick_damage;
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::SHADOW, current_time);
                            dmg = 15.0 + (0.10 * sp);
                        }
                        double drain_hope_mult = (current_time < drain_hope_channel_end) ? 1.10 : 1.0;
                        dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier * drain_hope_mult;

                        // Baseline DoT Crit + Pandemic bonus (Affliction)
                        if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
                            result.total_damage_crits++;
                            result.crits++;
                            double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                            dmg *= pand_crit_mult;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_siphon_life += dmg;
                        result.total_damage += dmg;
                        player_health = std::min(stats.max_health, player_health + dmg);

                        if (dot_siphon_life.ticks_remaining > 0) {
                            queue.push(current_time + dot_siphon_life.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::SIPHON_LIFE));
                        } else {
                            dot_siphon_life.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::IMMOLATE)) {
                    if (dot_immolate.active && dot_immolate.ticks_remaining > 0) {
                        dot_immolate.ticks_remaining--;
                        result.total_damage_events++;
                        double dmg = dot_immolate.tick_damage;
                        if (!mechanics.snapshot_dots) {
                            double sp = get_current_sp(School::FIRE, current_time);
                            dmg = (485.0 / 5.0) + (0.65 / 5.0) * sp;
                        }
                        dmg *= get_current_fire_multiplier(current_time) * destro_spell_mult * malediction_mult * stats.all_damage_multiplier;

                        // Baseline DoT Crit + Ruin bonus (Destruction)
                        if (rng.chance(calculate_crit_chance(School::FIRE, stats))) {
                            result.total_damage_crits++;
                            result.crits++;
                            dmg *= destro_crit_mult;
                        }

                        if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                        result.dmg_immolate += dmg;
                        result.total_damage += dmg;

                        if (dot_immolate.ticks_remaining > 0) {
                            queue.push(current_time + dot_immolate.tick_interval, EventType::DOT_TICK, static_cast<uint8_t>(SpellID::IMMOLATE));
                        } else {
                            dot_immolate.active = false;
                        }
                    }
                } else if (ev.spell_id == static_cast<uint8_t>(SpellID::CURSE_OF_DOOM)) {
                    result.total_damage_events++;
                    double sp = get_current_sp(School::SHADOW, current_time);
                    double dmg = 3200.0 + 2.0 * sp;
                    dmg *= get_current_shadow_multiplier(current_time) * malediction_mult * stats.all_damage_multiplier;
                    if (rng.chance(calculate_crit_chance(School::SHADOW, stats))) {
                        result.total_damage_crits++;
                        result.crits++;
                        double pand_crit_mult = 1.0 + 0.50 * (1.0 + talents.aff.pandemic * 0.33333333);
                        dmg *= pand_crit_mult;
                    }
                    if (race == Race::TROLL && target.is_beast) { dmg *= 1.05; }
                    result.dmg_doom += dmg;
                    result.dmg_curse += dmg;
                    result.total_damage += dmg;
                    dot_agony.active = false;
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
                    if (rng.chance(0.95)) {
                        double master_sp = get_current_sp(School::SHADOW, current_time);
                        double bonus_ap = mechanics.pet_scaling ? (mechanics.pet_sp_ratio * master_sp) : 0.0;
                        double base_swing = rng.range(145.0, 195.0) + (bonus_ap / 14.0) * 2.0;

                        // Unholy Power in Forever: +2% per point (+10% at 5/5)
                        base_swing *= (1.0 + talents.demo.unholy_power * 0.02);

                        // Orc Command: +5% pet damage
                        if (race == Race::ORC) base_swing *= 1.05;

                        double armor_mult = 0.86;
                        double swing_dmg = base_swing * armor_mult;

                        if (rng.chance(0.05)) {
                            swing_dmg *= 2.0;
                        }

                        // Judgement of Wisdom proc on pet melee hit (50% chance to restore 59 mana)
                        if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                            pet_mana = std::min(pet_max_mana, pet_mana + 59.0);
                        }

                        // Demonic Brand proc (Succubus deals bonus Shadow damage)
                        if (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && current_time < demonic_brand_expire) {
                            demonic_brand_charges--;
                            double brand_dmg = talents.demo.demonic_brand * rng.range(13.0, 14.0);
                            brand_dmg *= (1.0 + talents.demo.unholy_power * 0.02);
                            if (race == Race::ORC) brand_dmg *= 1.05;
                            if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) brand_dmg *= 1.15;
                            if (buffs.curse_of_shadows) brand_dmg *= 1.10;
                            brand_dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                            swing_dmg += brand_dmg;
                        }

                        result.dmg_pet += swing_dmg;
                        result.dmg_pet_succubus += swing_dmg;
                        result.total_damage += swing_dmg;
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

                        // Lash of Pain (Rank 6): 99 - 115 shadow damage + pet SP scaling
                        if (rng.chance(0.83)) {
                            double master_sp = get_current_sp(School::SHADOW, current_time);
                            double pet_sp = mechanics.pet_scaling ? (mechanics.pet_sp_ratio * master_sp) : 0.0;
                            double base_lop = rng.range(99.0, 115.0) + (1.5 / 3.5) * pet_sp;

                            // Unholy Power (+2%/pt) and Improved Sayaad (+10%/pt)
                            base_lop *= (1.0 + talents.demo.unholy_power * 0.02);
                            base_lop *= (1.0 + talents.demo.improved_sayaad * 0.10);

                            // Orc Command: +5% pet damage
                            if (race == Race::ORC) base_lop *= 1.05;

                            if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) base_lop *= 1.15;
                            if (buffs.curse_of_shadows) base_lop *= 1.10;

                            if (rng.chance(0.05)) {
                                base_lop *= 1.5;
                            }

                            base_lop *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);

                            // Judgement of Wisdom proc on pet spell hit
                            if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                                pet_mana = std::min(pet_max_mana, pet_mana + 59.0);
                            }

                            // Demonic Brand proc (Succubus deals bonus Shadow damage)
                            if (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && current_time < demonic_brand_expire) {
                                demonic_brand_charges--;
                                double brand_dmg = talents.demo.demonic_brand * rng.range(13.0, 14.0);
                                brand_dmg *= (1.0 + talents.demo.unholy_power * 0.02);
                                if (race == Race::ORC) brand_dmg *= 1.05;
                                if (buffs.shadow_weaving && !mechanics.personal_shadow_weaving) brand_dmg *= 1.15;
                                if (buffs.curse_of_shadows) brand_dmg *= 1.10;
                                brand_dmg *= calculate_partial_resist_multiplier(School::SHADOW, target.current_shadow_resistance, rng);
                                base_lop += brand_dmg;
                            }

                            result.dmg_pet += base_lop;
                            result.dmg_pet_succubus += base_lop;
                            result.total_damage += base_lop;
                        }
                    }

                    // Improved Sayaad reduces Lash of Pain cooldown by 1.0s per rank (9s -> 6s at 3/3)
                    double lop_cd = std::max(3.0, 9.0 - 1.0 * talents.demo.improved_sayaad);
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

                        // Imp Firebolt (Rank 7): 85 - 98 fire damage + pet SP scaling
                        if (rng.chance(0.83)) {
                            double master_sp = get_current_sp(School::FIRE, current_time);
                            double pet_sp = mechanics.pet_scaling ? (mechanics.pet_sp_ratio * master_sp) : 0.0;
                            double base_fb = rng.range(85.0, 98.0) + (1.5 / 3.5) * pet_sp;

                            base_fb *= (1.0 + talents.demo.unholy_power * 0.02);
                            base_fb *= (1.0 + talents.demo.improved_imp * 0.10);

                            // Orc Command: +5% pet damage
                            if (race == Race::ORC) base_fb *= 1.05;

                            if (buffs.curse_of_elements) base_fb *= 1.10;

                            if (rng.chance(0.05)) {
                                base_fb *= 1.5;
                            }

                            base_fb *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);

                            // Judgement of Wisdom proc on pet spell hit
                            if (buffs.judgement_of_wisdom && rng.chance(0.50)) {
                                pet_mana = std::min(pet_max_mana, pet_mana + 59.0);
                            }

                            // Demonic Brand proc (Imp deals bonus Fire damage)
                            if (talents.demo.demonic_brand > 0 && demonic_brand_charges > 0 && current_time < demonic_brand_expire) {
                                demonic_brand_charges--;
                                double brand_dmg = talents.demo.demonic_brand * rng.range(13.0, 14.0);
                                brand_dmg *= (1.0 + talents.demo.unholy_power * 0.02);
                                if (race == Race::ORC) brand_dmg *= 1.05;
                                if (buffs.curse_of_elements) brand_dmg *= 1.10;
                                brand_dmg *= calculate_partial_resist_multiplier(School::FIRE, target.current_fire_resistance, rng);
                                base_fb += brand_dmg;
                            }

                            result.dmg_pet += base_fb;
                            result.dmg_pet_imp += base_fb;
                            result.total_damage += base_fb;
                        }
                    }

                    // Improved Imp reduces Firebolt cast time / interval (1.5s down to 1.0s)
                    double fb_interval = std::max(0.5, 1.5 - 0.15 * talents.demo.improved_imp);
                    if (!can_cast) {
                        // If OOM, retry on 1.0s intervals
                        fb_interval = 1.0;
                    }
                    if (current_time + fb_interval < fight_duration) {
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
    target.update_isb_uptime(fight_duration);
    result.dps = result.total_damage / fight_duration;
    result.isb_uptime_percent = (target.total_isb_uptime / fight_duration) * 100.0;

    return result;
}

} // namespace warlock
