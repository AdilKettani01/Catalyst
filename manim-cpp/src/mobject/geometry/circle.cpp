// Circle and related geometry implementations
#include "manim/mobject/geometry/circle.hpp"
#include <cmath>

namespace manim {

// ============================================================================
// Circle
// ============================================================================

Circle::Circle(float radius, uint32_t num_segments)
    : radius_(radius), num_segments_(num_segments) {
    generate_points();
}

Circle& Circle::set_radius(float radius) {
    radius_ = radius;
    generate_points();
    return *this;
}

Circle& Circle::surround(const Mobject& mobject, float buffer) {
    auto [min_p, max_p] = mobject.get_bounding_box();
    math::Vec3 center = (min_p + max_p) * 0.5f;
    math::Vec3 size = max_p - min_p;
    float max_dim = std::max({size.x, size.y, size.z});

    radius_ = max_dim * 0.5f * (1.0f + buffer);
    generate_points();
    move_to(center);

    return *this;
}

void Circle::generate_points() {
    cpu_points_.clear();

    float angle_step = 2.0f * math::PI / num_segments_;

    for (uint32_t i = 0; i < num_segments_; ++i) {
        float angle1 = i * angle_step;
        float angle2 = (i + 1) * angle_step;

        math::Vec3 p1{radius_ * std::cos(angle1), radius_ * std::sin(angle1), 0.0f};
        math::Vec3 p2{radius_ * std::cos(angle2), radius_ * std::sin(angle2), 0.0f};

        // Create smooth bezier curve for circular arc
        float angle_mid = (angle1 + angle2) * 0.5f;
        float control_radius = radius_ / std::cos(angle_step * 0.5f);

        math::Vec3 h1{control_radius * std::cos(angle1 + angle_step * 0.25f),
                      control_radius * std::sin(angle1 + angle_step * 0.25f), 0.0f};
        math::Vec3 h2{control_radius * std::cos(angle2 - angle_step * 0.25f),
                      control_radius * std::sin(angle2 - angle_step * 0.25f), 0.0f};

        if (i == 0) {
            cpu_points_.push_back(p1);
        }
        cpu_points_.push_back(h1);
        cpu_points_.push_back(h2);
        cpu_points_.push_back(p2);
    }

    points_dirty_ = true;
    tessellation_dirty_ = true;
}

// ============================================================================
// Dot
// ============================================================================

Dot::Dot(const math::Vec3& position, float radius)
    : Circle(radius, 16) {
    set_fill(math::Vec4{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f);
    set_stroke(math::Vec4{0.0f, 0.0f, 0.0f, 0.0f}, 0.0f);
    move_to(position);
}

// ============================================================================
// Ellipse
// ============================================================================

Ellipse::Ellipse(float width, float height, uint32_t num_segments)
    : width_(width), height_(height), num_segments_(num_segments) {
    generate_points();
}

Ellipse& Ellipse::set_width(float width) {
    width_ = width;
    generate_points();
    return *this;
}

Ellipse& Ellipse::set_height(float height) {
    height_ = height;
    generate_points();
    return *this;
}

void Ellipse::generate_points() {
    cpu_points_.clear();

    float a = width_ * 0.5f;
    float b = height_ * 0.5f;
    float angle_step = 2.0f * math::PI / num_segments_;

    for (uint32_t i = 0; i < num_segments_; ++i) {
        float angle1 = i * angle_step;
        float angle2 = (i + 1) * angle_step;

        math::Vec3 p1{a * std::cos(angle1), b * std::sin(angle1), 0.0f};
        math::Vec3 p2{a * std::cos(angle2), b * std::sin(angle2), 0.0f};

        // Control points for ellipse
        float cos1 = std::cos(angle1);
        float sin1 = std::sin(angle1);
        float cos2 = std::cos(angle2);
        float sin2 = std::sin(angle2);

        float k = 4.0f / 3.0f * std::tan(angle_step / 4.0f);

        math::Vec3 h1{a * (cos1 - k * sin1), b * (sin1 + k * cos1), 0.0f};
        math::Vec3 h2{a * (cos2 + k * sin2), b * (sin2 - k * cos2), 0.0f};

        if (i == 0) {
            cpu_points_.push_back(p1);
        }
        cpu_points_.push_back(h1);
        cpu_points_.push_back(h2);
        cpu_points_.push_back(p2);
    }

    points_dirty_ = true;
    tessellation_dirty_ = true;
}

// ============================================================================
// Arc
// ============================================================================

Arc::Arc(float radius, float start_angle, float angle, uint32_t num_segments)
    : radius_(radius), start_angle_(start_angle), angle_(angle), num_segments_(num_segments) {
    generate_points();
}

Arc& Arc::set_angle(float angle) {
    angle_ = angle;
    generate_points();
    return *this;
}

void Arc::generate_points() {
    cpu_points_.clear();

    if (std::abs(angle_) < 1e-6f) {
        return;
    }

    float angle_step = angle_ / num_segments_;

    for (uint32_t i = 0; i < num_segments_; ++i) {
        float angle1 = start_angle_ + i * angle_step;
        float angle2 = start_angle_ + (i + 1) * angle_step;

        math::Vec3 p1{radius_ * std::cos(angle1), radius_ * std::sin(angle1), 0.0f};
        math::Vec3 p2{radius_ * std::cos(angle2), radius_ * std::sin(angle2), 0.0f};

        float k = 4.0f / 3.0f * std::tan(angle_step / 4.0f);

        float cos1 = std::cos(angle1);
        float sin1 = std::sin(angle1);
        float cos2 = std::cos(angle2);
        float sin2 = std::sin(angle2);

        math::Vec3 h1{radius_ * (cos1 - k * sin1), radius_ * (sin1 + k * cos1), 0.0f};
        math::Vec3 h2{radius_ * (cos2 + k * sin2), radius_ * (sin2 - k * cos2), 0.0f};

        if (i == 0) {
            cpu_points_.push_back(p1);
        }
        cpu_points_.push_back(h1);
        cpu_points_.push_back(h2);
        cpu_points_.push_back(p2);
    }

    points_dirty_ = true;
    tessellation_dirty_ = true;
}

// ============================================================================
// Annulus
// ============================================================================

Annulus::Annulus(float inner_radius, float outer_radius, uint32_t num_segments)
    : inner_radius_(inner_radius), outer_radius_(outer_radius), num_segments_(num_segments) {
    generate_points();
}

void Annulus::generate_points() {
    cpu_points_.clear();

    float angle_step = 2.0f * math::PI / num_segments_;

    // Outer circle
    for (uint32_t i = 0; i < num_segments_; ++i) {
        float angle1 = i * angle_step;
        float angle2 = (i + 1) * angle_step;

        math::Vec3 p1{outer_radius_ * std::cos(angle1), outer_radius_ * std::sin(angle1), 0.0f};
        math::Vec3 p2{outer_radius_ * std::cos(angle2), outer_radius_ * std::sin(angle2), 0.0f};

        float k = 4.0f / 3.0f * std::tan(angle_step / 4.0f);
        float cos1 = std::cos(angle1);
        float sin1 = std::sin(angle1);
        float cos2 = std::cos(angle2);
        float sin2 = std::sin(angle2);

        math::Vec3 h1{outer_radius_ * (cos1 - k * sin1), outer_radius_ * (sin1 + k * cos1), 0.0f};
        math::Vec3 h2{outer_radius_ * (cos2 + k * sin2), outer_radius_ * (sin2 - k * cos2), 0.0f};

        if (i == 0) {
            cpu_points_.push_back(p1);
        }
        cpu_points_.push_back(h1);
        cpu_points_.push_back(h2);
        cpu_points_.push_back(p2);
    }

    // Inner circle (reversed)
    for (int i = num_segments_ - 1; i >= 0; --i) {
        float angle1 = i * angle_step;
        float angle2 = (i - 1 >= 0 ? i - 1 : num_segments_ - 1) * angle_step;

        math::Vec3 p1{inner_radius_ * std::cos(angle1), inner_radius_ * std::sin(angle1), 0.0f};
        math::Vec3 p2{inner_radius_ * std::cos(angle2), inner_radius_ * std::sin(angle2), 0.0f};

        float k = 4.0f / 3.0f * std::tan(angle_step / 4.0f);
        float cos1 = std::cos(angle1);
        float sin1 = std::sin(angle1);
        float cos2 = std::cos(angle2);
        float sin2 = std::sin(angle2);

        math::Vec3 h1{inner_radius_ * (cos1 + k * sin1), inner_radius_ * (sin1 - k * cos1), 0.0f};
        math::Vec3 h2{inner_radius_ * (cos2 - k * sin2), inner_radius_ * (sin2 + k * cos2), 0.0f};

        cpu_points_.push_back(h1);
        cpu_points_.push_back(h2);
        cpu_points_.push_back(p2);
    }

    points_dirty_ = true;
    tessellation_dirty_ = true;
}

}  // namespace manim
