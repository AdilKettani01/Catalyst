/**
 * @file async_compute.cpp
 * @brief Async compute manager implementation
 */

#include "manim/core/async_compute.hpp"
#include "manim/core/memory_pool.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace manim {

AsyncComputeManager::~AsyncComputeManager() {
    shutdown();
}

bool AsyncComputeManager::initialize(VkDevice device, VkPhysicalDevice physical_device,
                                     uint32_t graphics_queue_family, MemoryPool* memory_pool,
                                     const AsyncComputeConfig& config) {
    device_ = device;
    graphics_queue_family_ = graphics_queue_family;
    memory_pool_ = memory_pool;
    enabled_ = config.enable_async;

    if (!enabled_) {
        spdlog::info("AsyncComputeManager: disabled by configuration");
        return false;
    }

    // Find compute queue family
    int32_t compute_family = find_compute_queue_family(physical_device, graphics_queue_family);

    if (compute_family >= 0 && config.prefer_dedicated_compute_queue) {
        compute_queue_family_ = static_cast<uint32_t>(compute_family);
        is_dedicated_compute_queue_ = (compute_queue_family_ != graphics_queue_family_);
        spdlog::info("AsyncComputeManager: using {} compute queue family {}",
                    is_dedicated_compute_queue_ ? "dedicated" : "shared",
                    compute_queue_family_);
    } else {
        // Fall back to graphics queue family for compute
        compute_queue_family_ = graphics_queue_family_;
        is_dedicated_compute_queue_ = false;
        spdlog::info("AsyncComputeManager: using graphics queue for compute");
    }

    // Get compute queue
    // Note: If using dedicated queue, this assumes the queue was created during device creation
    // For shared queue, we use the same queue family but different queue index
    vkGetDeviceQueue(device_, compute_queue_family_, 0, &compute_queue_);

    if (compute_queue_ == VK_NULL_HANDLE && is_dedicated_compute_queue_) {
        // Dedicated compute queue was not allocated during device creation
        // Fall back to graphics queue
        spdlog::warn("AsyncComputeManager: dedicated compute queue not available, falling back to graphics queue");
        compute_queue_family_ = graphics_queue_family_;
        is_dedicated_compute_queue_ = false;
        vkGetDeviceQueue(device_, compute_queue_family_, 0, &compute_queue_);
    }

    if (compute_queue_ == VK_NULL_HANDLE) {
        spdlog::error("AsyncComputeManager: failed to get compute queue");
        enabled_ = false;
        return false;
    }

    // Create command pool for async compute
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = compute_queue_family_;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        spdlog::error("AsyncComputeManager: failed to create command pool");
        enabled_ = false;
        return false;
    }

    // Create timeline semaphore for synchronization
    VkSemaphoreTypeCreateInfo timeline_info{};
    timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timeline_info.initialValue = 0;

    VkSemaphoreCreateInfo sem_info{};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.pNext = &timeline_info;

    if (vkCreateSemaphore(device_, &sem_info, nullptr, &compute_complete_semaphore_) != VK_SUCCESS) {
        spdlog::error("AsyncComputeManager: failed to create timeline semaphore");
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
        enabled_ = false;
        return false;
    }

    // Pre-allocate command buffers
    command_buffers_.resize(config.max_pending_tasks);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& entry : command_buffers_) {
        if (vkAllocateCommandBuffers(device_, &alloc_info, &entry.cmd) != VK_SUCCESS) {
            spdlog::error("AsyncComputeManager: failed to allocate command buffer");
            shutdown();
            return false;
        }

        if (vkCreateFence(device_, &fence_info, nullptr, &entry.fence) != VK_SUCCESS) {
            spdlog::error("AsyncComputeManager: failed to create fence");
            shutdown();
            return false;
        }

        entry.in_use = false;
    }

    spdlog::info("AsyncComputeManager: initialized with {} command buffers, {} queue",
                command_buffers_.size(),
                is_dedicated_compute_queue_ ? "dedicated compute" : "shared graphics/compute");

    return true;
}

void AsyncComputeManager::shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        wait_idle();

        for (auto& entry : command_buffers_) {
            if (entry.fence != VK_NULL_HANDLE) {
                vkDestroyFence(device_, entry.fence, nullptr);
            }
        }
        command_buffers_.clear();

        if (compute_complete_semaphore_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, compute_complete_semaphore_, nullptr);
            compute_complete_semaphore_ = VK_NULL_HANDLE;
        }

        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
            command_pool_ = VK_NULL_HANDLE;
        }
    }

    device_ = VK_NULL_HANDLE;
    compute_queue_ = VK_NULL_HANDLE;
    enabled_ = false;
}

int32_t AsyncComputeManager::find_compute_queue_family(VkPhysicalDevice physical_device,
                                                        uint32_t graphics_family) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    // First, look for a dedicated compute queue (compute but not graphics)
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        const auto& props = queue_families[i];
        bool has_compute = (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
        bool has_graphics = (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;

        if (has_compute && !has_graphics && i != graphics_family) {
            spdlog::debug("Found dedicated compute queue family: {}", i);
            return static_cast<int32_t>(i);
        }
    }

    // Second, look for any compute-capable queue different from graphics
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (i == graphics_family) continue;

        const auto& props = queue_families[i];
        bool has_compute = (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;

        if (has_compute) {
            spdlog::debug("Found compute queue family: {}", i);
            return static_cast<int32_t>(i);
        }
    }

    // Fall back to graphics queue family
    spdlog::debug("No separate compute queue, using graphics family {}", graphics_family);
    return static_cast<int32_t>(graphics_family);
}

AsyncComputeManager::CommandBufferEntry* AsyncComputeManager::acquire_command_buffer() {
    std::lock_guard<std::mutex> lock(cmd_mutex_);

    for (auto& entry : command_buffers_) {
        if (!entry.in_use) {
            // Check if fence is signaled (previous work completed)
            if (vkGetFenceStatus(device_, entry.fence) == VK_SUCCESS) {
                vkResetFences(device_, 1, &entry.fence);
                vkResetCommandBuffer(entry.cmd, 0);
                entry.in_use = true;
                return &entry;
            }
        }
    }

    // All buffers busy, try to find one that's completed
    for (auto& entry : command_buffers_) {
        if (entry.in_use) {
            VkResult result = vkWaitForFences(device_, 1, &entry.fence, VK_TRUE, 0);
            if (result == VK_SUCCESS) {
                // Invoke callback if any
                if (entry.callback) {
                    AsyncTaskHandle handle{entry.task_id};
                    entry.callback(handle);
                    entry.callback = nullptr;
                }
                pending_count_--;

                vkResetFences(device_, 1, &entry.fence);
                vkResetCommandBuffer(entry.cmd, 0);
                entry.in_use = true;
                return &entry;
            }
        }
    }

    return nullptr;
}

void AsyncComputeManager::release_command_buffer(CommandBufferEntry* entry) {
    std::lock_guard<std::mutex> lock(cmd_mutex_);
    if (entry) {
        entry->in_use = false;
        entry->callback = nullptr;
    }
}

AsyncTaskHandle AsyncComputeManager::submit_async(
    std::function<void(VkCommandBuffer)> record_func,
    AsyncCompletionCallback callback) {

    if (!is_available()) {
        spdlog::warn("AsyncComputeManager: not available, cannot submit");
        return AsyncTaskHandle{0};
    }

    CommandBufferEntry* entry = acquire_command_buffer();
    if (!entry) {
        spdlog::warn("AsyncComputeManager: no command buffers available");
        return AsyncTaskHandle{0};
    }

    // Begin command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(entry->cmd, &begin_info) != VK_SUCCESS) {
        release_command_buffer(entry);
        return AsyncTaskHandle{0};
    }

    // Record user commands
    record_func(entry->cmd);

    if (vkEndCommandBuffer(entry->cmd) != VK_SUCCESS) {
        release_command_buffer(entry);
        return AsyncTaskHandle{0};
    }

    // Setup timeline semaphore signal
    uint64_t signal_value = ++timeline_value_;

    VkTimelineSemaphoreSubmitInfo timeline_submit{};
    timeline_submit.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_submit.signalSemaphoreValueCount = 1;
    timeline_submit.pSignalSemaphoreValues = &signal_value;

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_submit;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &entry->cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &compute_complete_semaphore_;

    VkResult result = vkQueueSubmit(compute_queue_, 1, &submit_info, entry->fence);
    if (result != VK_SUCCESS) {
        spdlog::error("AsyncComputeManager: queue submit failed");
        release_command_buffer(entry);
        return AsyncTaskHandle{0};
    }

    // Track task
    entry->task_id = next_task_id_++;
    entry->signal_value = signal_value;
    entry->callback = callback;
    pending_count_++;

    spdlog::debug("AsyncComputeManager: submitted task {} with signal value {}",
                 entry->task_id, signal_value);

    return AsyncTaskHandle{entry->task_id};
}

bool AsyncComputeManager::wait_for_task(AsyncTaskHandle handle, uint64_t timeout_ns) {
    if (!handle.valid()) return true;

    std::lock_guard<std::mutex> lock(cmd_mutex_);

    for (auto& entry : command_buffers_) {
        if (entry.in_use && entry.task_id == handle.id) {
            VkResult result = vkWaitForFences(device_, 1, &entry.fence, VK_TRUE, timeout_ns);
            if (result == VK_SUCCESS) {
                if (entry.callback) {
                    entry.callback(handle);
                    entry.callback = nullptr;
                }
                entry.in_use = false;
                pending_count_--;
                return true;
            }
            return false;
        }
    }

    // Task not found, assume completed
    return true;
}

void AsyncComputeManager::wait_idle() {
    if (compute_queue_ != VK_NULL_HANDLE) {
        vkQueueWaitIdle(compute_queue_);
    }

    // Process all callbacks
    poll_completions();
}

void AsyncComputeManager::poll_completions() {
    std::lock_guard<std::mutex> lock(cmd_mutex_);

    for (auto& entry : command_buffers_) {
        if (entry.in_use) {
            VkResult result = vkGetFenceStatus(device_, entry.fence);
            if (result == VK_SUCCESS) {
                if (entry.callback) {
                    AsyncTaskHandle handle{entry.task_id};
                    entry.callback(handle);
                    entry.callback = nullptr;
                }
                entry.in_use = false;
                pending_count_--;
            }
        }
    }
}

VkBufferMemoryBarrier AsyncComputeManager::create_compute_to_graphics_barrier(
    VkBuffer buffer, VkDeviceSize size,
    uint32_t compute_queue_family, uint32_t graphics_queue_family) {

    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = compute_queue_family;
    barrier.dstQueueFamilyIndex = graphics_queue_family;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = size;

    return barrier;
}

// ========================================================================
// Global AsyncComputeManager Accessor
// ========================================================================

static std::unique_ptr<AsyncComputeManager> g_async_compute;
static std::mutex g_async_mutex;

AsyncComputeManager& getGlobalAsyncCompute() {
    std::lock_guard<std::mutex> lock(g_async_mutex);
    if (!g_async_compute) {
        g_async_compute = std::make_unique<AsyncComputeManager>();
    }
    return *g_async_compute;
}

void initializeGlobalAsyncCompute(VkDevice device, VkPhysicalDevice physical_device,
                                   uint32_t graphics_queue_family, MemoryPool* pool) {
    std::lock_guard<std::mutex> lock(g_async_mutex);
    if (!g_async_compute) {
        g_async_compute = std::make_unique<AsyncComputeManager>();
    }
    g_async_compute->initialize(device, physical_device, graphics_queue_family, pool);
}

void shutdownGlobalAsyncCompute() {
    std::lock_guard<std::mutex> lock(g_async_mutex);
    if (g_async_compute) {
        g_async_compute->shutdown();
        g_async_compute.reset();
    }
}

bool isAsyncComputeAvailable() {
    std::lock_guard<std::mutex> lock(g_async_mutex);
    return g_async_compute != nullptr && g_async_compute->is_available();
}

} // namespace manim
