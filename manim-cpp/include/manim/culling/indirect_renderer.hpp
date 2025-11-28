#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/culling/gpu_culling_pipeline.hpp"
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace manim {

// Forward declarations
class Mobject;
class Scene;

namespace culling {

/**
 * @brief GPU-driven indirect rendering system
 *
 * Implements a fully GPU-driven rendering pipeline where:
 * 1. Culling determines visibility on GPU
 * 2. Draw commands are generated on GPU from visibility results
 * 3. vkCmdDrawIndexedIndirectCount renders all visible objects in one call
 *
 * This eliminates CPU-GPU sync points and enables rendering millions of objects
 * with minimal CPU overhead.
 *
 * Data flow:
 * Objects -> Culling Pipeline -> Visible Indices -> Generate Draws -> Indirect Draw Buffer
 */
class IndirectRenderer {
public:
    /**
     * @brief Statistics about indirect rendering
     */
    struct Stats {
        uint32_t total_objects = 0;
        uint32_t visible_objects = 0;
        uint32_t draw_calls = 0;
        uint32_t total_vertices = 0;
        uint32_t total_indices = 0;
        float generate_draws_time_ms = 0.0f;
        float render_time_ms = 0.0f;
    };

    /**
     * @brief Configuration for indirect rendering
     */
    struct Config {
        uint32_t max_objects = 100000;          // Maximum objects in scene
        uint32_t max_draw_calls = 100000;       // Maximum indirect draw calls
        uint32_t max_vertices = 10000000;       // Maximum vertices in merged buffer
        uint32_t max_indices = 30000000;        // Maximum indices in merged buffer
        bool enable_draw_compaction = true;     // Compact draw calls by material
        bool enable_lod = false;                // Enable LOD selection (future)
    };

    IndirectRenderer();
    ~IndirectRenderer();

    // Non-copyable, moveable
    IndirectRenderer(const IndirectRenderer&) = delete;
    IndirectRenderer& operator=(const IndirectRenderer&) = delete;
    IndirectRenderer(IndirectRenderer&&) noexcept;
    IndirectRenderer& operator=(IndirectRenderer&&) noexcept;

    /**
     * @brief Initialize the indirect renderer with default config
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t graphics_queue_family,
        uint32_t compute_queue_family
    ) {
        Config default_config;
        initialize(device, physical_device, memory_pool, graphics_queue_family, compute_queue_family, default_config);
    }

    /**
     * @brief Initialize the indirect renderer with custom config
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t graphics_queue_family,
        uint32_t compute_queue_family,
        const Config& config
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
     * @brief Get/set configuration
     */
    void set_config(const Config& config) { config_ = config; }
    const Config& get_config() const { return config_; }

    // ========================================================================
    // Scene Management
    // ========================================================================

    /**
     * @brief Begin a new frame
     *
     * Resets internal state for new frame. Call before submitting objects.
     */
    void begin_frame();

    /**
     * @brief Submit objects for rendering
     *
     * Uploads geometry and draw info to GPU buffers.
     * Should be called after object transforms are finalized.
     *
     * @param mobjects Vector of mobjects to render
     */
    void submit_objects(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Update existing objects (positions changed)
     *
     * Only updates bounds and transforms, not geometry.
     */
    void update_object_transforms(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    // ========================================================================
    // Rendering
    // ========================================================================

    /**
     * @brief Execute culling and generate draw commands
     *
     * @param view_proj View-projection matrix for culling
     * @param cmd_buffer Command buffer for compute operations
     */
    void cull_and_generate_draws(
        const math::Mat4& view_proj,
        VkCommandBuffer cmd_buffer
    );

    /**
     * @brief Record indirect draw commands
     *
     * Records vkCmdDrawIndexedIndirectCount into command buffer.
     * Must be called after cull_and_generate_draws.
     *
     * @param cmd_buffer Graphics command buffer
     */
    void record_draws(VkCommandBuffer cmd_buffer);

    /**
     * @brief Bind vertex and index buffers for indirect rendering
     *
     * Call this before record_draws() to bind the merged geometry buffers.
     *
     * @param cmd_buffer Graphics command buffer
     */
    void bind_geometry_buffers(VkCommandBuffer cmd_buffer);

    /**
     * @brief Check if indirect count extension is supported
     *
     * Returns true if VK_KHR_draw_indirect_count or Vulkan 1.2+ is available.
     */
    bool supports_indirect_count() const { return supports_indirect_count_; }

    /**
     * @brief Set whether to use indirect count (for testing/fallback)
     */
    void set_use_indirect_count(bool use) { use_indirect_count_ = use; }

    /**
     * @brief Get number of draw calls (after culling)
     */
    uint32_t get_draw_count() const;

    /**
     * @brief Get statistics
     */
    const Stats& get_stats() const { return stats_; }

    // ========================================================================
    // Buffer Access (for shader binding)
    // ========================================================================

    /**
     * @brief Get merged vertex buffer
     */
    const GPUBuffer& get_vertex_buffer() const { return vertex_buffer_; }

    /**
     * @brief Get merged index buffer
     */
    const GPUBuffer& get_index_buffer() const { return index_buffer_; }

    /**
     * @brief Get indirect draw command buffer
     */
    const GPUBuffer& get_draw_command_buffer() const { return draw_command_buffer_; }

    /**
     * @brief Get object data buffer (transforms, materials for per-instance data)
     */
    const GPUBuffer& get_object_data_buffer() const { return object_data_buffer_; }

    /**
     * @brief Get draw count buffer
     */
    const GPUBuffer& get_draw_count_buffer() const { return draw_count_buffer_; }

    /**
     * @brief Get culling pipeline (for advanced usage)
     */
    GPUCullingPipeline& get_culling_pipeline() { return *culling_pipeline_; }

private:
    // Configuration
    Config config_;
    bool initialized_ = false;

    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;
    uint32_t graphics_queue_family_ = 0;
    uint32_t compute_queue_family_ = 0;

    // Culling pipeline
    std::unique_ptr<GPUCullingPipeline> culling_pipeline_;

    // Generate draws pipeline
    VkPipeline generate_draws_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout generate_draws_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout generate_draws_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool generate_draws_desc_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet generate_draws_desc_set_ = VK_NULL_HANDLE;

    // Merged geometry buffers
    GPUBuffer vertex_buffer_;           // All object vertices merged
    GPUBuffer index_buffer_;            // All object indices merged

    // Per-object data
    GPUBuffer object_data_buffer_;      // Transforms, materials, etc.
    GPUBuffer object_draw_info_buffer_; // Index ranges for each object

    // Indirect draw resources
    GPUBuffer draw_command_buffer_;     // VkDrawIndexedIndirectCommand array
    GPUBuffer draw_count_buffer_;       // uint32_t draw count

    // State tracking
    uint32_t object_count_ = 0;
    uint32_t vertex_count_ = 0;
    uint32_t index_count_ = 0;
    Stats stats_{};

    // Feature detection
    bool supports_indirect_count_ = false;  // VK_KHR_draw_indirect_count or Vulkan 1.2+
    bool use_indirect_count_ = true;        // Whether to use indirect count when available
    PFN_vkCmdDrawIndexedIndirectCount vkCmdDrawIndexedIndirectCountFn_ = nullptr;

    // Object tracking for updates
    std::vector<ObjectDrawInfo> object_draw_info_;
    std::vector<ObjectBounds> object_bounds_;

    // Pipeline creation
    void create_generate_draws_pipeline();
    void allocate_buffers();
    void update_descriptor_set();
    void detect_indirect_count_support();

    // Geometry management
    void upload_geometry(const std::vector<std::shared_ptr<Mobject>>& mobjects);
    void upload_object_draw_info();
};

} // namespace culling
} // namespace manim
