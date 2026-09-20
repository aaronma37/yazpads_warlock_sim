#include "test_framework.hpp"
#include "src/sim/mana_regen.hpp"
#include "src/sim/player_class.hpp"
#include "src/sim/stats.hpp"

using namespace sim;

TEST_CASE(ManaRegen, SpiritCoefficients) {
    CHECK_NEAR(ManaRegenCalculator::get_spirit_coefficient(PlayerClass::PRIEST), 0.009327, 0.000001);
    CHECK_NEAR(ManaRegenCalculator::get_spirit_coefficient(PlayerClass::WARLOCK), 0.007725, 0.000001);
}

TEST_CASE(ManaRegen, PriestSpiritRegenOutside5SR) {
    // Example: Priest with 200 Int, 250 Spirit
    // tick = 5.0 * (0.001 + sqrt(200) * 250 * 0.009327)
    // sqrt(200) = 14.1421356
    // sqrt(200) * 250 * 0.009327 = 32.97746
    // tick = 5.0 * 32.97846 = ~164.89
    double int_val = 200.0;
    double spirit_val = 250.0;
    double tick = ManaRegenCalculator::calculate_spirit_regen_per_tick(int_val, spirit_val, PlayerClass::PRIEST);
    CHECK(tick > 160.0);
    CHECK(tick < 170.0);
}

TEST_CASE(ManaRegen, Mp5TickCalculation) {
    // 50 MP5 -> 50 * 0.40 = 20 mana per 2-second tick
    double mp5_tick = ManaRegenCalculator::calculate_mp5_regen_per_tick(50.0);
    CHECK_NEAR(mp5_tick, 20.0, 0.001);
}

TEST_CASE(ManaRegen, FiveSecondRuleStateTracker) {
    FiveSecondRuleTracker tracker;
    CHECK(!tracker.is_inside_5sr(0.0));

    // Cast finishes and spends mana at t = 2.5s
    tracker.on_mana_spent(2.5);
    CHECK(tracker.is_inside_5sr(2.5));
    CHECK(tracker.is_inside_5sr(4.0));
    CHECK(tracker.is_inside_5sr(7.49));
    CHECK_NEAR(tracker.time_remaining_in_5sr(4.0), 3.5, 0.001);

    // Exactly at 7.5s (2.5 + 5.0) -> FSR expires
    CHECK(!tracker.is_inside_5sr(7.5));
    CHECK(!tracker.is_inside_5sr(8.0));
    CHECK_NEAR(tracker.time_remaining_in_5sr(8.0), 0.0, 0.001);
}

TEST_CASE(ManaRegen, Inside5SRWithAndWithoutMeditation) {
    double int_val = 200.0;
    double spirit_val = 200.0;
    double mp5_val = 30.0; // 12 mana per tick

    // 1. Outside 5SR: Full spirit regen + MP5
    double outside = ManaRegenCalculator::calculate_tick_mana(
        int_val, spirit_val, mp5_val, false, 0.0, PlayerClass::PRIEST
    );
    double base_spirit = ManaRegenCalculator::calculate_spirit_regen_per_tick(int_val, spirit_val, PlayerClass::PRIEST);
    CHECK_NEAR(outside, base_spirit + 12.0, 0.001);

    // 2. Inside 5SR without Meditation (0% regen while casting): MP5 only!
    double inside_no_med = ManaRegenCalculator::calculate_tick_mana(
        int_val, spirit_val, mp5_val, true, 0.0, PlayerClass::PRIEST
    );
    CHECK_NEAR(inside_no_med, 12.0, 0.001);

    // 3. Inside 5SR with 3/3 Meditation (15% spirit regen): MP5 + 15% spirit
    double inside_med15 = ManaRegenCalculator::calculate_tick_mana(
        int_val, spirit_val, mp5_val, true, 0.15, PlayerClass::PRIEST
    );
    CHECK_NEAR(inside_med15, 12.0 + (base_spirit * 0.15), 0.001);
}

TEST_CASE(ManaRegen, StatsHolyPower) {
    Stats s;
    s.spell_power = 100.0;
    s.holy_power = 50.0;
    CHECK_NEAR(s.effective_holy_power(), 150.0, 0.001);
    CHECK_NEAR(s.holy_multiplier, 1.0, 0.001);
    CHECK_NEAR(s.holy_crit_bonus_multiplier, 1.5, 0.001);
}

TEST_CASE(ManaRegen, ExpandedRaces) {
    CHECK_EQ(std::string(race_to_string(Race::DWARF)), "Dwarf");
    CHECK_EQ(std::string(race_to_string(Race::NIGHT_ELF)), "Night Elf");
    CHECK_EQ(std::string(race_faction(Race::DWARF)), "Alliance");
    CHECK_EQ(std::string(race_faction(Race::NIGHT_ELF)), "Alliance");

    BaseAttributes dwarf_attrs = get_base_attributes_for_race(Race::DWARF);
    CHECK(dwarf_attrs.stamina > 100.0);
    CHECK(dwarf_attrs.intellect > 100.0);

    BaseAttributes nelf_attrs = get_base_attributes_for_race(Race::NIGHT_ELF);
    CHECK(nelf_attrs.stamina > 100.0);
    CHECK(nelf_attrs.spirit > 100.0);

    BaseAttributes priest_human = get_base_attributes_for_class_and_race(PlayerClass::PRIEST, Race::HUMAN);
    CHECK_NEAR(priest_human.base_mana, 1456.0, 0.01);
    CHECK_NEAR(priest_human.base_spell_crit, 1.24, 0.01);
}
