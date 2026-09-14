#pragma once
#include "imgui.h"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/parallel_runner.hpp"

namespace warlock {

inline void render_panel_sim_control(
    WarlockSimulator& sim,
    int& iterations,
    int& thread_count,
    BatchSimResult& last_result,
    bool& is_running,
    float& progress
) {
    // Big prominent RUN button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.15f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.22f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.30f, 1.0f, 1.0f));

    if (is_running) ImGui::BeginDisabled();
    if (ImGui::Button(">>> RUN DES SIMULATION <<<", ImVec2(-1, 38))) {
        is_running = true;
        progress = 0.0f;
        last_result = ParallelSimRunner::run_batch(sim, iterations, thread_count, [&](float p) {
            progress = p;
        });
        is_running = false;
        progress = 1.0f;
    }
    if (is_running) ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    if (is_running) {
        ImGui::ProgressBar(progress, ImVec2(-1, 6));
    }

    if (last_result.total_iterations > 0) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Completed %d sims in %.3fs (%.0f sims/sec)",
            last_result.total_iterations, last_result.total_sim_time_seconds, last_result.iterations_per_second);
    }

    ImGui::Separator();

    // Encounter & Simulation Parameters
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Encounter & Simulation Parameters:");
    ImGui::SliderFloat("Fight Duration (s)", (float*)&sim.fight_duration, 10.0f, 600.0f, "%.0f seconds");
    
    // Target Level & Type
    const char* level_presets[] = { "Level 60 (Equal Lvl)", "Level 61 (+1 Lvl)", "Level 62 (+2 Lvl)", "Level 63 (Raid Boss)" };
    int current_lvl_idx = (sim.target_config.level >= 60 && sim.target_config.level <= 63) ? (sim.target_config.level - 60) : 3;
    ImGui::SetNextItemWidth(180);
    if (ImGui::Combo("Target Level", &current_lvl_idx, level_presets, IM_ARRAYSIZE(level_presets))) {
        sim.target_config.level = 60 + current_lvl_idx;
    }

    const char* creature_types[] = { "Humanoid", "Beast (Troll +5%)", "Demon", "Undead", "Dragonkin", "Elemental", "Giant", "Mechanical", "Other" };
    int current_type_idx = static_cast<int>(sim.target_config.creature_type);
    if (current_type_idx < 0 || current_type_idx >= IM_ARRAYSIZE(creature_types)) current_type_idx = 0;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    if (ImGui::Combo("Target Type", &current_type_idx, creature_types, IM_ARRAYSIZE(creature_types))) {
        sim.target_config.creature_type = static_cast<CreatureType>(current_type_idx);
        sim.target_config.is_beast = (sim.target_config.creature_type == CreatureType::BEAST);
    }

    ImGui::SliderInt("Iterations", &iterations, 1000, 100000, "%d fights");

    int max_threads = static_cast<int>(std::thread::hardware_concurrency());
    if (max_threads <= 0) max_threads = 4;
    ImGui::SliderInt("Worker Threads", &thread_count, 1, max_threads, "%d threads");

    ImGui::Separator();

    // Raid Buffs & Consumables
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Raid Buffs & Consumables:");
    if (ImGui::CollapsingHeader("Raid Buffs", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Arcane Intellect (+31 Int)", &sim.buffs.arcane_intellect);
        ImGui::SameLine(220);
        ImGui::Checkbox("Blessing of Kings (+10% Stats)", &sim.buffs.blessing_of_kings);
        ImGui::Checkbox("Blessing of Wisdom (+30 MP5)", &sim.buffs.blessing_of_wisdom);
        ImGui::SameLine(220);
        ImGui::Checkbox("Mark of the Wild (+12 Stats)", &sim.buffs.mark_of_the_wild);
        ImGui::Checkbox("Judgement of Wisdom (Mana Proc)", &sim.buffs.judgement_of_wisdom);
        ImGui::SameLine(220);
        ImGui::Checkbox("Shadow Weaving (+15% Shadow)", &sim.buffs.shadow_weaving);
    }

    if (ImGui::CollapsingHeader("Consumables", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Flask of Supreme Power (+150 SP)", &sim.buffs.flask_of_supreme_power);
        ImGui::SameLine(240);
        ImGui::Checkbox("Brilliant Wizard Oil (+36 SP, +1% Crit)", &sim.buffs.brilliant_wizard_oil);
        ImGui::Checkbox("Greater Arcane Elixir (+35 SP)", &sim.buffs.greater_arcane_elixir);
        ImGui::SameLine(240);
        ImGui::Checkbox("Elixir of Shadow Power (+40 Shadow)", &sim.buffs.elixir_of_shadow_power);
        ImGui::Checkbox("Major Mana Potions (120s CD)", &sim.buffs.use_mana_potions);
        ImGui::SameLine(240);
        ImGui::Checkbox("Demonic Runes (120s CD)", &sim.buffs.use_demonic_runes);
    }

    if (ImGui::CollapsingHeader("World Buffs", ImGuiTreeNodeFlags_None)) {
        ImGui::Checkbox("Rallying Cry (+10% Spell Crit)", &sim.buffs.rallying_cry);
        ImGui::SameLine(240);
        ImGui::Checkbox("Songflower (+5% Crit, +15 Stats)", &sim.buffs.songflower);
        ImGui::Checkbox("Spirit of Zandalar (+10% Stats)", &sim.buffs.spirit_of_zandalar);
        ImGui::SameLine(240);
        ImGui::Checkbox("Sayge's Fortune (+10% Damage)", &sim.buffs.sayges_fortune);
    }

    if (ImGui::CollapsingHeader("Pet & Sacrifice", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Sacrifice Succubus (+15% Shadow Damage)", &sim.buffs.sacrifice_succubus);
        ImGui::SameLine(260);
        ImGui::Checkbox("Sacrifice Imp (+15% Fire Damage)", &sim.buffs.sacrifice_imp);
    }
}

} // namespace warlock
