#include "test_framework.hpp"
#include "src/ui/common/frame_geometry.hpp"

using namespace warlock;

TEST_CASE(FrameGeometry, DialogLayout800x600) {
    FrameLayout L = compute_frame_layout(800.0f, 600.0f, 32.0f, 16.0f);
    CHECK_NEAR(L.corner, 32.0f, 1e-6f);
    CHECK_NEAR(L.edge, 16.0f, 1e-6f);
    // Corners occupy the box corners.
    CHECK_NEAR(L.tl.x1, 32.0f, 1e-6f);
    CHECK_NEAR(L.tr.x0, 768.0f, 1e-6f);
    CHECK_NEAR(L.bl.y0, 568.0f, 1e-6f);
    CHECK_NEAR(L.br.x0, 768.0f, 1e-6f);
    CHECK_NEAR(L.br.y0, 568.0f, 1e-6f);
    // Edges span exactly between corners.
    CHECK_NEAR(L.top.x0, 32.0f, 1e-6f);
    CHECK_NEAR(L.top.x1, 768.0f, 1e-6f);
    CHECK_NEAR(L.top.y1, 16.0f, 1e-6f);
    CHECK_NEAR(L.bottom.y0, 584.0f, 1e-6f);
    CHECK_NEAR(L.left.y0, 32.0f, 1e-6f);
    CHECK_NEAR(L.left.y1, 568.0f, 1e-6f);
    CHECK_NEAR(L.left.x1, 16.0f, 1e-6f);
    CHECK_NEAR(L.right.x0, 784.0f, 1e-6f);
}

TEST_CASE(FrameGeometry, TooltipLayout200x100) {
    FrameLayout L = compute_frame_layout(200.0f, 100.0f, 8.0f, 8.0f);
    CHECK_NEAR(L.corner, 8.0f, 1e-6f);
    CHECK_NEAR(L.top.x0, 8.0f, 1e-6f);
    CHECK_NEAR(L.top.x1, 192.0f, 1e-6f);
    CHECK_NEAR(L.left.y0, 8.0f, 1e-6f);
    CHECK_NEAR(L.left.y1, 92.0f, 1e-6f);
}

TEST_CASE(FrameGeometry, SmallBoxClampsCorner) {
    // 40px box cannot fit 32px corners: they clamp to half the box.
    FrameLayout L = compute_frame_layout(40.0f, 40.0f, 32.0f, 16.0f);
    CHECK_NEAR(L.corner, 20.0f, 1e-6f);
    CHECK(L.tr.x0 >= L.tl.x0);
    CHECK(L.bl.y0 >= L.tl.y0);
    CHECK(L.top.x1 >= L.top.x0);
    CHECK(L.left.y1 >= L.left.y0);
}

TEST_CASE(FrameGeometry, DegenerateInputIsEmpty) {
    FrameLayout L = compute_frame_layout(0.0f, 600.0f, 32.0f, 16.0f);
    CHECK_NEAR(L.corner, 0.0f, 1e-6f);
    CHECK_NEAR(L.top.x1 - L.top.x0, 0.0f, 1e-6f);
    FrameLayout N = compute_frame_layout(-10.0f, -10.0f, 32.0f, 16.0f);
    CHECK_NEAR(N.corner, 0.0f, 1e-6f);
}

TEST_CASE(FrameGeometry, CornerAtlasQuadrants) {
    float u0, v0, u1, v1;
    corner_atlas_uv(CORNER_TL, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.0f, 1e-6f); CHECK_NEAR(v0, 0.0f, 1e-6f);
    CHECK_NEAR(u1, 0.5f, 1e-6f); CHECK_NEAR(v1, 0.5f, 1e-6f);
    corner_atlas_uv(CORNER_TR, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.5f, 1e-6f); CHECK_NEAR(v0, 0.0f, 1e-6f);
    CHECK_NEAR(u1, 1.0f, 1e-6f); CHECK_NEAR(v1, 0.5f, 1e-6f);
    corner_atlas_uv(CORNER_BL, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.0f, 1e-6f); CHECK_NEAR(v0, 0.5f, 1e-6f);
    CHECK_NEAR(u1, 0.5f, 1e-6f); CHECK_NEAR(v1, 1.0f, 1e-6f);
    corner_atlas_uv(CORNER_BR, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.5f, 1e-6f); CHECK_NEAR(v0, 0.5f, 1e-6f);
    CHECK_NEAR(u1, 1.0f, 1e-6f); CHECK_NEAR(v1, 1.0f, 1e-6f);
    // Bad index falls back to the full range.
    corner_atlas_uv(99, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.0f, 1e-6f); CHECK_NEAR(v0, 0.0f, 1e-6f);
    CHECK_NEAR(u1, 1.0f, 1e-6f); CHECK_NEAR(v1, 1.0f, 1e-6f);
}
