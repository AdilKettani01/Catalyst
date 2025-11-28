/**
 * @file text_animation.cpp
 * @brief Text animation implementations
 */

#include "manim/animation/text_animation.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace manim {

// ============================================================================
// WriteText Implementation
// ============================================================================

WriteText::WriteText(std::shared_ptr<Text> text, float run_time, float lag_ratio)
    : Animation(text, run_time),
      text_(text),
      lag_ratio_(lag_ratio) {}

void WriteText::begin() {
    Animation::begin();

    if (!text_) return;

    const std::string& str = text_->get_text();
    size_t num_chars = str.length();

    if (num_chars == 0) return;

    // Calculate start/end times for each character based on lag_ratio
    char_states_.resize(num_chars);

    float total_lag = lag_ratio_ * (num_chars - 1);
    float char_duration = 1.0f / (1.0f + total_lag);

    for (size_t i = 0; i < num_chars; ++i) {
        char_states_[i].start_alpha = static_cast<float>(i) * lag_ratio_ * char_duration;
        char_states_[i].end_alpha = char_states_[i].start_alpha + char_duration;

        // Store original color
        auto [min_pt, max_pt] = text_->get_char_bounding_box(i);
        char_states_[i].original_color = text_->get_color();
    }

    // Initially hide all characters
    for (size_t i = 0; i < num_chars; ++i) {
        text_->set_char_color(i, math::Vec4(0, 0, 0, 0));
    }
}

void WriteText::finish() {
    // Restore all characters to full visibility
    for (size_t i = 0; i < char_states_.size(); ++i) {
        text_->set_char_color(i, char_states_[i].original_color);
    }

    Animation::finish();
}

void WriteText::interpolate_mobject(float alpha) {
    if (!text_ || char_states_.empty()) return;

    for (size_t i = 0; i < char_states_.size(); ++i) {
        const auto& state = char_states_[i];

        if (alpha < state.start_alpha) {
            // Character not yet visible
            text_->set_char_color(i, math::Vec4(0, 0, 0, 0));
        } else if (alpha >= state.end_alpha) {
            // Character fully visible
            text_->set_char_color(i, state.original_color);
        } else {
            // Character animating
            float char_alpha = (alpha - state.start_alpha) / (state.end_alpha - state.start_alpha);

            if (stroke_first_) {
                // Two phases: stroke then fill
                if (char_alpha < 0.5f) {
                    // Stroke phase
                    float stroke_alpha = char_alpha * 2.0f;
                    math::Vec4 color = stroke_color_;
                    color.w = stroke_alpha;
                    text_->set_char_color(i, color);
                } else {
                    // Fill phase
                    float fill_alpha = (char_alpha - 0.5f) * 2.0f;
                    math::Vec4 color = state.original_color;
                    color.w = fill_alpha + (1.0f - fill_alpha) * stroke_color_.w;
                    // Blend from stroke color to original
                    color.x = stroke_color_.x + fill_alpha * (state.original_color.x - stroke_color_.x);
                    color.y = stroke_color_.y + fill_alpha * (state.original_color.y - stroke_color_.y);
                    color.z = stroke_color_.z + fill_alpha * (state.original_color.z - stroke_color_.z);
                    text_->set_char_color(i, color);
                }
            } else {
                // Simple fade in
                math::Vec4 color = state.original_color;
                color.w *= char_alpha;
                text_->set_char_color(i, color);
            }
        }
    }
}

// ============================================================================
// UnwriteText Implementation
// ============================================================================

void UnwriteText::interpolate_mobject(float alpha) {
    // Reverse alpha for unwrite effect
    WriteText::interpolate_mobject(1.0f - alpha);
}

// ============================================================================
// AddTextLetterByLetter Implementation
// ============================================================================

AddTextLetterByLetter::AddTextLetterByLetter(std::shared_ptr<Text> text,
                                               float run_time,
                                               float time_per_char)
    : Animation(text, run_time),
      text_(text),
      time_per_char_(time_per_char) {}

void AddTextLetterByLetter::begin() {
    Animation::begin();

    if (!text_) return;

    num_chars_ = text_->get_text().length();

    // Calculate time per character if not specified
    if (time_per_char_ <= 0 && num_chars_ > 0) {
        time_per_char_ = run_time_ / num_chars_;
    }

    // Initially hide all characters
    for (size_t i = 0; i < num_chars_; ++i) {
        text_->set_char_color(i, math::Vec4(0, 0, 0, 0));
    }
}

void AddTextLetterByLetter::finish() {
    // Ensure all characters visible
    math::Vec4 color = text_->get_color();
    for (size_t i = 0; i < num_chars_; ++i) {
        text_->set_char_color(i, color);
    }

    Animation::finish();
}

void AddTextLetterByLetter::interpolate_mobject(float alpha) {
    if (!text_ || num_chars_ == 0) return;

    // Calculate which character we're at
    float chars_shown = alpha * num_chars_;
    size_t full_chars = static_cast<size_t>(chars_shown);
    float partial = chars_shown - full_chars;

    math::Vec4 full_color = text_->get_color();

    for (size_t i = 0; i < num_chars_; ++i) {
        size_t idx = reverse_ ? (num_chars_ - 1 - i) : i;

        if (i < full_chars) {
            // Fully visible
            text_->set_char_color(idx, full_color);
        } else if (i == full_chars) {
            // Partially visible (fading in)
            math::Vec4 color = full_color;
            color.w *= partial;
            text_->set_char_color(idx, color);
        } else {
            // Hidden
            text_->set_char_color(idx, math::Vec4(0, 0, 0, 0));
        }
    }
}

// ============================================================================
// RemoveTextLetterByLetter Implementation
// ============================================================================

void RemoveTextLetterByLetter::interpolate_mobject(float alpha) {
    // Reverse the animation
    AddTextLetterByLetter::interpolate_mobject(1.0f - alpha);
}

// ============================================================================
// TypewriterEffect Implementation
// ============================================================================

TypewriterEffect::TypewriterEffect(std::shared_ptr<Text> text,
                                     float run_time,
                                     bool show_cursor)
    : Animation(text, run_time),
      text_(text),
      show_cursor_(show_cursor) {}

void TypewriterEffect::begin() {
    Animation::begin();

    if (!text_) return;

    original_text_ = text_->get_text();

    // Generate timing for each character
    size_t num_chars = original_text_.length();
    char_times_.resize(num_chars);

    if (speed_variation_ > 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(1.0f - speed_variation_,
                                                    1.0f + speed_variation_);

        float cumulative = 0;
        for (size_t i = 0; i < num_chars; ++i) {
            char_times_[i] = cumulative;
            cumulative += dist(gen);
        }

        // Normalize to [0, 1]
        if (cumulative > 0) {
            for (auto& t : char_times_) {
                t /= cumulative;
            }
        }
    } else {
        // Linear timing
        for (size_t i = 0; i < num_chars; ++i) {
            char_times_[i] = static_cast<float>(i) / num_chars;
        }
    }

    // Start with empty text
    text_->set_text("");
}

void TypewriterEffect::finish() {
    text_->set_text(original_text_);
    Animation::finish();
}

void TypewriterEffect::interpolate_mobject(float alpha) {
    if (!text_ || original_text_.empty()) return;

    // Find how many characters to show
    size_t chars_to_show = 0;
    for (size_t i = 0; i < char_times_.size(); ++i) {
        if (alpha >= char_times_[i]) {
            chars_to_show = i + 1;
        }
    }

    // Build displayed text
    std::string displayed = original_text_.substr(0, chars_to_show);

    // Add blinking cursor
    if (show_cursor_ && alpha < 1.0f) {
        float blink_phase = std::fmod(alpha * run_time_ * blink_rate_, 1.0f);
        if (blink_phase < 0.5f) {
            displayed += cursor_char_;
        }
    }

    text_->set_text(displayed);
}

// ============================================================================
// TransformText Implementation
// ============================================================================

TransformText::TransformText(std::shared_ptr<Text> source,
                              std::shared_ptr<Text> target,
                              float run_time)
    : Animation(source, run_time),
      source_(source),
      target_(target) {}

void TransformText::begin() {
    Animation::begin();
    compute_character_matches();
}

void TransformText::finish() {
    // Final state should match target
    if (source_ && target_) {
        source_->set_text(target_->get_text());
        source_->set_color(target_->get_color());
    }
    Animation::finish();
}

void TransformText::interpolate_mobject(float alpha) {
    if (!source_ || !target_) return;

    // Interpolate matched characters
    for (const auto& [src_idx, tgt_idx] : char_matches_) {
        auto [src_min, src_max] = source_->get_char_bounding_box(src_idx);
        auto [tgt_min, tgt_max] = target_->get_char_bounding_box(tgt_idx);

        // Interpolate position
        math::Vec3 pos = src_min + alpha * (tgt_min - src_min);

        // Interpolate color
        math::Vec4 src_color = source_->get_color();
        math::Vec4 tgt_color = target_->get_color();
        math::Vec4 color = src_color + alpha * (tgt_color - src_color);

        source_->set_char_color(src_idx, color);
    }

    // Fade out unmatched source characters
    for (int idx : fade_out_chars_) {
        math::Vec4 color = source_->get_color();
        color.w *= (1.0f - alpha);
        source_->set_char_color(idx, color);
    }

    // Fade in new target characters
    // (Would need to add these as submobjects)
    for (int idx : fade_in_chars_) {
        (void)idx;  // Would handle adding new characters
    }
}

void TransformText::compute_character_matches() {
    char_matches_.clear();
    fade_out_chars_.clear();
    fade_in_chars_.clear();

    if (!source_ || !target_) return;

    const std::string& src_text = source_->get_text();
    const std::string& tgt_text = target_->get_text();

    switch (match_strategy_) {
        case MatchStrategy::Position: {
            // Match by position index
            size_t min_len = std::min(src_text.length(), tgt_text.length());
            for (size_t i = 0; i < min_len; ++i) {
                char_matches_.push_back({static_cast<int>(i), static_cast<int>(i)});
            }
            for (size_t i = min_len; i < src_text.length(); ++i) {
                fade_out_chars_.push_back(static_cast<int>(i));
            }
            for (size_t i = min_len; i < tgt_text.length(); ++i) {
                fade_in_chars_.push_back(static_cast<int>(i));
            }
            break;
        }

        case MatchStrategy::Character: {
            // Match same characters
            std::vector<bool> src_matched(src_text.length(), false);
            std::vector<bool> tgt_matched(tgt_text.length(), false);

            // First pass: exact position matches
            size_t min_len = std::min(src_text.length(), tgt_text.length());
            for (size_t i = 0; i < min_len; ++i) {
                if (src_text[i] == tgt_text[i]) {
                    char_matches_.push_back({static_cast<int>(i), static_cast<int>(i)});
                    src_matched[i] = true;
                    tgt_matched[i] = true;
                }
            }

            // Second pass: match remaining same characters
            for (size_t si = 0; si < src_text.length(); ++si) {
                if (src_matched[si]) continue;
                for (size_t ti = 0; ti < tgt_text.length(); ++ti) {
                    if (tgt_matched[ti]) continue;
                    if (src_text[si] == tgt_text[ti]) {
                        char_matches_.push_back({static_cast<int>(si), static_cast<int>(ti)});
                        src_matched[si] = true;
                        tgt_matched[ti] = true;
                        break;
                    }
                }
            }

            // Collect unmatched
            for (size_t i = 0; i < src_text.length(); ++i) {
                if (!src_matched[i]) fade_out_chars_.push_back(static_cast<int>(i));
            }
            for (size_t i = 0; i < tgt_text.length(); ++i) {
                if (!tgt_matched[i]) fade_in_chars_.push_back(static_cast<int>(i));
            }
            break;
        }

        case MatchStrategy::FirstToFirst: {
            // Match first to first, second to second, etc.
            size_t min_len = std::min(src_text.length(), tgt_text.length());
            for (size_t i = 0; i < min_len; ++i) {
                char_matches_.push_back({static_cast<int>(i), static_cast<int>(i)});
            }
            for (size_t i = min_len; i < src_text.length(); ++i) {
                fade_out_chars_.push_back(static_cast<int>(i));
            }
            for (size_t i = min_len; i < tgt_text.length(); ++i) {
                fade_in_chars_.push_back(static_cast<int>(i));
            }
            break;
        }
    }
}

// ============================================================================
// TextColorWave Implementation
// ============================================================================

TextColorWave::TextColorWave(std::shared_ptr<Text> text,
                              const math::Vec4& color1,
                              const math::Vec4& color2,
                              float run_time)
    : Animation(text, run_time),
      text_(text),
      color1_(color1),
      color2_(color2) {}

void TextColorWave::interpolate_mobject(float alpha) {
    if (!text_) return;

    const std::string& str = text_->get_text();
    size_t num_chars = str.length();

    for (size_t i = 0; i < num_chars; ++i) {
        // Calculate wave position for this character
        float wave_pos = static_cast<float>(i) / wave_width_ - alpha * wave_speed_;
        float t = (std::sin(wave_pos * 2.0f * 3.14159f) + 1.0f) / 2.0f;

        // Interpolate color
        math::Vec4 color;
        color.x = color1_.x + t * (color2_.x - color1_.x);
        color.y = color1_.y + t * (color2_.y - color1_.y);
        color.z = color1_.z + t * (color2_.z - color1_.z);
        color.w = color1_.w + t * (color2_.w - color1_.w);

        text_->set_char_color(i, color);
    }
}

// ============================================================================
// IndicateText Implementation
// ============================================================================

IndicateText::IndicateText(std::shared_ptr<Text> text,
                            float run_time,
                            float scale_factor)
    : Animation(text, run_time),
      text_(text),
      scale_factor_(scale_factor) {}

void IndicateText::begin() {
    Animation::begin();

    if (text_) {
        original_color_ = text_->get_color();
    }
}

void IndicateText::finish() {
    if (text_) {
        text_->set_color(original_color_);
        text_->scale(1.0f / scale_factor_);  // Reset scale
    }
    Animation::finish();
}

void IndicateText::interpolate_mobject(float alpha) {
    if (!text_) return;

    // Pulse effect: quick in, quick out
    float pulse;
    if (alpha < 0.5f) {
        pulse = alpha * 2.0f;  // 0 -> 1
    } else {
        pulse = (1.0f - alpha) * 2.0f;  // 1 -> 0
    }

    // Apply smooth easing
    pulse = 3.0f * pulse * pulse - 2.0f * pulse * pulse * pulse;  // Smoothstep

    // Interpolate color to highlight
    math::Vec4 color;
    color.x = original_color_.x + pulse * (highlight_color_.x - original_color_.x);
    color.y = original_color_.y + pulse * (highlight_color_.y - original_color_.y);
    color.z = original_color_.z + pulse * (highlight_color_.z - original_color_.z);
    color.w = original_color_.w;

    text_->set_color(color);

    // Apply scale
    float scale = 1.0f + pulse * (scale_factor_ - 1.0f);
    text_->scale(scale / original_scale_);
    original_scale_ = scale;
}

} // namespace manim
