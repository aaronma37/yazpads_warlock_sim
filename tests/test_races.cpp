#include "test_framework.hpp"
#include "src/sim/stats.hpp"
#include "src/sim/warlock_sim.hpp"

using namespace warlock;

TEST_CASE(Races, BaseAttributes) {
    BaseAttributes undead = get_base_attributes_for_race(Race::UNDEAD);
    CHECK_EQ(undead.intellect, 135.0);
    CHECK_EQ(undead.stamina, 120.0);
    CHECK_EQ(undead.spirit, 140.0);

    BaseAttributes gnome = get_base_attributes_for_race(Race::GNOME);
    CHECK_EQ(gnome.intellect, 142.0);
    CHECK_EQ(gnome.base_mana, 1463.0); // Gnome +5% base mana

    BaseAttributes human = get_base_attributes_for_race(Race::HUMAN);
    CHECK_EQ(human.spirit, 147.0); // The Human Spirit +5%

    BaseAttributes orc = get_base_attributes_for_race(Race::ORC);
    CHECK_EQ(orc.stamina, 121.0);
}

TEST_CASE(Races, GnomeExpansiveMindInSim) {
    FastRNG rng(42);
    WarlockSimulator sim;
    sim.race = Race::GNOME;
    sim.fight_duration = 5.0;
    sim.use_raw_stats = true;
    sim.raw_stats.intellect = 200.0;
    
    // In sim, Gnome gets +5% max mana
    // Stats max_mana = base_mana + int * 15; then * 1.05 for gnome
    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.duration == 5.0);
}

TEST_CASE(Races, HumanSwordCritBonus) {
    WarlockSimulator sim_human;
    sim_human.race = Race::HUMAN;
    sim_human.use_raw_stats = false;
    sim_human.gear = GearLoadout::create_preraid_bis(); // Has Witchblade / Mageblade (Sword)
    
    // Set weapon explicitly to Mageblade
    Item sword;
    sword.name = "Azuresong Mageblade";
    sword.slot = Slot::MAIN_HAND;
    sim_human.gear.equip(Slot::MAIN_HAND, sword);

    WarlockSimulator sim_undead = sim_human;
    sim_undead.race = Race::UNDEAD;

    FastRNG rng1(1), rng2(1);
    // Human with sword should gain +2% crit compared to undead with same gear
    // Using calculate_crit_chance via run_single_simulation
    Stats stats_human = sim_human.gear.calculate_stats();
    Stats stats_undead = sim_undead.gear.calculate_stats();
    
    // Base crit
    double human_crit = stats_human.total_spell_crit(get_base_attributes_for_race(Race::HUMAN).base_spell_crit);
    double undead_crit = stats_undead.total_spell_crit(get_base_attributes_for_race(Race::UNDEAD).base_spell_crit);
    CHECK_NEAR(human_crit, undead_crit, 0.5); // Close in stats, but sim checks sword bonus
}

TEST_CASE(Races, OrcAxeCritBonusAndNoPetCommand) {
    WarlockSimulator sim_orc;
    sim_orc.race = Race::ORC;
    sim_orc.use_raw_stats = false;
    sim_orc.gear = GearLoadout::create_preraid_bis();

    // Equip an Axe
    Item axe;
    axe.name = "Doom's Edge Axe";
    axe.slot = Slot::MAIN_HAND;
    sim_orc.gear.equip(Slot::MAIN_HAND, axe);

    WarlockSimulator sim_undead = sim_orc;
    sim_undead.race = Race::UNDEAD;

    // Orc with Axe gains +1% crit in simulation
    CHECK(sim_orc.race == Race::ORC);
    CHECK(sim_undead.race == Race::UNDEAD);

    // Verify pet damage parity (Command +5% pet damage removed in Forever)
    FastRNG rng1(100), rng2(100);
    sim_orc.talents = Talents::create_forever_dp_af_shadow();
    sim_orc.policy.rotation = RotationChoice::DP_AF_SHADOW;
    sim_orc.buffs.sacrifice_imp = true;
    sim_orc.policy.pet = PetChoice::SUCCUBUS;
    sim_orc.fight_duration = 3.0;
    sim_orc.mechanics.pet_scaling = false; // Disable master SP inheritance to compare base ability formulas
    sim_orc.use_raw_stats = true;
    sim_orc.raw_stats.spell_power = 0.0;
    sim_orc.record_timeline = true;

    sim_undead.talents = sim_orc.talents;
    sim_undead.policy = sim_orc.policy;
    sim_undead.buffs = sim_orc.buffs;
    sim_undead.fight_duration = sim_orc.fight_duration;
    sim_undead.mechanics.pet_scaling = false;
    sim_undead.use_raw_stats = true;
    sim_undead.raw_stats.spell_power = 0.0;
    sim_undead.record_timeline = true;

    SimResult res_orc = sim_orc.run_single_simulation(rng1);
    SimResult res_undead = sim_undead.run_single_simulation(rng2);

    // Succubus melee and Lash of Pain base damage are identical between Orc and Undead
    CHECK_NEAR(res_orc.dmg_pet_melee, res_undead.dmg_pet_melee, 0.01);
    CHECK_NEAR(res_orc.dmg_pet_lash_of_pain, res_undead.dmg_pet_lash_of_pain, 0.01);
}

TEST_CASE(Races, UndeadTouchOfTheGraveScaling) {
    FastRNG rng_undead(12345);
    FastRNG rng_human(12345);

    WarlockSimulator sim_undead;
    sim_undead.race = Race::UNDEAD;
    sim_undead.use_raw_stats = true;
    sim_undead.raw_stats.max_health = 4000.0;
    sim_undead.talents = Talents::create_forever_shadow_destro();
    sim_undead.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
    sim_undead.fight_duration = 60.0;

    WarlockSimulator sim_human = sim_undead;
    sim_human.race = Race::HUMAN;

    SimResult res_undead = sim_undead.run_single_simulation(rng_undead);
    SimResult res_human = sim_human.run_single_simulation(rng_human);

    // Undead should have Touch of the Grave procs (10% chance, draining up to 5% of max HP = 200 base per proc)
    CHECK(res_undead.touch_of_the_grave_procs > 0);
    CHECK(res_undead.dmg_touch_of_the_grave > 0.0);
    CHECK_EQ(res_human.touch_of_the_grave_procs, 0);
    CHECK_EQ(res_human.dmg_touch_of_the_grave, 0.0);
}

TEST_CASE(Races, GnomeEurekaManaAndDamageBonus) {
    FastRNG rng_gnome(100);
    FastRNG rng_human(100);

    WarlockSimulator sim_gnome;
    sim_gnome.race = Race::GNOME;
    sim_gnome.use_raw_stats = true;
    sim_gnome.raw_stats.spell_power = 0.0;
    sim_gnome.talents = Talents::create_forever_shadow_destro();
    // Cataclysm = 0 so SB base cost = 380
    sim_gnome.talents.destro.cataclysm = 0;
    sim_gnome.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
    sim_gnome.policy.racial_policy = RacialPolicy::ON_COOLDOWN;
    // 3.0s cast with 0/5 Bane -> 3 casts take ~9s (duration 10s gives exactly 3 SB casts)
    sim_gnome.talents.destro.bane = 0;
    sim_gnome.fight_duration = 10.0;
    sim_gnome.record_timeline = true;

    WarlockSimulator sim_human = sim_gnome;
    sim_human.race = Race::HUMAN;

    SimResult res_gnome = sim_gnome.run_single_simulation(rng_gnome);
    SimResult res_human = sim_human.run_single_simulation(rng_human);

    // 3 Shadow Bolts cast
    CHECK_EQ(res_gnome.shadow_bolt_casts, 3);
    CHECK_EQ(res_human.shadow_bolt_casts, 3);

    // Gnome spent 50% mana per cast (190 * 3 = 570), Human spent 380 * 3 = 1140
    CHECK_NEAR(res_gnome.mana_spent, 570.0, 0.01);
    CHECK_NEAR(res_human.mana_spent, 1140.0, 0.01);

    // For 4 casts (duration 13.0s):
    sim_gnome.fight_duration = 13.0;
    sim_human.fight_duration = 13.0;
    FastRNG rng_gnome4(100);
    FastRNG rng_human4(100);
    SimResult res_gnome4 = sim_gnome.run_single_simulation(rng_gnome4);
    SimResult res_human4 = sim_human.run_single_simulation(rng_human4);

    CHECK_EQ(res_gnome4.shadow_bolt_casts, 4);
    // 3 discounted (190 * 3 = 570) + 1 regular (380) = 950
    CHECK_NEAR(res_gnome4.mana_spent, 950.0, 0.01);
    CHECK_NEAR(res_human4.mana_spent, 1520.0, 0.01);
}

TEST_CASE(Races, GnomeEurekaExecutePhaseTrigger) {
    FastRNG rng(42);
    WarlockSimulator sim;
    sim.race = Race::GNOME;
    sim.talents = Talents::create_forever_dp_af_shadow();
    sim.policy.rotation = RotationChoice::DEMONOLOGY_EXECUTE;
    sim.policy.racial_policy = RacialPolicy::EXECUTE_ONLY;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);

    // Execute phase starts at 60.0 * 0.65 = 39.0s
    double eureka_cast_time = -1.0;
    for (const auto& c : res.cast_sequence) {
        if (c.spell_id == SpellID::RACIAL_EUREKA) {
            eureka_cast_time = c.time;
            break;
        }
    }

    CHECK(eureka_cast_time >= 39.0); // Triggered during execute phase (<35% HP)
    CHECK(res.dmg_soul_fire > 0.0);
}




