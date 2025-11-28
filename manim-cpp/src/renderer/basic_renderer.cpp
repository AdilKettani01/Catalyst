// Minimal CPU renderer for basic shapes
#include "manim/renderer/basic_renderer.hpp"

#include "manim/mobject/geometry/circle.hpp"
#include "manim/mobject/vmobject.hpp"
#include "manim/scene/camera.hpp"
#include "manim/scene/scene.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace manim {

BasicRenderer::BasicRenderer() {
    memory_pool_ = std::make_unique<MemoryPool>();
}

void BasicRenderer::initialize(const RendererConfig& config) {
    config_ = config;
    width_ = config.width;
    height_ = config.height;
    framebuffer_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4, 0);
    clear_color_ = math::Vec4{0, 0, 0, 1};
    clear(clear_color_);
    stats_ = {};
    stats_.frame_number = 0;

    spdlog::info("BasicRenderer initialized ({}x{})", width_, height_);
}

void BasicRenderer::shutdown() {
    framebuffer_.clear();
    framebuffer_.shrink_to_fit();
}

void BasicRenderer::begin_frame() {
    begin_frame_timing();
    clear(clear_color_);
    stats_.draw_calls = 0;
}

void BasicRenderer::end_frame() {
    end_frame_timing();
}

void BasicRenderer::render_scene(Scene& scene, Camera& /*camera*/) {
    // Draw all mobjects (including submobjects)
    auto all_mobjects = scene.getMobjectFamilyMembers();
    for (auto& mob : all_mobjects) {
        if (mob) {
            render_mobject(*mob);
        }
    }

    stats_.frame_number++;
}

void BasicRenderer::render_mobject(Mobject& mobject) {
    // Handle known geometry types first for proper fill rendering
    if (auto* circle = dynamic_cast<Circle*>(&mobject)) {
        const auto center = circle->get_center();
        const float radius = circle->get_radius();

        const float fill_alpha = circle->get_fill_opacity() * circle->get_fill_color().w;
        if (fill_alpha > 0.0f) {
            auto fill_color = circle->get_fill_color();
            fill_color.w = fill_alpha;
            draw_filled_circle(center, radius, fill_color);
        }

        const float stroke_alpha = circle->get_stroke_opacity() * circle->get_stroke_color().w;
        if (stroke_alpha > 0.0f && circle->get_stroke_width() > 0.0f) {
            float ppu = pixels_per_unit();
            float half_width_world = (circle->get_stroke_width() * 0.5f) / std::max(ppu, 1.0f);
            auto stroke_color = circle->get_stroke_color();
            stroke_color.w = stroke_alpha;
            draw_ring(center, std::max(0.0f, radius - half_width_world), radius + half_width_world, stroke_color);
        }

        stats_.draw_calls++;
        return;
    }

    if (auto* ellipse = dynamic_cast<Ellipse*>(&mobject)) {
        auto [min_corner, max_corner] = ellipse->get_bounding_box();
        math::Vec3 center = (min_corner + max_corner) * 0.5f;
        math::Vec3 size = max_corner - min_corner;
        float rx = size.x * 0.5f;
        float ry = size.y * 0.5f;

        const float fill_alpha = ellipse->get_fill_opacity() * ellipse->get_fill_color().w;
        if (fill_alpha > 0.0f) {
            auto fill_color = ellipse->get_fill_color();
            fill_color.w = fill_alpha;
            draw_filled_ellipse(center, rx, ry, fill_color);
        }

        const float stroke_alpha = ellipse->get_stroke_opacity() * ellipse->get_stroke_color().w;
        if (stroke_alpha > 0.0f && ellipse->get_stroke_width() > 0.0f) {
            float ppu = pixels_per_unit();
            float half_width_world = (ellipse->get_stroke_width() * 0.5f) / std::max(ppu, 1.0f);
            auto stroke_color = ellipse->get_stroke_color();
            stroke_color.w = stroke_alpha;
            draw_ring(center, std::max(0.0f, std::min(rx, ry) - half_width_world),
                      std::max(rx, ry) + half_width_world, stroke_color);
        }

        stats_.draw_calls++;
        return;
    }

    if (auto* annulus = dynamic_cast<Annulus*>(&mobject)) {
        auto color = annulus->get_fill_color();
        color.w = annulus->get_fill_opacity() * color.w;
        draw_ring(annulus->get_center(), annulus->get_inner_radius(), annulus->get_outer_radius(), color);
        stats_.draw_calls++;
        return;
    }

    // Generic VMobject: draw anchors as a polyline
    if (auto* vmobject = dynamic_cast<VMobject*>(&mobject)) {
        auto anchors = vmobject->get_anchors();
        if (anchors.size() >= 2) {
            auto stroke_color = vmobject->get_stroke_color();
            stroke_color.w *= vmobject->get_stroke_opacity();
            draw_polyline(anchors, stroke_color, vmobject->get_stroke_width());
            stats_.draw_calls++;
        }
        return;
    }

    // Fallback: treat bounding box as a small point
    auto bbox = mobject.get_bounding_box();
    math::Vec3 center = (bbox.first + bbox.second) * 0.5f;
    draw_filled_circle(center, 0.02f, mobject.get_color());
    stats_.draw_calls++;
}

void BasicRenderer::clear(const math::Vec4& color) {
    clear_color_ = color;
    auto rgba = to_rgba(clear_color_);

    for (size_t i = 0; i + 3 < framebuffer_.size(); i += 4) {
        framebuffer_[i] = static_cast<uint8_t>(std::clamp(rgba.r, 0.0f, 1.0f) * 255.0f);
        framebuffer_[i + 1] = static_cast<uint8_t>(std::clamp(rgba.g, 0.0f, 1.0f) * 255.0f);
        framebuffer_[i + 2] = static_cast<uint8_t>(std::clamp(rgba.b, 0.0f, 1.0f) * 255.0f);
        framebuffer_[i + 3] = 255;
    }
}

void BasicRenderer::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    framebuffer_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4, 0);
}

void BasicRenderer::capture_frame(const std::string& output_path) {
    if (framebuffer_.empty() || width_ == 0 || height_ == 0) {
        spdlog::warn("No framebuffer to capture");
        return;
    }

    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        spdlog::error("Failed to open {} for writing", output_path);
        return;
    }

    out << "P6\n" << width_ << " " << height_ << "\n255\n";
    for (uint32_t y = 0; y < height_; ++y) {
        for (uint32_t x = 0; x < width_; ++x) {
            size_t idx = (static_cast<size_t>(y) * width_ + x) * 4;
            out.put(static_cast<char>(framebuffer_[idx]));
            out.put(static_cast<char>(framebuffer_[idx + 1]));
            out.put(static_cast<char>(framebuffer_[idx + 2]));
        }
    }

    spdlog::info("Captured frame to {}", output_path);
}

bool BasicRenderer::supports_feature(const std::string& feature) const {
    return feature == "cpu-basic" || feature == "basic-shapes";
}

MemoryPool& BasicRenderer::get_memory_pool() {
    return *memory_pool_;
}

void BasicRenderer::set_world_height(float height) {
    world_height_ = std::max(0.1f, height);
}

float BasicRenderer::pixels_per_unit() const {
    if (world_height_ <= 0.0f || height_ == 0) {
        return 1.0f;
    }
    return static_cast<float>(height_) / world_height_;
}

std::optional<BasicRenderer::ScreenPoint> BasicRenderer::world_to_screen(const math::Vec3& world) const {
    if (width_ == 0 || height_ == 0) {
        return std::nullopt;
    }

    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float world_width = world_height_ * aspect;

    float x_ndc = (world.x - world_center_.x) / (world_width * 0.5f);
    float y_ndc = (world.y - world_center_.y) / (world_height_ * 0.5f);

    float x = (x_ndc * 0.5f + 0.5f) * static_cast<float>(width_ - 1);
    float y = (1.0f - (y_ndc * 0.5f + 0.5f)) * static_cast<float>(height_ - 1);

    int ix = static_cast<int>(std::round(x));
    int iy = static_cast<int>(std::round(y));
    ix = std::clamp(ix, 0, static_cast<int>(width_) - 1);
    iy = std::clamp(iy, 0, static_cast<int>(height_) - 1);

    return ScreenPoint{ix, iy};
}

BasicRenderer::ColorRGBA BasicRenderer::to_rgba(const math::Vec4& color) const {
    return {
        std::clamp(color.x, 0.0f, 1.0f),
        std::clamp(color.y, 0.0f, 1.0f),
        std::clamp(color.z, 0.0f, 1.0f),
        std::clamp(color.w, 0.0f, 1.0f)
    };
}

void BasicRenderer::blend_pixel(int x, int y, const ColorRGBA& color) {
    if (x < 0 || y < 0 || x >= static_cast<int>(width_) || y >= static_cast<int>(height_)) {
        return;
    }

    size_t idx = (static_cast<size_t>(y) * width_ + static_cast<size_t>(x)) * 4;
    float dst_r = framebuffer_[idx] / 255.0f;
    float dst_g = framebuffer_[idx + 1] / 255.0f;
    float dst_b = framebuffer_[idx + 2] / 255.0f;

    float src_a = color.a;
    float inv_a = 1.0f - src_a;

    float out_r = color.r * src_a + dst_r * inv_a;
    float out_g = color.g * src_a + dst_g * inv_a;
    float out_b = color.b * src_a + dst_b * inv_a;

    framebuffer_[idx] = static_cast<uint8_t>(std::clamp(out_r, 0.0f, 1.0f) * 255.0f);
    framebuffer_[idx + 1] = static_cast<uint8_t>(std::clamp(out_g, 0.0f, 1.0f) * 255.0f);
    framebuffer_[idx + 2] = static_cast<uint8_t>(std::clamp(out_b, 0.0f, 1.0f) * 255.0f);
    framebuffer_[idx + 3] = 255;
}

void BasicRenderer::draw_filled_circle(const math::Vec3& center, float radius, const math::Vec4& color) {
    auto center_px = world_to_screen(center);
    if (!center_px) {
        return;
    }

    float radius_px = radius * pixels_per_unit();
    int r = static_cast<int>(std::ceil(radius_px));
    auto rgba = to_rgba(color);

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            float dist2 = static_cast<float>(dx * dx + dy * dy);
            if (dist2 <= radius_px * radius_px) {
                blend_pixel(center_px->x + dx, center_px->y + dy, rgba);
            }
        }
    }
}

void BasicRenderer::draw_ring(const math::Vec3& center, float inner_radius, float outer_radius, const math::Vec4& color) {
    auto center_px = world_to_screen(center);
    if (!center_px) {
        return;
    }

    float ppu = pixels_per_unit();
    float inner_px = std::max(0.0f, inner_radius * ppu);
    float outer_px = std::max(inner_px, outer_radius * ppu);

    int r = static_cast<int>(std::ceil(outer_px));
    float inner2 = inner_px * inner_px;
    float outer2 = outer_px * outer_px;
    auto rgba = to_rgba(color);

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            float dist2 = static_cast<float>(dx * dx + dy * dy);
            if (dist2 <= outer2 && dist2 >= inner2) {
                blend_pixel(center_px->x + dx, center_px->y + dy, rgba);
            }
        }
    }
}

void BasicRenderer::draw_filled_ellipse(const math::Vec3& center, float rx, float ry, const math::Vec4& color) {
    auto center_px = world_to_screen(center);
    if (!center_px) {
        return;
    }

    float ppu = pixels_per_unit();
    float rx_px = rx * ppu;
    float ry_px = ry * ppu;

    int max_r = static_cast<int>(std::ceil(std::max(rx_px, ry_px)));
    auto rgba = to_rgba(color);

    float inv_rx2 = 1.0f / (rx_px * rx_px + 1e-6f);
    float inv_ry2 = 1.0f / (ry_px * ry_px + 1e-6f);

    for (int dy = -max_r; dy <= max_r; ++dy) {
        for (int dx = -max_r; dx <= max_r; ++dx) {
            float nx = static_cast<float>(dx);
            float ny = static_cast<float>(dy);
            float equation = (nx * nx) * inv_rx2 + (ny * ny) * inv_ry2;
            if (equation <= 1.0f) {
                blend_pixel(center_px->x + dx, center_px->y + dy, rgba);
            }
        }
    }
}

void BasicRenderer::draw_polyline(const std::vector<math::Vec3>& points, const math::Vec4& color, float width) {
    if (points.size() < 2) {
        return;
    }

    for (size_t i = 0; i + 1 < points.size(); ++i) {
        draw_line(points[i], points[i + 1], color, width);
    }
}

void BasicRenderer::draw_line(const math::Vec3& a, const math::Vec3& b, const math::Vec4& color, float width) {
    auto pa = world_to_screen(a);
    auto pb = world_to_screen(b);
    if (!pa || !pb) {
        return;
    }

    auto rgba = to_rgba(color);
    int dx = std::abs(pb->x - pa->x);
    int dy = std::abs(pb->y - pa->y);
    int steps = std::max(dx, dy);
    if (steps == 0) {
        blend_pixel(pa->x, pa->y, rgba);
        return;
    }

    float step_x = static_cast<float>(pb->x - pa->x) / static_cast<float>(steps);
    float step_y = static_cast<float>(pb->y - pa->y) / static_cast<float>(steps);
    float half_width = std::max(1.0f, width);
    half_width = std::max(1.0f, std::min(half_width, 10.0f));  // avoid huge strokes
    int radius = static_cast<int>(std::ceil(half_width * 0.5f));

    float x = static_cast<float>(pa->x);
    float y = static_cast<float>(pa->y);
    for (int i = 0; i <= steps; ++i) {
        int ix = static_cast<int>(std::round(x));
        int iy = static_cast<int>(std::round(y));

        for (int oy = -radius; oy <= radius; ++oy) {
            for (int ox = -radius; ox <= radius; ++ox) {
                if (ox * ox + oy * oy <= radius * radius) {
                    blend_pixel(ix + ox, iy + oy, rgba);
                }
            }
        }

        x += step_x;
        y += step_y;
    }
}

}  // namespace manim
