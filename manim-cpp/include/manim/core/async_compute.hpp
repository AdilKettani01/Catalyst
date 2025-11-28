/**
 * @file async_compute.hpp
 * @brief Async compute support for parallel GPU workloads
 *
 * Enables overlapping compute and graphics work on GPUs that support
 * separate compute queue families.
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>

namespace manim {

class GPUBuffer;
class MemoryPool;

/**
 * @brief Async compute task handle
 */
struct AsyncTaskHandle {
    uint64_t id = 0;
    bool valid() const { return id != 0; }
};

/**
 * @brief Completion callback for async tasks
 */
using AsyncCompletionCallback = std::function<void(AsyncTaskHandle)>;

/**
 * @brief Configuration for async compute
 */
struct AsyncComputeConfig {
    bool enable_async = true;
    uint32_t max_pending_tasks = 16;
    bool prefer_dedicated_compute_queue = true;
};

/**
 * @brief Async Compute Manager
 *
 * Manages non-blocking compute dispatches on a separate compute queue,
 * enabling overlap with graphics work.
 *
 * Features:
 * - Separate compute queue (when available)
 * - Timeline semaphore synchronization
 * - Non-blocking dispatch with fence tracking
 * - Task completion callbacks
 */
class AsyncComputeManager {
public:
    AsyncComputeManager() = default;
    ~AsyncComputeManager();

    /**
     * @brief Initialize with Vulkan resources
     * @param device Logical device
     * @param physical_device Physical device
     * @param graphics_queue_family Graphics queue family index
     * @param memory_pool Memory pool for temp allocations
     * @param config Configuration options
     * @return true if async compute is available
     */
    bool initialize(VkDevice device, VkPhysicalDevice physical_device,
                    uint32_t graphics_queue_family, MemoryPool* memory_pool,
                    const AsyncComputeConfig& config = {});

    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();

    /**
     * @brief Check if async compute is available and enabled
     */
    bool is_available() const { return enabled_ && compute_queue_ != VK_NULL_HANDLE; }

    /**
     * @brief Check if using a dedicated compute queue (not shared with graphics)
     */
    bool is_dedicated_queue() const { return is_dedicated_compute_queue_; }

    /**
     * @brief Get the compute queue family index
     */
    uint32_t get_compute_queue_family() const { return compute_queue_family_; }

    /**
     * @brief Submit async compute work
     *
     * Records commands into a command buffer and submits to compute queue.
     * Returns immediately without waiting for completion.
     *
     * @param record_func Function that records compute commands
     * @param callback Optional callback when task completes
     * @return Task handle for tracking
     */
    AsyncTaskHandle submit_async(
        std::function<void(VkCommandBuffer)> record_func,
        AsyncCompletionCallback callback = nullptr
    );

    /**
     * @brief Wait for a specific task to complete
     * @param handle Task handle
     * @param timeout_ns Timeout in nanoseconds (UINT64_MAX for infinite)
     * @return true if completed, false if timeout
     */
    bool wait_for_task(AsyncTaskHandle handle, uint64_t timeout_ns = UINT64_MAX);

    /**
     * @brief Wait for all pending tasks to complete
     */
    void wait_idle();

    /**
     * @brief Poll for completed tasks and invoke callbacks
     * Call this periodically (e.g., once per frame) to process completions
     */
    void poll_completions();

    /**
     * @brief Get number of pending tasks
     */
    uint32_t get_pending_count() const { return pending_count_.load(); }

    /**
     * @brief Get semaphore for graphics-compute synchronization
     *
     * Graphics work that depends on async compute results should wait
     * on this semaphore before accessing the data.
     */
    VkSemaphore get_compute_complete_semaphore() const { return compute_complete_semaphore_; }

    /**
     * @brief Signal value for the most recent async submission
     */
    uint64_t get_latest_signal_value() const { return timeline_value_.load(); }

    /**
     * @brief Create a buffer memory barrier for compute->graphics transition
     */
    static VkBufferMemoryBarrier create_compute_to_graphics_barrier(
        VkBuffer buffer, VkDeviceSize size,
        uint32_t compute_queue_family, uint32_t graphics_queue_family
    );

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    uint32_t compute_queue_family_ = 0;
    uint32_t graphics_queue_family_ = 0;

    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkSemaphore compute_complete_semaphore_ = VK_NULL_HANDLE;  // Timeline semaphore

    bool enabled_ = false;
    bool is_dedicated_compute_queue_ = false;

    std::atomic<uint64_t> timeline_value_{0};
    std::atomic<uint64_t> next_task_id_{1};
    std::atomic<uint32_t> pending_count_{0};

    MemoryPool* memory_pool_ = nullptr;

    // Command buffer pool
    struct CommandBufferEntry {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        uint64_t task_id = 0;
        uint64_t signal_value = 0;
        AsyncCompletionCallback callback;
        bool in_use = false;
    };
    std::vector<CommandBufferEntry> command_buffers_;
    std::mutex cmd_mutex_;

    // Find dedicated compute queue family
    int32_t find_compute_queue_family(VkPhysicalDevice physical_device, uint32_t graphics_family);

    // Get available command buffer
    CommandBufferEntry* acquire_command_buffer();
    void release_command_buffer(CommandBufferEntry* entry);
};

// Global async compute manager accessor
AsyncComputeManager& getGlobalAsyncCompute();
void initializeGlobalAsyncCompute(VkDevice device, VkPhysicalDevice physical_device,
                                   uint32_t graphics_queue_family, MemoryPool* pool);
void shutdownGlobalAsyncCompute();
bool isAsyncComputeAvailable();

} // namespace manim
