/**
 * @file hybrid_renderer.hpp
 * @brief Hybrid CPU-GPU renderer with intelligent work distribution
 *
 * Dynamically distributes work between CPU and GPU based on:
 * - Data size
 * - Computation complexity
 * - Current GPU load
 * - Memory availability
 */

#pragma once

#include <manim/renderer/renderer.hpp>
#include <manim/renderer/gpu_3d_renderer.hpp>
#include <manim/core/compute_engine.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>
#include <condition_variable>
#include <memory>

namespace manim {

class GPU3DScene;

/**
 * @brief Work item that can be executed on CPU or GPU
 */
struct RenderWork {
    enum class Type {
        Transform,         ///< Point transformation
        Tessellation,      ///< Bezier tessellation
        Lighting,          ///< Lighting calculation
        Physics,           ///< Physics simulation
        Particles,         ///< Particle update
        Culling,           ///< Frustum/occlusion culling
        DrawCall           ///< Draw call submission
    };

    Type type;
    size_t data_size;      ///< Size of data to process
    float complexity;      ///< Computational complexity (0-1)
    std::function<void()> cpu_function;
    std::function<void(VkCommandBuffer)> gpu_function;

    // Performance hints
    bool prefer_gpu = true;
    bool can_parallelize = true;
};

/**
 * @brief Scene complexity analysis results
 */
struct SceneComplexity {
    uint32_t num_mobjects = 0;
    uint32_t num_points = 0;
    uint32_t num_lights = 0;
    uint32_t num_particles = 0;
    uint32_t total_vertices = 0;
    uint32_t total_triangles = 0;

    bool has_transparency = false;
    bool has_animations = false;
    bool has_physics = false;

    float estimated_cpu_time_ms = 0.0f;
    float estimated_gpu_time_ms = 0.0f;
    float complexity_score = 0.0f;

    // Bottleneck detection
    enum class Bottleneck {
        None,
        CPUBound,
        GPUBound,
        MemoryBound,
        BandwidthBound
    };
    Bottleneck bottleneck = Bottleneck::None;
};

struct HybridRenderStats {
    float gpu_utilization = 0.0f;
    float cpu_utilization = 0.0f;
    int quality_level = 0;
    float frame_time_ms = 0.0f;
};

/**
 * @brief Work distribution strategy
 */
enum class DistributionStrategy {
    Static,         ///< Fixed CPU/GPU split
    Dynamic,        ///< Adaptive based on load
    Greedy,         ///< Always use faster option
    LoadBalanced    ///< Balance CPU/GPU load
};

/**
 * @brief Hybrid CPU-GPU renderer
 *
 * Features:
 * - Automatic work distribution
 * - Dynamic load balancing
 * - CPU-GPU synchronization
 * - Performance monitoring
 * - Bottleneck detection
 */
class HybridRenderer : public Renderer {
public:
    HybridRenderer();
    ~HybridRenderer() override;

    // Renderer interface
    void initialize(const RendererConfig& config) override;
    void shutdown() override;
    void begin_frame() override;
    void end_frame() override;
    void render_scene(Scene& scene, Camera& camera) override;
    void render_mobject(Mobject& mobject) override;
    void clear(const math::Vec4& color) override;
    void resize(uint32_t width, uint32_t height) override;
    const FrameStats& get_stats() const override { return stats_; }
    void capture_frame(const std::string& output_path) override;
    RendererType get_type() const override { return RendererType::Vulkan; }
    bool supports_feature(const std::string& feature) const override;
    MemoryPool& get_memory_pool() override { return *memory_pool_; }
    void set_render_mode(RenderMode mode) override { render_mode_ = mode; }

    // ========================================================================
    // Scene Analysis
    // ========================================================================

    /**
     * @brief Analyze scene complexity
     */
    SceneComplexity analyze_scene_complexity(Scene& scene);

    /**
     * @brief Detect current bottleneck
     */
    SceneComplexity::Bottleneck detect_bottleneck();

    // ========================================================================
    // Work Distribution
    // ========================================================================

    /**
     * @brief Distribute rendering tasks between CPU and GPU
     */
    void distribute_render_tasks(Scene& scene);

    /**
     * @brief Submit work item for processing
     */
    void submit_work(RenderWork work);

    /**
     * @brief Synchronize CPU and GPU work
     */
    void sync_cpu_gpu_work();

    /**
     * @brief Set distribution strategy
     */
    void set_distribution_strategy(DistributionStrategy strategy) {
        strategy_ = strategy;
    }

    // ========================================================================
    // CPU Tasks
    // ========================================================================

    /**
     * @brief Update scene logic on CPU
     */
    void update_scene_logic(Scene& scene, float dt);

    /**
     * @brief Perform frustum culling on CPU
     */
    void cull_objects(Scene& scene, Camera& camera);

    /**
     * @brief Prepare draw calls on CPU
     */
    void prepare_draw_calls(Scene& scene);

    /**
     * @brief Sort transparent objects (CPU)
     */
    void sort_transparent_objects(
        std::vector<Mobject*>& objects,
        const Camera& camera
    );

    // ========================================================================
    // GPU Tasks
    // ========================================================================

    /**
     * @brief Execute all draw calls on GPU
     */
    void execute_draw_calls(VkCommandBuffer cmd);

    /**
     * @brief Compute physics on GPU
     */
    void compute_physics_gpu(VkCommandBuffer cmd, float dt);

    /**
     * @brief Process particles on GPU
     */
    void process_particles_gpu(VkCommandBuffer cmd, float dt);

    /**
     * @brief Transform points in parallel on GPU
     */
    void transform_points_gpu(
        VkCommandBuffer cmd,
        const std::vector<Mobject*>& mobjects
    );

    // ========================================================================
    // Load Balancing
    // ========================================================================

    /**
     * @brief Get current CPU utilization (0-1)
     */
    float get_cpu_utilization() const { return cpu_utilization_.load(); }

    /**
     * @brief Get current GPU utilization (0-1)
     */
    float get_gpu_utilization() const { return gpu_utilization_.load(); }

    /**
     * @brief Decide whether to use CPU or GPU for work item
     */
    bool should_use_gpu(const RenderWork& work) const;

    /**
     * @brief Adjust work distribution based on performance
     */
    void adjust_distribution();

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set CPU thread count
     */
    void set_cpu_thread_count(uint32_t count);

    /**
     * @brief Enable/disable specific features
     */
    void enable_3d_rendering(bool enable) { use_3d_renderer_ = enable; }
    void enable_ray_tracing(bool enable);
    void enable_global_illumination(bool enable);
    void set_mode(RenderMode mode);
    HybridRenderStats render(const std::shared_ptr<GPU3DScene>& scene);
    void enable_adaptive_quality(bool enable) { adaptive_quality_enabled_ = enable; }
    void set_target_fps(float fps) { target_fps_ = fps; }

private:
    // GPU renderer
    std::unique_ptr<GPU3DRenderer> gpu_renderer_;

    // Compute engine for GPU tasks
    std::unique_ptr<ComputeEngine> compute_engine_;

    // CPU thread pool
    class ThreadPool {
    public:
        explicit ThreadPool(size_t num_threads);
        ~ThreadPool();

        template<typename F>
        void enqueue(F&& f);

        void wait_all();

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_{false};
        std::atomic<size_t> active_tasks_{0};
    };

    std::unique_ptr<ThreadPool> thread_pool_;

    // Work queues
    struct WorkQueue {
        std::vector<RenderWork> cpu_queue;
        std::vector<RenderWork> gpu_queue;
        std::mutex mutex;
    };
    WorkQueue work_queue_;

    // Distribution strategy
    DistributionStrategy strategy_ = DistributionStrategy::Dynamic;

    // Performance monitoring
    std::atomic<float> cpu_utilization_{0.0f};
    std::atomic<float> gpu_utilization_{0.0f};

    struct PerformanceHistory {
        static constexpr size_t HISTORY_SIZE = 60;  // 60 frames
        std::array<float, HISTORY_SIZE> cpu_times{};
        std::array<float, HISTORY_SIZE> gpu_times{};
        size_t current_frame = 0;

        void add_sample(float cpu_time, float gpu_time);
        float get_avg_cpu_time() const;
        float get_avg_gpu_time() const;
    };
    PerformanceHistory perf_history_;

    // Scene complexity cache
    SceneComplexity last_complexity_;

    // Render state
    RenderMode render_mode_ = RenderMode::Normal;
    RenderMode hybrid_mode_ = RenderMode::HYBRID_AUTO;
    bool use_3d_renderer_ = true;
    bool adaptive_quality_enabled_ = false;
    float target_fps_ = 60.0f;
    HybridRenderStats last_stats_{};

    // Draw calls
    struct DrawCall {
        Mobject* mobject;
        VkPipeline pipeline;
        VkBuffer vertex_buffer;
        VkBuffer index_buffer;
        uint32_t index_count;
        math::Mat4 transform;
    };
    std::vector<DrawCall> draw_calls_;

    // Vulkan resources
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;

    // Helper functions
    void create_vulkan_resources();
    void destroy_vulkan_resources();

    void process_cpu_queue();
    void process_gpu_queue(VkCommandBuffer cmd);

    float estimate_cpu_time(const RenderWork& work) const;
    float estimate_gpu_time(const RenderWork& work) const;

    // Culling
    std::vector<Mobject*> frustum_cull(
        const std::vector<Mobject*>& objects,
        const Camera& camera
    );

    std::vector<Mobject*> occlusion_cull(
        const std::vector<Mobject*>& objects,
        const Camera& camera
    );
};

/**
 * @brief RAII work synchronization guard
 */
class WorkSyncGuard {
public:
    explicit WorkSyncGuard(HybridRenderer& renderer)
        : renderer_(renderer) {}

    ~WorkSyncGuard() {
        renderer_.sync_cpu_gpu_work();
    }

private:
    HybridRenderer& renderer_;
};

} // namespace manim

// Template implementation
namespace manim {

template<typename F>
void HybridRenderer::ThreadPool::enqueue(F&& f) {
    {
        std::unique_lock lock(queue_mutex_);
        tasks_.emplace(std::forward<F>(f));
    }
    active_tasks_++;
    condition_.notify_one();
}

} // namespace manim
