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
 */
struct TextUniformBuffer {
    math::Mat4 projection;
    math::Mat4 view;
    math::Mat4 model;
    math::Vec2 atlas_size;
    float sdf_range;
    float time;
};

/**
 * @brief Effects uniform buffer
 */
struct TextEffectsBuffer {
    math::Vec4 outline_color;
    math::Vec4 glow_color;
    math::Vec4 shadow_color;
    math::Vec2 shadow_offset;
    float smoothing;
    float gamma;
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
