#pragma once

#include "manim/mobject/three_d/mesh.hpp"

#include <memory>

namespace manim {

struct PBRMaterial;

// Simple sphere placeholder derived from GPUMesh.
class Sphere : public GPUMesh {
public:
    Sphere(float /*radius*/ = 1.0f, uint32_t /*rings*/ = 16, uint32_t /*segments*/ = 16) {}
    ~Sphere() override = default;
    void set_pbr_material(const PBRMaterial& /*material*/) {}
};

}  // namespace manim
