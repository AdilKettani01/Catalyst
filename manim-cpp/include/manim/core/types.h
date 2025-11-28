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
    explicit Color(const Vec4& v) : r(v.x), g(v.y), b(v.z), a(v.w) {}

    Vec4 toVec4() const { return Vec4(r, g, b, a); }
    Vec3 toVec3() const { return Vec3(r, g, b); }
    operator Vec4() const { return toVec4(); }

    static Color rgbToHsv(const Color& c) { return c; }
    static Color hsvToRgb(const Color& c) { return c; }
    static Color from_hsv(float h, float s, float v) { (void)h; (void)s; (void)v; return Color(); }
    static Color interpolateColor(const Color& a, const Color& b, float t) {
        return Color(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
};

} // namespace manim
