/**
 * @file basic_renderer.hpp
 * @brief CPU reference renderer for simple 2D shapes.
 *
 * This renderer provides a minimal software pipeline that can draw basic
 * VMobjects (circles, ellipses, arcs, polylines) into an in-memory RGBA
 * framebuffer and write the result to disk. It is intended as a bootstrap
 * path for Week 3-4 before the GPU/Vulkan pipeline is ready.
 */

#pragma once

#include "manim/renderer/renderer.hpp"
#include <vector>
#include <string>
#include <optional>

namespace manim {

class BasicRenderer : public Renderer {
public:
    BasicRenderer();
    ~BasicRenderer() override = default;

    // Renderer interface
    void initialize(const RendererConfig& config) override;
    void shutdown() override;
    void begin_frame() override;
    void end_frame() override;
    void render_scene(Scene& scene, Camera& camera) override;
    void render_mobject(Mobject& mobject) override;
    void clear(const math::Vec4& color = math::Vec4{0, 0, 0, 1}) override;
    void resize(uint32_t width, uint32_t height) override;
    const FrameStats& get_stats() const override { return stats_; }
    void capture_frame(const std::string& output_path) override;
    RendererType get_type() const override { return RendererType::Vulkan; }
    bool supports_feature(const std::string& feature) const override;
    MemoryPool& get_memory_pool() override;
    void set_render_mode(RenderMode mode) override { render_mode_ = mode; }

    /**
     * @brief Set the world height in scene units for the orthographic view.
     *
     * A taller world height means shapes appear smaller on screen. Width is
     * derived from the current aspect ratio.
     */
    void set_world_height(float height);

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    float world_height_ = 6.0f;  // Scene units visible vertically
    math::Vec2 world_center_{0.0f, 0.0f};
    math::Vec4 clear_color_{0, 0, 0, 1};
    std::vector<uint8_t> framebuffer_;  // RGBA8

    // Rendering helpers
    struct ScreenPoint { int x; int y; };
    struct ColorRGBA { float r; float g; float b; float a; };

    float pixels_per_unit() const;
    std::optional<ScreenPoint> world_to_screen(const math::Vec3& world) const;
    ColorRGBA to_rgba(const math::Vec4& color) const;
    void blend_pixel(int x, int y, const ColorRGBA& color);
    void draw_filled_circle(const math::Vec3& center, float radius, const math::Vec4& color);
    void draw_ring(const math::Vec3& center, float inner_radius, float outer_radius, const math::Vec4& color);
    void draw_filled_ellipse(const math::Vec3& center, float rx, float ry, const math::Vec4& color);
    void draw_polyline(const std::vector<math::Vec3>& points, const math::Vec4& color, float width = 1.0f);
    void draw_line(const math::Vec3& a, const math::Vec3& b, const math::Vec4& color, float width);
};

}  // namespace manim
