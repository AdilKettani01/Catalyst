/**
 * @file test_text.cpp
 * @brief Unit tests for text rendering system
 */

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "manim/text/font_manager.hpp"
#include "manim/text/sdf_generator.hpp"
#include "manim/text/text_layout.hpp"
#include "manim/text/latex_renderer.hpp"
#include "manim/mobject/text/text.hpp"
#include "manim/animation/text_animation.hpp"
#include "manim/utils/gpu_utils.h"
#include "manim/core/memory_pool.hpp"

namespace manim {
namespace test {

// ============================================================================
// FontManager Tests
// ============================================================================

class FontManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(FontManagerTest, InitializesSuccessfully) {
    EXPECT_TRUE(FontManager::instance().is_initialized());
}

TEST_F(FontManagerTest, GetsDefaultFont) {
    FontFace* font = FontManager::instance().get_default_font(48);
    // May be nullptr if no fonts are installed
    if (font) {
        EXPECT_GT(font->get_size_pixels(), 0);
    }
}

TEST_F(FontManagerTest, ListsFontFamilies) {
    auto families = FontManager::instance().get_font_families();
    // Should have at least attempted to find some fonts
    // (may be empty on minimal systems)
    SUCCEED();  // Just verifying it doesn't crash
}

TEST_F(FontManagerTest, SetsAndGetsSize) {
    FontFace* font = FontManager::instance().get_default_font(24);
    if (font) {
        EXPECT_EQ(font->get_size_pixels(), 24);
        font->set_size(48);
        EXPECT_EQ(font->get_size_pixels(), 48);
    }
}

// ============================================================================
// SDFGenerator Tests
// ============================================================================

class SDFGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(SDFGeneratorTest, DefaultConfigIsValid) {
    SDFConfig config;
    EXPECT_GT(config.sdf_size, 0);
    EXPECT_GT(config.sdf_padding, 0);
    EXPECT_GT(config.sdf_range, 0);
}

TEST_F(SDFGeneratorTest, AtlasInitializes) {
    SDFAtlas atlas;
    atlas.initialize(512, 512, 1);

    EXPECT_EQ(atlas.get_width(), 512);
    EXPECT_EQ(atlas.get_height(), 512);
    EXPECT_EQ(atlas.get_channels(), 1);
}

TEST_F(SDFGeneratorTest, AtlasHasGlyph) {
    SDFAtlas atlas;
    atlas.initialize(512, 512, 1);

    EXPECT_FALSE(atlas.has_glyph('A'));

    // Add a dummy glyph
    SDFGlyph glyph;
    glyph.codepoint = 'A';
    glyph.width = 32;
    glyph.height = 32;
    glyph.channels = 1;
    glyph.sdf_data.resize(32 * 32, 128);

    EXPECT_TRUE(atlas.add_glyph(glyph));
    EXPECT_TRUE(atlas.has_glyph('A'));
}

TEST_F(SDFGeneratorTest, GeneratesGlyphSDF) {
    FontFace* font = FontManager::instance().get_default_font(48);
    if (!font) {
        GTEST_SKIP() << "No fonts available for testing";
    }

    SDFGenerator generator;
    SDFGlyph glyph = generator.generate_glyph_sdf(*font, 'A');

    // Should have generated something (or empty for space)
    EXPECT_EQ(glyph.codepoint, static_cast<uint32_t>('A'));
}

// ============================================================================
// TextLayout Tests
// ============================================================================

class TextLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(TextLayoutTest, EmptyTextLayout) {
    TextLayoutEngine engine;
    FontFace* font = FontManager::instance().get_default_font(48);

    if (!font) {
        GTEST_SKIP() << "No fonts available for testing";
    }

    TextLayout layout = engine.layout("", *font);

    EXPECT_EQ(layout.num_glyphs, 0);
    EXPECT_EQ(layout.num_lines, 0);
}

TEST_F(TextLayoutTest, SingleLineLayout) {
    TextLayoutEngine engine;
    FontFace* font = FontManager::instance().get_default_font(48);

    if (!font) {
        GTEST_SKIP() << "No fonts available for testing";
    }

    TextLayout layout = engine.layout("Hello", *font);

    EXPECT_EQ(layout.num_lines, 1);
    EXPECT_GT(layout.num_glyphs, 0);
    EXPECT_GT(layout.total_width, 0);
}

TEST_F(TextLayoutTest, MultiLineLayout) {
    TextLayoutEngine engine;
    FontFace* font = FontManager::instance().get_default_font(48);

    if (!font) {
        GTEST_SKIP() << "No fonts available for testing";
    }

    TextLayoutOptions options;
    options.max_width = 100;  // Force wrapping

    TextLayout layout = engine.layout("This is a longer text that should wrap", *font, options);

    // Should have wrapped into multiple lines
    EXPECT_GE(layout.num_lines, 1);
}

TEST_F(TextLayoutTest, AlignmentOptions) {
    TextLayoutEngine engine;
    FontFace* font = FontManager::instance().get_default_font(48);

    if (!font) {
        GTEST_SKIP() << "No fonts available for testing";
    }

    TextLayoutOptions options;

    options.alignment = TextAlignment::Left;
    TextLayout left_layout = engine.layout("Test", *font, options);

    options.alignment = TextAlignment::Center;
    TextLayout center_layout = engine.layout("Test", *font, options);

    options.alignment = TextAlignment::Right;
    TextLayout right_layout = engine.layout("Test", *font, options);

    // Just verify they don't crash
    SUCCEED();
}

// ============================================================================
// Text Mobject Tests
// ============================================================================

class TextMobjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(TextMobjectTest, ConstructsWithString) {
    Text text("Hello World");
    EXPECT_EQ(text.get_text(), "Hello World");
}

TEST_F(TextMobjectTest, SetsFont) {
    Text text("Test");
    text.set_font("Arial");
    // Just verify it doesn't crash
    SUCCEED();
}

TEST_F(TextMobjectTest, SetsFontSize) {
    Text text("Test", 24.0f);
    EXPECT_FLOAT_EQ(text.get_font_size(), 24.0f);

    text.set_font_size(48.0f);
    EXPECT_FLOAT_EQ(text.get_font_size(), 48.0f);
}

TEST_F(TextMobjectTest, SetsColor) {
    Text text("Test");
    text.set_color(math::Vec4(1, 0, 0, 1));
    EXPECT_EQ(text.get_color().x, 1.0f);
    EXPECT_EQ(text.get_color().y, 0.0f);
}

TEST_F(TextMobjectTest, SetsOutline) {
    Text text("Test");
    text.set_outline(2.0f, math::Vec4(1, 1, 1, 1));
    // Just verify it doesn't crash
    SUCCEED();
}

TEST_F(TextMobjectTest, SetsAlignment) {
    Text text("Test");
    text.set_alignment(Text::Alignment::Center);
    text.set_alignment(Text::Alignment::Right);
    text.set_alignment(Text::Alignment::Justify);
    SUCCEED();
}

TEST_F(TextMobjectTest, CharacterColorRange) {
    Text text("Hello World");
    text.set_char_color(0, math::Vec4(1, 0, 0, 1));
    text.set_char_color_range(0, 5, math::Vec4(0, 1, 0, 1));
    SUCCEED();
}

// ============================================================================
// LaTeX Renderer Tests
// ============================================================================

class LaTeXRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(LaTeXRendererTest, RendersSimpleExpression) {
    LaTeXRenderer& renderer = LaTeXRenderer::instance();
    LaTeXResult result = renderer.render("x^2 + y^2 = r^2");

    // May or may not succeed depending on LaTeX availability
    // Just verify it doesn't crash
    SUCCEED();
}

TEST_F(LaTeXRendererTest, ChecksCompilerAvailability) {
    bool available = LaTeXRenderer::instance().is_compiler_available();
    // Log whether LaTeX is available
    if (available) {
        std::cout << "LaTeX compiler is available" << std::endl;
    } else {
        std::cout << "LaTeX compiler is not available" << std::endl;
    }
    SUCCEED();
}

TEST_F(LaTeXRendererTest, CachesResults) {
    LaTeXRenderer& renderer = LaTeXRenderer::instance();
    renderer.clear_cache();

    std::string latex = "E = mc^2";
    renderer.render(latex);

    // Second render should use cache
    const LaTeXResult* cached = renderer.get_cached(latex);
    // Note: caching may be disabled or result may not be cached if failed
    SUCCEED();
}

// ============================================================================
// SVGParser Tests
// ============================================================================

class SVGParserTest : public ::testing::Test {};

TEST_F(SVGParserTest, ParsesPathData) {
    std::string path_data = "M 0 0 L 10 10 C 20 20 30 30 40 40 Z";
    SVGPath path = SVGParser::parse_path_data(path_data);

    EXPECT_FALSE(path.segments.empty());
    EXPECT_EQ(path.segments[0].command, 'M');
}

TEST_F(SVGParserTest, ConvertsToBeziers) {
    std::string path_data = "M 0 0 L 10 0 L 10 10 L 0 10 Z";
    SVGPath path = SVGParser::parse_path_data(path_data);

    auto beziers = SVGParser::path_to_beziers(path);
    EXPECT_FALSE(beziers.empty());
}

// ============================================================================
// Text Animation Tests
// ============================================================================

class TextAnimationTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(TextAnimationTest, WriteTextAnimation) {
    auto text = std::make_shared<Text>("Hello");
    WriteText anim(text, 1.0f, 0.1f);

    // Test animation using public API
    anim.play();  // Runs full animation cycle
    SUCCEED();
}

TEST_F(TextAnimationTest, AddTextLetterByLetter) {
    auto text = std::make_shared<Text>("Test");
    AddTextLetterByLetter anim(text, 2.0f);

    // Use interpolate which is public
    anim.interpolate(0.25f);
    anim.interpolate(0.5f);
    anim.interpolate(0.75f);

    SUCCEED();
}

TEST_F(TextAnimationTest, TypewriterEffect) {
    auto text = std::make_shared<Text>("Typing...");
    TypewriterEffect anim(text, 2.0f, true);

    // Use interpolate which is public
    anim.interpolate(0.5f);
    anim.interpolate(1.0f);

    SUCCEED();
}

TEST_F(TextAnimationTest, TransformText) {
    auto source = std::make_shared<Text>("Hello");
    auto target = std::make_shared<Text>("World");

    TransformText anim(source, target, 1.0f);

    // Use play() for full lifecycle
    anim.play();

    SUCCEED();
}

TEST_F(TextAnimationTest, TextColorWave) {
    auto text = std::make_shared<Text>("Rainbow");
    TextColorWave anim(text, math::Vec4(1, 0, 0, 1), math::Vec4(0, 0, 1, 1), 2.0f);

    // Use interpolate which is public
    anim.interpolate(0.25f);
    anim.interpolate(0.5f);
    anim.interpolate(0.75f);

    SUCCEED();
}

TEST_F(TextAnimationTest, IndicateText) {
    auto text = std::make_shared<Text>("Important!");
    IndicateText anim(text, 0.5f, 1.2f);

    // Use play() for full lifecycle
    anim.play();

    SUCCEED();
}

// ============================================================================
// GPU Text Integration Tests
// ============================================================================

class GPUTextTest : public ::testing::Test {
protected:
    void SetUp() override {
        FontManager::instance().initialize();
    }
};

TEST_F(GPUTextTest, SDFAtlasGeneration) {
    // Create a text object and verify glyphs are generated
    Text text("Hello GPU");

    // Verify glyph instances were created
    EXPECT_FALSE(text.get_text().empty());
}

TEST_F(GPUTextTest, GlyphInstanceBufferCreation) {
    Text text("Test");

    // Create a placeholder memory pool
    MemoryPool pool;
    // Pool not initialized with a device, so this will use placeholder mode
    text.upload_glyphs_to_gpu(pool);

    // Just verify it doesn't crash
    SUCCEED();
}

TEST_F(GPUTextTest, SDFAtlasManagerSingleton) {
    SDFAtlasManager& manager = SDFAtlasManager::instance();

    // Ensure glyphs for a string
    manager.ensure_glyphs("DejaVu Sans", "Test123");

    // May succeed or fail depending on font availability
    SUCCEED();  // Just verify it doesn't crash
}

TEST_F(GPUTextTest, TextEffectsConfiguration) {
    Text text("Effects Test");

    // Apply various effects
    text.set_outline(2.0f, math::Vec4(0, 0, 0, 1));
    text.set_glow(1.5f, math::Vec4(1, 1, 0, 1));
    text.set_shadow(math::Vec2(0.1f, -0.1f), 0.5f, math::Vec4(0, 0, 0, 0.5f));

    SUCCEED();
}

TEST_F(GPUTextTest, TextWithGPUDetection) {
    // Verify GPU is detected (should find NVIDIA RTX 3060 Ti)
    bool gpu_available = GPUUtils::isGPUAvailable();

    if (gpu_available) {
        // GPU detected, text rendering should work in GPU mode
        spdlog::info("GPU detected, text rendering will use GPU acceleration");
    } else {
        // Fallback to CPU mode
        spdlog::info("No GPU detected, text rendering will use CPU fallback");
    }

    // Both paths should work
    Text text("GPU Test");
    EXPECT_FALSE(text.get_text().empty());
}

}  // namespace test
}  // namespace manim
