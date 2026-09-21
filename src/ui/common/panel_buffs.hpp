#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "src/sim/buffs.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/priest/priest_sim.hpp"

namespace warlock {

inline void render_panel_buffs(BuffConfig& buffs) {
    // 1. Consumables (Default Collapsed)
    if (ImGui::CollapsingHeader("Consumables & Elixirs", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Flask of Supreme Power (+150 Spell Power)", &buffs.flask_of_supreme_power);
        ImGui::Checkbox("Flask of Distilled Wisdom (+2000 Mana)", &buffs.flask_of_distilled_wisdom);
        ImGui::Checkbox("Flask of the Titans (+1200 Health)", &buffs.flask_of_the_titans);
        ImGui::Separator();
        ImGui::Checkbox("Greater Arcane Elixir (+35 Spell Power)", &buffs.greater_arcane_elixir);
        ImGui::Checkbox("Elixir of Shadow Power (+40 Shadow Power)", &buffs.elixir_of_shadow_power);
        ImGui::Checkbox("Elixir of Greater Firepower (+40 Fire Power)", &buffs.elixir_of_greater_firepower);
        ImGui::Checkbox("Elixir of the Owl (+25 Int, +2% Spell Crit)", &buffs.elixir_of_the_owl);
        ImGui::Checkbox("Elixir of the Sages (+18 Int, +18 Spirit)", &buffs.elixir_of_the_sages);
        ImGui::Checkbox("Greater Mageblood Elixir (+20 MP5)", &buffs.greater_mageblood_elixir);
        ImGui::Checkbox("Mageblood Elixir (+12 MP5)", &buffs.mageblood_elixir);
        ImGui::Separator();
        ImGui::Checkbox("Brilliant Wizard Oil (+36 Spell Power, +1% Crit)", &buffs.brilliant_wizard_oil);
        ImGui::Checkbox("Use Major Mana Potions (~1800 Mana)", &buffs.use_mana_potions);
        ImGui::Checkbox("Use Demonic / Dark Runes (~1200 Mana)", &buffs.use_demonic_runes);
        ImGui::Unindent(8.0f);
    }

    // 2. Raid Buffs (Default Collapsed)
    if (ImGui::CollapsingHeader("Raid Buffs", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Arcane Intellect (+31 Intellect)", &buffs.arcane_intellect);
        ImGui::Checkbox("Blessing of Kings (+10% All Attributes)", &buffs.blessing_of_kings);
        ImGui::Checkbox("Blessing of Wisdom (+30 MP5)", &buffs.blessing_of_wisdom);
        ImGui::Checkbox("Mark of the Wild (+12 All Attributes)", &buffs.mark_of_the_wild);
        ImGui::Checkbox("Judgement of Wisdom (50% Chance for 59 Mana on Hit)", &buffs.judgement_of_wisdom);
        ImGui::Unindent(8.0f);
    }

    // 3. World Buffs (Default Collapsed, Off by Default)
    if (ImGui::CollapsingHeader("World Buffs (Default Off)", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Rallying Cry of the Dragonslayer (+10% Spell Crit)", &buffs.rallying_cry);
        ImGui::Checkbox("Songflower Serenade (+5% Spell Crit, +15 All Stats)", &buffs.songflower);
        ImGui::Checkbox("Spirit of Zandalar (+10% All Attributes)", &buffs.spirit_of_zandalar);
        ImGui::Checkbox("Warchief's Blessing (+300 HP, +10 MP5)", &buffs.warchiefs_blessing);
        ImGui::Checkbox("Sayge's Fortune (DMF +10% Damage Dealt)", &buffs.sayges_fortune);
        ImGui::Unindent(8.0f);
    }

    // 4. Target Raid Debuffs (Default Collapsed)
    if (ImGui::CollapsingHeader("Target Raid Debuffs", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Curse of Shadows (-75 Res, +10% Shadow/Arcane Dmg)", &buffs.curse_of_shadows);
        ImGui::Checkbox("Curse of the Elements (-75 Res, +10% Fire/Frost Dmg)", &buffs.curse_of_elements);
        ImGui::Checkbox("Shadow Weaving 5 Stacks (+15% Shadow Dmg)", &buffs.shadow_weaving);
        ImGui::Checkbox("Nightfall 2H Axe Proc (+15% Spell Damage Taken)", &buffs.nightfall_axe);
        ImGui::Unindent(8.0f);
    }
}

inline void render_panel_active_pet(WarlockSimulator& sim) {
    // Active Demon Pet (collapsible one-of checkboxes)
    if (ImGui::CollapsingHeader("Active Demon Pet", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        auto pet_option = [&](PetChoice pet) {
            bool selected = (sim.policy.pet == pet);
            const char* pet_icon = pet_choice_to_icon(pet);
            if (pet_icon[0] != '\0') {
                Texture2D icon = AssetManager::get().get_icon(pet_icon);
                ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(18, 18));
                ImGui::SameLine();
            }
            if (ImGui::Checkbox(pet_choice_to_string(pet), &selected)) {
                if (selected) {
                    sim.policy.pet = pet;
                } else {
                    sim.policy.pet = PetChoice::NONE;
                }
            }
        };
        pet_option(PetChoice::NONE);
        pet_option(PetChoice::SUCCUBUS);
        pet_option(PetChoice::IMP);
        ImGui::Unindent(8.0f);
    }
}

inline void render_panel_buffs(WarlockSimulator& sim) {
    BuffConfig& buffs = sim.buffs;
    render_panel_buffs(buffs);

    // Demonic Sacrifice Modifiers (Default Collapsed)
    if (ImGui::CollapsingHeader("Demonic Sacrifice Modifiers", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Sacrifice Imp (+15% Shadow Damage in Forever)", &buffs.sacrifice_imp);
        ImGui::Checkbox("Sacrifice Succubus (+15% Fire Damage in Forever)", &buffs.sacrifice_succubus);
        ImGui::Unindent(8.0f);
    }
}

inline void render_panel_buffs(priest::PriestSimulator& sim) {
    sim::BuffConfig& buffs = sim.buffs;
    render_panel_buffs(buffs);

    // 5. Priest Self-Buffs & Shields (Collapsing Header)
    if (ImGui::CollapsingHeader("Priest Self-Buffs & Protective Magic", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Inner Fire (+1395 Armor, 20 charges)", &buffs.inner_fire);
        ImGui::Checkbox("Power Word: Fortitude (+70 Stamina)", &buffs.power_word_fortitude);
        ImGui::Checkbox("Divine Spirit (+40 Spirit)", &buffs.divine_spirit);
        ImGui::Checkbox("Shadow Protection (+60 Shadow Resistance)", &buffs.shadow_protection);
        ImGui::Unindent(8.0f);
    }

    // 6. Priest Stances, Forms & Power (Collapsing Header)
    if (ImGui::CollapsingHeader("Priest Stances & Spell Forms", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Shadowform (+15% Shadow Dmg, -50% Mana cost, 2.0x Crit)", &sim.mechanics.shadowform_enabled);
        ImGui::Checkbox("Maintain Vampiric Embrace (20% Shadow Dmg heals party)", &sim.policy.cast_vampiric_embrace);
        ImGui::Checkbox("Use Power Infusion on Cooldown (+20% Spell Dmg & Haste)", &sim.policy.use_power_infusion);
        ImGui::Unindent(8.0f);
    }
}

} // namespace warlock
