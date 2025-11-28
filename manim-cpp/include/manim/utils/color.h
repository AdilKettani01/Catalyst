#pragma once

#include "manim/core/types.h"
#include <string>
#include <vector>

namespace manim {

// Stub color utilities for compatibility. Prefer using manim::Color static helpers in core/types.h.
inline Vec4 rgbToHsv(const Vec4& rgb) { return Color::rgbToHsv(Color(rgb)).toVec4(); }
inline Vec4 hsvToRgb(const Vec4& hsv) { return Color::hsvToRgb(Color(hsv)).toVec4(); }
inline Vec4 rgbToHsl(const Vec4& rgb) { return rgb; }
inline Vec4 hslToRgb(const Vec4& hsl) { return hsl; }
inline std::vector<Vec4> batchRgbToHsv(const std::vector<Vec4>& colors, bool = true) {
    std::vector<Vec4> out;
    out.reserve(colors.size());
    for (const auto& c : colors) out.push_back(rgbToHsv(c));
    return out;
}
inline std::vector<Vec4> batchHsvToRgb(const std::vector<Vec4>& colors, bool = true) {
    std::vector<Vec4> out;
    out.reserve(colors.size());
    for (const auto& c : colors) out.push_back(hsvToRgb(c));
    return out;
}
inline Vec4 interpolateColor(const Vec4& from, const Vec4& to, float alpha) {
    return Color::interpolateColor(Color(from), Color(to), alpha).toVec4();
}
inline Vec4 averageColor(const std::vector<Vec4>& colors) {
    if (colors.empty()) return Vec4{};
    Vec4 sum{0, 0, 0, 0};
    for (const auto& c : colors) sum += c;
    return sum / static_cast<float>(colors.size());
}
inline Vec4 invertColor(const Vec4& color) { return Vec4{1.0f, 1.0f, 1.0f, 1.0f} - color; }
inline Vec4 getNamedColor(const std::string&) { return Vec4{1.0f}; }

} // namespace manim
