/**
 * @file movement.hpp
 * @brief Movement and path-following animations
 *
 * Provides:
 * - MoveAlongPath: Move a mobject along a VMobject path
 * - Homotopy: Apply a function (x,y,z,t) -> (x',y',z') to morph points
 * - ComplexHomotopy: Homotopy in the complex plane
 */

#pragma once

#include "manim/animation/animation.hpp"
#include "manim/mobject/vmobject.hpp"
#include <complex>
#include <functional>

namespace manim {

/**
 * @brief Move a mobject along a path defined by a VMobject
 *
 * The mobject's center follows the path from start (alpha=0) to end (alpha=1).
 * The path is sampled using VMobject::point_from_proportion().
 *
 * Example:
 * @code
 * auto circle = std::make_shared<Circle>(2.0f);
 * auto dot = std::make_shared<Dot>();
 *
 * auto anim = std::make_shared<MoveAlongPath>(dot, circle, 2.0f);
 * scene.play({anim});
 * @endcode
 */
class MoveAlongPath : public Animation {
public:
    /**
     * @brief Construct a MoveAlongPath animation
     * @param mobject The mobject to move
     * @param path The VMobject path to follow
     * @param run_time Animation duration
     * @param rate_func Rate function for progress
     */
    MoveAlongPath(
        std::shared_ptr<Mobject> mobject,
        std::shared_ptr<VMobject> path,
        float run_time = 1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~MoveAlongPath() = default;

    void begin() override;
    void finish() override;

    /**
     * @brief Get the path being followed
     */
    std::shared_ptr<VMobject> get_path() const { return path_; }

    /**
     * @brief Get starting position
     */
    const math::Vec3& get_starting_position() const { return starting_position_; }

protected:
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<VMobject> path_;
    math::Vec3 starting_position_;
};

/**
 * @brief Move a mobject to a specific point
 *
 * Simple animation that moves a mobject from its current position
 * to a target position.
 */
class MoveTo : public Animation {
public:
    MoveTo(
        std::shared_ptr<Mobject> mobject,
        const math::Vec3& target,
        float run_time = 1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~MoveTo() = default;

    void begin() override;

protected:
    void interpolate_mobject(float alpha) override;

private:
    math::Vec3 target_;
    math::Vec3 start_;
};

/**
 * @brief Shift a mobject by a vector
 *
 * Moves a mobject by adding a displacement vector.
 */
class Shift : public Animation {
public:
    Shift(
        std::shared_ptr<Mobject> mobject,
        const math::Vec3& direction,
        float run_time = 1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~Shift() = default;

    void begin() override;

protected:
    void interpolate_mobject(float alpha) override;

private:
    math::Vec3 direction_;
    math::Vec3 start_;
};

// ============================================================================
// Homotopy Types
// ============================================================================

/**
 * @brief Function type for 3D homotopy: (x, y, z, t) -> Vec3
 *
 * The function takes a 3D point and a time parameter t in [0, 1],
 * and returns the transformed point.
 */
using HomotopyFunc = std::function<math::Vec3(float x, float y, float z, float t)>;

/**
 * @brief Function type for complex homotopy: (complex, t) -> complex
 *
 * The function takes a complex number (x + iy) and time t in [0, 1],
 * and returns a transformed complex number.
 */
using ComplexHomotopyFunc = std::function<std::complex<float>(std::complex<float>, float)>;

/**
 * @brief Apply a continuous deformation to a mobject over time
 *
 * Homotopy applies a function f(x, y, z, t) to every point of a mobject
 * as t varies from 0 to 1. This enables complex morphing effects.
 *
 * At t=0, points should typically remain unchanged (identity).
 * At t=1, points are in their final transformed state.
 *
 * Example - rotating twist:
 * @code
 * auto homotopy = std::make_shared<Homotopy>(
 *     [](float x, float y, float z, float t) {
 *         float angle = t * M_PI * z;  // Twist increases with z
 *         float cos_a = std::cos(angle);
 *         float sin_a = std::sin(angle);
 *         return math::Vec3(
 *             x * cos_a - y * sin_a,
 *             x * sin_a + y * cos_a,
 *             z
 *         );
 *     },
 *     square,
 *     3.0f
 * );
 * @endcode
 */
class Homotopy : public Animation {
public:
    /**
     * @brief Construct a Homotopy animation
     * @param homotopy The homotopy function (x, y, z, t) -> Vec3
     * @param mobject The mobject to transform
     * @param run_time Animation duration
     * @param rate_func Rate function for progress
     */
    Homotopy(
        HomotopyFunc homotopy,
        std::shared_ptr<Mobject> mobject,
        float run_time = 3.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~Homotopy() = default;

    void begin() override;
    void finish() override;

    /**
     * @brief Get the homotopy function
     */
    const HomotopyFunc& get_homotopy_func() const { return homotopy_; }

protected:
    void interpolate_mobject(float alpha) override;

    /**
     * @brief Apply the homotopy at a specific time
     */
    void apply_homotopy_at_time(float t);

    HomotopyFunc homotopy_;
    std::vector<math::Vec3> starting_points_;
};

/**
 * @brief Homotopy in the complex plane
 *
 * ComplexHomotopy is a specialized version of Homotopy that works
 * in the complex plane. Points are treated as complex numbers (x + iy),
 * transformed, and the result is mapped back to (x', y', z).
 * The z-coordinate is preserved.
 *
 * Example - complex rotation:
 * @code
 * auto complex_homotopy = std::make_shared<ComplexHomotopy>(
 *     [](std::complex<float> z, float t) {
 *         // Rotate by t * PI radians
 *         return z * std::exp(std::complex<float>(0, t * M_PI));
 *     },
 *     square,
 *     2.0f
 * );
 * @endcode
 *
 * Example - conformal mapping:
 * @code
 * auto conformal = std::make_shared<ComplexHomotopy>(
 *     [](std::complex<float> z, float t) {
 *         // Interpolate from identity to 1/z
 *         auto inv = 1.0f / z;
 *         return z + t * (inv - z);
 *     },
 *     grid,
 *     3.0f
 * );
 * @endcode
 */
class ComplexHomotopy : public Homotopy {
public:
    /**
     * @brief Construct a ComplexHomotopy animation
     * @param complex_homotopy The complex homotopy function
     * @param mobject The mobject to transform
     * @param run_time Animation duration
     * @param rate_func Rate function for progress
     */
    ComplexHomotopy(
        ComplexHomotopyFunc complex_homotopy,
        std::shared_ptr<Mobject> mobject,
        float run_time = 3.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~ComplexHomotopy() = default;

private:
    /**
     * @brief Convert a complex homotopy function to a 3D homotopy function
     */
    static HomotopyFunc wrap_complex_func(ComplexHomotopyFunc func);

    ComplexHomotopyFunc complex_homotopy_;
};

/**
 * @brief Apply a point-wise function to morph a mobject
 *
 * Similar to Homotopy but the function is time-independent.
 * The animation interpolates from identity to the full transformation.
 *
 * Example:
 * @code
 * auto apply_func = std::make_shared<ApplyPointwiseFunction>(
 *     [](const math::Vec3& p) {
 *         return math::Vec3(p.x * p.x, p.y, p.z);
 *     },
 *     grid
 * );
 * @endcode
 */
class ApplyPointwiseFunction : public Animation {
public:
    using PointwiseFunc = std::function<math::Vec3(const math::Vec3&)>;

    ApplyPointwiseFunction(
        PointwiseFunc func,
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~ApplyPointwiseFunction() = default;

    void begin() override;

protected:
    void interpolate_mobject(float alpha) override;

private:
    PointwiseFunc func_;
    std::vector<math::Vec3> starting_points_;
    std::vector<math::Vec3> target_points_;
};

/**
 * @brief Apply a complex function to morph a mobject (pointwise version)
 *
 * Time-independent version of ComplexHomotopy.
 * Note: Named differently from transform.hpp::ApplyComplexFunction
 */
class ApplyComplexPointwiseFunction : public ApplyPointwiseFunction {
public:
    using ComplexFunc = std::function<std::complex<float>(std::complex<float>)>;

    ApplyComplexPointwiseFunction(
        ComplexFunc func,
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~ApplyComplexPointwiseFunction() = default;

private:
    static PointwiseFunc wrap_complex_func(ComplexFunc func);
};

}  // namespace manim
