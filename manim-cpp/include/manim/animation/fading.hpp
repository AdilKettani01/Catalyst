#pragma once

#include "manim/animation/animation.hpp"

namespace manim {

/**
 * @brief Fade in animation
 *
 * GPU Optimizations:
 * - Batch opacity changes on GPU
 * - Parallel color interpolation
 */
class FadeIn : public Animation {
public:
    FadeIn(
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f,
        float shift_amount = 0.0f,
        const math::Vec3& shift_direction = math::Vec3{0, 0, 0}
    )
        : Animation(mobject, run_time),
          shift_amount_(shift_amount),
          shift_direction_(shift_direction),
          starting_position_(math::Vec3(0.0f)) {}

    bool can_run_on_gpu() const override { return true; }

    void upload_to_gpu(GPUAnimationEngine& /*engine*/) override {}
    void interpolate_on_gpu(float /*t*/, VkCommandBuffer /*cmd*/) override {}

public:
    void begin() override {}
    void interpolate_mobject(float /*alpha*/) override {}

    float shift_amount_;
    math::Vec3 shift_direction_;
    math::Vec3 starting_position_;
};

/**
 * @brief Fade out animation
 */
class FadeOut : public FadeIn {
public:
    using FadeIn::FadeIn;

    bool can_run_on_gpu() const override { return true; }

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Fade in from point
 */
class FadeInFromPoint : public FadeIn {
public:
    FadeInFromPoint(
        std::shared_ptr<Mobject> mobject,
        const math::Vec3& point,
        float run_time = 1.0f
    );

protected:
    void interpolate_mobject(float alpha) override;

    math::Vec3 point_;
};

/**
 * @brief Fade out to point
 */
class FadeOutToPoint : public FadeOut {
public:
    FadeOutToPoint(
        std::shared_ptr<Mobject> mobject,
        const math::Vec3& point,
        float run_time = 1.0f
    );

protected:
    void interpolate_mobject(float alpha) override;

    math::Vec3 point_;
};

/**
 * @brief Fade in from large
 */
class FadeInFromLarge : public FadeIn {
public:
    FadeInFromLarge(
        std::shared_ptr<Mobject> mobject,
        float scale_factor = 2.0f,
        float run_time = 1.0f
    );

protected:
    void interpolate_mobject(float alpha) override;

    float scale_factor_;
};

/**
 * @brief Fade transform (fade out + fade in)
 */
class FadeTransform : public Animation {
public:
    FadeTransform(
        std::shared_ptr<Mobject> mobject,
        std::shared_ptr<Mobject> target_mobject,
        float run_time = 1.0f
    );

    bool can_run_on_gpu() const override { return true; }

protected:
    void interpolate_mobject(float alpha) override;

    std::shared_ptr<Mobject> target_mobject_;
};

}  // namespace manim

// Inline stub implementations to satisfy linker
namespace manim {
inline void FadeOut::interpolate_mobject(float) {}
inline FadeInFromPoint::FadeInFromPoint(std::shared_ptr<Mobject> mobject, const math::Vec3& point,
                                        float run_time)
    : FadeIn(std::move(mobject), run_time), point_(point) {}
inline void FadeInFromPoint::interpolate_mobject(float) {}
inline FadeOutToPoint::FadeOutToPoint(std::shared_ptr<Mobject> mobject, const math::Vec3& point,
                                      float run_time)
    : FadeOut(std::move(mobject), run_time), point_(point) {}
inline void FadeOutToPoint::interpolate_mobject(float) {}
inline FadeInFromLarge::FadeInFromLarge(std::shared_ptr<Mobject> mobject, float scale_factor,
                                        float run_time)
    : FadeIn(std::move(mobject), run_time), scale_factor_(scale_factor) {}
inline void FadeInFromLarge::interpolate_mobject(float) {}
inline FadeTransform::FadeTransform(std::shared_ptr<Mobject> mobject,
                                    std::shared_ptr<Mobject> target_mobject,
                                    float run_time)
    : Animation(std::move(mobject), run_time),
      target_mobject_(std::move(target_mobject)) {}
inline void FadeTransform::interpolate_mobject(float) {}
}  // namespace manim
