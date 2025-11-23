#pragma once

#include "manim/mobject/three_d/mesh.hpp"
#include <functional>

namespace manim {

/**
 * @brief GPU-accelerated parametric surface
 *
 * Evaluates parametric functions on GPU for efficient surface generation.
 *
 * GPU Optimizations:
 * - Parallel surface evaluation on GPU compute shaders
 * - Adaptive tessellation based on curvature
 * - Real-time parameter animation
 * - GPU normal/tangent computation
 */
class GPUSurface : public GPUMesh {
public:
    /**
     * @brief Surface function: (u, v) -> (x, y, z)
     */
    using SurfaceFunction = std::function<math::Vec3(float, float)>;

    GPUSurface() = default;
    virtual ~GPUSurface() = default;

    // ========================================================================
    // Surface Definition
    // ========================================================================

    /**
     * @brief Set parametric surface function
     * @param func Surface function (u, v) -> position
     * @param u_range Range for u parameter [u_min, u_max]
     * @param v_range Range for v parameter [v_min, v_max]
     * @param resolution_u, resolution_v Grid resolution
     */
    void set_surface_function(
        SurfaceFunction func,
        const std::pair<float, float>& u_range = {0.0f, 1.0f},
        const std::pair<float, float>& v_range = {0.0f, 1.0f},
        uint32_t resolution_u = 32,
        uint32_t resolution_v = 32
    );

    /**
     * @brief Set surface with color function
     * @param surface_func Surface function
     * @param color_func Color function (u, v) -> color
     */
    void set_surface_with_color(
        SurfaceFunction surface_func,
        std::function<math::Vec4(float, float)> color_func,
        const std::pair<float, float>& u_range = {0.0f, 1.0f},
        const std::pair<float, float>& v_range = {0.0f, 1.0f},
        uint32_t resolution_u = 32,
        uint32_t resolution_v = 32
    );

    // ========================================================================
    // GPU Evaluation
    // ========================================================================

    /**
     * @brief Evaluate surface on GPU
     * @param engine Compute engine
     *
     * Dispatches compute shader to evaluate surface function in parallel
     */
    void evaluate_on_gpu(ComputeEngine& engine);

    /**
     * @brief Adaptive tessellation based on curvature
     * @param min_resolution Minimum grid resolution
     * @param max_resolution Maximum grid resolution
     * @param curvature_threshold Curvature threshold for subdivision
     */
    void adaptive_tessellation(
        uint32_t min_resolution,
        uint32_t max_resolution,
        float curvature_threshold
    );

    // ========================================================================
    // Common Surfaces
    // ========================================================================

    /**
     * @brief Create sphere surface
     */
    static std::shared_ptr<GPUSurface> create_sphere_surface(
        float radius = 1.0f,
        uint32_t resolution = 32
    );

    /**
     * @brief Create parametric sphere (using spherical coordinates)
     */
    static std::shared_ptr<GPUSurface> create_parametric_sphere(
        float radius = 1.0f,
        uint32_t resolution_theta = 32,
        uint32_t resolution_phi = 32
    );

    /**
     * @brief Create torus surface
     */
    static std::shared_ptr<GPUSurface> create_torus_surface(
        float major_radius = 1.0f,
        float minor_radius = 0.3f,
        uint32_t resolution_u = 48,
        uint32_t resolution_v = 24
    );

    /**
     * @brief Create mobius strip
     */
    static std::shared_ptr<GPUSurface> create_mobius_strip(
        float radius = 1.0f,
        float width = 0.5f,
        uint32_t resolution = 64
    );

    /**
     * @brief Create Klein bottle
     */
    static std::shared_ptr<GPUSurface> create_klein_bottle(
        float radius = 1.0f,
        uint32_t resolution = 64
    );

    /**
     * @brief Create parametric function plot z = f(x, y)
     */
    static std::shared_ptr<GPUSurface> create_function_plot(
        std::function<float(float, float)> func,
        const std::pair<float, float>& x_range = {-5.0f, 5.0f},
        const std::pair<float, float>& y_range = {-5.0f, 5.0f},
        uint32_t resolution = 50
    );

    /**
     * @brief Create surface of revolution
     * @param profile_func Profile curve (t) -> (r, h) in (radius, height)
     * @param t_range Range for profile parameter
     * @param rotation_segments Number of rotation segments
     */
    static std::shared_ptr<GPUSurface> create_surface_of_revolution(
        std::function<std::pair<float, float>(float)> profile_func,
        const std::pair<float, float>& t_range = {0.0f, 1.0f},
        uint32_t rotation_segments = 32,
        uint32_t profile_segments = 32
    );

    // ========================================================================
    // Utilities
    // ========================================================================

    /**
     * @brief Get parameter ranges
     */
    std::pair<float, float> get_u_range() const { return u_range_; }
    std::pair<float, float> get_v_range() const { return v_range_; }

    /**
     * @brief Get resolution
     */
    std::pair<uint32_t, uint32_t> get_resolution() const {
        return {resolution_u_, resolution_v_};
    }

    /**
     * @brief Update surface (re-evaluate with new parameters)
     */
    void update_surface();

protected:
    void generate_points() override;

private:
    /**
     * @brief Generate mesh from surface function (CPU fallback)
     */
    void generate_mesh_from_function();

    SurfaceFunction surface_func_;
    std::function<math::Vec4(float, float)> color_func_;

    std::pair<float, float> u_range_{0.0f, 1.0f};
    std::pair<float, float> v_range_{0.0f, 1.0f};

    uint32_t resolution_u_{32};
    uint32_t resolution_v_{32};

    bool has_color_func_{false};
};

}  // namespace manim
