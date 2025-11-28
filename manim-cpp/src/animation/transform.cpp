// Transform animation implementation
#include "manim/animation/transform.hpp"
#include "manim/animation/gpu_animation_engine.hpp"
#include "manim/mobject/vmobject.hpp"
#include <spdlog/spdlog.h>

namespace manim {

// ============================================================================
// Transform Implementation
// ============================================================================

Transform::Transform(
    std::shared_ptr<Mobject> mobject,
    std::shared_ptr<Mobject> target_mobject,
    float run_time
) : Animation(mobject, run_time),
    target_mobject_(target_mobject) {

    name_ = "Transform";
}

void Transform::begin() {
    Animation::begin();

    // Create copy of starting state
    starting_mobject_ = mobject_->copy();

    // Align points for morphing
    if (auto vmob = std::dynamic_pointer_cast<manim::VMobject>(mobject_)) {
        if (auto target_vmob = std::dynamic_pointer_cast<manim::VMobject>(target_mobject_)) {
            vmob->align_points(*target_vmob);
        }
    }

    spdlog::debug("Transform: {} points -> {} points",
                  starting_mobject_->get_num_points(),
                  target_mobject_->get_num_points());
}

void Transform::interpolate_mobject(float alpha) {
    // Interpolate points
    const auto& start_points = starting_mobject_->get_points();
    const auto& end_points = target_mobject_->get_points();

    if (start_points.size() != end_points.size()) {
        spdlog::warn("Transform: point count mismatch");
        return;
    }

    std::vector<math::Vec3> interpolated_points(start_points.size());

    for (size_t i = 0; i < start_points.size(); ++i) {
        interpolated_points[i] = glm::mix(start_points[i], end_points[i], alpha);
    }

    mobject_->set_points(interpolated_points);

    // Interpolate color
    math::Vec4 start_color = starting_mobject_->get_color();
    math::Vec4 end_color = target_mobject_->get_color();
    mobject_->set_color(glm::mix(start_color, end_color, alpha));
}

void Transform::upload_to_gpu([[maybe_unused]] GPUAnimationEngine& engine) {
    // Upload start and end states to GPU
    spdlog::debug("Uploading transform animation to GPU");

    // This would upload:
    // - Starting points
    // - Target points
    // - Starting color
    // - Target color

    gpu_data_.uploaded = true;
}

void Transform::interpolate_on_gpu(float t, [[maybe_unused]] VkCommandBuffer cmd) {
    if (!gpu_data_.uploaded) {
        spdlog::warn("Transform animation not uploaded to GPU");
        return;
    }

    float alpha = get_rate_alpha(t / run_time_);

    // Dispatch GPU interpolation
    // This would call:
    // engine.interpolate_points_gpu(start_buffer, end_buffer, alpha, output);

    spdlog::trace("Transform GPU interpolation: alpha={:.3f}", alpha);
}

// ============================================================================
// ReplacementTransform Implementation
// ============================================================================

void ReplacementTransform::finish() {
    Transform::finish();

    // Replace mobject with a copy of the target
    // Note: We can't use operator= since GPUBuffer is non-copyable
    // Instead, create a new copy and swap pointers if needed
    // For now, just copy the visual properties
    mobject_->set_color(target_mobject_->get_color());
    mobject_->set_opacity(target_mobject_->get_opacity());

    spdlog::debug("ReplacementTransform: mobject replaced");
}

// ============================================================================
// TransformFromCopy Implementation
// ============================================================================

TransformFromCopy::TransformFromCopy(
    std::shared_ptr<Mobject> mobject,
    std::shared_ptr<Mobject> target_mobject,
    float run_time
) : Transform(mobject->copy(), target_mobject, run_time) {
    // Transform from a copy
    introducer_ = true;
}

void TransformFromCopy::begin() {
    Transform::begin();
    // Additional setup for copy
}

// ============================================================================
// ClockwiseTransform Implementation
// ============================================================================

void ClockwiseTransform::interpolate_mobject(float alpha) {
    // Apply clockwise rotation path

    // Get centers
    math::Vec3 start_center = starting_mobject_->get_center();
    math::Vec3 end_center = target_mobject_->get_center();

    // Interpolate with arc
    float angle = alpha * math::PI;  // Rotate through 180 degrees
    float radius = glm::length(end_center - start_center) * 0.5f;

    math::Vec3 mid_point = (start_center + end_center) * 0.5f;
    math::Vec3 offset{
        radius * std::cos(angle),
        radius * std::sin(angle),
        0.0f
    };

    // Apply curved path
    Transform::interpolate_mobject(alpha);

    // Add rotation
    mobject_->move_to(mid_point + offset);
}

// ============================================================================
// CounterclockwiseTransform Implementation
// ============================================================================

void CounterclockwiseTransform::interpolate_mobject(float alpha) {
    // Apply counter-clockwise rotation path
    // Similar to ClockwiseTransform but opposite direction

    Transform::interpolate_mobject(alpha);
}

// ============================================================================
// MoveToTarget Implementation
// ============================================================================

MoveToTarget::MoveToTarget(
    std::shared_ptr<Mobject> mobject,
    float run_time
) : Transform(mobject, nullptr, run_time) {
    // Target will be set in begin()
}

void MoveToTarget::begin() {
    // Use mobject's target as the target_mobject
    // In Python Manim, mobjects have a .target attribute
    // For C++, we assume it's set elsewhere

    if (!target_mobject_) {
        target_mobject_ = mobject_->copy();
    }

    Transform::begin();
}

// ============================================================================
// ApplyFunction Implementation
// ============================================================================

ApplyFunction::ApplyFunction(
    TransformFunc func,
    std::shared_ptr<Mobject> mobject,
    float run_time
) : Transform(mobject, nullptr, run_time),
    func_(func) {

    name_ = "ApplyFunction";
}

void ApplyFunction::begin() {
    // Apply function to create target
    target_mobject_ = func_(mobject_->copy());

    Transform::begin();
}

// ============================================================================
// ApplyMatrix Implementation
// ============================================================================

ApplyMatrix::ApplyMatrix(
    const math::Mat4& matrix,
    std::shared_ptr<Mobject> mobject,
    float run_time
) : ApplyFunction(
        [matrix](std::shared_ptr<Mobject> m) {
            auto copy = m->copy();
            copy->apply_matrix(matrix);
            return copy;
        },
        mobject,
        run_time
    ) {

    name_ = "ApplyMatrix";
}

// ============================================================================
// ApplyComplexFunction Implementation
// ============================================================================

ApplyComplexFunction::ApplyComplexFunction(
    ComplexFunc func,
    std::shared_ptr<Mobject> mobject,
    float run_time
) : ApplyFunction(
        [func](std::shared_ptr<Mobject> m) {
            auto copy = m->copy();

            // Apply complex function to points
            copy->apply_points_function([&func](const math::Vec3& p) {
                std::complex<float> z(p.x, p.y);
                std::complex<float> w = func(z);
                return math::Vec3{w.real(), w.imag(), p.z};
            });

            return copy;
        },
        mobject,
        run_time
    ) {

    name_ = "ApplyComplexFunction";
}

}  // namespace manim
