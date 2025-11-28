// Base Animation implementation with GPU support
#include "manim/animation/animation.hpp"
#include "manim/animation/gpu_animation_engine.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace manim {

// ============================================================================
// Animation Implementation
// ============================================================================

Animation::Animation(
    std::shared_ptr<Mobject> mobject,
    float run_time,
    RateFunc rate_func
) : mobject_(mobject),
    run_time_(run_time),
    rate_func_(rate_func ? rate_func : rate_functions::linear) {

    if (!mobject) {
        throw std::invalid_argument("Animation mobject cannot be null");
    }

    name_ = typeid(*this).name();
}

void Animation::begin() {
    if (started_) {
        return;
    }

    started_ = true;
    finished_ = false;
    alpha_ = 0.0f;

    // Suspend mobject updating if requested
    if (suspend_mobject_updating_) {
        mobject_->suspend_updating();
    }

    spdlog::debug("Animation '{}' started", name_);
}

void Animation::interpolate(float t) {
    if (!started_) {
        begin();
    }

    if (finished_) {
        return;
    }

    // Clamp t to [0, run_time]
    t = std::clamp(t, 0.0f, run_time_);

    // Compute alpha [0, 1]
    alpha_ = (run_time_ > 0.0f) ? (t / run_time_) : 1.0f;

    // Apply rate function
    float rate_alpha = get_rate_alpha(alpha_);

    // Interpolate mobject
    interpolate_mobject(rate_alpha);
}

void Animation::finish() {
    if (finished_) {
        return;
    }

    // Ensure final state
    interpolate(run_time_);

    finished_ = true;

    // Resume mobject updating
    if (suspend_mobject_updating_) {
        mobject_->resume_updating();
    }

    spdlog::debug("Animation '{}' finished", name_);
}

void Animation::play() {
    begin();

    // Sample animation at regular intervals
    const int samples = 60;
    for (int i = 0; i <= samples; ++i) {
        float t = (static_cast<float>(i) / samples) * run_time_;
        interpolate(t);
    }

    finish();
}

float Animation::get_rate_alpha(float t) const {
    if (rate_func_) {
        return rate_func_(std::clamp(t, 0.0f, 1.0f));
    }
    return t;
}

void Animation::interpolate_mobject(float alpha) {
    // Override in subclasses
    spdlog::trace("Interpolating animation '{}' at alpha={:.3f}", name_, alpha);
}

void Animation::upload_to_gpu(GPUAnimationEngine& engine) {
    // Override in GPU-accelerated animations
}

void Animation::interpolate_on_gpu(float t, VkCommandBuffer cmd) {
    // Override in GPU-accelerated animations
}

std::shared_ptr<Animation> Animation::copy() const {
    auto copied = std::make_shared<Animation>(mobject_, run_time_, rate_func_);
    copied->execution_mode_ = execution_mode_;
    copied->remover_ = remover_;
    copied->introducer_ = introducer_;
    copied->name_ = name_;
    return copied;
}

// ============================================================================
// Rate Functions Implementation
// ============================================================================

namespace rate_functions {

float linear(float t) {
    return t;
}

float smooth(float t) {
    // 3t^2 - 2t^3 (smoothstep)
    return t * t * (3.0f - 2.0f * t);
}

float double_smooth(float t) {
    // Apply smooth twice
    float s1 = smooth(t);
    return smooth(s1);
}

float rush_into(float t) {
    // Accelerate (quadratic ease-in)
    return t * t;
}

float rush_from(float t) {
    // Decelerate (quadratic ease-out)
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float slow_into(float t) {
    // Ease in (similar to rush_into)
    return rush_into(t);
}

float exponential_decay(float t) {
    return 1.0f - std::exp(-5.0f * t);
}

float there_and_back(float t) {
    // 0 -> 1 -> 0 (parabola)
    return 4.0f * t * (1.0f - t);
}

float there_and_back_with_pause(float t, float pause_ratio) {
    float pause = pause_ratio;
    float total = 1.0f + pause;

    if (t < 0.5f / total) {
        // Going there
        return 2.0f * total * t;
    } else if (t < (0.5f + pause) / total) {
        // Pausing
        return 1.0f;
    } else {
        // Coming back
        return 2.0f * total * (1.0f - t);
    }
}

float running_start(float t, float pull_factor) {
    // Overshoot then return
    return t + pull_factor * std::sin(t * math::PI);
}

float wiggle(float t, int wiggles) {
    // Oscillate
    return std::sin(t * 2.0f * math::PI * wiggles) * (1.0f - t);
}

float ease_in_sine(float t) {
    return 1.0f - std::cos(t * math::PI / 2.0f);
}

float ease_out_sine(float t) {
    return std::sin(t * math::PI / 2.0f);
}

float ease_in_out_sine(float t) {
    return -(std::cos(math::PI * t) - 1.0f) / 2.0f;
}

float ease_in_cubic(float t) {
    return t * t * t;
}

float ease_out_cubic(float t) {
    float s = 1.0f - t;
    return 1.0f - s * s * s;
}

float ease_in_out_cubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float s = -2.0f * t + 2.0f;
        return 1.0f - s * s * s / 2.0f;
    }
}

float ease_in_elastic(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;

    float c4 = (2.0f * math::PI) / 3.0f;
    return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
}

float ease_out_elastic(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;

    float c4 = (2.0f * math::PI) / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float ease_in_out_elastic(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;

    float c5 = (2.0f * math::PI) / 4.5f;

    if (t < 0.5f) {
        return -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f;
    } else {
        return (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
    }
}

}  // namespace rate_functions

}  // namespace manim
