#pragma once

#include "manim/core/math.hpp"

namespace manim {

/**
 * @brief Minimal camera stub used by renderer interfaces.
 */
class Camera {
public:
    virtual ~Camera() = default;

    virtual void update(float /*dt*/) {}
};

}  // namespace manim
