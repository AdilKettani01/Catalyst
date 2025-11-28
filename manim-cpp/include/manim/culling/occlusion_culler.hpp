#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace manim {
namespace culling {

/**
 * @brief GPU-accelerated occlusion culling using Hierarchical Z-buffer (Hi-Z)
 *
 * Hi-Z occlusion culling works by:
 * 1. Rendering occluders to depth buffer (usually previous frame's depth)
 * 2. Building a mipmap pyramid where each level stores MAX depth of 4 texels
 * 3. For each object, projecting its AABB to screen and testing against Hi-Z
 *
 * This class manages:
 * - Hi-Z pyramid construction (compute shader mipmap generation)
 * - Occlusion test compute shader
 * - Previous frame depth buffer management
 */
class OcclusionCuller {
public:
    /**
     * @brief Hi-Z pyramid configuration
     */
    struct HiZConfig {
        uint32_t max_mip_levels = 10;          // Max pyramid levels (2^10 = 1024)
        bool use_previous_frame_depth = true;  // Use reprojected previous frame
        float depth_bias = 0.001f;             // Bias to prevent self-occlusion
        float depth_threshold = 0.05f;         // Threshold for temporal disocclusion detection
        bool use_temporal_reprojection = true; // Enable temporal reprojection for moving cameras
    };

    /**
     * @brief Statistics about occlusion culling
     */
    struct Stats {
        uint32_t objects_tested = 0;
        uint32_t objects_occluded = 0;
        float hiz_build_time_ms = 0.0f;
        float occlusion_test_time_ms = 0.0f;
        uint32_t hiz_mip_levels = 0;
    };

    OcclusionCuller();
    ~OcclusionCuller();

    // Non-copyable, moveable
    OcclusionCuller(const OcclusionCuller&) = delete;
    OcclusionCuller& operator=(const OcclusionCuller&) = delete;
    OcclusionCuller(OcclusionCuller&&) noexcept;
    OcclusionCuller& operator=(OcclusionCuller&&) noexcept;

    /**
     * @brief Initialize GPU resources
     * @param device Vulkan device
     * @param physical_device Physical device for memory properties
     * @param memory_pool Memory pool for buffer allocation
     * @param compute_queue_family Queue family index for compute operations
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t compute_queue_family
    );

    /**
     * @brief Cleanup all resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Set configuration
     */
    void set_config(const HiZConfig& config) { config_ = config; }
    const HiZConfig& get_config() const { return config_; }

    // ========================================================================
    // Hi-Z Pyramid Construction
    // ========================================================================

    /**
     * @brief Build Hi-Z pyramid from depth buffer
     * @param depth_image Source depth buffer (VK_FORMAT_D32_SFLOAT expected)
     * @param depth_image_view Image view for sampling
     * @param width Depth buffer width
     * @param height Depth buffer height
     * @param cmd_buffer Command buffer for recording (optional)
     */
    void build_hiz_pyramid(
        VkImage depth_image,
        VkImageView depth_image_view,
        uint32_t width,
        uint32_t height,
        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE
    );

    /**
     * @brief Build Hi-Z pyramid with temporal reprojection
     * @param depth_image Current frame depth buffer
     * @param depth_image_view Image view for sampling
     * @param width Depth buffer width
     * @param height Depth buffer height
     * @param curr_view_proj Current frame view-projection matrix
     * @param cmd_buffer Command buffer for recording (optional)
     *
     * This method automatically handles:
     * - Copying depth to Hi-Z base level
     * - Temporal reprojection from previous frame
     * - Mipmap chain construction
     * - Frame history management
     */
    void build_hiz_pyramid_temporal(
        VkImage depth_image,
        VkImageView depth_image_view,
        uint32_t width,
        uint32_t height,
        const math::Mat4& curr_view_proj,
        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE
    );

    /**
     * @brief Advance to next frame (call at end of frame)
     *
     * Swaps current/previous frame depth buffers for temporal reprojection
     */
    void end_frame();

    /**
     * @brief Get Hi-Z pyramid sampler (for external use)
     */
    VkSampler get_hiz_sampler() const { return hiz_sampler_; }

    /**
     * @brief Get Hi-Z pyramid image view (all mip levels)
     */
    VkImageView get_hiz_pyramid_view() const { return hiz_pyramid_view_; }

    /**
     * @brief Get number of mip levels in current pyramid
     */
    uint32_t get_mip_level_count() const { return current_mip_levels_; }

    // ========================================================================
    // Occlusion Testing
    // ========================================================================

    /**
     * @brief Perform occlusion culling on frustum-visible objects
     * @param bounds Object bounds array
     * @param frustum_visible_indices Indices of objects that passed frustum test
     * @param view_proj View-projection matrix for screen projection
     * @param screen_width Screen width in pixels
     * @param screen_height Screen height in pixels
     * @param cmd_buffer Command buffer (optional)
     * @return Indices of objects that passed occlusion test
     */
    std::vector<uint32_t> cull(
        const std::vector<ObjectBounds>& bounds,
        const std::vector<uint32_t>& frustum_visible_indices,
        const math::Mat4& view_proj,
        uint32_t screen_width,
        uint32_t screen_height,
        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE
    );

    /**
     * @brief Perform occlusion culling using GPU buffers directly
     * @param bounds_buffer GPU buffer containing ObjectBounds
     * @param frustum_visible_buffer GPU buffer containing visible indices
     * @param num_frustum_visible Number of frustum-visible objects
     * @param view_proj View-projection matrix
     * @param screen_width Screen width
     * @param screen_height Screen height
     * @param cmd_buffer Command buffer
     */
    void cull_gpu(
        const GPUBuffer& bounds_buffer,
        const GPUBuffer& frustum_visible_buffer,
        uint32_t num_frustum_visible,
        const math::Mat4& view_proj,
        uint32_t screen_width,
        uint32_t screen_height,
        VkCommandBuffer cmd_buffer
    );

    /**
     * @brief Get visibility results buffer
     */
    const GPUBuffer& get_visible_indices_buffer() const { return visible_indices_buffer_; }

    /**
     * @brief Get visible count (call after cull and GPU sync)
     */
    uint32_t get_visible_count();

    /**
     * @brief Download visible indices to CPU
     */
    std::vector<uint32_t> download_visible_indices();

    /**
     * @brief Get statistics
     */
    const Stats& get_stats() const { return stats_; }

    // ========================================================================
    // CPU Fallback (for testing/debugging)
    // ========================================================================

    /**
     * @brief CPU occlusion test (for debugging)
     */
    static std::vector<uint32_t> cull_cpu(
        const std::vector<ObjectBounds>& bounds,
        const std::vector<uint32_t>& frustum_visible_indices,
        const math::Mat4& view_proj,
        const std::vector<float>& depth_buffer,
        uint32_t width,
        uint32_t height
    );

private:
    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;

    // Hi-Z pyramid resources
    VkImage hiz_pyramid_image_ = VK_NULL_HANDLE;
    VkDeviceMemory hiz_pyramid_memory_ = VK_NULL_HANDLE;
    VkImageView hiz_pyramid_view_ = VK_NULL_HANDLE;        // All mip levels
    std::vector<VkImageView> hiz_mip_views_;              // Individual mip views
    VkSampler hiz_sampler_ = VK_NULL_HANDLE;
    uint32_t hiz_width_ = 0;
    uint32_t hiz_height_ = 0;
    uint32_t current_mip_levels_ = 0;

    // Hi-Z build pipeline
    VkPipeline hiz_build_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout hiz_build_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout hiz_build_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool hiz_build_desc_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> hiz_build_desc_sets_;   // One per mip level

    // Depth-to-HiZ copy pipeline (converts D32_SFLOAT to R32_SFLOAT)
    VkPipeline depth_copy_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout depth_copy_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout depth_copy_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet depth_copy_desc_set_ = VK_NULL_HANDLE;

    // Temporal reprojection pipeline
    VkPipeline reproject_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout reproject_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout reproject_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet reproject_desc_set_ = VK_NULL_HANDLE;

    // Temporal depth history (double-buffered)
    VkImage prev_depth_image_ = VK_NULL_HANDLE;
    VkDeviceMemory prev_depth_memory_ = VK_NULL_HANDLE;
    VkImageView prev_depth_view_ = VK_NULL_HANDLE;
    math::Mat4 prev_view_proj_{1.0f};      // Previous frame view-projection matrix
    math::Mat4 prev_view_proj_inv_{1.0f};  // Inverse of previous view-projection
    bool has_previous_frame_ = false;      // True after first frame completes

    // Occlusion test pipeline
    VkPipeline occlusion_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout occlusion_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout occlusion_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool occlusion_desc_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet occlusion_desc_set_ = VK_NULL_HANDLE;

    // Command resources
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;

    // GPU buffers
    GPUBuffer bounds_staging_buffer_;
    GPUBuffer frustum_visible_staging_buffer_;
    GPUBuffer visible_indices_buffer_;
    GPUBuffer atomic_counter_buffer_;

    // State
    bool initialized_ = false;
    uint32_t max_objects_ = 0;
    HiZConfig config_;
    Stats stats_;

    // Pipeline creation
    void create_hiz_build_pipeline();
    void create_occlusion_pipeline();
    void create_depth_copy_pipeline();
    void create_reproject_pipeline();
    void create_hiz_pyramid_resources(uint32_t width, uint32_t height);
    void create_temporal_resources(uint32_t width, uint32_t height);
    void cleanup_hiz_pyramid_resources();
    void cleanup_temporal_resources();
    void allocate_buffers(uint32_t max_objects);
    void update_occlusion_descriptor_set();

    // Depth extraction phases
    void copy_depth_to_hiz(VkCommandBuffer cmd, VkImageView depth_view);
    void reproject_temporal_depth(VkCommandBuffer cmd, VkImageView curr_depth_view);

    // Command helpers
    void begin_commands();
    void end_commands();
    void submit_and_wait();

    // Hi-Z building phases
    void dispatch_hiz_build(VkCommandBuffer cmd, uint32_t src_mip);
    void transition_hiz_for_read(VkCommandBuffer cmd);
    void transition_hiz_for_write(VkCommandBuffer cmd, uint32_t mip_level);
};

} // namespace culling
} // namespace manim
