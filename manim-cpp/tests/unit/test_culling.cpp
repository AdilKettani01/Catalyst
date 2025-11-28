#include <gtest/gtest.h>
#include "manim/culling/culling_types.hpp"
#include "manim/culling/frustum_culler.hpp"
#include "manim/culling/occlusion_culler.hpp"
#include "manim/culling/gpu_culling_pipeline.hpp"
#include "manim/culling/indirect_renderer.hpp"
#include "manim/core/math.hpp"
#include <glm/gtc/matrix_transform.hpp>

using namespace manim;
using namespace manim::culling;

// ============================================================================
// Frustum Plane Extraction Tests
// ============================================================================

TEST(FrustumCullingTest, ExtractFrustumPlanesIdentity) {
    // Identity matrix should give default frustum
    math::Mat4 identity(1.0f);
    auto frustum = FrustumCuller::extract_frustum_planes(identity);

    // All planes should be valid (non-zero normal)
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(math::Vec3(frustum.planes[i]));
        EXPECT_GT(len, 0.0f) << "Plane " << i << " has zero normal";
    }
}

TEST(FrustumCullingTest, ExtractFrustumPlanesOrtho) {
    // Orthographic projection
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
    auto frustum = FrustumCuller::extract_frustum_planes(ortho);

    // Verify planes are normalized
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(math::Vec3(frustum.planes[i]));
        EXPECT_NEAR(len, 1.0f, 0.001f) << "Plane " << i << " not normalized";
    }
}

TEST(FrustumCullingTest, ExtractFrustumPlanesPerspective) {
    // Perspective projection
    math::Mat4 persp = glm::perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 1000.0f);
    math::Mat4 view = glm::lookAt(
        math::Vec3(0.0f, 0.0f, 5.0f),
        math::Vec3(0.0f, 0.0f, 0.0f),
        math::Vec3(0.0f, 1.0f, 0.0f)
    );
    math::Mat4 view_proj = persp * view;

    auto frustum = FrustumCuller::extract_frustum_planes(view_proj);

    // Verify planes are normalized
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(math::Vec3(frustum.planes[i]));
        EXPECT_NEAR(len, 1.0f, 0.001f) << "Plane " << i << " not normalized";
    }
}

// ============================================================================
// AABB vs Frustum Tests (CPU)
// ============================================================================

TEST(FrustumCullingTest, AABBInsideFrustum) {
    // Create a simple frustum from orthographic projection
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
    auto frustum = FrustumCuller::extract_frustum_planes(ortho);

    // AABB at origin should be inside
    math::Vec3 aabb_min(-1.0f, -1.0f, -1.0f);
    math::Vec3 aabb_max(1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(FrustumCuller::test_aabb_frustum(aabb_min, aabb_max, frustum));
}

TEST(FrustumCullingTest, AABBOutsideFrustum) {
    // Create a simple frustum from orthographic projection
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
    auto frustum = FrustumCuller::extract_frustum_planes(ortho);

    // AABB far outside should be rejected
    math::Vec3 aabb_min(100.0f, 100.0f, 100.0f);
    math::Vec3 aabb_max(101.0f, 101.0f, 101.0f);

    EXPECT_FALSE(FrustumCuller::test_aabb_frustum(aabb_min, aabb_max, frustum));
}

TEST(FrustumCullingTest, AABBPartiallyInsideFrustum) {
    // Create a simple frustum from orthographic projection
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
    auto frustum = FrustumCuller::extract_frustum_planes(ortho);

    // AABB straddling the edge should be considered visible
    math::Vec3 aabb_min(8.0f, -1.0f, -1.0f);
    math::Vec3 aabb_max(12.0f, 1.0f, 1.0f);

    // Partially inside, should be visible (conservative)
    EXPECT_TRUE(FrustumCuller::test_aabb_frustum(aabb_min, aabb_max, frustum));
}

// ============================================================================
// CPU Batch Culling Tests
// ============================================================================

TEST(FrustumCullingTest, CPUBatchCulling) {
    // Create frustum
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
    auto frustum = FrustumCuller::extract_frustum_planes(ortho);
    frustum.num_objects = 5;

    // Create test objects
    std::vector<ObjectBounds> bounds;

    // Object 0: Inside (should be visible)
    bounds.emplace_back(math::Vec3(-1.0f), math::Vec3(1.0f), 0);

    // Object 1: Inside (should be visible)
    bounds.emplace_back(math::Vec3(5.0f, 5.0f, 5.0f), math::Vec3(6.0f, 6.0f, 6.0f), 1);

    // Object 2: Outside (should be culled)
    bounds.emplace_back(math::Vec3(50.0f, 50.0f, 50.0f), math::Vec3(51.0f, 51.0f, 51.0f), 2);

    // Object 3: Outside (should be culled)
    bounds.emplace_back(math::Vec3(-50.0f, -50.0f, -50.0f), math::Vec3(-49.0f, -49.0f, -49.0f), 3);

    // Object 4: Partially inside (should be visible)
    bounds.emplace_back(math::Vec3(9.0f, 0.0f, 0.0f), math::Vec3(15.0f, 1.0f, 1.0f), 4);

    // Run CPU culling
    auto visible = FrustumCuller::cull_cpu(bounds, frustum);

    // Verify results
    EXPECT_EQ(visible.size(), 3u);  // Objects 0, 1, 4 should be visible

    // Verify specific objects
    EXPECT_NE(std::find(visible.begin(), visible.end(), 0u), visible.end());
    EXPECT_NE(std::find(visible.begin(), visible.end(), 1u), visible.end());
    EXPECT_EQ(std::find(visible.begin(), visible.end(), 2u), visible.end());
    EXPECT_EQ(std::find(visible.begin(), visible.end(), 3u), visible.end());
    EXPECT_NE(std::find(visible.begin(), visible.end(), 4u), visible.end());
}

// ============================================================================
// ObjectBounds Tests
// ============================================================================

TEST(CullingTypesTest, ObjectBoundsCreation) {
    math::Vec3 min(-1.0f, -2.0f, -3.0f);
    math::Vec3 max(1.0f, 2.0f, 3.0f);
    uint32_t id = 42;

    ObjectBounds bounds(min, max, id);

    // Check AABB
    EXPECT_FLOAT_EQ(bounds.aabb_min.x, -1.0f);
    EXPECT_FLOAT_EQ(bounds.aabb_min.y, -2.0f);
    EXPECT_FLOAT_EQ(bounds.aabb_min.z, -3.0f);
    EXPECT_FLOAT_EQ(bounds.aabb_max.x, 1.0f);
    EXPECT_FLOAT_EQ(bounds.aabb_max.y, 2.0f);
    EXPECT_FLOAT_EQ(bounds.aabb_max.z, 3.0f);

    // Check object ID
    EXPECT_EQ(bounds.get_object_id(), 42u);

    // Check bounding sphere center
    EXPECT_FLOAT_EQ(bounds.sphere.x, 0.0f);
    EXPECT_FLOAT_EQ(bounds.sphere.y, 0.0f);
    EXPECT_FLOAT_EQ(bounds.sphere.z, 0.0f);

    // Check bounding sphere radius (should encompass all corners)
    float expected_radius = glm::length(max);
    EXPECT_NEAR(bounds.sphere.w, expected_radius, 0.001f);
}

TEST(CullingTypesTest, ObjectBoundsSizeAlignment) {
    EXPECT_EQ(sizeof(ObjectBounds), 48u);
    EXPECT_EQ(sizeof(BVHNode), 32u);
    EXPECT_EQ(alignof(ObjectBounds), 16u);
    EXPECT_EQ(alignof(BVHNode), 16u);
}

// ============================================================================
// BVHNode Tests
// ============================================================================

TEST(CullingTypesTest, BVHNodeLeafDetection) {
    BVHNode node;

    // Set as internal node
    node.left = 1;
    node.right = 2;
    EXPECT_FALSE(node.is_leaf());

    // Set as leaf node
    node.set_leaf(10, 5);
    EXPECT_TRUE(node.is_leaf());
    EXPECT_EQ(node.left, 10u);  // first_primitive
    EXPECT_EQ(node.get_primitive_count(), 5u);
}

// ============================================================================
// FrustumPlanes Tests
// ============================================================================

TEST(CullingTypesTest, FrustumPlanesSize) {
    EXPECT_EQ(sizeof(FrustumPlanes), 112u);
}

// ============================================================================
// GPU Culling Pipeline Tests
// ============================================================================

TEST(GPUCullingPipelineTest, Creation) {
    GPUCullingPipeline pipeline;
    EXPECT_FALSE(pipeline.is_initialized());
    EXPECT_EQ(pipeline.get_object_count(), 0u);
}

TEST(GPUCullingPipelineTest, ConfigDefaults) {
    GPUCullingPipeline pipeline;
    const auto& config = pipeline.get_config();

    EXPECT_TRUE(config.enable_frustum_culling);
    EXPECT_FALSE(config.enable_occlusion_culling);
    EXPECT_FALSE(config.enable_bvh_acceleration);
    EXPECT_TRUE(config.enable_compaction);
}

TEST(GPUCullingPipelineTest, StaticFrustumExtraction) {
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
    auto frustum = GPUCullingPipeline::extract_frustum_planes(ortho);

    // Should be same as FrustumCuller::extract_frustum_planes
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(math::Vec3(frustum.planes[i]));
        EXPECT_NEAR(len, 1.0f, 0.001f);
    }
}

// ============================================================================
// Performance Test (CPU baseline)
// ============================================================================

TEST(FrustumCullingTest, CPUPerformance) {
    // Create frustum
    math::Mat4 persp = glm::perspective(glm::radians(60.0f), 16.0f/9.0f, 0.1f, 1000.0f);
    math::Mat4 view = glm::lookAt(
        math::Vec3(0.0f, 0.0f, 50.0f),
        math::Vec3(0.0f, 0.0f, 0.0f),
        math::Vec3(0.0f, 1.0f, 0.0f)
    );
    auto frustum = FrustumCuller::extract_frustum_planes(persp * view);

    // Create 10000 random objects
    std::vector<ObjectBounds> bounds;
    bounds.reserve(10000);

    for (int i = 0; i < 10000; ++i) {
        float x = (i % 100) * 2.0f - 100.0f;
        float y = ((i / 100) % 100) * 2.0f - 100.0f;
        float z = (i / 10000) * 2.0f - 50.0f;

        bounds.emplace_back(
            math::Vec3(x, y, z),
            math::Vec3(x + 1.0f, y + 1.0f, z + 1.0f),
            static_cast<uint32_t>(i)
        );
    }

    // Time CPU culling
    auto start = std::chrono::high_resolution_clock::now();
    auto visible = FrustumCuller::cull_cpu(bounds, frustum);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<float, std::milli>(end - start).count();

    std::cout << "CPU frustum culling: " << bounds.size() << " objects, "
              << visible.size() << " visible, " << duration << " ms" << std::endl;

    // Should complete reasonably fast (< 10ms for 10K objects on modern CPU)
    EXPECT_LT(duration, 100.0f);  // Very generous limit for CI
}

// ============================================================================
// Occlusion Culler Tests
// ============================================================================

TEST(OcclusionCullerTest, Creation) {
    OcclusionCuller culler;
    EXPECT_FALSE(culler.is_initialized());
}

TEST(OcclusionCullerTest, ConfigDefaults) {
    OcclusionCuller culler;
    const auto& config = culler.get_config();

    EXPECT_EQ(config.max_mip_levels, 10u);
    EXPECT_TRUE(config.use_previous_frame_depth);
    EXPECT_FLOAT_EQ(config.depth_bias, 0.001f);
}

TEST(OcclusionCullerTest, CPUOcclusionCulling) {
    // Create orthographic view-projection matrix for simpler testing
    math::Mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
    math::Mat4 view = glm::lookAt(
        math::Vec3(0.0f, 0.0f, 50.0f),
        math::Vec3(0.0f, 0.0f, 0.0f),
        math::Vec3(0.0f, 1.0f, 0.0f)
    );
    math::Mat4 view_proj = ortho * view;

    // Create depth buffer - all very far (depth = 1.0)
    uint32_t width = 64;
    uint32_t height = 64;
    std::vector<float> depth_buffer(width * height, 1.0f);

    // Create test objects - all should be visible with depth = 1.0
    std::vector<ObjectBounds> bounds;

    // Object 0: At origin (visible)
    bounds.emplace_back(math::Vec3(-1.0f, -1.0f, -1.0f), math::Vec3(1.0f, 1.0f, 1.0f), 0);

    // Object 1: Closer to camera (visible)
    bounds.emplace_back(math::Vec3(-1.0f, -1.0f, 40.0f), math::Vec3(1.0f, 1.0f, 42.0f), 1);

    // Object 2: Off to the side (visible)
    bounds.emplace_back(math::Vec3(5.0f, 5.0f, 0.0f), math::Vec3(6.0f, 6.0f, 1.0f), 2);

    // All objects start as frustum visible
    std::vector<uint32_t> frustum_visible = {0, 1, 2};

    // Run CPU occlusion culling
    auto visible = OcclusionCuller::cull_cpu(bounds, frustum_visible, view_proj, depth_buffer, width, height);

    // With all depth = 1.0, all objects should be visible (nothing occluded)
    EXPECT_EQ(visible.size(), 3u);
}

TEST(OcclusionCullerTest, HiZConfigModification) {
    OcclusionCuller culler;

    OcclusionCuller::HiZConfig new_config;
    new_config.max_mip_levels = 8;
    new_config.depth_bias = 0.002f;
    new_config.use_previous_frame_depth = false;

    culler.set_config(new_config);
    const auto& config = culler.get_config();

    EXPECT_EQ(config.max_mip_levels, 8u);
    EXPECT_FLOAT_EQ(config.depth_bias, 0.002f);
    EXPECT_FALSE(config.use_previous_frame_depth);
}

TEST(OcclusionCullerTest, StatsInitialization) {
    OcclusionCuller culler;
    const auto& stats = culler.get_stats();

    EXPECT_EQ(stats.objects_tested, 0u);
    EXPECT_EQ(stats.objects_occluded, 0u);
    EXPECT_FLOAT_EQ(stats.hiz_build_time_ms, 0.0f);
    EXPECT_FLOAT_EQ(stats.occlusion_test_time_ms, 0.0f);
}

// ============================================================================
// GPU Culling Pipeline with Occlusion Tests
// ============================================================================

TEST(GPUCullingPipelineTest, OcclusionConfigEnabled) {
    GPUCullingPipeline pipeline;

    // Enable occlusion culling
    pipeline.enable_occlusion_culling(true);
    EXPECT_TRUE(pipeline.get_config().enable_occlusion_culling);

    // Disable occlusion culling
    pipeline.enable_occlusion_culling(false);
    EXPECT_FALSE(pipeline.get_config().enable_occlusion_culling);
}

TEST(GPUCullingPipelineTest, ScreenDimensions) {
    GPUCullingPipeline pipeline;

    pipeline.set_screen_dimensions(1920, 1080);
    // Note: We can't directly access private members, but this should not crash
    // and the values should be used when occlusion culling runs
}

TEST(GPUCullingPipelineTest, OcclusionCullerAccess) {
    GPUCullingPipeline pipeline;

    // Before initialization, occlusion culler should be null
    EXPECT_EQ(pipeline.get_occlusion_culler(), nullptr);
}

// ============================================================================
// Indirect Renderer Tests
// ============================================================================

TEST(IndirectRendererTest, Creation) {
    IndirectRenderer renderer;
    EXPECT_FALSE(renderer.is_initialized());
}

TEST(IndirectRendererTest, ConfigDefaults) {
    IndirectRenderer renderer;
    const auto& config = renderer.get_config();

    EXPECT_EQ(config.max_objects, 100000u);
    EXPECT_EQ(config.max_draw_calls, 100000u);
    EXPECT_EQ(config.max_vertices, 10000000u);
    EXPECT_EQ(config.max_indices, 30000000u);
    EXPECT_TRUE(config.enable_draw_compaction);
    EXPECT_FALSE(config.enable_lod);
}

TEST(IndirectRendererTest, ConfigModification) {
    IndirectRenderer renderer;

    IndirectRenderer::Config new_config;
    new_config.max_objects = 50000;
    new_config.max_draw_calls = 50000;
    new_config.enable_lod = true;

    renderer.set_config(new_config);
    const auto& config = renderer.get_config();

    EXPECT_EQ(config.max_objects, 50000u);
    EXPECT_EQ(config.max_draw_calls, 50000u);
    EXPECT_TRUE(config.enable_lod);
}

TEST(IndirectRendererTest, StatsInitialization) {
    IndirectRenderer renderer;
    const auto& stats = renderer.get_stats();

    EXPECT_EQ(stats.total_objects, 0u);
    EXPECT_EQ(stats.visible_objects, 0u);
    EXPECT_EQ(stats.draw_calls, 0u);
    EXPECT_EQ(stats.total_vertices, 0u);
    EXPECT_EQ(stats.total_indices, 0u);
    EXPECT_FLOAT_EQ(stats.generate_draws_time_ms, 0.0f);
    EXPECT_FLOAT_EQ(stats.render_time_ms, 0.0f);
}

// ============================================================================
// Draw Command Types Tests
// ============================================================================

TEST(CullingTypesTest, DrawIndexedIndirectCommandSize) {
    // VkDrawIndexedIndirectCommand is 20 bytes (5 x uint32_t)
    EXPECT_EQ(sizeof(DrawIndexedIndirectCommand), 20u);
}

TEST(CullingTypesTest, ObjectDrawInfoSize) {
    // ObjectDrawInfo should be 32 bytes (aligned for GPU)
    EXPECT_EQ(sizeof(ObjectDrawInfo), 32u);
}
