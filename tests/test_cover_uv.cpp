#include "test_framework.hpp"
#include "src/ui/common/cover_uv.hpp"

using namespace warlock;

// The sampled texture region must always have the same aspect ratio as the
// window: that is what guarantees the background is never stretched.
static void check_no_stretch(float tex_w, float tex_h, float win_w, float win_h) {
    float u0, v0, u1, v1;
    compute_cover_uv(tex_w, tex_h, win_w, win_h, u0, v0, u1, v1);
    const float sampled_aspect = ((u1 - u0) * tex_w) / ((v1 - v0) * tex_h);
    CHECK_NEAR(sampled_aspect, win_w / win_h, 1e-5f);
}

TEST_CASE(CoverUV, MatchingAspectUsesFullImage) {
    float u0, v0, u1, v1;
    compute_cover_uv(512.0f, 256.0f, 1024.0f, 512.0f, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.0f, 1e-6f);
    CHECK_NEAR(v0, 0.0f, 1e-6f);
    CHECK_NEAR(u1, 1.0f, 1e-6f);
    CHECK_NEAR(v1, 1.0f, 1e-6f);
}

TEST_CASE(CoverUV, WideWindowCropsTopAndBottom) {
    float u0, v0, u1, v1;
    compute_cover_uv(256.0f, 256.0f, 400.0f, 200.0f, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.0f, 1e-6f);
    CHECK_NEAR(u1, 1.0f, 1e-6f);
    CHECK(v0 > 0.0f);
    CHECK(v1 < 1.0f);
    CHECK_NEAR(v0 + v1, 1.0f, 1e-6f);  // centered crop
    check_no_stretch(256.0f, 256.0f, 400.0f, 200.0f);
}

TEST_CASE(CoverUV, NarrowWindowCropsSides) {
    float u0, v0, u1, v1;
    compute_cover_uv(512.0f, 256.0f, 200.0f, 400.0f, u0, v0, u1, v1);
    CHECK_NEAR(v0, 0.0f, 1e-6f);
    CHECK_NEAR(v1, 1.0f, 1e-6f);
    CHECK(u0 > 0.0f);
    CHECK(u1 < 1.0f);
    CHECK_NEAR(u0 + u1, 1.0f, 1e-6f);  // centered crop
    check_no_stretch(512.0f, 256.0f, 200.0f, 400.0f);
}

TEST_CASE(CoverUV, DegenerateInputFallsBackToFullImage) {
    float u0, v0, u1, v1;
    compute_cover_uv(0.0f, 256.0f, 400.0f, 200.0f, u0, v0, u1, v1);
    CHECK_NEAR(u0, 0.0f, 1e-6f);
    CHECK_NEAR(v0, 0.0f, 1e-6f);
    CHECK_NEAR(u1, 1.0f, 1e-6f);
    CHECK_NEAR(v1, 1.0f, 1e-6f);
    compute_cover_uv(256.0f, 256.0f, 0.0f, 200.0f, u0, v0, u1, v1);
    CHECK_NEAR(u1, 1.0f, 1e-6f);
    CHECK_NEAR(v1, 1.0f, 1e-6f);
}
