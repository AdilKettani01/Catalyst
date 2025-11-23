/**
 * @file vulkan_renderer.hpp
 * @brief Vulkan rendering backend implementation
 */

#pragma once

#include "renderer.hpp"
#include <vulkan/vulkan.h>

namespace manim {

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override = default;

    void initialize(const RendererConfig& config) override {}
    void shutdown() override {}
    void begin_frame() override {}
    void end_frame() override {}
    void render_scene(Scene& scene, Camera& camera) override {}
    void render_mobject(Mobject& mobject) override {}
    void clear(const math::Vec4& color = math::Vec4{0, 0, 0, 1}) override {}
    void resize(uint32_t width, uint32_t height) override {}
    const FrameStats& get_stats() const override { return stats_; }
    void capture_frame(const std::string& output_path) override {}
    RendererType get_type() const override { return RendererType::Vulkan; }
    bool supports_feature(const std::string& feature) const override { return false; }
    MemoryPool& get_memory_pool() override { return *memory_pool_; }
    void set_render_mode(RenderMode mode) override {}
};

} // namespace manim
