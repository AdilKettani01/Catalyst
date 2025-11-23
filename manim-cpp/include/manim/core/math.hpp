/**
 * @file math.hpp
 * @brief GPU-accelerated math library replacing NumPy
 *
 * Provides both CPU (SIMD) and GPU implementations with automatic selection.
 * API designed to be familiar to NumPy users while leveraging C++ features.
 */

#pragma once

#include <manim/core/memory_pool.hpp>
#include <manim/core/compute_engine.hpp>
#include <vector>
#include <span>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Eigen/Dense>

namespace manim::math {

// ============================================================================
// Type Aliases (matching Python/NumPy conventions)
// ============================================================================

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

// For more complex linear algebra (CPU-side)
using MatrixXf = Eigen::MatrixXf;
using VectorXf = Eigen::VectorXf;

// ============================================================================
// GPU-Accelerated Array (like NumPy ndarray)
// ============================================================================

/**
 * @brief GPU-accelerated array with automatic CPU/GPU synchronization
 *
 * This class mimics NumPy's ndarray but with GPU acceleration.
 * Data resides on GPU by default, CPU copy maintained for readback.
 */
template<typename T>
class Array {
public:
    Array() = default;

    /**
     * @brief Create array from CPU data
     */
    explicit Array(std::span<const T> data, ComputeEngine& engine, MemoryPool& pool);

    /**
     * @brief Create array of given size (uninitialized)
     */
    explicit Array(size_t size, ComputeEngine& engine, MemoryPool& pool);

    /**
     * @brief Create array filled with value
     */
    static Array filled(size_t size, const T& value, ComputeEngine& engine, MemoryPool& pool);

    /**
     * @brief Create array of zeros
     */
    static Array zeros(size_t size, ComputeEngine& engine, MemoryPool& pool);

    /**
     * @brief Create array of ones
     */
    static Array ones(size_t size, ComputeEngine& engine, MemoryPool& pool);

    /**
     * @brief Create linearly spaced array
     */
    static Array linspace(T start, T stop, size_t num, ComputeEngine& engine, MemoryPool& pool);

    /**
     * @brief Create array from range
     */
    static Array arange(T start, T stop, T step, ComputeEngine& engine, MemoryPool& pool);

    // Array operations (like NumPy)
    Array operator+(const Array& other) const;
    Array operator-(const Array& other) const;
    Array operator*(const Array& other) const;  // Element-wise
    Array operator/(const Array& other) const;

    Array operator+(const T& scalar) const;
    Array operator-(const T& scalar) const;
    Array operator*(const T& scalar) const;
    Array operator/(const T& scalar) const;

    /**
     * @brief Get data from GPU (syncs to CPU)
     */
    std::span<const T> data() const;

    /**
     * @brief Get mutable data (marks GPU as dirty)
     */
    std::span<T> data_mut();

    /**
     * @brief Get GPU buffer
     */
    const GPUBuffer& gpu_buffer() const { return gpu_buffer_; }

    /**
     * @brief Sync CPU -> GPU
     */
    void upload_to_gpu();

    /**
     * @brief Sync GPU -> CPU
     */
    void download_from_gpu();

    size_t size() const { return size_; }

    // Reductions
    T sum() const;
    T mean() const;
    T min() const;
    T max() const;

private:
    GPUBuffer gpu_buffer_;
    mutable std::vector<T> cpu_data_;
    size_t size_ = 0;

    ComputeEngine* engine_ = nullptr;
    MemoryPool* pool_ = nullptr;

    mutable bool cpu_dirty_ = true;   // CPU needs update from GPU
    mutable bool gpu_dirty_ = false;  // GPU needs update from CPU
};

// Type aliases for common array types
using FloatArray = Array<float>;
using Vec2Array = Array<Vec2>;
using Vec3Array = Array<Vec3>;
using Vec4Array = Array<Vec4>;
using Mat4Array = Array<Mat4>;

// ============================================================================
// Vector Operations (GPU-accelerated)
// ============================================================================

/**
 * @brief Normalize vectors on GPU
 */
Vec3Array normalize(const Vec3Array& vectors);

/**
 * @brief Dot product for arrays of vectors
 */
FloatArray dot(const Vec3Array& a, const Vec3Array& b);

/**
 * @brief Cross product for arrays of vectors
 */
Vec3Array cross(const Vec3Array& a, const Vec3Array& b);

/**
 * @brief Compute vector lengths
 */
FloatArray length(const Vec3Array& vectors);

/**
 * @brief Linear interpolation
 */
template<typename T>
Array<T> lerp(const Array<T>& a, const Array<T>& b, float t);

/**
 * @brief Smoothstep interpolation
 */
FloatArray smoothstep(const FloatArray& t);

// ============================================================================
// Matrix Operations (GPU-accelerated)
// ============================================================================

/**
 * @brief Transform points by matrix (batched)
 *
 * @param matrix Transformation matrix
 * @param points Array of points to transform
 * @return Transformed points
 */
Vec3Array transform(const Mat4& matrix, const Vec3Array& points);

/**
 * @brief Batch transform with different matrices for each point
 */
Vec3Array batch_transform(const Mat4Array& matrices, const Vec3Array& points);

/**
 * @brief Create rotation matrix from axis and angle
 */
Mat4 rotation_matrix(const Vec3& axis, float angle);

/**
 * @brief Create rotation matrix from quaternion
 */
Mat4 rotation_matrix_from_quaternion(const Quat& q);

/**
 * @brief Create look-at matrix
 */
Mat4 look_at(const Vec3& eye, const Vec3& center, const Vec3& up);

/**
 * @brief Create perspective projection matrix
 */
Mat4 perspective(float fovy, float aspect, float near, float far);

/**
 * @brief Create orthographic projection matrix
 */
Mat4 ortho(float left, float right, float bottom, float top, float near, float far);

// ============================================================================
// Bezier Operations (GPU-accelerated)
// ============================================================================

/**
 * @brief Evaluate cubic Bezier curve
 *
 * @param p0, p1, p2, p3 Control points
 * @param t Parameter (0 to 1)
 * @return Point on curve
 */
Vec3 bezier_point(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);

/**
 * @brief Evaluate multiple Bezier curves on GPU
 *
 * @param control_points Array of control points (4 per curve)
 * @param t_values Array of t parameters
 * @return Points on curves
 */
Vec3Array bezier_points_gpu(const Vec3Array& control_points, const FloatArray& t_values);

/**
 * @brief Tessellate Bezier curves into line segments
 *
 * @param control_points Control points (4 per curve)
 * @param segments_per_curve Number of line segments per curve
 * @return Tessellated vertices
 */
Vec3Array tessellate_bezier(const Vec3Array& control_points, uint32_t segments_per_curve);

/**
 * @brief Compute Bezier curve derivative
 */
Vec3 bezier_derivative(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);

/**
 * @brief Compute arc length of Bezier curve (adaptive quadrature)
 */
float bezier_arc_length(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3);

// ============================================================================
// Interpolation Functions (CPU-optimized)
// ============================================================================

/**
 * @brief Linear interpolation
 */
template<typename T>
inline T lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief Smoothstep interpolation (smooth s-curve)
 */
inline float smoothstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief Smootherstep (even smoother)
 */
inline float smootherstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/**
 * @brief Ease in (quadratic)
 */
inline float ease_in_quad(float t) {
    return t * t;
}

/**
 * @brief Ease out (quadratic)
 */
inline float ease_out_quad(float t) {
    return t * (2.0f - t);
}

/**
 * @brief Ease in-out (quadratic)
 */
inline float ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

// ============================================================================
// Geometric Functions
// ============================================================================

/**
 * @brief Angle between two vectors (radians)
 */
float angle_between(const Vec3& a, const Vec3& b);

/**
 * @brief Project vector a onto vector b
 */
Vec3 project(const Vec3& a, const Vec3& b);

/**
 * @brief Reflect vector across normal
 */
Vec3 reflect(const Vec3& v, const Vec3& normal);

/**
 * @brief Rotate vector around axis
 */
Vec3 rotate_vector(const Vec3& v, const Vec3& axis, float angle);

/**
 * @brief Convert Euler angles to quaternion
 */
Quat euler_to_quat(float pitch, float yaw, float roll);

/**
 * @brief Convert quaternion to Euler angles
 */
Vec3 quat_to_euler(const Quat& q);

/**
 * @brief Spherical linear interpolation for quaternions
 */
Quat slerp(const Quat& a, const Quat& b, float t);

// ============================================================================
// Color Operations
// ============================================================================

/**
 * @brief Convert RGB to HSV
 */
Vec3 rgb_to_hsv(const Vec3& rgb);

/**
 * @brief Convert HSV to RGB
 */
Vec3 hsv_to_rgb(const Vec3& hsv);

/**
 * @brief Interpolate colors in RGB space
 */
Vec4 color_lerp(const Vec4& a, const Vec4& b, float t);

/**
 * @brief Interpolate colors in HSV space (better for gradients)
 */
Vec4 color_lerp_hsv(const Vec4& a, const Vec4& b, float t);

// ============================================================================
// Random Number Generation
// ============================================================================

/**
 * @brief Random float in range [min, max]
 */
float random_float(float min = 0.0f, float max = 1.0f);

/**
 * @brief Random Vec3 with components in range [min, max]
 */
Vec3 random_vec3(float min = 0.0f, float max = 1.0f);

/**
 * @brief Random point on unit sphere
 */
Vec3 random_on_sphere();

/**
 * @brief Random point in unit sphere
 */
Vec3 random_in_sphere();

// ============================================================================
// Constants
// ============================================================================

constexpr float PI = 3.14159265358979323846f;
constexpr float TAU = 2.0f * PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

constexpr Vec3 RIGHT{1.0f, 0.0f, 0.0f};
constexpr Vec3 UP{0.0f, 1.0f, 0.0f};
constexpr Vec3 FORWARD{0.0f, 0.0f, 1.0f};  // Or -1 depending on convention
constexpr Vec3 ORIGIN{0.0f, 0.0f, 0.0f};

} // namespace manim::math
