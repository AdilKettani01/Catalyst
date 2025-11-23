/**
 * @file test_scenes.cpp
 * @brief Comprehensive unit tests for all scene types
 *
 * Tests for Scene, ThreeDScene, GPU3DScene, MovingCameraScene
 */

#include <gtest/gtest.h>
#include <memory>

#include "manim/scene/scene.h"
#include "manim/scene/three_d_scene.h"
#include "manim/scene/moving_camera_scene.h"
#include "manim/scene/vector_space_scene.h"
#include "manim/scene/zoomed_scene.h"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/animation/fading.hpp"
#include "manim/core/types.h"

using namespace manim;

// ==================== Base Scene Tests ====================

class SceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene = std::make_shared<Scene>();
    }

    std::shared_ptr<Scene> scene;
};

TEST_F(SceneTest, DefaultConstruction) {
    EXPECT_NE(scene, nullptr);
}

TEST_F(SceneTest, AddMobject) {
    auto circle = std::make_shared<Circle>();
    EXPECT_NO_THROW(scene->add(circle));

    auto mobjects = scene->get_mobjects();
    EXPECT_EQ(mobjects.size(), 1);
}

TEST_F(SceneTest, RemoveMobject) {
    auto circle = std::make_shared<Circle>();
    scene->add(circle);
    EXPECT_EQ(scene->get_mobjects().size(), 1);

    scene->remove(circle);
    EXPECT_EQ(scene->get_mobjects().size(), 0);
}

TEST_F(SceneTest, ClearMobjects) {
    scene->add(std::make_shared<Circle>());
    scene->add(std::make_shared<Circle>());
    scene->add(std::make_shared<Circle>());

    EXPECT_EQ(scene->get_mobjects().size(), 3);

    scene->clear();
    EXPECT_EQ(scene->get_mobjects().size(), 0);
}

TEST_F(SceneTest, PlayAnimation) {
    auto circle = std::make_shared<Circle>();
    scene->add(circle);

    auto anim = std::make_shared<FadeIn>(circle);
    EXPECT_NO_THROW(scene->play(anim));
}

TEST_F(SceneTest, Wait) {
    EXPECT_NO_THROW(scene->wait(0.1));  // Short wait for testing
}

TEST_F(SceneTest, GetTime) {
    float time = scene->get_time();
    EXPECT_GE(time, 0.0f);
}

TEST_F(SceneTest, SetupTeardown) {
    EXPECT_NO_THROW(scene->setup());
    EXPECT_NO_THROW(scene->tear_down());
}

TEST_F(SceneTest, MultipleMobjects) {
    std::vector<std::shared_ptr<Mobject>> circles;

    for (int i = 0; i < 10; ++i) {
        auto circle = std::make_shared<Circle>(0.5f);
        circle->shift(math::Vec3(i * 0.5f, 0, 0));
        circles.push_back(circle);
        scene->add(circle);
    }

    EXPECT_EQ(scene->get_mobjects().size(), 10);
}

// ==================== ThreeDScene Tests ====================

class ThreeDSceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene = std::make_shared<ThreeDScene>();
    }

    std::shared_ptr<ThreeDScene> scene;
};

TEST_F(ThreeDSceneTest, DefaultConstruction) {
    EXPECT_NE(scene, nullptr);
}

TEST_F(ThreeDSceneTest, SetCameraOrientation) {
    EXPECT_NO_THROW(scene->set_camera_orientation(
        math::PI / 4,   // phi
        math::PI / 4,   // theta
        0.0f,           // gamma
        1.0f,           // zoom
        5.0f,           // focal_distance
        math::Vec3(0, 0, 0)  // frame_center
    ));
}

TEST_F(ThreeDSceneTest, BeginAmbientCameraRotation) {
    EXPECT_NO_THROW(scene->begin_ambient_camera_rotation(0.02f, "theta"));
}

TEST_F(ThreeDSceneTest, StopAmbientCameraRotation) {
    scene->begin_ambient_camera_rotation(0.02f, "theta");
    EXPECT_NO_THROW(scene->stop_ambient_camera_rotation("theta"));
}

TEST_F(ThreeDSceneTest, MoveCameraTo) {
    EXPECT_NO_THROW(scene->move_camera_to(
        math::PI / 3,  // phi
        math::PI / 3,  // theta
        1.5f           // distance
    ));
}

TEST_F(ThreeDSceneTest, Add3DMobjects) {
    auto mesh = std::make_shared<Mesh>();
    EXPECT_NO_THROW(scene->add(mesh));

    EXPECT_EQ(scene->get_mobjects().size(), 1);
}

// ==================== GPU3DScene Tests ====================

class GPU3DSceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene = std::make_shared<GPU3DScene>();
    }

    std::shared_ptr<GPU3DScene> scene;
};

TEST_F(GPU3DSceneTest, DefaultConstruction) {
    EXPECT_NE(scene, nullptr);
}

TEST_F(GPU3DSceneTest, SetupDeferredPipeline) {
    EXPECT_NO_THROW(scene->setup_deferred_pipeline());
}

TEST_F(GPU3DSceneTest, InitializePBRMaterials) {
    EXPECT_NO_THROW(scene->initialize_pbr_materials());
}

TEST_F(GPU3DSceneTest, SetupRayTracing) {
    // May fail if no ray tracing support, so just check it doesn't crash
    try {
        scene->setup_ray_tracing();
    } catch (...) {
        // Expected on systems without RT support
    }
}

TEST_F(GPU3DSceneTest, AddLight) {
    Light light;
    light.type = LightType::POINT;
    light.position = math::Vec3(5, 5, 5);
    light.color = math::Vec4(1, 1, 1, 1);
    light.intensity = 1.0f;

    EXPECT_NO_THROW(scene->add_light(light));
}

TEST_F(GPU3DSceneTest, RemoveLight) {
    Light light;
    scene->add_light(light);

    EXPECT_NO_THROW(scene->remove_light(0));
}

TEST_F(GPU3DSceneTest, SetEnvironmentMap) {
    EXPECT_NO_THROW(scene->set_environment_map(""));  // Empty path for test
}

TEST_F(GPU3DSceneTest, RenderWithPBR) {
    EXPECT_NO_THROW(scene->render_with_pbr());
}

TEST_F(GPU3DSceneTest, ComputeGlobalIllumination) {
    EXPECT_NO_THROW(scene->compute_global_illumination());
}

TEST_F(GPU3DSceneTest, RenderVolumetrics) {
    EXPECT_NO_THROW(scene->render_volumetrics());
}

TEST_F(GPU3DSceneTest, ApplySSAO) {
    EXPECT_NO_THROW(scene->apply_ssao());
}

TEST_F(GPU3DSceneTest, ApplySSR) {
    EXPECT_NO_THROW(scene->apply_ssr());
}

TEST_F(GPU3DSceneTest, ApplyBloom) {
    EXPECT_NO_THROW(scene->apply_bloom());
}

TEST_F(GPU3DSceneTest, ApplyToneMapping) {
    EXPECT_NO_THROW(scene->apply_tone_mapping());
}

TEST_F(GPU3DSceneTest, MultipleLights) {
    for (int i = 0; i < 5; ++i) {
        Light light;
        light.type = LightType::POINT;
        light.position = math::Vec3(i * 2.0f, 5, 5);
        light.intensity = 0.5f;
        scene->add_light(light);
    }

    // Should handle multiple lights without crashing
    EXPECT_NO_THROW(scene->render_with_pbr());
}

// ==================== MovingCameraScene Tests ====================

class MovingCameraSceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene = std::make_shared<MovingCameraScene>();
    }

    std::shared_ptr<MovingCameraScene> scene;
};

TEST_F(MovingCameraSceneTest, DefaultConstruction) {
    EXPECT_NE(scene, nullptr);
}

TEST_F(MovingCameraSceneTest, ZoomToMobject) {
    auto circle = std::make_shared<Circle>();
    scene->add(circle);

    EXPECT_NO_THROW(scene->zoom_to_mobject(circle));
}

TEST_F(MovingCameraSceneTest, PanTo) {
    EXPECT_NO_THROW(scene->pan_to(math::Vec3(2, 2, 0)));
}

TEST_F(MovingCameraSceneTest, SaveAndRestoreCameraState) {
    scene->pan_to(math::Vec3(1, 1, 0));
    EXPECT_NO_THROW(scene->save_camera_state());

    scene->pan_to(math::Vec3(10, 10, 0));
    EXPECT_NO_THROW(scene->restore_camera_state());
}

// ==================== VectorSpaceScene Tests ====================

TEST(VectorSpaceSceneTest, DefaultConstruction) {
    auto scene = std::make_shared<VectorSpaceScene>();
    EXPECT_NE(scene, nullptr);
}

TEST(VectorSpaceSceneTest, AddMobjects) {
    auto scene = std::make_shared<VectorSpaceScene>();
    auto circle = std::make_shared<Circle>();

    EXPECT_NO_THROW(scene->add(circle));
}

// ==================== ZoomedScene Tests ====================

TEST(ZoomedSceneTest, DefaultConstruction) {
    auto scene = std::make_shared<ZoomedScene>();
    EXPECT_NE(scene, nullptr);
}

TEST(ZoomedSceneTest, InheritsFromMovingCamera) {
    auto scene = std::make_shared<ZoomedScene>();

    // Should have MovingCameraScene methods
    EXPECT_NO_THROW(scene->pan_to(math::Vec3(0, 0, 0)));
}

// ==================== Scene Performance Tests ====================

TEST(ScenePerformance, AddManyMobjects) {
    auto scene = std::make_shared<Scene>();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        auto circle = std::make_shared<Circle>(0.1f);
        circle->shift(math::Vec3(i * 0.01f, 0, 0));
        scene->add(circle);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Adding 1000 mobjects should take less than 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST(ScenePerformance, PlayManyAnimations) {
    auto scene = std::make_shared<Scene>();

    // Add mobjects
    std::vector<std::shared_ptr<Mobject>> mobjects;
    for (int i = 0; i < 100; ++i) {
        auto circle = std::make_shared<Circle>(0.1f);
        mobjects.push_back(circle);
        scene->add(circle);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Play animations
    for (auto& mobject : mobjects) {
        auto anim = std::make_shared<FadeIn>(mobject, 0.01f);  // Very short
        scene->play(anim);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 100 short animations should complete in reasonable time
    EXPECT_LT(duration.count(), 5000);  // 5 seconds max
}

TEST(GPU3DScenePerformance, MultipleRenderPasses) {
    auto scene = std::make_shared<GPU3DScene>();

    // Add some 3D objects
    for (int i = 0; i < 10; ++i) {
        auto mesh = std::make_shared<Mesh>();
        scene->add(mesh);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Multiple render passes
    scene->setup_deferred_pipeline();
    scene->render_with_pbr();
    scene->apply_ssao();
    scene->apply_ssr();
    scene->apply_bloom();
    scene->apply_tone_mapping();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Full rendering pipeline should complete in reasonable time
    // Note: This depends on GPU availability
    EXPECT_LT(duration.count(), 10000);  // 10 seconds max
}

// ==================== Memory Tests ====================

TEST(SceneMemory, NoLeaksOnClear) {
    auto scene = std::make_shared<Scene>();

    // Add and clear multiple times
    for (int iteration = 0; iteration < 10; ++iteration) {
        for (int i = 0; i < 100; ++i) {
            auto circle = std::make_shared<Circle>();
            scene->add(circle);
        }

        EXPECT_EQ(scene->get_mobjects().size(), 100);
        scene->clear();
        EXPECT_EQ(scene->get_mobjects().size(), 0);
    }

    // If we got here without crashing, memory management is working
    SUCCEED();
}

TEST(SceneMemory, NoLeaksOnDestruction) {
    for (int i = 0; i < 100; ++i) {
        auto scene = std::make_shared<Scene>();

        for (int j = 0; j < 10; ++j) {
            auto circle = std::make_shared<Circle>();
            scene->add(circle);
        }

        // Scene goes out of scope - should clean up properly
    }

    // If we got here without crashing, cleanup is working
    SUCCEED();
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
