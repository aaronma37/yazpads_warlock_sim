#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "asset_manager.hpp"
#include "frame_geometry.hpp"
#include "ui_theme.hpp"
#include <algorithm>
#include <string>

namespace warlock {

struct SliceBorder {
    float left = 8.0f;
    float top = 8.0f;
    float right = 8.0f;
    float bottom = 8.0f;

    SliceBorder() = default;
    SliceBorder(float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    SliceBorder(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
};

// ============================================================================
// Core 9-Slice Renderer
// ============================================================================
inline void DrawNineSlice(
    ImDrawList* drawList,
    ImTextureID texture,
    ImVec2 pMin,
    ImVec2 pMax,
    SliceBorder border,
    ImVec2 uvMin = ImVec2(0.0f, 0.0f),
    ImVec2 uvMax = ImVec2(1.0f, 1.0f),
    ImVec2 texSize = ImVec2(0.0f, 0.0f),
    ImU32 col = IM_COL32_WHITE
) {
    if (!drawList || !texture) return;

    float destW = pMax.x - pMin.x;
    float destH = pMax.y - pMin.y;
    if (destW <= 0.0f || destH <= 0.0f) return;

    // Constrain corner sizes if destination rectangle is smaller than corners
    float scaleX = (destW < (border.left + border.right)) ? (destW / (border.left + border.right)) : 1.0f;
    float scaleY = (destH < (border.top + border.bottom)) ? (destH / (border.top + border.bottom)) : 1.0f;

    float bL = border.left * scaleX;
    float bR = border.right * scaleX;
    float bT = border.top * scaleY;
    float bB = border.bottom * scaleY;

    // Screen positions
    float x0 = pMin.x;
    float x1 = pMin.x + bL;
    float x2 = pMax.x - bR;
    float x3 = pMax.x;

    float y0 = pMin.y;
    float y1 = pMin.y + bT;
    float y2 = pMax.y - bB;
    float y3 = pMax.y;

    // Compute UV splits
    float uvW = uvMax.x - uvMin.x;
    float uvH = uvMax.y - uvMin.y;

    float uvL, uvR, uvT, uvB;
    if (texSize.x > 0.0f && texSize.y > 0.0f) {
        uvL = uvMin.x + (border.left / texSize.x) * uvW;
        uvR = uvMax.x - (border.right / texSize.x) * uvW;
        uvT = uvMin.y + (border.top / texSize.y) * uvH;
        uvB = uvMax.y - (border.bottom / texSize.y) * uvH;
    } else {
        // Default 1/3 split if texture dimensions not passed
        uvL = uvMin.x + 0.333f * uvW;
        uvR = uvMax.x - 0.333f * uvW;
        uvT = uvMin.y + 0.333f * uvH;
        uvB = uvMax.y - 0.333f * uvH;
    }

    // 1. Top-Left Corner
    drawList->AddImage(texture, ImVec2(x0, y0), ImVec2(x1, y1), ImVec2(uvMin.x, uvMin.y), ImVec2(uvL, uvT), col);
    // 2. Top Edge
    if (x2 > x1)
        drawList->AddImage(texture, ImVec2(x1, y0), ImVec2(x2, y1), ImVec2(uvL, uvMin.y), ImVec2(uvR, uvT), col);
    // 3. Top-Right Corner
    drawList->AddImage(texture, ImVec2(x2, y0), ImVec2(x3, y1), ImVec2(uvR, uvMin.y), ImVec2(uvMax.x, uvT), col);

    // 4. Middle Left Edge
    if (y2 > y1)
        drawList->AddImage(texture, ImVec2(x0, y1), ImVec2(x1, y2), ImVec2(uvMin.x, uvT), ImVec2(uvL, uvB), col);
    // 5. Center Fill
    if (x2 > x1 && y2 > y1)
        drawList->AddImage(texture, ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(uvL, uvT), ImVec2(uvR, uvB), col);
    // 6. Middle Right Edge
    if (y2 > y1)
        drawList->AddImage(texture, ImVec2(x2, y1), ImVec2(x3, y2), ImVec2(uvR, uvT), ImVec2(uvMax.x, uvB), col);

    // 7. Bottom-Left Corner
    drawList->AddImage(texture, ImVec2(x0, y2), ImVec2(x1, y3), ImVec2(uvMin.x, uvB), ImVec2(uvL, uvMax.y), col);
    // 8. Bottom Edge
    if (x2 > x1)
        drawList->AddImage(texture, ImVec2(x1, y2), ImVec2(x2, y3), ImVec2(uvL, uvB), ImVec2(uvR, uvMax.y), col);
    // 9. Bottom-Right Corner
    drawList->AddImage(texture, ImVec2(x2, y2), ImVec2(x3, y3), ImVec2(uvR, uvB), ImVec2(uvMax.x, uvMax.y), col);
}

// ============================================================================
// 3-Slice Horizontal Renderer (Classic WoW Action & Panel Buttons)
// ============================================================================
inline void DrawThreeSliceHorizontal(
    ImDrawList* drawList,
    ImTextureID texture,
    ImVec2 pMin,
    ImVec2 pMax,
    float capWidth = 12.0f,
    float texWidth = 128.0f,
    ImVec2 uvMin = ImVec2(0.0f, 0.0f),
    ImVec2 uvMax = ImVec2(1.0f, 1.0f),
    ImU32 col = IM_COL32_WHITE
) {
    if (!drawList || !texture) return;

    float destW = pMax.x - pMin.x;
    if (destW <= 0.0f) return;

    float cap = capWidth;
    if (destW < 2.0f * cap) {
        cap = destW * 0.5f;
    }

    float uvW = uvMax.x - uvMin.x;
    float uvCap = (texWidth > 0.0f) ? (capWidth / texWidth) * uvW : 0.10f * uvW;

    float x0 = pMin.x;
    float x1 = pMin.x + cap;
    float x2 = pMax.x - cap;
    float x3 = pMax.x;

    float uv0 = uvMin.x;
    float uv1 = uvMin.x + uvCap;
    float uv2 = uvMax.x - uvCap;
    float uv3 = uvMax.x;

    // Left Cap
    drawList->AddImage(texture, ImVec2(x0, pMin.y), ImVec2(x1, pMax.y), ImVec2(uv0, uvMin.y), ImVec2(uv1, uvMax.y), col);
    // Middle Stretch
    if (x2 > x1) {
        drawList->AddImage(texture, ImVec2(x1, pMin.y), ImVec2(x2, pMax.y), ImVec2(uv1, uvMin.y), ImVec2(uv2, uvMax.y), col);
    }
    // Right Cap
    drawList->AddImage(texture, ImVec2(x2, pMin.y), ImVec2(x3, pMax.y), ImVec2(uv2, uvMin.y), ImVec2(uv3, uvMax.y), col);
}

// ============================================================================
// 3-Slice Vertical Renderer (Scrollbars, Column Borders)
// ============================================================================
inline void DrawThreeSliceVertical(
    ImDrawList* drawList,
    ImTextureID texture,
    ImVec2 pMin,
    ImVec2 pMax,
    float capHeight = 12.0f,
    float texHeight = 128.0f,
    ImVec2 uvMin = ImVec2(0.0f, 0.0f),
    ImVec2 uvMax = ImVec2(1.0f, 1.0f),
    ImU32 col = IM_COL32_WHITE
) {
    if (!drawList || !texture) return;

    float destH = pMax.y - pMin.y;
    if (destH <= 0.0f) return;

    float cap = capHeight;
    if (destH < 2.0f * cap) {
        cap = destH * 0.5f;
    }

    float uvH = uvMax.y - uvMin.y;
    float uvCap = (texHeight > 0.0f) ? (capHeight / texHeight) * uvH : 0.10f * uvH;

    float y0 = pMin.y;
    float y1 = pMin.y + cap;
    float y2 = pMax.y - cap;
    float y3 = pMax.y;

    float uv0 = uvMin.y;
    float uv1 = uvMin.y + uvCap;
    float uv2 = uvMax.y - uvCap;
    float uv3 = uvMax.y;

    // Top Cap
    drawList->AddImage(texture, ImVec2(pMin.x, y0), ImVec2(pMax.x, y1), ImVec2(uvMin.x, uv0), ImVec2(uvMax.x, uv1), col);
    // Middle Stretch
    if (y2 > y1) {
        drawList->AddImage(texture, ImVec2(pMin.x, y1), ImVec2(pMax.x, y2), ImVec2(uvMin.x, uv1), ImVec2(uvMax.x, uv2), col);
    }
    // Bottom Cap
    drawList->AddImage(texture, ImVec2(pMin.x, y2), ImVec2(pMax.x, y3), ImVec2(uvMin.x, uv2), ImVec2(uvMax.x, uv3), col);
}

// ============================================================================
// Seamless Tiled Texture Renderer (Parchment & Marble Backgrounds)
// ============================================================================
inline void DrawTiledTexture(
    ImDrawList* drawList,
    ImTextureID texture,
    ImVec2 pMin,
    ImVec2 pMax,
    ImVec2 tileSize = ImVec2(64.0f, 64.0f),
    ImU32 col = IM_COL32_WHITE
) {
    if (!drawList || !texture) return;

    float w = pMax.x - pMin.x;
    float h = pMax.y - pMin.y;
    if (w <= 0.0f || h <= 0.0f) return;

    float uvMaxX = w / (tileSize.x > 0.0f ? tileSize.x : 64.0f);
    float uvMaxY = h / (tileSize.y > 0.0f ? tileSize.y : 64.0f);

    drawList->AddImage(texture, pMin, pMax, ImVec2(0.0f, 0.0f), ImVec2(uvMaxX, uvMaxY), col);
}

// ============================================================================
// Classic WoW Action / Panel Button
// ============================================================================
inline bool WowButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0), bool enabled = true) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 3.0f, label_size.y + style.FramePadding.y * 2.5f);
    if (size.y < 24.0f) size.y = 24.0f;
    if (size.x < 50.0f) size.x = 50.0f;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    bool pressed = false;
    if (enabled) {
        pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    }

    ImDrawList* drawList = window->DrawList;

    // Determine textures
    const Texture2D& texUp = AssetManager::get().get_texture("UI-Panel-Button-Up");
    const Texture2D& texDown = AssetManager::get().get_texture("UI-Panel-Button-Down");
    const Texture2D& texHighlight = AssetManager::get().get_texture("UI-Panel-Button-Highlight");
    const Texture2D& texDisabled = AssetManager::get().get_texture("UI-Panel-Button-Disabled");

    Texture2D baseTex = texUp;
    if (!enabled) {
        baseTex = (texDisabled.id > 0) ? texDisabled : texUp;
    } else if (held) {
        baseTex = (texDown.id > 0) ? texDown : texUp;
    }

    // Blizzard UI-Panel-Button active sub-rect within 128x32 texture is [1..78] x [1..21]
    const ImVec2 kBtnUvMin(0.0078125f, 0.03125f);
    const ImVec2 kBtnUvMax(0.609375f, 0.65625f);
    const float kBtnActiveWidth = 78.0f;
    const float kBtnCapWidth = 12.0f;

    if (baseTex.id > 0) {
        DrawThreeSliceHorizontal(drawList, (ImTextureID)baseTex.id, bb.Min, bb.Max, kBtnCapWidth, kBtnActiveWidth, kBtnUvMin, kBtnUvMax);
    } else {
        ImU32 col = held ? IM_COL32(50, 35, 20, 255) : (hovered ? IM_COL32(95, 70, 40, 255) : IM_COL32(65, 45, 25, 255));
        drawList->AddRectFilled(bb.Min, bb.Max, col, 3.0f);
        drawList->AddRect(bb.Min, bb.Max, IM_COL32(140, 110, 60, 255), 3.0f, 0, 1.2f);
    }

    // Hover additive glow overlay
    if (hovered && enabled && texHighlight.id > 0) {
        DrawThreeSliceHorizontal(drawList, (ImTextureID)texHighlight.id, bb.Min, bb.Max, kBtnCapWidth, kBtnActiveWidth, kBtnUvMin, kBtnUvMax, IM_COL32(255, 255, 255, 180));
    }

    // Text rendering (Gold / Yellow / Parchment) with +1px displacement on click
    ImVec2 text_offset = (held && enabled) ? ImVec2(1.0f, 1.0f) : ImVec2(0.0f, 0.0f);
    ImVec2 text_pos = ImVec2(
        bb.Min.x + (size.x - label_size.x) * 0.5f + text_offset.x,
        bb.Min.y + (size.y - label_size.y) * 0.5f + text_offset.y
    );

    ImU32 text_col = !enabled ? IM_COL32(128, 128, 128, 255) : (hovered ? IM_COL32(255, 240, 120, 255) : IM_COL32(255, 209, 0, 255));
    
    // Draw text with subtle shadow
    drawList->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), IM_COL32(0, 0, 0, 220), label);
    drawList->AddText(text_pos, text_col, label);

    return pressed;
}

// ============================================================================
// Classic WoW Wide Red Dialog Button (e.g. "Copy Build to Clipboard")
// ============================================================================
inline bool WowRedDialogButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0), bool enabled = true) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 4.0f, 26.0f);
    if (size.y < 24.0f) size.y = 24.0f;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered = false, held = false;
    bool pressed = false;
    if (enabled) {
        pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    }

    ImDrawList* drawList = window->DrawList;

    const Texture2D& texUp = AssetManager::get().get_texture("UI-DialogBox-Button-Gold-Up");
    const Texture2D& texDown = AssetManager::get().get_texture("UI-DialogBox-Button-Gold-Down");
    const Texture2D& texHighlight = AssetManager::get().get_texture("UI-DialogBox-Button-Highlight");
    const Texture2D& fallback = AssetManager::get().get_fallback();

    Texture2D baseTex = (held && enabled) ? texDown : texUp;
    if (baseTex.id > 0 && baseTex.id != fallback.id) {
        DrawThreeSliceHorizontal(drawList, (ImTextureID)baseTex.id, bb.Min, bb.Max, 18.0f, (float)baseTex.width);
        if (hovered && enabled && texHighlight.id > 0 && texHighlight.id != fallback.id) {
            DrawThreeSliceHorizontal(drawList, (ImTextureID)texHighlight.id, bb.Min, bb.Max, 18.0f, (float)texHighlight.width, ImVec2(0,0), ImVec2(1,1), IM_COL32(255, 255, 255, 180));
        }
    } else {
        // Red beveled fallback button
        ImU32 col = held ? IM_COL32(90, 15, 15, 255) : (hovered ? IM_COL32(160, 25, 25, 255) : IM_COL32(120, 20, 20, 255));
        drawList->AddRectFilled(bb.Min, bb.Max, col, 4.0f);
        drawList->AddRect(bb.Min, bb.Max, IM_COL32(200, 160, 50, 255), 4.0f, 0, 1.5f);
    }

    ImVec2 text_offset = (held && enabled) ? ImVec2(1.0f, 1.0f) : ImVec2(0.0f, 0.0f);
    ImVec2 text_pos = ImVec2(
        bb.Min.x + (size.x - label_size.x) * 0.5f + text_offset.x,
        bb.Min.y + (size.y - label_size.y) * 0.5f + text_offset.y
    );

    ImU32 text_col = !enabled ? IM_COL32(128, 128, 128, 255) : (hovered ? IM_COL32(255, 240, 150, 255) : IM_COL32(255, 209, 0, 255));
    drawList->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), IM_COL32(0, 0, 0, 220), label);
    drawList->AddText(text_pos, text_col, label);

    return pressed;
}

// ============================================================================
// Classic WoW Checkbox
// ============================================================================
inline bool WowCheckbox(const char* label, bool* v) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // Strip ## suffix for display label (ImGui ID separator)
    const char* label_display_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_display_end);
    const float square_sz = 18.0f;
    const ImVec2 pos = window->DC.CursorPos;
    const float text_offset_x = square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x : 0.0f);
    const ImRect total_bb(pos, ImVec2(pos.x + text_offset_x + label_size.x, pos.y + std::max(square_sz, label_size.y)));

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    bool hovered = false, held = false;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        *v = !(*v);
        ImGui::MarkItemEdited(id);
    }

    ImDrawList* drawList = window->DrawList;
    const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));

    const Texture2D& texUp = AssetManager::get().get_texture("UI-CheckBox-Up");
    const Texture2D& texDown = AssetManager::get().get_texture("UI-CheckBox-Down");
    const Texture2D& texHighlight = AssetManager::get().get_texture("UI-CheckBox-Highlight");
    const Texture2D& texCheck = AssetManager::get().get_texture("UI-CheckBox-Check");

    // Exact active UV coordinates inside 32x32 texture
    const ImVec2 kCheckUvMin(0.1250f, 0.15625f);
    const ImVec2 kCheckUvMax(0.84375f, 0.81250f);

    Texture2D frameTex = held ? texDown : texUp;
    if (frameTex.id > 0) {
        drawList->AddImage((ImTextureID)frameTex.id, check_bb.Min, check_bb.Max, kCheckUvMin, kCheckUvMax);
    } else {
        // Beveled inset stone well: dark fill, then multi-tone border for depth
        ImU32 fillCol = held ? IM_COL32(20, 16, 12, 255) : IM_COL32(30, 25, 20, 255);
        drawList->AddRectFilled(check_bb.Min, check_bb.Max, fillCol, 1.5f);

        // Outer bevel: dark shadow on top-left (inset effect), lighter on bottom-right
        ImVec2 tl = check_bb.Min;
        ImVec2 br = check_bb.Max;
        // Top edge (dark - sunken into stone)
        drawList->AddLine(ImVec2(tl.x, tl.y), ImVec2(br.x, tl.y), IM_COL32(15, 12, 8, 255), 1.5f);
        // Left edge (dark)
        drawList->AddLine(ImVec2(tl.x, tl.y), ImVec2(tl.x, br.y), IM_COL32(15, 12, 8, 255), 1.5f);
        // Bottom edge (lighter brass highlight - light catches the lip)
        drawList->AddLine(ImVec2(tl.x, br.y), ImVec2(br.x, br.y), IM_COL32(120, 95, 50, 255), 1.5f);
        // Right edge (lighter brass)
        drawList->AddLine(ImVec2(br.x, tl.y), ImVec2(br.x, br.y), IM_COL32(120, 95, 50, 255), 1.5f);

        // Inner bevel: secondary tone for depth
        drawList->AddLine(ImVec2(tl.x + 1, tl.y + 1), ImVec2(br.x - 1, tl.y + 1), IM_COL32(40, 32, 22, 255), 1.0f);
        drawList->AddLine(ImVec2(tl.x + 1, tl.y + 1), ImVec2(tl.x + 1, br.y - 1), IM_COL32(40, 32, 22, 255), 1.0f);
        drawList->AddLine(ImVec2(tl.x + 1, br.y - 1), ImVec2(br.x - 1, br.y - 1), IM_COL32(85, 68, 38, 255), 1.0f);
        drawList->AddLine(ImVec2(br.x - 1, tl.y + 1), ImVec2(br.x - 1, br.y - 1), IM_COL32(85, 68, 38, 255), 1.0f);
    }

    // Hover highlight glow
    if (hovered) {
        if (texHighlight.id > 0) {
            drawList->AddImage((ImTextureID)texHighlight.id, check_bb.Min, check_bb.Max, kCheckUvMin, kCheckUvMax, IM_COL32(255, 255, 255, 180));
        } else {
            // Warm gold glow on hover
            drawList->AddRect(
                ImVec2(check_bb.Min.x - 1, check_bb.Min.y - 1),
                ImVec2(check_bb.Max.x + 1, check_bb.Max.y + 1),
                IM_COL32(255, 209, 0, hovered ? 100 : 0), 1.5f, 0, 1.5f);
        }
    }

    // Checkmark: bright gold on dark inset
    if (*v) {
        if (texCheck.id > 0) {
            drawList->AddImage((ImTextureID)texCheck.id, check_bb.Min, check_bb.Max, ImVec2(0.125f, 0.15625f), ImVec2(0.875f, 0.8125f));
        } else {
            // Draw a gold X mark (classic WoW style) with shadow
            float pad = 4.0f;
            float x0 = check_bb.Min.x + pad, y0 = check_bb.Min.y + pad;
            float x1 = check_bb.Max.x - pad, y1 = check_bb.Max.y - pad;
            // Shadow
            drawList->AddLine(ImVec2(x0 + 1, y0 + 1), ImVec2(x1 + 1, y1 + 1), IM_COL32(0, 0, 0, 200), 2.5f);
            drawList->AddLine(ImVec2(x1 + 1, y0 + 1), ImVec2(x0 + 1, y1 + 1), IM_COL32(0, 0, 0, 200), 2.5f);
            // Gold X
            drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 209, 0, 255), 2.5f);
            drawList->AddLine(ImVec2(x1, y0), ImVec2(x0, y1), IM_COL32(255, 209, 0, 255), 2.5f);
        }
    }

    // Label text with shadow
    if (label_size.x > 0.0f) {
        ImVec2 text_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + (square_sz - label_size.y) * 0.5f);
        ImU32 text_col = hovered ? IM_COL32(255, 240, 120, 255) : IM_COL32(235, 217, 184, 255);
        // Text shadow for readability
        drawList->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), IM_COL32(0, 0, 0, 180), label, label_display_end);
        drawList->AddText(text_pos, text_col, label, label_display_end);
    }

    return pressed;
}

// ============================================================================
// Classic WoW Close Button ("Red X")
// ============================================================================
inline bool WowCloseButton(const char* str_id = "##close", const ImVec2& size = ImVec2(22.0f, 22.0f)) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(str_id);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* drawList = window->DrawList;

    const Texture2D& texUp = AssetManager::get().get_texture("UI-Panel-MinimizeButton-Up");
    const Texture2D& texDown = AssetManager::get().get_texture("UI-Panel-MinimizeButton-Down");
    const Texture2D& texHighlight = AssetManager::get().get_texture("UI-Panel-MinimizeButton-Highlight");

    const ImVec2 kCloseUvMin(0.1875f, 0.21875f);
    const ImVec2 kCloseUvMax(0.78125f, 0.78125f);

    Texture2D tex = held ? texDown : texUp;
    if (tex.id > 0) {
        drawList->AddImage((ImTextureID)tex.id, bb.Min, bb.Max, kCloseUvMin, kCloseUvMax);
        if (hovered && texHighlight.id > 0) {
            drawList->AddImage((ImTextureID)texHighlight.id, bb.Min, bb.Max, kCloseUvMin, kCloseUvMax, IM_COL32(255, 255, 255, 200));
        }
    } else {
        drawList->AddCircleFilled(ImVec2(bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f), size.x * 0.45f, IM_COL32(180, 30, 30, 255));
        drawList->AddCircle(ImVec2(bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f), size.x * 0.45f, IM_COL32(255, 209, 0, 255));
        drawList->AddText(ImVec2(bb.Min.x + 6.0f, bb.Min.y + 2.0f), IM_COL32(255, 255, 255, 255), "X");
    }

    return pressed;
}

// ============================================================================
// Classic WoW Bigger Button (UI-Panel-BiggerButton-Up / Down / Disabled)
// ============================================================================
inline bool WowBiggerButton(const char* str_id = "##bigger_btn", const ImVec2& size = ImVec2(22.0f, 22.0f), bool enabled = true) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(str_id);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    bool pressed = false;
    if (enabled) {
        pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    }

    ImDrawList* drawList = window->DrawList;

    const Texture2D& texUp = AssetManager::get().get_texture("UI-Panel-BiggerButton-Up");
    const Texture2D& texDown = AssetManager::get().get_texture("UI-Panel-BiggerButton-Down");
    const Texture2D& texDisabled = AssetManager::get().get_texture("UI-Panel-BiggerButton-Disabled");
    const Texture2D& fallback = AssetManager::get().get_fallback();

    Texture2D tex = texUp;
    if (!enabled) {
        tex = (texDisabled.id > 0 && texDisabled.id != fallback.id) ? texDisabled : texUp;
    } else if (held) {
        tex = (texDown.id > 0 && texDown.id != fallback.id) ? texDown : texUp;
    }

    if (tex.id > 0 && tex.id != fallback.id) {
        drawList->AddImage((ImTextureID)(uintptr_t)tex.id, bb.Min, bb.Max);
        if (hovered && enabled) {
            drawList->AddImage((ImTextureID)(uintptr_t)tex.id, bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 120));
        }
    } else {
        ImU32 bg = held ? IM_COL32(60, 45, 25, 255) : (hovered ? IM_COL32(95, 75, 45, 255) : IM_COL32(75, 55, 30, 255));
        drawList->AddRectFilled(bb.Min, bb.Max, bg, 3.0f);
        drawList->AddRect(bb.Min, bb.Max, IM_COL32(200, 160, 60, 255), 3.0f, 0, 1.0f);
    }

    return pressed;
}

// ============================================================================
// Classic WoW Section Header Bar
// ============================================================================
inline void DrawWowSectionHeader(const char* title, float height = 28.0f) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    float availW = ImGui::GetContentRegionAvail().x;
    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + availW, pos.y + height));

    ImGui::ItemSize(ImVec2(availW, height));
    if (!ImGui::ItemAdd(bb, window->GetID(title))) return;

    ImDrawList* drawList = window->DrawList;

    const Texture2D& headerTex = AssetManager::get().get_texture("UI-DialogBox-Header");
    if (headerTex.id > 0) {
        // Center the ornate header banner with exact Blizzard UVs
        const ImVec2 kHeaderUvMin(0.2265625f, 0.0f);
        const ImVec2 kHeaderUvMax(0.76953125f, 0.625f);
        float bannerW = std::min(availW, 200.0f);
        float bannerX = bb.Min.x + (availW - bannerW) * 0.5f;
        drawList->AddImage((ImTextureID)headerTex.id, ImVec2(bannerX, bb.Min.y), ImVec2(bannerX + bannerW, bb.Max.y), kHeaderUvMin, kHeaderUvMax);
    } else {
        drawList->AddRectFilled(bb.Min, bb.Max, IM_COL32(35, 25, 18, 220), 4.0f);
        drawList->AddRect(bb.Min, bb.Max, IM_COL32(140, 110, 60, 255), 4.0f);
    }

    // Title Text centered
    ImVec2 textSize = ImGui::CalcTextSize(title);
    ImVec2 textPos = ImVec2(bb.Min.x + (availW - textSize.x) * 0.5f, bb.Min.y + (height - textSize.y) * 0.5f);
    drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), title);
    drawList->AddText(textPos, IM_COL32(255, 209, 0, 255), title);
}

// ============================================================================
// WoW Carved Stone Trough Frame Bevel (for Sliders & Inputs)
// ============================================================================
inline void DrawWowFrameBevel(ImVec2 pMin, ImVec2 pMax) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList) return;

    // Inset bevel: dark top-left (sunken into stone), lighter bottom-right (lip catching light)
    drawList->AddLine(ImVec2(pMin.x, pMin.y), ImVec2(pMax.x, pMin.y), IM_COL32(10, 8, 5, 220), 1.0f);   // Top dark
    drawList->AddLine(ImVec2(pMin.x, pMin.y), ImVec2(pMin.x, pMax.y), IM_COL32(10, 8, 5, 220), 1.0f);   // Left dark
    drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMax.y), IM_COL32(95, 75, 42, 200), 1.0f); // Bottom brass
    drawList->AddLine(ImVec2(pMax.x, pMin.y), ImVec2(pMax.x, pMax.y), IM_COL32(95, 75, 42, 200), 1.0f); // Right brass
}

// ============================================================================
// WoW 3-Slice Texture Input Box Border (Common-Input-Border.PNG)
// Slices into Left (8px cap), Middle (stretch), Right (8px cap)
// ============================================================================
inline void DrawWowInputBorder(ImVec2 pMin, ImVec2 pMax) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList) return;

    AssetManager& assets = AssetManager::get();
    const Texture2D& tex = assets.get_texture("Common-Input-Border");
    const Texture2D& fallback = assets.get_fallback();

    if (tex.id > 0 && tex.id != fallback.id) {
        // Common-Input-Border is 128x32 with active content height 20px (UV Y: 0.0 -> 20.0/32.0 = 0.625)
        // Left cap = 8.0px, Right cap = 8.0px
        DrawThreeSliceHorizontal(
            drawList,
            (ImTextureID)(uintptr_t)tex.id,
            pMin,
            pMax,
            8.0f,
            128.0f,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 20.0f / 32.0f),
            IM_COL32_WHITE
        );
    } else {
        DrawWowFrameBevel(pMin, pMax);
    }
}

// Push/Pop style for WoW-themed input fields and sliders
inline void PushWowInputStyle() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));             // Pure White input text
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));          // Border drawn via 3-slice texture
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.05f, 0.04f, 0.70f));     // Deep dark trough
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.10f, 0.07f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.16f, 0.13f, 0.09f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.72f, 0.53f, 0.04f, 1.00f));  // Brass knob
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.82f, 0.0f, 1.0f)); // Gold active
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);
}

inline void PopWowInputStyle() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(7);
}

// Reset text baseline offset so vertical column groups and table cells align consistently
inline void WowResetTextBaseline() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window) {
        window->DC.CurrLineTextBaseOffset = 0.0f;
    }
}

// ============================================================================
// Convenience WoW Slider/Input Wrappers (3-slice Common-Input-Border)
// ============================================================================
inline bool WowSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::SliderInt(label, v, v_min, v_max, format, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

inline bool WowSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

inline bool WowSliderDouble(const char* label, double* v, double v_min, double v_max, const char* format = "%.0f", ImGuiSliderFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

inline bool WowInputInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::InputInt(label, v, 0, 0, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

inline bool WowInputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::InputDouble(label, v, step, step_fast, format, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

inline bool WowInputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::InputFloat(label, v, step, step_fast, format, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

inline bool WowInputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0) {
    PushWowInputStyle();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);
    bool changed = ImGui::InputText(label, buf, buf_size, flags);
    ImVec2 frame_min = ImGui::GetItemRectMin();
    ImVec2 frame_max = ImGui::GetItemRectMax();
    drawList->ChannelsSetCurrent(0);
    DrawWowInputBorder(frame_min, frame_max);
    drawList->ChannelsMerge();
    PopWowInputStyle();
    return changed;
}

// ============================================================================
// Classic WoW Authentic Casting Bar / Loading Bar / Progress Bar
//
// Uses authentic Blizzard textures:
//   - UI-CastingBar-Border-Small / UI-CastingBar-Border (3-slice brass frame)
//   - UI-StatusBar (horizontal gradient fill texture)
//   - UI-CastingBar-Spark (additive leading edge glow)
//   - UI-CastingBar-Flash-Small / UI-CastingBar-Flash (additive completion flash)
// ============================================================================
inline void WowProgressBar(
    float fraction,
    const ImVec2& size_arg = ImVec2(-1, 0),
    const char* overlay = nullptr,
    ImU32 bar_color = IM_COL32(255, 178, 0, 255),
    bool show_spark = true
) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    if (size_arg.y > 0.0f) {
        size.y = size_arg.y;
    } else if (size.y < 16.0f) {
        size.y = 16.0f;
    }
    if (size.y < 6.0f) size.y = 6.0f;
    if (size.x < 20.0f) size.x = 20.0f;

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, 0)) return;

    float f = std::clamp(fraction, 0.0f, 1.0f);
    ImDrawList* drawList = window->DrawList;

    AssetManager& assets = AssetManager::get();
    const Texture2D& fallback = assets.get_fallback();

    bool is_small = (size.y <= 28.0f);

    const Texture2D& borderSmallTex = assets.get_texture("UI-CastingBar-Border-Small");
    const Texture2D& borderLargeTex = assets.get_texture("UI-CastingBar-Border");
    const Texture2D& flashSmallTex  = assets.get_texture("UI-CastingBar-Flash-Small");
    const Texture2D& flashLargeTex  = assets.get_texture("UI-CastingBar-Flash");
    const Texture2D& sparkTex       = assets.get_texture("UI-CastingBar-Spark");
    const Texture2D& statusTex      = assets.get_texture("UI-StatusBar");

    const Texture2D& borderTex = is_small ? borderSmallTex : borderLargeTex;
    const Texture2D& flashTex  = is_small ? flashSmallTex : flashLargeTex;

    // Inner trough bounds (natural full height and width with balanced margins)
    float pad_top    = is_small ? std::max(2.5f, size.y * 0.16f) : std::max(5.0f, size.y * 0.22f);
    float pad_bottom = is_small ? std::max(2.5f, size.y * 0.18f) : std::max(5.0f, size.y * 0.22f);
    float pad_left   = is_small ? 5.0f : 10.0f;
    float pad_right  = is_small ? 5.0f : 10.0f;

    ImVec2 inner_min(bb.Min.x + pad_left, bb.Min.y + pad_top);
    ImVec2 inner_max(bb.Max.x - pad_right, bb.Max.y - pad_bottom);
    float inner_w = std::max(0.0f, inner_max.x - inner_min.x);
    float inner_h = std::max(0.0f, inner_max.y - inner_min.y);

    // 1. Dark recessed trough background with rounding
    if (inner_w > 0.0f && inner_h > 0.0f) {
        drawList->AddRectFilled(inner_min, inner_max, IM_COL32(14, 11, 8, 255), 2.0f);
        // Subtle top/left inner shadow
        drawList->AddLine(ImVec2(inner_min.x + 1.0f, inner_min.y + 1.0f), ImVec2(inner_max.x - 1.0f, inner_min.y + 1.0f), IM_COL32(5, 4, 3, 200), 1.0f);
        drawList->AddLine(ImVec2(inner_min.x + 1.0f, inner_min.y + 1.0f), ImVec2(inner_min.x + 1.0f, inner_max.y - 1.0f), IM_COL32(5, 4, 3, 200), 1.0f);
    }

    // 2. Bar Fill (UI-StatusBar texture smoothly stretched with NO tiling or solid fill)
    float fill_w = inner_w * f;
    if (fill_w > 0.5f && inner_h > 0.0f) {
        ImVec2 fill_max(inner_min.x + fill_w, inner_max.y);
        if (statusTex.id > 0 && statusTex.id != fallback.id) {
            drawList->AddImage(
                (ImTextureID)(uintptr_t)statusTex.id,
                inner_min,
                fill_max,
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 1.0f),
                bar_color
            );
        } else {
            drawList->AddRectFilled(inner_min, fill_max, bar_color, 2.0f);
        }
    }

    // 3. Spark (authentic leading-edge glow with transparent falloff)
    if (show_spark && f > 0.01f && f < 0.995f && sparkTex.id > 0 && sparkTex.id != fallback.id && inner_h >= 4.0f) {
        float spark_cx = inner_min.x + fill_w;
        float spark_cy = (inner_min.y + inner_max.y) * 0.5f;
        float spark_h  = inner_h * 2.2f;
        float spark_w  = spark_h * 0.75f;
        drawList->AddImage(
            (ImTextureID)(uintptr_t)sparkTex.id,
            ImVec2(spark_cx - spark_w * 0.5f, spark_cy - spark_h * 0.5f),
            ImVec2(spark_cx + spark_w * 0.5f, spark_cy + spark_h * 0.5f),
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f),
            IM_COL32(255, 255, 255, 240)
        );
    }

    // 4. WoW Casting Bar 3-Slice Border Overlay
    if (borderTex.id > 0 && borderTex.id != fallback.id) {
        if (is_small) {
            // UI-CastingBar-Border-Small: [28..227] x [22..43] inside 256x64
            const ImVec2 uv0(28.0f / 256.0f, 22.0f / 64.0f);
            const ImVec2 uv1(227.0f / 256.0f, 43.0f / 64.0f);
            DrawThreeSliceHorizontal(
                drawList,
                (ImTextureID)(uintptr_t)borderTex.id,
                bb.Min,
                bb.Max,
                10.0f,
                200.0f,
                uv0,
                uv1
            );
        } else {
            // UI-CastingBar-Border: [22..233] x [16..47] inside 256x64
            const ImVec2 uv0(22.0f / 256.0f, 16.0f / 64.0f);
            const ImVec2 uv1(233.0f / 256.0f, 47.0f / 64.0f);
            DrawThreeSliceHorizontal(
                drawList,
                (ImTextureID)(uintptr_t)borderTex.id,
                bb.Min,
                bb.Max,
                18.0f,
                212.0f,
                uv0,
                uv1
            );
        }
    } else {
        // Fallback procedural frame bevel
        drawList->AddRect(bb.Min, bb.Max, IM_COL32(140, 110, 60, 255), 2.0f, 0, 1.5f);
    }

    // 5. Completion Flash Glow Overlay (at 100%)
    if (f >= 0.999f && flashTex.id > 0 && flashTex.id != fallback.id) {
        if (is_small) {
            const ImVec2 uv0(21.0f / 256.0f, 14.0f / 64.0f);
            const ImVec2 uv1(234.0f / 256.0f, 50.0f / 64.0f);
            DrawThreeSliceHorizontal(
                drawList,
                (ImTextureID)(uintptr_t)flashTex.id,
                bb.Min,
                bb.Max,
                14.0f,
                214.0f,
                uv0,
                uv1,
                IM_COL32(255, 255, 255, 160)
            );
        } else {
            const ImVec2 uv0(19.0f / 256.0f, 13.0f / 64.0f);
            const ImVec2 uv1(236.0f / 256.0f, 50.0f / 64.0f);
            DrawThreeSliceHorizontal(
                drawList,
                (ImTextureID)(uintptr_t)flashTex.id,
                bb.Min,
                bb.Max,
                18.0f,
                218.0f,
                uv0,
                uv1,
                IM_COL32(255, 255, 255, 160)
            );
        }
    }

    // 6. Centered Overlay Text with 4-way Drop Shadow (Only if explicitly provided)
    if (overlay && overlay[0] != '\0' && size.y >= 12.0f) {
        ImVec2 text_sz = ImGui::CalcTextSize(overlay);
        ImVec2 text_pos(
            bb.Min.x + (size.x - text_sz.x) * 0.5f,
            bb.Min.y + (size.y - text_sz.y) * 0.5f
        );

        // 4-way dark drop shadow for high contrast readability
        drawList->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y), IM_COL32(0, 0, 0, 255), overlay);
        drawList->AddText(ImVec2(text_pos.x - 1.0f, text_pos.y), IM_COL32(0, 0, 0, 255), overlay);
        drawList->AddText(ImVec2(text_pos.x, text_pos.y + 1.0f), IM_COL32(0, 0, 0, 255), overlay);
        drawList->AddText(ImVec2(text_pos.x, text_pos.y - 1.0f), IM_COL32(0, 0, 0, 255), overlay);
        // Primary text
        drawList->AddText(text_pos, IM_COL32(255, 240, 150, 255), overlay);
    }
}

// ============================================================================
inline bool WowCollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // Strip ## suffix for display
    const char* label_display_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_display_end);

    const float height = 26.0f;
    const float arrowSize = 14.0f;
    const float filigreeWidth = 12.0f; // space for decorative brackets
    float availW = ImGui::GetContentRegionAvail().x;

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + availW, pos.y + height));

    // Use ImGui's tree node state management for open/close persistence
    ImGui::ItemSize(ImVec2(availW, height));
    if (!ImGui::ItemAdd(bb, id))
        return ImGui::TreeNodeUpdateNextOpen(id, flags);

    bool hovered = false, held = false;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    // Toggle open state on click
    bool is_open = ImGui::TreeNodeUpdateNextOpen(id, flags);
    if (pressed) {
        is_open = !is_open;
        window->DC.StateStorage->SetInt(id, is_open ? 1 : 0);
    }

    ImDrawList* drawList = window->DrawList;

    // --- Plus / Minus Button Indicator ---
    const float btnSize = 16.0f;
    float btnX = bb.Min.x + 2.0f;
    float btnY = bb.Min.y + (height - btnSize) * 0.5f;

    AssetManager& assets = AssetManager::get();
    const char* btnTexName = is_open ? (held ? "UI-MinusButton-Down" : "UI-MinusButton-Up")
                                     : (held ? "UI-PlusButton-Down" : "UI-PlusButton-Up");
    const Texture2D& btnTex = assets.get_texture(btnTexName);
    const Texture2D& fallbackTex = assets.get_fallback();

    if (btnTex.id > 0 && btnTex.id != fallbackTex.id) {
        drawList->AddImage(
            (ImTextureID)btnTex.id,
            ImVec2(btnX, btnY),
            ImVec2(btnX + btnSize, btnY + btnSize)
        );
    } else {
        // Fallback procedural +/- button
        ImU32 boxBg = held ? IM_COL32(20, 15, 10, 255) : IM_COL32(35, 25, 18, 255);
        drawList->AddRectFilled(ImVec2(btnX, btnY), ImVec2(btnX + btnSize, btnY + btnSize), boxBg, 2.0f);
        drawList->AddRect(ImVec2(btnX, btnY), ImVec2(btnX + btnSize, btnY + btnSize), IM_COL32(160, 130, 70, 255), 2.0f);
        float midX = btnX + btnSize * 0.5f;
        float midY = btnY + btnSize * 0.5f;
        drawList->AddLine(ImVec2(midX - 4.0f, midY), ImVec2(midX + 4.0f, midY), IM_COL32(255, 209, 0, 255), 1.5f);
        if (!is_open) {
            drawList->AddLine(ImVec2(midX, midY - 4.0f), ImVec2(midX, midY + 4.0f), IM_COL32(255, 209, 0, 255), 1.5f);
        }
    }

    // --- Title text (Classic WoW Gold with Shadow) ---
    float labelX = btnX + btnSize + 8.0f;
    float textY = bb.Min.y + (height - label_size.y) * 0.5f;

    ImU32 textCol = hovered ? IM_COL32(255, 235, 120, 255) : IM_COL32(255, 209, 0, 255);
    drawList->AddText(ImVec2(labelX + 1.0f, textY + 1.0f), IM_COL32(0, 0, 0, 240), label, label_display_end);
    drawList->AddText(ImVec2(labelX, textY), textCol, label, label_display_end);

    return is_open;
}

// ============================================================================
// High-Level WoW Skeuomorphic Background & Frame Helpers
// ============================================================================
inline void DrawWowDialogBorder(ImDrawList* drawList, ImVec2 pMin, ImVec2 pMax);

inline void DrawWowDialogBackdrop(
    ImDrawList* drawList,
    ImVec2 pMin,
    ImVec2 pMax,
    bool drawBorder = true
) {
    if (!drawList) return;

    AssetManager& assets = AssetManager::get();
    const Texture2D& guildParchment = assets.get_texture("UI-GuildAchievement-Parchment-Horizontal-Desaturated");
    const Texture2D& achParchment = assets.get_texture("UI-Achievement-Parchment-Horizontal-Desaturated");
    const Texture2D& bgTex = assets.get_texture("UI-DialogBox-Background-Dark");
    const Texture2D& fallbackTex = assets.get_fallback();

    if (guildParchment.id > 0 && guildParchment.id != fallbackTex.id) {
        drawList->AddImage((ImTextureID)guildParchment.id, pMin, pMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32(200, 190, 180, 255));
    } else if (achParchment.id > 0 && achParchment.id != fallbackTex.id) {
        drawList->AddImage((ImTextureID)achParchment.id, pMin, pMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32(200, 190, 180, 255));
    } else if (bgTex.id > 0 && bgTex.id != fallbackTex.id) {
        DrawTiledTexture(drawList, (ImTextureID)bgTex.id, pMin, pMax, ImVec2(64.0f, 64.0f), IM_COL32(230, 230, 230, 255));
    } else {
        drawList->AddRectFilled(pMin, pMax, IM_COL32(18, 16, 14, 250), 4.0f);
    }

    if (drawBorder) {
        DrawWowDialogBorder(drawList, pMin, pMax);
    }
}

// Authentic Blizzard dialog frame: 32px corners from the DialogFrame-Corners
// atlas (2x2 grid: TL, TR / BL, BR) with stretched Top/Bot/Left/Right edges.
// The edge strips are pixel-uniform along their tile axis, so one quad per
// edge is exact. Falls back to a brass rect when any piece is missing.
inline void DrawWowDialogBorder(ImDrawList* drawList, ImVec2 pMin, ImVec2 pMax) {
    if (!drawList) return;

    AssetManager& assets = AssetManager::get();
    const Texture2D& cornersTex = assets.get_texture("DialogFrame-Corners");
    const Texture2D& topTex = assets.get_texture("DialogFrame-Top");
    const Texture2D& botTex = assets.get_texture("DialogFrame-Bot");
    const Texture2D& leftTex = assets.get_texture("DialogFrame-Left");
    const Texture2D& rightTex = assets.get_texture("DialogFrame-Right");
    // Note: get_texture() returns the fallback texture (never id 0) for a
    // missing file, so presence is tested by identity, not by id != 0.
    const Texture2D& fallbackTex = assets.get_fallback();
    bool have_all = cornersTex.id != fallbackTex.id && topTex.id != fallbackTex.id &&
                    botTex.id != fallbackTex.id && leftTex.id != fallbackTex.id &&
                    rightTex.id != fallbackTex.id;
    if (!have_all) {
        drawList->AddRect(pMin, pMax, IM_COL32(100, 80, 45, 220), 4.0f, 0, 1.5f);
        return;
    }

    float w = pMax.x - pMin.x;
    float h = pMax.y - pMin.y;
    if (w <= 0.0f || h <= 0.0f) return;

    FrameLayout L = compute_frame_layout(w, h, 32.0f, 16.0f);
    ImTextureID atlas = (ImTextureID)cornersTex.id;

    const FrameRect* corner_rects[4] = {&L.tl, &L.tr, &L.bl, &L.br};
    for (int i = 0; i < 4; ++i) {
        float u0, v0, u1, v1;
        corner_atlas_uv(i, u0, v0, u1, v1);
        const FrameRect& r = *corner_rects[i];
        drawList->AddImage(atlas,
            ImVec2(pMin.x + r.x0, pMin.y + r.y0), ImVec2(pMin.x + r.x1, pMin.y + r.y1),
            ImVec2(u0, v0), ImVec2(u1, v1));
    }

    drawList->AddImage((ImTextureID)topTex.id,
        ImVec2(pMin.x + L.top.x0, pMin.y + L.top.y0), ImVec2(pMin.x + L.top.x1, pMin.y + L.top.y1));
    drawList->AddImage((ImTextureID)botTex.id,
        ImVec2(pMin.x + L.bottom.x0, pMin.y + L.bottom.y0), ImVec2(pMin.x + L.bottom.x1, pMin.y + L.bottom.y1));
    drawList->AddImage((ImTextureID)leftTex.id,
        ImVec2(pMin.x + L.left.x0, pMin.y + L.left.y0), ImVec2(pMin.x + L.left.x1, pMin.y + L.left.y1));
    drawList->AddImage((ImTextureID)rightTex.id,
        ImVec2(pMin.x + L.right.x0, pMin.y + L.right.y0), ImVec2(pMin.x + L.right.x1, pMin.y + L.right.y1));
}

// Inset container well for sub-panels and stat tables - transparent background & clean no-border
inline void BeginWowChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::BeginChild(str_id, size, false, flags);
}

inline void EndWowChild() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// ============================================================================
// Classic WoW Item & Spell Slot Rendering with Quality Borders & Gloss
// ============================================================================
inline void DrawWowItemSlot(
    ImDrawList* drawList,
    ImTextureID iconTex,
    ImVec2 pMin,
    ImVec2 pMax,
    ImVec4 qualityColor = wow_colors::QualityCommon,
    bool isHovered = false
) {
    if (!drawList) return;

    // 1. Dark Slot Inset Well
    drawList->AddRectFilled(pMin, pMax, IM_COL32(18, 14, 12, 255), 3.0f);

    // 2. Item Icon
    if (iconTex) {
        drawList->AddImage(iconTex, pMin, pMax);
    }

    // 3. Item Quality Border
    ImU32 qCol = ImGui::ColorConvertFloat4ToU32(qualityColor);
    drawList->AddRect(ImVec2(pMin.x - 1.0f, pMin.y - 1.0f), ImVec2(pMax.x + 1.0f, pMax.y + 1.0f), IM_COL32(10, 8, 6, 220), 3.0f, 0, 1.0f);
    drawList->AddRect(pMin, pMax, qCol, 2.0f, 0, 1.5f);

    // 4. Hover Gloss Highlight
    if (isHovered) {
        drawList->AddRectFilled(pMin, pMax, IM_COL32(255, 255, 255, 35), 2.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(255, 235, 140, 240), 2.0f, 0, 2.0f);
    }
}

// ============================================================================
// Classic WoW Talent Slot Rendering with Authentic Blizzard Rank Badges
// ============================================================================
// ============================================================================
// Classic WoW Talent Slot Rendering with Smooth Anti-Aliased Glow & Crisp Rank Badges
// ============================================================================
inline void DrawWowTalentSlot(
    ImDrawList* drawList,
    ImTextureID iconTex,
    ImVec2 pMin,
    ImVec2 pMax,
    int currentPts,
    int maxPts,
    bool isUnlocked,
    bool prereqMet,
    bool isHovered
) {
    if (!drawList) return;

    // 1. Dark Slot Inset Well
    drawList->AddRectFilled(pMin, pMax, IM_COL32(18, 14, 12, 255), 3.0f);

    // 2. Icon Texture
    if (iconTex) {
        drawList->AddImage(iconTex, pMin, pMax);
    }

    // 3. Locked / Unavailable Dark Tint
    if (!isUnlocked || !prereqMet) {
        drawList->AddRectFilled(pMin, pMax, IM_COL32(0, 0, 0, 130), 2.0f);
    }

    // 4. Smooth Anti-Aliased Outlines (Thinner & Crisp)
    if (currentPts == maxPts && maxPts > 0) {
        // Gold (Maxed) - subtle soft anti-aliased feather + crisp 1.2px gold core
        drawList->AddRect(ImVec2(pMin.x - 1.0f, pMin.y - 1.0f), ImVec2(pMax.x + 1.0f, pMax.y + 1.0f), IM_COL32(255, 210, 0, 65), 3.5f, 0, 1.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(255, 215, 0, 255), 2.5f, 0, 1.2f);
    } else if (currentPts > 0) {
        // Green (Points Invested) - subtle soft anti-aliased feather + crisp 1.2px green core
        drawList->AddRect(ImVec2(pMin.x - 1.0f, pMin.y - 1.0f), ImVec2(pMax.x + 1.0f, pMax.y + 1.0f), IM_COL32(30, 230, 30, 65), 3.5f, 0, 1.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(35, 230, 35, 255), 2.5f, 0, 1.2f);
    } else if (isUnlocked && prereqMet) {
        // Available (Silver / Brass)
        drawList->AddRect(ImVec2(pMin.x - 0.5f, pMin.y - 0.5f), ImVec2(pMax.x + 0.5f, pMax.y + 0.5f), IM_COL32(10, 8, 6, 200), 3.0f, 0, 1.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(180, 165, 130, 230), 2.5f, 0, 1.0f);
    } else {
        // Locked (Dark Slate)
        drawList->AddRect(ImVec2(pMin.x - 0.5f, pMin.y - 0.5f), ImVec2(pMax.x + 0.5f, pMax.y + 0.5f), IM_COL32(10, 8, 6, 180), 3.0f, 0, 1.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(60, 50, 44, 190), 2.0f, 0, 1.0f);
    }

    // 5. Hover Gloss Highlight
    if (isHovered) {
        drawList->AddRectFilled(pMin, pMax, IM_COL32(255, 255, 255, 35), 2.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(255, 235, 140, 240), 2.0f, 0, 2.0f);
    }

    // 6. Crisp Rank Badge in Bottom-Right Corner (Borderless, Compact)
    char buf[16];
    snprintf(buf, sizeof(buf), "%d/%d", currentPts, maxPts);
    ImVec2 textSz = ImGui::CalcTextSize(buf);
    float badgeW = textSz.x + 3.0f;
    float badgeH = textSz.y - 1.0f;
    ImVec2 badgeMin(pMax.x - badgeW + 1.0f, pMax.y - badgeH + 1.0f);
    ImVec2 badgeMax(badgeMin.x + badgeW, badgeMin.y + badgeH);

    // Solid dark rounded background (borderless)
    drawList->AddRectFilled(badgeMin, badgeMax, IM_COL32(0, 0, 0, 225), 2.5f);

    ImVec2 textPos(
        badgeMin.x + (badgeW - textSz.x) * 0.5f,
        badgeMin.y + (badgeH - textSz.y) * 0.5f - 0.5f
    );

    ImU32 textCol = (currentPts == maxPts && maxPts > 0) ? IM_COL32(255, 215, 40, 255)
                  : (currentPts > 0)                    ? IM_COL32(50, 245, 50, 255)
                  : (isUnlocked && prereqMet)           ? IM_COL32(220, 220, 220, 255)
                                                        : IM_COL32(130, 130, 130, 255);

    // 4-way black outline for razor-sharp readability
    drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y), IM_COL32(0, 0, 0, 255), buf);
    drawList->AddText(ImVec2(textPos.x - 1.0f, textPos.y), IM_COL32(0, 0, 0, 255), buf);
    drawList->AddText(ImVec2(textPos.x, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), buf);
    drawList->AddText(ImVec2(textPos.x, textPos.y - 1.0f), IM_COL32(0, 0, 0, 255), buf);
    drawList->AddText(textPos, textCol, buf);
}

// ============================================================================
// Classic WoW Talent Tree Frame Header & Border
// Renders an authentic WoW Spec Header Bar with compact rounded point capacity badge
// ============================================================================
inline bool DrawWowTalentTreeHeader(
    ImDrawList* drawList,
    ImVec2 colMin,
    ImVec2 colMax,
    const char* treeName,
    int points,
    const ImVec4& titleCol,
    const char* resetId
) {
    float headerH = 28.0f;
    ImVec2 hMin = colMin;
    ImVec2 hMax = ImVec2(colMax.x, colMin.y + headerH);

    // 1. Header Bar Background
    drawList->AddRectFilled(hMin, hMax, IM_COL32(25, 20, 16, 245), 3.0f, ImDrawFlags_RoundCornersTop);
    // Beveled brass edge at bottom of header
    drawList->AddLine(ImVec2(hMin.x, hMax.y), ImVec2(hMax.x, hMax.y), IM_COL32(110, 88, 48, 255), 1.5f);
    // Subtle top highlight
    drawList->AddLine(ImVec2(hMin.x + 1, hMin.y + 1), ImVec2(hMax.x - 1, hMin.y + 1), IM_COL32(95, 75, 42, 200), 1.0f);

    // 2. Title and Point Capacity Badge (Borderless, Compact)
    char nameBuf[64];
    snprintf(nameBuf, sizeof(nameBuf), "%s", treeName);
    ImVec2 nameSz = ImGui::CalcTextSize(nameBuf);
    float textX = hMin.x + 10.0f;
    float textY = hMin.y + (headerH - nameSz.y) * 0.5f;

    ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(titleCol);
    drawList->AddText(ImVec2(textX + 1.0f, textY + 1.0f), IM_COL32(0, 0, 0, 220), nameBuf);
    drawList->AddText(ImVec2(textX, textY), colU32, nameBuf);

    // Tree points capacity compact borderless rounded black badge
    char ptsBuf[16];
    snprintf(ptsBuf, sizeof(ptsBuf), "%d", points);
    ImVec2 ptsSz = ImGui::CalcTextSize(ptsBuf);
    float pBadgeW = ptsSz.x + 6.0f;
    float pBadgeH = ptsSz.y + 1.0f;
    float pBadgeX = textX + nameSz.x + 6.0f;
    float pBadgeY = hMin.y + (headerH - pBadgeH) * 0.5f;
    ImVec2 pbMin(pBadgeX, pBadgeY);
    ImVec2 pbMax(pBadgeX + pBadgeW, pBadgeY + pBadgeH);

    // Solid black rounded background (borderless)
    drawList->AddRectFilled(pbMin, pbMax, IM_COL32(0, 0, 0, 220), 2.5f);

    ImVec2 ptTextPos(pbMin.x + (pBadgeW - ptsSz.x) * 0.5f, pbMin.y + (pBadgeH - ptsSz.y) * 0.5f - 0.5f);
    drawList->AddText(ImVec2(ptTextPos.x + 1.0f, ptTextPos.y), IM_COL32(0, 0, 0, 255), ptsBuf);
    drawList->AddText(ImVec2(ptTextPos.x - 1.0f, ptTextPos.y), IM_COL32(0, 0, 0, 255), ptsBuf);
    drawList->AddText(ImVec2(ptTextPos.x, ptTextPos.y + 1.0f), IM_COL32(0, 0, 0, 255), ptsBuf);
    drawList->AddText(ImVec2(ptTextPos.x, ptTextPos.y - 1.0f), IM_COL32(0, 0, 0, 255), ptsBuf);
    drawList->AddText(ptTextPos, (points > 0 ? IM_COL32(255, 215, 40, 255) : IM_COL32(160, 160, 160, 255)), ptsBuf);

    // 3. Reset Button ("Red X" close button on the right)
    float btnSz = 18.0f;
    float btnX = hMax.x - btnSz - 6.0f;
    float btnY = hMin.y + (headerH - btnSz) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
    bool resetPressed = WowCloseButton(resetId, ImVec2(btnSz, btnSz));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset all points in %s", treeName);
    }

    return resetPressed;
}

// ============================================================================
// Render a compact borderless rounded black badge for talent capacity numbers (e.g. "Points: 31 / 51")
// ============================================================================
inline void DrawWowPointsBadge(int currentPts, int maxPts = 51) {
    char ptsStr[32];
    snprintf(ptsStr, sizeof(ptsStr), "%d / %d", currentPts, maxPts);
    ImVec2 textSz = ImGui::CalcTextSize(ptsStr);
    
    float padX = 5.0f;
    float padY = 1.0f;
    float badgeW = textSz.x + padX * 2.0f;
    float badgeH = textSz.y + padY * 2.0f;
    
    ImVec2 cPos = ImGui::GetCursorScreenPos();
    ImVec2 bMin(cPos.x, cPos.y);
    ImVec2 bMax(cPos.x + badgeW, cPos.y + badgeH);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // Solid dark rounded background (borderless)
    drawList->AddRectFilled(bMin, bMax, IM_COL32(0, 0, 0, 220), 3.0f);
    
    ImU32 textCol = (currentPts == maxPts) ? IM_COL32(50, 245, 50, 255)
                  : (currentPts > maxPts)  ? IM_COL32(255, 70, 70, 255)
                  : (currentPts > 0)       ? IM_COL32(255, 215, 40, 255)
                                           : IM_COL32(160, 160, 160, 255);
                                           
    ImVec2 tPos(bMin.x + padX, bMin.y + padY - 0.5f);
    // 4-way dark outline for crisp readability
    drawList->AddText(ImVec2(tPos.x + 1.0f, tPos.y), IM_COL32(0, 0, 0, 255), ptsStr);
    drawList->AddText(ImVec2(tPos.x - 1.0f, tPos.y), IM_COL32(0, 0, 0, 255), ptsStr);
    drawList->AddText(ImVec2(tPos.x, tPos.y + 1.0f), IM_COL32(0, 0, 0, 255), ptsStr);
    drawList->AddText(ImVec2(tPos.x, tPos.y - 1.0f), IM_COL32(0, 0, 0, 255), ptsStr);
    drawList->AddText(tPos, textCol, ptsStr);
    
    ImGui::Dummy(ImVec2(badgeW, badgeH));
}

// ============================================================================
// Classic WoW Tooltip Frame Backdrop
// ============================================================================
inline void DrawWowTooltipBackdrop(ImDrawList* drawList, ImVec2 pMin, ImVec2 pMax) {
    if (!drawList) return;

    const Texture2D& bg = AssetManager::get().get_texture("UI-Tooltip-Background");
    if (bg.id > 0) {
        DrawTiledTexture(drawList, (ImTextureID)bg.id, pMin, pMax, ImVec2(64.0f, 64.0f), IM_COL32(20, 16, 14, 250));
    } else {
        drawList->AddRectFilled(pMin, pMax, IM_COL32(10, 8, 8, 250), 3.0f);
    }

    AssetManager& assets = AssetManager::get();
    const Texture2D& tlTex = assets.get_texture("UI-Tooltip-TL");
    const Texture2D& trTex = assets.get_texture("UI-Tooltip-TR");
    const Texture2D& blTex = assets.get_texture("UI-Tooltip-BL");
    const Texture2D& brTex = assets.get_texture("UI-Tooltip-BR");
    const Texture2D& tTex = assets.get_texture("UI-Tooltip-T");
    const Texture2D& bTex = assets.get_texture("UI-Tooltip-B");
    const Texture2D& lTex = assets.get_texture("UI-Tooltip-L");
    const Texture2D& rTex = assets.get_texture("UI-Tooltip-R");
    // See DrawWowDialogBorder: presence is tested by fallback identity.
    const Texture2D& fallbackTex = assets.get_fallback();
    bool have_all = tlTex.id != fallbackTex.id && trTex.id != fallbackTex.id &&
                    blTex.id != fallbackTex.id && brTex.id != fallbackTex.id &&
                    tTex.id != fallbackTex.id && bTex.id != fallbackTex.id &&
                    lTex.id != fallbackTex.id && rTex.id != fallbackTex.id;
    if (!have_all) {
        drawList->AddRect(pMin, pMax, IM_COL32(110, 90, 50, 230), 3.0f, 0, 1.2f);
        return;
    }

    float w = pMax.x - pMin.x;
    float h = pMax.y - pMin.y;
    if (w <= 0.0f || h <= 0.0f) return;

    // 8px corners with stretched uniform edges (one quad per piece is exact).
    FrameLayout L = compute_frame_layout(w, h, 8.0f, 8.0f);
    drawList->AddImage((ImTextureID)tlTex.id,
        ImVec2(pMin.x + L.tl.x0, pMin.y + L.tl.y0), ImVec2(pMin.x + L.tl.x1, pMin.y + L.tl.y1));
    drawList->AddImage((ImTextureID)trTex.id,
        ImVec2(pMin.x + L.tr.x0, pMin.y + L.tr.y0), ImVec2(pMin.x + L.tr.x1, pMin.y + L.tr.y1));
    drawList->AddImage((ImTextureID)blTex.id,
        ImVec2(pMin.x + L.bl.x0, pMin.y + L.bl.y0), ImVec2(pMin.x + L.bl.x1, pMin.y + L.bl.y1));
    drawList->AddImage((ImTextureID)brTex.id,
        ImVec2(pMin.x + L.br.x0, pMin.y + L.br.y0), ImVec2(pMin.x + L.br.x1, pMin.y + L.br.y1));
    drawList->AddImage((ImTextureID)tTex.id,
        ImVec2(pMin.x + L.top.x0, pMin.y + L.top.y0), ImVec2(pMin.x + L.top.x1, pMin.y + L.top.y1));
    drawList->AddImage((ImTextureID)bTex.id,
        ImVec2(pMin.x + L.bottom.x0, pMin.y + L.bottom.y0), ImVec2(pMin.x + L.bottom.x1, pMin.y + L.bottom.y1));
    drawList->AddImage((ImTextureID)lTex.id,
        ImVec2(pMin.x + L.left.x0, pMin.y + L.left.y0), ImVec2(pMin.x + L.left.x1, pMin.y + L.left.y1));
    drawList->AddImage((ImTextureID)rTex.id,
        ImVec2(pMin.x + L.right.x0, pMin.y + L.right.y0), ImVec2(pMin.x + L.right.x1, pMin.y + L.right.y1));
}

// ============================================================================
// Vendored panel background art (assets/wow_classic/PanelBackgrounds).
// DrawPanelBanner renders a fixed-height aspect-fill banner with a centered
// gold title; DrawTiledPanelBackdrop tiles a stone texture over a known rect.
// Both fall back to flat stone colors when a texture is missing.
// ============================================================================
inline bool PanelTextureAvailable(const char* tex_name) {
    AssetManager& assets = AssetManager::get();
    const Texture2D& tex = assets.get_texture(tex_name);
    return tex.id != assets.get_fallback().id && tex.width > 0 && tex.height > 0;
}

inline void DrawTiledPanelBackdrop(ImVec2 pMin, ImVec2 pMax, const char* tex_name,
                                   ImU32 tint = IM_COL32(255, 255, 255, 255),
                                   ImVec2 tile = ImVec2(128.0f, 128.0f)) {
    if (!PanelTextureAvailable(tex_name)) return;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return;
    const Texture2D& tex = AssetManager::get().get_texture(tex_name);
    DrawTiledTexture(dl, (ImTextureID)(uintptr_t)tex.id, pMin, pMax, tile, tint);
}

inline void DrawPanelBanner(const char* title, const char* tex_name, float height = 64.0f,
                            ImU32 tint = IM_COL32(255, 255, 255, 255)) {
    float w = ImGui::GetContentRegionAvail().x;
    if (w < 10.0f || height <= 0.0f) return;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 sz(w, height);
    ImGui::Dummy(sz);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return;
    ImVec2 p1(p0.x + sz.x, p0.y + sz.y);
    AssetManager& assets = AssetManager::get();
    const Texture2D& tex = assets.get_texture(tex_name);
    if (tex.id != assets.get_fallback().id && tex.width > 0 && tex.height > 0) {
        // Aspect-fill center crop so parchment/stone art covers the strip.
        float scale = std::max(sz.x / static_cast<float>(tex.width),
                               sz.y / static_cast<float>(tex.height));
        float uvW = sz.x / (static_cast<float>(tex.width) * scale);
        float uvH = sz.y / (static_cast<float>(tex.height) * scale);
        ImVec2 uv0((1.0f - uvW) * 0.5f, (1.0f - uvH) * 0.5f);
        ImVec2 uv1(uv0.x + uvW, uv0.y + uvH);
        dl->AddImage((ImTextureID)(uintptr_t)tex.id, p0, p1, uv0, uv1, tint);
        // Darken slightly so the gold title stays legible on parchment.
        dl->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 90), 4.0f);
    } else {
        dl->AddRectFilled(p0, p1, IM_COL32(35, 25, 18, 220), 4.0f);
    }
    dl->AddRect(p0, p1, IM_COL32(140, 110, 60, 255), 4.0f, 0, 1.2f);
    ImVec2 ts = ImGui::CalcTextSize(title);
    ImVec2 tp(p0.x + (sz.x - ts.x) * 0.5f, p0.y + (sz.y - ts.y) * 0.5f);
    dl->AddText(ImVec2(tp.x + 1.0f, tp.y + 1.0f), IM_COL32(0, 0, 0, 255), title);
    dl->AddText(tp, IM_COL32(255, 209, 0, 255), title);
}

// ============================================================================
// Classic WoW Tab Bar & Tab Items (Texture-Based)
//
// Uses authentic Blizzard tab textures from the Character Frame:
//   UI-CHARACTER-ACTIVETAB   (128x32) — Selected tab with beveled stone frame
//   UI-CHARACTER-INACTIVETAB (128x32) — Unselected tab, dimmer stone
//   UI-Character-Tab-Highlight-yellow (128x32) — Gold hover glow
//
// Usage:
//   if (WowBeginTabBar("MyTabs")) {
//       if (WowBeginTabItem("Tab 1")) { ... WowEndTabItem(); }
//       if (WowBeginTabItem("Tab 2")) { ... WowEndTabItem(); }
//       WowEndTabBar();
//   }
// ============================================================================

// Internal: number of style colors pushed by WowBeginTabBar
inline constexpr int kWowTabStyleColorCount = 7;
inline constexpr int kWowTabStyleVarCount   = 3;

inline bool WowBeginTabBar(const char* str_id, ImGuiTabBarFlags flags = 0) {
    // Make ImGui's built-in tab background rendering fully transparent so we
    // can draw the WoW textures on top. We also suppress the overline and
    // tab bar border since we render our own ornamental separator.
    ImGui::PushStyleColor(ImGuiCol_Tab,                    ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabSelected,            ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,             ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabDimmed,              ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected,      ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline,    ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelectedOverline, ImVec4(0, 0, 0, 0));
    // NOTE: We do NOT push transparent text here — that's done per-tab in
    // WowBeginTabItem to avoid leaking invisible text into tab panel content.


    ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize,  0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabBarOverlineSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize,     0.0f);

    bool opened = ImGui::BeginTabBar(str_id, flags);
    return opened;
}

inline void WowEndTabBar() {
    // Draw a WoW brass/etched separator line below the tab bar
    ImGuiContext& g = *GImGui;
    ImGuiTabBar* tab_bar = g.CurrentTabBar;
    if (tab_bar) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float y = tab_bar->BarRect.Max.y;
        float x0 = tab_bar->SeparatorMinX;
        float x1 = tab_bar->SeparatorMaxX;
        // Dark shadow line
        dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), IM_COL32(10, 8, 5, 220), 1.0f);
        // Brass highlight line just below
        dl->AddLine(ImVec2(x0, y + 1.0f), ImVec2(x1, y + 1.0f), IM_COL32(90, 72, 40, 160), 1.0f);
    }

    ImGui::EndTabBar();
    ImGui::PopStyleVar(kWowTabStyleVarCount);
    ImGui::PopStyleColor(kWowTabStyleColorCount);
}

// Internal helper: draw a WoW tab texture (3-slice horizontal) at a given rect
// and render the label text in gold with a drop shadow.
inline void DrawWowTabOverlay(
    ImDrawList* drawList,
    ImVec2 tabMin,
    ImVec2 tabMax,
    const char* label,
    bool isSelected,
    bool isHovered
) {
    if (!drawList) return;

    float tabW = tabMax.x - tabMin.x;
    float tabH = tabMax.y - tabMin.y;
    if (tabW <= 0.0f || tabH <= 0.0f) return;

    AssetManager& assets = AssetManager::get();

    // Pick the right base texture
    const Texture2D& activeTex   = assets.get_texture("UI-CHARACTER-ACTIVETAB");
    const Texture2D& inactiveTex = assets.get_texture("UI-CHARACTER-INACTIVETAB");

    const Texture2D& baseTex = isSelected ? activeTex : inactiveTex;

    // The tab textures are 128x32. The active art occupies roughly the region
    // with some padding. We use a UV crop that captures the visible tab shape
    // while trimming the fully transparent margins.
    //
    // Active tab (128x32): visible art spans approximately [4..124] x [2..30]
    // Inactive tab (128x32): visible art spans approximately [4..124] x [6..30]
    const float kTexWidth  = 128.0f;
    const float kTexHeight = 32.0f;

    // Invert V (Y) UV coordinates so the rounded dog-eared corners are at the top
    // and the flat base is at the bottom, poking upward from the separator.
    // UI-CHARACTER-INACTIVETAB has transparent margins in the raw texture:
    // the base starts at row 2 and top art ends at row 28 (of 32px height).
    // Trimming UVs for inactive tabs ensures their bottom baseline is flush
    // with active tabs and the frame below.
    const ImVec2 uvMin = isSelected ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 28.0f / 32.0f);
    const ImVec2 uvMax = isSelected ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 2.0f / 32.0f);

    float padX = tabW * 0.03f;  // slight horizontal extension
    float padTop = isSelected ? 5.0f : 1.0f;  // selected tab extends UPWARD
    float padBottom = 1.0f;                  // flush with bottom separator

    ImVec2 texMin(tabMin.x - padX, tabMin.y - padTop);
    ImVec2 texMax(tabMax.x + padX, tabMax.y + padBottom);

    // Draw the base tab texture using 3-slice horizontal stretching
    // Cap width based on the texture's stone corner size (~16px of 128px)
    float capWidth = 16.0f;
    if (baseTex.id > 0) {
        DrawThreeSliceHorizontal(
            drawList, (ImTextureID)baseTex.id,
            texMin, texMax,
            capWidth, kTexWidth,
            uvMin, uvMax
        );
    } else {
        // Fallback: draw a simple beveled rect with rounded top corners
        ImU32 bgCol = isSelected ? IM_COL32(40, 32, 24, 240) : IM_COL32(24, 20, 16, 200);
        drawList->AddRectFilled(ImVec2(tabMin.x, tabMin.y - padTop), texMax, bgCol, 4.0f, ImDrawFlags_RoundCornersTop);
        drawList->AddRect(ImVec2(tabMin.x, tabMin.y - padTop), texMax, IM_COL32(100, 80, 45, 200), 4.0f, ImDrawFlags_RoundCornersTop, 1.0f);
    }

    // --- Gold label text with drop shadow ---
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    ImVec2 labelSize = ImGui::CalcTextSize(label, label_end, true);
    float textOffsetY = isSelected ? -2.0f : 0.0f;  // Text raises slightly when selected
    ImVec2 textPos(
        tabMin.x + (tabW - labelSize.x) * 0.5f,
        tabMin.y + (tabH - labelSize.y) * 0.5f + textOffsetY
    );

    // Text colors: gold for selected, parchment for inactive, bright on hover
    ImU32 textCol;
    if (isSelected) {
        textCol = isHovered ? IM_COL32(255, 240, 120, 255)  // Bright gold on hover
                            : IM_COL32(255, 209, 0, 255);    // Classic WoW Gold #FFD100
    } else {
        textCol = isHovered ? IM_COL32(255, 230, 150, 255)  // Warm gold on hover
                            : IM_COL32(190, 175, 140, 255);  // Muted parchment for inactive
    }

    // Drop shadow (1px offset)
    drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 220), label, label_end);
    // Main text
    drawList->AddText(textPos, textCol, label, label_end);
}

// WowBeginTabItem: wraps ImGui::BeginTabItem and overlays the WoW tab texture.
// Returns true if the tab contents should be rendered (same as ImGui::BeginTabItem).
// IMPORTANT: You must call WowEndTabItem() if this returns true.
inline bool WowBeginTabItem(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0) {
    // Push transparent text ONLY for the BeginTabItem call so ImGui's default
    // tab label is invisible. We re-draw our own gold text via DrawWowTabOverlay.
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));

    bool selected = ImGui::BeginTabItem(label, p_open, flags);

    // Immediately restore text color so tab panel content renders normally
    ImGui::PopStyleColor();

    // After BeginTabItem, the last item rect corresponds to the tab button
    ImVec2 tabMin = ImGui::GetItemRectMin();
    ImVec2 tabMax = ImGui::GetItemRectMax();

    // Determine tab state
    bool isSelected = selected;  // BeginTabItem returns true when tab is visible
    bool isHovered = ImGui::IsItemHovered();

    // Draw the WoW tab overlay on the window draw list
    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawWowTabOverlay(dl, tabMin, tabMax, label, isSelected, isHovered);

    return selected;
}

inline void WowEndTabItem() {
    ImGui::EndTabItem();
}

inline bool WowTabButton(const char* label, bool is_selected, float width = 0.0f, float height = 30.0f) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    float item_w = (width > 0.0f) ? width : (label_size.x + 36.0f);
    float item_h = (height > 0.0f) ? height : 30.0f;
    ImVec2 size(item_w, item_h);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawWowTabOverlay(dl, bb.Min, bb.Max, label, is_selected, hovered);

    return pressed;
}

} // namespace warlock
