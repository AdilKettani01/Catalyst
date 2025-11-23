// Hybrid CPU-GPU renderer implementation
#include "manim/renderer/hybrid_renderer.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>

namespace manim {

// ============================================================================
// HybridRenderer Implementation
// ============================================================================

HybridRenderer::HybridRenderer(
    std::unique_ptr<Renderer> gpu_renderer,
    std::shared_ptr<MemoryPool> memory_pool,
    std::shared_ptr<ComputeEngine> compute_engine
) : gpu_renderer_(std::move(gpu_renderer)),
    memory_pool_(memory_pool),
    compute_engine_(compute_engine),
    cpu_thread_pool_(std::thread::hardware_concurrency()) {

    spdlog::info("Initializing hybrid CPU-GPU renderer");
    spdlog::info("  CPU threads: {}", std::thread::hardware_concurrency());
}

HybridRenderer::~HybridRenderer() {
    shutdown();
}

void HybridRenderer::initialize(const RendererConfig& config) {
    gpu_renderer_->initialize(config);

    // Initialize distribution strategy
    current_strategy_ = DistributionStrategy::Adaptive;

    // Initialize performance counters
    cpu_utilization_ = 0.0f;
    gpu_utilization_ = 0.0f;

    spdlog::info("Hybrid renderer initialized");
}

void HybridRenderer::shutdown() {
    // Wait for all CPU tasks to complete
    for (auto& future : cpu_task_futures_) {
        if (future.valid()) {
            future.wait();
        }
    }

    gpu_renderer_->shutdown();
    spdlog::info("Hybrid renderer shut down");
}

void HybridRenderer::render_scene(Scene& scene, Camera& camera) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // 1. Analyze scene complexity
    SceneComplexity complexity = analyze_scene_complexity(scene);

    // 2. Distribute render tasks
    distribute_render_tasks(scene);

    // 3. Execute CPU tasks in parallel
    execute_cpu_tasks();

    // 4. Execute GPU tasks
    // This would submit Vulkan command buffers
    // execute_gpu_tasks();

    // 5. Synchronize CPU and GPU
    synchronize();

    // 6. Render final frame using GPU renderer
    gpu_renderer_->render_scene(scene, camera);

    // 7. Adjust distribution based on performance
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    adjust_distribution(complexity, duration.count());
}

void HybridRenderer::render_mobject(Mobject& mobject) {
    // Determine if this mobject should be rendered on CPU or GPU
    RenderWork work;
    work.type = RenderWork::Type::DrawCall;
    work.data_size = mobject.get_num_points();
    work.complexity = estimate_mobject_complexity(mobject);

    if (should_use_gpu(work)) {
        gpu_renderer_->render_mobject(mobject);
    } else {
        // Render on CPU (rasterize to texture, upload to GPU)
        render_mobject_cpu(mobject);
    }
}

SceneComplexity HybridRenderer::analyze_scene_complexity(Scene& scene) {
    SceneComplexity complexity;

    // Count mobjects
    complexity.num_mobjects = scene.get_mobject_count();

    // Count total vertices
    complexity.total_vertices = 0;
    complexity.total_triangles = 0;

    for (auto& mobject : scene.get_mobjects()) {
        complexity.total_vertices += mobject->get_num_points();
        // Estimate triangles (simplified)
        complexity.total_triangles += mobject->get_num_points() / 3;
    }

    // Count lights
    complexity.num_lights = scene.get_light_count();

    // Estimate complexity score (0-1)
    float vertex_score = std::min(complexity.total_vertices / 1000000.0f, 1.0f);
    float mobject_score = std::min(complexity.num_mobjects / 1000.0f, 1.0f);
    float light_score = std::min(complexity.num_lights / 16.0f, 1.0f);

    complexity.complexity_score = (vertex_score + mobject_score + light_score) / 3.0f;

    spdlog::debug("Scene complexity: {:.2f} ({} mobjects, {} vertices)",
                  complexity.complexity_score, complexity.num_mobjects, complexity.total_vertices);

    return complexity;
}

void HybridRenderer::distribute_render_tasks(Scene& scene) {
    cpu_work_queue_.clear();
    gpu_work_queue_.clear();

    // Scene update logic (always CPU)
    RenderWork scene_update;
    scene_update.type = RenderWork::Type::SceneUpdate;
    scene_update.complexity = 0.1f;
    scene_update.cpu_function = [&scene, this]() {
        update_scene_logic(scene, 0.016f);  // Assume 60 FPS
    };
    cpu_work_queue_.push_back(scene_update);

    // Frustum culling (CPU)
    RenderWork culling;
    culling.type = RenderWork::Type::Culling;
    culling.complexity = 0.2f;
    culling.cpu_function = [&scene, this]() {
        // cull_objects(scene, camera);
    };
    cpu_work_queue_.push_back(culling);

    // Animation updates (CPU or GPU depending on size)
    for (auto& mobject : scene.get_mobjects()) {
        size_t num_points = mobject->get_num_points();

        RenderWork anim_work;
        anim_work.type = RenderWork::Type::Animation;
        anim_work.data_size = num_points;
        anim_work.complexity = num_points / 10000.0f;

        if (should_use_gpu(anim_work)) {
            // GPU animation update
            anim_work.gpu_function = [mobject, this](VkCommandBuffer cmd) {
                // transform_points_gpu(cmd, {mobject.get()});
            };
            gpu_work_queue_.push_back(anim_work);
        } else {
            // CPU animation update
            anim_work.cpu_function = [mobject]() {
                // Update animation on CPU
            };
            cpu_work_queue_.push_back(anim_work);
        }
    }

    // Physics simulation (GPU if large)
    if (scene.has_physics()) {
        RenderWork physics;
        physics.type = RenderWork::Type::Physics;
        physics.data_size = scene.get_physics_particle_count();
        physics.complexity = 0.8f;

        if (physics.data_size > 10000) {
            // GPU physics
            physics.gpu_function = [&scene, this](VkCommandBuffer cmd) {
                // compute_physics_gpu(cmd, 0.016f);
            };
            gpu_work_queue_.push_back(physics);
        } else {
            // CPU physics
            physics.cpu_function = [&scene]() {
                // scene.update_physics_cpu(0.016f);
            };
            cpu_work_queue_.push_back(physics);
        }
    }

    spdlog::debug("Task distribution: {} CPU tasks, {} GPU tasks",
                  cpu_work_queue_.size(), gpu_work_queue_.size());
}

bool HybridRenderer::should_use_gpu(const RenderWork& work) const {
    // Decision based on strategy
    switch (current_strategy_) {
        case DistributionStrategy::GPUOnly:
            return true;

        case DistributionStrategy::CPUOnly:
            return false;

        case DistributionStrategy::BalancedStatic:
            // Simple threshold
            return work.data_size > 1000;

        case DistributionStrategy::Adaptive:
            // Dynamic decision based on current load
            if (work.data_size < 100) {
                return false;  // Too small for GPU
            }
            if (work.data_size > 100000) {
                return true;  // Large enough for GPU
            }

            // Check current GPU utilization
            if (gpu_utilization_ > 0.9f) {
                return false;  // GPU saturated, use CPU
            }
            if (cpu_utilization_ > 0.9f) {
                return true;  // CPU saturated, use GPU
            }

            // Default to GPU for medium-sized work
            return work.data_size > 1000;

        default:
            return true;
    }
}

void HybridRenderer::execute_cpu_tasks() {
    cpu_task_futures_.clear();

    for (auto& work : cpu_work_queue_) {
        if (work.cpu_function) {
            auto future = cpu_thread_pool_.enqueue([work]() {
                work.cpu_function();
            });
            cpu_task_futures_.push_back(std::move(future));
        }
    }
}

void HybridRenderer::synchronize() {
    // Wait for CPU tasks
    for (auto& future : cpu_task_futures_) {
        if (future.valid()) {
            future.wait();
        }
    }

    // GPU synchronization would use Vulkan fences/semaphores
}

void HybridRenderer::adjust_distribution(const SceneComplexity& complexity, float frame_time_ms) {
    if (current_strategy_ != DistributionStrategy::Adaptive) {
        return;
    }

    // Update utilization estimates (exponential moving average)
    const float alpha = 0.1f;

    // Estimate utilization based on frame time
    float target_frame_time = 16.67f;  // 60 FPS
    float gpu_ratio = static_cast<float>(gpu_work_queue_.size()) /
                      static_cast<float>(cpu_work_queue_.size() + gpu_work_queue_.size());
    float cpu_ratio = 1.0f - gpu_ratio;

    gpu_utilization_ = gpu_utilization_ * (1.0f - alpha) + (frame_time_ms / target_frame_time) * gpu_ratio * alpha;
    cpu_utilization_ = cpu_utilization_ * (1.0f - alpha) + (frame_time_ms / target_frame_time) * cpu_ratio * alpha;

    // Clamp
    gpu_utilization_ = std::clamp(gpu_utilization_, 0.0f, 1.0f);
    cpu_utilization_ = std::clamp(cpu_utilization_, 0.0f, 1.0f);

    // Detect bottlenecks
    if (gpu_utilization_ > 0.95f && cpu_utilization_ < 0.5f) {
        spdlog::debug("GPU bottleneck detected, shifting work to CPU");
        gpu_threshold_ *= 1.1f;  // Increase threshold to send less to GPU
    } else if (cpu_utilization_ > 0.95f && gpu_utilization_ < 0.5f) {
        spdlog::debug("CPU bottleneck detected, shifting work to GPU");
        gpu_threshold_ *= 0.9f;  // Decrease threshold to send more to GPU
    }

    spdlog::debug("Utilization: CPU={:.2f}, GPU={:.2f}, Threshold={:.0f}",
                  cpu_utilization_, gpu_utilization_, gpu_threshold_);
}

float HybridRenderer::estimate_mobject_complexity(Mobject& mobject) const {
    // Estimate rendering complexity (0-1)
    size_t num_points = mobject.get_num_points();

    // Complexity factors
    float vertex_complexity = std::min(num_points / 10000.0f, 1.0f);
    float shader_complexity = mobject.has_custom_shader() ? 0.5f : 0.1f;
    float transparency_complexity = mobject.has_transparency() ? 0.3f : 0.0f;

    return (vertex_complexity + shader_complexity + transparency_complexity) / 2.3f;
}

void HybridRenderer::update_scene_logic(Scene& scene, float dt) {
    // Update scene logic on CPU
    // - Process user input
    // - Update animations
    // - Update scene graph

    spdlog::debug("Updating scene logic (CPU)");
}

void HybridRenderer::render_mobject_cpu(Mobject& mobject) {
    // CPU rasterization (fallback)
    // Could use Skia or Cairo for 2D rendering

    spdlog::debug("Rendering mobject on CPU");
}

void HybridRenderer::set_distribution_strategy(DistributionStrategy strategy) {
    current_strategy_ = strategy;
    spdlog::info("Distribution strategy set to: {}", static_cast<int>(strategy));
}

DistributionStrategy HybridRenderer::get_distribution_strategy() const {
    return current_strategy_;
}

float HybridRenderer::get_cpu_utilization() const {
    return cpu_utilization_;
}

float HybridRenderer::get_gpu_utilization() const {
    return gpu_utilization_;
}

RendererType HybridRenderer::get_type() const {
    return gpu_renderer_->get_type();
}

}  // namespace manim
