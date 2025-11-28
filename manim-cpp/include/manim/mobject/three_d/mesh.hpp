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

    /**
     * @brief Upload instance buffer to GPU
     */
    void upload_instance_buffer(MemoryPool& pool);

    /**
     * @brief Get number of instances
     */
    uint32_t get_instance_count() const { return static_cast<uint32_t>(instances_.size()); }

    /**
     * @brief Clear all instances
     */
    void clear_instances() { instances_.clear(); instances_dirty_ = true; }

    /**
     * @brief Get instance buffer
     */
    const GPUBuffer* get_instance_buffer() const {
        return instance_buffer_ ? &*instance_buffer_ : nullptr;
    }

    /**
     * @brief Check if mesh has instancing enabled
     */
    bool has_instancing() const { return max_instances_ > 0; }

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

    /**
     * @brief Generate cone mesh
     * @param radius Base radius
     * @param height Cone height
     * @param radial_segments Number of segments around the cone
     * @param open_ended If true, no base cap
     */
    static std::shared_ptr<GPUMesh> create_cone(
        float radius = 1.0f,
        float height = 2.0f,
        uint32_t radial_segments = 32,
        bool open_ended = false
    );

    /**
     * @brief Generate capsule mesh (cylinder with hemisphere ends)
     * @param radius Radius of cylinder and hemispheres
     * @param height Total height including hemispheres
     * @param radial_segments Segments around circumference
     * @param hemisphere_segments Segments in each hemisphere
     */
    static std::shared_ptr<GPUMesh> create_capsule(
        float radius = 0.5f,
        float height = 2.0f,
        uint32_t radial_segments = 32,
        uint32_t hemisphere_segments = 8
    );

    /**
     * @brief Generate icosphere mesh (subdivided icosahedron)
     * Better triangle distribution than UV sphere
     * @param radius Sphere radius
     * @param subdivisions 0 = icosahedron (20 faces), each level multiplies faces by 4
     */
    static std::shared_ptr<GPUMesh> create_icosphere(
        float radius = 1.0f,
        uint32_t subdivisions = 2
    );

    /**
     * @brief Generate 3D arrow mesh
     * @param shaft_radius Radius of the arrow shaft
     * @param shaft_length Length of the shaft
     * @param head_radius Radius of the arrow head base
     * @param head_length Length of the arrow head
     * @param radial_segments Segments around circumference
     */
    static std::shared_ptr<GPUMesh> create_arrow(
        float shaft_radius = 0.05f,
        float shaft_length = 1.0f,
        float head_radius = 0.15f,
        float head_length = 0.3f,
        uint32_t radial_segments = 16
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
     * @brief Configuration for automatic LOD generation
     */
    struct LODGenerationConfig {
        std::vector<float> distance_thresholds;  // e.g., {10, 25, 50, 100}
        std::vector<float> target_ratios;        // e.g., {1.0, 0.5, 0.25, 0.1}
        bool preserve_boundaries = true;
        bool preserve_uv_seams = true;
        bool generate_morph_targets = true;      // For geomorphing
    };

    /**
     * @brief Add LOD level
     */
    void add_lod_level(const LOD& lod);
    void add_lod_level(float distance_threshold, uint32_t /*detail_level*/) {
        LOD lod;
        lod.distance_threshold = distance_threshold;
        add_lod_level(lod);
    }

    /**
     * @brief Select appropriate LOD based on distance
     */
    size_t select_lod(float camera_distance) const;

    /**
     * @brief Select LOD with hysteresis to prevent popping
     */
    size_t select_lod_with_hysteresis(float camera_distance, float hysteresis = 0.1f);

    /**
     * @brief Automatically generate LOD levels using Quadric Error Metrics
     * @param config LOD generation configuration
     */
    void generate_lod_levels(const LODGenerationConfig& config);

    /**
     * @brief Get number of LOD levels
     */
    size_t get_lod_count() const { return lod_levels_.size() + 1; }  // +1 for base mesh

    /**
     * @brief Get LOD level vertices/indices
     */
    const LOD* get_lod(size_t level) const {
        return level < lod_levels_.size() ? &lod_levels_[level] : nullptr;
    }

    // ========================================================================
    // Geomorphing (Smooth LOD Transitions)
    // ========================================================================

    /**
     * @brief Extended vertex structure for geomorphing
     * Stores both current and morph target positions for smooth transitions
     */
    struct GeomorphVertex {
        math::Vec3 position;        // Current LOD position
        float padding1;
        math::Vec3 morph_target;    // Next LOD position (for morphing)
        float padding2;
        math::Vec3 normal;
        float padding3;
        math::Vec3 morph_normal;    // Next LOD normal
        float padding4;
        math::Vec2 uv;
        math::Vec2 padding5;
        math::Vec4 tangent;
    };  // 96 bytes - aligned

    /**
     * @brief LOD transition state for geomorphing
     */
    struct LODTransitionState {
        size_t from_lod{0};
        size_t to_lod{0};
        float morph_factor{0.0f};       // 0 = from, 1 = to
        float transition_duration{0.5f};
        bool transitioning{false};
    };

    /**
     * @brief Start LOD transition (for geomorphing)
     * @param target_lod Target LOD level to transition to
     * @param duration Transition duration in seconds
     */
    void start_lod_transition(size_t target_lod, float duration = 0.5f);

    /**
     * @brief Update LOD transition state
     * @param dt Delta time in seconds
     */
    void update_lod_transition(float dt);

    /**
     * @brief Get current morph factor for shader
     */
    float get_morph_factor() const { return transition_state_.morph_factor; }

    /**
     * @brief Check if LOD transition is in progress
     */
    bool is_transitioning() const { return transition_state_.transitioning; }

    /**
     * @brief Get transition state
     */
    const LODTransitionState& get_transition_state() const { return transition_state_; }

    /**
     * @brief Build geomorph vertex buffer for current transition
     * Combines current LOD with morph targets from next LOD
     */
    void build_geomorph_buffer();

    /**
     * @brief Get geomorph vertices (for rendering during transition)
     */
    const std::vector<GeomorphVertex>& get_geomorph_vertices() const { return geomorph_vertices_; }

    // ========================================================================
    // Utilities
    // ========================================================================

    size_t get_num_vertices() const { return vertices_.size(); }
    size_t get_num_triangles() const { return indices_.size() / 3; }

    /**
     * @brief Copy this mesh
     */
    Ptr copy() const override {
        auto copied = std::make_shared<GPUMesh>();
        copied->vertices_ = vertices_;
        copied->indices_ = indices_;
        copied->mesh_dirty_ = true;
        return copied;
    }

    /**
     * @brief Compute mesh bounding box
     */
    std::pair<math::Vec3, math::Vec3> get_bounding_box() const;

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
    mutable size_t current_lod_{0};  // For hysteresis tracking

    // Geomorphing
    LODTransitionState transition_state_;
    std::vector<GeomorphVertex> geomorph_vertices_;
    std::optional<GPUBuffer> geomorph_buffer_;

    // Dirty flags
    bool mesh_dirty_{true};
    bool instances_dirty_{true};
};

using Mesh = GPUMesh;

}  // namespace manim
