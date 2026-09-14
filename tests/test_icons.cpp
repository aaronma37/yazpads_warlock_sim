#include "test_framework.hpp"
#include "src/sim/stats.hpp"
#include "src/sim/policy.hpp"
#include <string>
#include <set>

using namespace warlock;

TEST_CASE(Icons, RaceToIconCoversAllRaces) {
    // Small head icons read well at table size; full character portraits do not.
    CHECK_EQ(std::string(race_to_icon(Race::UNDEAD)), std::string("INV_Misc_Head_Undead_01.png"));
    CHECK_EQ(std::string(race_to_icon(Race::ORC)), std::string("INV_Misc_Head_Orc_01.png"));
    CHECK_EQ(std::string(race_to_icon(Race::TROLL)), std::string("INV_Misc_Head_Troll_01.png"));
    CHECK_EQ(std::string(race_to_icon(Race::HUMAN)), std::string("INV_Misc_Head_Human_01.png"));
    CHECK_EQ(std::string(race_to_icon(Race::GNOME)), std::string("INV_Misc_Head_Gnome_01.png"));

    std::set<std::string> seen;
    Race races[] = {Race::UNDEAD, Race::ORC, Race::TROLL, Race::HUMAN, Race::GNOME};
    for (Race r : races) {
        std::string icon = race_to_icon(r);
        CHECK(!icon.empty());
        CHECK(seen.find(icon) == seen.end()); // each race maps to a distinct file
        seen.insert(icon);
    }
    CHECK_EQ(seen.size(), (size_t)5);
}

TEST_CASE(Icons, PetChoiceToIcon) {
    CHECK_EQ(std::string(pet_choice_to_icon(PetChoice::IMP)), std::string("Spell_Shadow_SummonImp.png"));
    CHECK_EQ(std::string(pet_choice_to_icon(PetChoice::SUCCUBUS)), std::string("Spell_Shadow_SummonSuccubus.png"));
    CHECK(std::string(pet_choice_to_icon(PetChoice::NONE)).empty()); // nothing to depict when sacrificed / none
}
