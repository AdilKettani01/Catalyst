#pragma once

#include "manim/mobject/vmobject.hpp"
#include "manim/mobject/geometry/square.hpp"

namespace manim {

class Rectangle : public VMobject {
public:
    Rectangle(float width = 2.0f, float height = 1.0f) {
        (void)width;
        (void)height;
        // Geometry generation is stubbed; parameters kept for API parity.
    }
    ~Rectangle() override = default;
};

}  // namespace manim
