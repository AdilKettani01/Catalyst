#pragma once

#include "manim/core/types.h"
#include <vector>
#include <array>
#include <vulkan/vulkan.h>

namespace manim {

/**
 * @brief GPU-accelerated Bezier curve utilities
 */
namespace Bezier {

// ==================== CPU Bezier Functions ====================

/**
 * @brief Evaluate Bezier curve at parameter t
 * @param controlPoints Control points defining the curve
 * @param t Parameter value [0, 1]
 * @return Point on the curve
 */
Vec3 evaluate(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Evaluate multiple Bezier curves (batch processing)
 */
std::vector<Vec3> evaluateBatch(const std::vector<std::vector<Vec3>>& curves,
                                const std::vector<float>& t_values);

/**
 * @brief Get derivative of Bezier curve at parameter t
 */
Vec3 derivative(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Get second derivative at parameter t
 */
Vec3 secondDerivative(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Get tangent vector at parameter t
 */
Vec3 tangent(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Get normal vector at parameter t
 */
Vec3 normal(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Get binormal vector at parameter t
 */
Vec3 binormal(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Calculate curve length
 */
float length(const std::vector<Vec3>& controlPoints, size_t numSamples = 100);

/**
 * @brief Subdivide Bezier curve into two curves at parameter t
 */
std::pair<std::vector<Vec3>, std::vector<Vec3>> subdivide(
    const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Get bounding box of Bezier curve
 */
std::pair<Vec3, Vec3> boundingBox(const std::vector<Vec3>& controlPoints);

/**
 * @brief Convert Bezier curve to polyline
 */
std::vector<Vec3> toPolyline(const std::vector<Vec3>& controlPoints,
                             size_t numPoints);

/**
 * @brief Elevate degree of Bezier curve
 */
std::vector<Vec3> elevateDegree(const std::vector<Vec3>& controlPoints);

/**
 * @brief Find closest point on curve to a given point
 */
struct ClosestPointResult {
    Vec3 point;
    float t;
    float distance;
};
ClosestPointResult closestPoint(const std::vector<Vec3>& controlPoints,
                                const Vec3& targetPoint);

/**
 * @brief Intersect two Bezier curves
 */
std::vector<float> intersect(const std::vector<Vec3>& curve1,
                             const std::vector<Vec3>& curve2,
                             float tolerance = 1e-6f);

// ==================== GPU Bezier Functions ====================

/**
 * @brief GPU-accelerated batch Bezier evaluation
 */
class GPUBezierEvaluator {
public:
    GPUBezierEvaluator();
    ~GPUBezierEvaluator();

    /**
     * @brief Evaluate multiple curves on GPU
     * @param curves Control points for all curves
     * @param t_values Parameter values for each curve
     * @return Evaluated points
     */
    std::vector<Vec3> evaluateBatch(
        const std::vector<std::vector<Vec3>>& curves,
        const std::vector<float>& t_values
    );

    /**
     * @brief Evaluate single curve at multiple t values
     */
    std::vector<Vec3> evaluateMultipleT(
        const std::vector<Vec3>& controlPoints,
        const std::vector<float>& t_values
    );

    /**
     * @brief Compute derivatives for batch of curves
     */
    std::vector<Vec3> derivativeBatch(
        const std::vector<std::vector<Vec3>>& curves,
        const std::vector<float>& t_values
    );

    /**
     * @brief Tessellate curves into line segments on GPU
     */
    std::vector<std::vector<Vec3>> tessellateBatch(
        const std::vector<std::vector<Vec3>>& curves,
        size_t pointsPerCurve
    );

private:
    VkBuffer controlPointsBuffer;
    VkBuffer tValuesBuffer;
    VkBuffer resultBuffer;
    VkDeviceMemory controlPointsMemory;
    VkDeviceMemory tValuesMemory;
    VkDeviceMemory resultMemory;
    VkPipeline computePipeline;
    VkDescriptorSet descriptorSet;

    void createBuffers(size_t maxCurves, size_t maxPoints);
    void createComputePipeline();
    void dispatchCompute(size_t numCurves, size_t numPoints);
};

// ==================== Spline Functions ====================

/**
 * @brief Compute cubic Bezier spline through points
 */
std::vector<std::vector<Vec3>> cubicSpline(const std::vector<Vec3>& points,
                                           bool closed = false);

/**
 * @brief Compute Catmull-Rom spline
 */
std::vector<std::vector<Vec3>> catmullRomSpline(const std::vector<Vec3>& points,
                                                float tension = 0.5f,
                                                bool closed = false);

/**
 * @brief Compute B-spline
 */
std::vector<Vec3> bSpline(const std::vector<Vec3>& controlPoints,
                          size_t degree,
                          const std::vector<float>& knots,
                          const std::vector<float>& t_values);

/**
 * @brief Smooth path through points
 */
std::vector<Vec3> smoothPath(const std::vector<Vec3>& points,
                             float smoothness = 1.0f);

// ==================== Utility Functions ====================

/**
 * @brief De Casteljau's algorithm for stable evaluation
 */
Vec3 deCasteljau(const std::vector<Vec3>& controlPoints, float t);

/**
 * @brief Binomial coefficient (n choose k)
 */
inline uint64_t binomial(uint64_t n, uint64_t k) {
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;

    uint64_t result = 1;
    for (uint64_t i = 0; i < k; ++i) {
        result *= (n - i);
        result /= (i + 1);
    }
    return result;
}

/**
 * @brief Bernstein polynomial basis function
 */
inline float bernstein(size_t n, size_t i, float t) {
    return static_cast<float>(binomial(n, i)) *
           std::pow(t, static_cast<float>(i)) *
           std::pow(1.0f - t, static_cast<float>(n - i));
}

/**
 * @brief Pascal's triangle for binomial coefficients
 */
std::vector<std::vector<uint64_t>> pascalTriangle(size_t rows);

} // namespace Bezier

} // namespace manim
