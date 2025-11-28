#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/culling/frustum_culler.hpp"
#include "manim/culling/occlusion_culler.hpp"
#include "manim/culling/contribution_culler.hpp"
#include "manim/culling/software_rasterizer.hpp"
#include "manim/culling/cluster_culler.hpp"
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace manim {

// Forward declarations
class Scene;
class Mobject;

namespace culling {

/**
 * @brief Statistics from culling operations
 */
struct CullingStats {
    uint32_t total_objects = 0;
    uint32_t frustum_visible = 0;
    uint32_t contribution_visible = 0;
    uint32_t occlusion_visible = 0;
    uint32_t final_visible = 0;
    float frustum_cull_time_ms = 0.0f;
    float contribution_cull_time_ms = 0.0f;
    float occlusion_cull_time_ms = 0.0f;
    float software_raster_time_ms = 0.0f;
    float cluster_cull_time_ms = 0.0f;
    float total_time_ms = 0.0f;
};

/**
 * @brief Configuration for GPU culling pipeline
 */
struct CullingConfig {
    bool enable_frustum_culling = true;
    bool enable_contribution_culling = false; ///< Cull tiny objects (small screen size)
    bool enable_occlusion_culling = false;    ///< Hi-Z occlusion culling
    bool enable_software_rasterizer = false;  ///< Software rasterization for small occluders
    bool enable_cluster_culling = false;      ///< Meshlet/cluster culling (Vulkan 1.3+)
    bool enable_bvh_acceleration = false;     ///< BVH spatial acceleration
    bool enable_compaction = true;            ///< Output compacted visible indices
    uint32_t max_objects = 65536;             ///< Maximum objects to cull
    float occlusion_depth_bias = 0.001f;      ///< Bias for occlusion testing

    // Contribution culling settings
    float contribution_min_screen_size = 2.0f;    ///< Min pixel size before culling
    bool contribution_use_distance_scaling = true; ///< Adaptive threshold based on distance

    // Software rasterizer settings
    uint32_t software_raster_width = 128;     ///< Coarse depth buffer width
    uint32_t software_raster_height = 72;     ///< Coarse depth buffer height
    float small_occluder_threshold = 64.0f;   ///< Max screen pixels for software raster
};

/**
 * @brief Unified GPU culling pipeline
 *
 * Orchestrates frustum culling, occlusion culling, and spatial indexing
 * to produce a final visibility buffer for rendering.
 *
 * Usage:
 *   GPUCullingPipeline pipeline;
 *   pipeline.initialize(device, physical_device, memory_pool, queue_family);
 *   pipeline.update_object_bounds(mobjects);
 *   pipeline.execute(view_proj_matrix, cmd_buffer);
 *   auto visible = pipeline.get_visible_indices();
 */
class GPUCullingPipeline {
public:
    GPUCullingPipeline();
    ~GPUCullingPipeline();

    // Non-copyable, moveable
    GPUCullingPipeline(const GPUCullingPipeline&) = delete;
    GPUCullingPipeline& operator=(const GPUCullingPipeline&) = delete;
    GPUCullingPipeline(GPUCullingPipeline&&) noexcept;
    GPUCullingPipeline& operator=(GPUCullingPipeline&&) noexcept;

    /**
     * @brief Initialize the culling pipeline
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t compute_queue_family,
        const CullingConfig& config = {}
    );

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Update configuration
     */
    void set_config(const CullingConfig& config);

    /**
     * @brief Get current configuration
     */
    const CullingConfig& get_config() const { return config_; }

    /**
     * @brief Enable/disable frustum culling
     */
    void enable_frustum_culling(bool enable) { config_.enable_frustum_culling = enable; }

    /**
     * @brief Enable/disable contribution culling (tiny objects)
     */
    void enable_contribution_culling(bool enable) { config_.enable_contribution_culling = enable; }

    /**
     * @brief Enable/disable occlusion culling
     */
    void enable_occlusion_culling(bool enable) { config_.enable_occlusion_culling = enable; }

    /**
     * @brief Enable/disable software rasterizer for small occluders
     */
    void enable_software_rasterizer(bool enable) { config_.enable_software_rasterizer = enable; }

    /**
     * @brief Enable/disable cluster culling for mesh shading
     */
    void enable_cluster_culling(bool enable) { config_.enable_cluster_culling = enable; }

    // ========================================================================
    // Hi-Z Occlusion Setup
    // ========================================================================

    /**
     * @brief Update Hi-Z pyramid from depth buffer
     *
     * Call this after rendering occluders to update the Hi-Z pyramid
     * for the next frame's occlusion culling.
     *
     * @param depth_image Depth buffer image
     * @param depth_image_view Depth buffer image view
     * @param width Depth buffer width
     * @param height Depth buffer height
     * @param cmd_buffer Command buffer for recording
     */
    void update_hiz_pyramid(
        VkImage depth_image,
        VkImageView depth_image_view,
        uint32_t width,
        uint32_t height,
        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE
    );

    /**
     * @brief Set screen dimensions for occlusion culling
     */
    void set_screen_dimensions(uint32_t width, uint32_t height) {
        screen_width_ = width;
        screen_height_ = height;
    }

    /**
     * @brief Get occlusion culler (for advanced usage)
     */
    OcclusionCuller* get_occlusion_culler() { return occlusion_culler_.get(); }

    /**
     * @brief Get contribution culler (for advanced usage)
     */
    ContributionCuller* get_contribution_culler() { return contribution_culler_.get(); }

    /**
     * @brief Get software rasterizer (for advanced usage)
     */
    SoftwareRasterizer* get_software_rasterizer() { return software_rasterizer_.get(); }

    /**
     * @brief Get cluster culler (for advanced usage)
     */
    ClusterCuller* get_cluster_culler() { return cluster_culler_.get(); }

    // ========================================================================
    // Object Management
    // ========================================================================

    /**
     * @brief Update object bounds from mobjects
     * @param mobjects Vector of mobjects to extract bounds from
     */
    void update_object_bounds(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Update object bounds directly
     * @param bounds Pre-computed object bounds
     */
    void update_object_bounds(const std::vector<ObjectBounds>& bounds);

    /**
     * @brief Get number of objects
     */
    uint32_t get_object_count() const { return object_count_; }

    // ========================================================================
    // Culling Execution
    // ========================================================================

    /**
     * @brief Execute full culling pipeline
     *
     * Runs frustum culling (and optionally occlusion culling) to produce
     * a final visibility buffer.
     *
     * @param view_proj Combined view-projection matrix
     * @param cmd_buffer Optional command buffer to record to
     */
    void execute(const math::Mat4& view_proj, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Execute frustum culling only
     */
    void execute_frustum_culling(const math::Mat4& view_proj, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Execute with pre-computed frustum planes
     */
    void execute(const FrustumPlanes& frustum, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    // ========================================================================
    // Results
    // ========================================================================

    /**
     * @brief Get visible object indices (after execute)
     */
    std::vector<uint32_t> get_visible_indices();

    /**
     * @brief Get visible count
     */
    uint32_t get_visible_count() const;

    /**
     * @brief Check if specific object is visible
     */
    bool is_visible(uint32_t object_index) const;

    /**
     * @brief Get visibility buffer (for GPU-driven rendering)
     */
    const GPUBuffer& get_visibility_buffer() const;

    /**
     * @brief Get visible indices buffer (for GPU-driven rendering)
     */
    const GPUBuffer& get_visible_indices_buffer() const;

    /**
     * @brief Get culling statistics
     */
    const CullingStats& get_stats() const { return stats_; }

    // ========================================================================
    // Frustum Access
    // ========================================================================

    /**
     * @brief Get current frustum planes
     */
    const FrustumPlanes& get_frustum() const { return current_frustum_; }

    /**
     * @brief Extract frustum planes from matrix
     */
    static FrustumPlanes extract_frustum_planes(const math::Mat4& view_proj);

private:
    // Configuration
    CullingConfig config_;
    bool initialized_ = false;

    // Components
    std::unique_ptr<FrustumCuller> frustum_culler_;
    std::unique_ptr<ContributionCuller> contribution_culler_;
    std::unique_ptr<OcclusionCuller> occlusion_culler_;
    std::unique_ptr<SoftwareRasterizer> software_rasterizer_;
    std::unique_ptr<ClusterCuller> cluster_culler_;
    // SpatialIndex will be added in Phase 4

    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;
    uint32_t compute_queue_family_ = 0;

    // State
    uint32_t object_count_ = 0;
    uint32_t screen_width_ = 1920;
    uint32_t screen_height_ = 1080;
    FrustumPlanes current_frustum_{};
    math::Mat4 current_view_proj_{1.0f};
    CullingStats stats_{};

    // Cached object bounds (for CPU fallback and BVH)
    std::vector<ObjectBounds> cached_bounds_;
    std::vector<uint32_t> frustum_visible_cache_;
};

} // namespace culling
} // namespace manim
