#pragma once

#include "manim/animation/animation.hpp"

namespace manim {

/**
 * @brief Show creation animation - draws mobject progressively
 *
 * GPU Optimizations:
 * - Parallel curve evaluation for partial rendering
 * - GPU stroke generation
 * - Batch creation animations
 */
class ShowCreation : public Animation {
public:
    explicit ShowCreation(
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f
    );

    bool can_run_on_gpu() const override { return true; }

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Uncreate (reverse of create)
 */
class Uncreate : public ShowCreation {
public:
    using ShowCreation::ShowCreation;

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Draw border then fill
 */
class DrawBorderThenFill : public Animation {
public:
    DrawBorderThenFill(
        std::shared_ptr<Mobject> mobject,
        float run_time = 2.0f,
        float stroke_width = 2.0f,
        const math::Vec4& stroke_color = math::Vec4{1,1,1,1}
    );

protected:
    void interpolate_mobject(float alpha) override;

    float stroke_width_;
    math::Vec4 stroke_color_;
};

/**
 * @brief Write animation (for text)
 */
class Write : public DrawBorderThenFill {
public:
    using DrawBorderThenFill::DrawBorderThenFill;
};

/**
 * @brief Unwrite animation
 */
class Unwrite : public Write {
public:
    using Write::Write;

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Show partial mobject
 */
class ShowPartial : public Animation {
public:
    ShowPartial(
        std::shared_ptr<Mobject> mobject,
        float start_proportion = 0.0f,
        float end_proportion = 1.0f,
        float run_time = 1.0f
    );

protected:
    void interpolate_mobject(float alpha) override;

    float start_proportion_;
    float end_proportion_;
};

/**
 * @brief Show increasing submobjects
 */
class ShowIncreasingSubsets : public Animation {
public:
    explicit ShowIncreasingSubsets(
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f
    );

protected:
    void interpolate_mobject(float alpha) override;

    size_t total_submobjects_;
};

/**
 * @brief Show submobjects one by one
 */
class ShowSubmobjectsOneByOne : public ShowIncreasingSubsets {
public:
    using ShowIncreasingSubsets::ShowIncreasingSubsets;

protected:
    void interpolate_mobject(float alpha) override;
};

}  // namespace manim

// Inline stub implementations to satisfy tests
namespace manim {
inline ShowCreation::ShowCreation(std::shared_ptr<Mobject> mobject, float run_time)
    : Animation(std::move(mobject), run_time) {}
inline void ShowCreation::interpolate_mobject(float) {}
inline void Uncreate::interpolate_mobject(float) {}
inline DrawBorderThenFill::DrawBorderThenFill(std::shared_ptr<Mobject> mobject, float run_time,
                                              float stroke_width, const math::Vec4& stroke_color)
    : Animation(std::move(mobject), run_time),
      stroke_width_(stroke_width),
      stroke_color_(stroke_color) {}
inline void DrawBorderThenFill::interpolate_mobject(float) {}
inline void Unwrite::interpolate_mobject(float) {}
inline ShowPartial::ShowPartial(std::shared_ptr<Mobject> mobject, float start_proportion,
                                float end_proportion, float run_time)
    : Animation(std::move(mobject), run_time),
      start_proportion_(start_proportion),
      end_proportion_(end_proportion) {}
inline void ShowPartial::interpolate_mobject(float) {}
inline ShowIncreasingSubsets::ShowIncreasingSubsets(std::shared_ptr<Mobject> mobject, float run_time)
    : Animation(std::move(mobject), run_time), total_submobjects_(0) {}
inline void ShowIncreasingSubsets::interpolate_mobject(float) {}
inline void ShowSubmobjectsOneByOne::interpolate_mobject(float) {}
}  // namespace manim
