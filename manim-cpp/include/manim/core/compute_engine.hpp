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
#include <array>
#include <glm/glm.hpp>

namespace manim {

namespace math {
class Mat4Wrapper;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = Mat4Wrapper;
} // namespace math

/**
 * @brief Push constants for Bezier tessellation shader
 */
struct BezierTessellationPushConstants {
    uint32_t num_curves;
    uint32_t segments_per_curve;
    uint32_t compute_normals;  // bool as uint
    uint32_t compute_tangents; // bool as uint
};

/**
 * @brief Push constants for float-to-sortable conversion shader
 */
struct FloatConvertPushConstants {
    uint32_t num_elements;
    uint32_t direction;  // 0 = float->sortable, 1 = sortable->float
};

/**
 * @brief Push constants for radix sort shaders
 */
struct RadixSortPushConstants {
    uint32_t num_elements;
    uint32_t bit_shift;       // Which 4-bit digit (0, 4, 8, ... 28)
    uint32_t num_workgroups;
    uint32_t padding;
};

/**
 * @brief Push constants for prefix sum shader
 */
struct PrefixSumPushConstants {
    uint32_t num_elements;
    uint32_t stride;
};

/**
 * @brief Push constants for matrix multiply shader
 */
struct MatrixMultiplyPushConstants {
    uint32_t num_matrices;
    uint32_t operation;  // 0 = copy, 1 = self multiply
};

/**
 * @brief Compute shader pipeline
 */
class ComputePipeline {
public:
    ComputePipeline() = default;
    ~ComputePipeline() = default;

    void create(VkDevice device, const std::string& shader_path);
    void create_from_spirv(VkDevice device, const std::vector<uint32_t>& spirv,
                           VkDescriptorSetLayout descriptor_layout,
                           const std::vector<VkPushConstantRange>& push_constants);
    void destroy();

    VkPipeline get_pipeline() const { return pipeline_; }
    VkPipelineLayout get_layout() const { return layout_; }
    VkDescriptorSetLayout get_descriptor_layout() const { return descriptor_layout_; }
    bool is_valid() const { return pipeline_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
    VkShaderModule shader_module_ = VK_NULL_HANDLE;
    bool owns_descriptor_layout_ = false;
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

    std::vector<glm::vec3> normalize_vectors(const std::vector<glm::vec3>& vectors) {
        return vectors;
    }

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

    /**
     * @brief GPU Bezier tessellation with real compute shader
     *
     * @param control_points Control points buffer (4 vec4 per curve)
     * @param vertices Output tessellated vertices
     * @param num_curves Number of curves
     * @param segments_per_curve Segments per curve
     * @param compute_normals Whether to compute normals
     * @param compute_tangents Whether to compute tangents
     * @param normals Output normals buffer (optional)
     * @param tangents Output tangents buffer (optional)
     */
    void tessellate_bezier_gpu(
        const GPUBuffer& control_points,
        GPUBuffer& vertices,
        uint32_t num_curves,
        uint32_t segments_per_curve,
        bool compute_normals = false,
        bool compute_tangents = false,
        GPUBuffer* normals = nullptr,
        GPUBuffer* tangents = nullptr
    );

    /**
     * @brief Check if GPU compute is available
     */
    bool is_gpu_compute_available() const { return bezier_pipeline_.is_valid(); }

    /**
     * @brief CPU fallback tessellation for Bezier curves
     */
    std::vector<math::Vec3> tessellate_bezier_cpu(
        const std::vector<std::array<math::Vec3, 4>>& curves,
        uint32_t segments_per_curve
    );

    // ========================================================================
    // GPU Radix Sort Operations
    // ========================================================================

    /**
     * @brief GPU radix sort for float arrays
     *
     * Uses 4-bit radix sort with 8 passes for 32-bit floats.
     * Handles negative numbers via float-to-sortable conversion.
     *
     * @param buffer GPU buffer containing floats to sort
     * @param num_elements Number of elements in the buffer
     */
    void radix_sort_gpu(GPUBuffer& buffer, uint32_t num_elements);

    /**
     * @brief Check if radix sort GPU pipeline is available
     */
    bool is_radix_sort_available() const { return float_convert_pipeline_.is_valid(); }

    // ========================================================================
    // GPU Matrix Operations
    // ========================================================================

    /**
     * @brief Batch matrix multiply on GPU
     *
     * @param matrices Input matrix buffer
     * @param results Output result buffer (can be same as matrices for in-place)
     * @param num_matrices Number of mat4 matrices
     * @param operation 0 = copy/identity, 1 = self multiply (A*A)
     */
    void dispatch_matrix_multiply(GPUBuffer& matrices, GPUBuffer& results,
                                  uint32_t num_matrices, uint32_t operation = 0);

    /**
     * @brief Check if matrix multiply GPU pipeline is available
     */
    bool is_matrix_multiply_available() const { return matrix_multiply_pipeline_.is_valid(); }

    // Convenience CPU fallbacks
    std::vector<glm::vec3> tessellate_beziers_batch(const std::vector<std::array<glm::vec3, 4>>& /*curves*/,
                                                    uint32_t /*samples_per_curve*/) {
        return {};
    }
    std::vector<glm::vec3> tessellate_beziers_batch(const std::vector<std::vector<glm::vec3>>& /*curves*/,
                                                    uint32_t /*samples_per_curve*/) {
        return {};
    }

    std::vector<glm::vec3> transform_points_gpu(const std::vector<glm::vec3>& points,
                                                const glm::mat4& /*transform*/) {
        return points;
    }

    std::vector<glm::vec3> transform_points_cpu(const std::vector<glm::vec3>& points,
                                                const glm::mat4& /*transform*/) {
        return points;
    }

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

    // Bezier tessellation pipeline
    ComputePipeline bezier_pipeline_;
    VkDescriptorSetLayout bezier_descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet bezier_descriptor_set_ = VK_NULL_HANDLE;

    // Radix sort pipelines
    ComputePipeline float_convert_pipeline_;
    ComputePipeline radix_histogram_pipeline_;
    ComputePipeline prefix_sum_pipeline_;
    ComputePipeline radix_scatter_pipeline_;
    VkDescriptorSetLayout radix_descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet radix_descriptor_sets_[4] = {};  // One per pipeline

    // Matrix multiply pipeline
    ComputePipeline matrix_multiply_pipeline_;
    VkDescriptorSetLayout matrix_multiply_descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet matrix_multiply_descriptor_set_ = VK_NULL_HANDLE;

    // Path to shader directory (set during initialization)
    std::string shader_dir_;

    // Helper functions
    void create_command_pool();
    void load_built_in_shaders();
    void create_bezier_tessellation_pipeline();
    void create_radix_sort_pipelines();
    void create_matrix_multiply_pipeline();
    void create_descriptor_pool();
    std::vector<uint32_t> load_spirv_file(const std::string& path);
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

// ========================================================================
// Global ComputeEngine Accessor (for GPUUtils)
// ========================================================================

/**
 * @brief Get the global ComputeEngine instance
 * @throws std::runtime_error if ComputeEngine not initialized
 */
ComputeEngine& getGlobalComputeEngine();

/**
 * @brief Initialize the global ComputeEngine instance
 * @param device Vulkan device
 * @param queue Compute queue
 * @param pool Memory pool
 */
void initializeGlobalComputeEngine(VkDevice device, VkQueue queue, MemoryPool& pool);

/**
 * @brief Shutdown and destroy the global ComputeEngine instance
 */
void shutdownGlobalComputeEngine();

/**
 * @brief Check if the global ComputeEngine is initialized and ready
 * @return true if initialized with valid GPU compute capability
 */
bool isComputeEngineInitialized();

} // namespace manim
