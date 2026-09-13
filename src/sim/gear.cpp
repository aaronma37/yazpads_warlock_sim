#include "gear.hpp"
#include <unordered_map>

namespace warlock {

const std::vector<Item>& ItemDatabase::all_items() {
    static const std::vector<Item> db = {
        // --- HEAD ---
        {101, "Mish'undare, Circlet of the Mind Flayer", Slot::HEAD, ItemQuality::EPIC, 3, 15, 24, 9, 35, 0, 0, 0, 2.0, 0, 0, "", "inv_helmet_08.png"},
        {102, "Doomcaller's Circlet", Slot::HEAD, ItemQuality::EPIC, 5, 27, 21, 0, 33, 0, 0, 1.0, 1.0, 0, 0, "Doomcaller", "inv_crown_02.png"},
        {103, "Plagueheart Diadem", Slot::HEAD, ItemQuality::EPIC, 6, 29, 20, 0, 37, 0, 0, 1.0, 1.0, 0, 0, "Plagueheart", "inv_helmet_32.png"},
        {104, "Felheart Horns", Slot::HEAD, ItemQuality::EPIC, 1, 27, 20, 0, 20, 0, 0, 0, 0, 0, 0, "Felheart", "inv_helmet_08.png"},
        {105, "Spellweaver's Turban", Slot::HEAD, ItemQuality::RARE, 1, 0, 9, 0, 36, 0, 0, 1.0, 0, 0, 0, "", "inv_helmet_28.png"},

        // --- NECK ---
        {201, "Choker of the Fire Lord", Slot::NECK, ItemQuality::EPIC, 1, 0, 7, 0, 34, 0, 0, 0, 0, 0, 0, "", "inv_jewelry_necklace_08.png"},
        {202, "Charm of the Shifting Sands", Slot::NECK, ItemQuality::EPIC, 5, 12, 12, 0, 25, 0, 0, 1.0, 0, 0, 0, "", "inv_jewelry_necklace_20.png"},
        {203, "Gem of Nerubis", Slot::NECK, ItemQuality::EPIC, 6, 0, 0, 0, 21, 0, 0, 2.0, 1.0, 0, 0, "", "inv_misc_gem_pearl_04.png"},
        {204, "Star of Mystaria", Slot::NECK, ItemQuality::RARE, 1, 0, 9, 9, 0, 0, 0, 1.0, 0, 0, 0, "", "inv_jewelry_necklace_07.png"},
        {205, "Diana's Pearl Necklace", Slot::NECK, ItemQuality::RARE, 1, 0, 8, 8, 20, 0, 0, 0, 0, 0, 0, "", "inv_jewelry_necklace_08.png"},

        // --- SHOULDERS ---
        {301, "Mantle of the Blackwing Cabal", Slot::SHOULDERS, ItemQuality::EPIC, 3, 16, 16, 0, 34, 0, 0, 0, 0, 0, 0, "", "inv_shoulder_23.png"},
        {302, "Doomcaller's Mantle", Slot::SHOULDERS, ItemQuality::EPIC, 5, 23, 17, 0, 28, 0, 0, 1.0, 0, 0, 0, "Doomcaller", "inv_shoulder_22.png"},
        {303, "Plagueheart Shoulderpads", Slot::SHOULDERS, ItemQuality::EPIC, 6, 22, 16, 0, 36, 0, 0, 1.0, 0, 0, 0, "Plagueheart", "inv_shoulder_25.png"},
        {304, "Felcloth Epaulets", Slot::SHOULDERS, ItemQuality::RARE, 1, 0, 0, 0, 0, 26, 0, 0, 0, 0, 0, "", "inv_shoulder_23.png"},

        // --- BACK ---
        {401, "Cloak of the Brood Lord", Slot::BACK, ItemQuality::EPIC, 3, 12, 14, 0, 28, 0, 0, 0, 0, 0, 0, "", "inv_misc_cape_18.png"},
        {402, "Cloak of the Devoured", Slot::BACK, ItemQuality::EPIC, 5, 11, 10, 0, 30, 0, 0, 1.0, 0, 0, 0, "", "inv_misc_cape_06.png"},
        {403, "Cloak of the Necro-Stalker", Slot::BACK, ItemQuality::EPIC, 6, 17, 0, 0, 24, 0, 0, 0, 1.0, 0, 0, "", "inv_misc_cape_18.png"},
        {404, "Archivist Cape", Slot::BACK, ItemQuality::RARE, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, "", "inv_misc_cape_18.png"},

        // --- CHEST ---
        {501, "Bloodvine Vest", Slot::CHEST, ItemQuality::RARE, 4, 0, 13, 0, 27, 0, 0, 2.0, 0, 0, 0, "Bloodvine", "inv_chest_cloth_21.png"},
        {502, "Robe of the Void", Slot::CHEST, ItemQuality::EPIC, 1, 14, 0, 0, 0, 46, 0, 0, 0, 0, 0, "", "inv_misc_cape_14.png"},
        {503, "Doomcaller's Robes", Slot::CHEST, ItemQuality::EPIC, 5, 31, 20, 0, 41, 0, 0, 1.0, 0, 0, 0, "Doomcaller", "inv_chest_cloth_16.png"},
        {504, "Plagueheart Robe", Slot::CHEST, ItemQuality::EPIC, 6, 27, 24, 0, 48, 0, 0, 1.0, 1.0, 0, 0, "Plagueheart", "inv_chest_cloth_18.png"},

        // --- WRISTS ---
        {601, "Bracers of Arcane Accuracy", Slot::WRISTS, ItemQuality::EPIC, 3, 9, 12, 0, 21, 0, 0, 1.0, 0, 0, 0, "", "inv_bracer_07.png"},
        {602, "Rockfury Bracers", Slot::WRISTS, ItemQuality::EPIC, 5, 0, 0, 0, 27, 0, 0, 1.0, 0, 0, 0, "", "inv_bracer_09.png"},
        {603, "Plagueheart Bindings", Slot::WRISTS, ItemQuality::EPIC, 6, 17, 16, 0, 27, 0, 0, 1.0, 0, 0, 0, "Plagueheart", "inv_bracer_07.png"},
        {604, "Sublime Wristguards", Slot::WRISTS, ItemQuality::RARE, 1, 0, 10, 6, 12, 0, 0, 0, 0, 0, 0, "", "inv_bracer_07.png"},

        // --- HANDS ---
        {701, "Ebony Flame Gloves", Slot::HANDS, ItemQuality::EPIC, 3, 17, 0, 0, 0, 43, 0, 0, 0, 0, 0, "", "inv_gauntlets_19.png"},
        {702, "Dark Storm Gauntlets", Slot::HANDS, ItemQuality::EPIC, 5, 19, 15, 0, 37, 0, 0, 1.0, 0, 0, 0, "", "inv_gauntlets_17.png"},
        {703, "Plagueheart Gloves", Slot::HANDS, ItemQuality::EPIC, 6, 22, 17, 0, 29, 0, 0, 1.0, 1.0, 0, 0, "Plagueheart", "inv_gauntlets_14.png"},
        {704, "Felcloth Gloves", Slot::HANDS, ItemQuality::RARE, 1, 0, 0, 0, 0, 33, 0, 0, 0, 0, 0, "", "inv_gauntlets_19.png"},

        // --- WAIST ---
        {801, "Nemesis Belt", Slot::WAIST, ItemQuality::EPIC, 3, 23, 16, 0, 25, 0, 0, 0, 0, 0, 0, "Nemesis", "inv_belt_12.png"},
        {802, "Eyestalk Waist Cord", Slot::WAIST, ItemQuality::EPIC, 5, 9, 9, 0, 41, 0, 0, 0, 1.0, 0, 0, "", "inv_belt_22.png"},
        {803, "Plagueheart Belt", Slot::WAIST, ItemQuality::EPIC, 6, 22, 18, 0, 37, 0, 0, 0, 1.0, 0, 0, "Plagueheart", "inv_belt_12.png"},
        {804, "Ban'thok Sash", Slot::WAIST, ItemQuality::RARE, 1, 10, 11, 0, 12, 0, 0, 1.0, 0, 0, 0, "", "inv_belt_03.png"},

        // --- LEGS ---
        {901, "Bloodvine Leggings", Slot::LEGS, ItemQuality::RARE, 4, 0, 6, 0, 19, 0, 0, 1.0, 0, 0, 0, "Bloodvine", "inv_pants_cloth_05.png"},
        {902, "Doomcaller's Trousers", Slot::LEGS, ItemQuality::EPIC, 5, 23, 24, 0, 39, 0, 0, 1.0, 1.0, 0, 0, "Doomcaller", "inv_pants_cloth_14.png"},
        {903, "Plagueheart Leggings", Slot::LEGS, ItemQuality::EPIC, 6, 26, 24, 0, 37, 0, 0, 1.0, 1.0, 0, 0, "Plagueheart", "inv_pants_cloth_08.png"},
        {904, "Felheart Pants", Slot::LEGS, ItemQuality::EPIC, 1, 20, 20, 0, 20, 0, 0, 0, 0, 0, 0, "Felheart", "inv_pants_cloth_05.png"},

        // --- FEET ---
        {1001, "Bloodvine Boots", Slot::FEET, ItemQuality::RARE, 4, 0, 16, 0, 19, 0, 0, 1.0, 0, 0, 0, "Bloodvine", "inv_boots_05.png"},
        {1002, "Doomcaller's Footwraps", Slot::FEET, ItemQuality::EPIC, 5, 23, 17, 0, 28, 0, 0, 0, 1.0, 0, 0, "Doomcaller", "inv_boots_cloth_01.png"},
        {1003, "Plagueheart Sandals", Slot::FEET, ItemQuality::EPIC, 6, 18, 17, 0, 32, 0, 0, 1.0, 0, 0, 0, "Plagueheart", "inv_boots_05.png"},
        {1004, "Maleki's Footwraps", Slot::FEET, ItemQuality::RARE, 1, 0, 9, 9, 0, 27, 0, 0, 0, 0, 0, "", "inv_boots_05.png"},

        // --- RINGS ---
        {1101, "Band of Forced Concentration", Slot::RING1, ItemQuality::EPIC, 3, 0, 0, 0, 21, 0, 0, 1.0, 0, 0, 0, "", "inv_jewelry_ring_37.png"},
        {1102, "Ring of the Shadow Flame", Slot::RING1, ItemQuality::EPIC, 3, 0, 0, 0, 0, 44, 0, 0, 0, 0, 0, "", "inv_jewelry_ring_34.png"},
        {1103, "Ring of the Fallen God", Slot::RING1, ItemQuality::EPIC, 5, 6, 5, 0, 37, 0, 0, 1.0, 0, 0, 0, "", "inv_jewelry_ring_40.png"},
        {1104, "Band of the Inevitable", Slot::RING1, ItemQuality::EPIC, 6, 0, 0, 0, 36, 0, 0, 1.0, 0, 0, 0, "", "inv_jewelry_ring_37.png"},
        {1105, "Underworld Band", Slot::RING1, ItemQuality::RARE, 1, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, "", "inv_jewelry_ring_34.png"},

        // --- TRINKETS ---
        {1201, "Talisman of Ephemeral Power", Slot::TRINKET1, ItemQuality::EPIC, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", "inv_jewelry_talisman_07.png", true, 175, 15, 90},
        {1202, "Neltharion's Tear", Slot::TRINKET1, ItemQuality::EPIC, 3, 0, 0, 0, 44, 0, 0, 2.0, 0, 0, 0, "", "inv_misc_gem_pearl_04.png"},
        {1203, "Briarwood Reed", Slot::TRINKET1, ItemQuality::RARE, 1, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, "", "inv_misc_herb_01.png"},
        {1204, "Mark of the Champion", Slot::TRINKET1, ItemQuality::EPIC, 6, 0, 0, 0, 85, 0, 0, 0, 0, 0, 0, "", "inv_trinket_naxxramas04.png"},
        {1205, "The Restless Gaze", Slot::TRINKET1, ItemQuality::EPIC, 6, 0, 0, 0, 0, 0, 0, 0, 2.0, 0, 0, "", "inv_jewelry_talisman_14.png"},

        // --- MAIN HAND ---
        {1301, "Azuresong Mageblade", Slot::MAIN_HAND, ItemQuality::EPIC, 1, 0, 0, 0, 40, 0, 0, 0, 1.0, 0, 0, "", "inv_sword_39.png"},
        {1302, "Claw of Chromaggus", Slot::MAIN_HAND, ItemQuality::EPIC, 3, 7, 17, 0, 64, 0, 0, 0, 0, 0, 0, "", "inv_weapon_shortblade_21.png"},
        {1303, "Sharpened Silithid Femur", Slot::MAIN_HAND, ItemQuality::EPIC, 5, 10, 14, 0, 72, 0, 0, 0, 1.0, 0, 0, "", "inv_weapon_shortblade_25.png"},
        {1304, "Midnight Haze", Slot::MAIN_HAND, ItemQuality::EPIC, 6, 12, 12, 0, 85, 0, 0, 0, 1.0, 0, 0, "", "inv_weapon_shortblade_21.png"},
        {1305, "Witchblade", Slot::MAIN_HAND, ItemQuality::RARE, 1, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, "", "inv_weapon_shortblade_21.png"},

        // --- OFF HAND ---
        {1401, "Jin'do's Bag of Whammies", Slot::OFF_HAND, ItemQuality::EPIC, 4, 11, 8, 0, 18, 0, 0, 1.0, 0, 0, 0, "", "inv_misc_bag_10.png"},
        {1402, "Royal Scepter of Vek'lor", Slot::OFF_HAND, ItemQuality::EPIC, 5, 10, 9, 0, 20, 0, 0, 1.0, 0, 0, 0, "", "inv_wand_07.png"},
        {1403, "The Castigator", Slot::OFF_HAND, ItemQuality::EPIC, 6, 0, 12, 0, 41, 0, 0, 0, 0, 0, 0, "", "inv_mace_14.png"},
        {1404, "Tome of Shadow Force", Slot::OFF_HAND, ItemQuality::RARE, 1, 0, 0, 0, 0, 34, 0, 0, 0, 0, 0, "", "inv_misc_bag_10.png"},

        // --- TWO HAND / STAFF ---
        {1501, "Staff of the Shadow Flame", Slot::TWO_HAND, ItemQuality::EPIC, 3, 24, 29, 0, 84, 0, 0, 0, 2.0, 0, 0, "", "inv_staff_30.png"},
        {1502, "Soulseeker", Slot::TWO_HAND, ItemQuality::EPIC, 6, 30, 31, 0, 126, 0, 0, 0, 2.0, 0, 0, "", "inv_staff_33.png"},

        // --- RANGED / WAND ---
        {1601, "Skul's Ghastly Touch", Slot::RANGED, ItemQuality::RARE, 1, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, "", "inv_wand_06.png"},
        {1602, "Dragon's Touch", Slot::RANGED, ItemQuality::EPIC, 3, 7, 12, 0, 19, 0, 0, 0, 0, 0, 0, "", "inv_wand_01.png"},
        {1603, "Wand of Fates", Slot::RANGED, ItemQuality::EPIC, 6, 8, 7, 0, 23, 0, 0, 1.0, 0, 0, 0, "", "inv_wand_11.png"}
    };
    return db;
}

const Item* ItemDatabase::find_by_name(const std::string& name) {
    for (const auto& item : all_items()) {
        if (item.name == name) return &item;
    }
    return nullptr;
}

const Item* ItemDatabase::find_by_id(int id) {
    for (const auto& item : all_items()) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

std::vector<Item> ItemDatabase::get_items_for_slot(Slot slot) {
    std::vector<Item> result;
    for (const auto& item : all_items()) {
        if (slot == Slot::RING1 || slot == Slot::RING2) {
            if (item.slot == Slot::RING1 || item.slot == Slot::RING2) result.push_back(item);
        } else if (slot == Slot::TRINKET1 || slot == Slot::TRINKET2) {
            if (item.slot == Slot::TRINKET1 || item.slot == Slot::TRINKET2) result.push_back(item);
        } else if (item.slot == slot) {
            result.push_back(item);
        }
    }
    return result;
}

GearLoadout GearLoadout::create_preraid_bis() {
    GearLoadout g;
    g.name = "Pre-Raid BiS";
    if (auto item = ItemDatabase::find_by_id(105)) g.equip(Slot::HEAD, *item);
    if (auto item = ItemDatabase::find_by_id(204)) g.equip(Slot::NECK, *item);
    if (auto item = ItemDatabase::find_by_id(304)) g.equip(Slot::SHOULDERS, *item);
    if (auto item = ItemDatabase::find_by_id(404)) g.equip(Slot::BACK, *item);
    if (auto item = ItemDatabase::find_by_id(502)) g.equip(Slot::CHEST, *item);
    if (auto item = ItemDatabase::find_by_id(604)) g.equip(Slot::WRISTS, *item);
    if (auto item = ItemDatabase::find_by_id(704)) g.equip(Slot::HANDS, *item);
    if (auto item = ItemDatabase::find_by_id(804)) g.equip(Slot::WAIST, *item);
    if (auto item = ItemDatabase::find_by_id(904)) g.equip(Slot::LEGS, *item);
    if (auto item = ItemDatabase::find_by_id(1004)) g.equip(Slot::FEET, *item);
    if (auto item = ItemDatabase::find_by_id(1105)) g.equip(Slot::RING1, *item);
    if (auto item = ItemDatabase::find_by_id(1105)) g.equip(Slot::RING2, *item);
    if (auto item = ItemDatabase::find_by_id(1201)) g.equip(Slot::TRINKET1, *item);
    if (auto item = ItemDatabase::find_by_id(1203)) g.equip(Slot::TRINKET2, *item);
    if (auto item = ItemDatabase::find_by_id(1305)) g.equip(Slot::MAIN_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1404)) g.equip(Slot::OFF_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1601)) g.equip(Slot::RANGED, *item);
    return g;
}

GearLoadout GearLoadout::create_phase3_bis() {
    GearLoadout g;
    g.name = "Phase 3/4 BiS (BWL / Bloodvine)";
    if (auto item = ItemDatabase::find_by_id(101)) g.equip(Slot::HEAD, *item);
    if (auto item = ItemDatabase::find_by_id(201)) g.equip(Slot::NECK, *item);
    if (auto item = ItemDatabase::find_by_id(301)) g.equip(Slot::SHOULDERS, *item);
    if (auto item = ItemDatabase::find_by_id(401)) g.equip(Slot::BACK, *item);
    if (auto item = ItemDatabase::find_by_id(501)) g.equip(Slot::CHEST, *item); // Bloodvine
    if (auto item = ItemDatabase::find_by_id(601)) g.equip(Slot::WRISTS, *item);
    if (auto item = ItemDatabase::find_by_id(701)) g.equip(Slot::HANDS, *item);
    if (auto item = ItemDatabase::find_by_id(801)) g.equip(Slot::WAIST, *item);
    if (auto item = ItemDatabase::find_by_id(901)) g.equip(Slot::LEGS, *item);  // Bloodvine
    if (auto item = ItemDatabase::find_by_id(1001)) g.equip(Slot::FEET, *item); // Bloodvine
    if (auto item = ItemDatabase::find_by_id(1101)) g.equip(Slot::RING1, *item);
    if (auto item = ItemDatabase::find_by_id(1102)) g.equip(Slot::RING2, *item);
    if (auto item = ItemDatabase::find_by_id(1202)) g.equip(Slot::TRINKET1, *item); // Nelth Tear
    if (auto item = ItemDatabase::find_by_id(1201)) g.equip(Slot::TRINKET2, *item); // TOEP
    if (auto item = ItemDatabase::find_by_id(1302)) g.equip(Slot::MAIN_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1401)) g.equip(Slot::OFF_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1602)) g.equip(Slot::RANGED, *item);
    return g;
}

GearLoadout GearLoadout::create_phase5_bis() {
    GearLoadout g;
    g.name = "Phase 5 BiS (AQ40)";
    if (auto item = ItemDatabase::find_by_id(102)) g.equip(Slot::HEAD, *item);
    if (auto item = ItemDatabase::find_by_id(202)) g.equip(Slot::NECK, *item);
    if (auto item = ItemDatabase::find_by_id(302)) g.equip(Slot::SHOULDERS, *item);
    if (auto item = ItemDatabase::find_by_id(402)) g.equip(Slot::BACK, *item);
    if (auto item = ItemDatabase::find_by_id(503)) g.equip(Slot::CHEST, *item);
    if (auto item = ItemDatabase::find_by_id(602)) g.equip(Slot::WRISTS, *item);
    if (auto item = ItemDatabase::find_by_id(702)) g.equip(Slot::HANDS, *item);
    if (auto item = ItemDatabase::find_by_id(802)) g.equip(Slot::WAIST, *item);
    if (auto item = ItemDatabase::find_by_id(902)) g.equip(Slot::LEGS, *item);
    if (auto item = ItemDatabase::find_by_id(1002)) g.equip(Slot::FEET, *item);
    if (auto item = ItemDatabase::find_by_id(1103)) g.equip(Slot::RING1, *item);
    if (auto item = ItemDatabase::find_by_id(1102)) g.equip(Slot::RING2, *item);
    if (auto item = ItemDatabase::find_by_id(1202)) g.equip(Slot::TRINKET1, *item);
    if (auto item = ItemDatabase::find_by_id(1201)) g.equip(Slot::TRINKET2, *item);
    if (auto item = ItemDatabase::find_by_id(1303)) g.equip(Slot::MAIN_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1402)) g.equip(Slot::OFF_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1602)) g.equip(Slot::RANGED, *item);
    return g;
}

GearLoadout GearLoadout::create_phase6_bis() {
    GearLoadout g;
    g.name = "Phase 6 BiS (Naxxramas)";
    if (auto item = ItemDatabase::find_by_id(103)) g.equip(Slot::HEAD, *item);
    if (auto item = ItemDatabase::find_by_id(203)) g.equip(Slot::NECK, *item);
    if (auto item = ItemDatabase::find_by_id(303)) g.equip(Slot::SHOULDERS, *item);
    if (auto item = ItemDatabase::find_by_id(403)) g.equip(Slot::BACK, *item);
    if (auto item = ItemDatabase::find_by_id(504)) g.equip(Slot::CHEST, *item);
    if (auto item = ItemDatabase::find_by_id(603)) g.equip(Slot::WRISTS, *item);
    if (auto item = ItemDatabase::find_by_id(703)) g.equip(Slot::HANDS, *item);
    if (auto item = ItemDatabase::find_by_id(803)) g.equip(Slot::WAIST, *item);
    if (auto item = ItemDatabase::find_by_id(903)) g.equip(Slot::LEGS, *item);
    if (auto item = ItemDatabase::find_by_id(1003)) g.equip(Slot::FEET, *item);
    if (auto item = ItemDatabase::find_by_id(1104)) g.equip(Slot::RING1, *item);
    if (auto item = ItemDatabase::find_by_id(1103)) g.equip(Slot::RING2, *item);
    if (auto item = ItemDatabase::find_by_id(1204)) g.equip(Slot::TRINKET1, *item);
    if (auto item = ItemDatabase::find_by_id(1202)) g.equip(Slot::TRINKET2, *item);
    if (auto item = ItemDatabase::find_by_id(1304)) g.equip(Slot::MAIN_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1403)) g.equip(Slot::OFF_HAND, *item);
    if (auto item = ItemDatabase::find_by_id(1603)) g.equip(Slot::RANGED, *item);
    return g;
}

} // namespace warlock
