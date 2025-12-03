/**
 * @file text_fade_animation.cpp
 * @brief Text fade animation example with video output
 *
 * Renders TEXT1, TEXT2, TEXT3 with fade in/out animations to video.
 *
 * Usage:
 *   ./bin/text_fade_animation [--gpu]
 *
 * Output:
 *   /tmp/manim_frames/text_animation.mp4
 */

#include "manim/renderer/basic_renderer.hpp"
#include "manim/renderer/vulkan_renderer.hpp"
#include "manim/mobject/text/text.hpp"
#include "manim/scene/scene.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Simple frame-by-frame video renderer
class VideoRenderer {
public:
    VideoRenderer(std::shared_ptr<manim::Renderer> renderer, int fps = 30)
        : renderer_(renderer), fps_(fps), frame_count_(0) {
        // Create frames directory
        frames_dir_ = "/tmp/manim_frames/text_anim_frames";
        fs::create_directories(frames_dir_);
    }

    void capture_frame() {
        std::ostringstream filename;
        filename << frames_dir_ << "/frame_" << std::setfill('0') << std::setw(5) << frame_count_ << ".ppm";
        renderer_->capture_frame(filename.str());
        frame_count_++;
    }

    void render_wait(double seconds) {
        int frames = static_cast<int>(seconds * fps_);
        for (int i = 0; i < frames; i++) {
            capture_frame();
        }
    }

    void render_fade_in(std::shared_ptr<manim::Text> text, double duration = 1.0) {
        int frames = static_cast<int>(duration * fps_);
        for (int i = 0; i <= frames; i++) {
            float alpha = static_cast<float>(i) / frames;
            text->set_opacity(alpha);
            renderer_->clear();
            renderer_->render_mobject(*text);
            capture_frame();
        }
    }

    void render_fade_out(std::shared_ptr<manim::Text> text, double duration = 1.0) {
        int frames = static_cast<int>(duration * fps_);
        for (int i = 0; i <= frames; i++) {
            float alpha = 1.0f - static_cast<float>(i) / frames;
            text->set_opacity(alpha);
            renderer_->clear();
            renderer_->render_mobject(*text);
            capture_frame();
        }
    }

    void render_static(std::shared_ptr<manim::Text> text, double seconds) {
        int frames = static_cast<int>(seconds * fps_);
        for (int i = 0; i < frames; i++) {
            renderer_->clear();
            renderer_->render_mobject(*text);
            capture_frame();
        }
    }

    bool encode_video(const std::string& output_path) {
        std::ostringstream cmd;
        cmd << "ffmpeg -y -framerate " << fps_
            << " -i " << frames_dir_ << "/frame_%05d.ppm"
            << " -c:v libx264 -pix_fmt yuv420p -crf 18 "
            << output_path << " 2>/dev/null";

        int result = std::system(cmd.str().c_str());

        // Clean up frames
        fs::remove_all(frames_dir_);

        return result == 0;
    }

    int get_frame_count() const { return frame_count_; }

private:
    std::shared_ptr<manim::Renderer> renderer_;
    std::string frames_dir_;
    int fps_;
    int frame_count_;
};

int main(int argc, char** argv) {
    bool use_gpu = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--gpu") == 0) {
            use_gpu = true;
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
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize Vulkan: " << e.what() << std::endl;
        }
    }

    if (!renderer) {
        std::cout << "Initializing BasicRenderer (CPU mode)..." << std::endl;
        auto basic_renderer = std::make_shared<manim::BasicRenderer>();
        basic_renderer->initialize(cfg);
        renderer = basic_renderer;
    }

    // Create video renderer
    VideoRenderer video(renderer, 30);  // 30 FPS

    std::cout << "Creating text fade animation..." << std::endl;

    // Create text objects
    auto text1 = std::make_shared<manim::Text>("TEXT1", 120.0f);
    auto text2 = std::make_shared<manim::Text>("TEXT2", 120.0f);
    auto text3 = std::make_shared<manim::Text>("TEXT3", 120.0f);

    // Set white color
    text1->set_color(manim::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
    text2->set_color(manim::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
    text3->set_color(manim::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});

    // TEXT1 sequence
    std::cout << "Rendering TEXT1..." << std::endl;
    video.render_fade_in(text1, 1.0);
    video.render_static(text1, 4.0);
    video.render_fade_out(text1, 1.0);

    // TEXT2 sequence
    std::cout << "Rendering TEXT2..." << std::endl;
    video.render_fade_in(text2, 1.0);
    video.render_static(text2, 4.0);
    video.render_fade_out(text2, 1.0);

    // TEXT3 sequence
    std::cout << "Rendering TEXT3..." << std::endl;
    video.render_fade_in(text3, 1.0);
    video.render_static(text3, 4.0);
    video.render_fade_out(text3, 1.0);

    std::cout << "Rendered " << video.get_frame_count() << " frames" << std::endl;

    // Encode to video
    std::string output = "/tmp/manim_frames/text_animation.mp4";
    std::cout << "Encoding video..." << std::endl;

    if (video.encode_video(output)) {
        std::cout << "\nVideo saved to: " << output << std::endl;
        std::cout << "Play with: ffplay " << output << std::endl;
        std::cout << "Or: xdg-open " << output << std::endl;
    } else {
        std::cerr << "Failed to encode video. Is ffmpeg installed?" << std::endl;
        return 1;
    }

    return 0;
}
