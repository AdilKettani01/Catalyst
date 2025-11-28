/**
 * @file composition.hpp
 * @brief Animation composition classes for combining multiple animations
 *
 * Provides:
 * - AnimationGroup: Run multiple animations in parallel with lag support
 * - Succession: Run animations sequentially
 * - LaggedStart: AnimationGroup with staggered start times
 */

#pragma once

#include "manim/animation/animation.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>

namespace manim {

/**
 * @brief Animation timing information for group scheduling
 */
struct AnimationTiming {
    std::shared_ptr<Animation> animation;
    float start_time;   // Normalized [0, 1]
    float end_time;     // Normalized [0, 1]
};

/**
 * @brief Run multiple animations in parallel with optional lag
 *
 * AnimationGroup plays multiple animations together. The lag_ratio parameter
 * controls the staggering:
 * - lag_ratio = 0: All animations start and end together
 * - lag_ratio = 1: Animations play sequentially (like Succession)
 * - lag_ratio between 0 and 1: Staggered start times
 *
 * Example:
 * @code
 * auto group = std::make_shared<AnimationGroup>(
 *     std::vector{fade_in, transform, scale},
 *     2.0f,  // Total run_time
 *     0.2f   // 20% lag ratio
 * );
 * scene.play({group});
 * @endcode
 */
class AnimationGroup : public Animation {
public:
    /**
     * @brief Construct an animation group
     * @param animations Vector of animations to run
     * @param run_time Total duration (-1 = auto-calculate from animations)
     * @param lag_ratio Stagger ratio [0, 1]
     * @param rate_func Rate function for global progress
     */
    AnimationGroup(
        std::vector<std::shared_ptr<Animation>> animations,
        float run_time = -1.0f,
        float lag_ratio = 0.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~AnimationGroup() = default;

    // Animation lifecycle overrides
    void begin() override;
    void finish() override;

    // GPU support
    bool can_run_on_gpu() const override;
    void upload_to_gpu(GPUAnimationEngine& engine) override;
    void interpolate_on_gpu(float t, VkCommandBuffer cmd) override;

    // Accessors
    const std::vector<AnimationTiming>& get_timings() const { return timings_; }
    size_t get_animation_count() const { return animations_.size(); }
    float get_lag_ratio() const { return lag_ratio_; }

    /**
     * @brief Get animation at index
     */
    std::shared_ptr<Animation> get_animation(size_t index) const;

    /**
     * @brief Get all animations
     */
    const std::vector<std::shared_ptr<Animation>>& get_animations() const {
        return animations_;
    }

protected:
    void interpolate_mobject(float alpha) override;

    /**
     * @brief Build timing information for all animations
     */
    virtual void build_timings();

    /**
     * @brief Calculate sub-alpha for a specific animation given global alpha
     * @param global_alpha Global progress [0, 1]
     * @param timing Animation timing info
     * @return Local alpha for the animation [0, 1]
     */
    float get_sub_alpha(float global_alpha, const AnimationTiming& timing) const;

    /**
     * @brief Calculate total duration from animations
     */
    float calculate_max_end_time() const;

    std::vector<std::shared_ptr<Animation>> animations_;
    std::vector<AnimationTiming> timings_;
    float lag_ratio_{0.0f};
    float max_end_time_{1.0f};
};

/**
 * @brief Run animations one after another
 *
 * Succession plays animations sequentially. Each animation must complete
 * before the next one begins. This is equivalent to AnimationGroup with
 * lag_ratio = 1.0.
 *
 * Example:
 * @code
 * auto succession = std::make_shared<Succession>(
 *     std::vector{write, transform, fade_out}
 * );
 * scene.play({succession});
 * @endcode
 */
class Succession : public AnimationGroup {
public:
    /**
     * @brief Construct a succession
     * @param animations Vector of animations to run sequentially
     * @param run_time Total duration (-1 = sum of animation run times)
     * @param rate_func Rate function for global progress
     */
    Succession(
        std::vector<std::shared_ptr<Animation>> animations,
        float run_time = -1.0f,
        RateFunc rate_func = nullptr
    );

    virtual ~Succession() = default;

protected:
    void interpolate_mobject(float alpha) override;

    /**
     * @brief Build sequential timings
     */
    void build_timings() override;

    /**
     * @brief Advance to a specific animation index
     *
     * Finishes all animations before the target index to ensure
     * proper state transitions.
     */
    void advance_to_animation(size_t target_index);

private:
    size_t active_index_{0};
    std::vector<bool> animation_finished_;
};

/**
 * @brief AnimationGroup with default staggered start times
 *
 * LaggedStart is a convenience class that creates an AnimationGroup
 * with a default lag_ratio of 0.05 (5%). This creates a nice staggered
 * effect when animating multiple similar objects.
 *
 * Example:
 * @code
 * std::vector<std::shared_ptr<Animation>> fades;
 * for (auto& mobject : mobjects) {
 *     fades.push_back(std::make_shared<FadeIn>(mobject));
 * }
 * auto lagged = std::make_shared<LaggedStart>(fades, 0.1f);
 * scene.play({lagged});
 * @endcode
 */
class LaggedStart : public AnimationGroup {
public:
    /**
     * @brief Construct a lagged start animation
     * @param animations Vector of animations
     * @param lag_ratio Stagger ratio (default 0.05 = 5%)
     * @param run_time Total duration (-1 = auto-calculate)
     */
    LaggedStart(
        std::vector<std::shared_ptr<Animation>> animations,
        float lag_ratio = 0.05f,
        float run_time = -1.0f
    );

    virtual ~LaggedStart() = default;
};

/**
 * @brief Factory function to create LaggedStart from mobjects
 *
 * Creates the same animation type for each mobject with staggered starts.
 *
 * @tparam AnimClass Animation class to instantiate
 * @tparam Args Additional constructor arguments
 * @param mobjects Vector of mobjects to animate
 * @param lag_ratio Stagger ratio
 * @param args Additional arguments passed to animation constructor
 * @return Shared pointer to LaggedStart
 *
 * Example:
 * @code
 * auto lagged = make_lagged_start_map<FadeIn>(
 *     mobjects,
 *     0.1f,  // lag_ratio
 *     1.0f   // run_time for each FadeIn
 * );
 * @endcode
 */
template<typename AnimClass, typename... Args>
std::shared_ptr<LaggedStart> make_lagged_start_map(
    const std::vector<std::shared_ptr<Mobject>>& mobjects,
    float lag_ratio = 0.05f,
    Args&&... args
) {
    std::vector<std::shared_ptr<Animation>> animations;
    animations.reserve(mobjects.size());

    for (const auto& mobject : mobjects) {
        animations.push_back(
            std::make_shared<AnimClass>(mobject, std::forward<Args>(args)...)
        );
    }

    return std::make_shared<LaggedStart>(std::move(animations), lag_ratio);
}

/**
 * @brief Create LaggedStart from submobjects of a parent
 *
 * Useful for animating all children of a VGroup or similar container.
 *
 * @tparam AnimClass Animation class to instantiate
 * @tparam Args Additional constructor arguments
 */
template<typename AnimClass, typename... Args>
std::shared_ptr<LaggedStart> make_lagged_start_from_submobjects(
    std::shared_ptr<Mobject> parent,
    float lag_ratio = 0.05f,
    Args&&... args
) {
    return make_lagged_start_map<AnimClass>(
        parent->get_submobjects(),
        lag_ratio,
        std::forward<Args>(args)...
    );
}

}  // namespace manim
