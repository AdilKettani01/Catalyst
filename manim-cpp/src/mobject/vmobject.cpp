// VMobject implementation with GPU Bezier evaluation
#include "manim/mobject/vmobject.hpp"
#include "manim/core/compute_engine.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>

namespace manim {

// ============================================================================
// VMobject Implementation
// ============================================================================

VMobject::VMobject() : Mobject() {
    // VMobject-specific initialization
}

void VMobject::generate_points() {
    // Override in subclasses
}

void VMobject::init_colors() {
    // Initialize default colors
    set_fill(fill_color_, fill_opacity_);
    set_stroke(stroke_color_, stroke_width_, stroke_opacity_);
}

// ============================================================================
// Bezier Curve Management
// ============================================================================

void VMobject::set_points(const std::vector<math::Vec3>& points) {
    points_cpu_ = points;
    ensure_valid_points();
    gpu_dirty_ = true;
    tessellation_dirty_ = true;
    fill_vertex_buffer_.reset();
    fill_index_buffer_.reset();
    stroke_vertex_buffer_.reset();
    stroke_index_buffer_.reset();
}

void VMobject::add_cubic_bezier_curve(
    const std::array<math::Vec3, 2>& anchors,
    const std::array<math::Vec3, 2>& handles
) {
    // Add 4 control points: anchor0, handle0, handle1, anchor1
    if (!points_cpu_.empty() && has_new_path_started_) {
        // Continue from last anchor
        points_cpu_.push_back(handles[0]);
        points_cpu_.push_back(handles[1]);
        points_cpu_.push_back(anchors[1]);
    } else {
        // New curve
        points_cpu_.push_back(anchors[0]);
        points_cpu_.push_back(handles[0]);
        points_cpu_.push_back(handles[1]);
        points_cpu_.push_back(anchors[1]);
        has_new_path_started_ = true;
    }

    gpu_dirty_ = true;
    tessellation_dirty_ = true;
    fill_vertex_buffer_.reset();
    fill_index_buffer_.reset();
    stroke_vertex_buffer_.reset();
    stroke_index_buffer_.reset();
}

void VMobject::add_line_to(const math::Vec3& point) {
    if (points_cpu_.empty()) {
        points_cpu_.push_back(math::Vec3{0.0f});
    }

    math::Vec3 start = points_cpu_.back();
    math::Vec3 handle1 = start + (point - start) * 0.333f;
    math::Vec3 handle2 = start + (point - start) * 0.667f;

    add_cubic_bezier_curve({start, point}, {handle1, handle2});
    fill_vertex_buffer_.reset();
    fill_index_buffer_.reset();
    stroke_vertex_buffer_.reset();
    stroke_index_buffer_.reset();
}

void VMobject::start_new_path(const math::Vec3& point) {
    points_cpu_.push_back(point);
    has_new_path_started_ = false;
    tessellation_dirty_ = true;
    fill_vertex_buffer_.reset();
    fill_index_buffer_.reset();
    stroke_vertex_buffer_.reset();
    stroke_index_buffer_.reset();
}

void VMobject::close_path() {
    if (points_cpu_.size() < 4) {
        return;
    }

    math::Vec3 first = points_cpu_[0];
    math::Vec3 last = points_cpu_.back();

    if (glm::length(first - last) > tolerance_for_point_equality_) {
        add_line_to(first);
    }
}

void VMobject::make_smooth() {
    compute_smooth_handles();
    tessellation_dirty_ = true;
}

void VMobject::make_jagged() {
    // Convert to straight lines between anchors
    auto anchors = get_anchors();
    points_cpu_.clear();

    for (size_t i = 0; i < anchors.size() - 1; ++i) {
        math::Vec3 start = anchors[i];
        math::Vec3 end = anchors[i + 1];
        math::Vec3 handle1 = start + (end - start) * 0.333f;
        math::Vec3 handle2 = start + (end - start) * 0.667f;

        add_cubic_bezier_curve({start, end}, {handle1, handle2});
    }

    tessellation_dirty_ = true;
}

void VMobject::compute_smooth_handles() {
    // Compute smooth cubic spline handles using Catmull-Rom algorithm
    auto anchors = get_anchors();
    if (anchors.size() < 2) {
        return;
    }

    points_cpu_.clear();

    for (size_t i = 0; i < anchors.size() - 1; ++i) {
        math::Vec3 p0 = (i > 0) ? anchors[i - 1] : anchors[i];
        math::Vec3 p1 = anchors[i];
        math::Vec3 p2 = anchors[i + 1];
        math::Vec3 p3 = (i + 2 < anchors.size()) ? anchors[i + 2] : anchors[i + 1];

        // Catmull-Rom handles
        math::Vec3 handle1 = p1 + (p2 - p0) / 6.0f;
        math::Vec3 handle2 = p2 - (p3 - p1) / 6.0f;

        add_cubic_bezier_curve({p1, p2}, {handle1, handle2});
    }
}

void VMobject::ensure_valid_points() {
    // Points array must have size 0 or 1 mod 4
    size_t remainder = points_cpu_.size() % 4;

    if (remainder != 0 && remainder != 1) {
        // Invalid size, truncate
        size_t new_size = (points_cpu_.size() / 4) * 4;
        if (points_cpu_.size() > 0) {
            new_size++;  // Keep at least the first anchor
        }
        points_cpu_.resize(new_size);
        spdlog::warn("VMobject points resized to valid size: {}", new_size);
    }
}

// ============================================================================
// GPU Bezier Evaluation
// ============================================================================

void VMobject::tessellate_on_gpu(uint32_t segments_per_curve, ComputeEngine& engine) {
    size_t num_curves = get_num_curves();
    if (num_curves == 0) {
        tessellated_points_.clear();
        tessellation_dirty_ = false;
        return;
    }

    // Prefer real GPU compute if available
    if (engine.is_gpu_compute_available() && memory_pool_) {
        // Prepare control points buffer (4 vec4 per curve)
        VkDeviceSize cp_size = num_curves * 4 * sizeof(math::Vec4);
        std::vector<math::Vec4> control_points;
        control_points.reserve(num_curves * 4);

        for (size_t i = 0; i < num_curves; ++i) {
            auto curve_pts = get_curve_points(i);
            for (const auto& p : curve_pts) {
                control_points.push_back(math::Vec4(p, 1.0f));
            }
        }

        // Allocate GPU buffers
        auto cp_buffer = memory_pool_->allocate_buffer(
            cp_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );

        VkDeviceSize out_size = num_curves * segments_per_curve * sizeof(math::Vec3);
        auto out_buffer = memory_pool_->allocate_buffer(
            out_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );

        // Upload control points
        if (void* ptr = cp_buffer.map()) {
            std::memcpy(ptr, control_points.data(), cp_size);
            cp_buffer.unmap();
        }

        // Run GPU tessellation
        engine.tessellate_bezier_gpu(cp_buffer, out_buffer,
                                     static_cast<uint32_t>(num_curves), segments_per_curve);

        // Download results
        tessellated_points_.resize(num_curves * segments_per_curve);
        if (const void* ptr = out_buffer.map()) {
            std::memcpy(tessellated_points_.data(), ptr, out_size);
            out_buffer.unmap();
        }

        tessellation_segments_ = segments_per_curve;
        tessellation_dirty_ = false;
        spdlog::debug("VMobject GPU tessellation complete: {} curves, {} segments", num_curves, segments_per_curve);
        return;
    }

    // Fall back to CPU compute engine helper
    std::vector<std::array<math::Vec3, 4>> curves;
    curves.reserve(num_curves);
    for (size_t i = 0; i < num_curves; ++i) {
        curves.push_back(get_curve_points(i));
    }

    auto tess = engine.tessellate_bezier_cpu(curves, segments_per_curve);
    if (!tess.empty()) {
        tessellated_points_ = std::move(tess);
        tessellation_segments_ = segments_per_curve;
        tessellation_dirty_ = false;
        return;
    }

    // Fallback if compute engine did nothing
    ensure_tessellation(segments_per_curve);
}

void VMobject::ensure_tessellation(uint32_t segments_per_curve) {
    if (!tessellation_dirty_ && segments_per_curve == tessellation_segments_) {
        return;
    }

    tessellation_segments_ = std::max<uint32_t>(1, segments_per_curve);
    tessellated_points_.clear();

    size_t num_curves = get_num_curves();
    if (num_curves == 0) {
        tessellation_dirty_ = false;
        return;
    }

    tessellated_points_.reserve(num_curves * (tessellation_segments_ + 1));
    const float tol2 = tolerance_for_point_equality_ * tolerance_for_point_equality_;

    auto eval_curve = [](const std::array<math::Vec3, 4>& c, float t) -> math::Vec3 {
        math::Vec3 p01 = glm::mix(c[0], c[1], t);
        math::Vec3 p12 = glm::mix(c[1], c[2], t);
        math::Vec3 p23 = glm::mix(c[2], c[3], t);
        math::Vec3 p012 = glm::mix(p01, p12, t);
        math::Vec3 p123 = glm::mix(p12, p23, t);
        return glm::mix(p012, p123, t);
    };

    auto estimate_segments = [&](const std::array<math::Vec3, 4>& c) -> uint32_t {
        float chord = glm::length(c[3] - c[0]);
        float ctrl_len = glm::length(c[1] - c[0]) +
                         glm::length(c[2] - c[1]) +
                         glm::length(c[3] - c[2]);
        float flatness = (chord > 1e-5f) ? (ctrl_len / std::max(chord, 1e-5f)) : 1.0f;

        math::Vec3 line = c[3] - c[0];
        float denom = std::max(glm::length(line), 1e-4f);
        float dev1 = glm::length(glm::cross(line, c[1] - c[0])) / denom;
        float dev2 = glm::length(glm::cross(line, c[2] - c[0])) / denom;
        float deviation = std::max(dev1, dev2);

        float curvature_score = 1.0f + flatness + deviation * 2.0f;
        uint32_t segments = static_cast<uint32_t>(std::ceil(curvature_score * static_cast<float>(tessellation_segments_)));
        segments = std::clamp<uint32_t>(segments, std::max<uint32_t>(4, tessellation_segments_), 64);
        return segments;
    };

    for (size_t i = 0; i < num_curves; ++i) {
        auto curve_pts = get_curve_points(i);
        uint32_t segments = estimate_segments(curve_pts);
        for (uint32_t s = 0; s <= segments; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(segments);
            math::Vec3 p = eval_curve(curve_pts, t);
            if (!tessellated_points_.empty()) {
                math::Vec3 diff = p - tessellated_points_.back();
                if (glm::dot(diff, diff) <= tol2) {
                    continue;
                }
            }
            tessellated_points_.push_back(p);
        }
    }

    if (is_closed() && !tessellated_points_.empty()) {
        math::Vec3 diff = tessellated_points_.front() - tessellated_points_.back();
        if (glm::dot(diff, diff) > tol2) {
            tessellated_points_.push_back(tessellated_points_.front());
        }
    }

    tessellation_dirty_ = false;
}

const std::vector<math::Vec3>& VMobject::get_tessellated_points() const {
    const_cast<VMobject*>(this)->ensure_tessellation(tessellation_segments_);
    return tessellated_points_;
}

void VMobject::update_bezier_gpu_buffers(MemoryPool& pool) {
    if (!tessellation_dirty_) {
        return;
    }

    // Update control points buffer
    size_t num_curves = get_num_curves();
    if (num_curves == 0) {
        return;
    }

    VkDeviceSize buffer_size = sizeof(math::Vec4) * num_curves * 4;

    std::vector<math::Vec4> control_points;
    control_points.reserve(num_curves * 4);

    for (size_t i = 0; i < num_curves; ++i) {
        auto curve_pts = get_curve_points(i);
        for (const auto& p : curve_pts) {
            control_points.push_back(math::Vec4(p, 1.0f));
        }
    }

    if (!bezier_control_points_buffer_ ||
        bezier_control_points_buffer_->get_size() < buffer_size) {
        bezier_control_points_buffer_ = pool.allocate_buffer(
            buffer_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::DeviceLocal,
            MemoryUsage::Dynamic
        );
    }

    // Upload control points
    // bezier_control_points_buffer_->upload(control_points.data(), buffer_size);
}

// ============================================================================
// Stroke and Fill Properties
// ============================================================================

VMobject& VMobject::set_fill(const math::Vec4& color, float opacity) {
    fill_color_ = color;
    fill_opacity_ = opacity;
    return *this;
}

VMobject& VMobject::set_fill_opacity(float opacity) {
    fill_opacity_ = opacity;
    return *this;
}

VMobject& VMobject::set_stroke(const math::Vec4& color, float width, float opacity) {
    stroke_color_ = color;
    stroke_width_ = width;
    stroke_opacity_ = opacity;
    return *this;
}

VMobject& VMobject::set_stroke_width(float width) {
    stroke_width_ = width;
    return *this;
}

VMobject& VMobject::set_stroke_opacity(float opacity) {
    stroke_opacity_ = opacity;
    return *this;
}

VMobject& VMobject::set_background_stroke(const math::Vec4& color, float width) {
    background_stroke_color_ = color;
    background_stroke_width_ = width;
    return *this;
}

VMobject& VMobject::set_sheen(float factor, const math::Vec3& direction) {
    sheen_factor_ = factor;
    sheen_direction_ = glm::normalize(direction);
    return *this;
}

VMobject& VMobject::set_joint_type(LineJointType type) {
    joint_type_ = type;
    return *this;
}

VMobject& VMobject::set_cap_style(CapStyleType style) {
    cap_style_ = style;
    return *this;
}

// ============================================================================
// Curve Utilities
// ============================================================================

size_t VMobject::get_num_curves() const {
    if (points_cpu_.size() < 4) {
        return 0;
    }

    // Each curve has 4 points, but they share endpoints
    // Formula: (n - 1) / 3 where n is number of points (excluding first anchor)
    return (points_cpu_.size() - 1) / 3;
}

std::array<math::Vec3, 4> VMobject::get_curve_points(size_t index) const {
    size_t start_idx = index * 3;

    if (start_idx + 3 >= points_cpu_.size()) {
        throw std::out_of_range("Curve index out of range");
    }

    return {
        points_cpu_[start_idx],
        points_cpu_[start_idx + 1],
        points_cpu_[start_idx + 2],
        points_cpu_[start_idx + 3]
    };
}

std::vector<math::Vec3> VMobject::get_anchors() const {
    std::vector<math::Vec3> anchors;

    if (points_cpu_.empty()) {
        return anchors;
    }

    // Anchors are at indices 0, 3, 6, 9, ...
    for (size_t i = 0; i < points_cpu_.size(); i += 3) {
        anchors.push_back(points_cpu_[i]);
    }

    return anchors;
}

math::Vec3 VMobject::point_from_proportion(float alpha) const {
    // Clamp alpha to [0, 1]
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    size_t num_curves = get_num_curves();
    if (num_curves == 0) {
        return math::Vec3{0.0f};
    }

    // Find which curve
    float curve_alpha = alpha * static_cast<float>(num_curves);
    size_t curve_idx = std::min(
        static_cast<size_t>(curve_alpha),
        num_curves - 1
    );
    float t = curve_alpha - static_cast<float>(curve_idx);

    // Evaluate cubic Bezier at t
    auto pts = get_curve_points(curve_idx);

    // De Casteljau's algorithm
    math::Vec3 p01 = glm::mix(pts[0], pts[1], t);
    math::Vec3 p12 = glm::mix(pts[1], pts[2], t);
    math::Vec3 p23 = glm::mix(pts[2], pts[3], t);

    math::Vec3 p012 = glm::mix(p01, p12, t);
    math::Vec3 p123 = glm::mix(p12, p23, t);

    return glm::mix(p012, p123, t);
}

float VMobject::get_arc_length() const {
    // Approximate using tessellated points or numerical integration
    float length = 0.0f;

    size_t num_curves = get_num_curves();
    constexpr int samples = 10;

    for (size_t i = 0; i < num_curves; ++i) {
        auto pts = get_curve_points(i);

        math::Vec3 prev = pts[0];
        for (int j = 1; j <= samples; ++j) {
            float t = static_cast<float>(j) / static_cast<float>(samples);

            // Evaluate bezier
            math::Vec3 p01 = glm::mix(pts[0], pts[1], t);
            math::Vec3 p12 = glm::mix(pts[1], pts[2], t);
            math::Vec3 p23 = glm::mix(pts[2], pts[3], t);

            math::Vec3 p012 = glm::mix(p01, p12, t);
            math::Vec3 p123 = glm::mix(p12, p23, t);

            math::Vec3 curr = glm::mix(p012, p123, t);

            length += glm::length(curr - prev);
            prev = curr;
        }
    }

    return length;
}

void VMobject::insert_n_curves(size_t n) {
    // Subdivide each curve into n+1 curves
    std::vector<math::Vec3> new_points;
    size_t num_curves = get_num_curves();

    for (size_t i = 0; i < num_curves; ++i) {
        auto pts = get_curve_points(i);

        for (size_t j = 0; j <= n; ++j) {
            float t0 = static_cast<float>(j) / static_cast<float>(n + 1);
            float t1 = static_cast<float>(j + 1) / static_cast<float>(n + 1);

            // Subdivide curve [t0, t1] using De Casteljau
            // Simplified: just add new curve with interpolated points
            math::Vec3 p0 = point_from_proportion(
                (static_cast<float>(i) + t0) / static_cast<float>(num_curves)
            );
            math::Vec3 p3 = point_from_proportion(
                (static_cast<float>(i) + t1) / static_cast<float>(num_curves)
            );

            // Approximate handles
            math::Vec3 p1 = p0 + (p3 - p0) * 0.333f;
            math::Vec3 p2 = p0 + (p3 - p0) * 0.667f;

            if (new_points.empty()) {
                new_points.push_back(p0);
            }
            new_points.push_back(p1);
            new_points.push_back(p2);
            new_points.push_back(p3);
        }
    }

    points_cpu_ = new_points;
    gpu_dirty_ = true;
    tessellation_dirty_ = true;
}

void VMobject::align_points(const VMobject& other) {
    // Align number of curves for morphing
    size_t this_curves = get_num_curves();
    size_t other_curves = other.get_num_curves();

    if (this_curves < other_curves) {
        insert_n_curves(other_curves - this_curves);
    }
}

bool VMobject::is_closed() const {
    if (points_cpu_.size() < 4) {
        return false;
    }

    math::Vec3 first = points_cpu_[0];
    math::Vec3 last = points_cpu_.back();

    return glm::length(first - last) < tolerance_for_point_equality_;
}

// ============================================================================
// GPU Rendering Data
// ============================================================================

VMobject::GPUStrokeData VMobject::get_gpu_stroke_data() const {
    GPUStrokeData data;
    data.color = stroke_color_;
    data.width = stroke_width_;
    data.opacity = stroke_opacity_;
    data.joint_type = static_cast<uint32_t>(joint_type_);
    data.cap_style = static_cast<uint32_t>(cap_style_);
    return data;
}

VMobject::GPUFillData VMobject::get_gpu_fill_data() const {
    GPUFillData data;
    data.color = fill_color_;
    data.opacity = fill_opacity_;
    data.sheen_factor = sheen_factor_;
    data.sheen_direction = sheen_direction_;
    return data;
}

VMobject::Ptr VMobject::copy() const {
    auto copied = std::make_shared<VMobject>();

    // Copy base class data
    copied->points_cpu_ = points_cpu_;
    copied->submobjects_ = submobjects_;
    copied->color_ = color_;
    copied->opacity_ = opacity_;
    copied->z_index_ = z_index_;

    // Copy VMobject specific data
    copied->fill_color_ = fill_color_;
    copied->fill_opacity_ = fill_opacity_;
    copied->stroke_color_ = stroke_color_;
    copied->stroke_width_ = stroke_width_;
    copied->stroke_opacity_ = stroke_opacity_;
    copied->background_stroke_color_ = background_stroke_color_;
    copied->background_stroke_width_ = background_stroke_width_;
    copied->background_stroke_opacity_ = background_stroke_opacity_;
    copied->sheen_factor_ = sheen_factor_;
    copied->sheen_direction_ = sheen_direction_;
    copied->joint_type_ = joint_type_;
    copied->cap_style_ = cap_style_;
    copied->has_new_path_started_ = has_new_path_started_;
    copied->shade_in_3d_ = shade_in_3d_;
    copied->close_new_points_ = close_new_points_;
    copied->n_points_per_cubic_curve_ = n_points_per_cubic_curve_;
    copied->tolerance_for_point_equality_ = tolerance_for_point_equality_;
    copied->tessellated_points_ = tessellated_points_;
    copied->tessellation_segments_ = tessellation_segments_;

    // Cached GPU buffers are not copied; mark dirty
    copied->fill_vertex_buffer_.reset();
    copied->fill_index_buffer_.reset();
    copied->stroke_vertex_buffer_.reset();
    copied->stroke_index_buffer_.reset();
    copied->fill_indices_count_ = 0;
    copied->stroke_indices_count_ = 0;

    // Mark as needing GPU update
    copied->gpu_dirty_ = true;
    copied->tessellation_dirty_ = true;

    // Don't copy GPU buffers - they'll be recreated when needed

    return copied;
}

// ============================================================================
// VGroup Implementation
// ============================================================================

VGroup::VGroup(const std::vector<std::shared_ptr<VMobject>>& vmobjects) {
    for (const auto& vmob : vmobjects) {
        add(vmob);
    }
}

VGroup& VGroup::add(std::shared_ptr<VMobject> vmobject) {
    Mobject::add(std::static_pointer_cast<Mobject>(vmobject));
    return *this;
}

void VMobject::build_fill_geometry(const math::Mat4& world_transform,
                                   uint32_t segments_per_curve,
                                   std::vector<math::Vec3>& out_vertices,
                                   std::vector<uint32_t>& out_indices) {
    out_vertices.clear();
    out_indices.clear();

    if (fill_opacity_ <= 0.0f) {
        return;
    }

    ensure_tessellation(segments_per_curve);
    if (tessellated_points_.size() < 3) {
        return;
    }

    out_vertices.reserve(tessellated_points_.size());
    for (const auto& p : tessellated_points_) {
        math::Vec4 hp = world_transform * math::Vec4(p, 1.0f);
        out_vertices.push_back(math::Vec3(hp));
    }

    const float tol2 = tolerance_for_point_equality_ * tolerance_for_point_equality_;
    if (is_closed() && out_vertices.size() > 1) {
        math::Vec3 diff = out_vertices.front() - out_vertices.back();
        if (glm::dot(diff, diff) <= tol2) {
            out_vertices.pop_back();
        }
    }

    if (out_vertices.size() < 3) {
        out_vertices.clear();
        return;
    }

    out_indices.reserve((out_vertices.size() - 2) * 3);
    for (uint32_t i = 1; i + 1 < out_vertices.size(); ++i) {
        out_indices.push_back(0);
        out_indices.push_back(i);
        out_indices.push_back(i + 1);
    }
}

void VMobject::build_stroke_geometry(const math::Mat4& world_transform,
                                     uint32_t segments_per_curve,
                                     std::vector<math::Vec3>& out_vertices,
                                     std::vector<uint32_t>& out_indices,
                                     float stroke_width_world_override) {
    out_vertices.clear();
    out_indices.clear();

    // Use override if provided (for pixel-to-world conversion), otherwise use member
    float stroke_width_world = (stroke_width_world_override > 0.0f)
        ? stroke_width_world_override
        : stroke_width_;

    if (stroke_width_world <= 0.0f || stroke_opacity_ <= 0.0f) {
        return;
    }

    ensure_tessellation(segments_per_curve);
    if (tessellated_points_.size() < 2) {
        return;
    }

    std::vector<math::Vec3> polyline;
    polyline.reserve(tessellated_points_.size());
    for (const auto& p : tessellated_points_) {
        polyline.push_back(math::Vec3(world_transform * math::Vec4(p, 1.0f)));
    }

    const float tol2 = tolerance_for_point_equality_ * tolerance_for_point_equality_;
    bool closed = is_closed();
    if (closed && polyline.size() > 1) {
        math::Vec3 diff = polyline.front() - polyline.back();
        if (glm::dot(diff, diff) <= tol2) {
            polyline.pop_back();
        }
    }

    if (polyline.size() < 2) {
        return;
    }

    float axis_x = glm::length(math::Vec3(world_transform[0]));
    float axis_y = glm::length(math::Vec3(world_transform[1]));
    float stroke_scale = (axis_x + axis_y) * 0.5f;
    if (stroke_scale <= 0.0f) {
        stroke_scale = 1.0f;
    }

    float half_width = stroke_width_world * 0.5f * stroke_scale;

    auto append_segment = [&](const math::Vec3& a, const math::Vec3& b) {
        math::Vec3 dir = b - a;
        float len2 = glm::dot(dir, dir);
        if (len2 <= tol2) {
            return;
        }
        dir /= std::sqrt(len2);
        math::Vec3 perp{-dir.y, dir.x, 0.0f};
        float perp_len2 = glm::dot(perp, perp);
        if (perp_len2 <= 1e-8f) {
            // Degenerate direction; pick an arbitrary perpendicular
            math::Vec3 alt = glm::cross(dir, math::Vec3(0, 0, 1));
            float alt_len2 = glm::dot(alt, alt);
            if (alt_len2 <= 1e-8f) {
                perp = math::Vec3(0, 1, 0);
            } else {
                perp = alt / std::sqrt(alt_len2);
            }
        } else {
            perp /= std::sqrt(perp_len2);
        }
        perp *= half_width;

        uint32_t base = static_cast<uint32_t>(out_vertices.size());
        out_vertices.push_back(a + perp);
        out_vertices.push_back(a - perp);
        out_vertices.push_back(b + perp);
        out_vertices.push_back(b - perp);

        out_indices.push_back(base + 0);
        out_indices.push_back(base + 1);
        out_indices.push_back(base + 2);
        out_indices.push_back(base + 2);
        out_indices.push_back(base + 1);
        out_indices.push_back(base + 3);
    };

    for (size_t i = 0; i + 1 < polyline.size(); ++i) {
        append_segment(polyline[i], polyline[i + 1]);
    }
    if (closed && polyline.size() > 2) {
        append_segment(polyline.back(), polyline.front());
    }
}

void VMobject::update_gpu_fill_buffers(MemoryPool& pool, const math::Mat4& world_transform) {
    constexpr uint32_t kDefaultSegments = 12;
    const uint32_t segments = std::max<uint32_t>(tessellation_segments_, kDefaultSegments);

    std::vector<math::Vec3> verts;
    std::vector<uint32_t> indices;
    build_fill_geometry(world_transform, segments, verts, indices);

    if (verts.size() < 3 || indices.size() < 3) {
        fill_vertex_buffer_.reset();
        fill_index_buffer_.reset();
        fill_indices_count_ = 0;
        return;
    }

    VkDeviceSize vb_size = verts.size() * sizeof(math::Vec3);
    VkDeviceSize ib_size = indices.size() * sizeof(uint32_t);

    if (!fill_vertex_buffer_ || fill_vertex_buffer_->get_size() < vb_size) {
        fill_vertex_buffer_ = pool.allocate_buffer(
            vb_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );
    }
    if (!fill_index_buffer_ || fill_index_buffer_->get_size() < ib_size) {
        fill_index_buffer_ = pool.allocate_buffer(
            ib_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );
    }

    if (auto* vb = fill_vertex_buffer_ ? fill_vertex_buffer_->map() : nullptr) {
        std::memcpy(vb, verts.data(), static_cast<size_t>(vb_size));
        fill_vertex_buffer_->unmap();
    }
    if (auto* ib = fill_index_buffer_ ? fill_index_buffer_->map() : nullptr) {
        std::memcpy(ib, indices.data(), static_cast<size_t>(ib_size));
        fill_index_buffer_->unmap();
    }

    fill_indices_count_ = indices.size();
}

void VMobject::update_gpu_stroke_buffers(MemoryPool& pool, const math::Mat4& world_transform) {
    constexpr uint32_t kDefaultSegments = 12;
    const uint32_t segments = std::max<uint32_t>(tessellation_segments_, kDefaultSegments);

    std::vector<math::Vec3> verts;
    std::vector<uint32_t> indices;
    build_stroke_geometry(world_transform, segments, verts, indices);

    if (verts.empty() || indices.size() < 3) {
        stroke_vertex_buffer_.reset();
        stroke_index_buffer_.reset();
        stroke_indices_count_ = 0;
        return;
    }

    VkDeviceSize vb_size = verts.size() * sizeof(math::Vec3);
    VkDeviceSize ib_size = indices.size() * sizeof(uint32_t);

    if (!stroke_vertex_buffer_ || stroke_vertex_buffer_->get_size() < vb_size) {
        stroke_vertex_buffer_ = pool.allocate_buffer(
            vb_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );
    }
    if (!stroke_index_buffer_ || stroke_index_buffer_->get_size() < ib_size) {
        stroke_index_buffer_ = pool.allocate_buffer(
            ib_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );
    }

    if (auto* vb = stroke_vertex_buffer_ ? stroke_vertex_buffer_->map() : nullptr) {
        std::memcpy(vb, verts.data(), static_cast<size_t>(vb_size));
        stroke_vertex_buffer_->unmap();
    }
    if (auto* ib = stroke_index_buffer_ ? stroke_index_buffer_->map() : nullptr) {
        std::memcpy(ib, indices.data(), static_cast<size_t>(ib_size));
        stroke_index_buffer_->unmap();
    }

    stroke_indices_count_ = indices.size();
}

}  // namespace manim
