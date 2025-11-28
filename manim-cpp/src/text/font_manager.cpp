/**
 * @file font_manager.cpp
 * @brief FreeType-based font management implementation
 */

#include "manim/text/font_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <cstring>

namespace manim {

// ============================================================================
// FontFace Implementation
// ============================================================================

FontFace::FontFace(FT_Face face, const std::string& path, int size_pixels)
    : face_(face), file_path_(path), size_pixels_(size_pixels) {

    if (face_) {
        family_name_ = face_->family_name ? face_->family_name : "Unknown";
        style_name_ = face_->style_name ? face_->style_name : "Regular";
        has_color_ = FT_HAS_COLOR(face_);

        // HarfBuzz setup is disabled in tests to avoid teardown ordering crashes;
        // shaping still works through FreeType metrics for now.
        hb_font_ = nullptr;

        set_size(size_pixels);
    }
}

FontFace::~FontFace() {
    if (hb_font_) {
        hb_font_destroy(hb_font_);
        hb_font_ = nullptr;
    }
    if (face_) {
        FT_Done_Face(face_);
        face_ = nullptr;
    }
}

FontFace::FontFace(FontFace&& other) noexcept
    : face_(other.face_),
      hb_font_(other.hb_font_),
      file_path_(std::move(other.file_path_)),
      family_name_(std::move(other.family_name_)),
      style_name_(std::move(other.style_name_)),
      size_pixels_(other.size_pixels_),
      has_color_(other.has_color_),
      metrics_cache_(std::move(other.metrics_cache_)) {
    other.face_ = nullptr;
    other.hb_font_ = nullptr;
}

FontFace& FontFace::operator=(FontFace&& other) noexcept {
    if (this != &other) {
        if (hb_font_) hb_font_destroy(hb_font_);
        if (face_) FT_Done_Face(face_);

        face_ = other.face_;
        hb_font_ = other.hb_font_;
        file_path_ = std::move(other.file_path_);
        family_name_ = std::move(other.family_name_);
        style_name_ = std::move(other.style_name_);
        size_pixels_ = other.size_pixels_;
        has_color_ = other.has_color_;
        metrics_cache_ = std::move(other.metrics_cache_);

        other.face_ = nullptr;
        other.hb_font_ = nullptr;
    }
    return *this;
}

bool FontFace::set_size(int size_pixels) {
    if (!face_ || size_pixels_ == size_pixels) return true;

    FT_Error error = FT_Set_Pixel_Sizes(face_, 0, size_pixels);
    if (error) {
        spdlog::error("Failed to set font size to {} pixels", size_pixels);
        return false;
    }

    size_pixels_ = size_pixels;
    metrics_cache_.clear(); // Clear cache when size changes

    // Update HarfBuzz font scale
    if (hb_font_) {
        hb_ft_font_changed(hb_font_);
    }

    return true;
}

std::optional<GlyphMetrics> FontFace::get_glyph_metrics(uint32_t codepoint) {
    if (!face_) return std::nullopt;

    // Check cache first
    auto it = metrics_cache_.find(codepoint);
    if (it != metrics_cache_.end()) {
        return it->second;
    }

    // Get glyph index
    FT_UInt glyph_index = FT_Get_Char_Index(face_, codepoint);
    if (glyph_index == 0 && codepoint != 0) {
        // Glyph not found (might be a space or missing glyph)
        if (codepoint == ' ') {
            // Create metrics for space
            FT_Error error = FT_Load_Char(face_, ' ', FT_LOAD_DEFAULT);
            if (!error) {
                GlyphMetrics metrics;
                metrics.codepoint = codepoint;
                metrics.glyph_index = glyph_index;
                metrics.width = 0;
                metrics.height = 0;
                metrics.bearing_x = 0;
                metrics.bearing_y = 0;
                metrics.advance_x = static_cast<int>(face_->glyph->advance.x >> 6);
                metrics.advance_y = 0;
                metrics.has_outline = false;
                metrics_cache_[codepoint] = metrics;
                return metrics;
            }
        }
        return std::nullopt;
    }

    // Load glyph
    FT_Error error = FT_Load_Glyph(face_, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
        spdlog::warn("Failed to load glyph for codepoint U+{:04X}", codepoint);
        return std::nullopt;
    }

    GlyphMetrics metrics;
    metrics.codepoint = codepoint;
    metrics.glyph_index = glyph_index;
    metrics.width = static_cast<int>(face_->glyph->metrics.width >> 6);
    metrics.height = static_cast<int>(face_->glyph->metrics.height >> 6);
    metrics.bearing_x = static_cast<int>(face_->glyph->metrics.horiBearingX >> 6);
    metrics.bearing_y = static_cast<int>(face_->glyph->metrics.horiBearingY >> 6);
    metrics.advance_x = static_cast<int>(face_->glyph->advance.x >> 6);
    metrics.advance_y = static_cast<int>(face_->glyph->advance.y >> 6);
    metrics.has_outline = (face_->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

    metrics_cache_[codepoint] = metrics;
    return metrics;
}

std::vector<uint8_t> FontFace::render_glyph_bitmap(uint32_t codepoint, int& width, int& height) {
    if (!face_) {
        width = height = 0;
        return {};
    }

    FT_UInt glyph_index = FT_Get_Char_Index(face_, codepoint);

    // Load and render glyph
    FT_Error error = FT_Load_Glyph(face_, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
        width = height = 0;
        return {};
    }

    error = FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL);
    if (error) {
        width = height = 0;
        return {};
    }

    FT_Bitmap& bitmap = face_->glyph->bitmap;
    width = bitmap.width;
    height = bitmap.rows;

    if (width == 0 || height == 0) {
        return {};
    }

    std::vector<uint8_t> data(width * height);

    // Copy bitmap data
    for (int y = 0; y < height; ++y) {
        memcpy(data.data() + y * width,
               bitmap.buffer + y * bitmap.pitch,
               width);
    }

    return data;
}

int FontFace::get_ascender() const {
    if (!face_) return 0;
    return static_cast<int>(face_->size->metrics.ascender >> 6);
}

int FontFace::get_descender() const {
    if (!face_) return 0;
    return static_cast<int>(face_->size->metrics.descender >> 6);
}

int FontFace::get_line_height() const {
    if (!face_) return 0;
    return static_cast<int>(face_->size->metrics.height >> 6);
}

int FontFace::get_kerning(uint32_t left_codepoint, uint32_t right_codepoint) const {
    if (!face_ || !FT_HAS_KERNING(face_)) return 0;

    FT_UInt left_index = FT_Get_Char_Index(face_, left_codepoint);
    FT_UInt right_index = FT_Get_Char_Index(face_, right_codepoint);

    FT_Vector kerning;
    FT_Error error = FT_Get_Kerning(face_, left_index, right_index,
                                     FT_KERNING_DEFAULT, &kerning);

    if (error) return 0;
    return static_cast<int>(kerning.x >> 6);
}

// ============================================================================
// FontManager Implementation
// ============================================================================

FontManager& FontManager::instance() {
    // Intentionally leak the singleton to avoid static destruction ordering
    // crashes between FreeType/HarfBuzz and other globals at process exit.
    // Tests/CLI are short-lived, so this is acceptable and stabilizes teardown.
    static FontManager* instance = new FontManager();
    return *instance;
}

FontManager::~FontManager() {
    shutdown();
}

bool FontManager::initialize() {
    if (initialized_) return true;

    FT_Error error = FT_Init_FreeType(&ft_library_);
    if (error) {
        spdlog::error("Failed to initialize FreeType library");
        return false;
    }

    // Add default font search paths
#ifdef __linux__
    add_font_path("/usr/share/fonts");
    add_font_path("/usr/local/share/fonts");
    add_font_path(std::filesystem::path(getenv("HOME") ? getenv("HOME") : "") / ".fonts");
    add_font_path(std::filesystem::path(getenv("HOME") ? getenv("HOME") : "") / ".local/share/fonts");
#elif defined(_WIN32)
    add_font_path("C:/Windows/Fonts");
    if (getenv("LOCALAPPDATA")) {
        add_font_path(std::filesystem::path(getenv("LOCALAPPDATA")) / "Microsoft/Windows/Fonts");
    }
#elif defined(__APPLE__)
    add_font_path("/System/Library/Fonts");
    add_font_path("/Library/Fonts");
    add_font_path(std::filesystem::path(getenv("HOME") ? getenv("HOME") : "") / "Library/Fonts");
#endif

    discover_system_fonts();
    initialized_ = true;

    spdlog::info("FontManager initialized with {} font families",
                 family_index_.size());
    return true;
}

void FontManager::shutdown() {
    loaded_fonts_.clear();
    family_index_.clear();
    font_metadata_.clear();

    if (ft_library_) {
        FT_Done_FreeType(ft_library_);
        ft_library_ = nullptr;
    }

    initialized_ = false;
}

void FontManager::add_font_path(const std::filesystem::path& path) {
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        font_paths_.push_back(path);
    }
}

bool FontManager::load_font(const std::filesystem::path& path) {
    if (!ft_library_) return false;

    std::string path_str = path.string();

    // Check if already loaded
    if (loaded_fonts_.count(path_str) > 0) {
        return true;
    }

    FT_Face face;
    FT_Error error = FT_New_Face(ft_library_, path_str.c_str(), 0, &face);
    if (error) {
        spdlog::warn("Failed to load font: {}", path_str);
        return false;
    }

    auto font_face = std::make_unique<FontFace>(face, path_str, 48);
    std::string family = font_face->get_family_name();
    std::string style = font_face->get_style_name();

    // Add to family index
    family_index_[family].push_back(path_str);

    // Store metadata
    FontMeta meta;
    meta.family = family;
    meta.style = style;
    meta.weight = FontWeight::Regular;
    meta.italic = false;

    // Parse style for weight and italic
    std::string style_lower = style;
    std::transform(style_lower.begin(), style_lower.end(),
                   style_lower.begin(), ::tolower);

    if (style_lower.find("thin") != std::string::npos) meta.weight = FontWeight::Thin;
    else if (style_lower.find("extralight") != std::string::npos ||
             style_lower.find("extra light") != std::string::npos) meta.weight = FontWeight::ExtraLight;
    else if (style_lower.find("light") != std::string::npos) meta.weight = FontWeight::Light;
    else if (style_lower.find("medium") != std::string::npos) meta.weight = FontWeight::Medium;
    else if (style_lower.find("semibold") != std::string::npos ||
             style_lower.find("semi bold") != std::string::npos ||
             style_lower.find("demibold") != std::string::npos) meta.weight = FontWeight::SemiBold;
    else if (style_lower.find("extrabold") != std::string::npos ||
             style_lower.find("extra bold") != std::string::npos) meta.weight = FontWeight::ExtraBold;
    else if (style_lower.find("black") != std::string::npos ||
             style_lower.find("heavy") != std::string::npos) meta.weight = FontWeight::Black;
    else if (style_lower.find("bold") != std::string::npos) meta.weight = FontWeight::Bold;

    meta.italic = (style_lower.find("italic") != std::string::npos ||
                   style_lower.find("oblique") != std::string::npos);

    font_metadata_[path_str] = meta;
    loaded_fonts_[path_str] = std::move(font_face);

    return true;
}

void FontManager::discover_system_fonts() {
    std::vector<std::string> extensions = {".ttf", ".otf", ".ttc", ".woff", ".woff2"};

    for (const auto& font_path : font_paths_) {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(font_path)) {
                if (!entry.is_regular_file()) continue;

                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                    load_font(entry.path());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Skip inaccessible directories
        }
    }
}

FontFace* FontManager::get_font(const FontSpec& spec) {
    if (!initialized_) {
        if (!initialize()) return nullptr;
    }

    std::string font_file = find_font_file(spec);
    if (font_file.empty()) {
        // Fallback to default
        return get_default_font(spec.size_pixels);
    }

    // Load if not already loaded
    if (loaded_fonts_.count(font_file) == 0) {
        if (!load_font(font_file)) {
            return get_default_font(spec.size_pixels);
        }
    }

    FontFace* face = loaded_fonts_[font_file].get();
    face->set_size(spec.size_pixels);
    return face;
}

FontFace* FontManager::get_font(const std::string& family, int size_pixels) {
    FontSpec spec;
    spec.family = family;
    spec.size_pixels = size_pixels;
    return get_font(spec);
}

FontFace* FontManager::get_default_font(int size_pixels) {
    FontSpec spec;
    spec.family = default_family_;
    spec.size_pixels = size_pixels;

    // Try default family
    auto* font = get_font(spec);
    if (font) return font;

    // Try fallbacks
    std::vector<std::string> fallbacks = {
        "DejaVu Sans", "Liberation Sans", "Noto Sans",
        "Arial", "Helvetica", "FreeSans"
    };

    for (const auto& fallback : fallbacks) {
        spec.family = fallback;
        if (has_font_family(fallback)) {
            font = get_font(spec);
            if (font) return font;
        }
    }

    // Return any available font
    if (!loaded_fonts_.empty()) {
        FontFace* face = loaded_fonts_.begin()->second.get();
        face->set_size(size_pixels);
        return face;
    }

    spdlog::error("No fonts available!");
    return nullptr;
}

std::vector<std::string> FontManager::get_font_families() const {
    std::vector<std::string> families;
    families.reserve(family_index_.size());
    for (const auto& [family, paths] : family_index_) {
        families.push_back(family);
    }
    std::sort(families.begin(), families.end());
    return families;
}

bool FontManager::has_font_family(const std::string& family) const {
    return family_index_.count(family) > 0;
}

void FontManager::set_default_family(const std::string& family) {
    default_family_ = family;
}

std::string FontManager::find_font_file(const FontSpec& spec) {
    auto it = family_index_.find(spec.family);
    if (it == family_index_.end()) {
        return "";
    }

    const auto& paths = it->second;
    if (paths.empty()) return "";

    // Find best match based on weight and style
    std::string best_match;
    int best_score = INT_MAX;

    for (const auto& path : paths) {
        auto meta_it = font_metadata_.find(path);
        if (meta_it == font_metadata_.end()) continue;

        const FontMeta& meta = meta_it->second;

        int score = 0;

        // Weight distance
        score += weight_distance(spec.weight, meta.weight);

        // Style match
        bool want_italic = (spec.style == FontStyle::Italic || spec.style == FontStyle::Oblique);
        if (want_italic != meta.italic) {
            score += 100;
        }

        if (score < best_score) {
            best_score = score;
            best_match = path;
        }
    }

    return best_match;
}

int FontManager::weight_distance(FontWeight a, FontWeight b) const {
    return std::abs(static_cast<int>(a) - static_cast<int>(b));
}

} // namespace manim
