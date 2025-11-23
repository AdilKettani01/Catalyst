// Base Mobject implementation with GPU optimization
#include "manim/mobject/mobject.hpp"
#include "manim/core/compute_engine.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace manim {

// ============================================================================
// Mobject Implementation
// ============================================================================

Mobject::Mobject()
    : name_(typeid(*this).name()),
      dim_(3),
      z_index_(0.0f),
      color_(math::Vec4{1.0f, 1.0f, 1.0f, 1.0f}),
      updating_suspended_(false),
      gpu_dirty_(false),
      transform_dirty_(false),
      num_points_(0) {

    local_transform_ = math::Mat4(1.0f);  // Identity
    world_transform_ = math::Mat4(1.0f);
    prev_transform_ = math::Mat4(1.0f);

    generate_points();
    init_colors();
}

Mobject::~Mobject() {
    // Cleanup if needed
}

// ============================================================================
// Points Management
// ============================================================================

void Mobject::set_points(std::span<const math::Vec3> points) {
    points_cpu_.assign(points.begin(), points.end());
    num_points_ = points_cpu_.size();
    gpu_dirty_ = true;
    point_hash_ = std::nullopt;  // Invalidate hash
}

std::span<const math::Vec3> Mobject::get_points() const {
    return std::span<const math::Vec3>(points_cpu_);
}

// ============================================================================
// Submobjects
// ============================================================================

void Mobject::add(Ptr mobject) {
    if (mobject.get() == this) {
        throw std::runtime_error("Cannot add mobject as submobject of itself");
    }

    // Check if already added
    auto it = std::find(submobjects_.begin(), submobjects_.end(), mobject);
    if (it != submobjects_.end()) {
        return;
    }

    submobjects_.push_back(mobject);
    mobject->parent_ = shared_from_this();
}

void Mobject::remove(Ptr mobject) {
    auto it = std::find(submobjects_.begin(), submobjects_.end(), mobject);
    if (it != submobjects_.end()) {
        (*it)->parent_.reset();
        submobjects_.erase(it);
    }
}

void Mobject::clear_submobjects() {
    for (auto& submob : submobjects_) {
        submob->parent_.reset();
    }
    submobjects_.clear();
}

std::vector<Mobject::Ptr> Mobject::get_family() const {
    std::vector<Ptr> family;
    family.push_back(const_cast<Mobject*>(this)->shared_from_this());

    for (const auto& submob : submobjects_) {
        auto sub_family = submob->get_family();
        family.insert(family.end(), sub_family.begin(), sub_family.end());
    }

    return family;
}

// ============================================================================
// Transformations
// ============================================================================

void Mobject::apply_matrix(const math::Mat4& matrix, bool about_point, const math::Vec3& point) {
    if (about_point) {
        // Translate to origin, apply matrix, translate back
        auto translate_to = glm::translate(math::Mat4(1.0f), -point);
        auto translate_back = glm::translate(math::Mat4(1.0f), point);
        local_transform_ = translate_back * matrix * translate_to * local_transform_;
    } else {
        local_transform_ = matrix * local_transform_;
    }

    transform_dirty_ = true;

    // Apply to points
    for (auto& p : points_cpu_) {
        math::Vec4 p4(p, 1.0f);
        p4 = matrix * p4;
        p = math::Vec3(p4);
    }

    gpu_dirty_ = true;
}

void Mobject::apply_points_function(const std::function<math::Vec3(math::Vec3)>& func) {
    for (auto& p : points_cpu_) {
        p = func(p);
    }
    gpu_dirty_ = true;
    point_hash_ = std::nullopt;
}

Mobject& Mobject::shift(const math::Vec3& offset) {
    auto translation = glm::translate(math::Mat4(1.0f), offset);
    apply_matrix(translation);
    return *this;
}

Mobject& Mobject::scale(float factor) {
    auto scale_mat = glm::scale(math::Mat4(1.0f), math::Vec3(factor));
    apply_matrix(scale_mat, true, get_center());
    return *this;
}

Mobject& Mobject::scale(const math::Vec3& factors) {
    auto scale_mat = glm::scale(math::Mat4(1.0f), factors);
    apply_matrix(scale_mat, true, get_center());
    return *this;
}

Mobject& Mobject::rotate(float angle, const math::Vec3& axis) {
    auto rotation = glm::rotate(math::Mat4(1.0f), angle, axis);
    apply_matrix(rotation, true, get_center());
    return *this;
}

math::Vec3 Mobject::get_center() const {
    if (points_cpu_.empty()) {
        return math::Vec3(0.0f);
    }

    math::Vec3 sum(0.0f);
    for (const auto& p : points_cpu_) {
        sum += p;
    }
    return sum / static_cast<float>(points_cpu_.size());
}

std::pair<math::Vec3, math::Vec3> Mobject::get_bounding_box() const {
    if (points_cpu_.empty()) {
        return {math::Vec3(0.0f), math::Vec3(0.0f)};
    }

    math::Vec3 min_corner = points_cpu_[0];
    math::Vec3 max_corner = points_cpu_[0];

    for (const auto& p : points_cpu_) {
        min_corner = glm::min(min_corner, p);
        max_corner = glm::max(max_corner, p);
    }

    return {min_corner, max_corner};
}

// ============================================================================
// Animation
// ============================================================================

void Mobject::suspend_updating() {
    updating_suspended_ = true;
    for (auto& submob : submobjects_) {
        submob->suspend_updating();
    }
}

void Mobject::resume_updating() {
    updating_suspended_ = false;
    for (auto& submob : submobjects_) {
        submob->resume_updating();
    }
}

void Mobject::update(float dt, bool recurse_down) {
    if (updating_suspended_) {
        return;
    }

    // Apply updaters
    for (auto& updater : updaters_) {
        updater(*this, dt);
    }

    // Update submobjects
    if (recurse_down) {
        for (auto& submob : submobjects_) {
            submob->update(dt, true);
        }
    }
}

void Mobject::add_updater(UpdaterFunc updater) {
    updaters_.push_back(updater);
}

void Mobject::remove_updater(size_t index) {
    if (index < updaters_.size()) {
        updaters_.erase(updaters_.begin() + index);
    }
}

void Mobject::clear_updaters() {
    updaters_.clear();
}

// ============================================================================
// Style
// ============================================================================

Mobject& Mobject::set_color(const math::Vec4& color) {
    color_ = color;
    gpu_dirty_ = true;
    return *this;
}

void Mobject::init_colors() {
    // Initialize color data if needed
}

// ============================================================================
// Protected Virtual Methods
// ============================================================================

void Mobject::generate_points() {
    // Default: no points
    // Derived classes should override
}

// ============================================================================
// GPU Helper Methods
// ============================================================================

void Mobject::mark_as_gpu_dirty() {
    gpu_dirty_ = true;
}

bool Mobject::is_gpu_dirty() const {
    return gpu_dirty_;
}

void Mobject::clear_gpu_dirty_flag() {
    gpu_dirty_ = false;
}

// ============================================================================
// World Transform Management
// ============================================================================

void Mobject::update_world_transform() {
    if (auto p = parent_.lock()) {
        world_transform_ = p->world_transform_ * local_transform_;
    } else {
        world_transform_ = local_transform_;
    }

    transform_dirty_ = false;

    // Update children
    for (auto& child : submobjects_) {
        child->update_world_transform();
    }
}

const math::Mat4& Mobject::get_world_transform() const {
    if (transform_dirty_) {
        const_cast<Mobject*>(this)->update_world_transform();
    }
    return world_transform_;
}

}  // namespace manim