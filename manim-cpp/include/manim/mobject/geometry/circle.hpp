#pragma once

#include "manim/mobject/vmobject.hpp"

namespace manim {

/**
 * @brief Circle VMobject with GPU-optimized rendering
 */
class Circle : public VMobject {
public:
    explicit Circle(float radius = 1.0f, uint32_t num_segments = 64);
    virtual ~Circle() = default;

    /**
     * @brief Set circle radius
     */
    Circle& set_radius(float radius);

    /**
     * @brief Get radius
     */
    float get_radius() const { return radius_; }

    /**
     * @brief Surround another mobject
     */
    Circle& surround(const Mobject& mobject, float buffer = 0.2f);

protected:
    void generate_points() override;

private:
    float radius_{1.0f};
    uint32_t num_segments_{64};
};

/**
 * @brief Dot - small filled circle
 */
class Dot : public Circle {
public:
    explicit Dot(const math::Vec3& position = math::Vec3{0.0f}, float radius = 0.08f);

    static constexpr float DEFAULT_DOT_RADIUS = 0.08f;
};

/**
 * @brief Ellipse
 */
class Ellipse : public VMobject {
public:
    Ellipse(float width = 2.0f, float height = 1.0f, uint32_t num_segments = 64);

    Ellipse& set_width(float width);
    Ellipse& set_height(float height);

protected:
    void generate_points() override;

private:
    float width_{2.0f};
    float height_{1.0f};
    uint32_t num_segments_{64};
};

/**
 * @brief Arc
 */
class Arc : public VMobject {
public:
    Arc(
        float radius = 1.0f,
        float start_angle = 0.0f,
        float angle = math::PI / 2.0f,
        uint32_t num_segments = 32
    );

    Arc& set_angle(float angle);
    float get_angle() const { return angle_; }

protected:
    void generate_points() override;

private:
    float radius_{1.0f};
    float start_angle_{0.0f};
    float angle_{math::PI / 2.0f};
    uint32_t num_segments_{32};
};

/**
 * @brief Annulus (ring)
 */
class Annulus : public VMobject {
public:
    Annulus(
        float inner_radius = 0.5f,
        float outer_radius = 1.0f,
        uint32_t num_segments = 64
    );

protected:
    void generate_points() override;

private:
    float inner_radius_{0.5f};
    float outer_radius_{1.0f};
    uint32_t num_segments_{64};
};

}  // namespace manim
