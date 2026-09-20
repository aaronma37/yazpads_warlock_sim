#pragma once
#include <string>
#include <vector>
#include <array>
#include <memory>
#include "stats.hpp"

namespace sim {

enum class Slot : uint8_t {
    HEAD = 0,
    NECK,
    SHOULDERS,
    BACK,
    CHEST,
    WRISTS,
    HANDS,
    WAIST,
    LEGS,
    FEET,
    RING1,
    RING2,
    TRINKET1,
    TRINKET2,
    MAIN_HAND,
    OFF_HAND,
    RANGED,     // Wand
    TWO_HAND,
    COUNT
};

inline const char* slot_to_name(Slot s) {
    switch (s) {
        case Slot::HEAD: return "Head";
        case Slot::NECK: return "Neck";
        case Slot::SHOULDERS: return "Shoulders";
        case Slot::BACK: return "Back";
        case Slot::CHEST: return "Chest";
        case Slot::WRISTS: return "Wrists";
        case Slot::HANDS: return "Hands";
        case Slot::WAIST: return "Waist";
        case Slot::LEGS: return "Legs";
        case Slot::FEET: return "Feet";
        case Slot::RING1: return "Ring 1";
        case Slot::RING2: return "Ring 2";
        case Slot::TRINKET1: return "Trinket 1";
        case Slot::TRINKET2: return "Trinket 2";
        case Slot::MAIN_HAND: return "Main Hand";
        case Slot::OFF_HAND: return "Off Hand";
        case Slot::RANGED: return "Wand";
        case Slot::TWO_HAND: return "Two Hand";
        default: return "Unknown";
    }
}

enum class ItemQuality : uint8_t {
    COMMON = 0,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY
};

struct Item {
    int id = 0;
    std::string name;
    Slot slot = Slot::HEAD;
    ItemQuality quality = ItemQuality::EPIC;
    int phase = 1; // 1 to 6

    double stamina = 0.0;
    double intellect = 0.0;
    double spirit = 0.0;

    double spell_power = 0.0;
    double shadow_power = 0.0;
    double fire_power = 0.0;
    double spell_hit = 0.0;       // in percent (e.g. 1.0 = 1%)
    double spell_crit = 0.0;      // in percent (e.g. 1.0 = 1%)
    double spell_haste = 0.0;
    double mp5 = 0.0;

    std::string set_name = "";    // E.g. "Bloodvine", "Nemesis", "Plagueheart"
    std::string icon = "";        // E.g. "inv_helmet_08.jpg"

    // Trinket On-Use / Proc effect
    bool has_on_use = false;
    double on_use_spell_power = 0.0;
    double on_use_duration = 0.0;
    double on_use_cooldown = 0.0;
};

inline std::string get_default_slot_icon(Slot s) {
    switch (s) {
        case Slot::HEAD: return "INV_Helmet_01.png";
        case Slot::NECK: return "INV_Jewelry_Necklace_01.png";
        case Slot::SHOULDERS: return "INV_Shoulder_02.png";
        case Slot::BACK: return "INV_Misc_Cape_18.png";
        case Slot::CHEST: return "INV_Chest_Cloth_17.png";
        case Slot::WRISTS: return "INV_Bracer_07.png";
        case Slot::HANDS: return "INV_Gauntlets_19.png";
        case Slot::WAIST: return "INV_Belt_03.png";
        case Slot::LEGS: return "INV_Pants_Cloth_05.png";
        case Slot::FEET: return "INV_Boots_05.png";
        case Slot::RING1:
        case Slot::RING2: return "INV_Jewelry_Ring_01.png";
        case Slot::TRINKET1:
        case Slot::TRINKET2: return "INV_Jewelry_Talisman_07.png";
        case Slot::MAIN_HAND: return "INV_Sword_39.png";
        case Slot::OFF_HAND: return "INV_Misc_Bag_10.png";
        case Slot::RANGED: return "INV_Wand_01.png";
        case Slot::TWO_HAND: return "INV_Staff_30.png";
        default: return "INV_Helmet_01.png";
    }
}

// Item database containing iconic Classic items
class ItemDatabase {
public:
    static const std::vector<Item>& all_items();
    static const Item* find_by_name(const std::string& name);
    static const Item* find_by_id(int id);
    static std::vector<Item> get_items_for_slot(Slot slot);
};

// Character Gear Loadout
struct GearLoadout {
    std::string name = "Custom Loadout";
    std::array<Item, static_cast<size_t>(Slot::COUNT)> items;

    // Stat overrides or bonuses (e.g. enchants)
    double extra_spell_power = 0.0; // Weapon +30, Head/Legs +18, Shoulders +18, etc.
    double extra_shadow_power = 0.0;
    double extra_spell_hit = 0.0;
    double extra_spell_crit = 0.0;

    void equip(Slot slot, const Item& item) {
        items[static_cast<size_t>(slot)] = item;
    }

    const Item& get(Slot slot) const {
        return items[static_cast<size_t>(slot)];
    }

    Item& get(Slot slot) {
        return items[static_cast<size_t>(slot)];
    }

    Stats calculate_stats() const {
        Stats s;
        int bloodvine_pieces = 0;
        int nemesis_pieces = 0;
        int plagueheart_pieces = 0;

        for (size_t i = 0; i < static_cast<size_t>(Slot::COUNT); ++i) {
            const auto& item = items[i];
            s.stamina += item.stamina;
            s.intellect += item.intellect;
            s.spirit += item.spirit;

            s.spell_power += item.spell_power;
            s.shadow_power += item.shadow_power;
            s.fire_power += item.fire_power;
            s.spell_hit_percent += item.spell_hit;
            s.spell_crit_percent += item.spell_crit;
            s.spell_haste_percent += item.spell_haste;
            s.mp5 += item.mp5;

            if (item.set_name == "Bloodvine") bloodvine_pieces++;
            if (item.set_name == "Nemesis") nemesis_pieces++;
            if (item.set_name == "Plagueheart") plagueheart_pieces++;
        }

        // Set Bonuses
        // Bloodvine Garb (3 items): +2% Spell Hit
        if (bloodvine_pieces >= 3) {
            s.spell_hit_percent += 2.0;
        }
        // Nemesis (3 items): +23 Spell Power
        if (nemesis_pieces >= 3) {
            s.spell_power += 23.0;
        }
        // Plagueheart (4 items): Corruption damage +12%
        // Handled in damage calculation

        // Add standard raid enchants:
        // Weapon: +30 Spell Damage
        // Head & Legs: +8 Spell Damage (Libram of Voracity / Zandalar Signet)
        // Shoulders: +18 Spell Damage (Zandalar Honor Token)
        // Wrists: +4 Stats (+4 int = ~0.06 crit)
        // Chest: +4 All Stats
        // Gloves: +20 Shadow Spell Damage (Darkmoon / Rider)
        // Boots: Minor Speed
        s.spell_power += extra_spell_power + 30.0 /*weapon*/ + 16.0 /*head/legs*/ + 18.0 /*shoulder*/;
        s.shadow_power += extra_shadow_power + 20.0 /*gloves*/;
        s.spell_hit_percent += extra_spell_hit;
        s.spell_crit_percent += extra_spell_crit;
        s.stamina += 4.0;
        s.intellect += 4.0;

        return s;
    }

    // Built-in iconic loadouts
    static GearLoadout create_preraid_bis();
    static GearLoadout create_phase3_bis(); // BWL / Bloodvine
    static GearLoadout create_phase5_bis(); // AQ40
    static GearLoadout create_phase6_bis(); // Naxxramas BiS
};

} // namespace sim

namespace warlock {
    using sim::Slot;
    using sim::slot_to_name;
    using sim::ItemQuality;
    using sim::Item;
    using sim::get_default_slot_icon;
    using sim::ItemDatabase;
    using sim::GearLoadout;
}

