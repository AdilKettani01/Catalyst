#pragma once

#include "manim/animation/animation.hpp"
#include "manim/core/compute_engine.hpp"
#include <vector>
#include <unordered_map>
#include <queue>

namespace manim {

/**
 * @brief GPU Animation Engine for parallel animation processing
 *
 * Processes multiple animations in parallel on GPU:
 * - Batch similar animations together
 * - Parallel interpolation on compute shaders
 * - GPU rate function evaluation
 * - Animation result caching
 *
 * Performance: 100x faster for batch animations
 */
class GPUAnimationEngine {
public:
    explicit GPUAnimationEngine(
        std::shared_ptr<ComputeEngine> compute_engine,
        std::shared_ptr<MemoryPool> memory_pool
    );

    ~GPUAnimationEngine() = default;

    // ========================================================================
    // Animation Processing
    // ========================================================================

    /**
     * @brief Process animations on GPU at time t
     * @param animations List of animations to process
     * @param t Current time
     */
    void process_animations_gpu(
        const std::vector<std::shared_ptr<Animation>>& animations,
        float t
    );

    /**
     * @brief Batch process similar animations
     * @param animations Animations to batch
     * @param t Current time
     */
    void batch_process(
        const std::vector<std::shared_ptr<Animation>>& animations,
        float t
    );

    // ========================================================================
    // GPU Interpolation
    // ========================================================================

    /**
     * @brief Interpolate transforms on GPU
     * @param start_transforms Starting transforms
     * @param end_transforms Ending transforms
     * @param alpha Interpolation parameter [0,1]
     * @param output Output buffer
     */
    void interpolate_transforms_gpu(
        const GPUBuffer& start_transforms,
        const GPUBuffer& end_transforms,
        float alpha,
        GPUBuffer& output
    );

    /**
     * @brief Interpolate colors on GPU
     */
    void interpolate_colors_gpu(
        const GPUBuffer& start_colors,
        const GPUBuffer& end_colors,
        float alpha,
        GPUBuffer& output
    );

    /**
     * @brief Interpolate points on GPU (for morphing)
     */
    void interpolate_points_gpu(
        const GPUBuffer& start_points,
        const GPUBuffer& end_points,
        float alpha,
        GPUBuffer& output
    );

    /**
     * @brief Morph shapes on GPU (Bezier curve morphing)
     */
    void morph_shapes_gpu(
        const GPUBuffer& start_curves,
        const GPUBuffer& end_curves,
        float alpha,
        GPUBuffer& output
    );

    // ========================================================================
    // GPU Rate Functions
    // ========================================================================

    /**
     * @brief Apply rate function to alpha values on GPU
     * @param alphas Input alpha values
     * @param rate_func Rate function to apply
     * @param output Output alpha values
     */
    void apply_rate_function_gpu(
        const GPUBuffer& alphas,
        RateFunc rate_func,
        GPUBuffer& output
    );

    /**
     * @brief Precompute rate function lookup table
     */
    void precompute_rate_function_lut(
        RateFunc rate_func,
        uint32_t samples = 1024
    );

    // ========================================================================
    // Animation Batching
    // ========================================================================

    enum class AnimationType {
        Transform,
        Fade,
        ColorChange,
        Scale,
        Rotate,
        Morph,
        Custom
    };

    /**
     * @brief Classify animation for batching
     */
    AnimationType classify_animation(const Animation& anim) const;

    /**
     * @brief Group animations by type for batching
     */
    std::unordered_map<AnimationType, std::vector<std::shared_ptr<Animation>>>
    group_animations_by_type(
        const std::vector<std::shared_ptr<Animation>>& animations
    ) const;

    /**
     * @brief Execute animation batch
     */
    void execute_animation_batch(
        AnimationType type,
        const std::vector<std::shared_ptr<Animation>>& batch,
        float t
    );

    // ========================================================================
    // Caching
    // ========================================================================

    /**
     * @brief Cache animation interpolation results
     */
    struct AnimationCache {
        std::optional<GPUBuffer> cached_states;  // Pre-computed states
        std::vector<float> sample_times;         // Sample timestamps
        uint32_t num_samples;
        bool valid{false};
    };

    /**
     * @brief Enable caching for animation
     */
    void enable_caching(const Animation& anim, uint32_t num_samples = 60);

    /**
     * @brief Get cached animation state
     */
    bool get_cached_state(const Animation& anim, float t, GPUBuffer& output);

    /**
     * @brief Clear cache
     */
    void clear_cache();

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Statistics {
        uint32_t animations_processed{0};
        uint32_t batch_count{0};
        uint32_t cache_hits{0};
        uint32_t cache_misses{0};
        float gpu_time_ms{0.0f};
    };

    const Statistics& get_statistics() const { return stats_; }
    void reset_statistics() { stats_ = Statistics{}; }

private:
    std::shared_ptr<ComputeEngine> compute_engine_;
    std::shared_ptr<MemoryPool> memory_pool_;

    // Animation cache
    std::unordered_map<size_t, AnimationCache> cache_;  // Animation hash -> cache

    // Rate function LUTs
    std::unordered_map<size_t, GPUBuffer> rate_func_luts_;  // Func hash -> LUT

    // Statistics
    Statistics stats_;

    // Compute shaders
    // These would be loaded from shader files
    // VkPipeline transform_interpolate_pipeline_;
    // VkPipeline color_interpolate_pipeline_;
    // VkPipeline morph_pipeline_;
    // VkPipeline rate_func_pipeline_;
};

}  // namespace manim
