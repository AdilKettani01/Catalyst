/**
 * @file gpu_3d_renderer.hpp
 * @brief Advanced GPU-based 3D rendering pipeline
 *
 * Implements state-of-the-art rendering techniques:
 * - Deferred rendering
 * - Cascaded shadow maps
 * - Ray-traced shadows and GI (RTX/DXR)
 * - Voxel global illumination
 * - Screen-space reflections
 * - Advanced post-processing (TAA, motion blur, DoF, bloom)
 */

#pragma once

#include <manim/renderer/renderer.hpp>
#include <manim/renderer/text_renderer.hpp>
#include <manim/core/compute_engine.hpp>
#include <manim/culling/indirect_renderer.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <vector>

namespace manim {

/**
 * @brief G-Buffer for deferred rendering
 */
struct GBuffer {
    GPUImage position;        ///< World position (RGB) + depth (A)
    GPUImage normal;          ///< Normal (RGB) + roughness (A)
    GPUImage albedo;          ///< Albedo color (RGB) + metallic (A)
    GPUImage emission;        ///< Emissive (RGB) + AO (A)
    GPUImage velocity;        ///< Motion vectors for TAA
    GPUImage depth;           ///< Depth buffer

    void create(MemoryPool& pool, uint32_t width, uint32_t height);
    void destroy();
};

/**
 * @brief Shadow map configuration
 */
struct ShadowConfig {
    static constexpr int MAX_CASCADES = 4;

    bool enable_csm = true;           ///< Cascaded shadow maps
    bool enable_vsm = false;          ///< Variance shadow maps
    bool enable_ray_traced = false;   ///< Ray-traced shadows

    uint32_t resolution = 2048;
    int num_cascades = 4;
    float cascade_split_lambda = 0.95f;  ///< Practical split scheme

    // Filtering
    bool enable_pcf = true;           ///< Percentage-closer filtering
    int pcf_samples = 4;
};

/**
 * @brief Global illumination configuration
 */
struct GIConfig {
    enum class Method {
        None,
        SSGI,         ///< Screen-space GI
        VXGI,         ///< Voxel-based GI
        RayTraced     ///< Ray-traced GI (RTX)
    };

    Method method = Method::VXGI;

    // VXGI settings
    uint32_t voxel_resolution = 256;
    uint32_t voxel_mip_levels = 8;
    float voxel_size = 0.1f;

    // SSGI settings
    int ssgi_samples = 16;
    float ssgi_radius = 0.5f;

    // Ray tracing settings
    int rt_samples_per_pixel = 4;
    int rt_max_bounces = 3;
};

/**
 * @brief Post-processing configuration
 */
struct PostProcessConfig {
    // Temporal Anti-Aliasing
    bool enable_taa = true;
    float taa_blend_factor = 0.05f;

    // Motion Blur
    bool enable_motion_blur = false;
    float motion_blur_strength = 0.5f;
    int motion_blur_samples = 8;

    // Depth of Field
    bool enable_dof = false;
    float dof_focal_distance = 10.0f;
    float dof_aperture = 0.1f;
    int dof_samples = 16;

    // Bloom
    bool enable_bloom = true;
    float bloom_threshold = 1.0f;
    float bloom_intensity = 0.2f;
    int bloom_passes = 5;

    // Tone Mapping
    enum class ToneMapper {
        None,
        Reinhard,
        ACES,
        Uncharted2
    };
    ToneMapper tone_mapper = ToneMapper::ACES;
    float exposure = 1.0f;

    // Color Grading
    float gamma = 2.2f;
    float saturation = 1.0f;
    float contrast = 1.0f;
};

/**
 * @brief Aggregate configuration for the GPU3DRenderer
 */
struct GPU3DConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool enable_swapchain = false;
    uint32_t swapchain_image_count = 2;
    ShadowConfig shadow_config{};
    GIConfig gi_config{};
    PostProcessConfig postprocess_config{};
};

/**
 * @brief Advanced GPU 3D rendering pipeline
 */
class GPU3DRenderer {
public:
    GPU3DRenderer();
    ~GPU3DRenderer();

    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkQueue graphics_queue,
        uint32_t graphics_queue_family,
        MemoryPool& memory_pool,
        uint32_t width,
        uint32_t height,
        uint32_t msaa_samples = 4  ///< MSAA sample count (1, 2, 4, 8, 16)
    );
    void setup_deferred_pipeline() {}

    void shutdown();

    void resize(uint32_t width, uint32_t height);

    // ========================================================================
    // Main Rendering Pipeline
    // ========================================================================

    /**
     * @brief Render full 3D scene
     */
    void render(
        VkCommandBuffer cmd,
        Scene& scene,
        Camera& camera
    );
    void advance_swapchain();

    // ========================================================================
    // Deferred Rendering Pipeline
    // ========================================================================

    /**
     * @brief Geometry pass - fill G-buffer
     */
    void render_geometry_pass(
        VkCommandBuffer cmd,
        Scene& scene,
        Camera& camera
    );

    /**
     * @brief Lighting pass - compute lighting from G-buffer
     */
    void render_lighting_pass(VkCommandBuffer cmd);

    /**
     * @brief Transparency pass - forward rendering for transparent objects
     */
    void render_transparency_pass(
        VkCommandBuffer cmd,
        Scene& scene,
        Camera& camera
    );

    // ========================================================================
    // Shadow Techniques
    // ========================================================================

    /**
     * @brief Render cascaded shadow maps
     */
    void render_cascaded_shadow_maps(
        VkCommandBuffer cmd,
        Scene& scene,
        const math::Vec3& light_dir
    );

    /**
     * @brief Compute ray-traced shadows (RTX/DXR)
     */
    void render_ray_traced_shadows(
        VkCommandBuffer cmd,
        VkAccelerationStructureKHR tlas
    );

    // ========================================================================
    // Global Illumination
    // ========================================================================

    /**
     * @brief Voxelize scene for VXGI
     */
    void voxelize_scene(
        VkCommandBuffer cmd,
        Scene& scene
    );

    /**
     * @brief Compute voxel global illumination
     */
    void compute_voxel_gi(VkCommandBuffer cmd);

    /**
     * @brief Render screen-space reflections
     */
    void render_screen_space_reflections(VkCommandBuffer cmd);

    /**
     * @brief Compute ray-traced global illumination
     */
    void render_ray_traced_gi(
        VkCommandBuffer cmd,
        VkAccelerationStructureKHR tlas
    );

    // ========================================================================
    // Post-Processing
    // ========================================================================

    /**
     * @brief Apply temporal anti-aliasing
     */
    void apply_taa(VkCommandBuffer cmd);

    /**
     * @brief Apply motion blur
     */
    void apply_motion_blur(VkCommandBuffer cmd);

    /**
     * @brief Apply depth of field
     */
    void apply_dof(VkCommandBuffer cmd);

    /**
     * @brief Apply bloom effect
     */
    void apply_bloom(VkCommandBuffer cmd);

    /**
     * @brief Apply tone mapping
     */
    void apply_tone_mapping(VkCommandBuffer cmd);

    /**
     * @brief Full post-processing chain
     */
    void post_process(VkCommandBuffer cmd);

    // ========================================================================
    // Configuration
    // ========================================================================

    void set_shadow_config(const ShadowConfig& config) { shadow_config_ = config; }
    void set_gi_config(const GIConfig& config) { gi_config_ = config; }
    void set_post_process_config(const PostProcessConfig& config) { post_config_ = config; }

    const ShadowConfig& get_shadow_config() const { return shadow_config_; }
    const GIConfig& get_gi_config() const { return gi_config_; }
    const PostProcessConfig& get_post_process_config() const { return post_config_; }

    // ========================================================================
    // Render Targets
    // ========================================================================

    const GPUImage& get_final_image() const;
    const GBuffer& get_gbuffer() const { return gbuffer_; }
    const GPUImage& get_swapchain_image() const { return swapchain_images_.empty() ? final_image_ : swapchain_images_[swapchain_present_index_]; }

    /**
     * @brief Get depth image for Hi-Z pyramid construction
     *
     * Use with OcclusionCuller::build_hiz_pyramid_temporal() after render pass:
     * @code
     * renderer.render(cmd, scene, camera);
     * // After render pass:
     * occlusion_culler.build_hiz_pyramid_temporal(
     *     renderer.get_depth_image().get_image(),
     *     renderer.get_depth_image().get_view(),
     *     width, height, view_proj, cmd);
     * occlusion_culler.end_frame();
     * @endcode
     */
    const GPUImage& get_depth_image() const { return depth_image_; }

    // ========================================================================
    // Indirect Rendering
    // ========================================================================

    /**
     * @brief Enable/disable GPU-driven indirect rendering
     */
    void set_indirect_rendering_enabled(bool enabled) { indirect_rendering_enabled_ = enabled; }
    bool is_indirect_rendering_enabled() const { return indirect_rendering_enabled_; }

    /**
     * @brief Get indirect renderer for advanced usage
     */
    culling::IndirectRenderer& get_indirect_renderer() { return indirect_renderer_; }
    const culling::IndirectRenderer& get_indirect_renderer() const { return indirect_renderer_; }

    /**
     * @brief Render using GPU-driven indirect rendering
     *
     * This method performs:
     * 1. Submit objects from scene to indirect renderer
     * 2. Execute GPU culling
     * 3. Generate draw commands on GPU
     * 4. Record indirect draw calls
     *
     * @param cmd Command buffer
     * @param scene Scene to render
     * @param camera Camera for view-projection matrix
     */
    void render_indirect(
        VkCommandBuffer cmd,
        Scene& scene,
        Camera& camera
    );

private:
    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    uint32_t graphics_queue_family_ = 0;
    MemoryPool* memory_pool_ = nullptr;

    GPU3DConfig config_{};
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // G-Buffer
    GBuffer gbuffer_;

    // Shadow maps
    struct ShadowCascade {
        GPUImage depth_map;
        math::Mat4 view_proj_matrix;
        float split_depth;
    };
    std::array<ShadowCascade, ShadowConfig::MAX_CASCADES> shadow_cascades_;

    // Voxel GI
    GPUImage voxel_grid_;           // 3D texture
    std::vector<GPUImage> voxel_mipmaps_;

    // Render targets
    GPUImage hdr_image_;            // HDR render target
    GPUImage final_image_;          // Final LDR output
    std::vector<GPUImage> swapchain_images_; // Offscreen swapchain images
    uint32_t swapchain_index_ = 0;
    uint32_t swapchain_present_index_ = 0;
    uint32_t swapchain_image_count_ = 2;
    bool use_swapchain_ = false;
    GPUImage depth_image_;          // Depth buffer (single-sampled when not using MSAA)

    // MSAA resources
    VkSampleCountFlagBits msaa_samples_ = VK_SAMPLE_COUNT_1_BIT;
    GPUImage msaa_color_image_;     // Multisampled color target (when MSAA enabled)
    GPUImage msaa_depth_image_;     // Multisampled depth target (when MSAA enabled)

    // Post-processing
    GPUImage taa_history_;          // Previous frame for TAA
    std::vector<GPUImage> bloom_mips_;  // Bloom downsample chain

    // Configuration
    ShadowConfig shadow_config_;
    GIConfig gi_config_;
    PostProcessConfig post_config_;

    // Indirect rendering
    culling::IndirectRenderer indirect_renderer_;
    bool indirect_rendering_enabled_ = false;
    bool indirect_renderer_initialized_ = false;

    // Text rendering
    TextRenderer text_renderer_;
    bool text_renderer_initialized_ = false;

    // Pipelines
    VkPipeline geometry_pipeline_ = VK_NULL_HANDLE;
    VkPipeline lighting_pipeline_ = VK_NULL_HANDLE;
    VkPipeline shadow_pipeline_ = VK_NULL_HANDLE;
    VkPipeline voxelization_pipeline_ = VK_NULL_HANDLE;
    VkPipeline ssr_pipeline_ = VK_NULL_HANDLE;

    // Compute pipelines
    VkPipeline voxel_gi_pipeline_ = VK_NULL_HANDLE;
    VkPipeline bloom_pipeline_ = VK_NULL_HANDLE;
    VkPipeline taa_pipeline_ = VK_NULL_HANDLE;
    VkPipeline tonemap_pipeline_ = VK_NULL_HANDLE;

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE; // single-frame fallback
    std::vector<VkFramebuffer> swapchain_framebuffers_;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline triangle_pipeline_ = VK_NULL_HANDLE;
    std::vector<GPUBuffer> frame_vertex_buffers_;
    std::vector<GPUBuffer> frame_index_buffers_;
    GPUBuffer batched_fill_vertex_buffer_;
    GPUBuffer batched_fill_index_buffer_;
    GPUBuffer batched_stroke_vertex_buffer_;
    GPUBuffer batched_stroke_index_buffer_;
    VkDeviceSize batched_fill_vertex_capacity_ = 0;
    VkDeviceSize batched_fill_index_capacity_ = 0;
    VkDeviceSize batched_stroke_vertex_capacity_ = 0;
    VkDeviceSize batched_stroke_index_capacity_ = 0;

    // Ray tracing pipelines (optional)
    VkPipeline rt_shadow_pipeline_ = VK_NULL_HANDLE;
    VkPipeline rt_gi_pipeline_ = VK_NULL_HANDLE;

    // Descriptor sets
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout gbuffer_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout lighting_layout_ = VK_NULL_HANDLE;

    // Helper functions
    enum class DrawBufferSet { FillBatch, StrokeBatch, Custom };

    struct BatchedDraw {
        DrawBufferSet buffer_set;
        uint32_t first_index;
        uint32_t index_count;
        int32_t vertex_offset;
        math::Vec4 color;
        VkBuffer custom_vertex = VK_NULL_HANDLE;
        VkBuffer custom_index = VK_NULL_HANDLE;
    };

    std::vector<BatchedDraw> batched_draws_;
    uint32_t tessellation_segments_per_curve_ = 16;

    void create_gbuffer();
    void create_shadow_maps();
    void create_voxel_grid();
    void create_render_targets();
    void create_pipelines();
    void create_render_pass();
    void create_framebuffers();
    void create_triangle_pipeline();
    void destroy_resources();
    void upload_batched_buffers(const std::vector<math::Vec3>& vertices,
                                const std::vector<uint32_t>& indices,
                                GPUBuffer& vertex_buffer,
                                GPUBuffer& index_buffer,
                                VkDeviceSize& vertex_capacity,
                                VkDeviceSize& index_capacity);

    // Shadow helpers
    void calculate_cascade_splits(
        const Camera& camera,
        const math::Vec3& light_dir
    );

    math::Mat4 calculate_light_view_proj(
        const Camera& camera,
        const math::Vec3& light_dir,
        float near_z,
        float far_z
    );
};

} // namespace manim
