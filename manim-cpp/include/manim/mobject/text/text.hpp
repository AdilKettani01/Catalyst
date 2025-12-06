#pragma once

#include "manim/mobject/vmobject.hpp"
#include <string>
#include <memory>

namespace manim {

/**
 * @brief GPU-accelerated text rendering using SDF (Signed Distance Fields)
 *
 * Modern text rendering using:
 * - MSDF (Multi-channel Signed Distance Fields) for sharp text at any scale
 * - GPU glyph atlas texture
 * - Instanced rendering for characters
 * - Real-time text effects (glow, outline, shadow)
 *
 * Performance: 1000s of text objects at 60+ FPS
 */
class Text : public VMobject {
public:
    Text() = default;
    explicit Text(const std::string& text, float font_size = 48.0f);
    virtual ~Text() = default;

    // ========================================================================
    // Text Content
    // ========================================================================

    /**
     * @brief Set text string
     */
    Text& set_text(const std::string& text);

    /**
     * @brief Get text string
     */
    const std::string& get_text() const { return text_; }

    // ========================================================================
    // Font Properties
    // ========================================================================

    /**
     * @brief Set font
     */
    Text& set_font(const std::string& font_name);

    /**
     * @brief Get font name
     */
    const std::string& get_font_name() const { return font_name_; }

    /**
     * @brief Set font size
     */
    Text& set_font_size(float size);

    float get_font_size() const { return font_size_; }

    /**
     * @brief Set font weight (bold, normal, light)
     */
    enum class Weight {
        Thin = 100,
        ExtraLight = 200,
        Light = 300,
        Normal = 400,
        Medium = 500,
        SemiBold = 600,
        Bold = 700,
        ExtraBold = 800,
        Black = 900
    };

    Text& set_weight(Weight weight);

    /**
     * @brief Set italic
     */
    Text& set_italic(bool italic);

    // ========================================================================
    // Text Effects
    // ========================================================================

    /**
     * @brief Set outline
     */
    Text& set_outline(float width, const math::Vec4& color = math::Vec4{0,0,0,1});

    /**
     * @brief Set glow effect
     */
    Text& set_glow(float intensity, const math::Vec4& color);

    /**
     * @brief Set shadow
     */
    Text& set_shadow(const math::Vec2& offset, float blur, const math::Vec4& color);

    // ========================================================================
    // Layout
    // ========================================================================

    enum class Alignment {
        Left,
        Center,
        Right,
        Justify
    };

    /**
     * @brief Set text alignment
     */
    Text& set_alignment(Alignment alignment);

    /**
     * @brief Set line spacing
     */
    Text& set_line_spacing(float spacing);

    /**
     * @brief Set maximum width (for wrapping)
     */
    Text& set_max_width(float width);

    // ========================================================================
    // GPU SDF Rendering
    // ========================================================================

    /**
     * @brief Glyph instance data for GPU rendering
     *
     * Layout must match shader vertex inputs (sdf_text.vert):
     * - location 2: vec4 (position.xy, size.xy) - pos_size  [position + size read as single vec4]
     * - location 3: vec4 uv_rect
     * - location 4: vec4 color
     * - location 5: vec4 effects (outline_width, glow_intensity, shadow_offset, shadow_blur)
     *
     * Total size: 64 bytes (4 x vec4 equivalent)
     * Stride for instanced rendering must equal sizeof(GlyphInstance)
     */
    struct GlyphInstance {
        math::Vec2 position;      // Screen position (location 2.xy) - 8 bytes
        math::Vec2 size;          // Glyph size (location 2.zw) - 8 bytes
        // Note: position + size are read together as vec4 at location 2
        math::Vec4 uv_rect;       // UV rectangle in atlas (x, y, w, h) - location 3, 16 bytes
        math::Vec4 color;         // Text color - location 4, 16 bytes
        math::Vec4 effects;       // (outline_width, glow_intensity, shadow_offset, shadow_blur) - location 5, 16 bytes
        // Total: 64 bytes per instance
    };

    /**
     * @brief Upload text to GPU as glyph instances
     */
    void upload_glyphs_to_gpu(MemoryPool& pool);

    /**
     * @brief Render text using instanced SDF glyphs
     */
    void render_sdf_text(VkCommandBuffer cmd);

    /**
     * @brief Get SDF atlas texture
     */
    static const GPUImage& get_sdf_atlas();

    /**
     * @brief Get glyph instance buffer for GPU rendering
     */
    const GPUBuffer* get_glyph_buffer() const {
        return glyph_instance_buffer_.has_value() ? &(*glyph_instance_buffer_) : nullptr;
    }

    /**
     * @brief Get number of glyphs to render
     */
    size_t get_glyph_count() const { return glyph_instances_.size(); }

    // ========================================================================
    // Per-Character Access
    // ========================================================================

    /**
     * @brief Get bounding box for character at index
     */
    std::pair<math::Vec3, math::Vec3> get_char_bounding_box(size_t index) const;

    /**
     * @brief Color individual characters
     */
    Text& set_char_color(size_t index, const math::Vec4& color);

    /**
     * @brief Color range of characters
     */
    Text& set_char_color_range(size_t start, size_t end, const math::Vec4& color);

protected:
    void generate_points() override;

    /**
     * @brief Layout text (compute glyph positions)
     */
    void layout_text();

private:
    // Text content
    std::string text_;
    float font_size_{48.0f};
    std::string font_name_{"Arial"};
    Weight weight_{Weight::Normal};
    bool italic_{false};

    // Layout
    Alignment alignment_{Alignment::Left};
    float line_spacing_{1.2f};
    float max_width_{std::numeric_limits<float>::max()};

    // Effects
    float outline_width_{0.0f};
    math::Vec4 outline_color_{0,0,0,1};
    float glow_intensity_{0.0f};
    math::Vec4 glow_color_{1,1,1,1};
    math::Vec2 shadow_offset_{0,0};
    float shadow_blur_{0.0f};
    math::Vec4 shadow_color_{0,0,0,0.5f};

    // GPU data
    std::vector<GlyphInstance> glyph_instances_;
    std::optional<GPUBuffer> glyph_instance_buffer_;
    bool glyphs_dirty_{true};

    // SDF atlas (shared across all Text objects)
    static std::shared_ptr<GPUImage> sdf_atlas_;
};

/**
 * @brief TeX mobject for mathematical formulas
 *
 * Renders LaTeX equations using:
 * - Precompiled SVG from LaTeX
 * - GPU path rendering
 * - Or rasterized SDF for complex formulas
 */
class Tex : public VMobject {
public:
    Tex() = default;
    explicit Tex(const std::string& latex_string);

    /**
     * @brief Set LaTeX string
     */
    Tex& set_latex(const std::string& latex);

    const std::string& get_latex() const { return latex_; }

    /**
     * @brief Color parts of formula
     */
    Tex& set_color_by_tex(const std::string& tex, const math::Vec4& color);

protected:
    void generate_points() override;

    /**
     * @brief Compile LaTeX to SVG
     */
    void compile_latex();

private:
    std::string latex_;
    std::vector<std::string> tex_parts_;
};

/**
 * @brief Math text (simplified TeX)
 */
class MathTex : public Tex {
public:
    explicit MathTex(const std::string& math_string);
};

}  // namespace manim
