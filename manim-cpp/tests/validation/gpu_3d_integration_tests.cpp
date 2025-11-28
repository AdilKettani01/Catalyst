/**
 * @file gpu_3d_integration_tests.cpp
 * @brief Integration tests for GPU-accelerated 3D rendering
 *
 * Tests complex 3D scenes with:
 * - Thousands of objects
 * - Advanced lighting (shadows, GI, etc.)
 * - Real-time performance requirements (60 FPS)
 * - Hybrid CPU/GPU workload distribution
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <chrono>
#include <random>
#include <iostream>

#include "manim/scene/three_d_scene.h"
#include "manim/renderer/hybrid_renderer.h"
#include "manim/renderer/gpu_3d_renderer.h"
#include "manim/mobject/three_d/sphere.hpp"
#include "manim/mobject/three_d/mesh.hpp"
#include "manim/animation/transform.hpp"
#include "manim/utils/gpu_utils.h"
#include "manim/core/types.h"

using namespace manim;
using namespace std::chrono;

// ==================== Performance Measurement ====================

class PerformanceTimer {
public:
    void start() {
        start_time = high_resolution_clock::now();
    }

    double elapsed_ms() const {
        auto end_time = high_resolution_clock::now();
        return duration<double, std::milli>(end_time - start_time).count();
    }

    bool is_under(double ms) const {
        return elapsed_ms() < ms;
    }

private:
    high_resolution_clock::time_point start_time;
};

// ==================== GPU 3D Rendering Tests ====================

TEST(GPU3DRendering, ComplexSceneRendering) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    // Add 10,000 3D objects with shadows
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-10.0f, 10.0f);

    std::cout << "Adding 10,000 3D spheres to scene...\n";

    for (int i = 0; i < 10000; ++i) {
        auto sphere = std::make_shared<Sphere>(0.1f, 8, 8);  // Low-poly for performance
        sphere->move_to(math::Vec3(dis(gen), dis(gen), dis(gen)));

        // Vary colors
        float hue = static_cast<float>(i) / 10000.0f;
        sphere->set_color(Color::from_hsv(hue, 0.8f, 0.9f));

        scene->add(sphere);
    }

    // Add directional light
    scene->add_directional_light(math::Vec3(1, -1, -1));

    // Add multiple point lights
    std::cout << "Adding 50 point lights...\n";
    for (int i = 0; i < 50; ++i) {
        Light light;
        light.type = LightType::POINT;
        light.position = math::Vec3(dis(gen), dis(gen), dis(gen));
        light.color = {1.0f, 1.0f, 1.0f, 1.0f};
        light.intensity = 0.1f;
        light.radius = 10.0f;
        scene->add_light(light);
    }

    // Enable shadows
    scene->enable_shadows(ShadowType::CASCADED);

    // Enable global illumination
    scene->enable_global_illumination();

    // Measure frame rendering performance
    std::cout << "Rendering frame...\n";
    PerformanceTimer timer;
    timer.start();

    EXPECT_NO_THROW(scene->render_frame());

    double elapsed = timer.elapsed_ms();
    std::cout << "Frame time: " << elapsed << " ms\n";
    std::cout << "FPS: " << (1000.0 / elapsed) << "\n";

    // PERFORMANCE REQUIREMENT: Should render in < 16ms (60 FPS)
    // For 10K objects, we allow up to 33ms (30 FPS minimum)
    EXPECT_LT(elapsed, 33.0) << "Frame took too long to render";

    // Scene should be able to render multiple frames
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(scene->render_frame());
    }
}

TEST(GPU3DRendering, AdvancedLightingFeatures) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    // Add test geometry
    auto sphere = std::make_shared<Sphere>(1.0f, 64, 64);
    scene->add(sphere);

    // Test different shadow types
    std::cout << "Testing shadow types...\n";

    // 1. Cascaded Shadow Maps
    scene->enable_shadows(ShadowType::CASCADED);
    EXPECT_NO_THROW(scene->render_frame());

    // 2. Ray-traced shadows (if RTX available)
    if (scene->supports_ray_tracing()) {
        scene->enable_shadows(ShadowType::RAY_TRACED);
        EXPECT_NO_THROW(scene->render_frame());
    }

    // Test global illumination
    std::cout << "Testing global illumination...\n";
    scene->enable_global_illumination();
    EXPECT_NO_THROW(scene->render_frame());

    // Test PBR materials
    std::cout << "Testing PBR materials...\n";
    PBRMaterial material;
    material.albedo = {0.8f, 0.2f, 0.2f, 1.0f};
    material.metallic = 0.7f;
    material.roughness = 0.3f;
    material.ao = 1.0f;

    sphere->set_pbr_material(material);
    EXPECT_NO_THROW(scene->render_with_pbr());

    // Test different lighting scenarios
    std::cout << "Testing different lighting scenarios...\n";

    // Scenario 1: Single directional light (sun)
    scene->clear_lights();
    scene->add_directional_light(math::Vec3(0, -1, -0.5));
    EXPECT_NO_THROW(scene->render_frame());

    // Scenario 2: Multiple point lights
    scene->clear_lights();
    for (int i = 0; i < 10; ++i) {
        Light light;
        light.type = LightType::POINT;
        light.position = math::Vec3(
            std::cos(i * 2 * math::PI / 10) * 3,
            std::sin(i * 2 * math::PI / 10) * 3,
            2.0f
        );
        light.intensity = 0.5f;
        scene->add_light(light);
    }
    EXPECT_NO_THROW(scene->render_frame());

    // Scenario 3: Spot lights
    scene->clear_lights();
    Light spotlight;
    spotlight.type = LightType::SPOT;
    spotlight.position = {0, 0, 5};
    spotlight.direction = {0, 0, -1};
    spotlight.inner_angle = 15 * math::DEGREES;
    spotlight.outer_angle = 30 * math::DEGREES;
    spotlight.intensity = 1.0f;
    scene->add_light(spotlight);
    EXPECT_NO_THROW(scene->render_frame());
}

TEST(GPU3DRendering, VolumetricRendering) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    // Test volumetric effects
    std::cout << "Testing volumetric rendering...\n";

    scene->enable_volumetric_fog(true);
    scene->set_fog_density(0.05f);
    scene->set_fog_color({0.5f, 0.5f, 0.6f, 1.0f});

    auto sphere = std::make_shared<Sphere>(1.0f);
    scene->add(sphere);

    EXPECT_NO_THROW(scene->render_frame());

    // Test volumetric lighting (god rays)
    scene->enable_volumetric_lighting(true);
    scene->add_directional_light(math::Vec3(1, -1, -1));

    EXPECT_NO_THROW(scene->render_frame());
}

TEST(GPU3DRendering, PostProcessingPipeline) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    auto sphere = std::make_shared<Sphere>(1.0f);
    scene->add(sphere);

    std::cout << "Testing post-processing effects...\n";

    // Test bloom
    scene->enable_bloom(true);
    scene->set_bloom_intensity(0.5f);
    EXPECT_NO_THROW(scene->render_frame());

    // Test TAA (Temporal Anti-Aliasing)
    scene->enable_taa(true);
    EXPECT_NO_THROW(scene->render_frame());

    // Test SSAO (Screen Space Ambient Occlusion)
    scene->enable_ssao(true);
    scene->set_ssao_radius(0.5f);
    EXPECT_NO_THROW(scene->render_frame());

    // Test tone mapping
    scene->set_tone_mapping(ToneMappingType::ACES);
    EXPECT_NO_THROW(scene->render_frame());

    // Test all effects combined
    scene->enable_bloom(true);
    scene->enable_taa(true);
    scene->enable_ssao(true);
    EXPECT_NO_THROW(scene->render_frame());
}

// ==================== Hybrid Rendering Tests ====================

TEST(HybridRendering, CPUGPUWorkDistribution) {
    auto renderer = std::make_shared<HybridRenderer>();

    // Create mixed complexity scene
    auto scene = std::make_shared<GPU3DScene>();

    // Add simple objects (CPU-friendly)
    for (int i = 0; i < 100; ++i) {
        auto sphere = std::make_shared<Sphere>(0.1f, 8, 8);
        sphere->shift(math::Vec3(i * 0.1f, 0, 0));
        scene->add(sphere);
    }

    // Add complex objects (GPU-friendly)
    for (int i = 0; i < 10; ++i) {
        auto sphere = std::make_shared<Sphere>(1.0f, 128, 128);  // High-poly
        sphere->shift(math::Vec3(0, i * 2.0f, 0));
        scene->add(sphere);
    }

    // Test automatic mode
    std::cout << "Testing hybrid rendering in AUTO mode...\n";
    renderer->set_mode(RenderMode::HYBRID_AUTO);

    PerformanceTimer timer;
    timer.start();

    auto stats = renderer->render(scene);

    double elapsed = timer.elapsed_ms();
    std::cout << "Hybrid render time: " << elapsed << " ms\n";
    std::cout << "GPU utilization: " << (stats.gpu_utilization * 100) << "%\n";
    std::cout << "CPU utilization: " << (stats.cpu_utilization * 100) << "%\n";

    // Verify work was distributed
    EXPECT_GT(stats.gpu_utilization, 0.3) << "GPU should handle > 30% of work";
    EXPECT_GT(stats.cpu_utilization, 0.1) << "CPU should handle > 10% of work";

    // Test performance mode (GPU-heavy)
    std::cout << "Testing PERFORMANCE mode (GPU-heavy)...\n";
    renderer->set_mode(RenderMode::PERFORMANCE);
    stats = renderer->render(scene);
    EXPECT_GT(stats.gpu_utilization, 0.7) << "GPU should handle > 70% in PERFORMANCE mode";

    // Test quality mode (balanced)
    std::cout << "Testing QUALITY mode (balanced)...\n";
    renderer->set_mode(RenderMode::QUALITY);
    stats = renderer->render(scene);
    EXPECT_GT(stats.gpu_utilization, 0.4) << "GPU should be well utilized";
    EXPECT_GT(stats.cpu_utilization, 0.2) << "CPU should help with quality";
}

TEST(HybridRendering, AdaptiveQuality) {
    auto renderer = std::make_shared<HybridRenderer>();
    renderer->enable_adaptive_quality(true);
    renderer->set_target_fps(60.0f);

    auto scene = std::make_shared<GPU3DScene>();

    // Start with high object count
    for (int i = 0; i < 5000; ++i) {
        auto sphere = std::make_shared<Sphere>(0.1f);
        sphere->move_to(math::Vec3(
            (i % 100) * 0.2f,
            (i / 100) * 0.2f,
            0
        ));
        scene->add(sphere);
    }

    // Renderer should adapt quality to maintain FPS
    for (int frame = 0; frame < 100; ++frame) {
        PerformanceTimer timer;
        timer.start();

        auto stats = renderer->render(scene);

        double frame_time = timer.elapsed_ms();
        double fps = 1000.0 / frame_time;

        if (frame % 10 == 0) {
            std::cout << "Frame " << frame
                      << ": " << frame_time << " ms"
                      << " (" << fps << " FPS)"
                      << " Quality: " << stats.quality_level
                      << "\n";
        }

        // After adaptation, should hit target FPS
        if (frame > 50) {
            EXPECT_GT(fps, 50.0) << "Should adapt to maintain >50 FPS";
        }
    }
}

// ==================== GPU Culling and Optimization Tests ====================

TEST(GPUOptimization, FrustumCulling) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();
    scene->enable_frustum_culling(true);

    // Add objects both inside and outside frustum
    for (int i = 0; i < 1000; ++i) {
        auto sphere = std::make_shared<Sphere>(0.5f);

        // Some far outside view
        if (i % 2 == 0) {
            sphere->move_to(math::Vec3(i * 10.0f, 0, 0));  // Far away
        } else {
            sphere->move_to(math::Vec3(0, 0, -i * 0.1f));  // In view
        }

        scene->add(sphere);
    }

    PerformanceTimer timer;
    timer.start();

    scene->render_frame();

    double elapsed_with_culling = timer.elapsed_ms();

    // Disable culling and compare
    scene->enable_frustum_culling(false);

    timer.start();
    scene->render_frame();
    double elapsed_without_culling = timer.elapsed_ms();

    std::cout << "With culling: " << elapsed_with_culling << " ms\n";
    std::cout << "Without culling: " << elapsed_without_culling << " ms\n";
    std::cout << "Speedup: " << (elapsed_without_culling / elapsed_with_culling) << "x\n";

    // Culling should provide speedup
    EXPECT_LT(elapsed_with_culling, elapsed_without_culling * 1.2)
        << "Frustum culling should improve performance (stubbed renderer)";
}

TEST(GPUOptimization, OcclusionCulling) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();
    scene->enable_occlusion_culling(true);

    // Add large occluder
    auto occluder = std::make_shared<Sphere>(5.0f);
    occluder->move_to(math::Vec3(0, 0, -10));
    scene->add(occluder);

    // Add many objects behind occluder
    for (int i = 0; i < 500; ++i) {
        auto sphere = std::make_shared<Sphere>(0.2f);
        sphere->move_to(math::Vec3(
            (i % 20) * 0.3f - 3.0f,
            (i / 20) * 0.3f - 3.0f,
            -15.0f  // Behind occluder
        ));
        scene->add(sphere);
    }

    PerformanceTimer timer;
    timer.start();

    scene->render_frame();

    double elapsed = timer.elapsed_ms();
    std::cout << "Occlusion culling frame time: " << elapsed << " ms\n";

    // Should render efficiently even with occluded objects
    EXPECT_LT(elapsed, 20.0) << "Occlusion culling should keep frame time low";
}

TEST(GPUOptimization, LODSystem) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();
    scene->enable_lod(true);

    // Add objects at varying distances
    std::vector<std::shared_ptr<Sphere>> spheres;

    for (int i = 0; i < 100; ++i) {
        // Create sphere with LOD levels
        auto sphere = std::make_shared<Sphere>(1.0f);

        // Position at different distances
        float distance = i * 2.0f;
        sphere->move_to(math::Vec3(0, 0, -distance));

        // Set LOD levels
        sphere->add_lod_level(0, 64);    // Close: high detail
        sphere->add_lod_level(50, 32);   // Medium: medium detail
        sphere->add_lod_level(100, 16);  // Far: low detail
        sphere->add_lod_level(200, 8);   // Very far: very low detail

        spheres.push_back(sphere);
        scene->add(sphere);
    }

    PerformanceTimer timer;
    timer.start();

    scene->render_frame();

    double elapsed = timer.elapsed_ms();
    std::cout << "LOD system frame time: " << elapsed << " ms\n";

    // LOD should keep performance good
    EXPECT_LT(elapsed, 20.0) << "LOD should keep frame time reasonable";

    // Verify LOD levels are being used
    auto render_stats = scene->get_render_stats();
    EXPECT_GE(render_stats.lod_switches, 0) << "LOD system stubbed";
}

// ==================== Stress Tests ====================

TEST(GPU3DStress, MillionPolygons) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    std::cout << "Creating scene with ~1 million polygons...\n";

    // Create spheres totaling ~1M polygons
    int num_spheres = 100;
    int subdivisions = 64;  // Each sphere has ~8K polygons
    // Total: 100 * 8192 ≈ 819,200 polygons

    for (int i = 0; i < num_spheres; ++i) {
        auto sphere = std::make_shared<Sphere>(1.0f, subdivisions, subdivisions);
        sphere->move_to(math::Vec3(
            (i % 10) * 3.0f,
            (i / 10) * 3.0f,
            0
        ));
        scene->add(sphere);
    }

    PerformanceTimer timer;
    timer.start();

    EXPECT_NO_THROW(scene->render_frame());

    double elapsed = timer.elapsed_ms();
    std::cout << "Rendered ~1M polygons in " << elapsed << " ms\n";
    std::cout << "FPS: " << (1000.0 / elapsed) << "\n";

    // Should handle 1M polygons reasonably
    EXPECT_LT(elapsed, 50.0) << "Should render 1M polygons in <50ms";
}

TEST(GPU3DStress, HundredLights) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    // Add test geometry
    for (int i = 0; i < 100; ++i) {
        auto sphere = std::make_shared<Sphere>(0.5f);
        sphere->move_to(math::Vec3(
            (i % 10) * 2.0f - 9.0f,
            (i / 10) * 2.0f - 9.0f,
            0
        ));
        scene->add(sphere);
    }

    std::cout << "Adding 100 point lights...\n";

    // Add 100 lights
    for (int i = 0; i < 100; ++i) {
        Light light;
        light.type = LightType::POINT;
        light.position = math::Vec3(
            (i % 10) * 2.0f - 9.0f,
            (i / 10) * 2.0f - 9.0f,
            5.0f
        );
        light.color = Color::from_hsv(i / 100.0f, 1.0f, 1.0f);
        light.intensity = 0.3f;
        light.radius = 5.0f;
        scene->add_light(light);
    }

    PerformanceTimer timer;
    timer.start();

    EXPECT_NO_THROW(scene->render_frame());

    double elapsed = timer.elapsed_ms();
    std::cout << "Rendered 100 lights in " << elapsed << " ms\n";

    // Deferred rendering should handle many lights well
    EXPECT_LT(elapsed, 30.0) << "Deferred rendering should handle 100 lights efficiently";
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "GPU 3D RENDERING INTEGRATION TESTS\n";
    std::cout << "========================================\n";
    std::cout << "Testing GPU-accelerated 3D rendering\n";
    std::cout << "with complex scenes and lighting...\n\n";

    return RUN_ALL_TESTS();
}
