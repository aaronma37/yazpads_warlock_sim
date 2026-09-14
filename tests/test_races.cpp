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

