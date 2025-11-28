/**
 * @file font_manager.hpp
 * @brief FreeType-based font management system for GPU text rendering
 *
 * Provides font loading, glyph metrics, and integration with the SDF generator.
 * Uses FreeType for font parsing and HarfBuzz for text shaping.
 */

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>

namespace manim {

/**
 * @brief Glyph metrics for text layout and rendering
 */
struct GlyphMetrics {
    uint32_t codepoint;          // Unicode codepoint
    uint32_t glyph_index;        // FreeType glyph index

    // Metrics in pixels (at render size)
    int width;                   // Glyph bitmap width
    int height;                  // Glyph bitmap height
    int bearing_x;               // Horizontal bearing (left side)
    int bearing_y;               // Vertical bearing (top side)
    int advance_x;               // Horizontal advance to next glyph
    int advance_y;               // Vertical advance (typically 0 for horizontal text)

    // UV coordinates in atlas (set by SDFAtlas)
    float u0 = 0, v0 = 0;        // Top-left UV
    float u1 = 0, v1 = 0;        // Bottom-right UV

    // SDF-specific
    int sdf_padding;             // Padding added for SDF
    bool has_outline;            // Whether glyph has outlines (not space)
};

/**
 * @brief Font face wrapper with metrics caching
 */
class FontFace {
public:
    FontFace() = default;
    FontFace(FT_Face face, const std::string& path, int size_pixels);
    ~FontFace();

    FontFace(FontFace&& other) noexcept;
    FontFace& operator=(FontFace&& other) noexcept;
    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;

    /**
     * @brief Set font size in pixels
     */
    bool set_size(int size_pixels);

    /**
     * @brief Get glyph metrics for a character
     */
    std::optional<GlyphMetrics> get_glyph_metrics(uint32_t codepoint);

    /**
     * @brief Get glyph bitmap (grayscale, 8-bit)
     */
    std::vector<uint8_t> render_glyph_bitmap(uint32_t codepoint, int& width, int& height);

    /**
     * @brief Get font ascender (pixels above baseline)
     */
    int get_ascender() const;

    /**
     * @brief Get font descender (pixels below baseline, negative)
     */
    int get_descender() const;

    /**
     * @brief Get line height
     */
    int get_line_height() const;

    /**
     * @brief Get kerning between two glyphs
     */
    int get_kerning(uint32_t left_codepoint, uint32_t right_codepoint) const;

    /**
     * @brief Get font family name
     */
    const std::string& get_family_name() const { return family_name_; }

    /**
     * @brief Get font style name
     */
    const std::string& get_style_name() const { return style_name_; }

    /**
     * @brief Get FreeType face (for advanced usage)
     */
    FT_Face get_ft_face() const { return face_; }

    /**
     * @brief Get HarfBuzz font (for text shaping)
     */
    hb_font_t* get_hb_font() const { return hb_font_; }

    /**
     * @brief Check if font has color glyphs (emoji)
     */
    bool has_color_glyphs() const { return has_color_; }

    /**
     * @brief Get current size in pixels
     */
    int get_size_pixels() const { return size_pixels_; }

private:
    FT_Face face_ = nullptr;
    hb_font_t* hb_font_ = nullptr;
    std::string file_path_;
    std::string family_name_;
    std::string style_name_;
    int size_pixels_ = 0;
    bool has_color_ = false;

    // Metrics cache
    mutable std::unordered_map<uint32_t, GlyphMetrics> metrics_cache_;
};

/**
 * @brief Font weight enumeration
 */
enum class FontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Regular = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

/**
 * @brief Font style enumeration
 */
enum class FontStyle {
    Normal,
    Italic,
    Oblique
};

/**
 * @brief Font request specification
 */
struct FontSpec {
    std::string family = "sans-serif";
    FontWeight weight = FontWeight::Regular;
    FontStyle style = FontStyle::Normal;
    int size_pixels = 48;
};

/**
 * @brief Central font management system
 *
 * Handles:
 * - FreeType library initialization
 * - Font discovery and loading
 * - Font caching by family/weight/style
 * - Fallback font resolution
 */
class FontManager {
public:
    /**
     * @brief Get singleton instance
     */
    static FontManager& instance();

    /**
     * @brief Initialize the font system
     */
    bool initialize();

    /**
     * @brief Shutdown and free all resources
     */
    void shutdown();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Add a font search path
     */
    void add_font_path(const std::filesystem::path& path);

    /**
     * @brief Load a specific font file
     */
    bool load_font(const std::filesystem::path& path);

    /**
     * @brief Get a font face matching the specification
     */
    FontFace* get_font(const FontSpec& spec);

    /**
     * @brief Get a font face by family name and size
     */
    FontFace* get_font(const std::string& family, int size_pixels);

    /**
     * @brief Get the default font
     */
    FontFace* get_default_font(int size_pixels = 48);

    /**
     * @brief Get all loaded font families
     */
    std::vector<std::string> get_font_families() const;

    /**
     * @brief Check if a font family is available
     */
    bool has_font_family(const std::string& family) const;

    /**
     * @brief Get FreeType library handle
     */
    FT_Library get_ft_library() const { return ft_library_; }

    /**
     * @brief Set the default font family
     */
    void set_default_family(const std::string& family);

private:
    FontManager() = default;
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    /**
     * @brief Discover system fonts
     */
    void discover_system_fonts();

    /**
     * @brief Find best matching font for spec
     */
    std::string find_font_file(const FontSpec& spec);

    /**
     * @brief Get weight score for matching
     */
    int weight_distance(FontWeight a, FontWeight b) const;

    FT_Library ft_library_ = nullptr;
    bool initialized_ = false;

    // Font search paths
    std::vector<std::filesystem::path> font_paths_;

    // Loaded fonts (key: file path)
    std::unordered_map<std::string, std::unique_ptr<FontFace>> loaded_fonts_;

    // Font family index (family name -> list of file paths)
    std::unordered_map<std::string, std::vector<std::string>> family_index_;

    // Font metadata (file path -> metadata)
    struct FontMeta {
        std::string family;
        std::string style;
        FontWeight weight;
        bool italic;
    };
    std::unordered_map<std::string, FontMeta> font_metadata_;

    std::string default_family_ = "DejaVu Sans";
};

} // namespace manim
