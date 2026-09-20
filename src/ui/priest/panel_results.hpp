#pragma once
#include "imgui.h"
#include "implot.h"
#include "src/ui/common/asset_manager.hpp"
#include "src/sim/priest/parallel_runner.hpp"
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace priest {

inline void render_priest_panel_results(const BatchSimResult& batch) {
    if (batch.total_iterations == 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No simulation results yet. Click '>>> RUN PRIEST DES SIMULATION <<<' above to begin.");
        return;
    }

    // Top Summary Metric Cards (Exact Warlock 4-Column Layout)
    ImGui::Columns(4, "PriestResultsSummaryCols", false);

    ImGui::TextDisabled("MEAN DPS");
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1f", batch.mean_dps);
    ImGui::TextDisabled("+/- %.1f std dev", batch.std_dev_dps);
    ImGui::NextColumn();

    ImGui::TextDisabled("P50 MEDIAN (P5 - P95)");
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%.1f", batch.p50_dps);
    ImGui::TextDisabled("[%.1f - %.1f]", batch.p5_dps, batch.p95_dps);
    ImGui::NextColumn();

    ImGui::TextDisabled("SHADOW WEAVING PROCS");
    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "%.1f", batch.mean_sw_weaving_procs);
    ImGui::TextDisabled("Mean Procs / Fight");
    ImGui::NextColumn();

    ImGui::TextDisabled("MANA SPENT / GAINED");
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.0f / %.0f", batch.mean_mana_spent, batch.mean_mana_gained);
    ImGui::TextDisabled("Fight Mana Averages");
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Separator();

    if (ImGui::BeginTabBar("PriestResultsTabBar")) {
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

                if (ImPlot::BeginPlot("Sample Fight Timeline##Priest", ImVec2(-1, 320))) {
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
            ImGui::Text("Spell Damage Contribution (%% of Total Damage & Breakdown):");
            ImGui::Separator();

            struct PriestSpellRow {
                const char* name;
                const char* icon_name;
                double pct;
                SpellID id;
            };

            std::vector<PriestSpellRow> rows = {
                { "Shadow Word: Pain", "spell_shadow_shadowwordpain", batch.pct_sw_pain, SpellID::SHADOW_WORD_PAIN },
                { "Mind Flay", "spell_shadow_siphonmana", batch.pct_mind_flay, SpellID::MIND_FLAY },
                { "Mind Blast", "spell_shadow_unholyfrenzy", batch.pct_mind_blast, SpellID::MIND_BLAST },
                { "Shadow Word: Death", "spell_shadow_demonicfortitude", batch.pct_sw_death, SpellID::SHADOW_WORD_DEATH },
                { "Devouring Plague", "spell_shadow_devouringplague", batch.pct_devouring_plague, SpellID::DEVOURING_PLAGUE },
                { "Smite", "spell_holy_holysmite", batch.pct_smite, SpellID::SMITE },
                { "Holy Fire", "spell_holy_searinglight", batch.pct_holy_fire, SpellID::HOLY_FIRE },
            };

            // Sort descending by share %
            std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
                return a.pct > b.pct;
            });

            if (ImGui::BeginTable("PriestFullDmgBreakdown", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Spell / Ability", ImGuiTableColumnFlags_WidthFixed, 220);
                ImGui::TableSetupColumn("Damage Share (%)", ImGuiTableColumnFlags_WidthFixed, 130);
                ImGui::TableSetupColumn("Visual Contribution", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Mean Casts", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("Mean Hits", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("Crit %", ImGuiTableColumnFlags_WidthFixed, 90);
                ImGui::TableHeadersRow();

                for (const auto& r : rows) {
                    if (r.pct <= 0.0001) continue;

                    ImGui::TableNextRow();

                    // Col 0: Icon + Name
                    ImGui::TableSetColumnIndex(0);
                    Texture2D icon = warlock::AssetManager::get().get_icon(r.icon_name);
                    if (icon.id > 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
                        ImGui::SameLine(0, 6);
                    }
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.70f, 1.0f), "%s", r.name);

                    // Col 1: Share %
                    ImGui::TableSetColumnIndex(1);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.2f%%", r.pct * 100.0);

                    // Col 2: Progress Bar
                    ImGui::TableSetColumnIndex(2);
                    ImGui::ProgressBar(static_cast<float>(r.pct), ImVec2(-1, 16));

                    // Spell stats if recorded
                    size_t idx = static_cast<size_t>(r.id);
                    if (idx < batch.spell_stats.size()) {
                        const auto& st = batch.spell_stats[idx];
                        ImGui::TableSetColumnIndex(3);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%.1f", st.mean_casts);

                        ImGui::TableSetColumnIndex(4);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%.1f", st.mean_hits);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::AlignTextToFramePadding();
                        double crit_pct = (st.mean_hits > 0.0) ? (st.mean_crits / st.mean_hits * 100.0) : 0.0;
                        ImGui::Text("%.1f%%", crit_pct);
                    } else {
                        ImGui::TableSetColumnIndex(3); ImGui::Text("-");
                        ImGui::TableSetColumnIndex(4); ImGui::Text("-");
                        ImGui::TableSetColumnIndex(5); ImGui::Text("-");
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        // Tab 4: Observed Spell Cast Sequence
        if (ImGui::BeginTabItem("Observed Spell Cast Sequence")) {
            const auto& seq = batch.sample_timeline.cast_sequence;
            if (seq.empty()) {
                ImGui::TextDisabled("No sample cast sequence available. Run a simulation to generate observed sequence.");
            } else {
                int opener_count = (int)std::min(seq.size(), (size_t)16);
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Flow (First %d Spells Cast):", opener_count);
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
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Complete Cast History (%zu total casts in sample fight):", seq.size());
                ImGui::SameLine();

                auto generate_priest_cast_history_text = [&seq, &batch]() -> std::string {
                    std::ostringstream ss;
                    ss << "========================================================================================\n";
                    ss << "                       OBSERVED SPELL CAST SEQUENCE & COMBAT HISTORY                    \n";
                    ss << "========================================================================================\n";
                    ss << "Mean DPS: " << std::fixed << std::setprecision(1) << batch.mean_dps 
                       << " | Total Casts: " << seq.size() << "\n\n";
                    ss << std::left << std::setw(5)  << "#"
                       << std::setw(10) << "Time (s)"
                       << std::setw(24) << "Spell"
                       << std::setw(12) << "Cast Time"
                       << std::setw(18) << "Result / Dmg"
                       << "Trigger / Role\n";
                    ss << "----------------------------------------------------------------------------------------\n";

                    for (size_t i = 0; i < seq.size(); ++i) {
                        const auto& cast = seq[i];
                        std::string cast_str = (cast.cast_time > 0.0) ? (std::to_string(cast.cast_time).substr(0, 3) + "s") : "Instant";
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

                        ss << std::left << std::setw(5)  << (i + 1)
                           << std::setw(10) << time_buf.str()
                           << std::setw(24) << spell_id_to_string(cast.spell_id)
                           << std::setw(12) << cast_str
                           << std::setw(18) << res_str
                           << cast.tag << "\n";
                    }
                    ss << "========================================================================================\n";
                    return ss.str();
                };

                static float export_feedback_timer = 0.0f;
                static std::string export_feedback_msg = "";

                if (ImGui::Button("📋 Copy Cast History to Clipboard##Priest")) {
                    std::string text = generate_priest_cast_history_text();
                    ImGui::SetClipboardText(text.c_str());
                    export_feedback_msg = "Copied to clipboard!";
                    export_feedback_timer = 3.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("💾 Export to Text File (priest_cast_sequence.txt)##Priest")) {
                    std::string text = generate_priest_cast_history_text();
                    std::ofstream out("priest_cast_sequence.txt");
                    if (out.is_open()) {
                        out << text;
                        out.close();
                        export_feedback_msg = "Saved to priest_cast_sequence.txt!";
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

                if (ImGui::BeginTable("PriestAllCastsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(-1, 240))) {
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

                        // Col 3: Cast Time
                        ImGui::TableSetColumnIndex(3);
                        if (cast.cast_time > 0.0) {
                            ImGui::Text("%.1fs", cast.cast_time);
                        } else {
                            ImGui::TextDisabled("Instant");
                        }

                        // Col 4: Result / Damage
                        ImGui::TableSetColumnIndex(4);
                        if (cast.damage > 0.0) {
                            if (cast.is_crit) {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%.0f (CRIT)", cast.damage);
                            } else {
                                ImGui::Text("%.0f", cast.damage);
                            }
                        } else if (cast.is_miss) {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "MISS");
                        } else {
                            ImGui::TextDisabled("-");
                        }

                        // Col 5: Tag
                        ImGui::TableSetColumnIndex(5);
                        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", cast.tag.c_str());
                    }

                    ImGui::EndTable();
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

} // namespace priest
