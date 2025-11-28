/**
 * @file latex_renderer.cpp
 * @brief LaTeX formula rendering implementation
 */

#include "manim/text/latex_renderer.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <regex>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace manim {

// ============================================================================
// Math Symbol Tables
// ============================================================================

const std::unordered_map<std::string, MathSymbol> LaTeXRenderer::math_symbols_ = {
    // Greek lowercase
    {"alpha", {"alpha", 0x03B1, "STIX Two Math", false, 1.0f}},
    {"beta", {"beta", 0x03B2, "STIX Two Math", false, 1.0f}},
    {"gamma", {"gamma", 0x03B3, "STIX Two Math", false, 1.0f}},
    {"delta", {"delta", 0x03B4, "STIX Two Math", false, 1.0f}},
    {"epsilon", {"epsilon", 0x03F5, "STIX Two Math", false, 1.0f}},
    {"zeta", {"zeta", 0x03B6, "STIX Two Math", false, 1.0f}},
    {"eta", {"eta", 0x03B7, "STIX Two Math", false, 1.0f}},
    {"theta", {"theta", 0x03B8, "STIX Two Math", false, 1.0f}},
    {"iota", {"iota", 0x03B9, "STIX Two Math", false, 1.0f}},
    {"kappa", {"kappa", 0x03BA, "STIX Two Math", false, 1.0f}},
    {"lambda", {"lambda", 0x03BB, "STIX Two Math", false, 1.0f}},
    {"mu", {"mu", 0x03BC, "STIX Two Math", false, 1.0f}},
    {"nu", {"nu", 0x03BD, "STIX Two Math", false, 1.0f}},
    {"xi", {"xi", 0x03BE, "STIX Two Math", false, 1.0f}},
    {"pi", {"pi", 0x03C0, "STIX Two Math", false, 1.0f}},
    {"rho", {"rho", 0x03C1, "STIX Two Math", false, 1.0f}},
    {"sigma", {"sigma", 0x03C3, "STIX Two Math", false, 1.0f}},
    {"tau", {"tau", 0x03C4, "STIX Two Math", false, 1.0f}},
    {"upsilon", {"upsilon", 0x03C5, "STIX Two Math", false, 1.0f}},
    {"phi", {"phi", 0x03D5, "STIX Two Math", false, 1.0f}},
    {"chi", {"chi", 0x03C7, "STIX Two Math", false, 1.0f}},
    {"psi", {"psi", 0x03C8, "STIX Two Math", false, 1.0f}},
    {"omega", {"omega", 0x03C9, "STIX Two Math", false, 1.0f}},

    // Greek uppercase
    {"Gamma", {"Gamma", 0x0393, "STIX Two Math", false, 1.0f}},
    {"Delta", {"Delta", 0x0394, "STIX Two Math", false, 1.0f}},
    {"Theta", {"Theta", 0x0398, "STIX Two Math", false, 1.0f}},
    {"Lambda", {"Lambda", 0x039B, "STIX Two Math", false, 1.0f}},
    {"Xi", {"Xi", 0x039E, "STIX Two Math", false, 1.0f}},
    {"Pi", {"Pi", 0x03A0, "STIX Two Math", false, 1.0f}},
    {"Sigma", {"Sigma", 0x03A3, "STIX Two Math", false, 1.0f}},
    {"Phi", {"Phi", 0x03A6, "STIX Two Math", false, 1.0f}},
    {"Psi", {"Psi", 0x03A8, "STIX Two Math", false, 1.0f}},
    {"Omega", {"Omega", 0x03A9, "STIX Two Math", false, 1.0f}},

    // Operators
    {"sum", {"sum", 0x2211, "STIX Two Math", true, 1.5f}},
    {"prod", {"prod", 0x220F, "STIX Two Math", true, 1.5f}},
    {"int", {"int", 0x222B, "STIX Two Math", true, 1.5f}},
    {"oint", {"oint", 0x222E, "STIX Two Math", true, 1.5f}},
    {"partial", {"partial", 0x2202, "STIX Two Math", false, 1.0f}},
    {"nabla", {"nabla", 0x2207, "STIX Two Math", false, 1.0f}},
    {"infty", {"infty", 0x221E, "STIX Two Math", false, 1.0f}},

    // Relations
    {"leq", {"leq", 0x2264, "STIX Two Math", true, 1.0f}},
    {"geq", {"geq", 0x2265, "STIX Two Math", true, 1.0f}},
    {"neq", {"neq", 0x2260, "STIX Two Math", true, 1.0f}},
    {"approx", {"approx", 0x2248, "STIX Two Math", true, 1.0f}},
    {"equiv", {"equiv", 0x2261, "STIX Two Math", true, 1.0f}},
    {"in", {"in", 0x2208, "STIX Two Math", true, 1.0f}},
    {"subset", {"subset", 0x2282, "STIX Two Math", true, 1.0f}},
    {"supset", {"supset", 0x2283, "STIX Two Math", true, 1.0f}},

    // Arrows
    {"rightarrow", {"rightarrow", 0x2192, "STIX Two Math", true, 1.0f}},
    {"leftarrow", {"leftarrow", 0x2190, "STIX Two Math", true, 1.0f}},
    {"Rightarrow", {"Rightarrow", 0x21D2, "STIX Two Math", true, 1.0f}},
    {"Leftarrow", {"Leftarrow", 0x21D0, "STIX Two Math", true, 1.0f}},

    // Misc
    {"times", {"times", 0x00D7, "STIX Two Math", true, 1.0f}},
    {"div", {"div", 0x00F7, "STIX Two Math", true, 1.0f}},
    {"pm", {"pm", 0x00B1, "STIX Two Math", true, 1.0f}},
    {"cdot", {"cdot", 0x22C5, "STIX Two Math", true, 1.0f}},
    {"sqrt", {"sqrt", 0x221A, "STIX Two Math", false, 1.0f}},
};

const std::unordered_map<std::string, uint32_t> MathFontRenderer::greek_letters_ = {
    {"alpha", 0x03B1}, {"beta", 0x03B2}, {"gamma", 0x03B3}, {"delta", 0x03B4},
    {"epsilon", 0x03F5}, {"zeta", 0x03B6}, {"eta", 0x03B7}, {"theta", 0x03B8},
    {"iota", 0x03B9}, {"kappa", 0x03BA}, {"lambda", 0x03BB}, {"mu", 0x03BC},
    {"nu", 0x03BD}, {"xi", 0x03BE}, {"pi", 0x03C0}, {"rho", 0x03C1},
    {"sigma", 0x03C3}, {"tau", 0x03C4}, {"upsilon", 0x03C5}, {"phi", 0x03D5},
    {"chi", 0x03C7}, {"psi", 0x03C8}, {"omega", 0x03C9},
};

const std::unordered_map<std::string, uint32_t> MathFontRenderer::operators_ = {
    {"sum", 0x2211}, {"prod", 0x220F}, {"int", 0x222B}, {"infty", 0x221E},
    {"partial", 0x2202}, {"nabla", 0x2207}, {"sqrt", 0x221A},
};

// ============================================================================
// LaTeXRenderer Implementation
// ============================================================================

LaTeXRenderer& LaTeXRenderer::instance() {
    static LaTeXRenderer instance;
    return instance;
}

LaTeXResult LaTeXRenderer::render(const std::string& latex) {
    // Check cache first
    if (config_.cache_enabled) {
        auto* cached = get_cached(latex);
        if (cached) {
            return *cached;
        }
    }

    LaTeXResult result;

    if (config_.use_external_compiler && is_compiler_available()) {
        result = compile_external(latex);
    } else {
        result = render_simple_math(latex);
    }

    // Cache result
    if (config_.cache_enabled && result.success) {
        std::string hash = compute_hash(latex);
        cache_[hash] = result;
    }

    return result;
}

std::shared_ptr<VMobject> LaTeXRenderer::render_to_mobject(const std::string& latex) {
    LaTeXResult result = render(latex);

    if (!result.success) {
        spdlog::warn("LaTeX rendering failed: {}", result.error_message);
        return nullptr;
    }

    auto vmobject = std::make_shared<VMobject>();
    paths_to_vmobject(result.paths, *vmobject);

    return vmobject;
}

bool LaTeXRenderer::is_compiler_available() const {
#ifdef _WIN32
    return system((config_.latex_compiler + " --version > NUL 2>&1").c_str()) == 0;
#else
    return system((config_.latex_compiler + " --version > /dev/null 2>&1").c_str()) == 0;
#endif
}

void LaTeXRenderer::clear_cache() {
    cache_.clear();
}

const LaTeXResult* LaTeXRenderer::get_cached(const std::string& latex) const {
    std::string hash = compute_hash(latex);
    auto it = cache_.find(hash);
    if (it != cache_.end()) {
        return &it->second;
    }
    return nullptr;
}

LaTeXResult LaTeXRenderer::compile_external(const std::string& latex) {
    LaTeXResult result;

    // Create temp directory
    std::filesystem::create_directories(config_.temp_dir);

    std::string hash = compute_hash(latex);
    std::string base_name = config_.temp_dir + "/" + hash;
    std::string tex_file = base_name + ".tex";
    std::string dvi_file = base_name + ".dvi";
    std::string svg_file = base_name + ".svg";

    // Check if SVG already exists
    if (std::filesystem::exists(svg_file)) {
        result.paths = parse_svg(svg_file);
        result.success = !result.paths.empty();
        return result;
    }

    // Write TeX file
    {
        std::ofstream ofs(tex_file);
        if (!ofs) {
            result.error_message = "Failed to create TeX file";
            return result;
        }
        ofs << config_.preamble;
        ofs << latex;
        ofs << config_.postamble;
    }

    // Compile LaTeX
    std::string latex_cmd = config_.latex_compiler +
                            " -interaction=nonstopmode -output-format=dvi " +
                            "-output-directory=" + config_.temp_dir + " " +
                            tex_file + " > /dev/null 2>&1";

    int ret = system(latex_cmd.c_str());
    if (ret != 0) {
        result.error_message = "LaTeX compilation failed";
        return result;
    }

    // Convert DVI to SVG
    std::string dvi_cmd = config_.dvi_converter +
                          " --no-fonts --exact-bbox " +
                          dvi_file + " -o " + svg_file + " > /dev/null 2>&1";

    ret = system(dvi_cmd.c_str());
    if (ret != 0) {
        result.error_message = "DVI to SVG conversion failed";
        return result;
    }

    // Parse SVG
    result.paths = parse_svg(svg_file);
    result.success = !result.paths.empty();

    return result;
}

std::vector<SVGPath> LaTeXRenderer::parse_svg(const std::filesystem::path& svg_path) {
    return SVGParser::parse_file(svg_path);
}

SVGPath LaTeXRenderer::parse_svg_path_data(const std::string& d) {
    return SVGParser::parse_path_data(d);
}

void LaTeXRenderer::paths_to_vmobject(const std::vector<SVGPath>& paths, VMobject& vmobject) {
    for (const auto& path : paths) {
        auto beziers = SVGParser::path_to_beziers(path);

        if (beziers.size() >= 4) {
            for (size_t i = 0; i + 3 < beziers.size(); i += 3) {
                // Scale to scene coordinates
                math::Vec3 p0 = beziers[i] * config_.scale;
                math::Vec3 p1 = beziers[i + 1] * config_.scale;
                math::Vec3 p2 = beziers[i + 2] * config_.scale;
                math::Vec3 p3 = beziers[i + 3] * config_.scale;

                vmobject.add_cubic_bezier_curve(p0, p1, p2, p3);
            }
        }

        // Set colors
        if (path.fill_color.w > 0) {
            vmobject.set_fill(path.fill_color);
        }
        if (path.stroke_width > 0) {
            vmobject.set_stroke(path.stroke_color, path.stroke_width * config_.scale);
        }
    }
}

LaTeXResult LaTeXRenderer::render_simple_math(const std::string& latex) {
    LaTeXResult result;
    result.success = true;

    // Parse LaTeX and convert to paths using font glyphs
    auto symbols = parse_simple_latex(latex);

    // For now, just return empty paths - full implementation would render using fonts
    spdlog::info("Simple math rendering for: {} ({} symbols)", latex, symbols.size());

    return result;
}

std::vector<MathSymbol> LaTeXRenderer::parse_simple_latex(const std::string& latex) {
    std::vector<MathSymbol> symbols;
    std::string buffer;

    bool in_command = false;

    for (size_t i = 0; i < latex.length(); ++i) {
        char c = latex[i];

        if (c == '\\') {
            in_command = true;
            buffer.clear();
            continue;
        }

        if (in_command) {
            if (std::isalpha(c)) {
                buffer += c;
            } else {
                // End of command
                if (!buffer.empty()) {
                    auto it = math_symbols_.find(buffer);
                    if (it != math_symbols_.end()) {
                        symbols.push_back(it->second);
                    }
                }
                buffer.clear();
                in_command = false;

                // Process current character if not space
                if (!std::isspace(c)) {
                    MathSymbol sym;
                    sym.name = std::string(1, c);
                    sym.codepoint = static_cast<uint32_t>(c);
                    sym.is_operator = (c == '+' || c == '-' || c == '=' || c == '<' || c == '>');
                    symbols.push_back(sym);
                }
            }
        } else if (!std::isspace(c)) {
            MathSymbol sym;
            sym.name = std::string(1, c);
            sym.codepoint = static_cast<uint32_t>(c);
            sym.is_operator = (c == '+' || c == '-' || c == '=' || c == '<' || c == '>');
            symbols.push_back(sym);
        }
    }

    // Handle trailing command
    if (!buffer.empty()) {
        auto it = math_symbols_.find(buffer);
        if (it != math_symbols_.end()) {
            symbols.push_back(it->second);
        }
    }

    return symbols;
}

std::string LaTeXRenderer::compute_hash(const std::string& latex) const {
    // Simple hash for caching
    std::hash<std::string> hasher;
    return std::to_string(hasher(latex + config_.preamble));
}

// ============================================================================
// SVGParser Implementation
// ============================================================================

std::vector<SVGPath> SVGParser::parse_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        spdlog::warn("Failed to open SVG file: {}", path.string());
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_string(buffer.str());
}

std::vector<SVGPath> SVGParser::parse_string(const std::string& svg) {
    std::vector<SVGPath> paths;

    // Simple regex-based SVG path extraction
    // A full implementation would use a proper XML parser

    std::regex path_regex(R"REGEX(<path[^>]*d\s*=\s*"([^"]*)"[^>]*>)REGEX");
    std::regex fill_regex(R"REGEX(fill\s*=\s*"([^"]*)")REGEX");
    std::regex stroke_regex(R"REGEX(stroke\s*=\s*"([^"]*)")REGEX");
    std::regex stroke_width_regex(R"REGEX(stroke-width\s*=\s*"([^"]*)")REGEX");

    std::sregex_iterator it(svg.begin(), svg.end(), path_regex);
    std::sregex_iterator end;

    while (it != end) {
        std::string path_tag = it->str();
        std::string d_attr = (*it)[1].str();

        SVGPath path = parse_path_data(d_attr);

        // Extract fill color
        std::smatch fill_match;
        if (std::regex_search(path_tag, fill_match, fill_regex)) {
            std::string fill = fill_match[1].str();
            if (fill != "none") {
                // Parse color (simplified - just handle black for now)
                if (fill == "black" || fill == "#000000") {
                    path.fill_color = math::Vec4(0, 0, 0, 1);
                } else if (fill == "white" || fill == "#ffffff") {
                    path.fill_color = math::Vec4(1, 1, 1, 1);
                }
            }
        }

        // Extract stroke
        std::smatch stroke_match;
        if (std::regex_search(path_tag, stroke_match, stroke_regex)) {
            std::string stroke = stroke_match[1].str();
            if (stroke != "none") {
                path.stroke_color = math::Vec4(0, 0, 0, 1);
            }
        }

        // Extract stroke width
        std::smatch sw_match;
        if (std::regex_search(path_tag, sw_match, stroke_width_regex)) {
            path.stroke_width = std::stof(sw_match[1].str());
        }

        paths.push_back(path);
        ++it;
    }

    return paths;
}

SVGPath SVGParser::parse_path_data(const std::string& d) {
    SVGPath path;

    const char* ptr = d.c_str();
    char current_cmd = 'M';

    while (*ptr) {
        skip_separators(ptr);
        if (!*ptr) break;

        // Check for command
        if (std::isalpha(*ptr)) {
            current_cmd = *ptr++;
        }

        SVGPathSegment segment;
        segment.command = current_cmd;

        // Parse numbers based on command
        int num_values = 0;
        switch (std::toupper(current_cmd)) {
            case 'M': case 'L': case 'T': num_values = 2; break;
            case 'H': case 'V': num_values = 1; break;
            case 'C': num_values = 6; break;
            case 'S': case 'Q': num_values = 4; break;
            case 'A': num_values = 7; break;
            case 'Z': num_values = 0; break;
            default: break;
        }

        for (int i = 0; i < num_values && *ptr; ++i) {
            skip_separators(ptr);
            if (*ptr == '-' || *ptr == '+' || std::isdigit(*ptr) || *ptr == '.') {
                segment.values.push_back(parse_number(ptr));
            }
        }

        path.segments.push_back(segment);

        // Update implicit command (M becomes L after first point)
        if (current_cmd == 'M') current_cmd = 'L';
        if (current_cmd == 'm') current_cmd = 'l';
    }

    return path;
}

std::vector<math::Vec3> SVGParser::path_to_beziers(const SVGPath& path) {
    std::vector<math::Vec3> points;

    math::Vec3 current(0, 0, 0);
    math::Vec3 start(0, 0, 0);
    math::Vec3 last_control(0, 0, 0);

    for (const auto& seg : path.segments) {
        bool relative = std::islower(seg.command);
        char cmd = std::toupper(seg.command);

        switch (cmd) {
            case 'M': {
                if (seg.values.size() >= 2) {
                    float x = seg.values[0];
                    float y = seg.values[1];
                    current = relative ? current + math::Vec3(x, y, 0) : math::Vec3(x, y, 0);
                    start = current;
                    points.push_back(current);
                }
                break;
            }
            case 'L': {
                if (seg.values.size() >= 2) {
                    float x = seg.values[0];
                    float y = seg.values[1];
                    math::Vec3 end = relative ? current + math::Vec3(x, y, 0) : math::Vec3(x, y, 0);

                    // Convert line to cubic Bezier
                    math::Vec3 c1 = current + (end - current) / 3.0f;
                    math::Vec3 c2 = current + (end - current) * 2.0f / 3.0f;

                    points.push_back(c1);
                    points.push_back(c2);
                    points.push_back(end);

                    current = end;
                }
                break;
            }
            case 'C': {
                if (seg.values.size() >= 6) {
                    math::Vec3 c1(seg.values[0], seg.values[1], 0);
                    math::Vec3 c2(seg.values[2], seg.values[3], 0);
                    math::Vec3 end(seg.values[4], seg.values[5], 0);

                    if (relative) {
                        c1 = current + c1;
                        c2 = current + c2;
                        end = current + end;
                    }

                    points.push_back(c1);
                    points.push_back(c2);
                    points.push_back(end);

                    last_control = c2;
                    current = end;
                }
                break;
            }
            case 'Q': {
                if (seg.values.size() >= 4) {
                    math::Vec3 control(seg.values[0], seg.values[1], 0);
                    math::Vec3 end(seg.values[2], seg.values[3], 0);

                    if (relative) {
                        control = current + control;
                        end = current + end;
                    }

                    // Convert quadratic to cubic
                    math::Vec3 c1, c2;
                    quadratic_to_cubic(current, control, end, c1, c2);

                    points.push_back(c1);
                    points.push_back(c2);
                    points.push_back(end);

                    last_control = control;
                    current = end;
                }
                break;
            }
            case 'Z': {
                if (current.x != start.x || current.y != start.y) {
                    // Close path with line
                    math::Vec3 c1 = current + (start - current) / 3.0f;
                    math::Vec3 c2 = current + (start - current) * 2.0f / 3.0f;

                    points.push_back(c1);
                    points.push_back(c2);
                    points.push_back(start);
                }
                current = start;
                break;
            }
            // Add more commands as needed (H, V, S, T, A)
        }
    }

    return points;
}

float SVGParser::parse_number(const char*& ptr) {
    char* end;
    float value = std::strtof(ptr, &end);
    ptr = end;
    return value;
}

void SVGParser::skip_separators(const char*& ptr) {
    while (*ptr && (std::isspace(*ptr) || *ptr == ',')) {
        ++ptr;
    }
}

void SVGParser::quadratic_to_cubic(const math::Vec3& p0, const math::Vec3& p1, const math::Vec3& p2,
                                    math::Vec3& c1, math::Vec3& c2) {
    // Convert quadratic Bezier (P0, P1, P2) to cubic (P0, C1, C2, P2)
    // C1 = P0 + 2/3 * (P1 - P0)
    // C2 = P2 + 2/3 * (P1 - P2)
    c1 = p0 + (p1 - p0) * (2.0f / 3.0f);
    c2 = p2 + (p1 - p2) * (2.0f / 3.0f);
}

// ============================================================================
// MathFontRenderer Implementation
// ============================================================================

bool MathFontRenderer::initialize() {
    if (initialized_) return true;

    FontManager& fm = FontManager::instance();
    if (!fm.is_initialized()) {
        fm.initialize();
    }

    // Try to find a math font
    std::vector<std::string> math_fonts = {
        "STIX Two Math",
        "Latin Modern Math",
        "DejaVu Math TeX Gyre",
        "Cambria Math",
        "XITS Math"
    };

    for (const auto& font : math_fonts) {
        if (fm.has_font_family(font)) {
            math_font_family_ = font;
            initialized_ = true;
            spdlog::info("Using math font: {}", font);
            return true;
        }
    }

    spdlog::warn("No math font found, using default");
    math_font_family_ = "";
    initialized_ = true;
    return true;
}

FontFace* MathFontRenderer::get_math_font() {
    if (!initialized_) initialize();
    return FontManager::instance().get_font(math_font_family_, 48);
}

std::shared_ptr<VMobject> MathFontRenderer::render(const std::string& expression) {
    (void)expression;  // TODO: Implement
    return std::make_shared<VMobject>();
}

} // namespace manim
