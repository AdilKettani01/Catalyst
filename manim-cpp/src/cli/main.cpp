#include "manim/cli/config.hpp"
#include "manim/cli/gpu_detector.hpp"
#include "manim/renderer/basic_renderer.hpp"
#include "manim/renderer/vulkan_renderer.hpp"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/core/math.hpp"
#include "manim/scene/scene.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdio>

int main(int argc, char** argv) {
    CLI::App app{"Manim C++ - Mathematical Animation Engine"};
    app.require_subcommand(1);

    // Config
    auto& config = manim::getGlobalConfig();

    // Global options
    std::string quality_str = "high";
    std::string backend_str = "auto";
    
    app.add_option("-q,--quality", quality_str, "Quality preset (low/medium/high/ultra)");
    app.add_option("-b,--backend", backend_str, "GPU backend (auto/vulkan/opengl/dx12)");
    app.add_option("--width", config.width, "Output width");
    app.add_option("--height", config.height, "Output height");
    app.add_option("--fps", config.frameRate, "Frame rate");

    // Render command
    auto* render = app.add_subcommand("render", "Render a scene");
    std::string scene_file;
    std::string scene_name;
    bool use_gpu = false;
    int num_frames = 1;
    std::string output_dir = "media/frames";

    render->add_option("file", scene_file, "Output file or base name")->required();
    render->add_option("scene", scene_name, "Scene name");
    render->add_flag("--gpu", use_gpu, "Use GPU (Vulkan) renderer instead of CPU");
    render->add_option("--frames", num_frames, "Number of frames to render (for sequences)");
    render->add_option("--output-dir", output_dir, "Output directory for frame sequences");

    render->callback([&]() {
        // Configure renderer
        manim::RendererConfig renderer_cfg;
        renderer_cfg.width = config.width;
        renderer_cfg.height = config.height;
        renderer_cfg.vsync = false;

        // Select renderer based on --gpu flag
        std::shared_ptr<manim::Renderer> renderer;
        if (use_gpu) {
            std::cout << "Initializing VulkanRenderer (GPU)..." << std::endl;
            auto vulkan_renderer = std::make_shared<manim::VulkanRenderer>();
            try {
                vulkan_renderer->initialize(renderer_cfg);
                renderer = vulkan_renderer;
                std::cout << "GPU renderer initialized successfully." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to initialize Vulkan: " << e.what() << std::endl;
                std::cerr << "Falling back to CPU renderer." << std::endl;
                renderer = std::make_shared<manim::BasicRenderer>();
                renderer->initialize(renderer_cfg);
            }
        } else {
            renderer = std::make_shared<manim::BasicRenderer>();
            renderer->initialize(renderer_cfg);
        }

        // Create scene
        manim::Scene scene(renderer);

        // Create demo shapes
        auto circle = std::make_shared<manim::Circle>(1.0f);
        circle->set_fill(manim::math::Vec4{0.2f, 0.6f, 1.0f, 1.0f}, 0.9f);
        circle->set_stroke(manim::math::Vec4{0.05f, 0.2f, 0.4f, 1.0f}, 3.0f);
        circle->shift(manim::math::Vec3(-1.2f, 0.0f, 0.0f));

        auto ellipse = std::make_shared<manim::Ellipse>(2.2f, 1.1f);
        ellipse->set_fill(manim::math::Vec4{0.9f, 0.4f, 0.3f, 1.0f}, 0.85f);
        ellipse->set_stroke(manim::math::Vec4{0.35f, 0.08f, 0.02f, 1.0f}, 2.5f);
        ellipse->shift(manim::math::Vec3(1.1f, 0.2f, 0.0f));

        auto dot = std::make_shared<manim::Dot>(manim::math::Vec3(0.0f, -0.8f, 0.0f), 0.15f);
        dot->set_fill(manim::math::Vec4{0.95f, 0.95f, 0.95f, 1.0f}, 1.0f);

        scene.add(circle);
        scene.add(ellipse);
        scene.add(dot);

        // Render frames
        if (num_frames > 1) {
            // Frame sequence mode
            std::filesystem::create_directories(output_dir);
            std::cout << "Rendering " << num_frames << " frames to " << output_dir << "..." << std::endl;

            for (int i = 0; i < num_frames; i++) {
                scene.render(false);

                char filename[512];
                std::snprintf(filename, sizeof(filename), "%s/frame_%04d.ppm", output_dir.c_str(), i + 1);
                renderer->capture_frame(filename);

                if ((i + 1) % 10 == 0 || i == num_frames - 1) {
                    std::cout << "  Frame " << (i + 1) << "/" << num_frames << std::endl;
                }
            }

            std::cout << "Rendered " << num_frames << " frames to " << output_dir << std::endl;
            std::cout << "\nConvert to video with:" << std::endl;
            std::cout << "  ffmpeg -framerate " << config.frameRate << " -i " << output_dir
                      << "/frame_%04d.ppm -c:v libx264 -pix_fmt yuv420p output.mp4" << std::endl;
        } else {
            // Single frame mode
            scene.render(false);

            std::string output_path = scene_file.empty() ? "output.ppm" : scene_file;
            if (output_path.find('.') == std::string::npos) {
                output_path += ".ppm";
            }

            renderer->capture_frame(output_path);
            std::cout << "Rendered to " << output_path;
            if (use_gpu) {
                std::cout << " (GPU)";
            }
            std::cout << std::endl;
        }
    });

    // Checkhealth command
    auto* checkhealth = app.add_subcommand("checkhealth", "Check system health");
    bool full_check = false;
    bool benchmark = false;
    checkhealth->add_flag("--full", full_check, "Full health check");
    checkhealth->add_flag("--benchmark", benchmark, "Run benchmarks");
    checkhealth->callback([&]() {
        auto& detector = manim::getGPUDetector();
        detector.detectGPUs();
        detector.printSystemReport();
        
        if (benchmark) {
            detector.fullBenchmark();
            detector.printBenchmarkResults();
        }
        
        auto health = manim::runHealthCheck();
        std::cout << "\nHealth Check: " << (health.passed ? "PASSED" : "FAILED") << std::endl;
    });

    // Init command
    auto* init = app.add_subcommand("init", "Initialize new project");
    std::string project_name;
    init->add_option("name", project_name, "Project name")->required();
    init->callback([&]() {
        std::cout << "Initializing project: " << project_name << std::endl;
    });

    // Config command
    auto* cfg = app.add_subcommand("cfg", "Manage configuration");
    cfg->add_subcommand("show", "Show current configuration")->callback([&]() {
        std::cout << config.toString() << std::endl;
    });
    cfg->add_subcommand("reset", "Reset to defaults")->callback([&]() {
        config = manim::RenderConfig();
        std::cout << "Configuration reset to defaults" << std::endl;
    });

    CLI11_PARSE(app, argc, argv);

    return 0;
}
