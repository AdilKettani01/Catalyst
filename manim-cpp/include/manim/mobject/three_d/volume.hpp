#pragma once

#include "manim/mobject/mobject.hpp"
#include "manim/core/math.hpp"
#include <functional>

namespace manim {

/**
 * @brief GPU-accelerated volumetric rendering
 *
 * Renders 3D scalar fields using ray marching on GPU.
 * Applications: smoke, clouds, fire, medical imaging, scientific visualization
 *
 * GPU Techniques:
 * - Ray marching in fragment/compute shader
 * - 3D texture sampling
 * - Transfer functions for color mapping
 * - Emission-absorption model
 * - Adaptive step size
 */
class GPUVolume : public Mobject {
public:
    GPUVolume() = default;
    virtual ~GPUVolume() = default;

    // ========================================================================
    // Volume Data
    // ========================================================================

    /**
     * @brief Set volume data from 3D array
     * @param data 3D array of scalar values [0, 1]
     * @param width, height, depth Dimensions
     */
    void set_volume_data(
        const float* data,
        uint32_t width,
        uint32_t height,
        uint32_t depth
    );

    /**
     * @brief Set volume data from function
     * @param func Function(x, y, z) -> density [0, 1]
     * @param resolution Resolution of voxel grid
     */
    void set_volume_from_function(
        std::function<float(float, float, float)> func,
        uint32_t resolution_x,
        uint32_t resolution_y,
        uint32_t resolution_z,
        const math::Vec3& bounds_min = math::Vec3{-1, -1, -1},
        const math::Vec3& bounds_max = math::Vec3{ 1,  1,  1}
    );

    // ========================================================================
    // Transfer Function
    // ========================================================================

    /**
     * @brief Transfer function maps density to color and opacity
     */
    struct TransferFunction {
        std::vector<math::Vec4> color_map;  // Density -> RGBA
        float density_min{0.0f};
        float density_max{1.0f};
    };

    /**
     * @brief Set transfer function
     */
    void set_transfer_function(const TransferFunction& tf);

    /**
     * @brief Create gradient transfer function
     */
    static TransferFunction create_gradient_transfer_function(
        const std::vector<math::Vec4>& colors,
        const std::vector<float>& positions
    );

    /**
     * @brief Preset transfer functions
     */
    static TransferFunction preset_fire();
    static TransferFunction preset_smoke();
    static TransferFunction preset_cloud();
    static TransferFunction preset_medical_bone();

    // ========================================================================
    // GPU Ray Marching
    // ========================================================================

    /**
     * @brief Render volume using ray marching
     * @param cmd Vulkan command buffer
     * @param view_proj View-projection matrix
     * @param camera_pos Camera position
     */
    void ray_march_gpu(
        VkCommandBuffer cmd,
        const math::Mat4& view_proj,
        const math::Vec3& camera_pos
    );

    /**
     * @brief Render volumetric lighting (with scattering)
     */
    void render_volumetric_lighting(
        VkCommandBuffer cmd,
        const math::Mat4& view_proj,
        const math::Vec3& light_pos
    );

    // ========================================================================
    // Ray Marching Parameters
    // ========================================================================

    struct RayMarchParams {
        uint32_t max_steps{256};          // Maximum ray march steps
        float step_size{0.01f};            // Ray march step size
        float density_threshold{0.01f};    // Min density to accumulate
        float absorption{1.0f};            // Light absorption coefficient
        float scattering{0.5f};            // Light scattering coefficient
        bool use_adaptive_step{true};      // Adaptive step size based on density
    };

    void set_ray_march_params(const RayMarchParams& params) {
        ray_march_params_ = params;
    }

    const RayMarchParams& get_ray_march_params() const {
        return ray_march_params_;
    }

    // ========================================================================
    // GPU Textures
    // ========================================================================

    /**
     * @brief Upload volume data to GPU 3D texture
     */
    void upload_to_gpu_3d_texture(MemoryPool& pool);

    /**
     * @brief Get 3D texture
     */
    const GPUImage& get_volume_texture() const { return *volume_texture_; }

    /**
     * @brief Get transfer function texture (1D)
     */
    const GPUImage& get_transfer_function_texture() const {
        return *transfer_function_texture_;
    }

    // ========================================================================
    // Utilities
    // ========================================================================

    math::Vec3 get_dimensions() const {
        return math::Vec3{
            static_cast<float>(width_),
            static_cast<float>(height_),
            static_cast<float>(depth_)
        };
    }

    math::Vec3 get_bounds_min() const { return bounds_min_; }
    math::Vec3 get_bounds_max() const { return bounds_max_; }

    void set_bounds(const math::Vec3& min_bounds, const math::Vec3& max_bounds) {
        bounds_min_ = min_bounds;
        bounds_max_ = max_bounds;
    }

protected:
    void generate_points() override;

private:
    // CPU data
    std::vector<float> volume_data_;
    uint32_t width_{0}, height_{0}, depth_{0};

    // Bounds in world space
    math::Vec3 bounds_min_{-1, -1, -1};
    math::Vec3 bounds_max_{ 1,  1,  1};

    // Transfer function
    TransferFunction transfer_function_;

    // Ray marching
    RayMarchParams ray_march_params_;

    // GPU resources
    std::optional<GPUImage> volume_texture_;              // 3D texture
    std::optional<GPUImage> transfer_function_texture_;   // 1D texture
    std::optional<GPUBuffer> ray_march_params_buffer_;    // Uniform buffer

    bool volume_dirty_{true};
    bool transfer_function_dirty_{true};
};

}  // namespace manim
