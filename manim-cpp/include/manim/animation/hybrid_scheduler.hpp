#pragma once

#include "manim/animation/animation.hpp"
#include "manim/animation/gpu_animation_engine.hpp"
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <future>

namespace manim {

/**
 * @brief Hybrid CPU-GPU Animation Scheduler
 *
 * Intelligently distributes animations between CPU and GPU:
 * - Simple animations (transform, fade, scale) → GPU batch processing
 * - Complex animations (procedural, conditional) → CPU multi-threading
 * - Hybrid animations (CPU logic + GPU execution) → Both
 * - Automatic load balancing based on complexity
 *
 * Performance: Optimal utilization of CPU and GPU resources
 */
class HybridAnimationScheduler {
public:
    HybridAnimationScheduler(
        std::shared_ptr<GPUAnimationEngine> gpu_engine,
        uint32_t cpu_thread_count = 0  // 0 = auto-detect
    );

    ~HybridAnimationScheduler();

    // ========================================================================
    // Animation Scheduling
    // ========================================================================

    /**
     * @brief Schedule animations for execution
     * @param animations List of animations to schedule
     */
    void schedule_animations(
        const std::vector<std::shared_ptr<Animation>>& animations
    );

    /**
     * @brief Execute scheduled animations at time t
     * @param t Current time
     */
    void execute_animations(float t);

    /**
     * @brief Clear scheduled animations
     */
    void clear_schedule();

    // ========================================================================
    // Animation Classification
    // ========================================================================

    enum class AnimationComplexity {
        Simple,      // GPU-friendly (transforms, fades, colors)
        Medium,      // Hybrid (some CPU logic needed)
        Complex,     // CPU-only (procedural, conditional)
        VeryComplex  // Multi-threaded CPU
    };

    /**
     * @brief Analyze animation complexity
     */
    AnimationComplexity classify_animation(const Animation& anim) const;

    /**
     * @brief Classify all scheduled animations
     */
    void classify_animations();

    // ========================================================================
    // GPU Scheduling
    // ========================================================================

    /**
     * @brief Schedule simple animations for GPU batch processing
     */
    void schedule_gpu_animations();

    /**
     * @brief Execute GPU animation batches
     */
    void execute_gpu_batches(float t);

    // ========================================================================
    // CPU Scheduling
    // ========================================================================

    /**
     * @brief Schedule complex animations for CPU processing
     */
    void schedule_cpu_animations();

    /**
     * @brief Execute CPU animations (multi-threaded)
     */
    void execute_cpu_animations(float t);

    /**
     * @brief Process procedural animations on CPU
     */
    void process_procedural_animations(float t);

    /**
     * @brief Handle conditional animations
     */
    void handle_conditional_animations(float t);

    // ========================================================================
    // Hybrid Processing
    // ========================================================================

    /**
     * @brief Execute hybrid animations (CPU logic + GPU execution)
     */
    void execute_hybrid_animations(float t);

    // ========================================================================
    // Synchronization
    // ========================================================================

    /**
     * @brief Synchronize CPU and GPU timelines
     */
    void synchronize_timelines();

    /**
     * @brief Wait for all animations to complete
     */
    void wait_for_completion();

    // ========================================================================
    // Load Balancing
    // ========================================================================

    struct LoadBalance {
        float cpu_utilization{0.0f};
        float gpu_utilization{0.0f};
        uint32_t cpu_animation_count{0};
        uint32_t gpu_animation_count{0};
        uint32_t hybrid_animation_count{0};
    };

    /**
     * @brief Get current load balance
     */
    const LoadBalance& get_load_balance() const { return load_balance_; }

    /**
     * @brief Adjust distribution based on performance
     */
    void adjust_distribution();

    // ========================================================================
    // Optimization Strategies
    // ========================================================================

    enum class Strategy {
        Auto,           // Automatic classification
        PreferGPU,      // Maximize GPU usage
        PreferCPU,      // Maximize CPU usage
        Balanced,       // Balance CPU and GPU
        MinLatency      // Minimize total execution time
    };

    /**
     * @brief Set scheduling strategy
     */
    void set_strategy(Strategy strategy) { strategy_ = strategy; }

    Strategy get_strategy() const { return strategy_; }

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Statistics {
        uint32_t total_animations{0};
        uint32_t gpu_animations{0};
        uint32_t cpu_animations{0};
        uint32_t hybrid_animations{0};
        float total_time_ms{0.0f};
        float gpu_time_ms{0.0f};
        float cpu_time_ms{0.0f};
        uint32_t batches_executed{0};
    };

    const Statistics& get_statistics() const { return stats_; }
    void reset_statistics() { stats_ = Statistics{}; }

private:
    // GPU engine
    std::shared_ptr<GPUAnimationEngine> gpu_engine_;

    // CPU thread pool
    class ThreadPool {
    public:
        explicit ThreadPool(uint32_t thread_count);
        ~ThreadPool();

        template<typename F>
        std::future<void> enqueue(F&& f);

        void wait_all();

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_{false};
        std::atomic<uint32_t> active_tasks_{0};
    };

    std::unique_ptr<ThreadPool> cpu_thread_pool_;

    // Scheduled animations (classified)
    struct ClassifiedAnimations {
        std::vector<std::shared_ptr<Animation>> simple;     // GPU
        std::vector<std::shared_ptr<Animation>> medium;     // Hybrid
        std::vector<std::shared_ptr<Animation>> complex;    // CPU
        std::vector<std::shared_ptr<Animation>> very_complex; // Multi-threaded CPU
    };

    ClassifiedAnimations classified_animations_;

    // All scheduled animations
    std::vector<std::shared_ptr<Animation>> all_animations_;

    // Strategy
    Strategy strategy_{Strategy::Auto};

    // Load balancing
    LoadBalance load_balance_;

    // Statistics
    Statistics stats_;

    // Complexity thresholds
    struct ComplexityThresholds {
        uint32_t simple_max_points{1000};        // < 1K points → GPU
        uint32_t medium_max_points{10000};       // < 10K points → Hybrid
        uint32_t complex_max_points{100000};     // < 100K points → CPU
        // > 100K points → Multi-threaded CPU
    };

    ComplexityThresholds thresholds_;

    // Helpers
    bool is_simple_transform(const Animation& anim) const;
    bool is_batchable(const Animation& anim) const;
    bool requires_cpu_logic(const Animation& anim) const;
};

}  // namespace manim
