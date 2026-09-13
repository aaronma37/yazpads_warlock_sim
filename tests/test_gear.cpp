#include "test_framework.hpp"
#include "src/sim/gear.hpp"

using namespace warlock;

TEST_CASE(Gear, EmptyLoadoutDefaults) {
    GearLoadout loadout;
    Stats s = loadout.calculate_stats();
    // Default enchants: weapon (30) + head/legs (16) + shoulder (18) = 64 SP; gloves = 20 Shadow
    CHECK_NEAR(s.spell_power, 64.0, 0.001);
    CHECK_NEAR(s.shadow_power, 20.0, 0.001);
    CHECK_NEAR(s.stamina, 4.0, 0.001);   // Chest enchant
    CHECK_NEAR(s.intellect, 4.0, 0.001); // Chest enchant
}

TEST_CASE(Gear, EquipItemStats) {
    GearLoadout loadout;
    Item head;
    head.slot = Slot::HEAD;
    head.name = "Mish'undare, Circlet of the Mind Flayer";
    head.spell_power = 35.0;
    head.spell_crit = 2.0;
    head.intellect = 24.0;
    head.stamina = 15.0;

    loadout.equip(Slot::HEAD, head);
    Stats s = loadout.calculate_stats();

    CHECK_NEAR(s.spell_power, 64.0 + 35.0, 0.001);
    CHECK_NEAR(s.spell_crit_percent, 2.0, 0.001);
    CHECK_NEAR(s.intellect, 4.0 + 24.0, 0.001);
    CHECK_NEAR(s.stamina, 4.0 + 15.0, 0.001);
}

TEST_CASE(Gear, BloodvineSetBonus) {
    GearLoadout loadout;
    Item chest, legs, boots;
    chest.slot = Slot::CHEST;
    chest.set_name = "Bloodvine";
    legs.slot = Slot::LEGS;
    legs.set_name = "Bloodvine";
    boots.slot = Slot::FEET;
    boots.set_name = "Bloodvine";

    // 2 pieces: no set bonus
    loadout.equip(Slot::CHEST, chest);
    loadout.equip(Slot::LEGS, legs);
    Stats s2 = loadout.calculate_stats();
    CHECK_NEAR(s2.spell_hit_percent, 0.0, 0.001);

    // 3 pieces: +2% Spell Hit
    loadout.equip(Slot::FEET, boots);
    Stats s3 = loadout.calculate_stats();
    CHECK_NEAR(s3.spell_hit_percent, 2.0, 0.001);
}

TEST_CASE(Gear, NemesisSetBonus) {
    GearLoadout loadout;
    Item i1, i2, i3;
    i1.slot = Slot::HEAD;
    i1.set_name = "Nemesis";
    i2.slot = Slot::SHOULDERS;
    i2.set_name = "Nemesis";
    i3.slot = Slot::CHEST;
    i3.set_name = "Nemesis";

    loadout.equip(Slot::HEAD, i1);
    loadout.equip(Slot::SHOULDERS, i2);
    Stats s2 = loadout.calculate_stats();
    CHECK_NEAR(s2.spell_power, 64.0, 0.001);

    // 3 pieces: +23 Spell Power
    loadout.equip(Slot::CHEST, i3);
    Stats s3 = loadout.calculate_stats();
    CHECK_NEAR(s3.spell_power, 64.0 + 23.0, 0.001);
}

TEST_CASE(Gear, PresetPhaseLoadouts) {
    GearLoadout p3 = GearLoadout::create_phase3_bis();
    Stats s_p3 = p3.calculate_stats();
    CHECK(s_p3.spell_power > 400.0);
    CHECK(s_p3.spell_hit_percent >= 5.0);

    GearLoadout p6 = GearLoadout::create_phase6_bis();
    Stats s_p6 = p6.calculate_stats();
    CHECK(s_p6.spell_power > s_p3.spell_power);
}
