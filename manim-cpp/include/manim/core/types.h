/**
 * @file types.h
 * @brief Common type definitions used throughout Manim
 */

#pragma once

#include <manim/core/math.hpp>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace manim {

// Import common math types from manim::math namespace
using math::Vec2;
using math::Vec3;
using math::Vec4;
using math::Mat3;
using math::Mat4;
using math::Quat;
using math::MatrixXf;
using math::VectorXf;
using math::Array;

// Common scalar types
using Real = float;
using Index = uint32_t;

// Color type (RGBA)
struct Color {
    float r, g, b, a;

    Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_), g(g_), b(b_), a(a_) {}

    Vec4 toVec4() const { return Vec4(r, g, b, a); }
    Vec3 toVec3() const { return Vec3(r, g, b); }
};

} // namespace manim
