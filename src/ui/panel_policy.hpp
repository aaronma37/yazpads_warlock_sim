#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "src/sim/policy.hpp"
#include "src/sim/warlock_sim.hpp"
#include <vector>
#include <algorithm>

namespace warlock {

inline void render_panel_policy_controls(PolicyConfig& policy) {
    ImGui::TextColored(ImVec4(0.8f, 0.7f, 1.0f, 1.0f), "Combat Policy & Spell Priorities:");
    ImGui::Separator();

    // 1. Curse Priority
    ImGui::Text("Curse Assignment:");
    int curse_idx = static_cast<int>(policy.curse);
    const char* curse_names[] = { "None", "Curse of Shadows (+10% Shadow Dmg)", "Curse of the Elements (+10% Fire Dmg)", "Curse of Agony (Solo DPS)", "Curse of Doom (60s Burst)" };
    if (ImGui::Combo("##CurseCombo", &curse_idx, curse_names, IM_ARRAYSIZE(curse_names))) {
        policy.curse = static_cast<CurseChoice>(curse_idx);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Corruption Maintenance
    ImGui::Text("Corruption Usage:");
    bool use_corr = (policy.corruption == DotPolicy::ALWAYS);
    if (ImGui::Checkbox("Always Maintain Corruption DoT", &use_corr)) {
        policy.corruption = use_corr ? DotPolicy::ALWAYS : DotPolicy::NEVER;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("In SM/Ruin, Corruption triggers Nightfall procs for instant Shadow Bolts.\nIn DS/Ruin, depends on raid debuff slots.");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Immolate Maintenance
    ImGui::Text("Immolate Usage:");
    ImGui::Checkbox("Maintain Immolate (Fire Destro / Firelock)", &policy.maintain_immolate);

    ImGui::Spacing();
    ImGui::Separator();

    // 4. Shadowburn
    ImGui::Text("Shadowburn Usage:");
    int sb_idx = static_cast<int>(policy.shadowburn);
    const char* sb_names[] = { "Never (Conserve Shards / Mana)", "On Cooldown (Every 8s)", "Execute Phase Only (Last 20% HP)" };
    if (ImGui::Combo("##ShadowburnCombo", &sb_idx, sb_names, IM_ARRAYSIZE(sb_names))) {
        policy.shadowburn = static_cast<ShadowburnPolicy>(sb_idx);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 5. Life Tap Threshold
    ImGui::Text("Life Tap Mana Management:");
    ImGui::SliderFloat("Life Tap Threshold (%% Mana)", (float*)&policy.life_tap_threshold_pct, 5.0f, 60.0f, "%.0f%%");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When current mana drops below this percentage, player prioritizes casting Life Tap.");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 6. Procs and Trinkets
    ImGui::Text("Procs & Trinket Timing:");
    ImGui::Checkbox("Cast Instant Shadow Bolt on Nightfall Proc", &policy.cast_nightfall_procs);
    ImGui::Checkbox("Use On-Use Trinkets on Cooldown", &policy.use_trinkets_on_cooldown);
}

inline void render_panel_policy(PolicyConfig& policy) {
    render_panel_policy_controls(policy);
}

inline void render_panel_policy(WarlockSimulator& sim) {
    render_panel_policy_controls(sim.policy);

    // 7. Live Observed Spell Cast Sequence Preview
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Live Observed Spell Cast Sequence (120s Fight):");

    // Run quick deterministic single sample simulation
    WarlockSimulator s = sim;
    s.record_timeline = true;
    FastRNG rng(0x13374242ULL);
    SimResult sample = s.run_single_simulation(rng);
    const auto& seq = sample.cast_sequence;

    if (!seq.empty()) {
        int opener_count = (int)std::min(seq.size(), (size_t)16);
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Sequence (First %d Spells):", opener_count);
        ImGui::BeginChild("PolicyOpenerBox", ImVec2(-1, 56), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (size_t i = 0; i < std::min(seq.size(), (size_t)24); ++i) {
            const auto& cast = seq[i];
            if (i > 0) {
                ImGui::SameLine();
                ImGui::TextDisabled("->");
                ImGui::SameLine();
            }
            ImGui::BeginGroup();
            Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(cast.spell_id));
            ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", spell_id_to_name(cast.spell_id));
                ImGui::Text("Time: %.1fs  |  Cast Duration: %.1fs", cast.time, cast.cast_time);
                ImGui::Text("Role: %s", cast.tag.c_str());
                if (cast.damage > 0.0) {
                    ImGui::TextColored(cast.is_crit ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                                       "Damage: %.0f %s", cast.damage, cast.is_crit ? "(CRIT!)" : "");
                } else if (cast.is_miss) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Result: MISS / RESIST");
                }
                ImGui::EndTooltip();
            }
            ImGui::TextDisabled("%.1fs", cast.time);
            ImGui::EndGroup();
        }
        ImGui::EndChild();

        // Cast Order & Frequency Table
        struct SpellStat {
            SpellID id;
            int count = 0;
            double first_cast = -1.0;
            std::string role;
        };
        std::vector<SpellStat> stats;
        int total_observed_casts = (int)seq.size();
        for (const auto& cast : seq) {
            auto it = std::find_if(stats.begin(), stats.end(), [&](const SpellStat& st){ return st.id == cast.spell_id; });
            if (it == stats.end()) {
                SpellStat st;
                st.id = cast.spell_id;
                st.count = 1;
                st.first_cast = cast.time;
                st.role = cast.tag;
                stats.push_back(st);
            } else {
                it->count++;
            }
        }
        std::sort(stats.begin(), stats.end(), [](const SpellStat& a, const SpellStat& b){
            return a.first_cast < b.first_cast;
        });

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observed Cast Priority (Live Fight Sample):");
        if (ImGui::BeginTable("PolicySpellsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 18);
            ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("First Cast", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Casts (Share)", ImGuiTableColumnFlags_WidthFixed, 85);
            ImGui::TableSetupColumn("Combat Role & Behavior", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            int rank = 1;
            for (const auto& st : stats) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextDisabled("%d", rank++);

                ImGui::TableNextColumn();
                Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(st.id));
                ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                ImGui::SameLine();
                ImGui::Text("%s", spell_id_to_name(st.id));

                ImGui::TableNextColumn();
                ImGui::Text("%.1fs", st.first_cast);

                ImGui::TableNextColumn();
                double share = total_observed_casts > 0 ? (100.0 * st.count / total_observed_casts) : 0.0;
                ImGui::Text("%dx (%.1f%%)", st.count, share);

                ImGui::TableNextColumn();
                std::string role_desc;
                if (st.id == SpellID::CURSE_OF_SHADOWS || st.id == SpellID::CURSE_OF_ELEMENTS || st.id == SpellID::CURSE_OF_DOOM) {
                    role_desc = "Opener Debuff (Maintained on boss)";
                } else if (st.id == SpellID::CORRUPTION) {
                    role_desc = "DoT (Maintained every 18s; Nightfall)";
                } else if (st.id == SpellID::IMMOLATE) {
                    role_desc = "DoT (Maintained every 15s; buffs Destro)";
                } else if (st.id == SpellID::SOUL_FIRE) {
                    role_desc = "Decimation Execute (Spammed <35% HP)";
                } else if (st.id == SpellID::SHADOWBURN) {
                    role_desc = "Instant Burst (Cast on 8s cooldown)";
                } else if (st.id == SpellID::CONFLAGRATE) {
                    role_desc = "Instant Burst (Cast on 10s cooldown)";
                } else if (st.id == SpellID::SHADOW_BOLT) {
                    role_desc = "Primary Cast Filler (2.5s cast, Bane)";
                } else if (st.id == SpellID::INCINERATE) {
                    role_desc = "Primary Cast Filler (2.0s cast, Bane)";
                } else if (st.id == SpellID::DRAIN_HOPE) {
                    role_desc = "Channeled Execute (6.0s channel)";
                } else if (st.id == SpellID::LIFE_TAP) {
                    role_desc = "Resource Tap (Cast when mana drops low)";
                } else {
                    role_desc = st.role;
                }
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", role_desc.c_str());
            }
            ImGui::EndTable();
        }
    }
}

} // namespace warlock
