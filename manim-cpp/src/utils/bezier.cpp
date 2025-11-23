#include "manim/utils/bezier.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

namespace manim {
namespace Bezier {

// ==================== CPU Bezier Functions ====================

Vec3 evaluate(const std::vector<Vec3>& controlPoints, float t) {
    if (controlPoints.empty()) return Vec3(0, 0, 0);
    if (controlPoints.size() == 1) return controlPoints[0];

    // Use De Casteljau's algorithm for numerical stability
    return deCasteljau(controlPoints, t);
}

std::vector<Vec3> evaluateBatch(const std::vector<std::vector<Vec3>>& curves,
                                const std::vector<float>& t_values) {
    std::vector<Vec3> results;
    results.reserve(curves.size());

    for (size_t i = 0; i < curves.size() && i < t_values.size(); ++i) {
        results.push_back(evaluate(curves[i], t_values[i]));
    }

    return results;
}

Vec3 derivative(const std::vector<Vec3>& controlPoints, float t) {
    if (controlPoints.size() < 2) return Vec3(0, 0, 0);

    size_t n = controlPoints.size() - 1;
    std::vector<Vec3> derivativePoints(n);

    // Compute derivative control points
    for (size_t i = 0; i < n; ++i) {
        derivativePoints[i] = (controlPoints[i + 1] - controlPoints[i]) * static_cast<float>(n);
    }

    return evaluate(derivativePoints, t);
}

Vec3 secondDerivative(const std::vector<Vec3>& controlPoints, float t) {
    if (controlPoints.size() < 3) return Vec3(0, 0, 0);

    // Derivative of derivative
    size_t n = controlPoints.size() - 1;
    std::vector<Vec3> firstDeriv(n);

    for (size_t i = 0; i < n; ++i) {
        firstDeriv[i] = (controlPoints[i + 1] - controlPoints[i]) * static_cast<float>(n);
    }

    return derivative(firstDeriv, t);
}

Vec3 tangent(const std::vector<Vec3>& controlPoints, float t) {
    Vec3 deriv = derivative(controlPoints, t);
    float len = glm::length(deriv);
    return len > 1e-8f ? deriv / len : Vec3(1, 0, 0);
}

Vec3 normal(const std::vector<Vec3>& controlPoints, float t) {
    Vec3 d1 = derivative(controlPoints, t);
    Vec3 d2 = secondDerivative(controlPoints, t);

    Vec3 n = glm::cross(glm::cross(d1, d2), d1);
    float len = glm::length(n);
    return len > 1e-8f ? n / len : Vec3(0, 1, 0);
}

Vec3 binormal(const std::vector<Vec3>& controlPoints, float t) {
    Vec3 t_vec = tangent(controlPoints, t);
    Vec3 n_vec = normal(controlPoints, t);
    return glm::cross(t_vec, n_vec);
}

float length(const std::vector<Vec3>& controlPoints, size_t numSamples) {
    if (controlPoints.size() < 2) return 0.0f;

    float totalLength = 0.0f;
    Vec3 prevPoint = controlPoints[0];

    for (size_t i = 1; i <= numSamples; ++i) {
        float t = static_cast<float>(i) / numSamples;
        Vec3 point = evaluate(controlPoints, t);
        totalLength += glm::distance(point, prevPoint);
        prevPoint = point;
    }

    return totalLength;
}

std::pair<std::vector<Vec3>, std::vector<Vec3>> subdivide(
    const std::vector<Vec3>& controlPoints, float t) {

    if (controlPoints.empty()) {
        return {{}, {}};
    }

    size_t n = controlPoints.size();
    std::vector<Vec3> left(n);
    std::vector<Vec3> right(n);

    // Use De Casteljau's algorithm
    std::vector<std::vector<Vec3>> levels(n);
    levels[0] = controlPoints;

    for (size_t level = 1; level < n; ++level) {
        levels[level].resize(n - level);
        for (size_t i = 0; i < n - level; ++i) {
            levels[level][i] = levels[level - 1][i] * (1.0f - t) +
                               levels[level - 1][i + 1] * t;
        }
    }

    // Extract left and right curves
    for (size_t i = 0; i < n; ++i) {
        left[i] = levels[i][0];
        right[i] = levels[n - 1 - i][i];
    }

    return {left, right};
}

std::pair<Vec3, Vec3> boundingBox(const std::vector<Vec3>& controlPoints) {
    if (controlPoints.empty()) {
        return {Vec3(0), Vec3(0)};
    }

    Vec3 min_point = controlPoints[0];
    Vec3 max_point = controlPoints[0];

    for (const auto& point : controlPoints) {
        min_point = glm::min(min_point, point);
        max_point = glm::max(max_point, point);
    }

    // Sample curve to refine bounds
    for (size_t i = 0; i <= 20; ++i) {
        float t = static_cast<float>(i) / 20.0f;
        Vec3 point = evaluate(controlPoints, t);
        min_point = glm::min(min_point, point);
        max_point = glm::max(max_point, point);
    }

    return {min_point, max_point};
}

std::vector<Vec3> toPolyline(const std::vector<Vec3>& controlPoints,
                             size_t numPoints) {
    std::vector<Vec3> polyline;
    polyline.reserve(numPoints);

    for (size_t i = 0; i < numPoints; ++i) {
        float t = static_cast<float>(i) / (numPoints - 1);
        polyline.push_back(evaluate(controlPoints, t));
    }

    return polyline;
}

std::vector<Vec3> elevateDegree(const std::vector<Vec3>& controlPoints) {
    if (controlPoints.empty()) return {};

    size_t n = controlPoints.size();
    std::vector<Vec3> elevated(n + 1);

    elevated[0] = controlPoints[0];
    elevated[n] = controlPoints[n - 1];

    for (size_t i = 1; i < n; ++i) {
        float ratio = static_cast<float>(i) / n;
        elevated[i] = controlPoints[i - 1] * (1.0f - ratio) +
                      controlPoints[i] * ratio;
    }

    return elevated;
}

ClosestPointResult closestPoint(const std::vector<Vec3>& controlPoints,
                                const Vec3& targetPoint) {
    // Use Newton-Raphson method for refinement
    float t = 0.5f;
    const int maxIterations = 20;
    const float epsilon = 1e-6f;

    for (int iter = 0; iter < maxIterations; ++iter) {
        Vec3 point = evaluate(controlPoints, t);
        Vec3 d1 = derivative(controlPoints, t);
        Vec3 d2 = secondDerivative(controlPoints, t);

        Vec3 diff = point - targetPoint;
        float numerator = glm::dot(diff, d1);
        float denominator = glm::dot(d1, d1) + glm::dot(diff, d2);

        if (std::abs(denominator) < epsilon) break;

        float dt = -numerator / denominator;
        t = std::clamp(t + dt, 0.0f, 1.0f);

        if (std::abs(dt) < epsilon) break;
    }

    Vec3 point = evaluate(controlPoints, t);
    return {point, t, glm::distance(point, targetPoint)};
}

std::vector<float> intersect(const std::vector<Vec3>& curve1,
                             const std::vector<Vec3>& curve2,
                             float tolerance) {
    std::vector<float> intersections;

    // Simple grid-based search
    const size_t samples = 100;
    for (size_t i = 0; i < samples; ++i) {
        float t1 = static_cast<float>(i) / samples;
        Vec3 p1 = evaluate(curve1, t1);

        for (size_t j = 0; j < samples; ++j) {
            float t2 = static_cast<float>(j) / samples;
            Vec3 p2 = evaluate(curve2, t2);

            if (glm::distance(p1, p2) < tolerance) {
                intersections.push_back(t1);
                break;
            }
        }
    }

    return intersections;
}

// ==================== GPU Bezier Functions ====================

GPUBezierEvaluator::GPUBezierEvaluator()
    : controlPointsBuffer(VK_NULL_HANDLE)
    , tValuesBuffer(VK_NULL_HANDLE)
    , resultBuffer(VK_NULL_HANDLE)
    , controlPointsMemory(VK_NULL_HANDLE)
    , tValuesMemory(VK_NULL_HANDLE)
    , resultMemory(VK_NULL_HANDLE)
    , computePipeline(VK_NULL_HANDLE)
    , descriptorSet(VK_NULL_HANDLE)
{
    std::cout << "GPU Bezier evaluator initialized" << std::endl;
    // TODO: Initialize Vulkan resources
}

GPUBezierEvaluator::~GPUBezierEvaluator() {
    // TODO: Cleanup Vulkan resources
}

std::vector<Vec3> GPUBezierEvaluator::evaluateBatch(
    const std::vector<std::vector<Vec3>>& curves,
    const std::vector<float>& t_values
) {
    std::cout << "GPU batch evaluation of " << curves.size() << " curves" << std::endl;

    // TODO: Implement GPU batch evaluation
    // For now, fallback to CPU
    return Bezier::evaluateBatch(curves, t_values);
}

std::vector<Vec3> GPUBezierEvaluator::evaluateMultipleT(
    const std::vector<Vec3>& controlPoints,
    const std::vector<float>& t_values
) {
    std::vector<Vec3> results;
    results.reserve(t_values.size());

    // TODO: GPU implementation
    for (float t : t_values) {
        results.push_back(evaluate(controlPoints, t));
    }

    return results;
}

std::vector<Vec3> GPUBezierEvaluator::derivativeBatch(
    const std::vector<std::vector<Vec3>>& curves,
    const std::vector<float>& t_values
) {
    std::vector<Vec3> results;
    results.reserve(curves.size());

    // TODO: GPU implementation
    for (size_t i = 0; i < curves.size() && i < t_values.size(); ++i) {
        results.push_back(derivative(curves[i], t_values[i]));
    }

    return results;
}

std::vector<std::vector<Vec3>> GPUBezierEvaluator::tessellateBatch(
    const std::vector<std::vector<Vec3>>& curves,
    size_t pointsPerCurve
) {
    std::vector<std::vector<Vec3>> results;
    results.reserve(curves.size());

    // TODO: GPU implementation
    for (const auto& curve : curves) {
        results.push_back(toPolyline(curve, pointsPerCurve));
    }

    return results;
}

void GPUBezierEvaluator::createBuffers(size_t maxCurves, size_t maxPoints) {
    // TODO: Create Vulkan buffers
}

void GPUBezierEvaluator::createComputePipeline() {
    // TODO: Create compute pipeline for Bezier evaluation
}

void GPUBezierEvaluator::dispatchCompute(size_t numCurves, size_t numPoints) {
    // TODO: Dispatch compute shader
}

// ==================== Spline Functions ====================

std::vector<std::vector<Vec3>> cubicSpline(const std::vector<Vec3>& points,
                                           bool closed) {
    if (points.size() < 2) return {};

    std::vector<std::vector<Vec3>> segments;
    size_t n = closed ? points.size() : points.size() - 1;

    for (size_t i = 0; i < n; ++i) {
        Vec3 p0 = points[i];
        Vec3 p3 = points[(i + 1) % points.size()];

        // Compute control points for cubic Bezier
        Vec3 p1 = p0 + (p3 - p0) * 0.33f;
        Vec3 p2 = p0 + (p3 - p0) * 0.67f;

        segments.push_back({p0, p1, p2, p3});
    }

    return segments;
}

std::vector<std::vector<Vec3>> catmullRomSpline(const std::vector<Vec3>& points,
                                                float tension,
                                                bool closed) {
    if (points.size() < 4) return {};

    std::vector<std::vector<Vec3>> segments;
    size_t n = closed ? points.size() : points.size() - 3;

    for (size_t i = 0; i < n; ++i) {
        Vec3 p0 = points[i];
        Vec3 p1 = points[(i + 1) % points.size()];
        Vec3 p2 = points[(i + 2) % points.size()];
        Vec3 p3 = points[(i + 3) % points.size()];

        // Convert Catmull-Rom to Bezier
        Vec3 t1 = (p2 - p0) * tension;
        Vec3 t2 = (p3 - p1) * tension;

        Vec3 b0 = p1;
        Vec3 b1 = p1 + t1 / 3.0f;
        Vec3 b2 = p2 - t2 / 3.0f;
        Vec3 b3 = p2;

        segments.push_back({b0, b1, b2, b3});
    }

    return segments;
}

std::vector<Vec3> bSpline(const std::vector<Vec3>& controlPoints,
                          size_t degree,
                          const std::vector<float>& knots,
                          const std::vector<float>& t_values) {
    // TODO: Implement B-spline evaluation
    std::vector<Vec3> results;
    results.reserve(t_values.size());

    for (float t : t_values) {
        results.push_back(controlPoints.empty() ? Vec3(0) : controlPoints[0]);
    }

    return results;
}

std::vector<Vec3> smoothPath(const std::vector<Vec3>& points,
                             float smoothness) {
    if (points.size() < 3) return points;

    // Simple Laplacian smoothing
    std::vector<Vec3> smoothed = points;
    size_t iterations = static_cast<size_t>(smoothness * 10);

    for (size_t iter = 0; iter < iterations; ++iter) {
        std::vector<Vec3> temp = smoothed;

        for (size_t i = 1; i < smoothed.size() - 1; ++i) {
            temp[i] = (smoothed[i - 1] + smoothed[i] * 2.0f + smoothed[i + 1]) * 0.25f;
        }

        smoothed = temp;
    }

    return smoothed;
}

// ==================== Utility Functions ====================

Vec3 deCasteljau(const std::vector<Vec3>& controlPoints, float t) {
    if (controlPoints.empty()) return Vec3(0);
    if (controlPoints.size() == 1) return controlPoints[0];

    std::vector<Vec3> points = controlPoints;

    while (points.size() > 1) {
        std::vector<Vec3> nextLevel(points.size() - 1);

        for (size_t i = 0; i < nextLevel.size(); ++i) {
            nextLevel[i] = points[i] * (1.0f - t) + points[i + 1] * t;
        }

        points = nextLevel;
    }

    return points[0];
}

std::vector<std::vector<uint64_t>> pascalTriangle(size_t rows) {
    std::vector<std::vector<uint64_t>> triangle(rows);

    for (size_t n = 0; n < rows; ++n) {
        triangle[n].resize(n + 1);
        triangle[n][0] = triangle[n][n] = 1;

        for (size_t k = 1; k < n; ++k) {
            triangle[n][k] = triangle[n - 1][k - 1] + triangle[n - 1][k];
        }
    }

    return triangle;
}

} // namespace Bezier
} // namespace manim
