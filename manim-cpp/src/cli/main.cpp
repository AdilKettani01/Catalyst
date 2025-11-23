#include "manim/cli/config.h"
#include "manim/cli/gpu_detector.h"
#include <CLI/CLI.hpp>
#include <iostream>

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
    render->add_option("file", scene_file, "Scene file")->required();
    render->add_option("scene", scene_name, "Scene name");
    render->callback([&]() {
        std::cout << "Rendering " << scene_file;
        if (!scene_name.empty()) std::cout << " scene: " << scene_name;
        std::cout << std::endl;
        std::cout << config.toString() << std::endl;
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
