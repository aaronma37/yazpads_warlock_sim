#pragma once
#include "imgui.h"
#include "implot.h"
#include "rlImGui.h"
#include "raylib.h"
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

namespace warlock {

// ============================================================================
// Classic WoW Color Palette Constants
// ============================================================================
namespace wow_colors {
    // Core Typography Colors
    constexpr ImVec4 Gold          = ImVec4(1.00f, 0.82f, 0.00f, 1.00f); // #FFD100 Classic WoW Gold
    constexpr ImVec4 YellowHighlight = ImVec4(1.00f, 0.92f, 0.40f, 1.00f); // #FFEA66 Hover / Highlight
    constexpr ImVec4 ParchmentText = ImVec4(0.92f, 0.85f, 0.72f, 1.00f); // #EBD9B8 Readable warm body text
    constexpr ImVec4 White         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF Pure White
    constexpr ImVec4 GrayMuted     = ImVec4(0.55f, 0.53f, 0.50f, 1.00f); // #8C8780 Disabled / Muted Text
    constexpr ImVec4 GreenBuff     = ImVec4(0.12f, 0.90f, 0.12f, 1.00f); // #1FE61F Positive / Buff
    constexpr ImVec4 RedDebuff     = ImVec4(0.95f, 0.20f, 0.20f, 1.00f); // #F23333 Danger / Warning
    constexpr ImVec4 BlueMana      = ImVec4(0.20f, 0.55f, 1.00f, 1.00f); // #338CFF Mana / Resource
    constexpr ImVec4 OrangeWarning = ImVec4(1.00f, 0.60f, 0.10f, 1.00f); // #FF991A

    // Item Quality Colors
    constexpr ImVec4 QualityPoor      = ImVec4(0.62f, 0.62f, 0.62f, 1.00f); // #9D9D9D Poor (Gray)
    constexpr ImVec4 QualityCommon    = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF Common (White)
    constexpr ImVec4 QualityUncommon  = ImVec4(0.12f, 1.00f, 0.00f, 1.00f); // #1EFF00 Uncommon (Green)
    constexpr ImVec4 QualityRare      = ImVec4(0.00f, 0.44f, 0.87f, 1.00f); // #0070DD Rare (Blue)
    constexpr ImVec4 QualityEpic      = ImVec4(0.64f, 0.21f, 0.93f, 1.00f); // #A335EE Epic (Purple)
    constexpr ImVec4 QualityLegendary = ImVec4(1.00f, 0.50f, 0.00f, 1.00f); // #FF8000 Legendary (Orange)
    constexpr ImVec4 QualityArtifact  = ImVec4(0.90f, 0.80f, 0.50f, 1.00f); // #E6CC80 Artifact (Gold/Light Yellow)

    // Class Colors
    constexpr ImVec4 ClassWarlock     = ImVec4(0.53f, 0.53f, 0.93f, 1.00f); // #8788EE Warlock Purple
    constexpr ImVec4 ClassPriest      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF Priest White
    constexpr ImVec4 ClassMage        = ImVec4(0.41f, 0.80f, 0.94f, 1.00f); // #69CCF0 Mage Cyan
}

// ============================================================================
// Font Storage & Management
// ============================================================================
struct WowFonts {
    ImFont* Regular = nullptr;    // Primary UI (14px) - Friz Quadrata / Marcellus
    ImFont* Header  = nullptr;    // Window & Section Headers (18px) - Friz Quadrata
    ImFont* Small   = nullptr;    // Subtext, Tooltips, Inset Labels (12px)
    ImFont* Flavor  = nullptr;    // Story / Flavor / Lore Text (15px) - Morpheus / MedievalSharp
    ImFont* Combat  = nullptr;    // Combat & DPS Numbers (16px) - Skurri / Cinzel-Bold
};

inline WowFonts g_wow_fonts;

inline std::string find_font_file(const std::vector<std::string>& filenames) {
    namespace fs = std::filesystem;
    std::vector<std::string> search_dirs = {
        "assets/fonts",
        "/assets/fonts",
        "../assets/fonts"
    };

    const char* app_dir = GetApplicationDirectory();
    if (app_dir && app_dir[0] != '\0') {
        search_dirs.push_back(std::string(app_dir) + "assets/fonts");
        search_dirs.push_back(std::string(app_dir) + "../assets/fonts");
    }

    for (const auto& fname : filenames) {
        for (const auto& dir : search_dirs) {
            std::string full_path = dir + "/" + fname;
            if (fs::exists(full_path) && fs::is_regular_file(full_path) && fs::file_size(full_path) > 1024) {
                return full_path;
            }
        }
    }
    return "";
}

inline void load_wow_fonts() {
    ImGuiIO& io = ImGui::GetIO();

    // Look for authentic Friz Quadrata or bundled fallback
    std::string friz_path = find_font_file({
        "FRIZQT__.TTF", "FRIZQT__.ttf", "frizqt__.ttf", "Friz-Quadrata-TT.ttf", 
        "Spectral-Bold.ttf", "Spectral-Regular.ttf", "Cinzel-Bold.ttf"
    });

    std::string morpheus_path = find_font_file({
        "MORPHEUS.TTF", "MORPHEUS.ttf", "morpheus.ttf",
        "MedievalSharp-Regular.ttf", "Fondamento-Regular.ttf", "Almendra-Regular.ttf"
    });

    std::string skurri_path = find_font_file({
        "SKURRI.TTF", "skurri.ttf", "SKURRI.ttf",
        "Spectral-Bold.ttf", "Cinzel-Bold.ttf", "Almendra-Bold.ttf"
    });

    std::string small_path = find_font_file({
        "ARIALN.TTF", "arialn.ttf", "Spectral-Regular.ttf", "Spectral-Bold.ttf", "FRIZQT__.TTF"
    });

    ImFontConfig font_cfg;
    font_cfg.PixelSnapH = true;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;

    if (!friz_path.empty()) {
        std::cout << "[WoW UI] Loading primary font from: " << friz_path << std::endl;
        g_wow_fonts.Regular = io.Fonts->AddFontFromFileTTF(friz_path.c_str(), 16.5f, &font_cfg);
        g_wow_fonts.Header  = io.Fonts->AddFontFromFileTTF(friz_path.c_str(), 21.0f, &font_cfg);
    } else {
        std::cout << "[WoW UI] Using default font for Regular/Header" << std::endl;
        g_wow_fonts.Regular = io.Fonts->AddFontDefault(&font_cfg);
        g_wow_fonts.Header  = g_wow_fonts.Regular;
    }

    if (!small_path.empty()) {
        g_wow_fonts.Small = io.Fonts->AddFontFromFileTTF(small_path.c_str(), 14.0f, &font_cfg);
    } else {
        g_wow_fonts.Small = g_wow_fonts.Regular;
    }

    if (!morpheus_path.empty()) {
        std::cout << "[WoW UI] Loading flavor/morpheus font from: " << morpheus_path << std::endl;
        g_wow_fonts.Flavor = io.Fonts->AddFontFromFileTTF(morpheus_path.c_str(), 17.0f, &font_cfg);
    } else {
        g_wow_fonts.Flavor = g_wow_fonts.Regular;
    }

    if (!skurri_path.empty()) {
        std::cout << "[WoW UI] Loading combat/skurri font from: " << skurri_path << std::endl;
        g_wow_fonts.Combat = io.Fonts->AddFontFromFileTTF(skurri_path.c_str(), 19.0f, &font_cfg);
    } else {
        g_wow_fonts.Combat = g_wow_fonts.Header;
    }

    // Set default ImGui font
    io.FontDefault = g_wow_fonts.Regular;
}

// ============================================================================
// Classic WoW Skeuomorphic Theme
// ============================================================================
inline void apply_wow_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Classic WoW frame geometry (slightly rounded stone frames, bevels)
    style.WindowRounding    = 4.0f;
    style.ChildRounding     = 3.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 6.0f;   // Round brass thumb knob
    style.TabRounding       = 4.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.5f;

    style.WindowPadding     = ImVec2(12.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize     = 16.0f;
    style.GrabMinSize       = 16.0f;  // Wider grab for brass knob visibility

    // --- Classic WoW Skeuomorphic Color Palette ---
    // Text
    colors[ImGuiCol_Text]                  = wow_colors::ParchmentText;
    colors[ImGuiCol_TextDisabled]          = wow_colors::GrayMuted;

    // Windows & Containers (Dark Stone / Charcoal Slate Background)
    colors[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.07f, 0.06f, 0.98f); // Dark Charcoal Stone
    colors[ImGuiCol_ChildBg]               = ImVec4(0.05f, 0.04f, 0.04f, 0.75f); // Inset Stone Well
    colors[ImGuiCol_PopupBg]               = ImVec4(0.07f, 0.06f, 0.05f, 0.98f);

    // Borders (Antique Brass / Stone Edges)
    colors[ImGuiCol_Border]                = ImVec4(0.38f, 0.30f, 0.18f, 0.85f); // Brass / Stone Border
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);

    // Widget Frames (Deep Dark Stone Trough / Carved Inset for Sliders & Inputs)
    colors[ImGuiCol_FrameBg]               = ImVec4(0.08f, 0.07f, 0.05f, 1.00f); // Dark brown stone trough
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.14f, 0.11f, 0.08f, 1.00f); // Warmer on hover
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.18f, 0.14f, 0.10f, 1.00f); // Active warm glow

    // Title Bars (WoW Dialog Wooden/Stone Header with Gold Text)
    colors[ImGuiCol_TitleBg]               = ImVec4(0.12f, 0.09f, 0.07f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.22f, 0.16f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.08f, 0.06f, 0.05f, 0.80f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.10f, 0.08f, 1.00f);

    // Scrollbars (Stone track with Brass Thumb)
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.06f, 0.05f, 0.04f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.36f, 0.28f, 0.16f, 1.00f); // Brass Grab
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.52f, 0.40f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.68f, 0.52f, 0.28f, 1.00f);

    // Interactive Controls (Checkmark & Slider Grabs — Brass Thumb Knob #B8860B)
    colors[ImGuiCol_CheckMark]             = wow_colors::Gold;
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.72f, 0.53f, 0.04f, 1.00f); // Brass knob #B8860B
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.82f, 0.00f, 1.00f); // Bright gold when dragging

    // Classic Red/Brass Panel Buttons
    colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.18f, 0.12f, 1.00f); // Bronze / Dark Bevel
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.42f, 0.28f, 0.16f, 1.00f); // Warm Bevel Glow
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.18f, 0.12f, 0.08f, 1.00f); // Pressed Inset

    // Headers & Collapsing Headers
    colors[ImGuiCol_Header]                = ImVec4(0.25f, 0.18f, 0.11f, 0.90f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.38f, 0.28f, 0.16f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.48f, 0.35f, 0.20f, 1.00f);

    // Separators (Etched Brass Line)
    colors[ImGuiCol_Separator]             = ImVec4(0.35f, 0.28f, 0.16f, 0.80f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.60f, 0.48f, 0.26f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = wow_colors::Gold;

    // Resize Grips
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.35f, 0.28f, 0.16f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.58f, 0.46f, 0.26f, 0.80f);
    colors[ImGuiCol_ResizeGripActive]      = wow_colors::Gold;

    // Tabs (Stone Arch Tabs)
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.12f, 0.09f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.32f, 0.24f, 0.15f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.26f, 0.19f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.12f, 0.09f, 0.07f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.20f, 0.15f, 0.10f, 1.00f);

    // Plots & Tables
    colors[ImGuiCol_PlotLines]             = wow_colors::Gold;
    colors[ImGuiCol_PlotLinesHovered]      = wow_colors::YellowHighlight;
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.65f, 0.45f, 0.18f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = wow_colors::Gold;
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.16f, 0.12f, 0.09f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.35f, 0.28f, 0.16f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22f, 0.18f, 0.12f, 0.80f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 0.90f, 0.60f, 0.03f);

    // Setup ImPlot style (Dark Parchment & Gold)
    ImPlotStyle& pstyle = ImPlot::GetStyle();
    pstyle.Colors[ImPlotCol_FrameBg]       = ImVec4(0.07f, 0.06f, 0.05f, 0.95f);
    pstyle.Colors[ImPlotCol_PlotBg]        = ImVec4(0.04f, 0.03f, 0.03f, 0.98f);
    pstyle.Colors[ImPlotCol_PlotBorder]    = ImVec4(0.35f, 0.28f, 0.16f, 0.80f);
    pstyle.Colors[ImPlotCol_LegendBg]      = ImVec4(0.09f, 0.07f, 0.06f, 0.90f);
    pstyle.Colors[ImPlotCol_LegendBorder]  = ImVec4(0.35f, 0.28f, 0.16f, 0.70f);
    pstyle.Colors[ImPlotCol_LegendText]    = wow_colors::ParchmentText;
}

// Backward compatibility alias
inline void apply_warlock_theme() {
    apply_wow_theme();
}

} // namespace warlock
