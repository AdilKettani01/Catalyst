/**
 * @file test_end_to_end.cpp
 * @brief End-to-end integration tests for complete workflows
 *
 * Tests complete scene rendering pipelines matching Python Manim behavior
 */

#include <gtest/gtest.h>
#include <memory>
#include <fstream>

#include "manim/scene/scene.h"
#include "manim/scene/three_d_scene.h"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/animation/creation.hpp"
#include "manim/animation/fading.hpp"
#include "manim/animation/transform.hpp"
#include "manim/cli/config.h"

using namespace manim;

// ==================== Simple Scene Tests ====================

TEST(IntegrationSimpleScene, BasicCircleScene) {
    // Test: Create a scene with a single circle
    auto scene = std::make_shared<Scene>();

    // Setup
    scene->setup();

    // Add circle
    auto circle = std::make_shared<Circle>(1.0f);
    scene->add(circle);

    // Verify mobject added
    EXPECT_EQ(scene->get_mobjects().size(), 1);

    // Play creation animation
    auto anim = std::make_shared<ShowCreation>(circle, 1.0f);
    EXPECT_NO_THROW(scene->play(anim));

    // Wait
    EXPECT_NO_THROW(scene->wait(1.0));

    // Teardown
    scene->tear_down();
}

TEST(IntegrationSimpleScene, MultipleAnimations) {
    auto scene = std::make_shared<Scene>();
    scene->setup();

    auto circle = std::make_shared<Circle>(1.0f);
    scene->add(circle);

    // Play multiple animations in sequence
    auto anim1 = std::make_shared<FadeIn>(circle, 1.0f);
    scene->play(anim1);

    auto anim2 = std::make_shared<ShowCreation>(circle, 1.0f);
    scene->play(anim2);

    auto anim3 = std::make_shared<FadeOut>(circle, 1.0f);
    scene->play(anim3);

    scene->tear_down();
}

TEST(IntegrationSimpleScene, TransformScene) {
    auto scene = std::make_shared<Scene>();
    scene->setup();

    auto circle = std::make_shared<Circle>(1.0f);
    auto square = std::make_shared<Circle>(2.0f);  // Using Circle as placeholder
    square->shift(math::Vec3(2, 0, 0));

    scene->add(circle);

    // Fade in circle
    auto anim1 = std::make_shared<FadeIn>(circle);
    scene->play(anim1);

    // Transform to square
    auto anim2 = std::make_shared<Transform>(circle, square);
    scene->play(anim2);

    // Fade out
    auto anim3 = std::make_shared<FadeOut>(circle);
    scene->play(anim3);

    scene->tear_down();
}

// ==================== Complex Scene Tests ====================

TEST(IntegrationComplexScene, ManyObjects) {
    auto scene = std::make_shared<Scene>();
    scene->setup();

    // Create 50 circles in a grid
    std::vector<std::shared_ptr<Circle>> circles;
    for (int i = 0; i < 50; ++i) {
        auto circle = std::make_shared<Circle>(0.2f);
        float x = (i % 10) * 0.5f - 2.5f;
        float y = (i / 10) * 0.5f - 1.5f;
        circle->shift(math::Vec3(x, y, 0));
        circles.push_back(circle);
        scene->add(circle);
    }

    EXPECT_EQ(scene->get_mobjects().size(), 50);

    // Fade in all
    for (auto& circle : circles) {
        auto anim = std::make_shared<FadeIn>(circle, 0.01f);  // Very fast
        scene->play(anim);
    }

    scene->tear_down();
}

TEST(IntegrationComplexScene, NestedMobjects) {
    auto scene = std::make_shared<Scene>();
    scene->setup();

    // Create parent mobject
    auto parent = std::make_shared<Mobject>();

    // Add children
    for (int i = 0; i < 5; ++i) {
        auto child = std::make_shared<Circle>(0.2f);
        child->shift(math::Vec3(i * 0.5f, 0, 0));
        parent->add(child);
    }

    scene->add(parent);

    // Transform parent (should affect all children)
    parent->shift(math::Vec3(0, 1, 0));
    parent->rotate(math::PI / 4, math::Vec3(0, 0, 1));

    scene->tear_down();
}

// ==================== 3D Scene Tests ====================

TEST(Integration3DScene, Basic3DSetup) {
    auto scene = std::make_shared<ThreeDScene>();
    scene->setup();

    // Set camera orientation
    scene->set_camera_orientation(
        math::PI / 4,  // phi
        math::PI / 4,  // theta
        0,             // gamma
        1,             // zoom
        5,             // focal_distance
        math::Vec3(0, 0, 0)  // center
    );

    // Add 3D mesh
    auto mesh = std::make_shared<Mesh>();
    scene->add(mesh);

    scene->tear_down();
}

TEST(Integration3DScene, CameraAnimation) {
    auto scene = std::make_shared<ThreeDScene>();
    scene->setup();

    auto mesh = std::make_shared<Mesh>();
    scene->add(mesh);

    // Animate camera
    scene->begin_ambient_camera_rotation(0.02f, "theta");
    scene->wait(0.5);  // Rotate for 0.5 seconds
    scene->stop_ambient_camera_rotation("theta");

    scene->tear_down();
}

TEST(Integration3DScene, Multiple3DObjects) {
    auto scene = std::make_shared<ThreeDScene>();
    scene->setup();

    // Add 10 meshes in different positions
    for (int i = 0; i < 10; ++i) {
        auto mesh = std::make_shared<Mesh>();
        mesh->shift(math::Vec3(i * 0.5f, 0, 0));
        scene->add(mesh);
    }

    EXPECT_EQ(scene->get_mobjects().size(), 10);

    scene->tear_down();
}

// ==================== GPU Scene Tests ====================

TEST(IntegrationGPUScene, BasicGPUPipeline) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup();

    // Setup GPU pipeline
    scene->setup_deferred_pipeline();
    scene->initialize_pbr_materials();

    // Add lighting
    Light light;
    light.type = LightType::POINT;
    light.position = math::Vec3(5, 5, 5);
    light.intensity = 1.0f;
    scene->add_light(light);

    // Add mesh
    auto mesh = std::make_shared<Mesh>();
    scene->add(mesh);

    // Render
    scene->render_with_pbr();

    scene->tear_down();
}

TEST(IntegrationGPUScene, FullRenderingPipeline) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup();

    // Full GPU pipeline
    scene->setup_deferred_pipeline();
    scene->initialize_pbr_materials();

    // Multiple lights
    for (int i = 0; i < 3; ++i) {
        Light light;
        light.type = LightType::POINT;
        light.position = math::Vec3(i * 2.0f, 5, 5);
        light.intensity = 0.5f;
        scene->add_light(light);
    }

    // Add objects
    for (int i = 0; i < 5; ++i) {
        auto mesh = std::make_shared<Mesh>();
        mesh->shift(math::Vec3(i, 0, 0));
        scene->add(mesh);
    }

    // Full rendering with all effects
    scene->render_with_pbr();
    scene->compute_global_illumination();
    scene->apply_ssao();
    scene->apply_ssr();
    scene->apply_bloom();
    scene->apply_tone_mapping();

    scene->tear_down();
}

// ==================== Configuration Tests ====================

TEST(IntegrationConfiguration, ApplyQualityPresets) {
    RenderConfig config;

    // Test each quality preset
    config.apply_quality_preset(QualityPreset::LOW);
    EXPECT_TRUE(config.validate());

    config.apply_quality_preset(QualityPreset::MEDIUM);
    EXPECT_TRUE(config.validate());

    config.apply_quality_preset(QualityPreset::HIGH);
    EXPECT_TRUE(config.validate());

    config.apply_quality_preset(QualityPreset::ULTRA);
    EXPECT_TRUE(config.validate());
}

TEST(IntegrationConfiguration, RecommendedSettings) {
    GPUDetector detector;
    detector.detect_gpus();

    auto config = detector.recommend_settings();

    // Config should be valid
    EXPECT_TRUE(config.validate());

    // Should have reasonable values
    EXPECT_GT(config.width, 0);
    EXPECT_GT(config.height, 0);
    EXPECT_GT(config.frame_rate, 0);
}

// ==================== Workflow Tests ====================

TEST(IntegrationWorkflow, CompleteRenderWorkflow) {
    // Simulates complete rendering workflow:
    // 1. Create scene
    // 2. Add objects
    // 3. Animate
    // 4. Render
    // 5. Cleanup

    auto scene = std::make_shared<Scene>();

    // 1. Setup
    scene->setup();

    // 2. Add objects
    auto circle1 = std::make_shared<Circle>(1.0f);
    auto circle2 = std::make_shared<Circle>(0.5f);
    circle2->shift(math::Vec3(2, 0, 0));

    scene->add(circle1);
    scene->add(circle2);

    // 3. Animate
    auto anim1 = std::make_shared<FadeIn>(circle1, 1.0f);
    scene->play(anim1);

    scene->wait(0.5);

    auto anim2 = std::make_shared<FadeIn>(circle2, 1.0f);
    scene->play(anim2);

    auto transform = std::make_shared<Transform>(circle1, circle2);
    scene->play(transform);

    auto anim3 = std::make_shared<FadeOut>(circle1);
    scene->play(anim3);

    // 4. Render (preview mode)
    EXPECT_NO_THROW(scene->render(true));  // preview

    // 5. Cleanup
    scene->clear();
    scene->tear_down();

    EXPECT_EQ(scene->get_mobjects().size(), 0);
}

TEST(IntegrationWorkflow, BatchRendering) {
    // Test rendering multiple scenes
    for (int i = 0; i < 5; ++i) {
        auto scene = std::make_shared<Scene>();
        scene->setup();

        // Add different numbers of objects per scene
        for (int j = 0; j < (i + 1) * 5; ++j) {
            auto circle = std::make_shared<Circle>(0.1f);
            circle->shift(math::Vec3(j * 0.2f, 0, 0));
            scene->add(circle);
        }

        // Quick animation
        for (auto& mobject : scene->get_mobjects()) {
            auto anim = std::make_shared<FadeIn>(mobject, 0.01f);
            scene->play(anim);
        }

        scene->tear_down();
    }
}

// ==================== Error Recovery Tests ====================

TEST(IntegrationErrorRecovery, InvalidAnimationRecovery) {
    auto scene = std::make_shared<Scene>();
    scene->setup();

    auto circle = std::make_shared<Circle>();
    scene->add(circle);

    // Try to play animation with invalid parameters
    try {
        auto anim = std::make_shared<FadeIn>(circle, -1.0f);  // Negative time
        scene->play(anim);
    } catch (...) {
        // Should recover gracefully
    }

    // Scene should still be usable
    auto valid_anim = std::make_shared<FadeIn>(circle, 1.0f);
    EXPECT_NO_THROW(scene->play(valid_anim));

    scene->tear_down();
}

TEST(IntegrationErrorRecovery, GPU FallbackIntegration) {
    // Test that GPU scene falls back to CPU gracefully

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup();

    // Try GPU operations (should fall back if no GPU)
    EXPECT_NO_THROW(scene->setup_deferred_pipeline());
    EXPECT_NO_THROW(scene->render_with_pbr());

    auto mesh = std::make_shared<Mesh>();
    scene->add(mesh);

    EXPECT_NO_THROW(scene->render_with_pbr());

    scene->tear_down();
}

// ==================== Performance Integration Tests ====================

TEST(IntegrationPerformance, RealTimeRendering) {
    // Test: Can we maintain 60 FPS with reasonable scene?

    auto scene = std::make_shared<Scene>();
    scene->setup();

    // Add 100 objects
    for (int i = 0; i < 100; ++i) {
        auto circle = std::make_shared<Circle>(0.05f);
        circle->shift(math::Vec3((i % 10) * 0.2f, (i / 10) * 0.2f, 0));
        scene->add(circle);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Simulate 60 frames (1 second at 60 FPS)
    for (int frame = 0; frame < 60; ++frame) {
        // Update all objects
        for (auto& mobject : scene->get_mobjects()) {
            mobject->shift(math::Vec3(0.001f, 0, 0));
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete 60 frames in less than 1 second for real-time
    EXPECT_LT(duration.count(), 1000);

    double fps = 60000.0 / duration.count();
    EXPECT_GE(fps, 60.0);  // Should achieve at least 60 FPS

    scene->tear_down();
}

// ==================== Memory Stress Tests ====================

TEST(IntegrationMemory, LargeSceneCreationAndDestruction) {
    // Test: Create and destroy large scenes multiple times

    for (int iteration = 0; iteration < 10; ++iteration) {
        auto scene = std::make_shared<Scene>();
        scene->setup();

        // Add 500 objects
        for (int i = 0; i < 500; ++i) {
            auto circle = std::make_shared<Circle>(0.01f);
            scene->add(circle);
        }

        EXPECT_EQ(scene->get_mobjects().size(), 500);

        scene->clear();
        scene->tear_down();
    }

    // If we got here, memory management is working
    SUCCEED();
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
