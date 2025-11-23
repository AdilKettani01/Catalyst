#pragma once

#include "manim/core/types.h"
#include <vector>

namespace manim {
namespace SpaceOps {

// Matrix operations with GPU acceleration
std::vector<Mat4> batchMatrixMultiply(const std::vector<Mat4>& a, const std::vector<Mat4>& b);
std::vector<Mat4> batchMatrixInverse(const std::vector<Mat4>& matrices);
std::vector<Vec3> batchMatrixTransform(const std::vector<Mat4>& matrices, const std::vector<Vec3>& vectors);

// Rotations
Mat4 rotationMatrix(const Vec3& axis, float angle);
Mat4 rotationBetweenVectors(const Vec3& from, const Vec3& to);
Vec3 rotateVector(const Vec3& vec, const Vec3& axis, float angle);

// Transformations
Mat4 scaleMatrix(const Vec3& scale);
Mat4 translationMatrix(const Vec3& translation);
Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);

// Utility
float angleBetween(const Vec3& v1, const Vec3& v2);
Vec3 projectVector(const Vec3& v, const Vec3& onto);
Vec3 normalize(const Vec3& v);

} // namespace SpaceOps
} // namespace manim
