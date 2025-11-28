/**
 * @file sdf_generator.cpp
 * @brief Signed Distance Field generator implementation
 *
 * Uses 8SSEDT (8-point Sequenced Signed Euclidean Distance Transform)
 * for high-quality SDF generation.
 */

#include "manim/text/sdf_generator.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace manim {

// ============================================================================
// SDFAtlas Implementation
// ============================================================================

void SDFAtlas::initialize(int width, int height, int channels) {
    width_ = width;
    height_ = height;
    channels_ = channels;

    texture_data_.resize(width * height * channels, 0);
    entries_.clear();

    current_row_y_ = 0;
    current_row_height_ = 0;
    current_x_ = 0;
    dirty_ = true;
}

bool SDFAtlas::add_glyph(const SDFGlyph& glyph) {
    if (glyph.width == 0 || glyph.height == 0) {
        // Empty glyph (like space), just store metrics
        AtlasEntry entry;
        entry.codepoint = glyph.codepoint;
        entry.x = 0;
        entry.y = 0;
        entry.width = 0;
        entry.height = 0;
        entry.metrics = glyph.metrics;
        entries_[glyph.codepoint] = entry;
        return true;
    }

    // Find space in atlas
    int x, y;
    if (!find_space(glyph.width, glyph.height, x, y)) {
        spdlog::warn("SDF atlas is full, cannot add glyph U+{:04X}", glyph.codepoint);
        return false;
    }

    // Copy glyph data to atlas
    copy_glyph_to_texture(glyph, x, y);

    // Create entry
    AtlasEntry entry;
    entry.codepoint = glyph.codepoint;
    entry.x = x;
    entry.y = y;
    entry.width = glyph.width;
    entry.height = glyph.height;
    entry.metrics = glyph.metrics;

    // Set UV coordinates
    entry.metrics.u0 = static_cast<float>(x) / width_;
    entry.metrics.v0 = static_cast<float>(y) / height_;
    entry.metrics.u1 = static_cast<float>(x + glyph.width) / width_;
    entry.metrics.v1 = static_cast<float>(y + glyph.height) / height_;

    entries_[glyph.codepoint] = entry;
    dirty_ = true;

    return true;
}

const AtlasEntry* SDFAtlas::get_entry(uint32_t codepoint) const {
    auto it = entries_.find(codepoint);
    if (it == entries_.end()) return nullptr;
    return &it->second;
}

bool SDFAtlas::has_glyph(uint32_t codepoint) const {
    return entries_.count(codepoint) > 0;
}

void SDFAtlas::upload_to_gpu(MemoryPool& pool) {
    if (!dirty_) return;

    // Create GPU image
    gpu_texture_ = pool.allocate_image(
        width_, height_,
        channels_ == 1 ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal
    );

    // Upload texture data to GPU image
    if (!texture_data_.empty()) {
        pool.upload_to_image(gpu_texture_, texture_data_.data(), texture_data_.size());
        spdlog::info("Uploaded SDF atlas {}x{} ({} glyphs) to GPU",
                    width_, height_, entries_.size());
    }

    dirty_ = false;
}

bool SDFAtlas::find_space(int width, int height, int& out_x, int& out_y) {
    // Simple row-based packing
    const int padding = 2; // Padding between glyphs

    if (current_x_ + width + padding > width_) {
        // Move to next row
        current_row_y_ += current_row_height_ + padding;
        current_row_height_ = 0;
        current_x_ = 0;
    }

    if (current_row_y_ + height + padding > height_) {
        // Atlas is full
        return false;
    }

    out_x = current_x_;
    out_y = current_row_y_;

    current_x_ += width + padding;
    current_row_height_ = std::max(current_row_height_, height);

    return true;
}

void SDFAtlas::copy_glyph_to_texture(const SDFGlyph& glyph, int x, int y) {
    for (int row = 0; row < glyph.height; ++row) {
        int src_offset = row * glyph.width * glyph.channels;
        int dst_offset = ((y + row) * width_ + x) * channels_;

        if (channels_ == glyph.channels) {
            std::memcpy(texture_data_.data() + dst_offset,
                       glyph.sdf_data.data() + src_offset,
                       glyph.width * channels_);
        } else if (channels_ == 1 && glyph.channels >= 1) {
            // Take first channel
            for (int col = 0; col < glyph.width; ++col) {
                texture_data_[dst_offset + col] = glyph.sdf_data[src_offset + col * glyph.channels];
            }
        }
    }
}

// ============================================================================
// SDFGenerator Implementation
// ============================================================================

SDFGenerator::SDFGenerator(const SDFConfig& config)
    : config_(config) {}

SDFGlyph SDFGenerator::generate_glyph_sdf(FontFace& font, uint32_t codepoint) {
    SDFGlyph result;
    result.codepoint = codepoint;

    // Get metrics
    auto metrics_opt = font.get_glyph_metrics(codepoint);
    if (!metrics_opt) {
        return result;
    }
    result.metrics = *metrics_opt;

    // Skip empty glyphs (like space)
    if (!result.metrics.has_outline || result.metrics.width == 0 || result.metrics.height == 0) {
        result.width = 0;
        result.height = 0;
        result.channels = 1;
        return result;
    }

    // Generate high-resolution bitmap
    int highres_width, highres_height;
    auto highres_bitmap = render_highres_glyph(font, codepoint, highres_width, highres_height);

    if (highres_bitmap.empty()) {
        return result;
    }

    // Calculate SDF dimensions with padding
    int sdf_width = static_cast<int>(std::ceil(static_cast<float>(highres_width) / config_.supersample))
                    + config_.sdf_padding * 2;
    int sdf_height = static_cast<int>(std::ceil(static_cast<float>(highres_height) / config_.supersample))
                     + config_.sdf_padding * 2;

    // Generate SDF from bitmap
    result.sdf_data = bitmap_to_sdf(highres_bitmap, highres_width, highres_height,
                                     sdf_width, sdf_height);
    result.width = sdf_width;
    result.height = sdf_height;
    result.channels = 1;
    result.metrics.sdf_padding = config_.sdf_padding;

    return result;
}

void SDFGenerator::generate_atlas(FontFace& font, const std::string& charset, SDFAtlas& atlas) {
    // Convert UTF-8 string to codepoints
    std::vector<uint32_t> codepoints;
    const char* ptr = charset.c_str();
    const char* end = ptr + charset.length();

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

    // Generate SDF for each unique codepoint
    std::sort(codepoints.begin(), codepoints.end());
    codepoints.erase(std::unique(codepoints.begin(), codepoints.end()), codepoints.end());

    for (uint32_t cp : codepoints) {
        if (!atlas.has_glyph(cp)) {
            SDFGlyph glyph = generate_glyph_sdf(font, cp);
            atlas.add_glyph(glyph);
        }
    }
}

void SDFGenerator::generate_default_atlas(FontFace& font, SDFAtlas& atlas) {
    // ASCII printable characters
    std::string ascii;
    for (char c = 32; c < 127; ++c) {
        ascii += c;
    }

    // Common punctuation and symbols
    ascii += "€£¥©®™°±×÷";

    // Common accented characters
    ascii += "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ";

    generate_atlas(font, ascii, atlas);
}

void SDFGenerator::generate_unicode_range(FontFace& font, uint32_t start, uint32_t end, SDFAtlas& atlas) {
    for (uint32_t cp = start; cp <= end; ++cp) {
        if (!atlas.has_glyph(cp)) {
            SDFGlyph glyph = generate_glyph_sdf(font, cp);
            if (glyph.metrics.has_outline || cp == ' ') {
                atlas.add_glyph(glyph);
            }
        }
    }
}

std::vector<uint8_t> SDFGenerator::render_highres_glyph(FontFace& font, uint32_t codepoint,
                                                         int& width, int& height) {
    // Temporarily increase font size for higher resolution
    int original_size = font.get_size_pixels();
    int highres_size = original_size * config_.supersample;

    font.set_size(highres_size);
    auto bitmap = font.render_glyph_bitmap(codepoint, width, height);
    font.set_size(original_size);

    return bitmap;
}

std::vector<uint8_t> SDFGenerator::bitmap_to_sdf(const std::vector<uint8_t>& bitmap,
                                                  int bitmap_width, int bitmap_height,
                                                  int sdf_width, int sdf_height) {
    // Create distance fields
    std::vector<float> outside_dist(sdf_width * sdf_height, std::numeric_limits<float>::max());
    std::vector<float> inside_dist(sdf_width * sdf_height, std::numeric_limits<float>::max());

    float scale_x = static_cast<float>(bitmap_width) / (sdf_width - config_.sdf_padding * 2);
    float scale_y = static_cast<float>(bitmap_height) / (sdf_height - config_.sdf_padding * 2);

    // Compute distance for each SDF pixel
    for (int sy = 0; sy < sdf_height; ++sy) {
        for (int sx = 0; sx < sdf_width; ++sx) {
            // Map to bitmap coordinates
            float bx = (sx - config_.sdf_padding) * scale_x;
            float by = (sy - config_.sdf_padding) * scale_y;

            int sdf_idx = sy * sdf_width + sx;

            // Find minimum distance to edge
            float min_dist = std::numeric_limits<float>::max();
            bool is_inside = false;

            // Sample bitmap at this location
            int bxi = static_cast<int>(bx);
            int byi = static_cast<int>(by);

            if (bxi >= 0 && bxi < bitmap_width && byi >= 0 && byi < bitmap_height) {
                is_inside = bitmap[byi * bitmap_width + bxi] > 127;
            }

            // Search for nearest edge within search radius
            int search_radius = static_cast<int>(config_.sdf_range * scale_x * 2);

            for (int dy = -search_radius; dy <= search_radius; ++dy) {
                for (int dx = -search_radius; dx <= search_radius; ++dx) {
                    int check_bx = bxi + dx;
                    int check_by = byi + dy;

                    if (check_bx < 0 || check_bx >= bitmap_width ||
                        check_by < 0 || check_by >= bitmap_height) continue;

                    bool check_inside = bitmap[check_by * bitmap_width + check_bx] > 127;

                    if (check_inside != is_inside) {
                        // Found an edge pixel
                        float dist_x = (bx - check_bx) / scale_x;
                        float dist_y = (by - check_by) / scale_y;
                        float dist = std::sqrt(dist_x * dist_x + dist_y * dist_y);
                        min_dist = std::min(min_dist, dist);
                    }
                }
            }

            if (is_inside) {
                inside_dist[sdf_idx] = min_dist;
            } else {
                outside_dist[sdf_idx] = min_dist;
            }
        }
    }

    // Refine distance fields using 8SSEDT
    compute_distance_field(outside_dist, outside_dist, sdf_width, sdf_height);
    compute_distance_field(inside_dist, inside_dist, sdf_width, sdf_height);

    // Create final SDF
    std::vector<uint8_t> sdf(sdf_width * sdf_height);
    float range = config_.sdf_range;

    for (int i = 0; i < sdf_width * sdf_height; ++i) {
        float outside = outside_dist[i];
        float inside = inside_dist[i];

        // Signed distance: negative inside, positive outside
        float signed_dist;
        if (inside < std::numeric_limits<float>::max() / 2) {
            signed_dist = -inside;
        } else if (outside < std::numeric_limits<float>::max() / 2) {
            signed_dist = outside;
        } else {
            signed_dist = 0;
        }

        // Map to [0, 1] range then to [0, 255]
        float normalized = 0.5f + signed_dist / (2.0f * range);
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        sdf[i] = static_cast<uint8_t>(normalized * 255.0f);
    }

    return sdf;
}

void SDFGenerator::compute_distance_field(const std::vector<float>& input,
                                           std::vector<float>& output,
                                           int width, int height) {
    // 8SSEDT: Two-pass algorithm for computing Euclidean distance transform

    output = input;

    // First pass: forward scan
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;

            // Check 4 neighbors in forward direction
            if (x > 0) {
                float d = output[(y) * width + (x - 1)] + 1.0f;
                output[idx] = std::min(output[idx], d);
            }
            if (y > 0) {
                float d = output[(y - 1) * width + x] + 1.0f;
                output[idx] = std::min(output[idx], d);
            }
            if (x > 0 && y > 0) {
                float d = output[(y - 1) * width + (x - 1)] + 1.414f;
                output[idx] = std::min(output[idx], d);
            }
            if (x < width - 1 && y > 0) {
                float d = output[(y - 1) * width + (x + 1)] + 1.414f;
                output[idx] = std::min(output[idx], d);
            }
        }
    }

    // Second pass: backward scan
    for (int y = height - 1; y >= 0; --y) {
        for (int x = width - 1; x >= 0; --x) {
            int idx = y * width + x;

            // Check 4 neighbors in backward direction
            if (x < width - 1) {
                float d = output[(y) * width + (x + 1)] + 1.0f;
                output[idx] = std::min(output[idx], d);
            }
            if (y < height - 1) {
                float d = output[(y + 1) * width + x] + 1.0f;
                output[idx] = std::min(output[idx], d);
            }
            if (x < width - 1 && y < height - 1) {
                float d = output[(y + 1) * width + (x + 1)] + 1.414f;
                output[idx] = std::min(output[idx], d);
            }
            if (x > 0 && y < height - 1) {
                float d = output[(y + 1) * width + (x - 1)] + 1.414f;
                output[idx] = std::min(output[idx], d);
            }
        }
    }
}

// ============================================================================
// SDFAtlasManager Implementation
// ============================================================================

SDFAtlasManager& SDFAtlasManager::instance() {
    static SDFAtlasManager instance;
    return instance;
}

SDFAtlas* SDFAtlasManager::get_atlas(const std::string& font_family, int size_hint) {
    AtlasKey key{font_family, size_hint};

    auto it = atlases_.find(key);
    if (it != atlases_.end()) {
        return it->second.get();
    }

    // Create new atlas
    auto atlas = std::make_unique<SDFAtlas>();
    atlas->initialize(2048, 2048, 1);  // 2K atlas

    // Generate default glyphs
    FontFace* font = FontManager::instance().get_font(font_family, config_.sdf_size);
    if (font) {
        generator_.set_config(config_);
        generator_.generate_default_atlas(*font, *atlas);
    }

    SDFAtlas* ptr = atlas.get();
    atlases_[key] = std::move(atlas);
    return ptr;
}

bool SDFAtlasManager::ensure_glyph(const std::string& font_family, uint32_t codepoint) {
    SDFAtlas* atlas = get_atlas(font_family);
    if (!atlas) return false;

    if (atlas->has_glyph(codepoint)) return true;

    FontFace* font = FontManager::instance().get_font(font_family, config_.sdf_size);
    if (!font) return false;

    SDFGlyph glyph = generator_.generate_glyph_sdf(*font, codepoint);
    return atlas->add_glyph(glyph);
}

bool SDFAtlasManager::ensure_glyphs(const std::string& font_family, const std::string& text) {
    SDFAtlas* atlas = get_atlas(font_family);
    if (!atlas) return false;

    FontFace* font = FontManager::instance().get_font(font_family, config_.sdf_size);
    if (!font) return false;

    generator_.generate_atlas(*font, text, *atlas);
    return true;
}

void SDFAtlasManager::upload_dirty_atlases(MemoryPool& pool) {
    for (auto& [key, atlas] : atlases_) {
        if (atlas->is_dirty()) {
            atlas->upload_to_gpu(pool);
        }
    }
}

void SDFAtlasManager::clear() {
    atlases_.clear();
}

} // namespace manim
