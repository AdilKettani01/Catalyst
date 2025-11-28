/**
 * @file text_animation.hpp
 * @brief Specialized animations for text mobjects
 *
 * Provides text-specific animations like:
 * - Write: Draw text character by character
 * - Unwrite: Reverse of write
 * - AddTextLetterByLetter: Fade in letters sequentially
 * - RemoveTextLetterByLetter: Fade out letters sequentially
 * - TypewriterEffect: Typewriter-style appearance
 */

#pragma once

#include "manim/animation/animation.hpp"
#include "manim/mobject/text/text.hpp"

namespace manim {

/**
 * @brief Write animation - draws text progressively
 *
 * The Write animation reveals text as if it were being written.
 * For SDF text, this progressively reveals characters with a
 * stroke-like effect before filling.
 */
class WriteText : public Animation {
public:
    /**
     * @brief Create write animation for text
     * @param text Text mobject to animate
     * @param run_time Duration of animation
     * @param lag_ratio Delay ratio between characters (0 = simultaneous, 1 = sequential)
     */
    WriteText(std::shared_ptr<Text> text,
              float run_time = 1.0f,
              float lag_ratio = 0.1f);

    bool can_run_on_gpu() const override { return true; }

    /**
     * @brief Set whether to show stroke before fill
     */
    WriteText& set_stroke_first(bool stroke_first) {
        stroke_first_ = stroke_first;
        return *this;
    }

    /**
     * @brief Set stroke color during write
     */
    WriteText& set_stroke_color(const math::Vec4& color) {
        stroke_color_ = color;
        return *this;
    }

    /**
     * @brief Set stroke width during write
     */
    WriteText& set_stroke_width(float width) {
        stroke_width_ = width;
        return *this;
    }

protected:
    void begin() override;
    void finish() override;
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<Text> text_;
    float lag_ratio_;
    bool stroke_first_ = true;
    math::Vec4 stroke_color_{1, 1, 1, 1};
    float stroke_width_ = 2.0f;

    // Per-character state
    struct CharState {
        float start_alpha;      // Alpha at which this char starts appearing
        float end_alpha;        // Alpha at which this char is fully visible
        math::Vec4 original_color;
    };
    std::vector<CharState> char_states_;
};

/**
 * @brief Unwrite animation - reverses write effect
 */
class UnwriteText : public WriteText {
public:
    using WriteText::WriteText;

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Add text letter by letter with fade
 *
 * Each character fades in sequentially with optional overlap.
 */
class AddTextLetterByLetter : public Animation {
public:
    /**
     * @brief Create letter-by-letter animation
     * @param text Text mobject to animate
     * @param run_time Duration of animation
     * @param time_per_char Time spent animating each character
     */
    AddTextLetterByLetter(std::shared_ptr<Text> text,
                           float run_time = 2.0f,
                           float time_per_char = 0.0f);

    bool can_run_on_gpu() const override { return true; }

    /**
     * @brief Set whether to start from left or right
     */
    AddTextLetterByLetter& set_reverse(bool reverse) {
        reverse_ = reverse;
        return *this;
    }

protected:
    void begin() override;
    void finish() override;
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<Text> text_;
    float time_per_char_;
    bool reverse_ = false;
    size_t num_chars_ = 0;
};

/**
 * @brief Remove text letter by letter with fade
 */
class RemoveTextLetterByLetter : public AddTextLetterByLetter {
public:
    using AddTextLetterByLetter::AddTextLetterByLetter;

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Typewriter effect - characters appear with cursor
 *
 * Creates a typewriter effect with an optional blinking cursor.
 */
class TypewriterEffect : public Animation {
public:
    TypewriterEffect(std::shared_ptr<Text> text,
                     float run_time = 2.0f,
                     bool show_cursor = true);

    bool can_run_on_gpu() const override { return true; }

    /**
     * @brief Set cursor character
     */
    TypewriterEffect& set_cursor(const std::string& cursor) {
        cursor_char_ = cursor;
        return *this;
    }

    /**
     * @brief Set cursor blink rate
     */
    TypewriterEffect& set_blink_rate(float rate) {
        blink_rate_ = rate;
        return *this;
    }

    /**
     * @brief Set typing speed variation (randomness)
     */
    TypewriterEffect& set_speed_variation(float variation) {
        speed_variation_ = variation;
        return *this;
    }

protected:
    void begin() override;
    void finish() override;
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<Text> text_;
    bool show_cursor_;
    std::string cursor_char_ = "|";
    float blink_rate_ = 2.0f;  // Blinks per second
    float speed_variation_ = 0.0f;

    std::string original_text_;
    std::vector<float> char_times_;  // Timing for each character
};

/**
 * @brief Text transform - morphs between two texts
 *
 * Handles character-level morphing between source and target text.
 */
class TransformText : public Animation {
public:
    TransformText(std::shared_ptr<Text> source,
                  std::shared_ptr<Text> target,
                  float run_time = 1.0f);

    bool can_run_on_gpu() const override { return true; }

    /**
     * @brief Set matching strategy for characters
     */
    enum class MatchStrategy {
        Position,       // Match by position
        Character,      // Match same characters
        FirstToFirst    // Match first to first, etc.
    };

    TransformText& set_match_strategy(MatchStrategy strategy) {
        match_strategy_ = strategy;
        return *this;
    }

protected:
    void begin() override;
    void finish() override;
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<Text> source_;
    std::shared_ptr<Text> target_;
    MatchStrategy match_strategy_ = MatchStrategy::Character;

    // Character matching pairs (source_idx, target_idx)
    std::vector<std::pair<int, int>> char_matches_;

    // Characters that fade out (no match)
    std::vector<int> fade_out_chars_;

    // Characters that fade in (new)
    std::vector<int> fade_in_chars_;

    void compute_character_matches();
};

/**
 * @brief Text color wave - animated color gradient
 */
class TextColorWave : public Animation {
public:
    TextColorWave(std::shared_ptr<Text> text,
                  const math::Vec4& color1,
                  const math::Vec4& color2,
                  float run_time = 2.0f);

    bool can_run_on_gpu() const override { return true; }

    /**
     * @brief Set wave speed
     */
    TextColorWave& set_wave_speed(float speed) {
        wave_speed_ = speed;
        return *this;
    }

    /**
     * @brief Set wave width (how many characters in one cycle)
     */
    TextColorWave& set_wave_width(float width) {
        wave_width_ = width;
        return *this;
    }

protected:
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<Text> text_;
    math::Vec4 color1_;
    math::Vec4 color2_;
    float wave_speed_ = 1.0f;
    float wave_width_ = 5.0f;
};

/**
 * @brief Indicate text - flash/highlight effect
 */
class IndicateText : public Animation {
public:
    IndicateText(std::shared_ptr<Text> text,
                 float run_time = 0.5f,
                 float scale_factor = 1.2f);

    bool can_run_on_gpu() const override { return true; }

    /**
     * @brief Set highlight color
     */
    IndicateText& set_color(const math::Vec4& color) {
        highlight_color_ = color;
        return *this;
    }

protected:
    void begin() override;
    void finish() override;
    void interpolate_mobject(float alpha) override;

private:
    std::shared_ptr<Text> text_;
    float scale_factor_;
    math::Vec4 highlight_color_{1, 1, 0, 1};  // Yellow highlight
    math::Vec4 original_color_;
    float original_scale_ = 1.0f;
};

} // namespace manim
