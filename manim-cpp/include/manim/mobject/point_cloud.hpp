#pragma once

#include "manim/mobject/mobject.hpp"
#include "manim/core/math.hpp"
#include <vector>
#include <optional>

namespace manim {

/**
 * @brief Point cloud mobject for efficient rendering of many points
 *
 * Optimized for rendering millions of points/particles on GPU.
 * Uses structure-of-arrays (SoA) layout for cache efficiency.
 *
 * GPU Optimizations:
 * - SoA memory layout for coalesced access
 * - GPU instanced rendering
 * - Compute shader updates
 * - Persistent mapped buffers for low latency
 */
class PointCloudMobject : public Mobject {
public:
    PointCloudMobject() = default;
    virtual ~PointCloudMobject() = default;

    // ========================================================================
    // Point Management
    // ========================================================================

    /**
     * @brief Set points with positions
     */
    void set_points(const std::vector<math::Vec3>& positions) override;

    /**
     * @brief Set points with positions and colors
     */
    void set_points_with_colors(
        const std::vector<math::Vec3>& positions,
        const std::vector<math::Vec4>& colors
    );

    /**
     * @brief Set points with full attributes
     */
    void set_points_with_attributes(
        const std::vector<math::Vec3>& positions,
        const std::vector<math::Vec4>& colors,
        const std::vector<float>& sizes
    );

    /**
     * @brief Add a single point
     */
    void add_point(const math::Vec3& position,
                   const math::Vec4& color = math::Vec4{1.0f},
                   float size = 1.0f);

    // ========================================================================
    // Rendering Properties
    // ========================================================================

    /**
     * @brief Set default point size
     */
    void set_point_size(float size);

    /**
     * @brief Set individual point sizes
     */
    void set_point_sizes(const std::vector<float>& sizes);

    /**
     * @brief Set point colors
     */
    void set_point_colors(const std::vector<math::Vec4>& colors);

    /**
     * @brief Set uniform color for all points
     */
    void set_uniform_color(const math::Vec4& color);

    // ========================================================================
    // GPU SoA Layout
    // ========================================================================

    /**
     * @brief Structure-of-Arrays layout for GPU efficiency
     *
     * Separate buffers for positions, colors, sizes for better cache locality
     */
    struct GPUPointCloudSoA {
        std::optional<GPUBuffer> positions;  // vec3[]
        std::optional<GPUBuffer> colors;     // vec4[]
        std::optional<GPUBuffer> sizes;      // float[]
        uint32_t count{0};
    };

    /**
     * @brief Upload point cloud to GPU using SoA layout
     */
    void upload_to_gpu_soa(MemoryPool& pool);

    /**
     * @brief Get GPU SoA buffers
     */
    const GPUPointCloudSoA& get_gpu_soa() const { return gpu_soa_; }

    // ========================================================================
    // Billboarding
    // ========================================================================

    enum class BillboardMode {
        None,           // No billboarding
        Spherical,      // Always face camera
        Cylindrical     // Rotate around Y axis only
    };

    void set_billboard_mode(BillboardMode mode) { billboard_mode_ = mode; }
    BillboardMode get_billboard_mode() const { return billboard_mode_; }

    // ========================================================================
    // Utilities
    // ========================================================================

    size_t get_num_points() const { return positions_.size(); }

    const std::vector<math::Vec3>& get_positions() const { return positions_; }
    const std::vector<math::Vec4>& get_colors() const { return colors_; }
    const std::vector<float>& get_sizes() const { return sizes_; }

    Ptr copy() const override {
        auto copied = std::make_shared<PointCloudMobject>();
        copied->positions_ = positions_;
        copied->colors_ = colors_;
        copied->sizes_ = sizes_;
        copied->default_point_size_ = default_point_size_;
        copied->billboard_mode_ = billboard_mode_;
        copied->gpu_dirty_ = true;
        return copied;
    }

protected:
    void generate_points() override;

private:
    // CPU data (SoA)
    std::vector<math::Vec3> positions_;
    std::vector<math::Vec4> colors_;
    std::vector<float> sizes_;

    // GPU data (SoA)
    GPUPointCloudSoA gpu_soa_;

    // Rendering properties
    float default_point_size_{1.0f};
    BillboardMode billboard_mode_{BillboardMode::Spherical};

    // Dirty flags
    bool soa_dirty_{true};
};

/**
 * @brief PMobject - Python Manim compatibility name
 */
using PMobject = PointCloudMobject;

}  // namespace manim
