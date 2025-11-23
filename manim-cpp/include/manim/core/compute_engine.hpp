/**
 * @file compute_engine.hpp
 * @brief GPU compute engine for parallel processing
 *
 * Provides high-level interface for GPU compute operations,
 * replacing NumPy operations with GPU-accelerated equivalents.
 */

#pragma once

#include <manim/core/memory_pool.hpp>
#include <vulkan/vulkan.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace manim {

/**
 * @brief Compute shader pipeline
 */
class ComputePipeline {
public:
    ComputePipeline() = default;
    ~ComputePipeline();

    void create(VkDevice device, const std::string& shader_path);
    void destroy();

    VkPipeline get_pipeline() const { return pipeline_; }
    VkPipelineLayout get_layout() const { return layout_; }
    VkDescriptorSetLayout get_descriptor_layout() const { return descriptor_layout_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
    VkShaderModule shader_module_ = VK_NULL_HANDLE;
};

/**
 * @brief GPU compute engine
 *
 * Features:
 * - Parallel vector/matrix operations
 * - Batch processing for multiple objects
 * - Automatic CPU/GPU selection based on data size
 * - Built-in compute shaders for common operations
 */
class ComputeEngine {
public:
    /**
     * @brief Initialize compute engine
     */
    void initialize(VkDevice device, VkQueue compute_queue, MemoryPool& memory_pool);

    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();

    // ========================================================================
    // Vector Operations (replaces numpy)
    // ========================================================================

    /**
     * @brief Add two vectors: result = a + b
     *
     * @param a First vector buffer
     * @param b Second vector buffer
     * @param result Output buffer
     * @param count Number of elements
     */
    void vector_add(
        const GPUBuffer& a,
        const GPUBuffer& b,
        GPUBuffer& result,
        uint32_t count
    );

    /**
     * @brief Subtract vectors: result = a - b
     */
    void vector_sub(
        const GPUBuffer& a,
        const GPUBuffer& b,
        GPUBuffer& result,
        uint32_t count
    );

    /**
     * @brief Multiply vectors element-wise: result = a * b
     */
    void vector_mul(
        const GPUBuffer& a,
        const GPUBuffer& b,
        GPUBuffer& result,
        uint32_t count
    );

    /**
     * @brief Scale vector: result = a * scalar
     */
    void vector_scale(
        const GPUBuffer& a,
        float scalar,
        GPUBuffer& result,
        uint32_t count
    );

    /**
     * @brief Dot product: result = dot(a, b)
     */
    float vector_dot(
        const GPUBuffer& a,
        const GPUBuffer& b,
        uint32_t count
    );

    /**
     * @brief Cross product for 3D vectors
     */
    void vector_cross(
        const GPUBuffer& a,
        const GPUBuffer& b,
        GPUBuffer& result,
        uint32_t count
    );

    /**
     * @brief Normalize vectors: result = a / ||a||
     */
    void vector_normalize(
        const GPUBuffer& a,
        GPUBuffer& result,
        uint32_t count
    );

    // ========================================================================
    // Matrix Operations
    // ========================================================================

    /**
     * @brief Matrix multiplication: result = a * b
     *
     * @param a Matrix A (m x k)
     * @param b Matrix B (k x n)
     * @param result Output matrix (m x n)
     * @param m Rows in A
     * @param k Columns in A / Rows in B
     * @param n Columns in B
     */
    void matrix_multiply(
        const GPUBuffer& a,
        const GPUBuffer& b,
        GPUBuffer& result,
        uint32_t m,
        uint32_t k,
        uint32_t n
    );

    /**
     * @brief Transform points by matrix: result = matrix * points
     *
     * @param matrix 4x4 transformation matrix
     * @param points Input points (vec3)
     * @param result Output transformed points
     * @param count Number of points
     */
    void transform_points(
        const glm::mat4& matrix,
        const GPUBuffer& points,
        GPUBuffer& result,
        uint32_t count
    );

    /**
     * @brief Batch transform multiple point sets with different matrices
     */
    void batch_transform(
        const GPUBuffer& matrices,  // Array of mat4
        const GPUBuffer& points,    // Array of vec3
        GPUBuffer& result,
        uint32_t num_objects,
        uint32_t points_per_object
    );

    // ========================================================================
    // Bezier Operations (GPU-accelerated)
    // ========================================================================

    /**
     * @brief Evaluate cubic Bezier curves on GPU
     *
     * @param control_points Buffer of vec4 (4 control points per curve)
     * @param t Parameter values (0 to 1)
     * @param result Output positions
     * @param num_curves Number of curves
     * @param samples_per_curve Samples per curve
     */
    void eval_bezier_curves(
        const GPUBuffer& control_points,
        const GPUBuffer& t_values,
        GPUBuffer& result,
        uint32_t num_curves,
        uint32_t samples_per_curve
    );

    /**
     * @brief Tessellate Bezier curves into line segments
     */
    void tessellate_bezier(
        const GPUBuffer& control_points,
        GPUBuffer& vertices,
        GPUBuffer& indices,
        uint32_t num_curves,
        uint32_t segments_per_curve
    );

    // ========================================================================
    // Utility Operations
    // ========================================================================

    /**
     * @brief Fill buffer with value
     */
    template<typename T>
    void fill_buffer(GPUBuffer& buffer, const T& value, uint32_t count);

    /**
     * @brief Copy buffer
     */
    void copy_buffer(
        const GPUBuffer& src,
        GPUBuffer& dst,
        VkDeviceSize size,
        VkDeviceSize src_offset = 0,
        VkDeviceSize dst_offset = 0
    );

    /**
     * @brief Dispatch custom compute shader
     *
     * @param pipeline Compute pipeline to use
     * @param descriptor_set Descriptor set with bound resources
     * @param group_count_x X dimension workgroups
     * @param group_count_y Y dimension workgroups
     * @param group_count_z Z dimension workgroups
     */
    void dispatch(
        const ComputePipeline& pipeline,
        VkDescriptorSet descriptor_set,
        uint32_t group_count_x,
        uint32_t group_count_y = 1,
        uint32_t group_count_z = 1
    );

    /**
     * @brief Submit commands and wait for completion
     */
    void submit_and_wait();

    /**
     * @brief Begin command recording
     */
    void begin_commands();

    /**
     * @brief End command recording
     */
    void end_commands();

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;

    // Command buffer for compute operations
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;

    // Built-in compute pipelines
    std::unordered_map<std::string, ComputePipeline> pipelines_;

    // Descriptor pool for compute operations
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

    // Helper functions
    void create_command_pool();
    void load_built_in_shaders();
    ComputePipeline& get_pipeline(const std::string& name);
};

/**
 * @brief RAII wrapper for compute command recording
 */
class ComputeScope {
public:
    ComputeScope(ComputeEngine& engine) : engine_(engine) {
        engine_.begin_commands();
    }

    ~ComputeScope() {
        engine_.end_commands();
        engine_.submit_and_wait();
    }

private:
    ComputeEngine& engine_;
};

} // namespace manim
