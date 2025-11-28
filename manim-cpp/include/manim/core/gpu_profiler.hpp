/**
 * @file gpu_profiler.hpp
 * @brief GPU Timeline Profiler with Vulkan timestamp queries
 *
 * Provides fine-grained GPU timing measurement for profiling
 * render and compute operations.
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <mutex>

namespace manim {

/**
 * @brief Individual timing result for a profiled scope
 */
struct ProfileResult {
    std::string name;
    double gpu_time_ms;      // GPU execution time in milliseconds
    double cpu_time_ms;      // CPU time (including GPU wait) in milliseconds
    uint64_t start_timestamp;
    uint64_t end_timestamp;
    uint32_t frame_number;
};

/**
 * @brief Accumulated statistics for a profiled scope
 */
struct ProfileStats {
    std::string name;
    double total_gpu_time_ms = 0.0;
    double min_gpu_time_ms = std::numeric_limits<double>::max();
    double max_gpu_time_ms = 0.0;
    double avg_gpu_time_ms = 0.0;
    uint32_t sample_count = 0;

    void add_sample(double gpu_time_ms) {
        total_gpu_time_ms += gpu_time_ms;
        min_gpu_time_ms = std::min(min_gpu_time_ms, gpu_time_ms);
        max_gpu_time_ms = std::max(max_gpu_time_ms, gpu_time_ms);
        sample_count++;
        avg_gpu_time_ms = total_gpu_time_ms / sample_count;
    }
};

/**
 * @brief GPU Timeline Profiler using Vulkan timestamp queries
 *
 * Usage:
 *   profiler.begin_frame(cmd);
 *   profiler.begin_scope(cmd, "RenderPass");
 *   // ... rendering commands ...
 *   profiler.end_scope(cmd, "RenderPass");
 *   profiler.end_frame(cmd);
 *   profiler.collect_results();
 */
class GPUProfiler {
public:
    static constexpr uint32_t MAX_QUERIES_PER_FRAME = 128;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    GPUProfiler() = default;
    ~GPUProfiler();

    /**
     * @brief Initialize the profiler with Vulkan device
     * @param device Vulkan logical device
     * @param physical_device Physical device for timestamp period
     */
    void initialize(VkDevice device, VkPhysicalDevice physical_device);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Check if profiler is initialized and ready
     */
    bool is_enabled() const { return enabled_ && query_pool_ != VK_NULL_HANDLE; }

    /**
     * @brief Enable or disable profiling
     */
    void set_enabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Begin profiling for a new frame
     * @param cmd Command buffer to record reset command
     */
    void begin_frame(VkCommandBuffer cmd);

    /**
     * @brief End profiling for current frame
     * @param cmd Command buffer (unused, for API consistency)
     */
    void end_frame(VkCommandBuffer cmd);

    /**
     * @brief Begin a profiled scope
     * @param cmd Command buffer to record timestamp
     * @param name Scope name for identification
     */
    void begin_scope(VkCommandBuffer cmd, const std::string& name);

    /**
     * @brief End a profiled scope
     * @param cmd Command buffer to record timestamp
     * @param name Scope name (must match begin_scope)
     */
    void end_scope(VkCommandBuffer cmd, const std::string& name);

    /**
     * @brief Collect results from previous frames (call after queue submission)
     */
    void collect_results();

    /**
     * @brief Get accumulated statistics for all scopes
     */
    const std::unordered_map<std::string, ProfileStats>& get_stats() const { return stats_; }

    /**
     * @brief Get results from the most recent completed frame
     */
    const std::vector<ProfileResult>& get_last_frame_results() const { return last_frame_results_; }

    /**
     * @brief Reset accumulated statistics
     */
    void reset_stats();

    /**
     * @brief Get total GPU time for last frame in milliseconds
     */
    double get_last_frame_gpu_time_ms() const { return last_frame_gpu_time_ms_; }

    /**
     * @brief Print profiling report to log
     */
    void print_report() const;

    /**
     * @brief Get current frame number
     */
    uint32_t get_frame_number() const { return frame_number_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueryPool query_pool_ = VK_NULL_HANDLE;
    float timestamp_period_ = 1.0f;  // nanoseconds per timestamp tick

    bool enabled_ = true;
    uint32_t frame_number_ = 0;
    uint32_t current_query_index_ = 0;

    // Per-frame tracking
    struct FrameData {
        uint32_t query_start = 0;
        uint32_t query_count = 0;
        std::vector<std::pair<std::string, uint32_t>> scope_queries;  // name -> start query index
        bool in_use = false;
    };
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frame_data_;
    uint32_t current_frame_slot_ = 0;

    // Active scopes stack (for nested profiling)
    std::unordered_map<std::string, uint32_t> active_scopes_;  // name -> start query index

    // Results
    std::vector<ProfileResult> last_frame_results_;
    std::unordered_map<std::string, ProfileStats> stats_;
    double last_frame_gpu_time_ms_ = 0.0;

    std::mutex mutex_;

    uint32_t allocate_query_pair();
    void process_frame_queries(const FrameData& frame);
};

/**
 * @brief RAII scope profiler for automatic begin/end
 */
class GPUProfileScope {
public:
    GPUProfileScope(GPUProfiler& profiler, VkCommandBuffer cmd, const std::string& name)
        : profiler_(profiler), cmd_(cmd), name_(name) {
        profiler_.begin_scope(cmd_, name_);
    }

    ~GPUProfileScope() {
        profiler_.end_scope(cmd_, name_);
    }

    // Non-copyable
    GPUProfileScope(const GPUProfileScope&) = delete;
    GPUProfileScope& operator=(const GPUProfileScope&) = delete;

private:
    GPUProfiler& profiler_;
    VkCommandBuffer cmd_;
    std::string name_;
};

// Convenience macro for profiling scopes
#define GPU_PROFILE_SCOPE(profiler, cmd, name) \
    GPUProfileScope _gpu_profile_scope_##__LINE__(profiler, cmd, name)

// Global profiler accessor
GPUProfiler& getGlobalGPUProfiler();
void initializeGlobalGPUProfiler(VkDevice device, VkPhysicalDevice physical_device);
void shutdownGlobalGPUProfiler();
bool isGPUProfilerInitialized();

} // namespace manim
