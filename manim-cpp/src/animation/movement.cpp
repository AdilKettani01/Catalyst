/**
 * @file movement.cpp
 * @brief Implementation of movement and homotopy animations
 */

#include "manim/animation/movement.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <glm/glm.hpp>

namespace manim {

// ============================================================================
// MoveAlongPath
// ============================================================================

MoveAlongPath::MoveAlongPath(
    std::shared_ptr<Mobject> mobject,
    std::shared_ptr<VMobject> path,
    float run_time,
    RateFunc rate_func
)
    : Animation(mobject, run_time, rate_func)
    , path_(path)
    , starting_position_(0.0f, 0.0f, 0.0f)
{
    name_ = "MoveAlongPath";

    if (!rate_func_) {
        rate_func_ = rate_functions::smooth;
    }
}

void MoveAlongPath::begin() {
    Animation::begin();

    if (mobject_) {
        starting_position_ = mobject_->get_center();
    }

    spdlog::debug("MoveAlongPath::begin() - starting at ({}, {}, {})",
                  starting_position_.x, starting_position_.y, starting_position_.z);
}

void MoveAlongPath::interpolate_mobject(float alpha) {
    if (!path_ || !mobject_) {
        return;
    }

    // Get the point on the path at this alpha
    math::Vec3 target_point = path_->point_from_proportion(alpha);

    // Move the mobject's center to that point
    mobject_->move_to(target_point);
}

void MoveAlongPath::finish() {
    // Ensure we end exactly at the final point
    if (path_ && mobject_) {
        math::Vec3 final_point = path_->point_from_proportion(1.0f);
        mobject_->move_to(final_point);
    }

    Animation::finish();
    spdlog::debug("MoveAlongPath::finish()");
}

// ============================================================================
// MoveTo
// ============================================================================

MoveTo::MoveTo(
    std::shared_ptr<Mobject> mobject,
    const math::Vec3& target,
    float run_time,
    RateFunc rate_func
)
    : Animation(mobject, run_time, rate_func)
    , target_(target)
    , start_(0.0f, 0.0f, 0.0f)
{
    name_ = "MoveTo";

    if (!rate_func_) {
        rate_func_ = rate_functions::smooth;
    }
}

void MoveTo::begin() {
    Animation::begin();

    if (mobject_) {
        start_ = mobject_->get_center();
    }
}

void MoveTo::interpolate_mobject(float alpha) {
    if (!mobject_) {
        return;
    }

    // Linear interpolation between start and target
    math::Vec3 current = glm::mix(start_, target_, alpha);
    mobject_->move_to(current);
}

// ============================================================================
// Shift
// ============================================================================

Shift::Shift(
    std::shared_ptr<Mobject> mobject,
    const math::Vec3& direction,
    float run_time,
    RateFunc rate_func
)
    : Animation(mobject, run_time, rate_func)
    , direction_(direction)
    , start_(0.0f, 0.0f, 0.0f)
{
    name_ = "Shift";

    if (!rate_func_) {
        rate_func_ = rate_functions::smooth;
    }
}

void Shift::begin() {
    Animation::begin();

    if (mobject_) {
        start_ = mobject_->get_center();
    }
}

void Shift::interpolate_mobject(float alpha) {
    if (!mobject_) {
        return;
    }

    // Move by alpha * direction from start
    math::Vec3 current = start_ + direction_ * alpha;
    mobject_->move_to(current);
}

// ============================================================================
// Homotopy
// ============================================================================

Homotopy::Homotopy(
    HomotopyFunc homotopy,
    std::shared_ptr<Mobject> mobject,
    float run_time,
    RateFunc rate_func
)
    : Animation(mobject, run_time, rate_func)
    , homotopy_(std::move(homotopy))
{
    name_ = "Homotopy";

    if (!rate_func_) {
        rate_func_ = rate_functions::smooth;
    }
}

void Homotopy::begin() {
    Animation::begin();

    // Store the starting points (copy from span to vector)
    if (mobject_) {
        auto points_span = mobject_->get_points();
        starting_points_.assign(points_span.begin(), points_span.end());
    }

    spdlog::debug("Homotopy::begin() - {} points", starting_points_.size());
}

void Homotopy::interpolate_mobject(float alpha) {
    if (!mobject_ || !homotopy_ || starting_points_.empty()) {
        return;
    }

    apply_homotopy_at_time(alpha);
}

void Homotopy::apply_homotopy_at_time(float t) {
    // Create transformed points
    std::vector<math::Vec3> transformed_points;
    transformed_points.reserve(starting_points_.size());

    for (const auto& p : starting_points_) {
        math::Vec3 new_point = homotopy_(p.x, p.y, p.z, t);
        transformed_points.push_back(new_point);
    }

    // Update the mobject with transformed points
    mobject_->set_points(transformed_points);
}

void Homotopy::finish() {
    // Apply final state (t=1)
    if (mobject_ && homotopy_ && !starting_points_.empty()) {
        apply_homotopy_at_time(1.0f);
    }

    Animation::finish();
    spdlog::debug("Homotopy::finish()");
}

// ============================================================================
// ComplexHomotopy
// ============================================================================

ComplexHomotopy::ComplexHomotopy(
    ComplexHomotopyFunc complex_homotopy,
    std::shared_ptr<Mobject> mobject,
    float run_time,
    RateFunc rate_func
)
    : Homotopy(wrap_complex_func(complex_homotopy), mobject, run_time, rate_func)
    , complex_homotopy_(std::move(complex_homotopy))
{
    name_ = "ComplexHomotopy";
}

HomotopyFunc ComplexHomotopy::wrap_complex_func(ComplexHomotopyFunc func) {
    return [func](float x, float y, float z, float t) -> math::Vec3 {
        // Treat (x, y) as a complex number
        std::complex<float> c(x, y);

        // Apply the complex function
        std::complex<float> result = func(c, t);

        // Return as Vec3, preserving z
        return math::Vec3(result.real(), result.imag(), z);
    };
}

// ============================================================================
// ApplyPointwiseFunction
// ============================================================================

ApplyPointwiseFunction::ApplyPointwiseFunction(
    PointwiseFunc func,
    std::shared_ptr<Mobject> mobject,
    float run_time,
    RateFunc rate_func
)
    : Animation(mobject, run_time, rate_func)
    , func_(std::move(func))
{
    name_ = "ApplyPointwiseFunction";

    if (!rate_func_) {
        rate_func_ = rate_functions::smooth;
    }
}

void ApplyPointwiseFunction::begin() {
    Animation::begin();

    if (mobject_ && func_) {
        // Store starting points (copy from span to vector)
        auto points_span = mobject_->get_points();
        starting_points_.assign(points_span.begin(), points_span.end());

        // Calculate target points
        target_points_.clear();
        target_points_.reserve(starting_points_.size());

        for (const auto& p : starting_points_) {
            target_points_.push_back(func_(p));
        }
    }

    spdlog::debug("ApplyPointwiseFunction::begin() - {} points",
                  starting_points_.size());
}

void ApplyPointwiseFunction::interpolate_mobject(float alpha) {
    if (!mobject_ || starting_points_.empty()) {
        return;
    }

    // Interpolate between starting and target points
    std::vector<math::Vec3> interpolated_points;
    interpolated_points.reserve(starting_points_.size());

    for (size_t i = 0; i < starting_points_.size(); ++i) {
        math::Vec3 interp = glm::mix(starting_points_[i], target_points_[i], alpha);
        interpolated_points.push_back(interp);
    }

    mobject_->set_points(interpolated_points);
}

// ============================================================================
// ApplyComplexPointwiseFunction
// ============================================================================

ApplyComplexPointwiseFunction::ApplyComplexPointwiseFunction(
    ComplexFunc func,
    std::shared_ptr<Mobject> mobject,
    float run_time,
    RateFunc rate_func
)
    : ApplyPointwiseFunction(wrap_complex_func(func), mobject, run_time, rate_func)
{
    name_ = "ApplyComplexPointwiseFunction";
}

ApplyPointwiseFunction::PointwiseFunc ApplyComplexPointwiseFunction::wrap_complex_func(ComplexFunc func) {
    return [func](const math::Vec3& p) -> math::Vec3 {
        std::complex<float> c(p.x, p.y);
        std::complex<float> result = func(c);
        return math::Vec3(result.real(), result.imag(), p.z);
    };
}

}  // namespace manim
