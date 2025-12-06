/**
 * @file text_renderer.hpp
 * @brief GPU text rendering using SDF (Signed Distance Fields)
 *
 * Provides high-performance text rendering using:
 * - SPIR-V shaders for SDF processing
 * - Instanced rendering for glyph batches
 * - Support for outlines, glow, and shadows
 */

#pragma once

#include <vulkan/vulkan.h>
#include "manim/core/memory_pool.hpp"
#include "manim/core/math.hpp"
#include "manim/mobject/text/text.hpp"
#include <vector>
#include <memory>

namespace manim {

/**
 * @brief Uniform buffer for SDF text rendering
 *
 * Layout follows std140 rules for GLSL uniform blocks:
 * - mat4: 64 bytes, aligned to 16
 * - vec2: 8 bytes, aligned to 8
 * - float: 4 bytes, aligned to 4
 */
struct alignas(16) TextUniformBuffer {
    math::Mat4 projection;      // offset 0, 64 bytes
    math::Mat4 view;            // offset 64, 64 bytes
    math::Mat4 model;           // offset 128, 64 bytes
    math::Vec2 atlas_size;      // offset 192, 8 bytes
    float sdf_range;            // offset 200, 4 bytes
    float time;                 // offset 204, 4 bytes
    // Total: 208 bytes (std140 requires 16-byte alignment for struct size)
};

/**
 * @brief Effects uniform buffer
 *
 * Layout follows std140 rules for GLSL uniform blocks:
 * - vec4: 16 bytes, aligned to 16
 * - vec2: 8 bytes, aligned to 8
 * - float: 4 bytes, aligned to 4
 */
struct alignas(16) TextEffectsBuffer {
    math::Vec4 outline_color;   // offset 0, 16 bytes
    math::Vec4 glow_color;      // offset 16, 16 bytes
    math::Vec4 shadow_color;    // offset 32, 16 bytes
    math::Vec2 shadow_offset;   // offset 48, 8 bytes
    float smoothing;            // offset 56, 4 bytes
    float gamma;                // offset 60, 4 bytes
    // Total: 64 bytes
};

/**
 * @brief GPU text renderer using SDF
 *
 * Manages the Vulkan pipeline and resources for rendering text
 * using signed distance fields.
 */
class TextRenderer {
public:
    TextRenderer() = default;
    ~TextRenderer();

    /**
     * @brief Initialize the text renderer
     */
    bool initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkRenderPass render_pass,
        MemoryPool& pool,
        uint32_t subpass = 0,
        VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT
    );

    /**
     * @brief Shutdown and release resources
     */
    void shutdown();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Bind the SDF atlas texture
     */
    void set_atlas(const GPUImage& atlas);

    /**
     * @brief Update projection and view matrices
     */
    void set_matrices(const math::Mat4& projection, const math::Mat4& view);

    /**
     * @brief Set default text effects
     */
    void set_default_effects(const TextEffectsBuffer& effects);

    /**
     * @brief Begin text rendering batch
     */
    void begin_batch(VkCommandBuffer cmd);

    /**
     * @brief Render a Text mobject
     *
     * @param cmd Command buffer
     * @param text Text mobject to render
     * @param model Model transformation matrix
     */
    void render_text(VkCommandBuffer cmd, Text& text, const math::Mat4& model);

    /**
     * @brief End text rendering batch
     */
    void end_batch(VkCommandBuffer cmd);

    /**
     * @brief Create GPU resources for a Text mobject
     */
    void upload_text_to_gpu(Text& text);

private:
    /**
     * @brief Load SPIR-V shaders
     */
    bool load_shaders();

    /**
     * @brief Create graphics pipeline
     */
    bool create_pipeline();

    /**
     * @brief Create descriptor set layout and pool
     */
    bool create_descriptors();

    /**
     * @brief Create quad vertex buffer (shared by all instances)
     */
    bool create_quad_buffer();

    /**
     * @brief Update descriptor set with atlas texture
     */
    void update_atlas_descriptor();

    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    MemoryPool* pool_ = nullptr;
    uint32_t subpass_ = 0;
    VkSampleCountFlagBits msaa_samples_ = VK_SAMPLE_COUNT_1_BIT;
    bool initialized_ = false;

    // Shaders
    VkShaderModule vert_shader_ = VK_NULL_HANDLE;
    VkShaderModule frag_shader_ = VK_NULL_HANDLE;

    // Pipeline
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    // Descriptors
    VkDescriptorSetLayout descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

    // Sampler for atlas texture
    VkSampler atlas_sampler_ = VK_NULL_HANDLE;

    // Quad vertex buffer (2 triangles forming a quad)
    GPUBuffer quad_vertex_buffer_;

    // Uniform buffers
    GPUBuffer uniform_buffer_;
    GPUBuffer effects_buffer_;

    // Current state
    TextUniformBuffer uniforms_{};
    TextEffectsBuffer effects_{};
    const GPUImage* current_atlas_ = nullptr;
    bool atlas_dirty_ = true;
};

} // namespace manim
