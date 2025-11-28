/**
 * @file composition.cpp
 * @brief Implementation of animation composition classes
 */

#include "manim/animation/composition.hpp"
#include "manim/mobject/mobject.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>

namespace manim {

// ============================================================================
// AnimationGroup
// ============================================================================

// Helper to get the first animation's mobject or create a placeholder
static std::shared_ptr<Mobject> get_group_mobject(
    const std::vector<std::shared_ptr<Animation>>& animations
) {
    if (!animations.empty() && animations[0]) {
        return animations[0]->get_mobject();
    }
    // Create a placeholder mobject for empty groups
    return std::make_shared<Mobject>();
}

AnimationGroup::AnimationGroup(
    std::vector<std::shared_ptr<Animation>> animations,
    float run_time,
    float lag_ratio,
    RateFunc rate_func
)
    : Animation(get_group_mobject(animations), run_time, rate_func)
    , animations_(std::move(animations))
    , lag_ratio_(std::clamp(lag_ratio, 0.0f, 1.0f))
{
    name_ = "AnimationGroup";

    // If no rate function provided, use linear
    if (!rate_func_) {
        rate_func_ = rate_functions::linear;
    }

    // Build timing information
    build_timings();

    // Calculate run_time if not specified
    if (run_time_ < 0.0f) {
        run_time_ = calculate_max_end_time();
    }
}

void AnimationGroup::build_timings() {
    timings_.clear();
    timings_.reserve(animations_.size());

    if (animations_.empty()) {
        max_end_time_ = 1.0f;
        return;
    }

    // Calculate total run time of all animations
    float total_animation_time = 0.0f;
    for (const auto& anim : animations_) {
        total_animation_time += anim->get_run_time();
    }

    if (total_animation_time <= 0.0f) {
        total_animation_time = 1.0f;
    }

    // Calculate time offsets based on lag_ratio
    // lag_ratio = 0: all start at t=0, end at their respective run_times
    // lag_ratio = 1: sequential (Succession)
    float accumulated_time = 0.0f;

    for (const auto& anim : animations_) {
        AnimationTiming timing;
        timing.animation = anim;

        // Start time is based on accumulated time * lag_ratio
        timing.start_time = accumulated_time * lag_ratio_;

        // End time depends on whether we're normalizing or not
        timing.end_time = timing.start_time + anim->get_run_time();

        timings_.push_back(timing);

        accumulated_time += anim->get_run_time();
    }

    // Calculate the maximum end time for normalization
    max_end_time_ = calculate_max_end_time();

    // Normalize all times to [0, 1]
    if (max_end_time_ > 0.0f) {
        for (auto& timing : timings_) {
            timing.start_time /= max_end_time_;
            timing.end_time /= max_end_time_;
        }
    }
}

float AnimationGroup::calculate_max_end_time() const {
    if (timings_.empty()) {
        return 1.0f;
    }

    float max_end = 0.0f;
    float accumulated = 0.0f;

    for (const auto& anim : animations_) {
        float start = accumulated * lag_ratio_;
        float end = start + anim->get_run_time();
        max_end = std::max(max_end, end);
        accumulated += anim->get_run_time();
    }

    return max_end > 0.0f ? max_end : 1.0f;
}

float AnimationGroup::get_sub_alpha(float global_alpha, const AnimationTiming& timing) const {
    // If global alpha is before this animation starts, return 0
    if (global_alpha <= timing.start_time) {
        return 0.0f;
    }

    // If global alpha is after this animation ends, return 1
    if (global_alpha >= timing.end_time) {
        return 1.0f;
    }

    // Calculate local alpha within the animation's time window
    float duration = timing.end_time - timing.start_time;
    if (duration <= 0.0f) {
        return 1.0f;
    }

    return (global_alpha - timing.start_time) / duration;
}

void AnimationGroup::begin() {
    started_ = true;
    finished_ = false;
    alpha_ = 0.0f;

    // Begin all sub-animations
    for (auto& timing : timings_) {
        timing.animation->begin();
    }

    spdlog::debug("AnimationGroup::begin() - {} animations", animations_.size());
}

void AnimationGroup::interpolate_mobject(float alpha) {
    // Interpolate each animation based on its timing
    for (const auto& timing : timings_) {
        float sub_alpha = get_sub_alpha(alpha, timing);

        // Only interpolate if we're within this animation's active range
        if (sub_alpha > 0.0f) {
            // Apply the animation's own rate function
            float rated_alpha = timing.animation->get_rate_alpha(sub_alpha);
            timing.animation->interpolate(sub_alpha * timing.animation->get_run_time());
        }
    }
}

void AnimationGroup::finish() {
    // Finish all sub-animations
    for (auto& timing : timings_) {
        timing.animation->finish();
    }

    finished_ = true;
    spdlog::debug("AnimationGroup::finish()");
}

bool AnimationGroup::can_run_on_gpu() const {
    // Group can run on GPU if all sub-animations can
    for (const auto& anim : animations_) {
        if (!anim->can_run_on_gpu()) {
            return false;
        }
    }
    return !animations_.empty();
}

void AnimationGroup::upload_to_gpu(GPUAnimationEngine& engine) {
    for (auto& anim : animations_) {
        anim->upload_to_gpu(engine);
    }
}

void AnimationGroup::interpolate_on_gpu(float t, VkCommandBuffer cmd) {
    float alpha = t / run_time_;
    if (rate_func_) {
        alpha = rate_func_(alpha);
    }

    for (const auto& timing : timings_) {
        float sub_alpha = get_sub_alpha(alpha, timing);
        if (sub_alpha > 0.0f) {
            float sub_t = sub_alpha * timing.animation->get_run_time();
            timing.animation->interpolate_on_gpu(sub_t, cmd);
        }
    }
}

std::shared_ptr<Animation> AnimationGroup::get_animation(size_t index) const {
    if (index < animations_.size()) {
        return animations_[index];
    }
    return nullptr;
}

// ============================================================================
// Succession
// ============================================================================

Succession::Succession(
    std::vector<std::shared_ptr<Animation>> animations,
    float run_time,
    RateFunc rate_func
)
    : AnimationGroup(std::move(animations), run_time, 1.0f, rate_func)
    , animation_finished_(animations_.size(), false)
{
    name_ = "Succession";
}

void Succession::build_timings() {
    timings_.clear();
    timings_.reserve(animations_.size());

    if (animations_.empty()) {
        max_end_time_ = 1.0f;
        return;
    }

    // For Succession, animations are strictly sequential
    float current_time = 0.0f;
    float total_time = 0.0f;

    // First pass: calculate total time
    for (const auto& anim : animations_) {
        total_time += anim->get_run_time();
    }

    if (total_time <= 0.0f) {
        total_time = 1.0f;
    }

    max_end_time_ = total_time;

    // Second pass: build normalized timings
    current_time = 0.0f;
    for (const auto& anim : animations_) {
        AnimationTiming timing;
        timing.animation = anim;
        timing.start_time = current_time / total_time;
        current_time += anim->get_run_time();
        timing.end_time = current_time / total_time;

        timings_.push_back(timing);
    }

    // Reset tracking
    animation_finished_.assign(animations_.size(), false);
}

void Succession::advance_to_animation(size_t target_index) {
    // Finish all animations before the target
    for (size_t i = active_index_; i < target_index && i < timings_.size(); ++i) {
        if (!animation_finished_[i]) {
            // Set to end state
            timings_[i].animation->interpolate(timings_[i].animation->get_run_time());
            timings_[i].animation->finish();
            animation_finished_[i] = true;
        }
    }

    // Begin the target animation if not started
    if (target_index < timings_.size() && target_index > active_index_) {
        if (!animation_finished_[target_index]) {
            timings_[target_index].animation->begin();
        }
    }

    active_index_ = target_index;
}

void Succession::interpolate_mobject(float alpha) {
    if (timings_.empty()) {
        return;
    }

    // Find which animation should be active at this alpha
    size_t target_index = 0;
    for (size_t i = 0; i < timings_.size(); ++i) {
        if (alpha >= timings_[i].start_time && alpha <= timings_[i].end_time) {
            target_index = i;
            break;
        } else if (alpha > timings_[i].end_time) {
            target_index = i + 1;
        }
    }

    // Clamp to valid range
    target_index = std::min(target_index, timings_.size() - 1);

    // Advance if needed (finishes previous animations)
    if (target_index > active_index_) {
        advance_to_animation(target_index);
    }

    // Interpolate the active animation
    if (active_index_ < timings_.size()) {
        const auto& timing = timings_[active_index_];
        float sub_alpha = get_sub_alpha(alpha, timing);

        // Apply sub-alpha to the active animation
        float sub_t = sub_alpha * timing.animation->get_run_time();
        timing.animation->interpolate(sub_t);
    }
}

// ============================================================================
// LaggedStart
// ============================================================================

LaggedStart::LaggedStart(
    std::vector<std::shared_ptr<Animation>> animations,
    float lag_ratio,
    float run_time
)
    : AnimationGroup(std::move(animations), run_time, lag_ratio, nullptr)
{
    name_ = "LaggedStart";
}

}  // namespace manim
