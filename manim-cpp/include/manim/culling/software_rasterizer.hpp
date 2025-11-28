#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <memory>
#include <atomic>

namespace manim {
namespace culling {

/**
 * @brief Configuration for software rasterizer
 */
struct SoftwareRasterizerConfig {
    uint32_t depth_buffer_width = 128;    ///< Coarse depth buffer width
    uint32_t depth_buffer_height = 72;    ///< Coarse depth buffer height (16:9 ratio)
    uint32_t tile_size = 8;               ///< Tile size for hierarchical processing
    float small_occluder_threshold = 64.0f; ///< Max screen pixels to use software raster
    bool use_simd = true;                 ///< Use SIMD acceleration (AVX2)
    bool enable_hierarchical = true;      ///< Use hierarchical depth buffer
    uint32_t max_triangles_per_object = 1000; ///< Max triangles to rasterize per object
};

/**
 * @brief Triangle for software rasterization
 */
struct alignas(16) RasterTriangle {
    math::Vec4 v0;  // xyz = position, w = 1/w
    math::Vec4 v1;
    math::Vec4 v2;
    uint32_t object_id;
    uint32_t flags;
    float padding[2];
};

/**
 * @brief Push constants for software rasterization compute shader
 */
struct SoftwareRasterPushConstants {
    math::Mat4 view_proj;
    uint32_t depth_width;
    uint32_t depth_height;
    uint32_t num_triangles;
    uint32_t flags;
};

/**
 * @brief Software rasterizer for small occluders
 *
 * This implements a CPU-side coarse rasterizer optimized for small objects
 * that would be expensive to process with full GPU occlusion culling.
 *
 * Key features:
 * - Coarse depth buffer (128x72 default) for approximate occlusion
 * - SIMD-accelerated triangle rasterization (AVX2)
 * - Hierarchical depth buffer with tile-based processing
 * - GPU compute shader alternative for batched processing
 *
 * Use cases:
 * - Small objects (< 64 pixels on screen)
 * - Static occluders that can be rasterized once
 * - Scenes with many small objects where Hi-Z is too expensive
 *
 * Integration:
 * @code
 * SoftwareRasterizer rasterizer;
 * rasterizer.initialize(device, memory_pool, queue_family);
 *
 * // Rasterize occluders
 * rasterizer.begin_frame(view_proj);
 * for (auto& occluder : small_occluders) {
 *     rasterizer.rasterize_object(occluder);
 * }
 *
 * // Test occlusion
 * for (auto& object : objects) {
 *     if (rasterizer.is_occluded(object.bounds)) {
 *         // Object is hidden, skip rendering
 *     }
 * }
 * @endcode
 */
class SoftwareRasterizer {
public:
    SoftwareRasterizer();
    ~SoftwareRasterizer();

    // Non-copyable
    SoftwareRasterizer(const SoftwareRasterizer&) = delete;
    SoftwareRasterizer& operator=(const SoftwareRasterizer&) = delete;

    /**
     * @brief Initialize the rasterizer
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t queue_family
    );

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Set configuration
     */
    void set_config(const SoftwareRasterizerConfig& config);
    const SoftwareRasterizerConfig& get_config() const { return config_; }

    // ========================================================================
    // CPU Rasterization (for small batch processing)
    // ========================================================================

    /**
     * @brief Begin a new frame - clears depth buffer
     */
    void begin_frame(const math::Mat4& view_proj, uint32_t viewport_width, uint32_t viewport_height);

    /**
     * @brief Rasterize triangles into depth buffer (CPU)
     *
     * @param triangles Array of triangles in world space
     * @param count Number of triangles
     */
    void rasterize_triangles_cpu(const RasterTriangle* triangles, uint32_t count);

    /**
     * @brief Rasterize AABB as occluder (simplified - uses 6 faces)
     */
    void rasterize_aabb_cpu(const math::Vec3& aabb_min, const math::Vec3& aabb_max);

    /**
     * @brief Test if AABB is occluded by current depth buffer
     *
     * @return true if completely occluded, false if potentially visible
     */
    bool is_occluded_cpu(const math::Vec3& aabb_min, const math::Vec3& aabb_max) const;

    /**
     * @brief Test if bounding sphere is occluded
     */
    bool is_occluded_sphere_cpu(const math::Vec3& center, float radius) const;

    /**
     * @brief Get the coarse depth buffer (for debugging)
     */
    const std::vector<float>& get_depth_buffer() const { return depth_buffer_; }

    /**
     * @brief Get hierarchical depth buffer mip
     */
    const std::vector<float>& get_hierarchical_mip(uint32_t level) const;

    // ========================================================================
    // GPU Rasterization (for large batch processing)
    // ========================================================================

    /**
     * @brief Rasterize triangles on GPU
     *
     * More efficient for large batches of small triangles.
     *
     * @param view_proj View-projection matrix
     * @param cmd_buffer Command buffer
     */
    void rasterize_triangles_gpu(
        const math::Mat4& view_proj,
        VkCommandBuffer cmd_buffer
    );

    /**
     * @brief Upload triangles to GPU buffer
     */
    void upload_triangles(const std::vector<RasterTriangle>& triangles);

    /**
     * @brief Get GPU depth buffer for shader binding
     */
    const GPUBuffer& get_gpu_depth_buffer() const { return gpu_depth_buffer_; }

    /**
     * @brief Download GPU depth buffer to CPU
     */
    void download_depth_buffer();

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Stats {
        uint32_t triangles_rasterized = 0;
        uint32_t aabbs_tested = 0;
        uint32_t aabbs_culled = 0;
        float rasterize_time_ms = 0.0f;
        float test_time_ms = 0.0f;
    };
    const Stats& get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats{}; }

private:
    // Configuration
    SoftwareRasterizerConfig config_;
    bool initialized_ = false;

    // Current frame state
    math::Mat4 view_proj_;
    uint32_t viewport_width_ = 1920;
    uint32_t viewport_height_ = 1080;
    float depth_scale_x_ = 1.0f;
    float depth_scale_y_ = 1.0f;

    // CPU depth buffer
    std::vector<float> depth_buffer_;
    std::vector<std::vector<float>> hierarchical_mips_;

    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;
    uint32_t queue_family_ = 0;

    // GPU buffers
    GPUBuffer gpu_depth_buffer_;
    GPUBuffer triangle_buffer_;
    GPUBuffer atomic_buffer_;

    // Compute pipeline for GPU rasterization
    VkPipeline raster_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout raster_pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout raster_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool raster_desc_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet raster_desc_set_ = VK_NULL_HANDLE;

    uint32_t triangle_count_ = 0;
    mutable Stats stats_;  // Mutable for statistics tracking in const methods

    // Internal helpers
    void create_depth_buffer();
    void create_hierarchical_mips();
    void update_hierarchical_mips();
    void create_gpu_resources();
    void create_compute_pipeline();

    // SIMD-optimized rasterization
    void rasterize_triangle_simd(
        const math::Vec4& v0,
        const math::Vec4& v1,
        const math::Vec4& v2
    );

    // Scalar fallback
    void rasterize_triangle_scalar(
        const math::Vec4& v0,
        const math::Vec4& v1,
        const math::Vec4& v2
    );

    // Transform vertex to screen space
    math::Vec4 project_vertex(const math::Vec3& world_pos) const;

    // Edge function for triangle rasterization
    static float edge_function(const math::Vec2& a, const math::Vec2& b, const math::Vec2& c);

    // Sample depth at screen position
    float sample_depth(float x, float y, uint32_t mip_level = 0) const;
};

} // namespace culling
} // namespace manim
