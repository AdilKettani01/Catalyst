// Base renderer implementation and factory
#include "manim/renderer/renderer.hpp"
#include "manim/renderer/vulkan_renderer.hpp"
#include "manim/renderer/opengl_renderer.hpp"

#ifdef _WIN32
    #include "manim/renderer/directx12_renderer.hpp"
#endif

#ifdef __APPLE__
    #include "manim/renderer/metal_renderer.hpp"
#endif

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace manim {

// ============================================================================
// Renderer Base Implementation
// ============================================================================

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::set_render_mode(RenderMode mode) {
    render_mode_ = mode;
    spdlog::info("Render mode changed to: {}", static_cast<int>(mode));
}

void Renderer::toggle_wireframe() {
    wireframe_enabled_ = !wireframe_enabled_;
    spdlog::info("Wireframe {}", wireframe_enabled_ ? "enabled" : "disabled");
}

void Renderer::set_vsync(bool enabled) {
    vsync_enabled_ = enabled;
    spdlog::info("VSync {}", enabled ? "enabled" : "disabled");
}

bool Renderer::supports_ray_tracing() const {
    return false;  // Override in derived classes
}

bool Renderer::supports_mesh_shaders() const {
    return false;  // Override in derived classes
}

FrameStats Renderer::get_frame_stats() const {
    return frame_stats_;
}

void Renderer::begin_frame_timing() {
    frame_timer_start_ = std::chrono::high_resolution_clock::now();
}

void Renderer::end_frame_timing() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - frame_timer_start_);

    frame_stats_.frame_time_ms = duration.count() / 1000.0f;
    frame_stats_.fps = 1000.0f / frame_stats_.frame_time_ms;

    // Update frame counter
    frame_stats_.frame_number++;
}

// ============================================================================
// Renderer Factory
// ============================================================================

std::unique_ptr<Renderer> create_renderer(RendererType type, const RendererConfig& config) {
    spdlog::info("Creating renderer of type: {}", static_cast<int>(type));

    switch (type) {
        case RendererType::Vulkan:
            #ifdef MANIM_ENABLE_VULKAN
                return std::make_unique<VulkanRenderer>();
            #else
                throw std::runtime_error("Vulkan support not compiled in");
            #endif

        case RendererType::OpenGL:
            #ifdef MANIM_ENABLE_OPENGL
                return std::make_unique<OpenGLRenderer>();
            #else
                throw std::runtime_error("OpenGL support not compiled in");
            #endif

        case RendererType::DirectX12:
            #ifdef MANIM_ENABLE_DIRECTX12
                return std::make_unique<DirectX12Renderer>();
            #else
                throw std::runtime_error("DirectX 12 support not compiled in");
            #endif

        case RendererType::Metal:
            #ifdef MANIM_ENABLE_METAL
                return std::make_unique<MetalRenderer>();
            #else
                throw std::runtime_error("Metal support not compiled in");
            #endif

        case RendererType::Skia:
            throw std::runtime_error("Skia renderer not yet implemented");

        default:
            throw std::runtime_error("Unknown renderer type");
    }
}

std::unique_ptr<Renderer> create_best_available_renderer(const RendererConfig& config) {
    // Try renderers in order of preference
    std::vector<RendererType> preferences;

#ifdef _WIN32
    preferences = {
        RendererType::Vulkan,
        RendererType::DirectX12,
        RendererType::OpenGL
    };
#elif defined(__APPLE__)
    preferences = {
        RendererType::Metal,
        RendererType::Vulkan,
        RendererType::OpenGL
    };
#else  // Linux
    preferences = {
        RendererType::Vulkan,
        RendererType::OpenGL
    };
#endif

    for (auto type : preferences) {
        try {
            auto renderer = create_renderer(type, config);
            renderer->initialize(config);
            spdlog::info("Successfully created {} renderer", static_cast<int>(type));
            return renderer;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to create renderer: {}", e.what());
            continue;
        }
    }

    throw std::runtime_error("Failed to create any renderer");
}

std::vector<RendererType> get_available_renderers() {
    std::vector<RendererType> available;

#ifdef MANIM_ENABLE_VULKAN
    available.push_back(RendererType::Vulkan);
#endif

#ifdef MANIM_ENABLE_OPENGL
    available.push_back(RendererType::OpenGL);
#endif

#ifdef MANIM_ENABLE_DIRECTX12
    available.push_back(RendererType::DirectX12);
#endif

#ifdef MANIM_ENABLE_METAL
    available.push_back(RendererType::Metal);
#endif

    return available;
}

std::string renderer_type_to_string(RendererType type) {
    switch (type) {
        case RendererType::Vulkan:    return "Vulkan";
        case RendererType::OpenGL:    return "OpenGL";
        case RendererType::DirectX12: return "DirectX 12";
        case RendererType::Metal:     return "Metal";
        case RendererType::Skia:      return "Skia";
        default:                      return "Unknown";
    }
}

// ============================================================================
// RendererConfig Default Values
// ============================================================================

RendererConfig RendererConfig::default_config() {
    RendererConfig config;
    config.width = 1920;
    config.height = 1080;
    config.msaa_samples = 4;
    config.vsync = true;
    config.enable_validation = false;
    config.enable_ray_tracing = false;
    config.enable_mesh_shaders = false;
    config.max_frames_in_flight = 2;
    return config;
}

RendererConfig RendererConfig::high_quality_config() {
    RendererConfig config;
    config.width = 3840;
    config.height = 2160;
    config.msaa_samples = 8;
    config.vsync = true;
    config.enable_validation = false;
    config.enable_ray_tracing = true;
    config.enable_mesh_shaders = true;
    config.max_frames_in_flight = 3;
    return config;
}

RendererConfig RendererConfig::performance_config() {
    RendererConfig config;
    config.width = 1920;
    config.height = 1080;
    config.msaa_samples = 1;
    config.vsync = false;
    config.enable_validation = false;
    config.enable_ray_tracing = false;
    config.enable_mesh_shaders = false;
    config.max_frames_in_flight = 2;
    return config;
}

}  // namespace manim
