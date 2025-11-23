#pragma once

#include "manim/core/types.h"
#include <string>
#include <vector>

namespace manim {
namespace Color {

// Color space conversions (GPU accelerated)
Vec4 rgbToHsv(const Vec4& rgb);
Vec4 hsvToRgb(const Vec4& hsv);
Vec4 rgbToHsl(const Vec4& rgb);
Vec4 hslToRgb(const Vec4& hsl);

// Batch conversions
std::vector<Vec4> batchRgbToHsv(const std::vector<Vec4>& colors, bool useGPU = true);
std::vector<Vec4> batchHsvToRgb(const std::vector<Vec4>& colors, bool useGPU = true);

// Color operations
Vec4 interpolateColor(const Vec4& from, const Vec4& to, float alpha);
Vec4 averageColor(const std::vector<Vec4>& colors);
Vec4 invertColor(const Vec4& color);

// Named colors
Vec4 getNamedColor(const std::string& name);

} // namespace Color
} // namespace manim
