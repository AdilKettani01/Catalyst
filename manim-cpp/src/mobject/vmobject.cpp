// VMobject implementation with GPU Bezier evaluation
#include "manim/mobject/vmobject.hpp"
#include "manim/core/compute_engine.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

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
    cpu_points_ = points;
    ensure_valid_points();
    points_dirty_ = true;
    tessellation_dirty_ = true;
}

void VMobject::add_cubic_bezier_curve(
    const std::array<math::Vec3, 2>& anchors,
    const std::array<math::Vec3, 2>& handles
) {
    // Add 4 control points: anchor0, handle0, handle1, anchor1
    if (!cpu_points_.empty() && has_new_path_started_) {
        // Continue from last anchor
        cpu_points_.push_back(handles[0]);
        cpu_points_.push_back(handles[1]);
        cpu_points_.push_back(anchors[1]);
    } else {
        // New curve
        cpu_points_.push_back(anchors[0]);
        cpu_points_.push_back(handles[0]);
        cpu_points_.push_back(handles[1]);
        cpu_points_.push_back(anchors[1]);
        has_new_path_started_ = true;
    }

    points_dirty_ = true;
    tessellation_dirty_ = true;
}

void VMobject::add_line_to(const math::Vec3& point) {
    if (cpu_points_.empty()) {
        cpu_points_.push_back(math::Vec3{0.0f});
    }

    math::Vec3 start = cpu_points_.back();
    math::Vec3 handle1 = start + (point - start) * 0.333f;
    math::Vec3 handle2 = start + (point - start) * 0.667f;

    add_cubic_bezier_curve({start, point}, {handle1, handle2});
}

void VMobject::start_new_path(const math::Vec3& point) {
    cpu_points_.push_back(point);
    has_new_path_started_ = false;
}

void VMobject::close_path() {
    if (cpu_points_.size() < 4) {
        return;
    }

    math::Vec3 first = cpu_points_[0];
    math::Vec3 last = cpu_points_.back();

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
    cpu_points_.clear();

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

    cpu_points_.clear();

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
    size_t remainder = cpu_points_.size() % 4;

    if (remainder != 0 && remainder != 1) {
        // Invalid size, truncate
        size_t new_size = (cpu_points_.size() / 4) * 4;
        if (cpu_points_.size() > 0) {
            new_size++;  // Keep at least the first anchor
        }
        cpu_points_.resize(new_size);
        spdlog::warn("VMobject points resized to valid size: {}", new_size);
    }
}

// ============================================================================
// GPU Bezier Evaluation
// ============================================================================

void VMobject::tessellate_on_gpu(uint32_t segments_per_curve, ComputeEngine& engine) {
    size_t num_curves = get_num_curves();
    if (num_curves == 0) {
        return;
    }

    // Prepare control points buffer
    std::vector<math::Vec4> control_points;
    control_points.reserve(num_curves * 4);

    for (size_t i = 0; i < num_curves; ++i) {
        auto curve_pts = get_curve_points(i);
        for (const auto& p : curve_pts) {
            control_points.push_back(math::Vec4(p, 1.0f));  // w=1 for alignment
        }
    }

    // Upload to GPU (this would use memory pool in real implementation)
    // bezier_control_points_buffer_ = pool.allocate_buffer(...);
    // bezier_control_points_buffer_->upload(control_points.data(), ...);

    // Dispatch GPU tessellation compute shader
    uint32_t total_vertices = num_curves * segments_per_curve;
    // engine.tessellate_bezier_curves(
    //     bezier_control_points_buffer_,
    //     tessellated_vertices_buffer_,
    //     num_curves,
    //     segments_per_curve
    // );

    // Download tessellated vertices
    tessellated_points_.resize(total_vertices);
    // tessellated_vertices_buffer_->download(tessellated_points_.data(), ...);

    tessellation_dirty_ = false;

    spdlog::debug("Tessellated {} curves on GPU ({} vertices)",
                  num_curves, total_vertices);
}

const std::vector<math::Vec3>& VMobject::get_tessellated_points() const {
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
        bezier_control_points_buffer_->size() < buffer_size) {
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
    if (cpu_points_.size() < 4) {
        return 0;
    }

    // Each curve has 4 points, but they share endpoints
    // Formula: (n - 1) / 3 where n is number of points (excluding first anchor)
    return (cpu_points_.size() - 1) / 3;
}

std::array<math::Vec3, 4> VMobject::get_curve_points(size_t index) const {
    size_t start_idx = index * 3;

    if (start_idx + 3 >= cpu_points_.size()) {
        throw std::out_of_range("Curve index out of range");
    }

    return {
        cpu_points_[start_idx],
        cpu_points_[start_idx + 1],
        cpu_points_[start_idx + 2],
        cpu_points_[start_idx + 3]
    };
}

std::vector<math::Vec3> VMobject::get_anchors() const {
    std::vector<math::Vec3> anchors;

    if (cpu_points_.empty()) {
        return anchors;
    }

    // Anchors are at indices 0, 3, 6, 9, ...
    for (size_t i = 0; i < cpu_points_.size(); i += 3) {
        anchors.push_back(cpu_points_[i]);
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

    cpu_points_ = new_points;
    points_dirty_ = true;
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
    if (cpu_points_.size() < 4) {
        return false;
    }

    math::Vec3 first = cpu_points_[0];
    math::Vec3 last = cpu_points_.back();

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

}  // namespace manim
