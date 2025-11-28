#pragma once

#include "manim/mobject/vmobject.hpp"
#include <vector>

namespace manim {

class Polygon : public VMobject {
public:
    Polygon() = default;
    explicit Polygon(const std::vector<math::Vec3>& points) { set_points(points); }
    ~Polygon() override = default;
};

}  // namespace manim
