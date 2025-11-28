#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace manim {

// Forward declarations
class Mobject;

namespace culling {

/**
 * @brief GPU-accelerated Linear BVH (LBVH) for spatial indexing
 *
 * Uses Morton codes for spatial hashing and Karras algorithm for
 * parallel tree construction. Supports efficient updates via
 * refit operations for animated scenes.
 *
 * Construction pipeline:
 * 1. Compute scene bounds (CPU)
 * 2. Compute Morton codes (GPU)
 * 3. Radix sort primitives by Morton code (GPU)
 * 4. Build internal nodes using Karras algorithm (GPU)
 * 5. Compute node bounds bottom-up (GPU)
 */
class SpatialIndex {
public:
    /**
     * @brief Statistics about the BVH
     */
    struct Stats {
        uint32_t num_objects = 0;
        uint32_t num_internal_nodes = 0;
        uint32_t num_leaf_nodes = 0;
        uint32_t tree_depth = 0;
        float build_time_ms = 0.0f;
        float refit_time_ms = 0.0f;
        BVHUpdateStrategy last_update = BVHUpdateStrategy::None;
    };

    SpatialIndex();
    ~SpatialIndex();

    // Non-copyable, moveable
    SpatialIndex(const SpatialIndex&) = delete;
    SpatialIndex& operator=(const SpatialIndex&) = delete;
    SpatialIndex(SpatialIndex&&) noexcept;
    SpatialIndex& operator=(SpatialIndex&&) noexcept;

    /**
     * @brief Initialize GPU resources
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t compute_queue_family
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
    // BVH Construction
    // ========================================================================

    /**
     * @brief Build BVH from object bounds
     * @param bounds Object bounds array
     * @param cmd_buffer Optional command buffer for async execution
     */
    void build(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Build BVH from mobjects
     */
    void build(const std::vector<std::shared_ptr<Mobject>>& mobjects, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Update BVH with new object positions (determines strategy automatically)
     * @param bounds Updated object bounds
     * @param cmd_buffer Optional command buffer
     * @return Strategy used for update
     */
    BVHUpdateStrategy update(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Refit BVH (update AABBs only, keep tree structure)
     *
     * Efficient for small movements - O(n) instead of O(n log n)
     */
    void refit(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Force full rebuild
     */
    void rebuild(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    // ========================================================================
    // Queries
    // ========================================================================

    /**
     * @brief Get BVH node buffer (for GPU traversal)
     */
    const GPUBuffer& get_node_buffer() const { return bvh_nodes_buffer_; }

    /**
     * @brief Get sorted primitive indices buffer
     */
    const GPUBuffer& get_primitive_indices_buffer() const { return sorted_indices_buffer_; }

    /**
     * @brief Get number of objects
     */
    uint32_t get_object_count() const { return object_count_; }

    /**
     * @brief Get statistics
     */
    const Stats& get_stats() const { return stats_; }

    /**
     * @brief Get scene bounds
     */
    const math::Vec3& get_scene_min() const { return scene_min_; }
    const math::Vec3& get_scene_max() const { return scene_max_; }

    // ========================================================================
    // CPU Operations (for debugging/testing)
    // ========================================================================

    /**
     * @brief Build BVH on CPU (for testing)
     */
    void build_cpu(const std::vector<ObjectBounds>& bounds);

    /**
     * @brief Download BVH nodes from GPU
     */
    std::vector<BVHNode> download_nodes();

    /**
     * @brief Compute Morton code for a point (CPU)
     */
    static uint32_t compute_morton_code(const math::Vec3& normalized_pos);

    // ========================================================================
    // GPU Build Configuration
    // ========================================================================

    /**
     * @brief Enable/disable GPU BVH construction
     */
    void set_use_gpu_build(bool use_gpu) { use_gpu_build_ = use_gpu; }
    bool get_use_gpu_build() const { return use_gpu_build_; }

    /**
     * @brief Check if GPU BVH construction is available
     */
    bool is_gpu_build_available() const;

private:
    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;

    // Pipelines
    VkPipeline morton_pipeline_ = VK_NULL_HANDLE;
    VkPipeline radix_sort_pipeline_ = VK_NULL_HANDLE;
    VkPipeline build_tree_pipeline_ = VK_NULL_HANDLE;
    VkPipeline refit_pipeline_ = VK_NULL_HANDLE;

    VkPipelineLayout morton_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout sort_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout build_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout refit_layout_ = VK_NULL_HANDLE;

    // GPU BVH construction pipelines (Karras algorithm)
    VkPipeline karras_pipeline_ = VK_NULL_HANDLE;
    VkPipeline generate_leaves_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout karras_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout generate_leaves_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout karras_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout generate_leaves_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet karras_desc_set_ = VK_NULL_HANDLE;
    VkDescriptorSet generate_leaves_desc_set_ = VK_NULL_HANDLE;

    // Descriptor sets
    VkDescriptorSetLayout morton_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout sort_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout build_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout refit_desc_layout_ = VK_NULL_HANDLE;

    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet morton_desc_set_ = VK_NULL_HANDLE;
    VkDescriptorSet sort_desc_set_ = VK_NULL_HANDLE;
    VkDescriptorSet build_desc_set_ = VK_NULL_HANDLE;
    VkDescriptorSet refit_desc_set_ = VK_NULL_HANDLE;

    // Command buffer resources
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;

    // GPU buffers
    GPUBuffer object_bounds_buffer_;     // Input bounds
    GPUBuffer morton_codes_buffer_;      // Morton code + index pairs
    GPUBuffer sorted_codes_buffer_;      // Sorted Morton codes (double buffer)
    GPUBuffer sorted_indices_buffer_;    // Sorted primitive indices
    GPUBuffer bvh_nodes_buffer_;         // BVH nodes
    GPUBuffer atomic_counters_buffer_;   // For parallel refit
    GPUBuffer parent_indices_buffer_;    // Parent indices for refit propagation

    // Radix sort scratch buffers
    GPUBuffer sort_scratch_buffer_;
    GPUBuffer histogram_buffer_;

    // State
    bool initialized_ = false;
    bool use_gpu_build_ = true;      // Use GPU for BVH construction
    uint32_t object_count_ = 0;
    uint32_t max_objects_ = 0;
    math::Vec3 scene_min_{0.0f};
    math::Vec3 scene_max_{0.0f};
    Stats stats_{};

    // Previous frame data (for update strategy decision)
    std::vector<ObjectBounds> prev_bounds_;
    float rebuild_threshold_ = 0.5f;  // Rebuild if max displacement > 50% of object size

    // Pipeline creation
    void create_morton_pipeline();
    void create_radix_sort_pipeline();
    void create_build_tree_pipeline();
    void create_refit_pipeline();
    void create_karras_pipeline();
    void create_generate_leaves_pipeline();
    void create_descriptor_resources();
    void allocate_buffers(uint32_t max_objects);
    void update_descriptor_sets();

    // Build phases
    void compute_scene_bounds(const std::vector<ObjectBounds>& bounds);
    void compute_morton_codes(VkCommandBuffer cmd);
    void radix_sort(VkCommandBuffer cmd);
    void build_tree(VkCommandBuffer cmd);
    void compute_node_bounds(VkCommandBuffer cmd);

    // GPU build phases (Karras algorithm)
    void upload_object_bounds(const std::vector<ObjectBounds>& bounds);
    void compute_morton_codes_gpu(VkCommandBuffer cmd);
    void sort_morton_codes_cpu();  // CPU fallback for Morton pair sorting
    void build_tree_gpu(VkCommandBuffer cmd);
    void generate_leaves_gpu(VkCommandBuffer cmd);
    void compute_node_bounds_gpu(VkCommandBuffer cmd);
    void build_gpu(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer);

    // Command helpers
    void begin_commands();
    void end_commands();
    void submit_and_wait();

    // Update strategy
    BVHUpdateStrategy determine_update_strategy(const std::vector<ObjectBounds>& bounds) const;
};

} // namespace culling
} // namespace manim
