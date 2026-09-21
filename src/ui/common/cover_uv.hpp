#pragma once

namespace warlock
{

// Compute aspect-preserving "cover" texture UVs for drawing an image into a
// window of a different aspect ratio. The image fills the whole window with no
// stretching; the overflow axis is center-cropped. Degenerate inputs yield the
// full (0,0)-(1,1) range.
inline void compute_cover_uv(float tex_w, float tex_h, float win_w, float win_h,
                             float& u0, float& v0, float& u1, float& v1)
{
  u0 = 0.0f;
  v0 = 0.0f;
  u1 = 1.0f;
  v1 = 1.0f;
  if (tex_w <= 0.0f || tex_h <= 0.0f || win_w <= 0.0f || win_h <= 0.0f)
    return;

  const float win_aspect = win_w / win_h;
  const float tex_aspect = tex_w / tex_h;

  if (win_aspect > tex_aspect)
  {
    // Window is wider than texture: crop top/bottom.
    const float v_span = tex_aspect / win_aspect;
    v0 = (1.0f - v_span) * 0.5f;
    v1 = v0 + v_span;
  }
  else
  {
    // Window is narrower than texture: crop sides (centered horizontally).
    const float u_span = win_aspect / tex_aspect;
    u0 = (1.0f - u_span) * 0.5f;
    u1 = u0 + u_span;
  }
}

}  // namespace warlock
