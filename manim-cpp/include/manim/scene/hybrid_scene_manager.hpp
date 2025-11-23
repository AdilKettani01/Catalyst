/**
 * @file hybrid_scene_manager.hpp
 * @brief Hybrid scene management for CPU/GPU workload distribution
 */

#pragma once

#include <manim/scene/scene.hpp>
#include <manim/mobject/mobject.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

namespace manim {

/**
 * @brief Hybrid scene manager for CPU/GPU workload distribution
 */
class HybridSceneManager {
public:
    HybridSceneManager();
    ~HybridSceneManager();

    /**
     * @brief Add scene to manager
     */
    void add_scene(std::shared_ptr<Scene> scene);

    /**
     * @brief Remove scene from manager
     */
    void remove_scene(std::shared_ptr<Scene> scene);

    /**
     * @brief Update all scenes
     */
    void update(float dt);

    /**
     * @brief Render all scenes
     */
    void render();

    /**
     * @brief Set CPU/GPU distribution strategy
     */
    enum class DistributionStrategy {
        Auto,      // Automatic based on workload
        CPUOnly,   // Force CPU rendering
        GPUOnly,   // Force GPU rendering
        Balanced   // Manual 50/50 split
    };

    void set_distribution_strategy(DistributionStrategy strategy);

    /**
     * @brief Get performance metrics
     */
    struct PerformanceMetrics {
        float cpu_usage;
        float gpu_usage;
        float frame_time;
        uint32_t draw_calls;
        uint32_t triangles;
    };

    PerformanceMetrics get_metrics() const;

private:
    std::vector<std::shared_ptr<Scene>> scenes_;
    DistributionStrategy strategy_ = DistributionStrategy::Auto;

    // Thread management
    std::vector<std::thread> worker_threads_;
    std::mutex scenes_mutex_;

    // Performance tracking
    mutable PerformanceMetrics metrics_{};

    // Workload distribution
    void distribute_workload();
    void process_cpu_tasks();
    void process_gpu_tasks();
};

}  // namespace manim