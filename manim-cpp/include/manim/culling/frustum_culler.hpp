#pragma once

#include "manim/culling/culling_types.hpp"
#include "manim/core/memory_pool.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace manim {

// Forward declarations
class Scene;
class Mobject;
class ComputeEngine;

namespace culling {

/**
 * @brief GPU-accelerated frustum culling
 *
 * Tests objects against 6 frustum planes using a compute shader.
 * Outputs visibility buffer (bitfield) and compacted visible indices.
 */
class FrustumCuller {
public:
    FrustumCuller();
    ~FrustumCuller();

    // Non-copyable
    FrustumCuller(const FrustumCuller&) = delete;
    FrustumCuller& operator=(const FrustumCuller&) = delete;

    // Moveable
    FrustumCuller(FrustumCuller&&) noexcept;
    FrustumCuller& operator=(FrustumCuller&&) noexcept;

    /**
     * @brief Initialize Vulkan resources
     * @param device Vulkan logical device
     * @param physical_device Vulkan physical device
     * @param memory_pool Memory pool for buffer allocation
     * @param compute_queue_family Compute queue family index
     */
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        uint32_t compute_queue_family
    );

    /**
     * @brief Cleanup Vulkan resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    // ========================================================================
    // Frustum Plane Extraction
    // ========================================================================

    /**
     * @brief Extract frustum planes from view-projection matrix
     *
     * Uses Gribb-Hartmann method to extract 6 planes from the combined
     * view-projection matrix. Planes are normalized.
     *
     * @param view_proj Combined view-projection matrix
     * @return FrustumPlanes struct with 6 normalized planes
     */
    static FrustumPlanes extract_frustum_planes(const math::Mat4& view_proj);

    // ========================================================================
    // Object Bounds Management
    // ========================================================================

    /**
     * @brief Upload object bounds to GPU
     * @param bounds Vector of object bounds
     */
    void upload_object_bounds(const std::vector<ObjectBounds>& bounds);

    /**
     * @brief Upload object bounds from scene mobjects
     * @param mobjects Vector of mobjects to extract bounds from
     */
    void upload_object_bounds_from_mobjects(
        const std::vector<std::shared_ptr<Mobject>>& mobjects
    );

    /**
     * @brief Get current object count
     */
    uint32_t get_object_count() const { return object_count_; }

    // ========================================================================
    // Culling Operations
    // ========================================================================

    /**
     * @brief Perform GPU frustum culling
     *
     * Tests all objects against the frustum and populates:
     * - Visibility bitfield (1 bit per object)
     * - Compacted list of visible object indices
     *
     * @param frustum Frustum planes
     * @param cmd_buffer Command buffer to record to (or null for immediate)
     */
    void cull(const FrustumPlanes& frustum, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Perform frustum culling with view-projection matrix
     */
    void cull(const math::Mat4& view_proj, VkCommandBuffer cmd_buffer = VK_NULL_HANDLE);

    /**
     * @brief Get visibility buffer (for reading results)
     */
    const GPUBuffer& get_visibility_buffer() const { return visibility_buffer_; }

    /**
     * @brief Get compacted visible indices buffer
     */
    const GPUBuffer& get_visible_indices_buffer() const { return visible_indices_buffer_; }

    /**
     * @brief Get visible count (after readback)
     */
    uint32_t get_visible_count() const { return visible_count_; }

    /**
     * @brief Download visible indices to CPU
     * @return Vector of visible object indices
     */
    std::vector<uint32_t> download_visible_indices();

    /**
     * @brief Check visibility of specific object (after culling)
     * @param object_index Index of object to check
     * @return true if object is visible
     */
    bool is_visible(uint32_t object_index) const;

    // ========================================================================
    // CPU Fallback
    // ========================================================================

    /**
     * @brief CPU-based frustum culling (fallback)
     * @param bounds Object bounds
     * @param frustum Frustum planes
     * @return Vector of visible object indices
     */
    static std::vector<uint32_t> cull_cpu(
        const std::vector<ObjectBounds>& bounds,
        const FrustumPlanes& frustum
    );

    /**
     * @brief Test single AABB against frustum (CPU)
     */
    static bool test_aabb_frustum(
        const math::Vec3& aabb_min,
        const math::Vec3& aabb_max,
        const FrustumPlanes& frustum
    );

private:
    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;

    // Compute pipeline
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

    // Command buffer resources (for immediate mode)
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;

    // GPU buffers
    GPUBuffer object_bounds_buffer_;     // Input: ObjectBounds[]
    GPUBuffer frustum_buffer_;           // Input: FrustumPlanes
    GPUBuffer visibility_buffer_;        // Output: bitfield (uint32_t[])
    GPUBuffer visible_indices_buffer_;   // Output: compacted indices
    GPUBuffer counter_buffer_;           // Atomic counter for compaction

    // State
    bool initialized_ = false;
    uint32_t object_count_ = 0;
    uint32_t max_objects_ = 0;
    mutable uint32_t visible_count_ = 0;
    mutable std::vector<uint32_t> visibility_bitfield_;

    // Pipeline creation
    void create_pipeline();
    void create_descriptor_resources();
    void allocate_buffers(uint32_t max_objects);
    void update_descriptor_set();

    // Command recording
    void begin_commands();
    void end_commands();
    void submit_and_wait();
};

} // namespace culling
} // namespace manim
