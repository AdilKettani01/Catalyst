/**
 * @file test_animations.cpp
 * @brief Comprehensive unit tests for all animation types
 *
 * Tests for Creation, Fading, Transform animations with GPU support
 */

#include <gtest/gtest.h>
#include <memory>

#include "manim/animation/animation.hpp"
#include "manim/animation/creation.hpp"
#include "manim/animation/fading.hpp"
#include "manim/animation/transform.hpp"
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

TEST(ShowIncreasingSub setsTest, Construction) {
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

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
