// GPU Animation Engine implementation
#include "manim/animation/gpu_animation_engine.hpp"
#include "manim/animation/transform.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace manim {

// ============================================================================
// GPUAnimationEngine Implementation
// ============================================================================

GPUAnimationEngine::GPUAnimationEngine(
    std::shared_ptr<ComputeEngine> compute_engine,
    std::shared_ptr<MemoryPool> memory_pool
) : compute_engine_(compute_engine),
    memory_pool_(memory_pool) {

    spdlog::info("GPU Animation Engine initialized");
}

// ============================================================================
// Animation Processing
// ============================================================================

void GPUAnimationEngine::process_animations_gpu(
    const std::vector<std::shared_ptr<Animation>>& animations,
    float t
) {
    if (animations.empty()) {
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Group animations by type
    auto groups = group_animations_by_type(animations);

    // Process each group
    for (const auto& [type, batch] : groups) {
        execute_animation_batch(type, batch, t);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats_.gpu_time_ms = duration.count() / 1000.0f;
    stats_.animations_processed += animations.size();

    spdlog::debug("Processed {} animations on GPU in {:.2f}ms",
                  animations.size(), stats_.gpu_time_ms);
}

void GPUAnimationEngine::batch_process(
    const std::vector<std::shared_ptr<Animation>>& animations,
    float t
) {
    // Group similar animations
    auto groups = group_animations_by_type(animations);

    stats_.batch_count = groups.size();

    for (const auto& [type, batch] : groups) {
        spdlog::debug("Processing batch of {} animations (type: {})",
                      batch.size(), static_cast<int>(type));

        execute_animation_batch(type, batch, t);
    }
}

// ============================================================================
// GPU Interpolation
// ============================================================================

void GPUAnimationEngine::interpolate_transforms_gpu(
    const GPUBuffer& start_transforms,
    const GPUBuffer& end_transforms,
    float alpha,
    GPUBuffer& output
) {
    // Dispatch compute shader for transform interpolation
    // This would use a compute shader like:
    // output = mix(start_transforms, end_transforms, alpha)

    spdlog::trace("Interpolating transforms on GPU: alpha={:.3f}", alpha);

    // In real implementation:
    // compute_engine_->dispatch_transform_interpolation(
    //     start_transforms, end_transforms, alpha, output
    // );
}

void GPUAnimationEngine::interpolate_colors_gpu(
    const GPUBuffer& start_colors,
    const GPUBuffer& end_colors,
    float alpha,
    GPUBuffer& output
) {
    // GPU color interpolation (linear or HSV)
    spdlog::trace("Interpolating colors on GPU: alpha={:.3f}", alpha);

    // compute_engine_->dispatch_color_interpolation(
    //     start_colors, end_colors, alpha, output
    // );
}

void GPUAnimationEngine::interpolate_points_gpu(
    const GPUBuffer& start_points,
    const GPUBuffer& end_points,
    float alpha,
    GPUBuffer& output
) {
    // GPU point interpolation for morphing
    spdlog::trace("Interpolating points on GPU: alpha={:.3f}", alpha);

    // compute_engine_->dispatch_point_interpolation(
    //     start_points, end_points, alpha, output
    // );
}

void GPUAnimationEngine::morph_shapes_gpu(
    const GPUBuffer& start_curves,
    const GPUBuffer& end_curves,
    float alpha,
    GPUBuffer& output
) {
    // GPU Bezier curve morphing
    spdlog::debug("Morphing shapes on GPU: alpha={:.3f}", alpha);

    // This would dispatch a compute shader that:
    // 1. Interpolates control points
    // 2. Re-tessellates curves
    // 3. Outputs morphed geometry

    // compute_engine_->dispatch_curve_morph(
    //     start_curves, end_curves, alpha, output
    // );
}

// ============================================================================
// GPU Rate Functions
// ============================================================================

void GPUAnimationEngine::apply_rate_function_gpu(
    const GPUBuffer& alphas,
    RateFunc rate_func,
    GPUBuffer& output
) {
    // Apply rate function to array of alpha values on GPU

    // Check if we have a precomputed LUT
    size_t func_hash = std::hash<void*>{}(reinterpret_cast<void*>(&rate_func));

    if (rate_func_luts_.find(func_hash) != rate_func_luts_.end()) {
        // Use LUT for fast evaluation
        spdlog::trace("Using LUT for rate function");
        // Sample from LUT
    } else {
        // Compute on GPU directly
        spdlog::trace("Computing rate function on GPU");
    }
}

void GPUAnimationEngine::precompute_rate_function_lut(
    RateFunc rate_func,
    uint32_t samples
) {
    // Precompute rate function lookup table

    std::vector<float> lut_data(samples);
    for (uint32_t i = 0; i < samples; ++i) {
        float t = static_cast<float>(i) / (samples - 1);
        lut_data[i] = rate_func(t);
    }

    // Upload to GPU buffer
    VkDeviceSize buffer_size = sizeof(float) * samples;
    GPUBuffer lut = memory_pool_->allocate_buffer(
        buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Static
    );

    // lut.upload(lut_data.data(), buffer_size);

    size_t func_hash = std::hash<void*>{}(reinterpret_cast<void*>(&rate_func));
    rate_func_luts_[func_hash] = std::move(lut);

    spdlog::debug("Precomputed rate function LUT with {} samples", samples);
}

// ============================================================================
// Animation Batching
// ============================================================================

GPUAnimationEngine::AnimationType
GPUAnimationEngine::classify_animation(const Animation& anim) const {
    // Classify animation by type for batching
    // This would use RTTI or virtual methods

    // Simplified classification:
    if (dynamic_cast<const Transform*>(&anim)) {
        return AnimationType::Transform;
    }
    // Add more classifications...

    return AnimationType::Custom;
}

std::unordered_map<GPUAnimationEngine::AnimationType, std::vector<std::shared_ptr<Animation>>>
GPUAnimationEngine::group_animations_by_type(
    const std::vector<std::shared_ptr<Animation>>& animations
) const {
    std::unordered_map<AnimationType, std::vector<std::shared_ptr<Animation>>> groups;

    for (const auto& anim : animations) {
        AnimationType type = classify_animation(*anim);
        groups[type].push_back(anim);
    }

    return groups;
}

void GPUAnimationEngine::execute_animation_batch(
    AnimationType type,
    const std::vector<std::shared_ptr<Animation>>& batch,
    float t
) {
    if (batch.empty()) {
        return;
    }

    spdlog::debug("Executing batch of {} animations (type: {})",
                  batch.size(), static_cast<int>(type));

    switch (type) {
        case AnimationType::Transform:
            // Batch transform interpolation
            for (const auto& anim : batch) {
                anim->interpolate(t);
            }
            break;

        case AnimationType::Fade:
            // Batch fade (opacity changes)
            for (const auto& anim : batch) {
                anim->interpolate(t);
            }
            break;

        case AnimationType::ColorChange:
            // Batch color interpolation
            for (const auto& anim : batch) {
                anim->interpolate(t);
            }
            break;

        default:
            // Process individually
            for (const auto& anim : batch) {
                anim->interpolate(t);
            }
            break;
    }
}

// ============================================================================
// Caching
// ============================================================================

void GPUAnimationEngine::enable_caching(const Animation& anim, uint32_t num_samples) {
    size_t anim_hash = std::hash<const Animation*>{}(&anim);

    AnimationCache cache;
    cache.num_samples = num_samples;
    cache.sample_times.resize(num_samples);

    for (uint32_t i = 0; i < num_samples; ++i) {
        cache.sample_times[i] = static_cast<float>(i) / (num_samples - 1);
    }

    // Allocate GPU buffer for cached states
    // cache.cached_states = memory_pool_->allocate_buffer(...);

    cache.valid = true;
    cache_[anim_hash] = std::move(cache);

    spdlog::debug("Enabled caching for animation with {} samples", num_samples);
}

bool GPUAnimationEngine::get_cached_state(
    const Animation& anim,
    float t,
    [[maybe_unused]] GPUBuffer& output
) {
    size_t anim_hash = std::hash<const Animation*>{}(&anim);

    auto it = cache_.find(anim_hash);
    if (it == cache_.end() || !it->second.valid) {
        stats_.cache_misses++;
        return false;
    }

    const AnimationCache& cache = it->second;

    // Find nearest cached state
    float normalized_t = t / anim.get_run_time();
    size_t sample_idx = static_cast<size_t>(normalized_t * (cache.num_samples - 1));
    sample_idx = std::min(sample_idx, static_cast<size_t>(cache.num_samples - 1));

    // Copy from cache to output
    // This would copy the cached state

    stats_.cache_hits++;
    return true;
}

void GPUAnimationEngine::clear_cache() {
    cache_.clear();
    spdlog::info("Animation cache cleared");
}

}  // namespace manim
