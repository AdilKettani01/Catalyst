/**
 * @file text_layout.hpp
 * @brief Text layout engine using HarfBuzz for text shaping
 *
 * Provides Unicode-aware text layout with:
 * - Complex script support (Arabic, Hindi, etc.)
 * - Bidirectional text
 * - Ligatures and kerning
 * - Line breaking and wrapping
 */

#pragma once

#include "manim/text/font_manager.hpp"
#include "manim/text/sdf_generator.hpp"
#include "manim/core/math.hpp"
#include <string>
#include <vector>
#include <memory>

namespace manim {

/**
 * @brief Text alignment options
 */
enum class TextAlignment {
    Left,
    Center,
    Right,
    Justify
};

/**
 * @brief Vertical alignment options
 */
enum class VerticalAlignment {
    Top,
    Center,
    Bottom,
    Baseline
};

/**
 * @brief Text direction
 */
enum class TextDirection {
    LeftToRight,
    RightToLeft,
    TopToBottom
};

/**
 * @brief Layout options for text
 */
struct TextLayoutOptions {
    TextAlignment alignment = TextAlignment::Left;
    VerticalAlignment vertical_alignment = VerticalAlignment::Baseline;
    TextDirection direction = TextDirection::LeftToRight;

    float line_height = 1.2f;       // Line height multiplier
    float letter_spacing = 0.0f;    // Additional spacing between characters
    float word_spacing = 0.0f;      // Additional spacing between words

    float max_width = 0.0f;         // Maximum line width (0 = no limit)
    bool word_wrap = true;          // Enable word wrapping

    float tab_width = 4.0f;         // Tab width in spaces
};

/**
 * @brief Positioned glyph for rendering
 */
struct PositionedGlyph {
    uint32_t codepoint;
    uint32_t glyph_index;
    size_t char_index;              // Index in original string

    // Position (in layout space, origin at top-left)
    float x;
    float y;

    // Size
    float width;
    float height;

    // Advance (for cursor positioning)
    float advance_x;
    float advance_y;

    // UV coordinates in SDF atlas
    float u0, v0, u1, v1;

    // Line information
    int line_index;
    bool is_whitespace;
    bool is_line_break;
};

/**
 * @brief Line metrics
 */
struct LineMetrics {
    int start_index;                // First glyph index in line
    int end_index;                  // Last glyph index + 1
    float width;                    // Line width
    float height;                   // Line height
    float baseline_y;               // Y position of baseline
    float ascender;                 // Max ascender in line
    float descender;                // Max descender in line
};

/**
 * @brief Complete text layout result
 */
struct TextLayout {
    std::vector<PositionedGlyph> glyphs;
    std::vector<LineMetrics> lines;

    // Bounding box
    float total_width;
    float total_height;

    // Metrics
    int num_glyphs;
    int num_lines;
    int num_characters;

    /**
     * @brief Get glyph at character index
     */
    const PositionedGlyph* get_glyph_at_char(size_t char_index) const;

    /**
     * @brief Get bounding box for character range
     */
    std::pair<math::Vec2, math::Vec2> get_char_bounds(size_t start, size_t end) const;

    /**
     * @brief Get line for y position
     */
    int get_line_at_y(float y) const;

    /**
     * @brief Get character index for position
     */
    size_t get_char_at_position(float x, float y) const;
};

/**
 * @brief Text shaping and layout engine
 *
 * Uses HarfBuzz for text shaping and provides:
 * - Unicode normalization
 * - Complex script shaping
 * - Bidirectional text support
 * - Line breaking
 */
class TextLayoutEngine {
public:
    TextLayoutEngine() = default;

    /**
     * @brief Layout text with given options
     */
    TextLayout layout(const std::string& text,
                      FontFace& font,
                      const TextLayoutOptions& options = TextLayoutOptions());

    /**
     * @brief Layout text and ensure all glyphs are in atlas
     */
    TextLayout layout_with_atlas(const std::string& text,
                                  FontFace& font,
                                  SDFAtlas& atlas,
                                  const TextLayoutOptions& options = TextLayoutOptions());

    /**
     * @brief Shape text using HarfBuzz
     */
    std::vector<PositionedGlyph> shape_text(const std::string& text,
                                             FontFace& font);

    /**
     * @brief Apply line breaking to shaped text
     */
    void apply_line_breaks(std::vector<PositionedGlyph>& glyphs,
                           const std::vector<LineMetrics>& lines,
                           const TextLayoutOptions& options);

    /**
     * @brief Compute line metrics
     */
    std::vector<LineMetrics> compute_line_metrics(
        const std::vector<PositionedGlyph>& glyphs,
        FontFace& font,
        const TextLayoutOptions& options);

    /**
     * @brief Apply alignment to layout
     */
    void apply_alignment(TextLayout& layout, const TextLayoutOptions& options);

private:
    /**
     * @brief Find line break opportunities
     */
    std::vector<size_t> find_break_points(const std::string& text);

    /**
     * @brief Break text into lines based on max width
     */
    std::vector<std::pair<size_t, size_t>> break_into_lines(
        const std::vector<PositionedGlyph>& glyphs,
        float max_width);

    /**
     * @brief Convert UTF-8 string to codepoints
     */
    std::vector<uint32_t> utf8_to_codepoints(const std::string& text);

    /**
     * @brief Check if character is a word boundary
     */
    bool is_word_boundary(uint32_t codepoint);

    /**
     * @brief Check if character is whitespace
     */
    bool is_whitespace(uint32_t codepoint);
};

/**
 * @brief GPU glyph instance data for batched rendering
 *
 * Layout must match shader vertex inputs (sdf_text.vert):
 * - location 2: vec4 position_size (xy = position, zw = size)
 * - location 3: vec4 uv_rect
 * - location 4: vec4 color
 * - location 5: vec4 effects (outline_width, glow_intensity, shadow_offset, shadow_blur)
 */
struct GPUGlyphInstance {
    math::Vec4 position_size;       // xy = position, zw = size
    math::Vec4 uv_rect;             // UV rectangle in atlas
    math::Vec4 color;               // RGBA color
    math::Vec4 effects;             // x = outline_width, y = glow_intensity, z = shadow_offset, w = shadow_blur
};

/**
 * @brief Text render data for GPU
 */
struct TextRenderData {
    std::vector<GPUGlyphInstance> instances;
    SDFAtlas* atlas;

    // Bounds
    math::Vec2 min_bound;
    math::Vec2 max_bound;

    // Per-character colors (optional)
    std::vector<math::Vec4> char_colors;
};

/**
 * @brief Build GPU render data from layout
 */
TextRenderData build_render_data(const TextLayout& layout,
                                  SDFAtlas& atlas,
                                  const math::Vec4& color = math::Vec4(1, 1, 1, 1),
                                  float scale = 1.0f);

} // namespace manim
