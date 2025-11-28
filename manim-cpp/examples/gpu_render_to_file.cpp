/**
 * @file gpu_render_to_file.cpp
 * @brief Minimal example demonstrating render-to-file functionality
 *
 * This example renders a simple scene with geometric shapes and saves
 * the result to a PPM file.
 *
 * Usage:
 *   ./gpu_render_to_file [output.ppm] [--gpu]
 *
 * Options:
 *   --gpu  Use VulkanRenderer (GPU) instead of BasicRenderer (CPU)
 *
 * Environment:
 *   MANIM_WINDOWED=1  - Show window during rendering (optional)
 *   VK_ICD_FILENAMES - Set to your GPU ICD if Vulkan selects wrong device
 *
 * Note: The GPU rendering path (VulkanRenderer + GPU3DRenderer) is still
 * being developed. Use CPU rendering (BasicRenderer) for reliable output.
 */

#include "manim/renderer/basic_renderer.hpp"
#include "manim/renderer/vulkan_renderer.hpp"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/scene/scene.hpp"
#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char** argv) {
    std::string output_path = "output.ppm";
    bool use_gpu = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--gpu") == 0) {
            use_gpu = true;
        } else if (argv[i][0] != '-') {
            output_path = argv[i];
            if (output_path.find('.') == std::string::npos) {
                output_path += ".ppm";
            }
        }
    }

    // Configure renderer
    manim::RendererConfig cfg;
    cfg.width = 1920;
    cfg.height = 1080;
    cfg.vsync = false;

    std::shared_ptr<manim::Renderer> renderer;

    if (use_gpu) {
        std::cout << "Initializing VulkanRenderer (GPU mode)..." << std::endl;
        auto vulkan_renderer = std::make_shared<manim::VulkanRenderer>();
        try {
            vulkan_renderer->initialize(cfg);
            if (vulkan_renderer->is_gpu_ready()) {
                renderer = vulkan_renderer;
                std::cout << "GPU renderer ready." << std::endl;
            } else {
                std::cerr << "GPU not ready, falling back to CPU." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize Vulkan: " << e.what() << std::endl;
            std::cerr << "Falling back to CPU renderer." << std::endl;
        }
    }

    if (!renderer) {
        std::cout << "Initializing BasicRenderer (CPU mode)..." << std::endl;
        auto basic_renderer = std::make_shared<manim::BasicRenderer>();
        basic_renderer->initialize(cfg);
        renderer = basic_renderer;
    }

    // Create scene with renderer
    manim::Scene scene(renderer);

    // Create a blue circle on the left
    auto circle = std::make_shared<manim::Circle>(1.0f);
    circle->set_fill(manim::math::Vec4{0.2f, 0.6f, 1.0f, 1.0f}, 0.9f);
    circle->set_stroke(manim::math::Vec4{0.05f, 0.2f, 0.4f, 1.0f}, 3.0f);
    circle->shift(manim::math::Vec3(-1.5f, 0.0f, 0.0f));

    // Create a red ellipse on the right
    auto ellipse = std::make_shared<manim::Ellipse>(2.2f, 1.1f);
    ellipse->set_fill(manim::math::Vec4{0.9f, 0.3f, 0.3f, 1.0f}, 0.85f);
    ellipse->set_stroke(manim::math::Vec4{0.4f, 0.1f, 0.1f, 1.0f}, 2.5f);
    ellipse->shift(manim::math::Vec3(1.5f, 0.0f, 0.0f));

    // Create a white dot at the center
    auto dot = std::make_shared<manim::Dot>(manim::math::Vec3(0.0f, 0.0f, 0.0f), 0.12f);
    dot->set_fill(manim::math::Vec4{0.95f, 0.95f, 0.95f, 1.0f}, 1.0f);

    // Add shapes to scene
    scene.add(circle);
    scene.add(ellipse);
    scene.add(dot);

    std::cout << "Rendering scene..." << std::endl;

    // Render the scene
    scene.render(false);  // false = no interactive preview

    // Capture frame to file
    renderer->capture_frame(output_path);

    std::cout << "Rendered to: " << output_path << std::endl;
    std::cout << "Resolution: " << cfg.width << "x" << cfg.height << std::endl;
    std::cout << "\nTo view: display " << output_path << " (ImageMagick)" << std::endl;
    std::cout << "To convert: ffmpeg -i " << output_path << " output.png" << std::endl;

    return 0;
}
