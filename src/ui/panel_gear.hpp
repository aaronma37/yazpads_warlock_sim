#pragma once
#include "imgui.h"
#include "rlImGui.h"
#include "asset_manager.hpp"
#include "src/sim/gear.hpp"
#include "src/sim/stats.hpp"
#include "src/sim/warlock_sim.hpp"
#include <string>

namespace warlock {

inline ImVec4 get_item_color(ItemQuality q) {
    switch (q) {
        case ItemQuality::EPIC: return ImVec4(0.64f, 0.21f, 0.93f, 1.0f); // Epic purple
        case ItemQuality::RARE: return ImVec4(0.00f, 0.44f, 0.87f, 1.0f); // Rare blue
        case ItemQuality::UNCOMMON: return ImVec4(0.12f, 1.00f, 0.00f, 1.0f);
        default: return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    }
}

// Renders an authentic WoW inventory slot button
inline void render_armory_slot(
    Slot slot,
    GearLoadout& gear,
    Slot& selecting_slot,
    bool& show_item_picker,
    char* search_filter,
    float slot_size = 46.0f
) {
    const Item& item = gear.get(slot);
    bool has_item = !item.name.empty();

    std::string icon_file = has_item ? item.icon : get_default_slot_icon(slot);
    const Texture2D& tex = AssetManager::get().get_icon(icon_file);

    ImVec4 border_col = has_item ? get_item_color(item.quality) : ImVec4(0.28f, 0.25f, 0.35f, 1.0f);

    ImGui::PushID(static_cast<int>(slot));
    ImGui::PushStyleColor(ImGuiCol_Border, border_col);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, has_item ? 2.0f : 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));

    if (rlImGuiImageButtonSize(slot_to_name(slot), &tex, Vector2{ slot_size, slot_size })) {
        selecting_slot = slot;
        show_item_picker = true;
        search_filter[0] = '\0';
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // Authentic WoW Tooltip
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (has_item) {
            ImGui::TextColored(get_item_color(item.quality), "%s", item.name.c_str());
            ImGui::TextDisabled("Phase %d %s", item.phase, slot_to_name(slot));
            ImGui::Separator();

            if (item.stamina > 0.0 || item.intellect > 0.0 || item.spirit > 0.0) {
                std::string attrs = "";
                if (item.stamina > 0.0) attrs += "+" + std::to_string((int)item.stamina) + " Stamina  ";
                if (item.intellect > 0.0) attrs += "+" + std::to_string((int)item.intellect) + " Intellect  ";
                if (item.spirit > 0.0) attrs += "+" + std::to_string((int)item.spirit) + " Spirit";
                ImGui::Text("%s", attrs.c_str());
            }

            if (item.spell_power > 0.0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Equip: Increases damage and healing done by magical spells by up to %.0f.", item.spell_power);
            }
            if (item.shadow_power > 0.0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Equip: Increases damage done by Shadow spells and effects by up to %.0f.", item.shadow_power);
            }
            if (item.fire_power > 0.0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Equip: Increases damage done by Fire spells and effects by up to %.0f.", item.fire_power);
            }
            if (item.spell_hit > 0.0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Equip: Improves your chance to hit with spells by %.0f%%.", item.spell_hit);
            }
            if (item.spell_crit > 0.0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Equip: Improves your chance to get a critical strike with spells by %.0f%%.", item.spell_crit);
            }
            if (item.has_on_use) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Use: Increases magical damage by up to %.0f for %.0fs. (%.0fs Cooldown)",
                    item.on_use_spell_power, item.on_use_duration, item.on_use_cooldown);
            }
            if (!item.set_name.empty()) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Set: %s", item.set_name.c_str());
            }
        } else {
            ImGui::TextDisabled("Empty %s Slot", slot_to_name(slot));
            ImGui::Text("Click to equip an item from the database.");
        }
        ImGui::EndTooltip();
    }

    ImGui::PopID();
}

inline void render_armory_panel(
    WarlockSimulator& sim,
    const Stats& total_stats,
    const BaseAttributes& base_attrs,
    std::string& character_name,
    int& selected_model_idx
) {
    GearLoadout& gear = sim.gear;
    static Slot selecting_slot = Slot::HEAD;
    static bool show_item_picker = false;
    static char search_filter[64] = "";

    const float slot_sz = 46.0f;
    const float model_w = 240.0f;
    const float model_h = 426.0f;

    // --- TOP LEVEL DUAL MODE SWITCHER ---
    float avail_w = ImGui::GetContentRegionAvail().x;
    float mode_btn_w = (avail_w - 6.0f) * 0.5f;

    bool in_gear_mode = !sim.use_raw_stats;
    if (in_gear_mode) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.20f, 0.68f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52f, 0.26f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.85f, 0.20f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.12f, 0.18f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.20f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.28f, 0.45f, 0.6f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }
    if (ImGui::Button("⚔ EQUIPPED ITEMS", ImVec2(mode_btn_w, 32))) {
        sim.use_raw_stats = false;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    bool in_raw_mode = sim.use_raw_stats;
    if (in_raw_mode) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.58f, 0.36f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 1.0f, 0.60f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.12f, 0.18f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.20f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.28f, 0.45f, 0.6f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }
    if (ImGui::Button("📊 DIRECT STAT VALUES", ImVec2(mode_btn_w, 32))) {
        sim.use_raw_stats = true;
        if (sim.raw_stats.spell_power == 0.0 && sim.raw_stats.shadow_power == 0.0) {
            sim.raw_stats = sim.gear.calculate_stats();
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Sub-mode status description
    if (!sim.use_raw_stats) {
        ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 0.9f), "Mode: 16 inventory items, set bonuses & enchants");
    } else {
        ImGui::TextColored(ImVec4(0.40f, 1.0f, 0.60f, 0.9f), "Mode: Direct theoretical numbers (gear overridden)");
    }

    // Character Identity & Race Selection Banner
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.10f, 0.16f, 0.90f));
    ImGui::BeginChild("ArmoryHeader", ImVec2(0, 105), true);

    // Row 1: Name and Level/Faction
    static char name_buf[32] = "Grimmortis";
    ImGui::SetNextItemWidth(140);
    if (ImGui::InputText("##CharName", name_buf, sizeof(name_buf))) {
        character_name = name_buf;
    }
    ImGui::SameLine();
    bool is_alliance = (sim.race == Race::HUMAN || sim.race == Race::GNOME);
    const char* f_icon = is_alliance ? "👑 Alliance" : "💀 Horde";
    ImVec4 f_col = is_alliance ? ImVec4(0.35f, 0.65f, 1.0f, 1.0f) : ImVec4(1.0f, 0.40f, 0.40f, 1.0f);
    ImGui::TextColored(f_col, "%s - Level 60 Warlock", f_icon);

    // Row 2: All 5 playable races grouped by faction
    ImGui::Text("Race:");
    ImGui::SameLine();

    auto race_btn = [&](const char* label, Race r) {
        bool selected = (sim.race == r);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.25f, 0.75f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.85f, 0.20f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.2f);
        }
        if (ImGui::SmallButton(label)) {
            sim.race = r;
            selected_model_idx = static_cast<int>(r);
            sim.base_attrs = get_base_attributes_for_race(r);
        }
        if (selected) {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }
    };

    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "Horde:");
    ImGui::SameLine();
    race_btn("Undead", Race::UNDEAD);
    ImGui::SameLine();
    race_btn("Orc", Race::ORC);
    ImGui::SameLine();
    race_btn("Troll", Race::TROLL);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "| Alliance:");
    ImGui::SameLine();
    race_btn("Human", Race::HUMAN);
    ImGui::SameLine();
    race_btn("Gnome", Race::GNOME);

    // Row 3: Active Racial Passives
    ImGui::Spacing();
    switch (sim.race) {
        case Race::HUMAN:
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "⚔ Sword Spec (+2%% Crit w/ Swords) | 🕊 Spirit (+5%%) | Will to Survive");
            break;
        case Race::GNOME:
            ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "🧠 Expansive Mind (+5%% Mana) | 💡 Eureka! (+10%% Dmg 3 casts) | Escape Artist");
            break;
        case Race::ORC:
            ImGui::TextColored(ImVec4(1.0f, 0.50f, 0.30f, 1.0f), "🩸 Blood Fury (+10%% SP for 15s) | 🐾 Command (+5%% Pet Dmg) | 🪓 Axe Spec (+1%% Crit)");
            break;
        case Race::UNDEAD:
            ImGui::TextColored(ImVec4(0.70f, 0.90f, 0.60f, 1.0f), "⚰ Touch of the Grave (Health Drain Proc) | 🛡 Will of the Forsaken | Cannibalize");
            break;
        case Race::TROLL:
            ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.80f, 1.0f), "🌀 Berserking (+10%% Haste for 10s) | 🐺 Beast Slaying (+5%% vs Beasts) | Regen");
            break;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    if (!sim.use_raw_stats) {
        // Presets row for equipped gear
        ImGui::Text("Loadout Presets:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Pre-Raid")) gear = GearLoadout::create_preraid_bis();
        ImGui::SameLine();
        if (ImGui::SmallButton("Phase 3/4")) gear = GearLoadout::create_phase3_bis();
        ImGui::SameLine();
        if (ImGui::SmallButton("Phase 5")) gear = GearLoadout::create_phase5_bis();
        ImGui::SameLine();
        if (ImGui::SmallButton("Phase 6")) gear = GearLoadout::create_phase6_bis();

        // The WoW Paperdoll Frame
        ImGui::BeginChild("PaperdollFrame", ImVec2(0, model_h + 80), true);

        // Layout: Left Column (8 slots) | Center Character Model | Right Column (8 slots)
        float start_x = ImGui::GetCursorPosX() + 15.0f;
        float start_y = ImGui::GetCursorPosY() + 5.0f;

        // --- LEFT COLUMN (Head down to Wrists) ---
        Slot left_slots[8] = {
            Slot::HEAD, Slot::NECK, Slot::SHOULDERS, Slot::BACK,
            Slot::CHEST, Slot::WRISTS, Slot::WRISTS, Slot::WRISTS
        };
        // Standard left slots in WoW: Head, Neck, Shoulders, Back, Chest, Shirt, Tabard, Wrist
        for (int i = 0; i < 6; ++i) {
            ImGui::SetCursorPos(ImVec2(start_x, start_y + i * (slot_sz + 6.0f)));
            Slot s;
            switch (i) {
                case 0: s = Slot::HEAD; break;
                case 1: s = Slot::NECK; break;
                case 2: s = Slot::SHOULDERS; break;
                case 3: s = Slot::BACK; break;
                case 4: s = Slot::CHEST; break;
                default: s = Slot::WRISTS; break;
            }
            render_armory_slot(s, gear, selecting_slot, show_item_picker, search_filter, slot_sz);
        }

        // Decorative Shirt and Tabard icons
        {
            ImGui::SetCursorPos(ImVec2(start_x, start_y + 6 * (slot_sz + 6.0f)));
            const Texture2D& shirt_tex = AssetManager::get().get_icon("INV_Chest_Cloth_17.png");
            rlImGuiImageButtonSize("Shirt", &shirt_tex, Vector2{ slot_sz, slot_sz });
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shirt Slot");

            ImGui::SetCursorPos(ImVec2(start_x, start_y + 7 * (slot_sz + 6.0f)));
            const Texture2D& tabard_tex = AssetManager::get().get_icon("INV_Misc_Cape_18.png");
            rlImGuiImageButtonSize("Tabard", &tabard_tex, Vector2{ slot_sz, slot_sz });
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tabard Slot");
        }

        // --- CENTER: CHARACTER 3D MODEL RENDER ---
        float model_x = start_x + slot_sz + 18.0f;
        ImGui::SetCursorPos(ImVec2(model_x, start_y));

        std::string model_file = "undead_warlock.png";
        switch (sim.race) {
            case Race::UNDEAD: model_file = "undead_warlock.png"; break;
            case Race::ORC:    model_file = "orc_warlock.png"; break;
            case Race::TROLL:  model_file = "troll_warlock.png"; break;
            case Race::HUMAN:  model_file = "human_warlock.png"; break;
            case Race::GNOME:  model_file = "gnome_warlock.png"; break;
        }
        const Texture2D& model_tex = AssetManager::get().get_model(model_file);

        // Frame behind character model
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + model_w, p_min.y + model_h);
        draw_list->AddRectFilled(p_min, p_max, IM_COL32(14, 12, 20, 255), 4.0f);
        draw_list->AddRect(p_min, p_max, IM_COL32(90, 70, 120, 180), 4.0f, 0, 1.5f);

        rlImGuiImageSize(&model_tex, static_cast<int>(model_w), static_cast<int>(model_h));

        // --- RIGHT COLUMN (Hands down to Trinket 2) ---
        float right_x = model_x + model_w + 18.0f;
        Slot right_slots[8] = {
            Slot::HANDS, Slot::WAIST, Slot::LEGS, Slot::FEET,
            Slot::RING1, Slot::RING2, Slot::TRINKET1, Slot::TRINKET2
        };
        for (int i = 0; i < 8; ++i) {
            ImGui::SetCursorPos(ImVec2(right_x, start_y + i * (slot_sz + 6.0f)));
            render_armory_slot(right_slots[i], gear, selecting_slot, show_item_picker, search_filter, slot_sz);
        }

        // --- BOTTOM ROW: WEAPONS & WAND ---
        float bottom_y = start_y + model_h + 12.0f;
        float weapons_center = model_x + (model_w - (3 * slot_sz + 2 * 12.0f)) * 0.5f;

        ImGui::SetCursorPos(ImVec2(weapons_center, bottom_y));
        render_armory_slot(Slot::MAIN_HAND, gear, selecting_slot, show_item_picker, search_filter, slot_sz);

        ImGui::SetCursorPos(ImVec2(weapons_center + slot_sz + 12.0f, bottom_y));
        render_armory_slot(Slot::OFF_HAND, gear, selecting_slot, show_item_picker, search_filter, slot_sz);

        ImGui::SetCursorPos(ImVec2(weapons_center + 2 * (slot_sz + 12.0f), bottom_y));
        render_armory_slot(Slot::RANGED, gear, selecting_slot, show_item_picker, search_filter, slot_sz);

        ImGui::EndChild();
    } else {
        // The Raw Stats Frame (Direct Numerical Input Mode)
        ImGui::BeginChild("RawStatsFrame", ImVec2(0, model_h + 80), true);
        ImGui::TextColored(ImVec4(0.40f, 1.0f, 0.60f, 1.0f), "DIRECT NUMERIC STAT SPECIFICATION (THEORYCRAFTING)");
        ImGui::TextDisabled("Directly input numeric stats to analyze scaling, stat weights & EP.");

        // Presets row
        ImGui::Spacing();
        ImGui::Text("Stat Presets:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Pre-Raid")) {
            sim.raw_stats = Stats();
            sim.raw_stats.spell_power = 320.0;
            sim.raw_stats.spell_hit_percent = 3.0;
            sim.raw_stats.spell_crit_percent = 4.0;
            sim.raw_stats.intellect = 110.0;
            sim.raw_stats.stamina = 130.0;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Phase 3/4")) {
            sim.raw_stats = Stats();
            sim.raw_stats.spell_power = 520.0;
            sim.raw_stats.spell_hit_percent = 8.0;
            sim.raw_stats.spell_crit_percent = 10.0;
            sim.raw_stats.intellect = 140.0;
            sim.raw_stats.stamina = 160.0;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Phase 5")) {
            sim.raw_stats = Stats();
            sim.raw_stats.spell_power = 640.0;
            sim.raw_stats.spell_hit_percent = 12.0;
            sim.raw_stats.spell_crit_percent = 14.0;
            sim.raw_stats.spell_haste_percent = 4.0;
            sim.raw_stats.intellect = 160.0;
            sim.raw_stats.stamina = 180.0;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Phase 6 BiS")) {
            sim.raw_stats = Stats();
            sim.raw_stats.spell_power = 780.0;
            sim.raw_stats.spell_hit_percent = 16.0;
            sim.raw_stats.spell_crit_percent = 18.0;
            sim.raw_stats.spell_haste_percent = 8.0;
            sim.raw_stats.intellect = 180.0;
            sim.raw_stats.stamina = 200.0;
        }

        if (ImGui::Button("📥 Import From Equipped Gear", ImVec2(200, 24))) {
            sim.raw_stats = sim.gear.calculate_stats();
        }
        ImGui::SameLine();
        if (ImGui::Button("🔄 Reset to 0", ImVec2(100, 24))) {
            sim.raw_stats = Stats();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // 1. Spell Damage
        ImGui::TextColored(ImVec4(0.85f, 0.55f, 1.0f, 1.0f), "1. SPELL DAMAGE (SPELL POWER):");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Generic Spell Power##Raw", &sim.raw_stats.spell_power, 10.0, 50.0, "%.0f SP");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Shadow Spell Power##Raw", &sim.raw_stats.shadow_power, 10.0, 50.0, "%.0f Shadow");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Fire Spell Power##Raw", &sim.raw_stats.fire_power, 10.0, 50.0, "%.0f Fire");

        ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f),
            "  -> Effective Shadow: %.0f SP | Effective Fire: %.0f SP",
            sim.raw_stats.effective_shadow_power(), sim.raw_stats.effective_fire_power());

        ImGui::Spacing();
        ImGui::Separator();

        // 2. Secondary Ratings
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.30f, 1.0f), "2. SECONDARY COMBAT RATINGS:");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Spell Hit Chance (%)##Raw", &sim.raw_stats.spell_hit_percent, 0.5, 1.0, "%.1f%% Hit");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Spell Crit Chance (%)##Raw", &sim.raw_stats.spell_crit_percent, 0.5, 1.0, "%.2f%% Crit");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Spell Haste (%)##Raw", &sim.raw_stats.spell_haste_percent, 1.0, 2.0, "%.1f%% Haste");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Mana Regen (MP5)##Raw", &sim.raw_stats.mp5, 5.0, 10.0, "%.0f MP5");

        ImGui::Spacing();
        ImGui::Separator();

        // 3. Base Attributes
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "3. BONUS ATTRIBUTES (GEAR / ENCHANTS):");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Bonus Intellect##Raw", &sim.raw_stats.intellect, 10.0, 25.0, "%.0f Int");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Bonus Stamina##Raw", &sim.raw_stats.stamina, 10.0, 25.0, "%.0f Stam");
        ImGui::SetNextItemWidth(130);
        ImGui::InputDouble("Bonus Spirit##Raw", &sim.raw_stats.spirit, 10.0, 25.0, "%.0f Spirit");

        ImGui::EndChild();
    }

    // Character Attributes & Spell Stats Sheet (Authentic WoW Stats Card)
    ImGui::BeginChild("ArmoryStatsSheet", ImVec2(0, 0), true);

    if (ImGui::CollapsingHeader("Attributes & Health/Mana", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(3, "AttrCols", false);
        ImGui::Text("Health: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "%.0f", total_stats.max_health);

        ImGui::Text("Mana: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "%.0f", total_stats.max_mana);

        ImGui::NextColumn();
        ImGui::Text("Stamina: %.0f", total_stats.stamina);
        ImGui::Text("Intellect: %.0f", total_stats.intellect);

        ImGui::NextColumn();
        ImGui::Text("Spirit: %.0f", total_stats.spirit);
        ImGui::Text("Armor: 1,480");
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("Spell Combat Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "SpellCols", false);

        ImGui::Text("Shadow Spell Power:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "+%.0f", total_stats.effective_shadow_power());

        ImGui::Text("Fire Spell Power:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "+%.0f", total_stats.effective_fire_power());

        ImGui::Text("Generic Spell Power:");
        ImGui::SameLine();
        ImGui::Text("+%.0f", total_stats.spell_power);

        ImGui::NextColumn();

        ImGui::Text("Spell Hit Chance:");
        ImGui::SameLine();
        double hit_vs_boss = 83.0 + total_stats.spell_hit_percent;
        if (hit_vs_boss > 99.0) hit_vs_boss = 99.0;
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "+%.1f%% (%.0f%% vs Boss)", total_stats.spell_hit_percent, hit_vs_boss);

        ImGui::Text("Spell Crit Chance:");
        ImGui::SameLine();
        double total_crit = total_stats.total_spell_crit(base_attrs.base_spell_crit);
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "%.2f%%", total_crit);

        ImGui::Text("Mana Regen (MP5):");
        ImGui::SameLine();
        ImGui::Text("+%.0f / 5s", total_stats.mp5);

        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("Resistances", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(5, "ResCols", false);
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.9f, 1.0f), "Arcane: 0");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f), "Fire: 15");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Nature: 0");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Frost: 0");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.3f, 1.0f, 1.0f), "Shadow: 20");
        ImGui::Columns(1);
    }

    ImGui::EndChild();

    // Modal Item Picker Popup
    if (show_item_picker) {
        ImGui::OpenPopup("Select Armory Item");
    }

    if (ImGui::BeginPopupModal("Select Armory Item", &show_item_picker, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select Item for Slot: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", slot_to_name(selecting_slot));

        ImGui::InputText("Search Filter", search_filter, 64);
        ImGui::Separator();

        auto candidates = ItemDatabase::get_items_for_slot(selecting_slot);

        ImGui::BeginChild("ArmoryPickerList", ImVec2(520, 320), true);

        // Unequip item option
        if (ImGui::Selectable("(Unequip Item / Empty Slot)")) {
            gear.equip(selecting_slot, Item());
            show_item_picker = false;
        }
        ImGui::Separator();

        for (const auto& item : candidates) {
            if (search_filter[0] != '\0') {
                std::string item_lower = item.name;
                std::string filter_lower = search_filter;
                for (char& c : item_lower) c = (char)tolower(c);
                for (char& c : filter_lower) c = (char)tolower(c);
                if (item_lower.find(filter_lower) == std::string::npos) continue;
            }

            ImGui::PushID(item.id);

            // Icon thumbnail
            const Texture2D& icon_tex = AssetManager::get().get_icon(item.icon);
            rlImGuiImageSize(&icon_tex, 28, 28);
            ImGui::SameLine();

            ImVec4 col = get_item_color(item.quality);
            ImGui::PushStyleColor(ImGuiCol_Text, col);

            char buf[128];
            snprintf(buf, sizeof(buf), "[P%d] %s", item.phase, item.name.c_str());
            if (ImGui::Selectable(buf, false, 0, ImVec2(340, 28))) {
                gear.equip(selecting_slot, item);
                gear.name = "Custom Loadout";
                show_item_picker = false;
            }
            ImGui::PopStyleColor();

            ImGui::SameLine(390);
            ImGui::TextDisabled("+%.0f SP, +%.0f Hit", item.spell_power + item.shadow_power, item.spell_hit);

            ImGui::PopID();
        }

        ImGui::EndChild();

        if (ImGui::Button("Close", ImVec2(100, 0))) {
            show_item_picker = false;
        }
        ImGui::EndPopup();
    }
}

} // namespace warlock
