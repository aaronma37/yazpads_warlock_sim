#pragma once
#include "imgui.h"
#include "implot.h"
#include "src/ui/common/asset_manager.hpp"
#include "src/ui/common/damage_breakdown_view.hpp"
#include "src/ui/common/wow_widgets.hpp"
#include "src/sim/priest/parallel_runner.hpp"
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace priest {

inline void render_priest_panel_results(const BatchSimResult& batch) {
    if (batch.total_iterations == 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No simulation results yet. Click 'RUN PRIEST DES SIMULATION' above to begin.");
        return;
    }

    // Top Summary Metric Cards over a tiled marble stone backdrop.
    warlock::DrawPanelBanner("Combat Results - DPS Summary", "UI-Background-Marble", 52.0f);
    warlock::BeginWowChild("PriestResultsSummaryBG", ImVec2(-1, 92), true);
    {
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImVec2 bp0(wp.x + 2.0f, wp.y + 2.0f);
        ImVec2 bp1(wp.x + ws.x - 2.0f, wp.y + ws.y - 2.0f);
        warlock::DrawTiledPanelBackdrop(bp0, bp1, "UI-Background-Rock", IM_COL32(255, 255, 255, 255));
        ImGui::GetWindowDrawList()->AddRectFilled(bp0, bp1, IM_COL32(0, 0, 0, 110));
    }
    ImGui::Columns(4, "PriestResultsSummaryCols", false);

    ImGui::TextDisabled("MEAN DPS");
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1f", batch.mean_dps);
    ImGui::TextDisabled("+/- %.1f std dev", batch.std_dev_dps);
    ImGui::NextColumn();

    ImGui::TextDisabled("P50 MEDIAN (P5 - P95)");
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%.1f", batch.p50_dps);
    ImGui::TextDisabled("[%.1f - %.1f]", batch.p5_dps, batch.p95_dps);
    ImGui::NextColumn();

    ImGui::TextDisabled("SHADOW WEAVING");
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "%.1f procs", batch.mean_sw_weaving_procs);
    ImGui::TextDisabled("Mean Procs / Fight");
    ImGui::NextColumn();

    ImGui::TextDisabled("CRIT / MISS RATE");
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "%.1f%% Crit", batch.crit_percent);
    ImGui::TextDisabled("%.1f%% Missed", batch.miss_percent);
    ImGui::NextColumn();

    ImGui::Columns(1);
    warlock::EndWowChild();
    ImGui::Separator();

    if (warlock::WowBeginTabBar("PriestResultsTabBar")) {
        // Tab 1: DPS Distribution Histogram
        if (warlock::WowBeginTabItem("DPS Histogram")) {
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

                if (ImPlot::BeginPlot("DPS Frequency Distribution##Priest", ImVec2(-1, 320))) {
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
            warlock::WowEndTabItem();
        }

        // Tab 2: Sample Combat Timeline
        if (warlock::WowBeginTabItem("Sample Combat Timeline")) {
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

                if (ImPlot::BeginPlot("Sample Fight Timeline##Priest", ImVec2(-1, 320))) {
                    ImPlot::SetupAxes("Fight Time (seconds)", "Value");
                    ImPlot::PlotLine("Player Mana", times.data(), manas.data(), static_cast<int>(times.size()));
                    ImPlot::PlotScatter("Spell Damage Hit", times.data(), dmgs.data(), static_cast<int>(times.size()));
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::Text("No timeline recorded.");
            }
            warlock::WowEndTabItem();
        }

        // Tab 3: Damage Breakdown (Styled matching golden standard)
        if (warlock::WowBeginTabItem("Damage Breakdown")) {
            ImGui::Text("Spell Damage Contribution (%% of Total + DPS):");
            ImGui::Separator();

            render_priest_damage_breakdown_bars(batch, 240.0f, 180.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Fight Averages:");
            if (batch.spell_stats[static_cast<size_t>(SpellID::SHADOW_WORD_PAIN)].mean_casts > 0.05)
                ImGui::BulletText("Shadow Word: Pain Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::SHADOW_WORD_PAIN)].mean_casts);
            if (batch.spell_stats[static_cast<size_t>(SpellID::MIND_FLAY)].mean_hits > 0.05)
                ImGui::BulletText("Mind Flay Ticks: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::MIND_FLAY)].mean_hits);
            if (batch.spell_stats[static_cast<size_t>(SpellID::MIND_BLAST)].mean_casts > 0.05)
                ImGui::BulletText("Mind Blast Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::MIND_BLAST)].mean_casts);
            if (batch.spell_stats[static_cast<size_t>(SpellID::SHADOW_WORD_DEATH)].mean_casts > 0.05)
                ImGui::BulletText("Shadow Word: Death Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::SHADOW_WORD_DEATH)].mean_casts);
            if (batch.spell_stats[static_cast<size_t>(SpellID::DEVOURING_PLAGUE)].mean_casts > 0.05)
                ImGui::BulletText("Devouring Plague Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::DEVOURING_PLAGUE)].mean_casts);
            if (batch.spell_stats[static_cast<size_t>(SpellID::SMITE)].mean_casts > 0.05)
                ImGui::BulletText("Smite Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::SMITE)].mean_casts);
            if (batch.spell_stats[static_cast<size_t>(SpellID::HOLY_FIRE)].mean_casts > 0.05)
                ImGui::BulletText("Holy Fire Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::HOLY_FIRE)].mean_casts);
            if (batch.spell_stats[static_cast<size_t>(SpellID::PENANCE)].mean_casts > 0.05)
                ImGui::BulletText("Penance Casts: %.1f", batch.spell_stats[static_cast<size_t>(SpellID::PENANCE)].mean_casts);
            ImGui::BulletText("Shadow Weaving Procs: %.1f", batch.mean_sw_weaving_procs);
            ImGui::BulletText("Mana Consumed: %.0f", batch.mean_mana_spent);
            if (batch.mean_mana_gained > 0.0) {
                ImGui::BulletText("Mana Gained / Regenerated: %.0f", batch.mean_mana_gained);
            }

            warlock::WowEndTabItem();
        }

        // Tab 4: Observed Spell Cast Sequence
        if (warlock::WowBeginTabItem("Observed Spell Cast Sequence")) {
            static bool priest_show_all_damage_instances = false;

            std::vector<SpellCastLog> seq;
            if (priest_show_all_damage_instances) {
                seq = batch.sample_timeline.get_damage_sequence();
            } else {
                seq = batch.sample_timeline.cast_sequence;
            }

            if (seq.empty()) {
                ImGui::TextDisabled("No sample sequence available. Run a simulation to generate observed sequence.");
            } else {
                ImGui::AlignTextToFramePadding();
                warlock::WowCheckbox("Show All Damage Instances (vs Casts Only)##PriestDamageToggle", &priest_show_all_damage_instances);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("When checked, shows all individual damage hits, DoT/channel ticks, and procs.\nWhen unchecked, shows player cast actions.");
                }
                ImGui::Spacing();

                int opener_count = (int)std::min(seq.size(), (size_t)16);
                if (priest_show_all_damage_instances) {
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Damage Flow (First %d Events):", opener_count);
                } else {
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Flow (First %d Spells Cast):", opener_count);
                }
                ImGui::BeginChild("PriestResultOpenerBox", ImVec2(-1, 56), true, ImGuiWindowFlags_HorizontalScrollbar);
                for (size_t i = 0; i < std::min(seq.size(), (size_t)24); ++i) {
                    const auto& cast = seq[i];
                    if (i > 0) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("->");
                        ImGui::SameLine();
                    }
                    ImGui::BeginGroup();
                    Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(cast.spell_id));
                    if (icon.id > 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", spell_id_to_string(cast.spell_id));
                        if (priest_show_all_damage_instances) {
                            ImGui::Text("Time: %.1fs", cast.time);
                            ImGui::Text("Type: %s", cast.tag.c_str());
                        } else {
                            ImGui::Text("Time: %.1fs  |  Cast Duration: %.1fs", cast.time, cast.cast_time);
                            ImGui::Text("Role: %s", cast.tag.c_str());
                        }
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
                if (priest_show_all_damage_instances) {
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Complete Damage History (%zu total damage events in sample fight):", seq.size());
                } else {
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Complete Cast History (%zu total casts in sample fight):", seq.size());
                }
                ImGui::SameLine();

                auto generate_priest_cast_history_text = [&seq, &batch]() -> std::string {
                    std::ostringstream ss;
                    ss << "========================================================================================\n";
                    if (priest_show_all_damage_instances) {
                        ss << "                       OBSERVED DAMAGE EVENT HISTORY & COMBAT LOG                       \n";
                    } else {
                        ss << "                       OBSERVED SPELL CAST SEQUENCE & COMBAT HISTORY                    \n";
                    }
                    ss << "========================================================================================\n";
                    ss << "Mean DPS: " << std::fixed << std::setprecision(1) << batch.mean_dps 
                       << " | Total " << (priest_show_all_damage_instances ? "Damage Events: " : "Casts: ") << seq.size() << "\n\n";
                    if (priest_show_all_damage_instances) {
                        ss << std::left << std::setw(5)  << "#"
                           << std::setw(10) << "Time (s)"
                           << std::setw(24) << "Spell / Source"
                           << std::setw(16) << "Event Type"
                           << "Result / Dmg\n";
                    } else {
                        ss << std::left << std::setw(5)  << "#"
                           << std::setw(10) << "Time (s)"
                           << std::setw(24) << "Spell"
                           << std::setw(14) << "Cast Time"
                           << std::setw(18) << "Result / Dmg"
                           << "Trigger / Role\n";
                    }
                    ss << "----------------------------------------------------------------------------------------\n";

                    for (size_t i = 0; i < seq.size(); ++i) {
                        const auto& cast = seq[i];
                        std::string res_str;
                        if (cast.damage > 0.0) {
                            res_str = std::to_string(static_cast<int>(cast.damage)) + (cast.is_crit ? " (CRIT)" : " (Hit)");
                        } else if (cast.is_miss) {
                            res_str = "MISS";
                        } else {
                            res_str = "-";
                        }

                        std::ostringstream time_buf;
                        time_buf << std::fixed << std::setprecision(1) << cast.time << "s";

                        if (priest_show_all_damage_instances) {
                            ss << std::left << std::setw(5)  << (i + 1)
                               << std::setw(10) << time_buf.str()
                               << std::setw(24) << spell_id_to_string(cast.spell_id)
                               << std::setw(16) << cast.tag
                               << res_str << "\n";
                        } else {
                            std::string cast_str = (cast.cast_time > 0.0) ? (std::to_string(cast.cast_time).substr(0, 3) + "s") : "Instant";
                            ss << std::left << std::setw(5)  << (i + 1)
                               << std::setw(10) << time_buf.str()
                               << std::setw(24) << spell_id_to_string(cast.spell_id)
                               << std::setw(14) << cast_str
                               << std::setw(18) << res_str
                               << cast.tag << "\n";
                        }
                    }
                    ss << "========================================================================================\n";
                    return ss.str();
                };

                static float export_feedback_timer = 0.0f;
                static std::string export_feedback_msg = "";

                if (warlock::WowButton(priest_show_all_damage_instances ? "📋 Copy Damage Log to Clipboard##Priest" : "📋 Copy Cast History to Clipboard##Priest")) {
                    std::string text = generate_priest_cast_history_text();
                    ImGui::SetClipboardText(text.c_str());
                    export_feedback_msg = "Copied to clipboard!";
                    export_feedback_timer = 3.0f;
                }
                ImGui::SameLine();
                if (warlock::WowButton(priest_show_all_damage_instances ? "💾 Export Damage Log (priest_damage_events.txt)##Priest" : "💾 Export to Text File (priest_cast_sequence.txt)##Priest")) {
                    std::string text = generate_priest_cast_history_text();
                    std::string fname = priest_show_all_damage_instances ? "priest_damage_events.txt" : "priest_cast_sequence.txt";
                    std::ofstream out(fname);
                    if (out.is_open()) {
                        out << text;
                        out.close();
                        export_feedback_msg = "Saved to " + fname + "!";
                    } else {
                        export_feedback_msg = "Failed to open file for writing.";
                    }
                    export_feedback_timer = 3.0f;
                }

                if (export_feedback_timer > 0.0f) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "✔ %s", export_feedback_msg.c_str());
                    export_feedback_timer -= ImGui::GetIO().DeltaTime;
                }

                int priest_table_cols = priest_show_all_damage_instances ? 5 : 6;
                if (ImGui::BeginTable("PriestAllCastsTable", priest_table_cols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(-1, 240))) {
                    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28);
                    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 50);
                    ImGui::TableSetupColumn("Spell / Source", ImGuiTableColumnFlags_WidthFixed, 140);
                    ImGui::TableSetupColumn(priest_show_all_damage_instances ? "Event Type" : "Cast Time", ImGuiTableColumnFlags_WidthFixed, priest_show_all_damage_instances ? 110 : 70);
                    ImGui::TableSetupColumn("Result / Damage", priest_show_all_damage_instances ? ImGuiTableColumnFlags_WidthStretch : ImGuiTableColumnFlags_WidthFixed, 130);
                    if (!priest_show_all_damage_instances) {
                        ImGui::TableSetupColumn("Trigger / Role", ImGuiTableColumnFlags_WidthStretch);
                    }
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < seq.size(); ++i) {
                        const auto& cast = seq[i];
                        ImGui::TableNextRow();

                        // Col 0: Index
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextDisabled("%zu", i + 1);

                        // Col 1: Time
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%.1fs", cast.time);

                        // Col 2: Spell
                        ImGui::TableSetColumnIndex(2);
                        Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(cast.spell_id));
                        if (icon.id > 0) {
                            ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                            ImGui::SameLine(0, 4);
                        }
                        ImGui::TextUnformatted(spell_id_to_string(cast.spell_id));

                        // Col 3: Cast Time / Event Type
                        ImGui::TableSetColumnIndex(3);
                        if (priest_show_all_damage_instances) {
                            ImGui::Text("%s", cast.tag.c_str());
                        } else {
                            if (cast.cast_time > 0.0) {
                                ImGui::Text("%.1fs", cast.cast_time);
                            } else {
                                ImGui::TextDisabled("Instant");
                            }
                        }

                        // Col 4: Result / Damage
                        ImGui::TableSetColumnIndex(4);
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

                        if (!priest_show_all_damage_instances) {
                            ImGui::TableSetColumnIndex(5);
                            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", cast.tag.c_str());
                        }
                    }
                    ImGui::EndTable();
                }
            }
            warlock::WowEndTabItem();
        }

        warlock::WowEndTabBar();
    }
}

inline void render_priest_build_sim_results(const BatchSimResult& batch) {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Simulation Results");
    ImGui::Spacing();

    if (batch.total_iterations == 0) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No simulation results yet.\nRun a simulation to view results.");
        return;
    }

    // Top DPS numbers
    ImGui::BeginGroup();
    ImGui::TextDisabled("MEAN DPS");
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1f DPS", batch.mean_dps);

    ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "(Min: %.0f  Max: %.0f  ±%.1f)",
                       batch.min_dps, batch.max_dps, batch.std_dev_dps);
    ImGui::TextDisabled("Median: %.1f (P5: %.1f - P95: %.1f)", batch.p50_dps, batch.p5_dps, batch.p95_dps);
    ImGui::TextDisabled("Crit: %.1f%%  |  Miss: %.1f%%", batch.crit_percent, batch.miss_percent);
    if (batch.mean_sw_weaving_procs > 0.0) {
        ImGui::TextDisabled("SW Weaving: %.1f procs", batch.mean_sw_weaving_procs);
    }
    ImGui::EndGroup();

    ImGui::Spacing();
    static bool open_detailed_modal = false;
    if (warlock::WowButton("Detailed View", ImVec2(-1, 26))) {
        open_detailed_modal = true;
        ImGui::OpenPopup("Detailed Simulation Results##Priest");
    }

    // Detailed View Modal
    ImGui::SetNextWindowSize(ImVec2(860, 620), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Detailed Simulation Results##Priest", &open_detailed_modal, ImGuiWindowFlags_None)) {
        render_priest_panel_results(batch);
        ImGui::Spacing();
        if (warlock::WowButton("Close", ImVec2(100, 28))) {
            open_detailed_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Damage Breakdown
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Damage Breakdown");
    ImGui::Spacing();
    float avail_w = ImGui::GetContentRegionAvail().x;
    float label_offset = 120.0f;
    float bar_w = std::max(60.0f, avail_w - label_offset - 8.0f);
    render_priest_damage_breakdown_bars(batch, bar_w, label_offset);
}

} // namespace priest
