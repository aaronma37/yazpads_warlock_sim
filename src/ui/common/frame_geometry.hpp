#pragma once

namespace warlock
{

// Pure geometry for authentic multi-piece Blizzard frames.
// Dialog boxes assemble DialogFrame-Top/Bot/Left/Right edges with 32x32
// corners from the DialogFrame-Corners atlas (2x2 grid: TL, TR / BL, BR).
// Tooltips assemble the 8x8 Tooltips T/B/L/R edges with 8x8 corners.
// Edge strips are pixel-uniform along their tile axis (verified against the
// extracted PNGs), so each edge renders as one stretched quad. No ImGui
// dependency: the renderers in wow_widgets.hpp consume these rects.

struct FrameRect
{
  float x0 = 0.0f;
  float y0 = 0.0f;
  float x1 = 0.0f;
  float y1 = 0.0f;
};

struct FrameLayout
{
  float corner = 0.0f;  // clamped corner size actually used
  float edge = 0.0f;    // clamped edge thickness actually used
  FrameRect tl, tr, bl, br;
  FrameRect top, bottom, left, right;
};

// Corner index convention matching the DialogFrame-Corners atlas quadrants.
enum CornerIndex
{
  CORNER_TL = 0,
  CORNER_TR = 1,
  CORNER_BL = 2,
  CORNER_BR = 3
};

// UV quadrant of corner_index inside a 2x2 corner atlas. Degenerate index
// yields the full (0,0)-(1,1) range.
inline void corner_atlas_uv(int corner_index, float& u0, float& v0, float& u1, float& v1)
{
  u0 = 0.0f;
  v0 = 0.0f;
  u1 = 1.0f;
  v1 = 1.0f;
  if (corner_index < CORNER_TL || corner_index > CORNER_BR)
    return;
  u0 = (corner_index == CORNER_TR || corner_index == CORNER_BR) ? 0.5f : 0.0f;
  u1 = u0 + 0.5f;
  v0 = (corner_index == CORNER_BL || corner_index == CORNER_BR) ? 0.5f : 0.0f;
  v1 = v0 + 0.5f;
}

// Frame rects relative to a (0,0)-(w,h) box. Corner and edge sizes clamp to
// fit; degenerate inputs yield empty rects, never negative spans.
inline FrameLayout compute_frame_layout(float w, float h, float corner, float edge)
{
  FrameLayout L;
  if (w <= 0.0f || h <= 0.0f || corner <= 0.0f)
    return L;

  float c = corner;
  if (c > w * 0.5f)
    c = w * 0.5f;
  if (c > h * 0.5f)
    c = h * 0.5f;
  float e = edge;
  if (e < 0.0f)
    e = 0.0f;
  if (e > w * 0.5f)
    e = w * 0.5f;
  if (e > h * 0.5f)
    e = h * 0.5f;

  L.corner = c;
  L.edge = e;

  L.tl = {0.0f, 0.0f, c, c};
  L.tr = {w - c, 0.0f, w, c};
  L.bl = {0.0f, h - c, c, h};
  L.br = {w - c, h - c, w, h};

  L.top = {c, 0.0f, w - c, e};
  L.bottom = {c, h - e, w - c, h};
  L.left = {0.0f, c, e, h - c};
  L.right = {w - e, c, w, h - c};
  return L;
}

}  // namespace warlock
