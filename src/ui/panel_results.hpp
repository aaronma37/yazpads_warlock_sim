#pragma once
#include "imgui.h"
#include "implot.h"
#include "asset_manager.hpp"
#include "src/sim/parallel_runner.hpp"
#include <vector>
#include <algorithm>

namespace warlock {

inline void render_panel_results(const BatchSimResult& batch) {
    if (batch.total_iterations == 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No simulation results yet. Click '>>> RUN DES SIMULATION <<<' above to begin.");
        return;
    }

    // Top Summary Metric Cards
    ImGui::Columns(4, "ResultsSummaryCols", false);

    ImGui::TextDisabled("MEAN DPS");
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1f", batch.mean_dps);
    ImGui::TextDisabled("+/- %.1f std dev", batch.std_dev_dps);
    ImGui::NextColumn();

    ImGui::TextDisabled("P50 MEDIAN (P5 - P95)");
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%.1f", batch.p50_dps);
    ImGui::TextDisabled("[%.1f - %.1f]", batch.p5_dps, batch.p95_dps);
    ImGui::NextColumn();

    ImGui::TextDisabled("ISB UPTIME");
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "%.1f%%", batch.mean_isb_uptime);
    ImGui::TextDisabled("Improved Shadow Bolt");
    ImGui::NextColumn();

    ImGui::TextDisabled("CRIT / MISS RATE");
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "%.1f%% Crit", batch.crit_percent);
    ImGui::TextDisabled("%.1f%% Missed", batch.miss_percent);
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Separator();

    if (ImGui::BeginTabBar("ResultsTabBar")) {
        // Tab 1: DPS Distribution Histogram
        if (ImGui::BeginTabItem("DPS Histogram")) {
            if (!batch.histogram.empty()) {
                std::vector<double> xs(batch.histogram.size());
                std::vector<double> ys(batch.histogram.size());
                double bar_width = 0.0;
                if (!batch.histogram.empty()) {
                    bar_width = batch.histogram[0].max_dps - batch.histogram[0].min_dps;
                }

                for (size_t i = 0; i < batch.histogram.size(); ++i) {
                    xs[i] = (batch.histogram[i].min_dps + batch.histogram[i].max_dps) * 0.5;
                    ys[i] = static_cast<double>(batch.histogram[i].count);
                }

                if (ImPlot::BeginPlot("DPS Frequency Distribution", ImVec2(-1, 320))) {
                    ImPlot::SetupAxes("Damage Per Second (DPS)", "Fight Count");
                    ImPlot::PlotBars("DPS Histogram", xs.data(), ys.data(), static_cast<int>(xs.size()), bar_width * 0.9);

                    // Vertical lines for Mean, P5, P95
                    double mean_x = batch.mean_dps;
                    double p5_x = batch.p5_dps;
                    double p95_x = batch.p95_dps;
                    ImPlot::PlotInfLines("Mean", &mean_x, 1);
                    ImPlot::PlotInfLines("P5 (Low)", &p5_x, 1);
                    ImPlot::PlotInfLines("P95 (High)", &p95_x, 1);

                    ImPlot::EndPlot();
                }
            }
            ImGui::EndTabItem();
        }

        // Tab 2: Sample Combat Timeline
        if (ImGui::BeginTabItem("Sample Combat Timeline")) {
            const auto& t = batch.sample_timeline.timeline;
            if (!t.empty()) {
                std::vector<double> times;
                std::vector<double> manas;
                std::vector<double> dmgs;
                times.reserve(t.size());
                manas.reserve(t.size());
                dmgs.reserve(t.size());

                for (const auto& pt : t) {
                    times.push_back(pt.time);
                    manas.push_back(pt.mana);
                    dmgs.push_back(pt.damage);
                }

                if (ImPlot::BeginPlot("Sample Fight Timeline", ImVec2(-1, 320))) {
                    ImPlot::SetupAxes("Fight Time (seconds)", "Value");
                    ImPlot::PlotLine("Player Mana", times.data(), manas.data(), static_cast<int>(times.size()));
                    ImPlot::PlotScatter("Spell Damage Hit", times.data(), dmgs.data(), static_cast<int>(times.size()));
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::Text("No timeline recorded.");
            }
            ImGui::EndTabItem();
        }

        // Tab 3: Damage Breakdown
        if (ImGui::BeginTabItem("Damage Breakdown")) {
            ImGui::Text("Spell Damage Contribution (%% of Total):");
            ImGui::Separator();

            ImGui::Text("Shadow Bolt: ");
            ImGui::SameLine(180);
            ImGui::ProgressBar(static_cast<float>(batch.pct_shadow_bolt * 0.01), ImVec2(240, 0), "");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "%.1f%%", batch.pct_shadow_bolt);

            ImGui::Text("Corruption: ");
            ImGui::SameLine(180);
            ImGui::ProgressBar(static_cast<float>(batch.pct_corruption * 0.01), ImVec2(240, 0), "");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "%.1f%%", batch.pct_corruption);

            ImGui::Text("Curse: ");
            ImGui::SameLine(180);
            ImGui::ProgressBar(static_cast<float>(batch.pct_curse * 0.01), ImVec2(240, 0), "");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%.1f%%", batch.pct_curse);

            ImGui::Text("Immolate: ");
            ImGui::SameLine(180);
            ImGui::ProgressBar(static_cast<float>(batch.pct_immolate * 0.01), ImVec2(240, 0), "");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "%.1f%%", batch.pct_immolate);

            ImGui::Text("Shadowburn: ");
            ImGui::SameLine(180);
            ImGui::ProgressBar(static_cast<float>(batch.pct_shadowburn * 0.01), ImVec2(240, 0), "");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.7f, 1.0f), "%.1f%%", batch.pct_shadowburn);

            if (batch.pct_conflagrate > 0.001) {
                ImGui::Text("Conflagrate: ");
                ImGui::SameLine(180);
                ImGui::ProgressBar(static_cast<float>(batch.pct_conflagrate * 0.01), ImVec2(240, 0), "");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.1f, 1.0f), "%.1f%%", batch.pct_conflagrate);
            }

            if (batch.pct_incinerate > 0.001) {
                ImGui::Text("Incinerate: ");
                ImGui::SameLine(180);
                ImGui::ProgressBar(static_cast<float>(batch.pct_incinerate * 0.01), ImVec2(240, 0), "");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%.1f%%", batch.pct_incinerate);
            }

            if (batch.pct_soul_fire > 0.001) {
                ImGui::Text("Soul Fire: ");
                ImGui::SameLine(180);
                ImGui::ProgressBar(static_cast<float>(batch.pct_soul_fire * 0.01), ImVec2(240, 0), "");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%.1f%%", batch.pct_soul_fire);
            }

            if (batch.pct_drain_hope > 0.001) {
                ImGui::Text("Drain Hope: ");
                ImGui::SameLine(180);
                ImGui::ProgressBar(static_cast<float>(batch.pct_drain_hope * 0.01), ImVec2(240, 0), "");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.6f, 0.3f, 0.9f, 1.0f), "%.1f%%", batch.pct_drain_hope);
            }

            if (batch.pct_pet > 0.001) {
                ImGui::Text("Demon (Pet): ");
                ImGui::SameLine(180);
                ImGui::ProgressBar(static_cast<float>(batch.pct_pet * 0.01), ImVec2(240, 0), "");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%.1f%% (%.1f Pet DPS)", batch.pct_pet, batch.mean_pet_dps);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Fight Averages:");
            ImGui::BulletText("Shadow Bolt Casts: %.1f", batch.mean_shadow_bolts);
            ImGui::BulletText("Shadow Bolt Crits: %.1f", batch.mean_crits);
            ImGui::BulletText("Life Taps Cast: %.1f", batch.mean_life_taps);
            ImGui::BulletText("Mana Consumed: %.0f", batch.mean_mana_spent);

            ImGui::EndTabItem();
        }

        // Tab 4: Observed Spell Cast Sequence
        if (ImGui::BeginTabItem("Observed Spell Cast Sequence")) {
            const auto& seq = batch.sample_timeline.cast_sequence;
            if (seq.empty()) {
                ImGui::TextDisabled("No sample cast sequence available. Run a simulation to generate observed sequence.");
            } else {
                // Opener Sequence Badges
                int opener_count = (int)std::min(seq.size(), (size_t)16);
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Flow (First %d Spells Cast):", opener_count);
                ImGui::BeginChild("ResultOpenerBox", ImVec2(-1, 56), true, ImGuiWindowFlags_HorizontalScrollbar);
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

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Complete Cast History (%zu total casts in 120s fight):", seq.size());

                if (ImGui::BeginTable("AllCastsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(-1, 240))) {
                    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28);
                    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 50);
                    ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthFixed, 140);
                    ImGui::TableSetupColumn("Cast Time", ImGuiTableColumnFlags_WidthFixed, 70);
                    ImGui::TableSetupColumn("Result / Damage", ImGuiTableColumnFlags_WidthFixed, 130);
                    ImGui::TableSetupColumn("Trigger / Role", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < seq.size(); ++i) {
                        const auto& cast = seq[i];
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("%zu", i + 1);

                        ImGui::TableNextColumn();
                        ImGui::Text("%.1fs", cast.time);

                        ImGui::TableNextColumn();
                        Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(cast.spell_id));
                        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                        ImGui::SameLine();
                        ImGui::Text("%s", spell_id_to_name(cast.spell_id));

                        ImGui::TableNextColumn();
                        if (cast.cast_time > 0.0) {
                            ImGui::Text("%.1fs", cast.cast_time);
                        } else {
                            ImGui::TextDisabled("Instant");
                        }

                        ImGui::TableNextColumn();
                        if (cast.damage > 0.0) {
                            if (cast.is_crit) {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%.0f CRIT!", cast.damage);
                            } else {
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%.0f Hit", cast.damage);
                            }
                        } else if (cast.is_miss) {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "MISS");
                        } else {
                            ImGui::TextDisabled("-");
                        }

                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", cast.tag.c_str());
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

} // namespace warlock
