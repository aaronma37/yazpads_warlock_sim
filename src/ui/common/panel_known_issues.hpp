#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"

namespace warlock {

inline void render_panel_known_issues() {
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Known Issues, Backlog & Roadmap");
    ImGui::TextDisabled("Backlog items, known unmodeled procs, downranking scope, and planned simulator enhancements.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Gear Chance-on-Hit / Proc Effects Backlog
    if (WowCollapsingHeader("1. Gear Chance-on-Hit / Proc Special Effects (Backlog)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Chance-on-Damage / Hit Procs: Items such as Blade of Eternal Darkness, Darkmoon Card: Crusade, Darkmoon Card: Wrath, and Eye of the Diminisher are currently treated with their static spell power/stats.");
        ImGui::BulletText("On-Use Trinkets & Set Bonuses: Currently active on-use trinkets (Tear of Neltharion, Restrained Essence of Sapphiron, ToEP, ZHC) and Tier set bonuses (T1, T2, T2.5, T3) are fully modeled with cooldown timers and spell power bursts.");
        ImGui::BulletText("Engine Roadmap Goal: Implement generalized proc event triggers (PPM and %% chance on damage roll) in the discrete-event queue for weapons and trinkets with internal cooldowns.");
        ImGui::Spacing();
    }

    // 2. Spell Downranking Scope
    if (WowCollapsingHeader("2. Spell Downranking & Base Spell Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Max Rank Default: All simulated spell casts utilize maximum rank spells (e.g. Shadow Bolt Rank 10, Corruption Rank 7, Immolate Rank 8, Shadowburn Rank 6, Life Tap Rank 6).");
        ImGui::BulletText("Downranking Penalty: In Classic WoW, spells learned below level 20 suffer a coefficient penalty ((RankLevel + 11) / 60). Downranking is not currently exposed in the priority list because max rank + Life Tap yields higher DPS.");
        ImGui::Spacing();
    }

    // 3. Multi-Target / Encounter Scope
    if (WowCollapsingHeader("3. Encounter Mechanics & Target Scope", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Single-Target Boss Focus: The engine simulates single-target discrete-event boss fights (default Level 63 raid boss, 17%% spell miss cap, partial resistance calculations).");
        ImGui::BulletText("AoE & Multi-Target Spells: Multi-DoT target swapping and AoE spells (Hellfire, Rain of Fire, Seed of Corruption) are not part of the single-target encounter loop.");
        ImGui::BulletText("Boss Armor & Resistance: Resistances default to 0 with Curse of Shadows/Elements (-75 resistance reduces boss base 24 resistance to 0, eliminating binary resists).");
        ImGui::Spacing();
    }

    // 4. Pet Survival Simulation
    if (WowCollapsingHeader("4. Pet Survival & Encounter Uptime", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Pet Uptime: When an active pet is selected (Imp / Succubus), the pet is assumed to attack uninterrupted unless sacrificed via Demonic Sacrifice.");
        ImGui::BulletText("Boss Cleave / Pet Damage: Pet health pool and boss environmental/cleave damage are not modeled in DPS runs.");
        ImGui::Spacing();
    }
}

} // namespace warlock
