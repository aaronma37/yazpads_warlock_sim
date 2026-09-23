#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "wow_widgets.hpp"
#include "src/sim/buffs.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/priest/priest_sim.hpp"

namespace warlock {

inline void render_panel_buffs(BuffConfig& buffs) {
    // 1. Consumables (Default Collapsed)
    if (WowCollapsingHeader("Consumables & Elixirs", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        WowCheckbox("Flask of Supreme Power (+150 Spell Power)", &buffs.flask_of_supreme_power);
        WowCheckbox("Flask of Distilled Wisdom (+2000 Mana)", &buffs.flask_of_distilled_wisdom);
        WowCheckbox("Flask of the Titans (+1200 Health)", &buffs.flask_of_the_titans);
        ImGui::Separator();
        WowCheckbox("Greater Arcane Elixir (+35 Spell Power)", &buffs.greater_arcane_elixir);
        WowCheckbox("Elixir of Shadow Power (+40 Shadow Power)", &buffs.elixir_of_shadow_power);
        WowCheckbox("Elixir of Greater Firepower (+40 Fire Power)", &buffs.elixir_of_greater_firepower);
        WowCheckbox("Elixir of the Owl (+25 Int, +2% Spell Crit)", &buffs.elixir_of_the_owl);
        WowCheckbox("Elixir of the Sages (+18 Int, +18 Spirit)", &buffs.elixir_of_the_sages);
        WowCheckbox("Greater Mageblood Elixir (+20 MP5)", &buffs.greater_mageblood_elixir);
        WowCheckbox("Mageblood Elixir (+12 MP5)", &buffs.mageblood_elixir);
        ImGui::Separator();
        WowCheckbox("Brilliant Wizard Oil (+36 Spell Power, +1% Crit)", &buffs.brilliant_wizard_oil);
        WowCheckbox("Use Major Mana Potions (~1800 Mana)", &buffs.use_mana_potions);
        WowCheckbox("Use Demonic / Dark Runes (~1200 Mana)", &buffs.use_demonic_runes);
        ImGui::Unindent(8.0f);
    }

    // 2. Raid Buffs (Default Collapsed)
    if (WowCollapsingHeader("Raid Buffs", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        WowCheckbox("Arcane Intellect (+31 Intellect)", &buffs.arcane_intellect);
        WowCheckbox("Blessing of Kings (+10% All Attributes)", &buffs.blessing_of_kings);
        WowCheckbox("Blessing of Wisdom (+30 MP5)", &buffs.blessing_of_wisdom);
        WowCheckbox("Mark of the Wild (+12 All Attributes)", &buffs.mark_of_the_wild);
        WowCheckbox("Judgement of Wisdom (50% Chance for 59 Mana on Hit)", &buffs.judgement_of_wisdom);
        ImGui::Unindent(8.0f);
    }

    // 3. World Buffs (Default Collapsed, Off by Default)
    if (WowCollapsingHeader("World Buffs (Default Off)", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        WowCheckbox("Rallying Cry of the Dragonslayer (+10% Spell Crit)", &buffs.rallying_cry);
        WowCheckbox("Songflower Serenade (+5% Spell Crit, +15 All Stats)", &buffs.songflower);
        WowCheckbox("Spirit of Zandalar (+10% All Attributes)", &buffs.spirit_of_zandalar);
        WowCheckbox("Warchief's Blessing (+300 HP, +10 MP5)", &buffs.warchiefs_blessing);
        WowCheckbox("Sayge's Fortune (DMF +10% Damage Dealt)", &buffs.sayges_fortune);
        ImGui::Unindent(8.0f);
    }

    // 4. Target Raid Debuffs (Default Collapsed)
    if (WowCollapsingHeader("Target Raid Debuffs", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        WowCheckbox("Curse of Shadows (-75 Res, +10% Shadow/Arcane Dmg)", &buffs.curse_of_shadows);
        WowCheckbox("Curse of the Elements (-75 Res, +10% Fire/Frost Dmg)", &buffs.curse_of_elements);
        WowCheckbox("Shadow Weaving 5 Stacks (+15% Shadow Dmg)", &buffs.shadow_weaving);
        WowCheckbox("Nightfall 2H Axe Proc (+15% Spell Damage Taken)", &buffs.nightfall_axe);
        ImGui::Unindent(8.0f);
    }
}

inline void render_panel_target_config(TargetConfig& target) {
    // 5. Target Configuration (Default Collapsed)
    if (WowCollapsingHeader("Target Configuration", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);

        auto render_field_int = [](const char* label, const char* id, int* val, int min_v, int max_v, float input_w = 80.0f) {
            ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "%s", label);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.5f));
            ImGui::SetNextItemWidth(input_w);
            WowInputInt(id, val, 1, 5);
            if (*val < min_v) *val = min_v;
            if (*val > max_v) *val = max_v;
            ImGui::PopStyleVar();
        };

        auto render_field_double = [](const char* label, const char* id, double* val, const char* fmt = "%.0f", float input_w = 80.0f) {
            ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "%s", label);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.5f));
            ImGui::SetNextItemWidth(input_w);
            WowInputDouble(id, val, 0.0, 0.0, fmt);
            ImGui::PopStyleVar();
        };

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 2.0f));
        if (ImGui::BeginTable("TargetConfigTable", 2, ImGuiTableFlags_None)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            render_field_int("Target Count", "##TargetCount", &target.target_count, 1, 5);
            ImGui::TableNextColumn();
            render_field_int("Target Level", "##TargetLevel", &target.level, 55, 65);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            render_field_double("Shadow Res", "##ShadowRes", &target.base_shadow_resistance, "%.0f");
            ImGui::TableNextColumn();
            render_field_double("Fire Res", "##FireRes", &target.base_fire_resistance, "%.0f");

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "Creature Type");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.5f));
        ImGui::SetNextItemWidth(170.0f);
        const char* creature_types[] = {"Humanoid", "Beast", "Demon", "Undead", "Dragonkin", "Elemental", "Giant", "Mechanical", "Other"};
        int current_type_idx = static_cast<int>(target.creature_type);
        if (current_type_idx < 0 || current_type_idx >= IM_ARRAYSIZE(creature_types))
            current_type_idx = 0;
        if (ImGui::Combo("##CreatureTypeCombo", &current_type_idx, creature_types, IM_ARRAYSIZE(creature_types))) {
            target.creature_type = static_cast<CreatureType>(current_type_idx);
            target.is_beast = (target.creature_type == CreatureType::BEAST);
        }
        ImGui::PopStyleVar();

        if (target.target_count > 1) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f),
                               "Multi-Target Active (%d targets)", target.target_count);
        }
        if (target.creature_type == CreatureType::BEAST || target.is_beast) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.80f, 1.0f),
                               "Beast Target (+5%% Troll racial)");
        }

        ImGui::Unindent(8.0f);
    }
}

inline void render_panel_buffs(BuffConfig& buffs, TargetConfig& target) {
    render_panel_buffs(buffs);
    render_panel_target_config(target);
}

inline void render_panel_buffs(WarlockSimulator& sim) {
    BuffConfig& buffs = sim.buffs;
    render_panel_buffs(buffs);
    render_panel_target_config(sim.target_config);
}

inline void render_panel_buffs(priest::PriestSimulator& sim) {
    sim::BuffConfig& buffs = sim.buffs;
    render_panel_buffs(buffs);
    render_panel_target_config(sim.target_config);

    // 5. Priest Self-Buffs & Shields (Collapsing Header)
    if (WowCollapsingHeader("Priest Self-Buffs & Protective Magic", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        WowCheckbox("Inner Fire (+1395 Armor, 20 charges)", &buffs.inner_fire);
        WowCheckbox("Power Word: Fortitude (+70 Stamina)", &buffs.power_word_fortitude);
        WowCheckbox("Divine Spirit (+40 Spirit)", &buffs.divine_spirit);
        WowCheckbox("Shadow Protection (+60 Shadow Resistance)", &buffs.shadow_protection);
        ImGui::Unindent(8.0f);
    }

    // 6. Priest Stances, Forms & Power (Collapsing Header)
    if (WowCollapsingHeader("Priest Stances & Spell Forms", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        WowCheckbox("Shadowform (+15% Shadow Dmg, -50% Mana cost, 2.0x Crit)", &sim.mechanics.shadowform_enabled);
        WowCheckbox("Maintain Vampiric Embrace (20% Shadow Dmg heals party)", &sim.policy.cast_vampiric_embrace);
        WowCheckbox("Use Power Infusion on Cooldown (+20% Spell Dmg & Haste)", &sim.policy.use_power_infusion);
        ImGui::Unindent(8.0f);
    }
}

} // namespace warlock
