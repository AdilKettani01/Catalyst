/**
 * @file gpu_particle_system.hpp
 * @brief GPU-accelerated particle system
 *
 * Features:
 * - Millions of particles in real-time (60 FPS)
 * - GPU physics simulation (gravity, forces, collisions)
 * - Volumetric rendering
 * - Fluid simulation (SPH, FLIP)
 * - Particle attractors/repellers
 * - Particle emission and lifetime management
 */

#pragma once

#include <manim/core/memory_pool.hpp>
#include <manim/core/math.hpp>
#include <manim/renderer/shader_system.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <functional>

namespace manim {

/**
 * @brief Single particle data (GPU-friendly layout)
 */
struct Particle {
    math::Vec3 position;
    float lifetime;           ///< Current lifetime (0 = dead)

    math::Vec3 velocity;
    float size;

    math::Vec4 color;

    math::Vec3 acceleration;
    float mass;

    // Padding to 64 bytes (cache-friendly)
    float _padding[4];
};
static_assert(sizeof(Particle) == 64, "Particle must be 64 bytes");

/**
 * @brief Particle emitter configuration
 */
struct ParticleEmitterConfig {
    // Emission
    uint32_t particles_per_second = 1000;
    uint32_t max_particles = 100000;
    float particle_lifetime = 5.0f;
    bool continuous = true;

    // Initial state
    math::Vec3 position{0.0f};
    math::Vec3 velocity{0.0f, 1.0f, 0.0f};
    math::Vec3 velocity_randomness{0.1f};

    // Appearance
    float size = 0.05f;
    float size_randomness = 0.0f;
    math::Vec4 color_start{1.0f, 1.0f, 1.0f, 1.0f};
    math::Vec4 color_end{1.0f, 1.0f, 1.0f, 0.0f};  // Fade out

    // Physics
    math::Vec3 gravity{0.0f, -9.81f, 0.0f};
    float drag = 0.1f;
    float mass = 1.0f;

    // Shape
    enum class Shape {
        Point,
        Sphere,
        Box,
        Cone,
        Mesh
    };
    Shape emission_shape = Shape::Point;
    float emission_radius = 0.5f;
};

/**
 * @brief Force field affecting particles
 */
struct ForceField {
    enum class Type {
        Gravity,       ///< Constant gravity
        Attractor,     ///< Point attractor
        Repeller,      ///< Point repeller
        Vortex,        ///< Swirling vortex
        Wind,          ///< Directional wind
        Turbulence     ///< Noise-based turbulence
    };

    Type type;
    math::Vec3 position;
    math::Vec3 direction;  // For wind, vortex
    float strength;
    float radius;          // Influence radius
    float falloff;         // Distance falloff exponent
};

/**
 * @brief GPU particle system
 */
class GPUParticleSystem {
public:
    GPUParticleSystem();
    ~GPUParticleSystem();

    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        MemoryPool& memory_pool,
        ShaderManager& shader_manager
    );

    void shutdown();

    // ========================================================================
    // Simulation
    // ========================================================================

    /**
     * @brief Update particle system (GPU compute shader)
     */
    void update(VkCommandBuffer cmd, float dt);

    /**
     * @brief Emit new particles
     */
    void emit_particles(VkCommandBuffer cmd, uint32_t count);

    /**
     * @brief Reset all particles
     */
    void reset();

    // ========================================================================
    // Rendering
    // ========================================================================

    /**
     * @brief Render particles (instanced draw)
     */
    void render(
        VkCommandBuffer cmd,
        const math::Mat4& view_proj,
        const math::Vec3& camera_pos
    );

    /**
     * @brief Render particles with volumetric lighting
     */
    void render_volumetric(
        VkCommandBuffer cmd,
        const math::Mat4& view_proj
    );

    // ========================================================================
    // Configuration
    // ========================================================================

    void set_emitter_config(const ParticleEmitterConfig& config) {
        emitter_config_ = config;
        resize_particle_buffer();
    }

    const ParticleEmitterConfig& get_emitter_config() const {
        return emitter_config_;
    }

    /**
     * @brief Add force field
     */
    void add_force_field(const ForceField& field);

    /**
     * @brief Remove all force fields
     */
    void clear_force_fields();

    /**
     * @brief Set collision plane
     */
    void set_collision_plane(
        const math::Vec3& point,
        const math::Vec3& normal,
        float restitution = 0.5f
    );

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /**
     * @brief Enable SPH fluid simulation
     */
    void enable_sph_fluid(bool enable) { use_sph_ = enable; }

    /**
     * @brief Set SPH parameters
     */
    struct SPHParams {
        float rest_density = 1000.0f;
        float gas_constant = 2000.0f;
        float viscosity = 0.01f;
        float smoothing_radius = 0.1f;
    };
    void set_sph_params(const SPHParams& params) { sph_params_ = params; }

    /**
     * @brief Enable GPU sorting (for transparency)
     */
    void enable_sorting(bool enable) { enable_sorting_ = enable; }

    /**
     * @brief Set custom update function (called before physics)
     */
    using UpdateCallback = std::function<void(VkCommandBuffer, float)>;
    void set_update_callback(UpdateCallback callback) {
        update_callback_ = callback;
    }

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Stats {
        uint32_t alive_particles = 0;
        uint32_t max_particles = 0;
        float update_time_ms = 0.0f;
        float render_time_ms = 0.0f;
    };

    const Stats& get_stats() const { return stats_; }

private:
    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    MemoryPool* memory_pool_ = nullptr;
    ShaderManager* shader_manager_ = nullptr;

    // Particle buffers (double-buffered for ping-pong)
    static constexpr int NUM_BUFFERS = 2;
    std::array<GPUBuffer, NUM_BUFFERS> particle_buffers_;
    int current_buffer_ = 0;

    GPUBuffer dead_particle_indices_;  // Free list
    GPUBuffer alive_count_buffer_;     // Atomic counter

    // Force fields
    GPUBuffer force_fields_buffer_;
    std::vector<ForceField> force_fields_;

    // Collision
    struct CollisionPlane {
        math::Vec3 point;
        math::Vec3 normal;
        float restitution;
    };
    std::vector<CollisionPlane> collision_planes_;
    GPUBuffer collision_planes_buffer_;

    // Configuration
    ParticleEmitterConfig emitter_config_;
    uint32_t max_particles_ = 0;

    // SPH fluid simulation
    bool use_sph_ = false;
    SPHParams sph_params_;
    GPUBuffer density_buffer_;
    GPUBuffer pressure_buffer_;

    // Sorting
    bool enable_sorting_ = true;
    GPUBuffer sort_keys_buffer_;   // For radix sort
    GPUBuffer sort_values_buffer_;

    // Compute pipelines
    VkPipeline emit_pipeline_ = VK_NULL_HANDLE;
    VkPipeline update_pipeline_ = VK_NULL_HANDLE;
    VkPipeline sph_density_pipeline_ = VK_NULL_HANDLE;
    VkPipeline sph_force_pipeline_ = VK_NULL_HANDLE;
    VkPipeline sort_pipeline_ = VK_NULL_HANDLE;

    // Graphics pipeline
    VkPipeline render_pipeline_ = VK_NULL_HANDLE;
    VkPipeline volumetric_pipeline_ = VK_NULL_HANDLE;

    // Descriptor sets
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout compute_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout graphics_layout_ = VK_NULL_HANDLE;
    VkDescriptorSet compute_descriptor_set_ = VK_NULL_HANDLE;
    VkDescriptorSet graphics_descriptor_set_ = VK_NULL_HANDLE;

    // Shaders
    std::shared_ptr<ShaderModule> emit_shader_;
    std::shared_ptr<ShaderModule> update_shader_;
    std::shared_ptr<ShaderModule> render_vs_;
    std::shared_ptr<ShaderModule> render_fs_;

    // Statistics
    Stats stats_;

    // Custom callback
    UpdateCallback update_callback_;

    // Helper functions
    void create_buffers();
    void create_pipelines();
    void create_descriptors();
    void resize_particle_buffer();

    void update_force_fields_buffer();
    void update_collision_planes_buffer();

    void emit_particles_gpu(VkCommandBuffer cmd, uint32_t count);
    void update_particles_cpu(float dt);  // Fallback CPU update

    // SPH simulation
    void compute_sph_density(VkCommandBuffer cmd);
    void compute_sph_forces(VkCommandBuffer cmd);

    // Sorting
    void sort_particles_by_depth(
        VkCommandBuffer cmd,
        const math::Vec3& camera_pos
    );

    // Radix sort on GPU
    void radix_sort_gpu(
        VkCommandBuffer cmd,
        GPUBuffer& keys,
        GPUBuffer& values,
        uint32_t num_elements
    );
};

/**
 * @brief Particle trail effect
 *
 * Creates trails behind moving particles
 */
class ParticleTrail {
public:
    void initialize(
        VkDevice device,
        MemoryPool& memory_pool,
        uint32_t max_trail_points
    );

    void update(VkCommandBuffer cmd, const Particle& particle, float dt);
    void render(VkCommandBuffer cmd, const math::Mat4& view_proj);

private:
    struct TrailPoint {
        math::Vec3 position;
        math::Vec4 color;
        float time;
    };

    GPUBuffer trail_buffer_;
    uint32_t max_points_ = 0;
    uint32_t current_point_ = 0;
};

/**
 * @brief Particle system factory
 *
 * Provides preset particle effects
 */
class ParticlePresets {
public:
    static ParticleEmitterConfig fire();
    static ParticleEmitterConfig smoke();
    static ParticleEmitterConfig rain();
    static ParticleEmitterConfig snow();
    static ParticleEmitterConfig explosion();
    static ParticleEmitterConfig fountain();
    static ParticleEmitterConfig sparks();
    static ParticleEmitterConfig magic();
};

} // namespace manim
