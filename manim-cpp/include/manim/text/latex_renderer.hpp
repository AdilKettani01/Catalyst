/**
 * @file latex_renderer.hpp
 * @brief LaTeX formula rendering support
 *
 * Provides LaTeX rendering capabilities via:
 * 1. External LaTeX compiler (pdflatex/xelatex) -> dvisvgm -> SVG parsing
 * 2. Built-in math symbol rendering using math fonts
 * 3. SVG path extraction for VMobject rendering
 */

#pragma once

#include "manim/mobject/vmobject.hpp"
#include "manim/text/font_manager.hpp"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>

namespace manim {

/**
 * @brief LaTeX compilation configuration
 */
struct LaTeXConfig {
    std::string latex_compiler = "pdflatex";    // or xelatex, lualatex
    std::string dvi_converter = "dvisvgm";
    std::string temp_dir = "/tmp/manim_latex";
    std::string preamble = R"(
\documentclass[preview]{standalone}
\usepackage{amsmath}
\usepackage{amssymb}
\usepackage{amsfonts}
\begin{document}
)";
    std::string postamble = R"(
\end{document}
)";
    bool cache_enabled = true;
    bool use_external_compiler = true;
    float scale = 0.05f;  // Scale factor for SVG to scene coordinates
};

/**
 * @brief SVG path segment
 */
struct SVGPathSegment {
    char command;  // M, L, C, Q, Z, etc.
    std::vector<float> values;
};

/**
 * @brief Parsed SVG path
 */
struct SVGPath {
    std::vector<SVGPathSegment> segments;
    math::Vec4 fill_color{0, 0, 0, 0};
    math::Vec4 stroke_color{1, 1, 1, 1};
    float stroke_width = 0.0f;

    // Bounding box
    float min_x = 0, min_y = 0;
    float max_x = 0, max_y = 0;
};

/**
 * @brief LaTeX compilation result
 */
struct LaTeXResult {
    bool success = false;
    std::string error_message;
    std::vector<SVGPath> paths;
    float width = 0;
    float height = 0;
};

/**
 * @brief Math symbol data for built-in rendering
 */
struct MathSymbol {
    std::string name;           // e.g., "alpha", "beta", "sum"
    uint32_t codepoint;         // Unicode codepoint
    std::string font_family;    // Font to use
    bool is_operator;           // Is it an operator (affects spacing)
    float scale = 1.0f;         // Relative size
};

/**
 * @brief LaTeX renderer
 *
 * Compiles LaTeX formulas to SVG paths that can be rendered as VMobjects.
 */
class LaTeXRenderer {
public:
    /**
     * @brief Get singleton instance
     */
    static LaTeXRenderer& instance();

    /**
     * @brief Set configuration
     */
    void set_config(const LaTeXConfig& config) { config_ = config; }
    const LaTeXConfig& get_config() const { return config_; }

    /**
     * @brief Render LaTeX formula to paths
     */
    LaTeXResult render(const std::string& latex);

    /**
     * @brief Render LaTeX formula and create VMobject
     */
    std::shared_ptr<VMobject> render_to_mobject(const std::string& latex);

    /**
     * @brief Check if external LaTeX compiler is available
     */
    bool is_compiler_available() const;

    /**
     * @brief Clear cache
     */
    void clear_cache();

    /**
     * @brief Get cached result if available
     */
    const LaTeXResult* get_cached(const std::string& latex) const;

private:
    LaTeXRenderer() = default;

    /**
     * @brief Compile LaTeX using external compiler
     */
    LaTeXResult compile_external(const std::string& latex);

    /**
     * @brief Parse SVG file to paths
     */
    std::vector<SVGPath> parse_svg(const std::filesystem::path& svg_path);

    /**
     * @brief Parse SVG path data string
     */
    SVGPath parse_svg_path_data(const std::string& d);

    /**
     * @brief Convert SVG paths to VMobject Bezier curves
     */
    void paths_to_vmobject(const std::vector<SVGPath>& paths, VMobject& vmobject);

    /**
     * @brief Render simple math using built-in fonts (fallback)
     */
    LaTeXResult render_simple_math(const std::string& latex);

    /**
     * @brief Parse simple LaTeX math expressions
     */
    std::vector<MathSymbol> parse_simple_latex(const std::string& latex);

    /**
     * @brief Compute hash for caching
     */
    std::string compute_hash(const std::string& latex) const;

    LaTeXConfig config_;
    std::unordered_map<std::string, LaTeXResult> cache_;

    // Math symbol lookup table
    static const std::unordered_map<std::string, MathSymbol> math_symbols_;
};

/**
 * @brief SVG parser utilities
 */
class SVGParser {
public:
    /**
     * @brief Parse SVG file
     */
    static std::vector<SVGPath> parse_file(const std::filesystem::path& path);

    /**
     * @brief Parse SVG string
     */
    static std::vector<SVGPath> parse_string(const std::string& svg);

    /**
     * @brief Parse path data (d attribute)
     */
    static SVGPath parse_path_data(const std::string& d);

    /**
     * @brief Convert SVG path to Bezier control points
     */
    static std::vector<math::Vec3> path_to_beziers(const SVGPath& path);

private:
    /**
     * @brief Parse number from string
     */
    static float parse_number(const char*& ptr);

    /**
     * @brief Skip whitespace and commas
     */
    static void skip_separators(const char*& ptr);

    /**
     * @brief Convert quadratic Bezier to cubic
     */
    static void quadratic_to_cubic(const math::Vec3& p0, const math::Vec3& p1, const math::Vec3& p2,
                                    math::Vec3& c1, math::Vec3& c2);

    /**
     * @brief Arc to Bezier conversion
     */
    static std::vector<math::Vec3> arc_to_beziers(const math::Vec3& start,
                                                   float rx, float ry, float rotation,
                                                   bool large_arc, bool sweep,
                                                   const math::Vec3& end);
};

/**
 * @brief Built-in math font support
 *
 * Uses STIX or Latin Modern Math fonts for rendering
 * when external LaTeX compiler is not available.
 */
class MathFontRenderer {
public:
    /**
     * @brief Initialize math font support
     */
    bool initialize();

    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Render math expression using fonts
     */
    std::shared_ptr<VMobject> render(const std::string& expression);

    /**
     * @brief Get font for math symbols
     */
    FontFace* get_math_font();

private:
    bool initialized_ = false;
    std::string math_font_family_ = "STIX Two Math";

    // Greek letter mappings
    static const std::unordered_map<std::string, uint32_t> greek_letters_;

    // Operator mappings
    static const std::unordered_map<std::string, uint32_t> operators_;
};

} // namespace manim
