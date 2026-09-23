#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"

namespace warlock {

inline void render_panel_mechanics_tab() {
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Game Mechanics & Vanilla Differences");
    ImGui::TextDisabled("Explicit documentation of deliberate design modifications, custom server rules, and engine differences from vanilla Classic WoW.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Improved Shadow Bolt (ISB) Charges Removal
    if (WowCollapsingHeader("1. Improved Shadow Bolt (ISB) - Aura vs Charges", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Deliberate Server Modification: ISB functions as a permanent aura/debuff window for 12 seconds upon landing a critical strike.");
        ImGui::BulletText("No Charge Consumption: Unlike vanilla Classic WoW where 4 non-periodic shadow damage hits consume the debuff, in this ruleset all shadow damage during the 12-second window receives the full +20%% shadow damage multiplier without consuming charges.");
        ImGui::BulletText("Impact: Eliminates ISB charge eating by other raid members (e.g. Priests, multiple Warlocks), creating near-constant +20%% shadow damage uptime on critical strikes.");
        ImGui::Spacing();
    }

    // 2. DoT Critical Strikes & Pandemic Scaling
    if (WowCollapsingHeader("2. DoT Critical Strikes & Pandemic Scaling", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("DoT Crits Enabled: Periodic spells (Corruption, Curse of Agony, Immolate DoT, Wrack, Siphon Life) can critically strike based on your spell crit chance.");
        ImGui::BulletText("Pandemic Talent: Affliction talent provides up to +100%% critical strike damage bonus to all periodic effects.");
        ImGui::BulletText("Ruin Interaction: In Destruction, Ruin (+100%% crit damage bonus) applies to direct hits as well as Immolate DoT ticks.");
        ImGui::BulletText("Vanilla Contrast: In vanilla Classic WoW, DoT ticks could never critically strike.");
        ImGui::Spacing();
    }

    // 3. Personal-Only Shadow Weaving
    if (WowCollapsingHeader("3. Shadow Weaving - Personal Only", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Deliberate Server Modification: Shadow Weaving (Priest talent) only increases the shadow damage done by the Priest who applied the debuff.");
        ImGui::BulletText("Warlock Impact: Warlocks no longer receive the +15%% shadow damage multiplier from raid Shadow Priests by default (toggleable in Mechanics settings).");
        ImGui::Spacing();
    }

    // 4. Demonic Sacrifice & Pet Resummoning (DP)
    if (WowCollapsingHeader("4. Demonic Sacrifice & Demonic Pact (DP)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Imp Demonic Sacrifice: In this ruleset, sacrificing an Imp grants +15%% Shadow Damage (rather than Fire Resistance in vanilla Classic).");
        ImGui::BulletText("Succubus Demonic Sacrifice: Grants +15%% Fire Damage.");
        ImGui::BulletText("Demonic Pact (DP) Capstone: Allows summoning a different active demon (such as Succubus for +10%% Shadow damage via Master Demonologist) without cancelling the Demonic Sacrifice buff.");
        ImGui::Spacing();
    }

    // 5. Raid Debuff Slot Limits & Snapshotting
    if (WowCollapsingHeader("5. Debuff Slots & Dynamic Scaling", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Infinite Debuff Slots (Default): Engine defaults to unlimited debuff slots, allowing full multi-DoT rotations without pushing off other raid debuffs.");
        ImGui::BulletText("Dynamic Spell Power Scaling: DoTs dynamically query active player spell power on each tick rather than snapshotting on initial application (classic snapshotting toggleable).");
        ImGui::Spacing();
    }

    // 6. Pet Stat Scaling & Mana Management
    if (WowCollapsingHeader("6. Pet Stat Scaling & Mana Management", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Imp Firebolt (Modern / Default): Deals 44 base Fire damage + 15%% pet SP inheritance (2.0 / 3.5 coefficient) with a 2.0s cast time.");
        ImGui::BulletText("Imp Firebolt (Classic 1.12): Toggleable in Mechanics to 85-98 base damage + 15%% pet SP inheritance (1.5/3.5 coefficient) and 1.5s cast time.");
        ImGui::BulletText("Pet SP Inheritance: Summoned demons inherit a percentage of the master's Spell Power (default 15%% for demon spells like Succubus Lash of Pain, or Classic Firebolt).");
        ImGui::BulletText("Pet AP Inheritance: Demons inherit master's Spell Power converted into Attack Power (default 57%% for melee auto-attacks like Succubus melee).");
        ImGui::BulletText("Customizable Ratios: Pet scaling toggle and custom SP / AP scaling percentages can be adjusted under Game Mechanics in the Presets & Setup panel.");
        ImGui::BulletText("Pet Mana Pools: Imp (1,150 mana) and Succubus (1,450 mana) consume mana on casts, benefiting from MP5 and raid mana restoration buffs (Blessing / Judgement of Wisdom).");
        ImGui::Spacing();
    }
}

} // namespace warlock
