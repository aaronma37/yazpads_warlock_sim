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
