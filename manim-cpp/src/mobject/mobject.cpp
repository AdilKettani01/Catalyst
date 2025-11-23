// Base Mobject implementation with GPU optimization
#include "manim/mobject/mobject.hpp"
#include "manim/core/compute_engine.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

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
      points_dirty_(false),
      transform_dirty_(false) {

    local_transform_ = math::Mat4(1.0f);  // Identity
    world_transform_ = math::Mat4(1.0f);
    prev_transform_ = math::Mat4(1.0f);

    generate_points();
    init_colors();
}

Mobject::~Mobject() = default;

// ============================================================================
// Points Management
// ============================================================================

void Mobject::set_points(const std::vector<math::Vec3>& points) {
    cpu_points_ = points;
    points_dirty_ = true;
    point_hash_ = std::nullopt;  // Invalidate hash
}

const std::vector<math::Vec3>& Mobject::get_points() const {
    return cpu_points_;
}

void Mobject::append_points(const std::vector<math::Vec3>& points) {
    cpu_points_.insert(cpu_points_.end(), points.begin(), points.end());
    points_dirty_ = true;
    point_hash_ = std::nullopt;
}

void Mobject::reset_points() {
    cpu_points_.clear();
    points_dirty_ = true;
    point_hash_ = std::nullopt;
}

size_t Mobject::get_num_points() const {
    return cpu_points_.size();
}

void Mobject::upload_points_to_gpu(MemoryPool& pool) {
    if (!points_dirty_ || cpu_points_.empty()) {
        return;
    }

    VkDeviceSize buffer_size = sizeof(math::Vec3) * cpu_points_.size();

    // Allocate or reallocate GPU buffer
    if (!points_buffer_ || points_buffer_->size() < buffer_size) {
        points_buffer_ = pool.allocate_buffer(
            buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::DeviceLocal,
            MemoryUsage::Dynamic
        );
    }

    // Upload data (would use staging buffer in real implementation)
    // points_buffer_->upload(cpu_points_.data(), buffer_size);

    points_dirty_ = false;
    spdlog::debug("Uploaded {} points to GPU for mobject '{}'", cpu_points_.size(), name_);
}

void Mobject::download_points_from_gpu() {
    if (!points_buffer_) {
        return;
    }

    // Download from GPU (would use staging buffer)
    // points_buffer_->download(cpu_points_.data(), points_buffer_->size());

    points_dirty_ = false;
}

// ============================================================================
// Submobjects
// ============================================================================

Mobject& Mobject::add(Ptr mobject) {
    if (mobject.get() == this) {
        throw std::runtime_error("Cannot add mobject as submobject of itself");
    }

    // Check if already added
    auto it = std::find(submobjects_.begin(), submobjects_.end(), mobject);
    if (it != submobjects_.end()) {
        return *this;
    }

    submobjects_.push_back(mobject);
    mobject->parent_ = shared_from_this();

    return *this;
}

Mobject& Mobject::remove(Ptr mobject) {
    auto it = std::find(submobjects_.begin(), submobjects_.end(), mobject);
    if (it != submobjects_.end()) {
        (*it)->parent_.reset();
        submobjects_.erase(it);
    }

    return *this;
}

const std::vector<Mobject::Ptr>& Mobject::get_submobjects() const {
    return submobjects_;
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

size_t Mobject::get_family_count() const {
    size_t count = 1;  // Self
    for (const auto& submob : submobjects_) {
        count += submob->get_family_count();
    }
    return count;
}

// ============================================================================
// Transformations
// ============================================================================

Mobject& Mobject::shift(const math::Vec3& offset) {
    local_transform_[3] += math::Vec4(offset, 0.0f);
    transform_dirty_ = true;
    return *this;
}

Mobject& Mobject::move_to(const math::Vec3& position) {
    math::Vec3 current_center = get_center();
    return shift(position - current_center);
}

Mobject& Mobject::scale(float factor) {
    return scale(math::Vec3{factor, factor, factor});
}

Mobject& Mobject::scale(const math::Vec3& factors) {
    math::Vec3 center = get_center();

    // Scale around center
    math::Mat4 to_origin = glm::translate(math::Mat4(1.0f), -center);
    math::Mat4 scale_mat = glm::scale(math::Mat4(1.0f), factors);
    math::Mat4 from_origin = glm::translate(math::Mat4(1.0f), center);

    local_transform_ = from_origin * scale_mat * to_origin * local_transform_;
    transform_dirty_ = true;

    return *this;
}

Mobject& Mobject::rotate(float angle, const math::Vec3& axis) {
    math::Vec3 center = get_center();

    // Rotate around center
    math::Mat4 to_origin = glm::translate(math::Mat4(1.0f), -center);
    math::Mat4 rotation = glm::rotate(math::Mat4(1.0f), angle, axis);
    math::Mat4 from_origin = glm::translate(math::Mat4(1.0f), center);

    local_transform_ = from_origin * rotation * to_origin * local_transform_;
    transform_dirty_ = true;

    return *this;
}

void Mobject::apply_transform(const math::Mat4& transform) {
    local_transform_ = transform * local_transform_;
    transform_dirty_ = true;
}

void Mobject::apply_matrix(const math::Mat4& matrix) {
    // Apply matrix directly to points (destructive)
    for (auto& point : cpu_points_) {
        math::Vec4 p = matrix * math::Vec4(point, 1.0f);
        point = math::Vec3(p) / p.w;
    }

    points_dirty_ = true;

    // Apply to submobjects
    for (auto& submob : submobjects_) {
        submob->apply_matrix(matrix);
    }
}

void Mobject::apply_points_function(
    std::function<math::Vec3(const math::Vec3&)> func
) {
    for (auto& point : cpu_points_) {
        point = func(point);
    }

    points_dirty_ = true;

    for (auto& submob : submobjects_) {
        submob->apply_points_function(func);
    }
}

void Mobject::apply_points_function_gpu(
    ComputeEngine& compute,
    const GPUBuffer& shader_params
) {
    if (!points_buffer_ || cpu_points_.empty()) {
        return;
    }

    // Use compute shader to transform points
    // compute.dispatch_point_transform(points_buffer_, shader_params);

    // Apply to submobjects
    for (auto& submob : submobjects_) {
        submob->apply_points_function_gpu(compute, shader_params);
    }
}

math::Mat4 Mobject::get_local_transform() const {
    return local_transform_;
}

math::Mat4 Mobject::get_world_transform() const {
    if (auto parent = parent_.lock()) {
        return parent->get_world_transform() * local_transform_;
    }
    return local_transform_;
}

void Mobject::update_world_transform() {
    prev_transform_ = world_transform_;
    world_transform_ = get_world_transform();
    transform_dirty_ = false;

    for (auto& submob : submobjects_) {
        submob->update_world_transform();
    }
}

// ============================================================================
// Bounding Box
// ============================================================================

math::Vec3 Mobject::get_center() const {
    if (cpu_points_.empty()) {
        return math::Vec3{0.0f, 0.0f, 0.0f};
    }

    math::Vec3 sum{0.0f, 0.0f, 0.0f};
    for (const auto& p : cpu_points_) {
        sum += p;
    }

    return sum / static_cast<float>(cpu_points_.size());
}

math::Vec3 Mobject::get_top() const {
    auto [min, max] = get_bounding_box();
    return math::Vec3{get_center().x, max.y, get_center().z};
}

math::Vec3 Mobject::get_bottom() const {
    auto [min, max] = get_bounding_box();
    return math::Vec3{get_center().x, min.y, get_center().z};
}

math::Vec3 Mobject::get_left() const {
    auto [min, max] = get_bounding_box();
    return math::Vec3{min.x, get_center().y, get_center().z};
}

math::Vec3 Mobject::get_right() const {
    auto [min, max] = get_bounding_box();
    return math::Vec3{max.x, get_center().y, get_center().z};
}

std::pair<math::Vec3, math::Vec3> Mobject::get_bounding_box() const {
    if (cpu_points_.empty() && submobjects_.empty()) {
        return {math::Vec3{0.0f}, math::Vec3{0.0f}};
    }

    math::Vec3 min_point{std::numeric_limits<float>::max()};
    math::Vec3 max_point{std::numeric_limits<float>::lowest()};

    // Check own points
    for (const auto& p : cpu_points_) {
        min_point = glm::min(min_point, p);
        max_point = glm::max(max_point, p);
    }

    // Check submobjects
    for (const auto& submob : submobjects_) {
        auto [sub_min, sub_max] = submob->get_bounding_box();
        min_point = glm::min(min_point, sub_min);
        max_point = glm::max(max_point, sub_max);
    }

    return {min_point, max_point};
}

float Mobject::get_width() const {
    auto [min, max] = get_bounding_box();
    return max.x - min.x;
}

float Mobject::get_height() const {
    auto [min, max] = get_bounding_box();
    return max.y - min.y;
}

float Mobject::get_depth() const {
    auto [min, max] = get_bounding_box();
    return max.z - min.z;
}

// ============================================================================
// Colors
// ============================================================================

Mobject& Mobject::set_color(const math::Vec4& color) {
    color_ = color;
    return *this;
}

math::Vec4 Mobject::get_color() const {
    return color_;
}

void Mobject::init_colors() {
    // Override in subclasses
}

// ============================================================================
// Updaters
// ============================================================================

void Mobject::add_updater(Updater updater, bool call_updater) {
    updaters_.push_back(updater);

    if (call_updater) {
        updater(*this, 0.0f);
    }
}

void Mobject::remove_updater(size_t index) {
    if (index < updaters_.size()) {
        updaters_.erase(updaters_.begin() + index);
    }
}

void Mobject::clear_updaters() {
    updaters_.clear();
}

void Mobject::update(float dt) {
    if (updating_suspended_) {
        return;
    }

    for (auto& updater : updaters_) {
        updater(*this, dt);
    }

    for (auto& submob : submobjects_) {
        submob->update(dt);
    }
}

void Mobject::suspend_updating(bool suspend) {
    updating_suspended_ = suspend;

    for (auto& submob : submobjects_) {
        submob->suspend_updating(suspend);
    }
}

// ============================================================================
// Utilities
// ============================================================================

std::string Mobject::get_name() const {
    return name_;
}

void Mobject::set_name(const std::string& name) {
    name_ = name;
}

float Mobject::get_z_index() const {
    return z_index_;
}

void Mobject::set_z_index(float z_index) {
    z_index_ = z_index;
}

uint64_t Mobject::get_point_hash() const {
    if (point_hash_) {
        return *point_hash_;
    }

    // Compute hash of points
    uint64_t hash = 0;
    for (const auto& p : cpu_points_) {
        hash ^= std::hash<float>{}(p.x);
        hash ^= std::hash<float>{}(p.y) << 1;
        hash ^= std::hash<float>{}(p.z) << 2;
    }

    point_hash_ = hash;
    return hash;
}

void Mobject::generate_points() {
    // Override in subclasses
}

Mobject::Ptr Mobject::copy() const {
    // Create new instance (shallow copy first)
    auto copied = std::make_shared<Mobject>(*this);

    // Deep copy submobjects
    copied->submobjects_.clear();
    for (const auto& submob : submobjects_) {
        copied->add(submob->copy());
    }

    // Copy points
    copied->cpu_points_ = cpu_points_;
    copied->points_dirty_ = true;  // Need to re-upload to GPU

    return copied;
}

}  // namespace manim
