/**
 * @file sdf_generator.hpp
 * @brief Signed Distance Field generator for GPU text rendering
 *
 * Generates high-quality SDF textures from font glyphs for resolution-independent
 * text rendering. Supports both standard SDF and MSDF (multi-channel SDF) for
 * sharper edges.
 */

#pragma once

#include "manim/text/font_manager.hpp"
#include "manim/core/memory_pool.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

namespace manim {

/**
 * @brief SDF generation parameters
 */
struct SDFConfig {
    int sdf_size = 64;              // Size of SDF glyph texture
    int sdf_padding = 8;            // Padding around glyph in SDF texture
    float sdf_range = 4.0f;         // Distance range in pixels (for SDF)
    bool use_msdf = true;           // Use multi-channel SDF for sharper edges
    int supersample = 4;            // Supersampling factor for anti-aliasing
};

/**
 * @brief Single glyph SDF data
 */
struct SDFGlyph {
    uint32_t codepoint;
    std::vector<uint8_t> sdf_data;  // SDF texture data (1 channel for SDF, 4 for MSDF)
    int width;
    int height;
    int channels;                   // 1 for SDF, 4 for MSDF (RGB + alpha)
    GlyphMetrics metrics;
};

/**
 * @brief Packed glyph atlas entry
 */
struct AtlasEntry {
    uint32_t codepoint;
    int x, y;                       // Position in atlas
    int width, height;              // Size in atlas
    GlyphMetrics metrics;           // Glyph metrics
};

/**
 * @brief GPU-resident SDF glyph atlas
 *
 * Packs multiple SDF glyphs into a single texture for efficient rendering.
 * Uses a bin-packing algorithm to minimize wasted space.
 */
class SDFAtlas {
public:
    SDFAtlas() = default;
    ~SDFAtlas() = default;

    /**
     * @brief Initialize atlas with given size
     */
    void initialize(int width, int height, int channels = 1);

    /**
     * @brief Add a glyph to the atlas
     * @return true if glyph was added, false if atlas is full
     */
    bool add_glyph(const SDFGlyph& glyph);

    /**
     * @brief Get atlas entry for a codepoint
     */
    const AtlasEntry* get_entry(uint32_t codepoint) const;

    /**
     * @brief Check if codepoint is in atlas
     */
    bool has_glyph(uint32_t codepoint) const;

    /**
     * @brief Upload atlas to GPU
     */
    void upload_to_gpu(MemoryPool& pool);

    /**
     * @brief Get GPU texture
     */
    const GPUImage& get_gpu_texture() const { return gpu_texture_; }

    /**
     * @brief Get atlas texture data (for debugging)
     */
    const std::vector<uint8_t>& get_texture_data() const { return texture_data_; }

    /**
     * @brief Get atlas dimensions
     */
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    int get_channels() const { return channels_; }

    /**
     * @brief Check if atlas needs re-upload
     */
    bool is_dirty() const { return dirty_; }

    /**
     * @brief Clear dirty flag
     */
    void clear_dirty() { dirty_ = false; }

    /**
     * @brief Get all entries
     */
    const std::unordered_map<uint32_t, AtlasEntry>& get_entries() const { return entries_; }

private:
    /**
     * @brief Simple bin-packing: find space for a rectangle
     */
    bool find_space(int width, int height, int& out_x, int& out_y);

    /**
     * @brief Copy glyph data into atlas texture
     */
    void copy_glyph_to_texture(const SDFGlyph& glyph, int x, int y);

    int width_ = 0;
    int height_ = 0;
    int channels_ = 1;

    std::vector<uint8_t> texture_data_;
    std::unordered_map<uint32_t, AtlasEntry> entries_;

    // Simple row-based packing state
    int current_row_y_ = 0;
    int current_row_height_ = 0;
    int current_x_ = 0;

    // GPU resources
    GPUImage gpu_texture_;
    bool dirty_ = true;
};

/**
 * @brief SDF glyph generator
 *
 * Generates signed distance field textures from FreeType glyph outlines.
 * Supports:
 * - Standard SDF (single channel)
 * - MSDF (multi-channel for sharper edges)
 * - GPU-accelerated generation (optional)
 */
class SDFGenerator {
public:
    SDFGenerator() = default;
    explicit SDFGenerator(const SDFConfig& config);

    /**
     * @brief Set SDF generation configuration
     */
    void set_config(const SDFConfig& config) { config_ = config; }

    /**
     * @brief Get current configuration
     */
    const SDFConfig& get_config() const { return config_; }

    /**
     * @brief Generate SDF for a single glyph
     */
    SDFGlyph generate_glyph_sdf(FontFace& font, uint32_t codepoint);

    /**
     * @brief Generate SDF atlas for a character set
     */
    void generate_atlas(FontFace& font, const std::string& charset, SDFAtlas& atlas);

    /**
     * @brief Generate SDF atlas for ASCII + common characters
     */
    void generate_default_atlas(FontFace& font, SDFAtlas& atlas);

    /**
     * @brief Generate SDF atlas for Unicode range
     */
    void generate_unicode_range(FontFace& font, uint32_t start, uint32_t end, SDFAtlas& atlas);

private:
    /**
     * @brief Render high-resolution glyph bitmap
     */
    std::vector<uint8_t> render_highres_glyph(FontFace& font, uint32_t codepoint,
                                               int& width, int& height);

    /**
     * @brief Generate SDF from bitmap using 8SSEDT algorithm
     */
    std::vector<uint8_t> bitmap_to_sdf(const std::vector<uint8_t>& bitmap,
                                        int bitmap_width, int bitmap_height,
                                        int sdf_width, int sdf_height);

    /**
     * @brief Generate MSDF from glyph outline
     */
    std::vector<uint8_t> generate_msdf(FontFace& font, uint32_t codepoint,
                                        int width, int height);

    /**
     * @brief Compute distance field using 8SSEDT
     * Eight-point Sequential Signed Distance Transform
     */
    void compute_distance_field(const std::vector<float>& input,
                                std::vector<float>& output,
                                int width, int height);

    SDFConfig config_;
};

/**
 * @brief Global SDF atlas manager
 *
 * Manages SDF atlases for different fonts/sizes to avoid regeneration.
 */
class SDFAtlasManager {
public:
    /**
     * @brief Get singleton instance
     */
    static SDFAtlasManager& instance();

    /**
     * @brief Get or create atlas for a font
     */
    SDFAtlas* get_atlas(const std::string& font_family, int size_hint = 48);

    /**
     * @brief Ensure character is in atlas
     */
    bool ensure_glyph(const std::string& font_family, uint32_t codepoint);

    /**
     * @brief Ensure all characters in string are in atlas
     */
    bool ensure_glyphs(const std::string& font_family, const std::string& text);

    /**
     * @brief Upload all dirty atlases to GPU
     */
    void upload_dirty_atlases(MemoryPool& pool);

    /**
     * @brief Clear all atlases
     */
    void clear();

    /**
     * @brief Set SDF configuration
     */
    void set_config(const SDFConfig& config) { config_ = config; }

private:
    SDFAtlasManager() = default;

    struct AtlasKey {
        std::string font_family;
        int size_hint;

        bool operator==(const AtlasKey& other) const {
            return font_family == other.font_family && size_hint == other.size_hint;
        }
    };

    struct AtlasKeyHash {
        size_t operator()(const AtlasKey& key) const {
            return std::hash<std::string>()(key.font_family) ^ (std::hash<int>()(key.size_hint) << 1);
        }
    };

    std::unordered_map<AtlasKey, std::unique_ptr<SDFAtlas>, AtlasKeyHash> atlases_;
    SDFGenerator generator_;
    SDFConfig config_;
};

} // namespace manim
