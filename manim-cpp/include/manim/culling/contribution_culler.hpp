#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace manim {
namespace culling {

/**
 * @brief Configuration for contribution culling
 */
struct ContributionCullingConfig {
    float min_screen_size = 2.0f;        ///< Minimum screen-space size in pixels
    float max_screen_distance = 1000.0f; ///< Max distance for threshold scaling
    bool use_distance_scaling = true;    ///< Adaptive threshold based on distance
    bool use_area_threshold = false;     ///< Use area instead of diameter (future)
};

/**
 * @brief Push constants for contribution culling shader
 */
struct ContributionCullPushConstants {
    math::Mat4 view_proj;
    float screen_width;
    float screen_height;
    float min_screen_size;
    float max_screen_distance;
    uint32_t num_frustum_visible;
    uint32_t flags;
    uint32_t padding[2];
};
static_assert(sizeof(ContributionCullPushConstants) <= 128, "Push constants too large");

/**
 * @brief GPU-accelerated contribution culling
 *
 * Contribution culling removes objects that are too small to meaningfully
 * contribute to the rendered image. This is an optimization that runs
 * before full occlusion testing.
 *
 * Key features:
 * - Screen-space size calculation using bounding sphere projection
 * - Configurable minimum pixel threshold
 * - Optional distance-based adaptive thresholding
 * - CPU fallback for validation
 *
 * Integration:
 * @code
 * ContributionCuller culler;
 * culler.initialize(device, physical_device, memory_pool, queue_family);
 * culler.set_config({.min_screen_size = 2.0f, .use_distance_scaling = true});
 *
 * // In render loop:
 * culler.cull(view_proj, frustum_visible_indices, cmd_buffer);
 * auto visible = culler.get_visible_indices();
 * @endcode
 */
class ContributionCuller {
public:
    ContributionCuller();
    ~ContributionCuller();

    // Non-copyable, moveable
    ContributionCuller(const ContributionCuller&) = delete;
    ContributionCuller& operator=(const ContributionCuller&) = delete;
    ContributionCuller(ContributionCuller&&) noexcept;
    ContributionCuller& operator=(ContributionCuller&&) noexcept;

    /**
     * @brief Initialize the culler
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t queue_family
    );

    /**
     * @brief Cleanup GPU resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Set configuration
     */
    void set_config(const ContributionCullingConfig& config) { config_ = config; }
    const ContributionCullingConfig& get_config() const { return config_; }

    /**
     * @brief Set viewport size (call on resize)
     */
    void set_viewport_size(uint32_t width, uint32_t height);

    /**
     * @brief Update object bounds buffer
     *
     * Must be called before culling if objects have changed.
     *
     * @param bounds Vector of object bounds
     */
    void update_bounds(const std::vector<ObjectBounds>& bounds);

    /**
     * @brief Set bounds buffer directly (for GPU-resident data)
     */
    void set_bounds_buffer(const GPUBuffer& buffer, uint32_t object_count);

    /**
     * @brief Execute contribution culling on GPU
     *
     * @param view_proj View-projection matrix
     * @param frustum_visible_count Number of frustum-visible objects
     * @param cmd_buffer Command buffer (if null, creates internal)
     */
    void cull(
        const math::Mat4& view_proj,
        uint32_t frustum_visible_count,
        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE
    );

    /**
     * @brief Get visible indices (downloads from GPU)
     */
    std::vector<uint32_t> get_visible_indices();

    /**
     * @brief Get visible count (requires GPU readback)
     */
    uint32_t get_visible_count();

    /**
     * @brief Get output visible buffer (for chaining to next stage)
     */
    const GPUBuffer& get_visible_buffer() const { return visible_indices_buffer_; }

    /**
     * @brief Get frustum visible buffer (for binding input)
     */
    const GPUBuffer& get_frustum_visible_buffer() const { return frustum_visible_buffer_; }

    /**
     * @brief CPU fallback for validation/debugging
     */
    std::vector<uint32_t> cull_cpu(
        const math::Mat4& view_proj,
        const std::vector<ObjectBounds>& bounds,
        const std::vector<uint32_t>& frustum_visible
    );

    // Statistics
    struct Stats {
        uint32_t input_count = 0;
        uint32_t output_count = 0;
        float cull_time_ms = 0.0f;
    };
    const Stats& get_stats() const { return stats_; }

private:
    // Configuration
    ContributionCullingConfig config_;
    bool initialized_ = false;
    uint32_t viewport_width_ = 1920;
    uint32_t viewport_height_ = 1080;

    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;
    uint32_t queue_family_ = 0;

    // Compute pipeline
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout desc_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool desc_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet desc_set_ = VK_NULL_HANDLE;

    // GPU buffers
    GPUBuffer bounds_buffer_;              // Object bounds (input)
    GPUBuffer frustum_visible_buffer_;     // Frustum visible indices (input)
    GPUBuffer visible_indices_buffer_;     // Output visible indices
    GPUBuffer atomic_counter_buffer_;      // Atomic counter for compaction

    uint32_t object_count_ = 0;
    bool using_external_bounds_ = false;

    // Statistics
    Stats stats_;

    // Internal helpers
    void create_pipeline();
    void create_descriptor_set();
    void allocate_buffers(uint32_t max_objects);
    void update_descriptor_set();
    VkShaderModule load_shader(const char* path);
};

} // namespace culling
} // namespace manim
