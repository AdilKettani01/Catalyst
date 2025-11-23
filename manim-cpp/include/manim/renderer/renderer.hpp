/**
 * @file renderer.hpp
 * @brief Base renderer interface for all rendering backends
 *
 * Provides abstract interface that all renderer implementations must follow.
 * Supports Vulkan, OpenGL, DirectX 12, and Metal backends.
 */

#pragma once

#include <manim/core/memory_pool.hpp>
#include <manim/core/math.hpp>
#include <memory>
#include <vector>
#include <string>

namespace manim {

// Forward declarations
class Mobject;
class Scene;
class Camera;

/**
 * @brief Rendering backend type
 */
enum class RendererType {
    Vulkan,      ///< Vulkan 1.3+ (primary, all platforms)
    OpenGL,      ///< OpenGL 4.6 (fallback)
    DirectX12,   ///< DirectX 12 (Windows only)
    Metal,       ///< Metal (macOS/iOS only)
    Skia         ///< Skia GPU backend (2D vector graphics)
};

/**
 * @brief Rendering configuration
 */
struct RendererConfig {
    // Display
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    bool fullscreen = false;
    bool vsync = true;

    // Quality
    uint32_t msaa_samples = 4;  ///< Multisample anti-aliasing
    bool enable_hdr = true;
    bool enable_bloom = true;
    bool enable_taa = true;      ///< Temporal anti-aliasing

    // Advanced rendering
    bool enable_ray_tracing = false;
    bool enable_global_illumination = false;
    bool enable_deferred_rendering = true;

    // Performance
    bool enable_frustum_culling = true;
    bool enable_occlusion_culling = true;
    uint32_t max_lights = 16;

    // Debug
    bool debug_wireframe = false;
    bool debug_normals = false;
    bool show_fps = true;
};

/**
 * @brief Frame timing statistics
 */
struct FrameStats {
    float fps = 0.0f;
    float frame_time_ms = 0.0f;
    float cpu_time_ms = 0.0f;
    float gpu_time_ms = 0.0f;

    uint32_t draw_calls = 0;
    uint32_t triangles = 0;
    uint32_t vertices = 0;

    uint64_t gpu_memory_used = 0;
    uint64_t cpu_memory_used = 0;
};

/**
 * @brief Abstract base renderer class
 *
 * All rendering backends (Vulkan, OpenGL, etc.) inherit from this.
 */
class Renderer {
public:
    virtual ~Renderer() = default;

    /**
     * @brief Initialize renderer
     */
    virtual void initialize(const RendererConfig& config) = 0;

    /**
     * @brief Shutdown and cleanup
     */
    virtual void shutdown() = 0;

    /**
     * @brief Begin new frame
     */
    virtual void begin_frame() = 0;

    /**
     * @brief End frame and present
     */
    virtual void end_frame() = 0;

    /**
     * @brief Render scene
     */
    virtual void render_scene(Scene& scene, Camera& camera) = 0;

    /**
     * @brief Render single mobject
     */
    virtual void render_mobject(Mobject& mobject) = 0;

    /**
     * @brief Clear screen
     */
    virtual void clear(const math::Vec4& color = math::Vec4{0, 0, 0, 1}) = 0;

    /**
     * @brief Resize viewport
     */
    virtual void resize(uint32_t width, uint32_t height) = 0;

    /**
     * @brief Get frame statistics
     */
    virtual const FrameStats& get_stats() const = 0;

    /**
     * @brief Capture current frame to image
     */
    virtual void capture_frame(const std::string& output_path) = 0;

    /**
     * @brief Get renderer type
     */
    virtual RendererType get_type() const = 0;

    /**
     * @brief Check if feature is supported
     */
    virtual bool supports_feature(const std::string& feature) const = 0;

    /**
     * @brief Get memory pool
     */
    virtual MemoryPool& get_memory_pool() = 0;

    /**
     * @brief Set rendering mode
     */
    enum class RenderMode {
        Normal,        ///< Standard rendering
        Wireframe,     ///< Wireframe only
        Normals,       ///< Visualize normals
        Depth,         ///< Depth visualization
        Lighting,      ///< Lighting only
        Shadows,       ///< Shadow maps
        GBuffer        ///< G-buffer visualization
    };

    virtual void set_render_mode(RenderMode mode) = 0;

protected:
    RendererConfig config_;
    FrameStats stats_;
    std::unique_ptr<MemoryPool> memory_pool_;
};

/**
 * @brief Create renderer based on type
 *
 * Factory function that creates the appropriate renderer implementation.
 */
std::unique_ptr<Renderer> create_renderer(
    RendererType type,
    const RendererConfig& config = {}
);

/**
 * @brief Detect best available renderer for current platform
 */
RendererType detect_best_renderer();

} // namespace manim
