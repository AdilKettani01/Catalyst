/**
 * @file text.cpp
 * @brief GPU-accelerated text rendering implementation
 */

#include "manim/mobject/text/text.hpp"
#include "manim/text/font_manager.hpp"
#include "manim/text/sdf_generator.hpp"
#include "manim/text/text_layout.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace manim {

// Static member initialization
std::shared_ptr<GPUImage> Text::sdf_atlas_;

// ============================================================================
// Text Implementation
// ============================================================================

Text::Text(const std::string& text, float font_size)
    : text_(text), font_size_(font_size) {
    generate_points();
}

Text& Text::set_text(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_font(const std::string& font_name) {
    if (font_name_ != font_name) {
        font_name_ = font_name;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_font_size(float size) {
    if (std::abs(font_size_ - size) > 0.001f) {
        font_size_ = size;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_weight(Weight weight) {
    if (weight_ != weight) {
        weight_ = weight;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_italic(bool italic) {
    if (italic_ != italic) {
        italic_ = italic;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_outline(float width, const math::Vec4& color) {
    outline_width_ = width;
    outline_color_ = color;
    glyphs_dirty_ = true;
    return *this;
}

Text& Text::set_glow(float intensity, const math::Vec4& color) {
    glow_intensity_ = intensity;
    glow_color_ = color;
    glyphs_dirty_ = true;
    return *this;
}

Text& Text::set_shadow(const math::Vec2& offset, float blur, const math::Vec4& color) {
    shadow_offset_ = offset;
    shadow_blur_ = blur;
    shadow_color_ = color;
    glyphs_dirty_ = true;
    return *this;
}

Text& Text::set_alignment(Alignment alignment) {
    if (alignment_ != alignment) {
        alignment_ = alignment;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_line_spacing(float spacing) {
    if (std::abs(line_spacing_ - spacing) > 0.001f) {
        line_spacing_ = spacing;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

Text& Text::set_max_width(float width) {
    if (std::abs(max_width_ - width) > 0.001f) {
        max_width_ = width;
        glyphs_dirty_ = true;
        generate_points();
    }
    return *this;
}

void Text::generate_points() {
    if (text_.empty()) {
        points_cpu_.clear();
        num_points_ = 0;
        glyph_instances_.clear();
        return;
    }

    // Initialize font system if needed
    FontManager& fm = FontManager::instance();
    if (!fm.is_initialized()) {
        fm.initialize();
    }

    // Get font spec
    FontSpec spec;
    spec.family = font_name_;
    spec.weight = static_cast<FontWeight>(weight_);
    spec.style = italic_ ? FontStyle::Italic : FontStyle::Normal;
    spec.size_pixels = static_cast<int>(font_size_);

    FontFace* font = fm.get_font(spec);
    if (!font) {
        spdlog::warn("Font '{}' not found, using default", font_name_);
        font = fm.get_default_font(static_cast<int>(font_size_));
    }

    if (!font) {
        spdlog::error("No fonts available for text rendering");
        return;
    }

    // Ensure glyphs are in atlas
    SDFAtlasManager& am = SDFAtlasManager::instance();
    am.ensure_glyphs(font_name_, text_);

    SDFAtlas* atlas = am.get_atlas(font_name_);
    if (!atlas) {
        spdlog::error("Failed to get SDF atlas");
        return;
    }

    // Layout text
    TextLayoutEngine layout_engine;
    TextLayoutOptions options;
    options.alignment = static_cast<TextAlignment>(alignment_);
    options.line_height = line_spacing_;
    options.max_width = max_width_ < 1e6f ? max_width_ : 0;
    options.word_wrap = max_width_ < 1e6f;

    TextLayout layout = layout_engine.layout_with_atlas(text_, *font, *atlas, options);

    // Convert layout to glyph instances
    glyph_instances_.clear();
    glyph_instances_.reserve(layout.glyphs.size());

    // Also generate VMobject points for bounding box and transforms
    points_cpu_.clear();

    // Scale factor to normalize text to Manim units (roughly 1 unit = 1 scene unit)
    float scale = 1.0f / font_size_;

    for (const auto& pg : layout.glyphs) {
        if (pg.is_whitespace) continue;

        const AtlasEntry* entry = atlas->get_entry(pg.codepoint);
        if (!entry || entry->width == 0) continue;

        GlyphInstance instance;

        // Position in normalized units
        float x = pg.x * scale;
        float y = -pg.y * scale;  // Flip Y for Manim coordinate system
        float w = pg.width * scale;
        float h = pg.height * scale;

        instance.position = math::Vec2(x, y);
        instance.size = math::Vec2(w, h);

        // UV from atlas
        instance.uv_rect = math::Vec4(
            entry->metrics.u0,
            entry->metrics.v0,
            entry->metrics.u1 - entry->metrics.u0,
            entry->metrics.v1 - entry->metrics.v0
        );

        // Color
        instance.color = color_;
        // Pack effects into vec4 matching shader: (outline_width, glow_intensity, shadow_offset, shadow_blur)
        // shadow_offset is a length here (will use the effects buffer for full 2D offset)
        float shadow_offset_len = std::sqrt(shadow_offset_.x * shadow_offset_.x + shadow_offset_.y * shadow_offset_.y);
        instance.effects = math::Vec4(outline_width_, glow_intensity_, shadow_offset_len, shadow_blur_);

        glyph_instances_.push_back(instance);

        // Add corner points for VMobject
        points_cpu_.push_back(math::Vec3(x, y, 0));
        points_cpu_.push_back(math::Vec3(x + w, y, 0));
        points_cpu_.push_back(math::Vec3(x + w, y - h, 0));
        points_cpu_.push_back(math::Vec3(x, y - h, 0));
    }

    num_points_ = points_cpu_.size();
    gpu_dirty_ = true;
    bbox_dirty_ = true;
    glyphs_dirty_ = true;
}

void Text::layout_text() {
    // Delegate to generate_points which handles full layout
    generate_points();
}

void Text::upload_glyphs_to_gpu(MemoryPool& pool) {
    if (!glyphs_dirty_ || glyph_instances_.empty()) return;

    // Create or resize instance buffer
    size_t buffer_size = glyph_instances_.size() * sizeof(GlyphInstance);

    glyph_instance_buffer_ = pool.allocate_buffer(
        buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Upload instance data
    glyph_instance_buffer_->upload(std::span<const GlyphInstance>(glyph_instances_));

    glyphs_dirty_ = false;
}

void Text::render_sdf_text(VkCommandBuffer cmd) {
    // This would be called by the renderer
    // The actual rendering would use:
    // 1. SDF text shader pipeline
    // 2. Atlas texture bound
    // 3. Instance buffer bound
    // 4. Draw call with instanced quads

    // Placeholder - actual implementation depends on renderer integration
    (void)cmd;
}

const GPUImage& Text::get_sdf_atlas() {
    static GPUImage dummy;
    if (sdf_atlas_) {
        return *sdf_atlas_;
    }
    return dummy;
}

std::pair<math::Vec3, math::Vec3> Text::get_char_bounding_box(size_t index) const {
    if (index >= glyph_instances_.size()) {
        return {math::Vec3(0), math::Vec3(0)};
    }

    const auto& inst = glyph_instances_[index];
    math::Vec3 min_pt(inst.position.x, inst.position.y - inst.size.y, 0);
    math::Vec3 max_pt(inst.position.x + inst.size.x, inst.position.y, 0);

    return {min_pt, max_pt};
}

Text& Text::set_char_color(size_t index, const math::Vec4& color) {
    if (index < glyph_instances_.size()) {
        glyph_instances_[index].color = color;
        glyphs_dirty_ = true;
    }
    return *this;
}

Text& Text::set_char_color_range(size_t start, size_t end, const math::Vec4& color) {
    end = std::min(end, glyph_instances_.size());
    for (size_t i = start; i < end; ++i) {
        glyph_instances_[i].color = color;
    }
    glyphs_dirty_ = true;
    return *this;
}

// ============================================================================
// Tex Implementation
// ============================================================================

Tex::Tex(const std::string& latex_string)
    : latex_(latex_string) {
    generate_points();
}

Tex& Tex::set_latex(const std::string& latex) {
    if (latex_ != latex) {
        latex_ = latex;
        generate_points();
    }
    return *this;
}

Tex& Tex::set_color_by_tex(const std::string& tex, const math::Vec4& color) {
    // Find submobjects containing this tex and color them
    // This would require parsing the LaTeX and mapping to generated paths

    // For now, store the mapping for when we render
    tex_parts_.push_back(tex);
    (void)color;  // TODO: Store color mapping
    return *this;
}

void Tex::generate_points() {
    if (latex_.empty()) {
        points_cpu_.clear();
        num_points_ = 0;
        return;
    }

    compile_latex();
}

void Tex::compile_latex() {
    // LaTeX compilation options:
    // 1. Shell out to latex/dvisvgm to generate SVG, then parse
    // 2. Use a LaTeX-to-SVG library
    // 3. For simple math, render using special math fonts

    // For now, we'll implement a simplified version that handles
    // basic math symbols using the math font

    spdlog::info("LaTeX compilation for: {}", latex_);

    // TODO: Implement full LaTeX support
    // This would involve:
    // 1. Writing a .tex file with the formula
    // 2. Running latex -> dvi -> svg pipeline
    // 3. Parsing SVG paths
    // 4. Converting to Bezier curves

    // Placeholder: treat as regular text for now
    // In a full implementation, this would generate proper SVG paths
}

// ============================================================================
// MathTex Implementation
// ============================================================================

MathTex::MathTex(const std::string& math_string)
    : Tex("$" + math_string + "$") {
    // MathTex wraps the string in $ $ for inline math mode
}

} // namespace manim
