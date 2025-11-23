#pragma once

#include "manim/mobject/mobject.hpp"
#include "manim/core/math.hpp"
#include <memory>
#include <functional>
#include <string>

namespace manim {

// Forward declarations
class Scene;
class GPUAnimationEngine;

/**
 * @brief Rate function type: [0,1] -> [0,1]
 */
using RateFunc = std::function<float(float)>;

/**
 * @brief Base animation class with GPU acceleration support
 *
 * Animations modify mobject properties over time. This class supports:
 * - CPU animation for complex/procedural animations
 * - GPU batch processing for simple transformations
 * - Hybrid scheduling for optimal performance
 * - Animation caching for reuse
 *
 * GPU Optimizations:
 * - Batch similar animations together
 * - Parallel interpolation on compute shaders
 * - Pre-computed animation buffers
 * - Cached rate function evaluations
 */
class Animation {
public:
    /**
     * @brief Animation execution mode
     */
    enum class ExecutionMode {
        CPU,           // Complex animations, procedural
        GPU,           // Simple transforms, batch-able
        Hybrid,        // Mix of CPU logic + GPU execution
        Auto           // Let scheduler decide
    };

    Animation(
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~Animation() = default;

    // ========================================================================
    // Animation Lifecycle
    // ========================================================================

    /**
     * @brief Initialize animation (called once before start)
     */
    virtual void begin();

    /**
     * @brief Update animation at time t [0, run_time]
     * @param t Current time in seconds
     */
    virtual void interpolate(float t);

    /**
     * @brief Finalize animation (called once after completion)
     */
    virtual void finish();

    /**
     * @brief Complete lifecycle: begin -> interpolate(0 to 1) -> finish
     */
    void play();

    // ========================================================================
    // GPU Support
    // ========================================================================

    /**
     * @brief Check if animation can run on GPU
     */
    virtual bool can_run_on_gpu() const { return false; }

    /**
     * @brief Get execution mode
     */
    ExecutionMode get_execution_mode() const { return execution_mode_; }

    /**
     * @brief Set execution mode
     */
    void set_execution_mode(ExecutionMode mode) { execution_mode_ = mode; }

    /**
     * @brief Upload animation data to GPU (for GPU-accelerated animations)
     */
    virtual void upload_to_gpu(GPUAnimationEngine& engine);

    /**
     * @brief Interpolate on GPU (for batch processing)
     */
    virtual void interpolate_on_gpu(float t, VkCommandBuffer cmd);

    // ========================================================================
    // Animation Properties
    // ========================================================================

    /**
     * @brief Get animation duration
     */
    float get_run_time() const { return run_time_; }

    /**
     * @brief Set animation duration
     */
    Animation& set_run_time(float time) { run_time_ = time; return *this; }

    /**
     * @brief Get rate function
     */
    RateFunc get_rate_func() const { return rate_func_; }

    /**
     * @brief Set rate function
     */
    Animation& set_rate_func(RateFunc func) { rate_func_ = func; return *this; }

    /**
     * @brief Get target mobject
     */
    std::shared_ptr<Mobject> get_mobject() const { return mobject_; }

    /**
     * @brief Set remover flag (remove mobject after animation)
     */
    Animation& set_remover(bool remover) { remover_ = remover; return *this; }

    bool is_remover() const { return remover_; }

    /**
     * @brief Set introducer flag (add mobject at start)
     */
    Animation& set_introducer(bool introducer) { introducer_ = introducer; return *this; }

    bool is_introducer() const { return introducer_; }

    // ========================================================================
    // Animation State
    // ========================================================================

    /**
     * @brief Check if animation is started
     */
    bool is_started() const { return started_; }

    /**
     * @brief Check if animation is finished
     */
    bool is_finished() const { return finished_; }

    /**
     * @brief Get current alpha (progress) [0, 1]
     */
    float get_alpha() const { return alpha_; }

    /**
     * @brief Get name
     */
    std::string get_name() const { return name_; }

    /**
     * @brief Set name
     */
    void set_name(const std::string& name) { name_ = name; }

    // ========================================================================
    // Animation Composition
    // ========================================================================

    /**
     * @brief Set suspend mobject updating during animation
     */
    void set_suspend_mobject_updating(bool suspend) {
        suspend_mobject_updating_ = suspend;
    }

    // ========================================================================
    // Utilities
    // ========================================================================

    /**
     * @brief Get interpolated alpha (with rate function applied)
     */
    float get_rate_alpha(float t) const;

    /**
     * @brief Clone animation
     */
    virtual std::shared_ptr<Animation> copy() const;

protected:
    /**
     * @brief Override to implement custom interpolation
     * @param alpha Interpolation parameter [0, 1] (rate function already applied)
     */
    virtual void interpolate_mobject(float alpha);

    // Target mobject
    std::shared_ptr<Mobject> mobject_;

    // Animation parameters
    float run_time_{1.0f};
    RateFunc rate_func_;

    // State
    bool started_{false};
    bool finished_{false};
    float alpha_{0.0f};

    // Flags
    bool remover_{false};              // Remove mobject after animation
    bool introducer_{false};           // Add mobject at start
    bool suspend_mobject_updating_{true};

    // Execution
    ExecutionMode execution_mode_{ExecutionMode::Auto};

    // Metadata
    std::string name_;

    // GPU data (for cacheable animations)
    struct GPUAnimationData {
        std::optional<GPUBuffer> start_state;
        std::optional<GPUBuffer> end_state;
        std::optional<GPUBuffer> interpolated_state;
        bool uploaded{false};
    };

    GPUAnimationData gpu_data_;
};

// ============================================================================
// Common Rate Functions
// ============================================================================

namespace rate_functions {

/**
 * @brief Linear interpolation (default)
 */
float linear(float t);

/**
 * @brief Smooth interpolation (3t^2 - 2t^3)
 */
float smooth(float t);

/**
 * @brief Double smooth (smoother than smooth)
 */
float double_smooth(float t);

/**
 * @brief Rush into (accelerate)
 */
float rush_into(float t);

/**
 * @brief Rush from (decelerate)
 */
float rush_from(float t);

/**
 * @brief Slow into (ease in)
 */
float slow_into(float t);

/**
 * @brief Exponential decay
 */
float exponential_decay(float t);

/**
 * @brief There and back (0 -> 1 -> 0)
 */
float there_and_back(float t);

/**
 * @brief There and back with pause
 */
float there_and_back_with_pause(float t, float pause_ratio = 1.0f / 3.0f);

/**
 * @brief Running start (overshoot then return)
 */
float running_start(float t, float pull_factor = -0.5f);

/**
 * @brief Wiggle (oscillate)
 */
float wiggle(float t, int wiggles = 2);

/**
 * @brief Ease in sine
 */
float ease_in_sine(float t);

/**
 * @brief Ease out sine
 */
float ease_out_sine(float t);

/**
 * @brief Ease in out sine
 */
float ease_in_out_sine(float t);

/**
 * @brief Ease in cubic
 */
float ease_in_cubic(float t);

/**
 * @brief Ease out cubic
 */
float ease_out_cubic(float t);

/**
 * @brief Ease in out cubic
 */
float ease_in_out_cubic(float t);

/**
 * @brief Ease in elastic
 */
float ease_in_elastic(float t);

/**
 * @brief Ease out elastic
 */
float ease_out_elastic(float t);

/**
 * @brief Ease in out elastic
 */
float ease_in_out_elastic(float t);

}  // namespace rate_functions

}  // namespace manim
