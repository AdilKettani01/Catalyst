/**
 * @file test_animations.cpp
 * @brief Comprehensive unit tests for all animation types
 *
 * Tests for Creation, Fading, Transform animations with GPU support
 */

#include <gtest/gtest.h>
#include <memory>
#include <complex>
#include <cmath>

#include "manim/animation/animation.hpp"
#include "manim/animation/creation.hpp"
#include "manim/animation/fading.hpp"
#include "manim/animation/transform.hpp"
#include "manim/animation/composition.hpp"
#include "manim/animation/movement.hpp"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/core/types.h"

using namespace manim;

// ==================== Base Animation Tests ====================

class AnimationTest : public ::testing::Test {
protected:
    void SetUp() override {
        mobject = std::make_shared<Circle>(1.0f);
    }

    std::shared_ptr<Mobject> mobject;
};

// ==================== ShowCreation Tests ====================

TEST_F(AnimationTest, ShowCreationConstruction) {
    auto anim = std::make_shared<ShowCreation>(mobject);
    EXPECT_NE(anim, nullptr);
    EXPECT_EQ(anim->get_mobject(), mobject);
}

TEST_F(AnimationTest, ShowCreationRunTime) {
    auto anim = std::make_shared<ShowCreation>(mobject, 2.0f);
    EXPECT_FLOAT_EQ(anim->get_run_time(), 2.0f);

    anim->set_run_time(3.0f);
    EXPECT_FLOAT_EQ(anim->get_run_time(), 3.0f);
}

TEST_F(AnimationTest, ShowCreationGPUSupport) {
    auto anim = std::make_shared<ShowCreation>(mobject);
    EXPECT_TRUE(anim->can_run_on_gpu());
}

TEST_F(AnimationTest, ShowCreationInterpolation) {
    auto anim = std::make_shared<ShowCreation>(mobject);

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.0f));
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    EXPECT_NO_THROW(anim->interpolate(1.0f));
    anim->finish();
}

// ==================== Uncreate Tests ====================

TEST_F(AnimationTest, UncreateConstruction) {
    auto anim = std::make_shared<Uncreate>(mobject);
    EXPECT_NE(anim, nullptr);
}

TEST_F(AnimationTest, UncreateInterpolation) {
    auto anim = std::make_shared<Uncreate>(mobject);

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    anim->finish();
}

// ==================== Write Tests ====================

TEST_F(AnimationTest, WriteConstruction) {
    auto anim = std::make_shared<Write>(mobject);
    EXPECT_NE(anim, nullptr);
}

TEST_F(AnimationTest, WriteWithParameters) {
    auto anim = std::make_shared<Write>(
        mobject,
        2.0f,  // run_time
        3.0f,  // stroke_width
        math::Vec4(1, 0, 0, 1)  // stroke_color
    );
    EXPECT_FLOAT_EQ(anim->get_run_time(), 2.0f);
}

// ==================== Unwrite Tests ====================

TEST_F(AnimationTest, UnwriteConstruction) {
    auto anim = std::make_shared<Unwrite>(mobject);
    EXPECT_NE(anim, nullptr);
}

// ==================== FadeIn Tests ====================

TEST_F(AnimationTest, FadeInConstruction) {
    auto anim = std::make_shared<FadeIn>(mobject);
    EXPECT_NE(anim, nullptr);
    EXPECT_TRUE(anim->can_run_on_gpu());
}

TEST_F(AnimationTest, FadeInWithShift) {
    auto anim = std::make_shared<FadeIn>(
        mobject,
        1.0f,  // run_time
        1.0f,  // shift_amount
        math::Vec3(0, 1, 0)  // shift UP
    );

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    anim->finish();
}

TEST_F(AnimationTest, FadeInInterpolation) {
    auto anim = std::make_shared<FadeIn>(mobject);

    anim->begin();

    // At t=0, should be invisible
    anim->interpolate(0.0f);

    // At t=0.5, should be partially visible
    anim->interpolate(0.5f);

    // At t=1, should be fully visible
    anim->interpolate(1.0f);

    anim->finish();
}

// ==================== FadeOut Tests ====================

TEST_F(AnimationTest, FadeOutConstruction) {
    auto anim = std::make_shared<FadeOut>(mobject);
    EXPECT_NE(anim, nullptr);
    EXPECT_TRUE(anim->can_run_on_gpu());
}

TEST_F(AnimationTest, FadeOutInterpolation) {
    auto anim = std::make_shared<FadeOut>(mobject);

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.0f));
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    EXPECT_NO_THROW(anim->interpolate(1.0f));
    anim->finish();
}

// ==================== FadeInFromPoint Tests ====================

TEST_F(AnimationTest, FadeInFromPointConstruction) {
    auto anim = std::make_shared<FadeInFromPoint>(
        mobject,
        math::Vec3(2, 2, 0)
    );
    EXPECT_NE(anim, nullptr);
}

TEST_F(AnimationTest, FadeInFromPointInterpolation) {
    auto anim = std::make_shared<FadeInFromPoint>(
        mobject,
        math::Vec3(5, 5, 0),
        1.0f
    );

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    anim->finish();
}

// ==================== FadeOutToPoint Tests ====================

TEST_F(AnimationTest, FadeOutToPointConstruction) {
    auto anim = std::make_shared<FadeOutToPoint>(
        mobject,
        math::Vec3(2, 2, 0)
    );
    EXPECT_NE(anim, nullptr);
}

// ==================== FadeInFromLarge Tests ====================

TEST_F(AnimationTest, FadeInFromLargeConstruction) {
    auto anim = std::make_shared<FadeInFromLarge>(
        mobject,
        2.0f  // scale_factor
    );
    EXPECT_NE(anim, nullptr);
}

TEST_F(AnimationTest, FadeInFromLargeInterpolation) {
    auto anim = std::make_shared<FadeInFromLarge>(mobject, 3.0f);

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    anim->finish();
}

// ==================== Transform Tests ====================

class TransformTest : public ::testing::Test {
protected:
    void SetUp() override {
        source = std::make_shared<Circle>(1.0f);
        target = std::make_shared<Circle>(2.0f);
        target->shift(math::Vec3(2, 0, 0));
    }

    std::shared_ptr<Mobject> source;
    std::shared_ptr<Mobject> target;
};

TEST_F(TransformTest, TransformConstruction) {
    auto anim = std::make_shared<Transform>(source, target);
    EXPECT_NE(anim, nullptr);
    EXPECT_TRUE(anim->can_run_on_gpu());
}

TEST_F(TransformTest, TransformInterpolation) {
    auto anim = std::make_shared<Transform>(source, target);

    anim->begin();

    auto center_0 = source->get_center();
    anim->interpolate(0.0f);
    // At t=0, should be at original position

    anim->interpolate(0.5f);
    // At t=0.5, should be halfway

    anim->interpolate(1.0f);
    // At t=1, should be at target

    anim->finish();
}

TEST_F(TransformTest, ReplacementTransformConstruction) {
    auto anim = std::make_shared<ReplacementTransform>(source, target);
    EXPECT_NE(anim, nullptr);
}

TEST_F(TransformTest, TransformFromCopyConstruction) {
    auto anim = std::make_shared<TransformFromCopy>(source, target);
    EXPECT_NE(anim, nullptr);
}

TEST_F(TransformTest, ClockwiseTransformConstruction) {
    auto anim = std::make_shared<ClockwiseTransform>(source, target);
    EXPECT_NE(anim, nullptr);
}

TEST_F(TransformTest, CounterclockwiseTransformConstruction) {
    auto anim = std::make_shared<CounterclockwiseTransform>(source, target);
    EXPECT_NE(anim, nullptr);
}

// ==================== MoveToTarget Tests ====================

TEST_F(TransformTest, MoveToTargetConstruction) {
    auto anim = std::make_shared<MoveToTarget>(source);
    EXPECT_NE(anim, nullptr);
}

// ==================== ApplyMatrix Tests ====================

TEST_F(TransformTest, ApplyMatrixConstruction) {
    math::Mat4 matrix;  // Identity matrix by default
    auto anim = std::make_shared<ApplyMatrix>(matrix, source);
    EXPECT_NE(anim, nullptr);
    EXPECT_TRUE(anim->can_run_on_gpu());
}

// ==================== ShowPartial Tests ====================

TEST(ShowPartialTest, Construction) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<ShowPartial>(
        mobject,
        0.0f,  // start_proportion
        0.5f   // end_proportion
    );
    EXPECT_NE(anim, nullptr);
}

TEST(ShowPartialTest, FullRange) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<ShowPartial>(
        mobject,
        0.0f,
        1.0f
    );

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    anim->finish();
}

// ==================== ShowIncreasingSubsets Tests ====================

TEST(ShowIncreasingSubsetsTest, Construction) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<ShowIncreasingSubsets>(mobject);
    EXPECT_NE(anim, nullptr);
}

// ==================== FadeTransform Tests ====================

TEST(FadeTransformTest, Construction) {
    auto source = std::make_shared<Circle>(1.0f);
    auto target = std::make_shared<Circle>(2.0f);

    auto anim = std::make_shared<FadeTransform>(source, target);
    EXPECT_NE(anim, nullptr);
    EXPECT_TRUE(anim->can_run_on_gpu());
}

// ==================== Animation Performance Tests ====================

TEST(AnimationPerformance, CreateManyAnimations) {
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::shared_ptr<Animation>> animations;
    animations.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
        auto mobject = std::make_shared<Circle>();
        animations.push_back(std::make_shared<FadeIn>(mobject));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should create 1000 animations in less than 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST(AnimationPerformance, InterpolateMany) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<FadeIn>(mobject);

    anim->begin();

    auto start = std::chrono::high_resolution_clock::now();

    // Interpolate 10,000 times
    for (int i = 0; i < 10000; ++i) {
        float t = static_cast<float>(i) / 10000.0f;
        anim->interpolate(t);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    anim->finish();

    // 10,000 interpolations should complete in less than 500ms
    EXPECT_LT(duration.count(), 500);
}

TEST(AnimationPerformance, BatchTransforms) {
    const int num_transforms = 100;

    std::vector<std::shared_ptr<Animation>> animations;
    animations.reserve(num_transforms);

    // Create 100 transform animations
    for (int i = 0; i < num_transforms; ++i) {
        auto source = std::make_shared<Circle>(1.0f);
        auto target = std::make_shared<Circle>(2.0f);
        animations.push_back(std::make_shared<Transform>(source, target));
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Run all animations to completion
    for (auto& anim : animations) {
        anim->begin();
        for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
            anim->interpolate(t);
        }
        anim->finish();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 100 transforms with 11 steps each should complete in less than 1 second
    EXPECT_LT(duration.count(), 1000);
}

// ==================== Thread Safety Tests ====================

TEST(AnimationThreadSafety, ConcurrentInterpolation) {
    const int num_threads = 4;

    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<Animation>> animations;

    // Create separate animations for each thread
    for (int t = 0; t < num_threads; ++t) {
        auto mobject = std::make_shared<Circle>();
        animations.push_back(std::make_shared<FadeIn>(mobject));
    }

    // Start all animations
    for (auto& anim : animations) {
        anim->begin();
    }

    // Interpolate concurrently
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (float alpha = 0.0f; alpha <= 1.0f; alpha += 0.01f) {
                animations[t]->interpolate(alpha);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Finish all animations
    for (auto& anim : animations) {
        anim->finish();
    }
}

// ==================== AnimationGroup Tests ====================

class AnimationGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
        circle1 = std::make_shared<Circle>(1.0f);
        circle2 = std::make_shared<Circle>(1.5f);
        circle3 = std::make_shared<Circle>(2.0f);
    }

    std::shared_ptr<Circle> circle1;
    std::shared_ptr<Circle> circle2;
    std::shared_ptr<Circle> circle3;
};

TEST_F(AnimationGroupTest, ConstructionWithAnimations) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1),
        std::make_shared<FadeIn>(circle2),
        std::make_shared<FadeIn>(circle3)
    };

    auto group = std::make_shared<AnimationGroup>(anims);
    EXPECT_EQ(group->get_animation_count(), 3);
}

TEST_F(AnimationGroupTest, ParallelExecution) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1, 1.0f),
        std::make_shared<FadeIn>(circle2, 1.0f),
        std::make_shared<FadeIn>(circle3, 1.0f)
    };

    auto group = std::make_shared<AnimationGroup>(anims, 2.0f, 0.0f);

    group->begin();
    EXPECT_NO_THROW(group->interpolate(0.5f));
    EXPECT_NO_THROW(group->interpolate(1.0f));
    group->finish();
}

TEST_F(AnimationGroupTest, LagRatioTiming) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1, 1.0f),
        std::make_shared<FadeIn>(circle2, 1.0f)
    };

    // 50% lag ratio - second animation starts when first is 50% done
    auto group = std::make_shared<AnimationGroup>(anims, -1.0f, 0.5f);

    EXPECT_FLOAT_EQ(group->get_lag_ratio(), 0.5f);

    const auto& timings = group->get_timings();
    EXPECT_EQ(timings.size(), 2);

    // First animation should start at 0
    EXPECT_FLOAT_EQ(timings[0].start_time, 0.0f);
}

TEST_F(AnimationGroupTest, GPUSupport) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1),
        std::make_shared<FadeIn>(circle2)
    };

    auto group = std::make_shared<AnimationGroup>(anims);

    // Should support GPU if all child animations do
    EXPECT_TRUE(group->can_run_on_gpu());
}

// ==================== Succession Tests ====================

TEST_F(AnimationGroupTest, SuccessionConstruction) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1, 1.0f),
        std::make_shared<FadeIn>(circle2, 1.0f),
        std::make_shared<FadeIn>(circle3, 1.0f)
    };

    auto succession = std::make_shared<Succession>(anims);
    EXPECT_EQ(succession->get_animation_count(), 3);
}

TEST_F(AnimationGroupTest, SuccessionSequential) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1, 1.0f),
        std::make_shared<FadeIn>(circle2, 1.0f)
    };

    auto succession = std::make_shared<Succession>(anims);

    succession->begin();

    // At alpha=0.25, should be in first animation (0.5 of first)
    EXPECT_NO_THROW(succession->interpolate(0.25f));

    // At alpha=0.75, should be in second animation (0.5 of second)
    EXPECT_NO_THROW(succession->interpolate(0.75f));

    succession->finish();
}

TEST_F(AnimationGroupTest, SuccessionTimings) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1, 1.0f),
        std::make_shared<FadeIn>(circle2, 2.0f)  // Different duration
    };

    auto succession = std::make_shared<Succession>(anims);
    const auto& timings = succession->get_timings();

    EXPECT_EQ(timings.size(), 2);

    // First animation: 0 to 1/3 (1 out of 3 total seconds)
    EXPECT_NEAR(timings[0].start_time, 0.0f, 0.01f);
    EXPECT_NEAR(timings[0].end_time, 1.0f / 3.0f, 0.01f);

    // Second animation: 1/3 to 1 (2 out of 3 total seconds)
    EXPECT_NEAR(timings[1].start_time, 1.0f / 3.0f, 0.01f);
    EXPECT_NEAR(timings[1].end_time, 1.0f, 0.01f);
}

// ==================== LaggedStart Tests ====================

TEST_F(AnimationGroupTest, LaggedStartConstruction) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1),
        std::make_shared<FadeIn>(circle2),
        std::make_shared<FadeIn>(circle3)
    };

    auto lagged = std::make_shared<LaggedStart>(anims, 0.1f);
    EXPECT_FLOAT_EQ(lagged->get_lag_ratio(), 0.1f);
}

TEST_F(AnimationGroupTest, LaggedStartDefaultLag) {
    std::vector<std::shared_ptr<Animation>> anims = {
        std::make_shared<FadeIn>(circle1),
        std::make_shared<FadeIn>(circle2)
    };

    auto lagged = std::make_shared<LaggedStart>(anims);  // Default 0.05
    EXPECT_FLOAT_EQ(lagged->get_lag_ratio(), 0.05f);
}

// ==================== MoveAlongPath Tests ====================

class MoveAlongPathTest : public ::testing::Test {
protected:
    void SetUp() override {
        path = std::make_shared<Circle>(2.0f);
        mobject = std::make_shared<Circle>(0.1f);  // Small dot
    }

    std::shared_ptr<Circle> path;
    std::shared_ptr<Circle> mobject;
};

TEST_F(MoveAlongPathTest, Construction) {
    auto anim = std::make_shared<MoveAlongPath>(mobject, path, 2.0f);
    EXPECT_NE(anim, nullptr);
    EXPECT_EQ(anim->get_path(), path);
}

TEST_F(MoveAlongPathTest, Interpolation) {
    auto anim = std::make_shared<MoveAlongPath>(mobject, path, 2.0f);

    anim->begin();

    // Mobject should move along the circle path
    EXPECT_NO_THROW(anim->interpolate(0.0f));
    EXPECT_NO_THROW(anim->interpolate(0.25f));  // Quarter way
    EXPECT_NO_THROW(anim->interpolate(0.5f));   // Half way
    EXPECT_NO_THROW(anim->interpolate(1.0f));   // Full circle

    anim->finish();
}

TEST_F(MoveAlongPathTest, StartPositionStored) {
    mobject->move_to(math::Vec3(1, 0, 0));
    auto anim = std::make_shared<MoveAlongPath>(mobject, path);

    anim->begin();

    // Starting position should be stored
    EXPECT_NEAR(anim->get_starting_position().x, 1.0f, 0.01f);
    EXPECT_NEAR(anim->get_starting_position().y, 0.0f, 0.01f);

    anim->finish();
}

// ==================== MoveTo Tests ====================

TEST(MoveToTest, Construction) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<MoveTo>(mobject, math::Vec3(5, 3, 0));
    EXPECT_NE(anim, nullptr);
}

TEST(MoveToTest, Interpolation) {
    auto mobject = std::make_shared<Circle>();
    mobject->move_to(math::Vec3(0, 0, 0));

    auto anim = std::make_shared<MoveTo>(mobject, math::Vec3(10, 0, 0));

    anim->begin();
    anim->interpolate(0.5f);

    // Should be halfway to target
    auto center = mobject->get_center();
    EXPECT_NEAR(center.x, 5.0f, 0.1f);

    anim->finish();
}

// ==================== Shift Tests ====================

TEST(ShiftTest, Construction) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<Shift>(mobject, math::Vec3(3, 4, 0));
    EXPECT_NE(anim, nullptr);
}

TEST(ShiftTest, Interpolation) {
    auto mobject = std::make_shared<Circle>();
    mobject->move_to(math::Vec3(0, 0, 0));

    auto anim = std::make_shared<Shift>(mobject, math::Vec3(6, 0, 0));

    anim->begin();
    anim->interpolate(0.5f);

    // Should have shifted by half the direction
    auto center = mobject->get_center();
    EXPECT_NEAR(center.x, 3.0f, 0.1f);

    anim->finish();
}

// ==================== Homotopy Tests ====================

class HomotopyTest : public ::testing::Test {
protected:
    void SetUp() override {
        mobject = std::make_shared<Circle>(1.0f);
    }

    std::shared_ptr<Circle> mobject;
};

TEST_F(HomotopyTest, IdentityHomotopy) {
    // Identity homotopy: f(x, y, z, t) = (x, y, z)
    auto homotopy = std::make_shared<Homotopy>(
        [](float x, float y, float z, float t) {
            return math::Vec3(x, y, z);
        },
        mobject
    );

    EXPECT_NE(homotopy, nullptr);

    homotopy->begin();
    EXPECT_NO_THROW(homotopy->interpolate(0.5f));
    homotopy->finish();
}

TEST_F(HomotopyTest, ScalingHomotopy) {
    // Scale from 1x to 2x
    auto homotopy = std::make_shared<Homotopy>(
        [](float x, float y, float z, float t) {
            float scale = 1.0f + t;  // 1 to 2
            return math::Vec3(x * scale, y * scale, z);
        },
        mobject,
        2.0f
    );

    homotopy->begin();
    EXPECT_NO_THROW(homotopy->interpolate(0.5f));
    homotopy->finish();
}

TEST_F(HomotopyTest, RotationHomotopy) {
    // Rotate by t * PI
    auto homotopy = std::make_shared<Homotopy>(
        [](float x, float y, float z, float t) {
            float angle = t * M_PI;
            float cos_a = std::cos(angle);
            float sin_a = std::sin(angle);
            return math::Vec3(
                x * cos_a - y * sin_a,
                x * sin_a + y * cos_a,
                z
            );
        },
        mobject,
        3.0f
    );

    homotopy->begin();

    auto initial_points = mobject->get_points();

    homotopy->interpolate(1.0f);

    // After t=1, points should be rotated 180 degrees
    auto final_points = mobject->get_points();

    // At least verify it ran without error
    EXPECT_FALSE(final_points.empty());

    homotopy->finish();
}

// ==================== ComplexHomotopy Tests ====================

TEST_F(HomotopyTest, ComplexRotation) {
    // Rotate by t * PI using complex multiplication
    auto complex_homotopy = std::make_shared<ComplexHomotopy>(
        [](std::complex<float> z, float t) {
            return z * std::exp(std::complex<float>(0, t * static_cast<float>(M_PI)));
        },
        mobject,
        2.0f
    );

    EXPECT_NE(complex_homotopy, nullptr);

    complex_homotopy->begin();
    EXPECT_NO_THROW(complex_homotopy->interpolate(0.5f));
    complex_homotopy->finish();
}

TEST_F(HomotopyTest, ComplexSquaring) {
    // Interpolate from z to z^2
    auto complex_homotopy = std::make_shared<ComplexHomotopy>(
        [](std::complex<float> z, float t) {
            auto z_squared = z * z;
            // Linear interpolation from z to z^2
            return z + t * (z_squared - z);
        },
        mobject,
        3.0f
    );

    complex_homotopy->begin();
    EXPECT_NO_THROW(complex_homotopy->interpolate(0.5f));
    complex_homotopy->finish();
}

TEST_F(HomotopyTest, ComplexInversion) {
    // Interpolate from z to 1/z (with safety for |z| < epsilon)
    auto complex_homotopy = std::make_shared<ComplexHomotopy>(
        [](std::complex<float> z, float t) {
            float abs_z = std::abs(z);
            if (abs_z < 0.1f) {
                return z;  // Avoid division near zero
            }
            auto inv = 1.0f / z;
            return z + t * (inv - z);
        },
        mobject,
        2.0f
    );

    complex_homotopy->begin();
    EXPECT_NO_THROW(complex_homotopy->interpolate(1.0f));
    complex_homotopy->finish();
}

// ==================== ApplyPointwiseFunction Tests ====================

TEST(ApplyPointwiseFunctionTest, Construction) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<ApplyPointwiseFunction>(
        [](const math::Vec3& p) {
            return math::Vec3(p.x * 2, p.y, p.z);
        },
        mobject
    );

    EXPECT_NE(anim, nullptr);
}

TEST(ApplyPointwiseFunctionTest, Interpolation) {
    auto mobject = std::make_shared<Circle>(1.0f);
    auto anim = std::make_shared<ApplyPointwiseFunction>(
        [](const math::Vec3& p) {
            // Stretch horizontally by 2x
            return math::Vec3(p.x * 2.0f, p.y, p.z);
        },
        mobject,
        1.0f
    );

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));  // 1.5x stretch
    EXPECT_NO_THROW(anim->interpolate(1.0f));  // 2x stretch
    anim->finish();
}

// ==================== ApplyComplexPointwiseFunction Tests ====================

TEST(ApplyComplexPointwiseFunctionTest, Construction) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<ApplyComplexPointwiseFunction>(
        [](std::complex<float> z) {
            return z * z;
        },
        mobject
    );

    EXPECT_NE(anim, nullptr);
}

TEST(ApplyComplexPointwiseFunctionTest, Squaring) {
    auto mobject = std::make_shared<Circle>(1.0f);
    auto anim = std::make_shared<ApplyComplexPointwiseFunction>(
        [](std::complex<float> z) {
            return z * z;
        },
        mobject,
        2.0f
    );

    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    EXPECT_NO_THROW(anim->interpolate(1.0f));
    anim->finish();
}

// ==================== LaggedStartMap Tests ====================

TEST(LaggedStartMapTest, CreateFromMobjects) {
    std::vector<std::shared_ptr<Mobject>> mobjects;
    for (int i = 0; i < 5; ++i) {
        mobjects.push_back(std::make_shared<Circle>(1.0f + i * 0.1f));
    }

    auto lagged = make_lagged_start_map<FadeIn>(mobjects, 0.1f, 1.0f);

    EXPECT_NE(lagged, nullptr);
    EXPECT_EQ(lagged->get_animation_count(), 5);
    EXPECT_FLOAT_EQ(lagged->get_lag_ratio(), 0.1f);
}

TEST(LaggedStartMapTest, ExecuteWithMultipleMobjects) {
    std::vector<std::shared_ptr<Mobject>> mobjects;
    for (int i = 0; i < 3; ++i) {
        mobjects.push_back(std::make_shared<Circle>());
    }

    auto lagged = make_lagged_start_map<FadeIn>(mobjects, 0.2f);

    lagged->begin();
    EXPECT_NO_THROW(lagged->interpolate(0.5f));
    lagged->finish();
}

// ==================== Main ====================

#if 0
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
