#pragma once

#include "manim/mobject/three_d/mesh.hpp"

namespace manim {

// Simplified surface placeholder: lightweight type derived from GPUMesh.
class Surface : public GPUMesh {
public:
    Surface() = default;
    ~Surface() override = default;
};

using GPUSurface = Surface;

}  // namespace manim
