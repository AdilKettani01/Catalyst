#pragma once

#include "manim/mobject/mobject.hpp"
#include "manim/core/math.hpp"
#include <vector>
#include <optional>

namespace manim {

/**
 * @brief GPU-accelerated 3D mesh object
 *
 * Complete GPU mesh system with:
 * - Vertex/index buffers on GPU
 * - Instanced rendering for multiple copies
 * - GPU normal/tangent computation
 * - Hardware tessellation
 * - LOD system
 *
 * Memory Layout:
 * - Interleaved vertex attributes (position, normal, uv, tangent)
 * - 32-bit indices
 * - Instance buffer for transform matrices
 */
class GPUMesh : public Mobject {
public:
    /**
     * @brief Vertex structure (64 bytes - cache aligned)
     */
    struct Vertex {
        math::Vec3 position;    // 12 bytes
        float padding1;         // 4 bytes (alignment)
        math::Vec3 normal;      // 12 bytes
        float padding2;         // 4 bytes
        math::Vec2 uv;          // 8 bytes
        math::Vec2 padding3;    // 8 bytes
        math::Vec4 tangent;     // 16 bytes (xyz = tangent, w = bitangent sign)

        Vertex() = default;
        Vertex(const math::Vec3& pos, const math::Vec3& norm = math::Vec3{0,0,1},
               const math::Vec2& uv_coord = math::Vec2{0,0})
            : position(pos), normal(norm), uv(uv_coord), tangent(math::Vec4{1,0,0,1}) {}
    };

    GPUMesh() = default;
    virtual ~GPUMesh() = default;

    // ========================================================================
    // Mesh Data
    // ========================================================================

    /**
     * @brief Set mesh vertices
     */
    void set_vertices(const std::vector<Vertex>& vertices);

    /**
     * @brief Set mesh indices (triangles)
     */
    void set_indices(const std::vector<uint32_t>& indices);

    /**
     * @brief Set mesh from positions and indices, auto-compute normals
     */
    void set_mesh(
        const std::vector<math::Vec3>& positions,
        const std::vector<uint32_t>& indices
    );

    const std::vector<Vertex>& get_vertices() const { return vertices_; }
    const std::vector<uint32_t>& get_indices() const { return indices_; }

    // ========================================================================
    // GPU Buffer Management
    // ========================================================================

    /**
     * @brief Upload mesh data to GPU
     */
    void upload_to_gpu(MemoryPool& pool);

    /**
     * @brief Update GPU buffers (for dynamic meshes)
     */
    void update_gpu_buffers();

    /**
     * @brief Get GPU buffers
     */
    const GPUBuffer& get_vertex_buffer() const { return *vertex_buffer_; }
    const GPUBuffer& get_index_buffer() const { return *index_buffer_; }

    // ========================================================================
    // Instanced Rendering
    // ========================================================================

    /**
     * @brief Instance data (per-instance transform + material)
     */
    struct InstanceData {
        math::Mat4 model_matrix;    // 64 bytes
        math::Vec4 color;           // 16 bytes
        // Material properties
        float metallic;             // 4 bytes
        float roughness;            // 4 bytes
        float ao;                   // 4 bytes
        float padding;              // 4 bytes
        // Total: 96 bytes
    };

    /**
     * @brief Setup instanced rendering
     */
    void setup_instancing(uint32_t max_instances);

    /**
     * @brief Add instance
     */
    void add_instance(const InstanceData& instance);

    /**
     * @brief Render all instances
     */
    void render_instanced(VkCommandBuffer cmd, uint32_t instance_count);

    /**
     * @brief Update instance buffer
     */
    void update_instance_buffer();

    // ========================================================================
    // GPU Geometry Processing
    // ========================================================================

    /**
     * @brief Compute vertex normals on GPU
     * @param smooth If true, uses smooth shading; otherwise flat shading
     */
    void compute_normals_gpu(ComputeEngine& engine, bool smooth = true);

    /**
     * @brief Compute tangent space (for normal mapping) on GPU
     */
    void compute_tangents_gpu(ComputeEngine& engine);

    /**
     * @brief GPU tessellation (subdivide triangles)
     * @param level Tessellation level (1 = no subdivision)
     */
    void tessellate_gpu(ComputeEngine& engine, uint32_t level);

    // ========================================================================
    // Procedural Mesh Generation
    // ========================================================================

    /**
     * @brief Generate sphere mesh
     */
    static std::shared_ptr<GPUMesh> create_sphere(
        float radius = 1.0f,
        uint32_t subdivisions = 32
    );

    /**
     * @brief Generate cube mesh
     */
    static std::shared_ptr<GPUMesh> create_cube(
        float size = 1.0f
    );

    /**
     * @brief Generate plane mesh
     */
    static std::shared_ptr<GPUMesh> create_plane(
        float width = 1.0f,
        float height = 1.0f,
        uint32_t subdivisions_x = 1,
        uint32_t subdivisions_y = 1
    );

    /**
     * @brief Generate cylinder mesh
     */
    static std::shared_ptr<GPUMesh> create_cylinder(
        float radius = 1.0f,
        float height = 2.0f,
        uint32_t segments = 32
    );

    /**
     * @brief Generate torus mesh
     */
    static std::shared_ptr<GPUMesh> create_torus(
        float major_radius = 1.0f,
        float minor_radius = 0.3f,
        uint32_t major_segments = 48,
        uint32_t minor_segments = 24
    );

    // ========================================================================
    // LOD System
    // ========================================================================

    struct LOD {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        float distance_threshold;
    };

    /**
     * @brief Add LOD level
     */
    void add_lod_level(const LOD& lod);

    /**
     * @brief Select appropriate LOD based on distance
     */
    size_t select_lod(float camera_distance) const;

    // ========================================================================
    // Utilities
    // ========================================================================

    size_t get_num_vertices() const { return vertices_.size(); }
    size_t get_num_triangles() const { return indices_.size() / 3; }

    /**
     * @brief Compute mesh bounding box
     */
    std::pair<math::Vec3, math::Vec3> get_bounding_box() const override;

    /**
     * @brief Recalculate normals (CPU)
     */
    void recalculate_normals(bool smooth = true);

    /**
     * @brief Merge vertices with same position
     */
    void weld_vertices(float threshold = 1e-6f);

protected:
    void generate_points() override;

private:
    // CPU data
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    // GPU buffers
    std::optional<GPUBuffer> vertex_buffer_;
    std::optional<GPUBuffer> index_buffer_;
    std::optional<GPUBuffer> instance_buffer_;

    // Instancing
    std::vector<InstanceData> instances_;
    uint32_t max_instances_{0};

    // LOD
    std::vector<LOD> lod_levels_;

    // Dirty flags
    bool mesh_dirty_{true};
    bool instances_dirty_{true};
};

}  // namespace manim
