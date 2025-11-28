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
 * @brief Meshlet/cluster data for mesh shading
 *
 * A meshlet is a small cluster of triangles (typically 64-128) that forms
 * the basic unit for mesh shader processing and culling.
 */
struct alignas(16) Meshlet {
    // Vertex and index offsets into global buffers
    uint32_t vertex_offset;     ///< Offset into meshlet vertex buffer
    uint32_t vertex_count;      ///< Number of vertices in this meshlet
    uint32_t index_offset;      ///< Offset into meshlet index buffer
    uint32_t triangle_count;    ///< Number of triangles in this meshlet

    // Bounding volume for frustum/occlusion culling
    math::Vec4 bounding_sphere; ///< xyz = center, w = radius

    // Cone for backface culling (optional)
    math::Vec4 cone_apex;       ///< xyz = apex position, w = unused
    math::Vec4 cone_axis_cutoff;///< xyz = axis (normalized), w = cos(half_angle)
};
static_assert(sizeof(Meshlet) == 64, "Meshlet must be 64 bytes");

/**
 * @brief Per-object meshlet info
 */
struct MeshletObjectInfo {
    uint32_t meshlet_offset;    ///< Offset into global meshlet array
    uint32_t meshlet_count;     ///< Number of meshlets for this object
    uint32_t object_id;         ///< Original object ID
    uint32_t flags;             ///< Object flags (visible, etc.)
    math::Mat4 transform;       ///< Object world transform
};

/**
 * @brief Configuration for cluster culling
 */
struct ClusterCullingConfig {
    uint32_t max_meshlets = 1000000;        ///< Maximum meshlets to process
    uint32_t max_vertices_per_meshlet = 64; ///< Vertices per meshlet (Vulkan limit: 256)
    uint32_t max_triangles_per_meshlet = 126; ///< Triangles per meshlet (Vulkan limit: 256)
    bool enable_cone_culling = true;        ///< Enable backface cone culling
    bool enable_frustum_culling = true;     ///< Enable frustum culling per meshlet
    bool enable_occlusion_culling = false;  ///< Enable Hi-Z occlusion culling
    float cone_culling_threshold = 0.7f;    ///< Cone cutoff threshold
};

/**
 * @brief Push constants for cluster culling shader
 */
struct ClusterCullPushConstants {
    math::Mat4 view_proj;
    math::Vec4 camera_position;     ///< xyz = camera pos, w = unused
    math::Vec4 frustum_planes[6];   ///< Pre-extracted frustum planes
    uint32_t meshlet_count;
    uint32_t flags;                 ///< Bit 0: cone culling, Bit 1: frustum, Bit 2: occlusion
    uint32_t padding[2];
};

/**
 * @brief GPU-accelerated cluster/meshlet culling for mesh shading
 *
 * Cluster culling provides fine-grained visibility determination at the
 * meshlet level, enabling efficient GPU-driven rendering with Vulkan 1.3
 * mesh shaders.
 *
 * Key features:
 * - Meshlet-level frustum culling
 * - Backface cone culling (entire meshlet facing away)
 * - Optional Hi-Z occlusion culling
 * - Direct integration with mesh shader pipeline
 *
 * Workflow:
 * 1. Build meshlets from mesh geometry (offline or load-time)
 * 2. Upload meshlet data to GPU
 * 3. Execute cluster culling compute shader
 * 4. Mesh shader reads visibility results
 *
 * Integration with mesh shaders:
 * @code
 * // In task shader:
 * uvec4 payload = uvec4(meshlet_id, 0, 0, 0);
 * if (cluster_culler.is_meshlet_visible(meshlet_id)) {
 *     EmitMeshTasksEXT(1, 1, 1, payload);
 * }
 *
 * // In mesh shader:
 * Meshlet m = meshlets[payload.x];
 * // Generate vertices and primitives...
 * @endcode
 */
class ClusterCuller {
public:
    ClusterCuller();
    ~ClusterCuller();

    // Non-copyable
    ClusterCuller(const ClusterCuller&) = delete;
    ClusterCuller& operator=(const ClusterCuller&) = delete;

    /**
     * @brief Initialize the cluster culler
     *
     * Requires Vulkan 1.3 for mesh shader support.
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
     * @brief Check if initialized and mesh shading is supported
     */
    bool is_initialized() const { return initialized_; }
    bool is_mesh_shading_supported() const { return mesh_shading_supported_; }

    /**
     * @brief Set configuration
     */
    void set_config(const ClusterCullingConfig& config) { config_ = config; }
    const ClusterCullingConfig& get_config() const { return config_; }

    // ========================================================================
    // Meshlet Management
    // ========================================================================

    /**
     * @brief Upload meshlets to GPU
     *
     * @param meshlets Vector of meshlet data
     */
    void upload_meshlets(const std::vector<Meshlet>& meshlets);

    /**
     * @brief Upload object info for meshlet groups
     */
    void upload_object_info(const std::vector<MeshletObjectInfo>& objects);

    /**
     * @brief Get meshlet buffer for shader binding
     */
    const GPUBuffer& get_meshlet_buffer() const { return meshlet_buffer_; }

    /**
     * @brief Get visibility buffer for shader binding
     */
    const GPUBuffer& get_visibility_buffer() const { return visibility_buffer_; }

    /**
     * @brief Get visible meshlet indices buffer
     */
    const GPUBuffer& get_visible_indices_buffer() const { return visible_indices_buffer_; }

    // ========================================================================
    // Culling Operations
    // ========================================================================

    /**
     * @brief Execute cluster culling on GPU
     *
     * @param view_proj View-projection matrix
     * @param camera_pos Camera position (for cone culling)
     * @param cmd_buffer Command buffer
     */
    void cull(
        const math::Mat4& view_proj,
        const math::Vec3& camera_pos,
        VkCommandBuffer cmd_buffer
    );

    /**
     * @brief Get visible meshlet count (requires GPU readback)
     */
    uint32_t get_visible_count();

    /**
     * @brief Download visible meshlet indices
     */
    std::vector<uint32_t> get_visible_indices();

    /**
     * @brief CPU fallback for validation
     */
    std::vector<uint32_t> cull_cpu(
        const math::Mat4& view_proj,
        const math::Vec3& camera_pos,
        const std::vector<Meshlet>& meshlets
    );

    // ========================================================================
    // Meshlet Generation (Static Utilities)
    // ========================================================================

    /**
     * @brief Generate meshlets from triangle mesh
     *
     * @param vertices Vertex positions
     * @param indices Triangle indices
     * @param max_vertices Max vertices per meshlet
     * @param max_triangles Max triangles per meshlet
     * @return Vector of generated meshlets
     */
    static std::vector<Meshlet> generate_meshlets(
        const std::vector<math::Vec3>& vertices,
        const std::vector<uint32_t>& indices,
        uint32_t max_vertices = 64,
        uint32_t max_triangles = 126
    );

    /**
     * @brief Compute bounding sphere for meshlet
     */
    static math::Vec4 compute_bounding_sphere(
        const std::vector<math::Vec3>& vertices,
        const std::vector<uint32_t>& meshlet_indices
    );

    /**
     * @brief Compute backface culling cone for meshlet
     *
     * @return Cone apex and axis+cutoff. If cone is invalid (normals too diverse),
     *         cutoff will be 1.0 (always visible)
     */
    static void compute_cone(
        const std::vector<math::Vec3>& vertices,
        const std::vector<math::Vec3>& normals,
        const std::vector<uint32_t>& meshlet_indices,
        math::Vec4& out_apex,
        math::Vec4& out_axis_cutoff
    );

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Stats {
        uint32_t total_meshlets = 0;
        uint32_t visible_meshlets = 0;
        uint32_t cone_culled = 0;
        uint32_t frustum_culled = 0;
        uint32_t occlusion_culled = 0;
        float cull_time_ms = 0.0f;
    };
    const Stats& get_stats() const { return stats_; }

private:
    // Configuration
    ClusterCullingConfig config_;
    bool initialized_ = false;
    bool mesh_shading_supported_ = false;

    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;
    uint32_t queue_family_ = 0;

    // Compute pipeline
    VkPipeline cull_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout cull_pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout cull_desc_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool cull_desc_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet cull_desc_set_ = VK_NULL_HANDLE;

    // GPU buffers
    GPUBuffer meshlet_buffer_;          ///< Meshlet data
    GPUBuffer object_info_buffer_;      ///< Per-object meshlet info
    GPUBuffer visibility_buffer_;       ///< Per-meshlet visibility bitfield
    GPUBuffer visible_indices_buffer_;  ///< Compacted visible meshlet indices
    GPUBuffer atomic_counter_buffer_;   ///< Atomic counter for compaction

    uint32_t meshlet_count_ = 0;
    uint32_t object_count_ = 0;
    Stats stats_;

    // Internal helpers
    void detect_mesh_shading_support();
    void create_pipeline();
    void allocate_buffers(uint32_t max_meshlets);
    void update_descriptor_set();

    // Culling helpers
    static bool test_sphere_frustum(
        const math::Vec4& sphere,
        const FrustumPlanes& frustum
    );

    static bool test_cone_backface(
        const math::Vec4& cone_apex,
        const math::Vec4& cone_axis_cutoff,
        const math::Vec3& camera_pos
    );
};

} // namespace culling
} // namespace manim
