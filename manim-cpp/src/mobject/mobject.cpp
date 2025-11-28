// Base Mobject implementation with GPU optimization (stubbed)
#include "manim/mobject/mobject.hpp"
#include "manim/core/compute_engine.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

namespace manim {

// ============================================================================
// Mobject Implementation
// ============================================================================

Mobject::Mobject()
    : num_points_(0),
      local_transform_(math::Mat4(1.0f)),
      world_transform_(math::Mat4(1.0f)),
      prev_transform_(math::Mat4(1.0f)),
      transform_dirty_(false),
      color_(math::Vec4{1.0f, 1.0f, 1.0f, 1.0f}),
      opacity_(1.0f),
      z_index_(0.0f),
      dim_(3),
      updating_suspended_(false),
      bbox_dirty_(true),
      gpu_dirty_(false),
      cpu_dirty_(false),
      memory_pool_(nullptr),
      compute_engine_(nullptr),
      name_(typeid(*this).name()) {
    generate_points();
    init_colors();
}

Mobject::~Mobject() = default;

void Mobject::initialize(MemoryPool& pool, ComputeEngine& engine) {
    memory_pool_ = &pool;
    compute_engine_ = &engine;
    sync_to_gpu();
}

void Mobject::generate_points() {
    points_cpu_.clear();
    points_cpu_.push_back(math::Vec3(0.0f));  // default origin point
    num_points_ = points_cpu_.size();
    bbox_dirty_ = true;
}

Mobject::Ptr Mobject::copy() const {
    auto cloned = std::make_shared<Mobject>();
    cloned->points_cpu_ = points_cpu_;
    cloned->num_points_ = num_points_;
    cloned->color_ = color_;
    cloned->opacity_ = opacity_;
    cloned->z_index_ = z_index_;
    cloned->dim_ = dim_;
    cloned->bbox_dirty_ = bbox_dirty_;
    cloned->bounding_box_ = bounding_box_;
    cloned->name_ = name_;
    cloned->gpu_dirty_ = true;
    cloned->cpu_dirty_ = cpu_dirty_;
    return cloned;
}

// ============================================================================
// Points Management
// ============================================================================

void Mobject::set_points(std::span<const math::Vec3> points) {
    points_cpu_.assign(points.begin(), points.end());
    num_points_ = points_cpu_.size();
    gpu_dirty_ = true;
    bbox_dirty_ = true;
    point_hash_ = std::nullopt;  // Invalidate hash
}

std::span<const math::Vec3> Mobject::get_points() const {
    if (cpu_dirty_) {
        sync_from_gpu();
    }
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

Mobject& Mobject::move_to(const math::Vec3& position) {
    auto offset = position - get_center();
    return shift(offset);
}

void Mobject::apply_transform(const math::Mat4& transform) {
    apply_matrix(transform);
}

void Mobject::apply_matrix(const math::Mat4& matrix, bool about_point, const math::Vec3& point) {
    if (about_point) {
        auto translate_to = glm::translate(math::Mat4(1.0f), -point);
        auto translate_back = glm::translate(math::Mat4(1.0f), point);
        local_transform_ = translate_back * matrix * translate_to * local_transform_;
    } else {
        local_transform_ = matrix * local_transform_;
    }

    transform_dirty_ = true;

    for (auto& p : points_cpu_) {
        math::Vec4 p4(p, 1.0f);
        p4 = matrix * p4;
        p = math::Vec3(p4);
    }

    gpu_dirty_ = true;
    bbox_dirty_ = true;
    point_hash_ = std::nullopt;
}

void Mobject::apply_points_function(const std::function<math::Vec3(math::Vec3)>& func) {
    for (auto& p : points_cpu_) {
        p = func(p);
    }
    gpu_dirty_ = true;
    bbox_dirty_ = true;
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

Mobject& Mobject::rotate_about_point(float angle, const math::Vec3& point,
                                     const math::Vec3& axis) {
    auto rotation = glm::rotate(math::Mat4(1.0f), angle, axis);
    apply_matrix(rotation, true, point);
    return *this;
}

Mobject& Mobject::next_to(const Mobject& other, const math::Vec3& direction, float buff) {
    math::Vec3 dir = direction;
    float len = glm::length(dir);
    if (len > 1e-6f) {
        dir = glm::normalize(dir);
    }
    auto target = other.get_center() + dir * buff;
    return move_to(target);
}

Mobject& Mobject::align_to(const Mobject& other) {
    return move_to(other.get_center());
}

Mobject& Mobject::to_edge(const math::Vec3& direction, float buff) {
    math::Vec3 dir = direction;
    float len = glm::length(dir);
    if (len > 1e-6f) {
        dir = glm::normalize(dir);
    }
    return shift(dir * buff);
}

math::Vec3 Mobject::get_center() const {
    if (points_cpu_.empty()) {
        return math::Vec3(0.0f);
    }

    math::Vec3 min_p = points_cpu_.front();
    math::Vec3 max_p = points_cpu_.front();
    for (const auto& p : points_cpu_) {
        min_p = glm::min(min_p, p);
        max_p = glm::max(max_p, p);
    }
    return (min_p + max_p) * 0.5f;
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

Mobject::BoundingBox Mobject::compute_bounding_box() const {
    auto [min_corner, max_corner] = get_bounding_box();
    return BoundingBox{min_corner, max_corner};
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

    for (auto& updater : updaters_) {
        updater(*this, dt);
    }

    if (recurse_down) {
        for (auto& submob : submobjects_) {
            submob->update(dt, true);
        }
    }
}

void Mobject::update_submobjects(float dt) {
    for (auto& submob : submobjects_) {
        submob->update(dt, true);
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

Mobject& Mobject::set_opacity(float opacity) {
    opacity_ = opacity;
    color_.w = opacity;
    gpu_dirty_ = true;
    return *this;
}

Mobject& Mobject::set_z_index(float z_index) {
    z_index_ = z_index;
    return *this;
}

void Mobject::init_colors() {
    // Initialize color data if needed
}

// ============================================================================
// Rendering / GPU helpers
// ============================================================================

void Mobject::render(Renderer& /*renderer*/) {
    // Base mobject does not render by itself
}

void Mobject::mark_dirty() {
    gpu_dirty_ = true;
    cpu_dirty_ = true;
}

void Mobject::mark_as_gpu_dirty() {
    gpu_dirty_ = true;
}

bool Mobject::is_gpu_dirty() const {
    return gpu_dirty_;
}

void Mobject::clear_gpu_dirty_flag() {
    gpu_dirty_ = false;
}

void Mobject::update_gpu_buffers() {
    if (!gpu_dirty_) {
        return;
    }
    sync_to_gpu();
    gpu_dirty_ = false;
}

void Mobject::sync_to_gpu() {
    if (!memory_pool_) {
        spdlog::warn("sync_to_gpu called without MemoryPool; skipping upload.");
        return;
    }

    if (points_cpu_.empty()) {
        points_buffer_ = GPUBuffer();
        cpu_dirty_ = false;
        return;
    }

    VkDeviceSize size = static_cast<VkDeviceSize>(points_cpu_.size() * sizeof(math::Vec3));
    if (points_buffer_.get_buffer() == VK_NULL_HANDLE || points_buffer_.get_size() < size) {
        points_buffer_ = memory_pool_->allocate_buffer(
            size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );
    }

    void* mapped = points_buffer_.map();
    if (mapped) {
        std::memcpy(mapped, points_cpu_.data(), static_cast<size_t>(size));
        points_buffer_.unmap();
    } else {
        spdlog::warn("Failed to map points buffer for upload.");
    }

    cpu_dirty_ = false;
}

void Mobject::sync_from_gpu() const {
    if (points_buffer_.get_buffer() == VK_NULL_HANDLE) {
        cpu_dirty_ = false;
        return;
    }

    const void* mapped = points_buffer_.map();
    if (!mapped) {
        spdlog::warn("Failed to map points buffer for readback.");
        cpu_dirty_ = false;
        return;
    }

    size_t count = static_cast<size_t>(points_buffer_.get_size() / sizeof(math::Vec3));
    points_cpu_.resize(count);
    std::memcpy(points_cpu_.data(), mapped, count * sizeof(math::Vec3));
    points_buffer_.unmap();
    cpu_dirty_ = false;
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
