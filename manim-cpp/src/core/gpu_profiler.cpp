/**
 * @file gpu_profiler.cpp
 * @brief GPU Timeline Profiler implementation
 */

#include "manim/core/gpu_profiler.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace manim {

GPUProfiler::~GPUProfiler() {
    shutdown();
}

void GPUProfiler::initialize(VkDevice device, VkPhysicalDevice physical_device) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (query_pool_ != VK_NULL_HANDLE) {
        spdlog::warn("GPUProfiler already initialized");
        return;
    }

    device_ = device;

    // Get timestamp period from physical device
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);
    timestamp_period_ = props.limits.timestampPeriod;

    if (timestamp_period_ == 0.0f) {
        spdlog::warn("GPU does not support timestamp queries, profiling disabled");
        enabled_ = false;
        return;
    }

    spdlog::info("GPUProfiler: timestamp period = {} ns", timestamp_period_);

    // Create query pool for timestamp queries
    VkQueryPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    pool_info.queryCount = MAX_QUERIES_PER_FRAME * MAX_FRAMES_IN_FLIGHT;

    if (vkCreateQueryPool(device_, &pool_info, nullptr, &query_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create timestamp query pool");
        enabled_ = false;
        return;
    }

    // Initialize frame data
    for (auto& frame : frame_data_) {
        frame.query_start = 0;
        frame.query_count = 0;
        frame.scope_queries.clear();
        frame.in_use = false;
    }

    spdlog::info("GPUProfiler initialized with {} queries", MAX_QUERIES_PER_FRAME * MAX_FRAMES_IN_FLIGHT);
}

void GPUProfiler::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (query_pool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyQueryPool(device_, query_pool_, nullptr);
        query_pool_ = VK_NULL_HANDLE;
    }

    device_ = VK_NULL_HANDLE;
    stats_.clear();
    last_frame_results_.clear();
    active_scopes_.clear();

    for (auto& frame : frame_data_) {
        frame.scope_queries.clear();
        frame.in_use = false;
    }
}

void GPUProfiler::begin_frame(VkCommandBuffer cmd) {
    if (!is_enabled() || cmd == VK_NULL_HANDLE) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Move to next frame slot
    current_frame_slot_ = frame_number_ % MAX_FRAMES_IN_FLIGHT;
    auto& frame = frame_data_[current_frame_slot_];

    // If this slot was in use, collect its results first
    if (frame.in_use) {
        process_frame_queries(frame);
    }

    // Reset frame data
    frame.query_start = current_frame_slot_ * MAX_QUERIES_PER_FRAME;
    frame.query_count = 0;
    frame.scope_queries.clear();
    frame.in_use = true;
    current_query_index_ = frame.query_start;
    active_scopes_.clear();

    // Reset query pool range for this frame
    vkCmdResetQueryPool(cmd, query_pool_, frame.query_start, MAX_QUERIES_PER_FRAME);

    // Record frame start timestamp
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pool_, current_query_index_);
    frame.scope_queries.push_back({"__frame_start__", current_query_index_});
    current_query_index_++;
    frame.query_count++;
}

void GPUProfiler::end_frame(VkCommandBuffer cmd) {
    if (!is_enabled() || cmd == VK_NULL_HANDLE) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto& frame = frame_data_[current_frame_slot_];

    // Record frame end timestamp
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_, current_query_index_);
    frame.scope_queries.push_back({"__frame_end__", current_query_index_});
    current_query_index_++;
    frame.query_count++;

    frame_number_++;
}

void GPUProfiler::begin_scope(VkCommandBuffer cmd, const std::string& name) {
    if (!is_enabled() || cmd == VK_NULL_HANDLE) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto& frame = frame_data_[current_frame_slot_];

    if (frame.query_count >= MAX_QUERIES_PER_FRAME - 1) {
        spdlog::warn("GPUProfiler: query limit reached, scope '{}' not recorded", name);
        return;
    }

    // Record start timestamp
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pool_, current_query_index_);
    active_scopes_[name] = current_query_index_;
    frame.scope_queries.push_back({name + "_start", current_query_index_});
    current_query_index_++;
    frame.query_count++;
}

void GPUProfiler::end_scope(VkCommandBuffer cmd, const std::string& name) {
    if (!is_enabled() || cmd == VK_NULL_HANDLE) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = active_scopes_.find(name);
    if (it == active_scopes_.end()) {
        spdlog::warn("GPUProfiler: end_scope called without matching begin_scope: {}", name);
        return;
    }

    auto& frame = frame_data_[current_frame_slot_];

    if (frame.query_count >= MAX_QUERIES_PER_FRAME) {
        active_scopes_.erase(it);
        return;
    }

    // Record end timestamp
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_, current_query_index_);
    frame.scope_queries.push_back({name + "_end", current_query_index_});
    current_query_index_++;
    frame.query_count++;

    active_scopes_.erase(it);
}

uint32_t GPUProfiler::allocate_query_pair() {
    auto& frame = frame_data_[current_frame_slot_];
    if (frame.query_count + 2 > MAX_QUERIES_PER_FRAME) {
        return UINT32_MAX;
    }
    uint32_t start = current_query_index_;
    current_query_index_ += 2;
    frame.query_count += 2;
    return start;
}

void GPUProfiler::collect_results() {
    if (!is_enabled()) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Process the oldest frame slot that might have results ready
    // We process frames that are at least 2 frames old to ensure GPU completion
    if (frame_number_ < MAX_FRAMES_IN_FLIGHT) return;

    uint32_t slot_to_process = (frame_number_ - MAX_FRAMES_IN_FLIGHT + 1) % MAX_FRAMES_IN_FLIGHT;
    auto& frame = frame_data_[slot_to_process];

    if (frame.in_use && frame.query_count > 0) {
        process_frame_queries(frame);
        frame.in_use = false;
    }
}

void GPUProfiler::process_frame_queries(const FrameData& frame) {
    if (frame.query_count == 0) return;

    // Read all timestamps for this frame
    std::vector<uint64_t> timestamps(frame.query_count);
    VkResult result = vkGetQueryPoolResults(
        device_, query_pool_,
        frame.query_start, frame.query_count,
        timestamps.size() * sizeof(uint64_t),
        timestamps.data(),
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
    );

    if (result != VK_SUCCESS) {
        spdlog::debug("GPUProfiler: query results not ready");
        return;
    }

    // Clear previous results
    last_frame_results_.clear();
    last_frame_gpu_time_ms_ = 0.0;

    // Build a map of scope name -> (start_query_local_index, end_query_local_index)
    std::unordered_map<std::string, std::pair<int, int>> scope_indices;

    for (size_t i = 0; i < frame.scope_queries.size(); ++i) {
        const auto& [name, query_idx] = frame.scope_queries[i];
        uint32_t local_idx = query_idx - frame.query_start;

        if (name.ends_with("_start")) {
            std::string base_name = name.substr(0, name.length() - 6);
            scope_indices[base_name].first = local_idx;
        } else if (name.ends_with("_end")) {
            std::string base_name = name.substr(0, name.length() - 4);
            scope_indices[base_name].second = local_idx;
        } else if (name == "__frame_start__") {
            scope_indices["__frame__"].first = local_idx;
        } else if (name == "__frame_end__") {
            scope_indices["__frame__"].second = local_idx;
        }
    }

    // Calculate timings
    for (const auto& [name, indices] : scope_indices) {
        if (indices.first >= 0 && indices.second >= 0 &&
            static_cast<size_t>(indices.first) < timestamps.size() &&
            static_cast<size_t>(indices.second) < timestamps.size()) {

            uint64_t start_ts = timestamps[indices.first];
            uint64_t end_ts = timestamps[indices.second];

            if (end_ts >= start_ts) {
                double duration_ns = (end_ts - start_ts) * timestamp_period_;
                double duration_ms = duration_ns / 1000000.0;

                ProfileResult result;
                result.name = name;
                result.gpu_time_ms = duration_ms;
                result.cpu_time_ms = 0.0;  // Not tracked
                result.start_timestamp = start_ts;
                result.end_timestamp = end_ts;
                result.frame_number = frame_number_ - MAX_FRAMES_IN_FLIGHT;

                last_frame_results_.push_back(result);

                // Update statistics
                if (name != "__frame__") {
                    stats_[name].name = name;
                    stats_[name].add_sample(duration_ms);
                } else {
                    last_frame_gpu_time_ms_ = duration_ms;
                }
            }
        }
    }
}

void GPUProfiler::reset_stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.clear();
    last_frame_results_.clear();
    last_frame_gpu_time_ms_ = 0.0;
}

void GPUProfiler::print_report() const {
    if (stats_.empty()) {
        spdlog::info("GPUProfiler: No profiling data collected");
        return;
    }

    std::stringstream ss;
    ss << "\n========== GPU Profiler Report ==========\n";
    ss << std::fixed << std::setprecision(3);

    // Sort by average time (descending)
    std::vector<std::pair<std::string, ProfileStats>> sorted_stats(stats_.begin(), stats_.end());
    std::sort(sorted_stats.begin(), sorted_stats.end(),
              [](const auto& a, const auto& b) { return a.second.avg_gpu_time_ms > b.second.avg_gpu_time_ms; });

    ss << std::setw(30) << "Scope"
       << std::setw(12) << "Avg (ms)"
       << std::setw(12) << "Min (ms)"
       << std::setw(12) << "Max (ms)"
       << std::setw(10) << "Samples" << "\n";
    ss << std::string(76, '-') << "\n";

    for (const auto& [name, stat] : sorted_stats) {
        ss << std::setw(30) << name
           << std::setw(12) << stat.avg_gpu_time_ms
           << std::setw(12) << stat.min_gpu_time_ms
           << std::setw(12) << stat.max_gpu_time_ms
           << std::setw(10) << stat.sample_count << "\n";
    }

    ss << "==========================================\n";
    ss << "Last frame GPU time: " << last_frame_gpu_time_ms_ << " ms\n";

    spdlog::info("{}", ss.str());
}

// ========================================================================
// Global GPUProfiler Accessor
// ========================================================================

static std::unique_ptr<GPUProfiler> g_gpu_profiler;
static std::mutex g_profiler_mutex;

GPUProfiler& getGlobalGPUProfiler() {
    std::lock_guard<std::mutex> lock(g_profiler_mutex);
    if (!g_gpu_profiler) {
        g_gpu_profiler = std::make_unique<GPUProfiler>();
    }
    return *g_gpu_profiler;
}

void initializeGlobalGPUProfiler(VkDevice device, VkPhysicalDevice physical_device) {
    std::lock_guard<std::mutex> lock(g_profiler_mutex);
    if (!g_gpu_profiler) {
        g_gpu_profiler = std::make_unique<GPUProfiler>();
    }
    g_gpu_profiler->initialize(device, physical_device);
    spdlog::info("Global GPUProfiler initialized");
}

void shutdownGlobalGPUProfiler() {
    std::lock_guard<std::mutex> lock(g_profiler_mutex);
    if (g_gpu_profiler) {
        g_gpu_profiler->shutdown();
        g_gpu_profiler.reset();
        spdlog::info("Global GPUProfiler shut down");
    }
}

bool isGPUProfilerInitialized() {
    std::lock_guard<std::mutex> lock(g_profiler_mutex);
    return g_gpu_profiler != nullptr && g_gpu_profiler->is_enabled();
}

} // namespace manim
