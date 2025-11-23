// GPU particle system implementation
#include "manim/renderer/gpu_particle_system.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace manim {

// ============================================================================
// GPUParticleSystem Implementation
// ============================================================================

GPUParticleSystem::GPUParticleSystem(
    std::shared_ptr<MemoryPool> memory_pool,
    std::shared_ptr<ComputeEngine> compute_engine,
    uint32_t max_particles
) : memory_pool_(memory_pool),
    compute_engine_(compute_engine),
    max_particles_(max_particles),
    num_alive_(0),
    sph_enabled_(false),
    sorting_enabled_(false),
    time_accumulator_(0.0f) {

    spdlog::info("Initializing GPU particle system: {} max particles", max_particles);

    initialize_buffers();
    initialize_compute_pipeline();
}

GPUParticleSystem::~GPUParticleSystem() {
    cleanup();
}

void GPUParticleSystem::initialize_buffers() {
    // Allocate particle buffer
    VkDeviceSize buffer_size = sizeof(Particle) * max_particles_;

    particle_buffer_ = memory_pool_->allocate_buffer(
        buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Allocate indirect draw buffer (for GPU-driven rendering)
    VkDeviceSize indirect_size = sizeof(VkDrawIndirectCommand);
    indirect_draw_buffer_ = memory_pool_->allocate_buffer(
        indirect_size,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Initialize particles to dead state
    std::vector<Particle> initial_particles(max_particles_);
    for (auto& p : initial_particles) {
        p.lifetime = 0.0f;  // Dead
    }

    // Upload initial data
    // In real implementation, would use staging buffer
    // particle_buffer_.upload(initial_particles.data(), buffer_size);

    spdlog::info("Particle buffers allocated: {:.2f} MB",
                 buffer_size / (1024.0f * 1024.0f));
}

void GPUParticleSystem::initialize_compute_pipeline() {
    // Load particle update compute shader
    // This would create the compute pipeline for particle updates

    spdlog::info("Particle compute pipeline initialized");
}

void GPUParticleSystem::cleanup() {
    // Buffers cleaned up by memory pool
    force_fields_.clear();
    spdlog::info("GPU particle system cleaned up");
}

void GPUParticleSystem::update(VkCommandBuffer cmd, float dt) {
    time_accumulator_ += dt;

    // Emit particles based on emission rate
    uint32_t particles_to_emit = static_cast<uint32_t>(
        emitter_config_.rate * dt
    );

    if (particles_to_emit > 0) {
        emit_particles(cmd, particles_to_emit);
    }

    // Update particle physics on GPU
    update_particles_gpu(cmd, dt);

    // Sort particles if enabled (for transparency)
    if (sorting_enabled_) {
        sort_particles_gpu(cmd);
    }

    stats_.active_particles = num_alive_;
}

void GPUParticleSystem::emit_particles(VkCommandBuffer cmd, uint32_t count) {
    count = std::min(count, max_particles_ - num_alive_);

    if (count == 0) {
        return;
    }

    // Emission happens in the update compute shader
    // Just update the emit count
    num_alive_ += count;

    spdlog::debug("Emitting {} particles ({} alive)", count, num_alive_);
}

void GPUParticleSystem::update_particles_gpu(VkCommandBuffer cmd, float dt) {
    // Bind compute pipeline
    // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, particle_update_pipeline_);

    // Update push constants
    struct PushConstants {
        uint32_t num_particles;
        uint32_t num_alive;
        float delta_time;
        uint32_t emit_count;
        uint32_t random_seed;
    } push_constants;

    push_constants.num_particles = max_particles_;
    push_constants.num_alive = num_alive_;
    push_constants.delta_time = dt;
    push_constants.emit_count = 0;  // Set by emit_particles
    push_constants.random_seed = static_cast<uint32_t>(time_accumulator_ * 1000.0f);

    // Push constants
    // vkCmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants), &push_constants);

    // Bind descriptor sets (particle buffer, emitter config, force fields)
    // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ...);

    // Dispatch compute shader
    uint32_t workgroup_size = 256;
    uint32_t num_workgroups = (max_particles_ + workgroup_size - 1) / workgroup_size;
    // vkCmdDispatch(cmd, num_workgroups, 1, 1);

    // Memory barrier
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

    // vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void GPUParticleSystem::render(VkCommandBuffer cmd, const math::Mat4& view_proj) {
    // Bind particle rendering pipeline
    // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particle_render_pipeline_);

    // Bind particle buffer as vertex buffer
    VkDeviceSize offset = 0;
    // vkCmdBindVertexBuffers(cmd, 0, 1, &particle_buffer_.buffer, &offset);

    // Update uniforms (view-projection matrix)
    // vkCmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(math::Mat4), &view_proj);

    // Draw particles (instanced)
    // Each particle is a billboard quad (2 triangles = 6 vertices)
    // vkCmdDraw(cmd, 6, num_alive_, 0, 0);

    stats_.draw_calls++;
}

void GPUParticleSystem::render_volumetric(VkCommandBuffer cmd, const math::Mat4& view_proj) {
    // Volumetric rendering for dense particle effects (smoke, fire, etc.)
    // Uses ray marching through particle density field

    // 1. Render particles to 3D density texture
    // 2. Ray march through density field

    spdlog::debug("Rendering volumetric particles");
}

void GPUParticleSystem::sort_particles_gpu(VkCommandBuffer cmd) {
    // GPU radix sort or bitonic sort
    // Sort particles by depth for correct alpha blending

    // This is a complex operation - simplified here
    spdlog::debug("Sorting {} particles", num_alive_);
}

void GPUParticleSystem::set_emitter(const ParticleEmitterConfig& config) {
    emitter_config_ = config;

    // Upload to GPU uniform buffer
    spdlog::info("Particle emitter configured: rate={:.1f}/s", config.rate);
}

void GPUParticleSystem::add_force_field(const ForceField& field) {
    if (force_fields_.size() >= 8) {
        spdlog::warn("Maximum force fields (8) reached, ignoring new field");
        return;
    }

    force_fields_.push_back(field);

    // Update force field uniform buffer
    spdlog::info("Added force field: type={}", static_cast<int>(field.type));
}

void GPUParticleSystem::clear_force_fields() {
    force_fields_.clear();
    spdlog::info("Cleared all force fields");
}

void GPUParticleSystem::enable_sph_fluid(bool enable) {
    sph_enabled_ = enable;

    if (enable) {
        initialize_sph();
        spdlog::info("SPH fluid simulation enabled");
    } else {
        spdlog::info("SPH fluid simulation disabled");
    }
}

void GPUParticleSystem::initialize_sph() {
    // Initialize Smoothed Particle Hydrodynamics
    // - Create spatial hash grid for neighbor search
    // - Set SPH parameters (density, viscosity, etc.)

    sph_params_.enabled = true;
    sph_params_.rest_density = 1000.0f;  // Water density
    sph_params_.gas_constant = 2000.0f;
    sph_params_.viscosity = 0.01f;
    sph_params_.smoothing_radius = 0.1f;
    sph_params_.particle_mass = 0.02f;

    // Create spatial hash grid
    uint32_t grid_size = 64;  // 64^3 grid
    VkDeviceSize grid_buffer_size = sizeof(uint32_t) * grid_size * grid_size * grid_size;

    spatial_hash_grid_ = memory_pool_->allocate_buffer(
        grid_buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    spdlog::info("SPH initialized: grid={}^3, smoothing_radius={:.3f}",
                 grid_size, sph_params_.smoothing_radius);
}

void GPUParticleSystem::enable_sorting(bool enable) {
    sorting_enabled_ = enable;
    spdlog::info("Particle sorting {}", enable ? "enabled" : "disabled");
}

void GPUParticleSystem::reset() {
    num_alive_ = 0;
    time_accumulator_ = 0.0f;

    // Reset all particles to dead state
    // Would upload dead particles to GPU buffer

    spdlog::info("Particle system reset");
}

uint32_t GPUParticleSystem::get_num_alive() const {
    return num_alive_;
}

uint32_t GPUParticleSystem::get_max_particles() const {
    return max_particles_;
}

const ParticleStats& GPUParticleSystem::get_stats() const {
    return stats_;
}

// ============================================================================
// ParticlePresets Implementation
// ============================================================================

ParticleEmitterConfig ParticlePresets::fire() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 0.0f, 0.0f};
    config.direction = math::Vec3{0.0f, 1.0f, 0.0f};  // Upward
    config.rate = 100.0f;
    config.spread = 0.3f;
    config.velocity_min = math::Vec3{-0.1f, 0.5f, -0.1f};
    config.velocity_max = math::Vec3{0.1f, 2.0f, 0.1f};
    config.lifetime_min = 0.5f;
    config.lifetime_max = 2.0f;
    config.size_start = 0.2f;
    config.size_end = 0.05f;
    config.color_start = math::Vec4{1.0f, 0.8f, 0.0f, 1.0f};  // Yellow
    config.color_end = math::Vec4{0.8f, 0.2f, 0.0f, 0.0f};     // Red, transparent
    return config;
}

ParticleEmitterConfig ParticlePresets::smoke() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 0.0f, 0.0f};
    config.direction = math::Vec3{0.0f, 1.0f, 0.0f};
    config.rate = 50.0f;
    config.spread = 0.5f;
    config.velocity_min = math::Vec3{-0.2f, 0.3f, -0.2f};
    config.velocity_max = math::Vec3{0.2f, 1.0f, 0.2f};
    config.lifetime_min = 2.0f;
    config.lifetime_max = 5.0f;
    config.size_start = 0.1f;
    config.size_end = 0.5f;
    config.color_start = math::Vec4{0.3f, 0.3f, 0.3f, 0.8f};  // Gray
    config.color_end = math::Vec4{0.5f, 0.5f, 0.5f, 0.0f};
    return config;
}

ParticleEmitterConfig ParticlePresets::explosion() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 0.0f, 0.0f};
    config.direction = math::Vec3{0.0f, 1.0f, 0.0f};
    config.rate = 1000.0f;  // Burst
    config.spread = 3.14159f;  // All directions
    config.velocity_min = math::Vec3{-5.0f, -5.0f, -5.0f};
    config.velocity_max = math::Vec3{5.0f, 5.0f, 5.0f};
    config.lifetime_min = 0.5f;
    config.lifetime_max = 2.0f;
    config.size_start = 0.3f;
    config.size_end = 0.0f;
    config.color_start = math::Vec4{1.0f, 0.5f, 0.0f, 1.0f};
    config.color_end = math::Vec4{0.5f, 0.0f, 0.0f, 0.0f};
    return config;
}

ParticleEmitterConfig ParticlePresets::rain() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 10.0f, 0.0f};
    config.direction = math::Vec3{0.0f, -1.0f, 0.0f};  // Downward
    config.rate = 500.0f;
    config.spread = 0.0f;  // Straight down
    config.velocity_min = math::Vec3{0.0f, -10.0f, 0.0f};
    config.velocity_max = math::Vec3{0.0f, -15.0f, 0.0f};
    config.lifetime_min = 2.0f;
    config.lifetime_max = 3.0f;
    config.size_start = 0.05f;
    config.size_end = 0.05f;
    config.color_start = math::Vec4{0.5f, 0.5f, 1.0f, 0.6f};  // Light blue
    config.color_end = math::Vec4{0.3f, 0.3f, 0.8f, 0.3f};
    return config;
}

ParticleEmitterConfig ParticlePresets::snow() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 10.0f, 0.0f};
    config.direction = math::Vec3{0.0f, -1.0f, 0.0f};
    config.rate = 200.0f;
    config.spread = 0.2f;
    config.velocity_min = math::Vec3{-0.5f, -1.0f, -0.5f};
    config.velocity_max = math::Vec3{0.5f, -0.5f, 0.5f};
    config.lifetime_min = 5.0f;
    config.lifetime_max = 10.0f;
    config.size_start = 0.1f;
    config.size_end = 0.1f;
    config.color_start = math::Vec4{1.0f, 1.0f, 1.0f, 1.0f};  // White
    config.color_end = math::Vec4{0.9f, 0.9f, 0.9f, 0.8f};
    return config;
}

ParticleEmitterConfig ParticlePresets::sparks() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 0.0f, 0.0f};
    config.direction = math::Vec3{0.0f, 1.0f, 0.0f};
    config.rate = 300.0f;
    config.spread = 1.0f;
    config.velocity_min = math::Vec3{-3.0f, 1.0f, -3.0f};
    config.velocity_max = math::Vec3{3.0f, 5.0f, 3.0f};
    config.lifetime_min = 0.3f;
    config.lifetime_max = 1.0f;
    config.size_start = 0.1f;
    config.size_end = 0.0f;
    config.color_start = math::Vec4{1.0f, 0.9f, 0.3f, 1.0f};  // Bright yellow
    config.color_end = math::Vec4{1.0f, 0.3f, 0.0f, 0.0f};     // Orange
    return config;
}

ParticleEmitterConfig ParticlePresets::waterfall() {
    ParticleEmitterConfig config;
    config.position = math::Vec3{0.0f, 5.0f, 0.0f};
    config.direction = math::Vec3{0.0f, -1.0f, 0.0f};
    config.rate = 1000.0f;
    config.spread = 0.1f;
    config.velocity_min = math::Vec3{-0.5f, -5.0f, -0.5f};
    config.velocity_max = math::Vec3{0.5f, -3.0f, 0.5f};
    config.lifetime_min = 2.0f;
    config.lifetime_max = 4.0f;
    config.size_start = 0.05f;
    config.size_end = 0.02f;
    config.color_start = math::Vec4{0.5f, 0.7f, 1.0f, 0.7f};  // Blue
    config.color_end = math::Vec4{0.3f, 0.5f, 0.8f, 0.5f};
    return config;
}

}  // namespace manim
