#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "panel_policy.hpp"
#include "src/sim/optimizer.hpp"
#include <vector>
#include <string>
#include <algorithm>

namespace warlock {

inline void render_panel_optimizer(
    WarlockSimulator& sim,
    std::vector<CandidateResult>& optimizer_results,
    bool& is_optimizing,
    float& opt_progress,
    std::string& current_opt_target,
    bool* request_switch_to_preset = nullptr
) {
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "⚡ Combinatorial Optimization & Brute-Force Exploration:");
    ImGui::TextWrapped("Spawns hundreds of thousands of parallel DES simulations across all CPU threads to explore, compare, and rank candidate builds.");
    ImGui::Separator();

    static int iters_per_candidate = 3000;
    static bool compare_all_races = false;
    static bool calculate_stat_weights = false;
    ImGui::SetNextItemWidth(200);
    ImGui::SliderInt("Sims Per Candidate", &iters_per_candidate, 1000, 20000, "%d fights");
    ImGui::SameLine(340);
    ImGui::Checkbox("Compare specs across all races (Undead, Orc, Troll, Human, Gnome)", &compare_all_races);
    ImGui::SameLine();
    ImGui::Checkbox("Calculate DPS per stat change (Stat Weights)", &calculate_stat_weights);

    ImGui::Spacing();
    if (is_optimizing) ImGui::BeginDisabled();

    if (ImGui::Button("Explore Standard Specs", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::optimize_talents(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        }, compare_all_races, calculate_stat_weights);
        is_optimizing = false;
        opt_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Combinatorial Talent Search", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::explore_combinatorial_talents(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        }, compare_all_races, calculate_stat_weights);
        is_optimizing = false;
        opt_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Explore Phase Gear Sets", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::optimize_gear(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        });
        is_optimizing = false;
        opt_progress = 1.0f;
    }

    ImGui::Spacing();
    if (ImGui::Button("Explore Consumables", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::compare_consumable_tiers(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        });
        is_optimizing = false;
        opt_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Calculate Stat Weights (EP)", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::compare_stat_values(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        });
        is_optimizing = false;
        opt_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Explore Rotation Policy", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::optimize_policy(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        });
        is_optimizing = false;
        opt_progress = 1.0f;
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.35f, 1.0f));
    if (ImGui::Button("🧬 Perturb & Maximize Active Setup (Talents + Rotation APL)", ImVec2(430, 30))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::perturb_preset(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        });
        is_optimizing = false;
        opt_progress = 1.0f;
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Takes your current active talents, pet, and rotation, and systematically evaluates localized mutations\n(e.g., shifting points between Agonizing Flames and Shadow Mastery, testing DoTs/Banes, pet choices, cooldowns)\nto uncover the highest DPS configuration!");
    }

    if (is_optimizing) ImGui::EndDisabled();

    if (is_optimizing) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Simulating: %s...", current_opt_target.c_str());
        ImGui::ProgressBar(opt_progress, ImVec2(-1, 8));
    }

    ImGui::Separator();

    auto apply_candidate_config = [&](const CandidateResult& r) {
        sim.race = r.race;
        sim.base_attrs = get_base_attributes_for_race(sim.race);
        if (r.category == "Talents" || r.category == "Combinatorial Talents") {
            sim.talents = r.talents;
            sim.buffs = r.buffs;
            sim.policy.pet = r.policy.pet;
            sim.policy.rotation = r.policy.rotation;

        } else if (r.category == "Gear") {
            sim.gear = r.gear;
            sim.use_raw_stats = false;
        } else if (r.category == "Consumables") {
            sim.buffs = r.buffs;
        } else if (r.category == "Stat Values (EP)") {
            sim.use_raw_stats = true;
            sim.raw_stats = r.raw_stats;
        } else if (r.category == "Policy") {
            sim.policy = r.policy;
        } else {
            sim.talents = r.talents;
            sim.gear = r.gear;
            sim.buffs = r.buffs;
            sim.policy = r.policy;
            sim.mechanics = r.mechanics;
            sim.use_raw_stats = r.use_raw_stats;
            sim.raw_stats = r.raw_stats;
        }
        if (request_switch_to_preset) {
            *request_switch_to_preset = true;
        }
    };

    if (!optimizer_results.empty()) {
        const auto& best = optimizer_results[0];
        static int selected_candidate_idx = 0;
        if (selected_candidate_idx >= static_cast<int>(optimizer_results.size())) {
            selected_candidate_idx = 0;
        }

        bool show_stat_weights = false;
        for (const auto& res : optimizer_results) {
            if (res.stat_weights.valid) {
                show_stat_weights = true;
                break;
            }
        }

        int num_cols = show_stat_weights ? 12 : 7;

        // Calculate active base combat stats for the header line
        Stats base_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
        sim.buffs.apply_to_stats(base_stats, sim.base_attrs, true, sim.mechanics.personal_shadow_weaving);
        if (sim.race == Race::HUMAN) {
            bool is_sword = true;
            if (!sim.use_raw_stats) {
                const Item& mh = sim.gear.get(Slot::MAIN_HAND);
                is_sword = (mh.name.find("Mageblade") != std::string::npos ||
                            mh.name.find("Sword") != std::string::npos ||
                            mh.name.find("Blade") != std::string::npos ||
                            mh.icon.find("sword") != std::string::npos ||
                            mh.icon.find("Sword") != std::string::npos);
            }
            if (is_sword) base_stats.spell_crit_percent += 2.0;
        }

        // Spec table header with active base stats matching Build Configuration
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Specs");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Base Stats:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.75f, 0.50f, 1.0f, 1.0f), "Shadow SP: %.0f", base_stats.effective_shadow_power());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "Fire SP: %.0f", base_stats.effective_fire_power());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Hit: %.1f%%", base_stats.spell_hit_percent);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Crit: %.2f%%", base_stats.total_spell_crit(sim.base_attrs.base_spell_crit));
        if (base_stats.spell_haste_percent > 0.0) {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Haste: %.1f%%", base_stats.spell_haste_percent);
        }
        if (base_stats.mp5 > 0.0) {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "MP5: %.0f", base_stats.mp5);
        }

        if (ImGui::BeginTable("OptLeaderboardTable", num_cols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45);
            ImGui::TableSetupColumn("Spec Name", ImGuiTableColumnFlags_WidthFixed, 190);
            ImGui::TableSetupColumn("Race", ImGuiTableColumnFlags_WidthFixed, 65);
            ImGui::TableSetupColumn("Action Priority Chain", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Damage Split (S / F / P)", ImGuiTableColumnFlags_WidthFixed, 150);
            ImGui::TableSetupColumn("Mean DPS", ImGuiTableColumnFlags_WidthFixed, 85);
            ImGui::TableSetupColumn("+/- StdDev", ImGuiTableColumnFlags_WidthFixed, 75);
            if (show_stat_weights) {
                ImGui::TableSetupColumn("DPS/SP", ImGuiTableColumnFlags_WidthFixed, 65);
                ImGui::TableSetupColumn("DPS/Hit", ImGuiTableColumnFlags_WidthFixed, 70);
                ImGui::TableSetupColumn("DPS/Crit", ImGuiTableColumnFlags_WidthFixed, 70);
                ImGui::TableSetupColumn("DPS/Haste", ImGuiTableColumnFlags_WidthFixed, 75);
                ImGui::TableSetupColumn("DPS/Int", ImGuiTableColumnFlags_WidthFixed, 65);
            }
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < optimizer_results.size(); ++i) {
                const auto& r = optimizer_results[i];
                bool is_selected = (selected_candidate_idx == static_cast<int>(i));

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (r.rank == 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1");
                } else {
                    ImGui::Text("#%d", r.rank);
                }

                ImGui::TableNextColumn();
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(r.name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    selected_candidate_idx = static_cast<int>(i);
                }
                ImGui::PopID();

                ImGui::TableNextColumn();
                ImVec4 race_col = (r.race == Race::HUMAN || r.race == Race::GNOME) ? ImVec4(0.4f, 0.75f, 1.0f, 1.0f) : ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
                ImGui::TextColored(race_col, "%s", race_to_string(r.race));

                ImGui::TableNextColumn();
                std::vector<PriorityRule> rules = r.policy.get_priority_rules(r.talents, r.race);
                for (size_t k = 0; k < rules.size(); ++k) {
                    const auto& rule = rules[k];
                    Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(rule.spell_id));
                    ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(18, 18));
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", rule.name.c_str());
                        if (!rule.condition_summary.empty()) {
                            ImGui::TextDisabled("%s", rule.condition_summary.c_str());
                        }
                        if (!rule.trigger_condition.empty()) {
                            ImGui::TextWrapped("%s", rule.trigger_condition.c_str());
                        }
                        ImGui::EndTooltip();
                    }
                    if (k + 1 < rules.size()) {
                        ImGui::SameLine(0, 3);
                    }
                }

                // Damage Breakdown: 3-section horizontal bar (Shadow, Fire, Pet)
                ImGui::TableNextColumn();
                double shadow_pct = r.batch.pct_shadow_bolt + r.batch.pct_corruption + r.batch.pct_curse + r.batch.pct_siphon_life + r.batch.pct_shadowburn + r.batch.pct_drain_hope + r.batch.pct_drain_life + r.batch.pct_drain_soul;
                double fire_pct = r.batch.pct_immolate + r.batch.pct_conflagrate + r.batch.pct_incinerate + r.batch.pct_searing_pain + r.batch.pct_soul_fire;
                double pet_pct = r.batch.pct_pet;

                double total_pct = shadow_pct + fire_pct + pet_pct;
                if (total_pct > 0.0) {
                    shadow_pct = (shadow_pct / total_pct) * 100.0;
                    fire_pct = (fire_pct / total_pct) * 100.0;
                    pet_pct = (pet_pct / total_pct) * 100.0;
                } else {
                    if (r.name.find("Fire") != std::string::npos || r.name.find("Incinerate") != std::string::npos) {
                        shadow_pct = 5.0; fire_pct = 95.0; pet_pct = 0.0;
                    } else if (r.name.find("DP") != std::string::npos || r.name.find("Demo") != std::string::npos || r.name.find("MD") != std::string::npos) {
                        shadow_pct = 70.0; fire_pct = 5.0; pet_pct = 25.0;
                    } else {
                        shadow_pct = 98.0; fire_pct = 2.0; pet_pct = 0.0;
                    }
                }

                float col_w = ImGui::GetContentRegionAvail().x;
                float bar_w = std::max(40.0f, col_w);
                float bar_h = 16.0f;
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + bar_w, p0.y + bar_h);

                std::string bar_btn_id = "##DmgBar_" + std::to_string(i);
                ImGui::InvisibleButton(bar_btn_id.c_str(), ImVec2(bar_w, bar_h));
                bool is_bar_hovered = ImGui::IsItemHovered();

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(p0, p1, IM_COL32(20, 20, 26, 255), 3.0f);

                float cur_bar_x = p0.x;
                float s_w = (float)(bar_w * (shadow_pct * 0.01));
                float f_w = (float)(bar_w * (fire_pct * 0.01));
                float p_w = (float)(bar_w * (pet_pct * 0.01));

                // 1. Shadow segment (Purple)
                if (shadow_pct > 0.5) {
                    ImVec2 b0(cur_bar_x, p0.y);
                    ImVec2 b1(std::min(p1.x, cur_bar_x + s_w), p1.y);
                    draw_list->AddRectFilled(b0, b1, IM_COL32(148, 65, 235, 235), 0.0f);
                    cur_bar_x += s_w;
                }

                // 2. Fire segment (Orange)
                if (fire_pct > 0.5) {
                    ImVec2 b0(cur_bar_x, p0.y);
                    ImVec2 b1(std::min(p1.x, cur_bar_x + f_w), p1.y);
                    draw_list->AddRectFilled(b0, b1, IM_COL32(245, 115, 30, 235), 0.0f);
                    cur_bar_x += f_w;
                }

                // 3. Pet segment (Emerald Green)
                if (pet_pct > 0.5) {
                    ImVec2 b0(cur_bar_x, p0.y);
                    ImVec2 b1(p1.x, p1.y);
                    draw_list->AddRectFilled(b0, b1, IM_COL32(40, 195, 90, 235), 0.0f);
                }

                draw_list->AddRect(p0, p1, is_bar_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(65, 65, 80, 255), 3.0f, 0, 1.0f);

                char shadow_txt[16], fire_txt[16], pet_txt[16];
                std::snprintf(shadow_txt, sizeof(shadow_txt), "%.0f%%", shadow_pct);
                std::snprintf(fire_txt, sizeof(fire_txt), "%.0f%%", fire_pct);
                std::snprintf(pet_txt, sizeof(pet_txt), "%.0f%%", pet_pct);

                float txt_y = p0.y + 1.0f;
                if (s_w >= 26.0f) {
                    ImVec2 sz = ImGui::CalcTextSize(shadow_txt);
                    float txt_x = p0.x + (s_w - sz.x) * 0.5f;
                    draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), shadow_txt);
                    draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), shadow_txt);
                }
                if (f_w >= 26.0f) {
                    ImVec2 sz = ImGui::CalcTextSize(fire_txt);
                    float txt_x = p0.x + s_w + (f_w - sz.x) * 0.5f;
                    draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), fire_txt);
                    draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), fire_txt);
                }
                if (p_w >= 26.0f) {
                    ImVec2 sz = ImGui::CalcTextSize(pet_txt);
                    float txt_x = p0.x + s_w + f_w + (p_w - sz.x) * 0.5f;
                    draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), pet_txt);
                    draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), pet_txt);
                }

                if (is_bar_hovered) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Damage Share Breakdown (%s):", r.name.c_str());
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "■ Shadow Damage: %.1f%%", shadow_pct);
                    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.20f, 1.0f), "■ Fire Damage:   %.1f%%", fire_pct);
                    ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.50f, 1.0f), "■ Pet Damage:    %.1f%%", pet_pct);
                    ImGui::EndTooltip();
                }

                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", r.mean_dps);

                ImGui::TableNextColumn();
                ImGui::TextDisabled("+/- %.1f", r.std_dev_dps);

                if (show_stat_weights) {
                    ImGui::TableNextColumn();
                    if (r.stat_weights.valid) {
                        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "+%.2f", r.stat_weights.dps_per_sp);
                    } else {
                        ImGui::TextDisabled("-");
                    }

                    ImGui::TableNextColumn();
                    if (r.stat_weights.valid) {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "+%.1f", r.stat_weights.dps_per_hit);
                    } else {
                        ImGui::TextDisabled("-");
                    }

                    ImGui::TableNextColumn();
                    if (r.stat_weights.valid) {
                        ImGui::TextColored(ImVec4(0.9f, 0.4f, 1.0f, 1.0f), "+%.1f", r.stat_weights.dps_per_crit);
                    } else {
                        ImGui::TextDisabled("-");
                    }

                    ImGui::TableNextColumn();
                    if (r.stat_weights.valid) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.8f, 1.0f), "+%.1f", r.stat_weights.dps_per_haste);
                    } else {
                        ImGui::TextDisabled("-");
                    }

                    ImGui::TableNextColumn();
                    if (r.stat_weights.valid) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "+%.2f", r.stat_weights.dps_per_int);
                    } else {
                        ImGui::TextDisabled("-");
                    }
                }
            }

            ImGui::EndTable();
        }

        // Candidate Detail Inspector
        if (selected_candidate_idx >= 0 && selected_candidate_idx < static_cast<int>(optimizer_results.size())) {
            const auto& sel = optimizer_results[selected_candidate_idx];
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "CANDIDATE INSPECTOR: #%d %s", sel.rank, sel.name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%.1f Mean DPS | Median: %.1f | P5-P95: %.1f - %.1f]",
                sel.mean_dps, sel.batch.p50_dps > 0 ? sel.batch.p50_dps : sel.mean_dps,
                sel.batch.p5_dps, sel.batch.p95_dps);

            ImGui::Spacing();
            std::vector<PriorityRule> candidate_rules = sel.policy.get_priority_rules(sel.talents, sim.race);
            render_priority_chain_subpane(candidate_rules);
            ImGui::Spacing();

            ImGui::Columns(2, "CandidateDetailCols", true);


            // Left Column: Damage Breakdown & Performance
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Damage Breakdown (%% of Total Damage):");
            const auto& b = sel.batch;

            auto draw_dmg_bar = [](const char* name, double pct, const ImVec4& col) {
                if (pct > 0.05) {
                    ImGui::Text("%-14s:", name);
                    ImGui::SameLine(130);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f%%", pct);
                    ImGui::ProgressBar(static_cast<float>(pct / 100.0), ImVec2(180, 15), buf);
                    ImGui::PopStyleColor();
                }
            };

            draw_dmg_bar("Shadow Bolt", b.pct_shadow_bolt, ImVec4(0.5f, 0.3f, 0.9f, 1.0f));
            draw_dmg_bar("Incinerate", b.pct_incinerate, ImVec4(1.0f, 0.4f, 0.1f, 1.0f));
            draw_dmg_bar("Searing Pain", b.pct_searing_pain, ImVec4(1.0f, 0.5f, 0.1f, 1.0f));
            draw_dmg_bar("Conflagrate", b.pct_conflagrate, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
            draw_dmg_bar("Shadowburn", b.pct_shadowburn, ImVec4(0.7f, 0.2f, 0.8f, 1.0f));
            draw_dmg_bar("Corruption", b.pct_corruption, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
            draw_dmg_bar("Immolate", b.pct_immolate, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
            if (b.pct_agony > 0.05) draw_dmg_bar("Bane of Agony", b.pct_agony, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
            if (b.pct_doom > 0.05) draw_dmg_bar("Curse of Doom", b.pct_doom, ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
            if (b.pct_siphon_life > 0.05) draw_dmg_bar("Siphon Life", b.pct_siphon_life, ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
            draw_dmg_bar("Soul Fire", b.pct_soul_fire, ImVec4(1.0f, 0.2f, 0.1f, 1.0f));
            draw_dmg_bar("Drain Hope", b.pct_drain_hope, ImVec4(0.3f, 0.9f, 0.6f, 1.0f));
            draw_dmg_bar("Drain Life", b.pct_drain_life, ImVec4(0.2f, 0.9f, 0.4f, 1.0f));
            draw_dmg_bar("Drain Soul", b.pct_drain_soul, ImVec4(0.5f, 0.4f, 0.9f, 1.0f));
            if (b.pct_pet_firebolt > 0.05) draw_dmg_bar("Imp (Firebolt)", b.pct_pet_firebolt, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
            if (b.pct_pet_lash_of_pain > 0.05) draw_dmg_bar("Succubus (Lash)", b.pct_pet_lash_of_pain, ImVec4(0.7f, 0.3f, 0.9f, 1.0f));
            if (b.pct_pet_melee > 0.05) draw_dmg_bar("Succubus (Melee)", b.pct_pet_melee, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            if (b.pct_demonic_brand > 0.05) draw_dmg_bar("Demonic Brand", b.pct_demonic_brand, ImVec4(0.9f, 0.4f, 0.8f, 1.0f));
            if (b.pct_pet > 0.05) {
                char pet_summary[64];
                snprintf(pet_summary, sizeof(pet_summary), "Total Pet: %.1f DPS (%.1f%%)", b.mean_pet_dps, b.pct_pet);
                ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", pet_summary);
            }

            ImGui::Spacing();
            ImGui::Text("Combat Performance:");
            ImGui::BulletText("ISB Vulnerability Uptime: %.1f%%", sel.isb_uptime);
            ImGui::BulletText("Spell Crit Rate: %.1f%% | Miss Rate: %.1f%%", b.crit_percent, b.miss_percent);
            ImGui::BulletText("Life Taps per fight: %.1f (Mana spent: %.0f)", b.mean_life_taps, b.mean_mana_spent);
            ImGui::BulletText("Min - Max DPS Range: [%.1f - %.1f]", sel.min_dps, sel.max_dps);

            if (sel.stat_weights.valid) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Local Stat Sensitivity / Weights (DPS per +1 Stat):");
                ImGui::BulletText("+1 Spell Power:  %.2f DPS", sel.stat_weights.dps_per_sp);
                ImGui::BulletText("+1%% Spell Hit:   %.1f DPS (EP: %.1f SP)", sel.stat_weights.dps_per_hit, sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_hit / sel.stat_weights.dps_per_sp) : 0.0);
                ImGui::BulletText("+1%% Spell Crit:  %.1f DPS (EP: %.1f SP)", sel.stat_weights.dps_per_crit, sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_crit / sel.stat_weights.dps_per_sp) : 0.0);
                ImGui::BulletText("+1%% Spell Haste: %.1f DPS (EP: %.1f SP)", sel.stat_weights.dps_per_haste, sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_haste / sel.stat_weights.dps_per_sp) : 0.0);
                ImGui::BulletText("+1 Intellect:     %.2f DPS", sel.stat_weights.dps_per_int);
            }

            ImGui::NextColumn();

            // Right Column: Observed Spell Cast Sequence & Combat Rotation
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Observed Combat Rotation & Cast Sequence:");

            // Obtain sample cast sequence
            std::vector<SpellCastLog> seq = b.sample_timeline.cast_sequence;
            if (seq.empty()) {
                WarlockSimulator s = sim;
                s.talents = sel.talents;
                s.policy = sel.policy;
                s.buffs = sel.buffs;
                s.record_timeline = true;
                FastRNG rng(0x13374242ULL);
                SimResult res = s.run_single_simulation(rng);
                seq = res.cast_sequence;
            }

            // 1. Opener Sequence Badges (First 16-24 Spells Cast)
            int opener_count = (int)std::min(seq.size(), (size_t)16);
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Sequence (First %d Spells Cast in Fight):", opener_count);
            ImGui::BeginChild("OpenerSequenceBox", ImVec2(-1, 56), true, ImGuiWindowFlags_HorizontalScrollbar);
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

            // 2. Observed Spell Cast Order & Usage Table
            struct SpellStat {
                SpellID id;
                int count = 0;
                double first_cast = -1.0;
                std::string role;
            };
            std::vector<SpellStat> stats;
            int total_observed_casts = (int)seq.size();
            for (const auto& cast : seq) {
                auto it = std::find_if(stats.begin(), stats.end(), [&](const SpellStat& s){ return s.id == cast.spell_id; });
                if (it == stats.end()) {
                    SpellStat s;
                    s.id = cast.spell_id;
                    s.count = 1;
                    s.first_cast = cast.time;
                    s.role = cast.tag;
                    stats.push_back(s);
                } else {
                    it->count++;
                }
            }
            std::sort(stats.begin(), stats.end(), [](const SpellStat& a, const SpellStat& b){
                return a.first_cast < b.first_cast;
            });

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observed Cast Order & Role Breakdown (120s Fight):");
            if (ImGui::BeginTable("ObservedSpellsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
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
                    if (st.id == SpellID::CURSE_OF_AGONY) {
                        role_desc = "DoT (Bane of Agony; maintained every 24s)";
                    } else if (st.id == SpellID::CURSE_OF_DOOM) {
                        role_desc = "Curse (Cast on 60s cooldown)";
                    } else if (st.id == SpellID::CURSE_OF_SHADOWS || st.id == SpellID::CURSE_OF_ELEMENTS) {
                        role_desc = "Raid Debuff";
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

            // 3. Pet & Setup Details
            ImGui::Spacing();
            std::string pet_desc;
            if (sel.buffs.sacrifice_imp && sel.buffs.sacrifice_succubus) {
                pet_desc = "Double Sacrificed";
            } else if (sel.talents.demo.demonic_pact > 0) {
                pet_desc = "Demonic Pact: Sac Imp (+15% Shadow) + Active Succubus (+10% MD, +60 SP, +3% SL)";
            } else if (sel.buffs.sacrifice_imp) {
                pet_desc = "Sacrificed Imp (+15% Shadow Damage)";
            } else if (sel.buffs.sacrifice_succubus) {
                pet_desc = "Sacrificed Succubus (+15% Fire Damage)";
            } else {
                pet_desc = pet_choice_to_string(sel.policy.pet);
            }
            ImGui::BulletText("Demon Pet / Sacrifice: %s", pet_desc.c_str());
            ImGui::BulletText("Mana Conservation: Life Tap below %.0f%% Mana", sel.policy.life_tap_threshold_pct);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.25f, 0.65f, 1.0f));
            if (ImGui::Button("▶ Load Configuration into Preset Simulation", ImVec2(320, 28))) {
                apply_candidate_config(sel);
            }
            ImGui::PopStyleColor();

            ImGui::Columns(1);
        }
    } else {
        ImGui::TextDisabled("Select an optimization target above to begin multi-threaded parameter search.");
    }
}

} // namespace warlock
