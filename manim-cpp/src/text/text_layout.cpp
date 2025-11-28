/**
 * @file text_layout.cpp
 * @brief Text layout engine implementation using HarfBuzz
 */

#include "manim/text/text_layout.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <locale>

namespace manim {

// ============================================================================
// TextLayout Implementation
// ============================================================================

const PositionedGlyph* TextLayout::get_glyph_at_char(size_t char_index) const {
    for (const auto& glyph : glyphs) {
        if (glyph.char_index == char_index) {
            return &glyph;
        }
    }
    return nullptr;
}

std::pair<math::Vec2, math::Vec2> TextLayout::get_char_bounds(size_t start, size_t end) const {
    math::Vec2 min_bound(std::numeric_limits<float>::max());
    math::Vec2 max_bound(std::numeric_limits<float>::lowest());

    for (const auto& glyph : glyphs) {
        if (glyph.char_index >= start && glyph.char_index < end) {
            min_bound.x = std::min(min_bound.x, glyph.x);
            min_bound.y = std::min(min_bound.y, glyph.y);
            max_bound.x = std::max(max_bound.x, glyph.x + glyph.width);
            max_bound.y = std::max(max_bound.y, glyph.y + glyph.height);
        }
    }

    return {min_bound, max_bound};
}

int TextLayout::get_line_at_y(float y) const {
    for (size_t i = 0; i < lines.size(); ++i) {
        if (y >= lines[i].baseline_y - lines[i].ascender &&
            y <= lines[i].baseline_y - lines[i].descender) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

size_t TextLayout::get_char_at_position(float x, float y) const {
    int line_idx = get_line_at_y(y);
    if (line_idx < 0) return std::string::npos;

    const auto& line = lines[line_idx];

    for (int i = line.start_index; i < line.end_index; ++i) {
        const auto& glyph = glyphs[i];
        if (x >= glyph.x && x <= glyph.x + glyph.advance_x) {
            return glyph.char_index;
        }
    }

    return std::string::npos;
}

// ============================================================================
// TextLayoutEngine Implementation
// ============================================================================

TextLayout TextLayoutEngine::layout(const std::string& text,
                                     FontFace& font,
                                     const TextLayoutOptions& options) {
    TextLayout result;
    result.num_characters = static_cast<int>(text.length());

    if (text.empty()) {
        result.total_width = 0;
        result.total_height = 0;
        result.num_glyphs = 0;
        result.num_lines = 0;
        return result;
    }

    // Shape text using HarfBuzz
    result.glyphs = shape_text(text, font);
    result.num_glyphs = static_cast<int>(result.glyphs.size());

    // Apply line breaking if max_width is set
    if (options.max_width > 0 && options.word_wrap) {
        auto line_breaks = break_into_lines(result.glyphs, options.max_width);

        // Reposition glyphs based on line breaks
        float y = 0;
        int line_idx = 0;
        float line_height = font.get_line_height() * options.line_height;

        for (const auto& [start, end] : line_breaks) {
            float x = 0;
            LineMetrics metrics;
            metrics.start_index = static_cast<int>(start);
            metrics.end_index = static_cast<int>(end);
            metrics.baseline_y = y + font.get_ascender();
            metrics.ascender = static_cast<float>(font.get_ascender());
            metrics.descender = static_cast<float>(font.get_descender());

            for (size_t i = start; i < end; ++i) {
                result.glyphs[i].x = x;
                result.glyphs[i].y = y;
                result.glyphs[i].line_index = line_idx;
                x += result.glyphs[i].advance_x;
            }

            metrics.width = x;
            metrics.height = line_height;
            result.lines.push_back(metrics);

            y += line_height;
            ++line_idx;
        }
    } else {
        // Single line
        float x = 0;
        LineMetrics metrics;
        metrics.start_index = 0;
        metrics.end_index = static_cast<int>(result.glyphs.size());
        metrics.baseline_y = static_cast<float>(font.get_ascender());
        metrics.ascender = static_cast<float>(font.get_ascender());
        metrics.descender = static_cast<float>(font.get_descender());

        for (auto& glyph : result.glyphs) {
            glyph.x = x;
            glyph.y = 0;
            glyph.line_index = 0;
            x += glyph.advance_x;
        }

        metrics.width = x;
        metrics.height = static_cast<float>(font.get_line_height()) * options.line_height;
        result.lines.push_back(metrics);
    }

    // Apply alignment
    apply_alignment(result, options);

    // Compute total bounds
    result.total_width = 0;
    result.total_height = 0;
    for (const auto& line : result.lines) {
        result.total_width = std::max(result.total_width, line.width);
        result.total_height = line.baseline_y - line.descender;
    }

    result.num_lines = static_cast<int>(result.lines.size());

    return result;
}

TextLayout TextLayoutEngine::layout_with_atlas(const std::string& text,
                                                FontFace& font,
                                                SDFAtlas& atlas,
                                                const TextLayoutOptions& options) {
    // First do the normal layout
    TextLayout result = layout(text, font, options);

    // Then set UV coordinates from atlas
    for (auto& glyph : result.glyphs) {
        const AtlasEntry* entry = atlas.get_entry(glyph.codepoint);
        if (entry) {
            glyph.u0 = entry->metrics.u0;
            glyph.v0 = entry->metrics.v0;
            glyph.u1 = entry->metrics.u1;
            glyph.v1 = entry->metrics.v1;
        }
    }

    return result;
}

std::vector<PositionedGlyph> TextLayoutEngine::shape_text(const std::string& text,
                                                           FontFace& font) {
    std::vector<PositionedGlyph> result;

    hb_font_t* hb_font = font.get_hb_font();
    if (!hb_font) {
        spdlog::warn("HarfBuzz font not available, using basic fallback layout");

        // Basic fallback: walk UTF-8 codepoints and use FreeType metrics/kerning
        std::vector<uint32_t> codepoints = utf8_to_codepoints(text);
        result.reserve(codepoints.size());

        float cursor_x = 0.0f;
        float cursor_y = 0.0f;
        uint32_t prev_cp = 0;
        bool has_prev = false;

        for (size_t i = 0; i < codepoints.size(); ++i) {
            uint32_t cp = codepoints[i];
            PositionedGlyph pg{};
            pg.codepoint = cp;
            pg.char_index = static_cast<int>(i);
            pg.line_index = 0;
            pg.is_whitespace = is_whitespace(cp);
            pg.is_line_break = (cp == '\n' || cp == '\r');

            // Kerning
            if (has_prev) {
                cursor_x += static_cast<float>(font.get_kerning(prev_cp, cp));
            }

            auto metrics = font.get_glyph_metrics(cp);
            if (metrics) {
                pg.width = static_cast<float>(metrics->width);
                pg.height = static_cast<float>(metrics->height);
                pg.advance_x = static_cast<float>(metrics->advance_x);
                pg.advance_y = static_cast<float>(metrics->advance_y);

                pg.x = cursor_x + metrics->bearing_x;
                pg.y = cursor_y - metrics->bearing_y;
            } else {
                // Minimal fallback metrics
                float line_h = static_cast<float>(font.get_line_height());
                pg.width = line_h * 0.5f;
                pg.height = line_h;
                pg.advance_x = pg.width;
                pg.advance_y = 0.0f;
                pg.x = cursor_x;
                pg.y = cursor_y;
            }

            cursor_x += pg.advance_x;
            prev_cp = cp;
            has_prev = true;

            result.push_back(pg);
        }

        return result;
    }

    // Create HarfBuzz buffer
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buffer, hb_language_from_string("en", -1));

    // Add text to buffer
    hb_buffer_add_utf8(buffer, text.c_str(), static_cast<int>(text.length()), 0, -1);

    // Shape text
    hb_shape(hb_font, buffer, nullptr, 0);

    // Get glyph info and positions
    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_count);

    result.reserve(glyph_count);

    float cursor_x = 0;
    float cursor_y = 0;

    for (unsigned int i = 0; i < glyph_count; ++i) {
        PositionedGlyph pg;

        pg.glyph_index = glyph_info[i].codepoint;  // After shaping, this is glyph index
        pg.char_index = glyph_info[i].cluster;

        // Get codepoint from cluster
        // For simple cases, we can get it from the original string
        if (pg.char_index < text.length()) {
            // Simple ASCII handling
            unsigned char c = text[pg.char_index];
            if ((c & 0x80) == 0) {
                pg.codepoint = c;
            } else {
                // UTF-8 decoding
                auto codepoints = utf8_to_codepoints(text.substr(pg.char_index));
                if (!codepoints.empty()) {
                    pg.codepoint = codepoints[0];
                }
            }
        }

        // Get metrics from font
        auto metrics = font.get_glyph_metrics(pg.codepoint);

        // Position from HarfBuzz (in 1/64th points)
        float x_offset = static_cast<float>(glyph_pos[i].x_offset) / 64.0f;
        float y_offset = static_cast<float>(glyph_pos[i].y_offset) / 64.0f;
        float x_advance = static_cast<float>(glyph_pos[i].x_advance) / 64.0f;
        float y_advance = static_cast<float>(glyph_pos[i].y_advance) / 64.0f;

        pg.x = cursor_x + x_offset;
        pg.y = cursor_y + y_offset;

        if (metrics) {
            pg.width = static_cast<float>(metrics->width);
            pg.height = static_cast<float>(metrics->height);

            // Adjust position by bearing
            pg.x += metrics->bearing_x;
            pg.y -= metrics->bearing_y;  // FreeType bearing is from baseline up
        } else {
            pg.width = x_advance;
            pg.height = static_cast<float>(font.get_line_height());
        }

        pg.advance_x = x_advance;
        pg.advance_y = y_advance;
        pg.line_index = 0;
        pg.is_whitespace = is_whitespace(pg.codepoint);
        pg.is_line_break = (pg.codepoint == '\n' || pg.codepoint == '\r');

        cursor_x += x_advance;
        cursor_y += y_advance;

        result.push_back(pg);
    }

    hb_buffer_destroy(buffer);

    return result;
}

void TextLayoutEngine::apply_alignment(TextLayout& layout, const TextLayoutOptions& options) {
    if (layout.lines.empty()) return;

    float max_width = 0;
    for (const auto& line : layout.lines) {
        max_width = std::max(max_width, line.width);
    }

    for (size_t line_idx = 0; line_idx < layout.lines.size(); ++line_idx) {
        auto& line = layout.lines[line_idx];
        float offset = 0;

        switch (options.alignment) {
            case TextAlignment::Left:
                offset = 0;
                break;
            case TextAlignment::Center:
                offset = (max_width - line.width) / 2.0f;
                break;
            case TextAlignment::Right:
                offset = max_width - line.width;
                break;
            case TextAlignment::Justify:
                // TODO: Implement justify by adjusting word spacing
                offset = 0;
                break;
        }

        // Apply offset to all glyphs in this line
        for (int i = line.start_index; i < line.end_index; ++i) {
            layout.glyphs[i].x += offset;
        }
    }
}

std::vector<std::pair<size_t, size_t>> TextLayoutEngine::break_into_lines(
    const std::vector<PositionedGlyph>& glyphs,
    float max_width) {

    std::vector<std::pair<size_t, size_t>> lines;
    if (glyphs.empty()) return lines;

    size_t line_start = 0;
    size_t last_break = 0;
    float line_width = 0;

    for (size_t i = 0; i < glyphs.size(); ++i) {
        const auto& glyph = glyphs[i];

        // Check for explicit line break
        if (glyph.is_line_break) {
            lines.push_back({line_start, i + 1});
            line_start = i + 1;
            line_width = 0;
            last_break = i + 1;
            continue;
        }

        // Track word boundaries
        if (glyph.is_whitespace) {
            last_break = i;
        }

        line_width += glyph.advance_x;

        // Check if we need to wrap
        if (line_width > max_width && i > line_start) {
            if (last_break > line_start) {
                // Break at last word boundary
                lines.push_back({line_start, last_break});
                line_start = last_break;

                // Skip whitespace at start of new line
                while (line_start < glyphs.size() && glyphs[line_start].is_whitespace) {
                    ++line_start;
                }
            } else {
                // No word boundary, break at current position
                lines.push_back({line_start, i});
                line_start = i;
            }

            // Recalculate line width
            line_width = 0;
            for (size_t j = line_start; j <= i; ++j) {
                line_width += glyphs[j].advance_x;
            }
            last_break = line_start;
        }
    }

    // Add final line
    if (line_start < glyphs.size()) {
        lines.push_back({line_start, glyphs.size()});
    }

    return lines;
}

std::vector<uint32_t> TextLayoutEngine::utf8_to_codepoints(const std::string& text) {
    std::vector<uint32_t> codepoints;
    const char* ptr = text.c_str();
    const char* end = ptr + text.length();

    while (ptr < end) {
        uint32_t cp = 0;
        unsigned char c = *ptr++;

        if ((c & 0x80) == 0) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            if (ptr < end) cp = (cp << 6) | (*ptr++ & 0x3F);
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            if (ptr < end) cp = (cp << 6) | (*ptr++ & 0x3F);
            if (ptr < end) cp = (cp << 6) | (*ptr++ & 0x3F);
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            if (ptr < end) cp = (cp << 6) | (*ptr++ & 0x3F);
            if (ptr < end) cp = (cp << 6) | (*ptr++ & 0x3F);
            if (ptr < end) cp = (cp << 6) | (*ptr++ & 0x3F);
        }

        codepoints.push_back(cp);
    }

    return codepoints;
}

bool TextLayoutEngine::is_word_boundary(uint32_t codepoint) {
    return is_whitespace(codepoint) ||
           codepoint == '-' ||
           codepoint == '/' ||
           codepoint == '\\';
}

bool TextLayoutEngine::is_whitespace(uint32_t codepoint) {
    return codepoint == ' ' ||
           codepoint == '\t' ||
           codepoint == '\n' ||
           codepoint == '\r' ||
           codepoint == 0x00A0 ||  // Non-breaking space
           codepoint == 0x2003 ||  // Em space
           codepoint == 0x2002;    // En space
}

// ============================================================================
// Helper Functions
// ============================================================================

TextRenderData build_render_data(const TextLayout& layout,
                                  SDFAtlas& atlas,
                                  const math::Vec4& color,
                                  float scale) {
    TextRenderData data;
    data.atlas = &atlas;
    data.instances.reserve(layout.glyphs.size());

    data.min_bound = math::Vec2(std::numeric_limits<float>::max());
    data.max_bound = math::Vec2(std::numeric_limits<float>::lowest());

    for (const auto& glyph : layout.glyphs) {
        if (glyph.is_whitespace) continue;

        const AtlasEntry* entry = atlas.get_entry(glyph.codepoint);
        if (!entry || entry->width == 0) continue;

        GPUGlyphInstance instance;

        // Position and size
        float x = glyph.x * scale;
        float y = glyph.y * scale;
        float w = glyph.width * scale;
        float h = glyph.height * scale;

        instance.position_size = math::Vec4(x, y, w, h);

        // UV rectangle
        instance.uv_rect = math::Vec4(
            entry->metrics.u0,
            entry->metrics.v0,
            entry->metrics.u1 - entry->metrics.u0,
            entry->metrics.v1 - entry->metrics.v0
        );

        // Color
        instance.color = color;

        // Effects (none by default)
        instance.effects = math::Vec4(0, 0, 0, 0);

        data.instances.push_back(instance);

        // Update bounds
        data.min_bound.x = std::min(data.min_bound.x, x);
        data.min_bound.y = std::min(data.min_bound.y, y);
        data.max_bound.x = std::max(data.max_bound.x, x + w);
        data.max_bound.y = std::max(data.max_bound.y, y + h);
    }

    return data;
}

} // namespace manim
