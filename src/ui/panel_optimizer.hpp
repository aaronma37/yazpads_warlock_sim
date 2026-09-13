#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
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
    std::string& current_opt_target
) {
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "Multi-Threaded Optimization Engine:");
    ImGui::TextWrapped("Spawns hundreds of thousands of parallel DES simulations across all CPU threads to brute-force the optimal talents, gear, and combat policy.");
    ImGui::Separator();

    static int iters_per_candidate = 3000;
    ImGui::SliderInt("Sims Per Candidate", &iters_per_candidate, 1000, 20000, "%d fights");

    ImGui::Spacing();
    if (is_optimizing) ImGui::BeginDisabled();

    if (ImGui::Button("Optimize Standard Specs", ImVec2(210, 28))) {
        is_optimizing = true;
        opt_progress = 0.0f;
        optimizer_results = Optimizer::optimize_talents(sim, iters_per_candidate, [&](float p, const std::string& name) {
            opt_progress = p;
            current_opt_target = name;
        });
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
        });
        is_optimizing = false;
        opt_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Optimize Phase Gear", ImVec2(210, 28))) {
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
    if (ImGui::Button("Optimize Consumables", ImVec2(210, 28))) {
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
    if (ImGui::Button("Optimize Rotation Policy", ImVec2(210, 28))) {
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
        ImGui::Text("Simulating: %s...", current_opt_target.c_str());
        ImGui::ProgressBar(opt_progress, ImVec2(-1, 8));
    }

    ImGui::Separator();

    if (!optimizer_results.empty()) {
        const auto& best = optimizer_results[0];
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "WINNING OPTIMAL CONFIGURATION: %s", best.name.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[%.1f Mean DPS]", best.mean_dps);

        ImGui::SameLine(500);
        if (ImGui::Button("Apply Best Config To Current Setup")) {
            if (best.category == "Talents" || best.category == "Combinatorial Talents") {
                sim.talents = best.talents;
                sim.buffs = best.buffs;
                sim.policy.pet = best.policy.pet;
            } else if (best.category == "Gear") {
                sim.gear = best.gear;
                sim.use_raw_stats = false;
            } else if (best.category == "Consumables") {
                sim.buffs = best.buffs;
            } else if (best.category == "Stat Values (EP)") {
                sim.use_raw_stats = true;
                sim.raw_stats = best.raw_stats;
            } else if (best.category == "Policy") {
                sim.policy = best.policy;
            }
        }

        ImGui::Separator();

        static int selected_candidate_idx = 0;
        if (selected_candidate_idx >= static_cast<int>(optimizer_results.size())) {
            selected_candidate_idx = 0;
        }

        // Leaderboard table
        ImGui::Text("Candidate Ranking Leaderboard (Click a candidate row or 'Inspect' to view damage breakdown & rotation):");
        if (ImGui::BeginTable("OptLeaderboardTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45);
            ImGui::TableSetupColumn("Candidate Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Mean DPS", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("+/- StdDev", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Delta vs BiS", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 130);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < optimizer_results.size(); ++i) {
                const auto& r = optimizer_results[i];
                bool is_selected = (selected_candidate_idx == static_cast<int>(i));

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (r.rank == 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1 [BEST]");
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
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", r.mean_dps);

                ImGui::TableNextColumn();
                ImGui::TextDisabled("+/- %.1f", r.std_dev_dps);

                ImGui::TableNextColumn();
                double delta = r.mean_dps - best.mean_dps;
                if (delta >= 0.0) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Baseline");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%.1f DPS", delta);
                }

                ImGui::TableNextColumn();
                ImGui::PushID(static_cast<int>(i + 10000));
                if (ImGui::SmallButton("Inspect")) {
                    selected_candidate_idx = static_cast<int>(i);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Apply")) {
                    if (r.category == "Talents" || r.category == "Combinatorial Talents") {
                        sim.talents = r.talents;
                        sim.buffs = r.buffs;
                        sim.policy.pet = r.policy.pet;
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
                    }
                }
                ImGui::PopID();
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

            ImGui::Columns(2, "CandidateDetailCols", true);

            // Left Column: Damage Breakdown & Performance
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Damage Breakdown (%% of Total Damage):");
            const auto& b = sel.batch;

            auto draw_dmg_bar = [](const char* name, double pct, const ImVec4& col) {
                if (pct > 0.05) {
                    ImGui::Text("%-14s: %5.1f%%", name, pct);
                    ImGui::SameLine(180);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f%%", pct);
                    ImGui::ProgressBar(static_cast<float>(pct / 100.0), ImVec2(180, 15), buf);
                    ImGui::PopStyleColor();
                }
            };

            draw_dmg_bar("Shadow Bolt", b.pct_shadow_bolt, ImVec4(0.5f, 0.3f, 0.9f, 1.0f));
            draw_dmg_bar("Incinerate", b.pct_incinerate, ImVec4(1.0f, 0.4f, 0.1f, 1.0f));
            draw_dmg_bar("Conflagrate", b.pct_conflagrate, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
            draw_dmg_bar("Shadowburn", b.pct_shadowburn, ImVec4(0.7f, 0.2f, 0.8f, 1.0f));
            draw_dmg_bar("Corruption", b.pct_corruption, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
            draw_dmg_bar("Immolate", b.pct_immolate, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
            draw_dmg_bar("Curse / Bane", b.pct_curse, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
            draw_dmg_bar("Soul Fire", b.pct_soul_fire, ImVec4(1.0f, 0.2f, 0.1f, 1.0f));
            draw_dmg_bar("Drain Hope", b.pct_drain_hope, ImVec4(0.3f, 0.9f, 0.6f, 1.0f));
            if (b.pct_pet > 0.05) {
                char pet_name[64];
                snprintf(pet_name, sizeof(pet_name), "Pet (%.1f DPS)", b.mean_pet_dps);
                draw_dmg_bar(pet_name, b.pct_pet, ImVec4(0.2f, 0.8f, 0.3f, 1.0f));
            }

            ImGui::Spacing();
            ImGui::Text("Combat Performance:");
            ImGui::BulletText("ISB Vulnerability Uptime: %.1f%%", sel.isb_uptime);
            ImGui::BulletText("Spell Crit Rate: %.1f%% | Miss Rate: %.1f%%", b.crit_percent, b.miss_percent);
            ImGui::BulletText("Life Taps per fight: %.1f (Mana spent: %.0f)", b.mean_life_taps, b.mean_mana_spent);
            ImGui::BulletText("Min - Max DPS Range: [%.1f - %.1f]", sel.min_dps, sel.max_dps);

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
            if (ImGui::Button("Apply This Exact Setup To Simulator", ImVec2(280, 26))) {
                if (sel.category == "Talents" || sel.category == "Combinatorial Talents") {
                    sim.talents = sel.talents;
                    sim.buffs = sel.buffs;
                    sim.policy.pet = sel.policy.pet;
                } else if (sel.category == "Gear") {
                    sim.gear = sel.gear;
                    sim.use_raw_stats = false;
                } else if (sel.category == "Consumables") {
                    sim.buffs = sel.buffs;
                } else if (sel.category == "Stat Values (EP)") {
                    sim.use_raw_stats = true;
                    sim.raw_stats = sel.raw_stats;
                } else if (sel.category == "Policy") {
                    sim.policy = sel.policy;
                }
            }

            ImGui::Columns(1);
        }
    } else {
        ImGui::TextDisabled("Select an optimization target above to begin multi-threaded parameter search.");
    }
}

} // namespace warlock
