/**
 * @file memory_pool.hpp
 * @brief GPU-first unified memory pool management for Manim C++
 *
 * Provides intelligent memory allocation, automatic CPU-GPU migration,
 * and zero-copy operations where possible.
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <span>
#include <glm/glm.hpp>

namespace manim {

/**
 * @brief Memory allocation type
 */
enum class MemoryType {
    DeviceLocal,        ///< GPU-only memory (fastest for GPU operations)
    HostVisible,        ///< CPU-visible GPU memory (for frequent updates)
    HostCached,         ///< Cached host memory (for GPU -> CPU readback)
    Staging,            ///< Temporary staging memory for transfers
    Unified             ///< Unified memory (CPU + GPU accessible)
};

/**
 * @brief Memory usage hint for automatic optimization
 */
enum class MemoryUsage {
    Static,             ///< Written once, read many times
    Dynamic,            ///< Updated frequently (every frame)
    Streaming,          ///< Updated once per frame, read once
    Readback            ///< GPU writes, CPU reads
};

/**
 * @brief GPU buffer handle with automatic lifetime management
 */
class GPUBuffer {
public:
    GPUBuffer() = default;
    GPUBuffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size);
    ~GPUBuffer();

    // Move-only semantics
    GPUBuffer(GPUBuffer&& other) noexcept;
    GPUBuffer& operator=(GPUBuffer&& other) noexcept;
    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;

    VkBuffer get_buffer() const { return buffer_; }
    VmaAllocation get_allocation() const { return allocation_; }
    VkDeviceSize get_size() const { return size_; }
    VkDeviceAddress get_device_address() const;

    // Map/unmap for host-visible memory
    void* map();
    void unmap();

    // Upload data to buffer
    template<typename T>
    void upload(std::span<const T> data);

    // Download data from buffer
    template<typename T>
    void download(std::span<T> data);

private:
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    void* mapped_ptr_ = nullptr;
};

/**
 * @brief GPU texture/image handle
 */
class GPUImage {
public:
    GPUImage() = default;
    GPUImage(VkImage image, VmaAllocation allocation, VkImageView view,
             uint32_t width, uint32_t height, VkFormat format);
    ~GPUImage();

    // Move-only
    GPUImage(GPUImage&& other) noexcept;
    GPUImage& operator=(GPUImage&& other) noexcept;
    GPUImage(const GPUImage&) = delete;
    GPUImage& operator=(const GPUImage&) = delete;

    VkImage get_image() const { return image_; }
    VkImageView get_view() const { return view_; }
    uint32_t get_width() const { return width_; }
    uint32_t get_height() const { return height_; }

private:
    VkImage image_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
};

/**
 * @brief Memory pool for efficient GPU allocations
 *
 * Features:
 * - Per-type memory pools (vertex, index, uniform, storage)
 * - Automatic defragmentation
 * - Statistics tracking
 * - Zero-copy operations
 */
class MemoryPool {
public:
    /**
     * @brief Initialize memory pool
     */
    void initialize(VkDevice device, VkPhysicalDevice physical_device);

    /**
     * @brief Shutdown and free all memory
     */
    void shutdown();

    /**
     * @brief Allocate GPU buffer
     *
     * @param size Buffer size in bytes
     * @param usage Vulkan buffer usage flags
     * @param memory_type Memory type for allocation
     * @param memory_usage Usage hint for optimization
     * @return Allocated GPU buffer
     */
    GPUBuffer allocate_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        MemoryType memory_type = MemoryType::DeviceLocal,
        MemoryUsage memory_usage = MemoryUsage::Static
    );

    /**
     * @brief Allocate GPU image/texture
     */
    GPUImage allocate_image(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        MemoryType memory_type = MemoryType::DeviceLocal
    );

    /**
     * @brief Create staging buffer for CPU->GPU upload
     */
    GPUBuffer create_staging_buffer(VkDeviceSize size);

    /**
     * @brief Upload data to GPU buffer with automatic staging
     *
     * This automatically creates a staging buffer if needed.
     */
    template<typename T>
    void upload_to_buffer(GPUBuffer& buffer, std::span<const T> data);

    /**
     * @brief Download data from GPU buffer with automatic staging
     */
    template<typename T>
    void download_from_buffer(const GPUBuffer& buffer, std::span<T> data);

    /**
     * @brief Get memory statistics
     */
    struct Stats {
        VkDeviceSize total_allocated;
        VkDeviceSize device_local_allocated;
        VkDeviceSize host_visible_allocated;
        size_t num_allocations;
        size_t num_buffers;
        size_t num_images;
    };

    Stats get_stats() const;

    /**
     * @brief Defragment memory (compact allocations)
     */
    void defragment();

    /**
     * @brief Get VMA allocator
     */
    VmaAllocator get_allocator() const { return allocator_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VmaAllocator allocator_ = VK_NULL_HANDLE;

    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<VkDeviceSize> total_allocated_{0};
    std::atomic<size_t> num_allocations_{0};

    // Helper functions
    VmaMemoryUsage get_vma_usage(MemoryType type, MemoryUsage usage) const;
    VkBufferUsageFlags add_transfer_flags(VkBufferUsageFlags usage, MemoryType type) const;
};

/**
 * @brief Ring buffer for per-frame dynamic allocations
 *
 * Triple-buffered for frame pipelining.
 */
class RingBuffer {
public:
    static constexpr int NUM_FRAMES = 3;

    void initialize(MemoryPool& pool, VkDeviceSize size_per_frame);
    void shutdown();

    /**
     * @brief Allocate from current frame's buffer
     *
     * @param size Size to allocate
     * @param alignment Alignment requirement
     * @return Offset into the current frame's buffer
     */
    struct Allocation {
        VkBuffer buffer;
        VkDeviceSize offset;
        VkDeviceSize size;
        void* mapped_ptr;
    };

    Allocation allocate(VkDeviceSize size, VkDeviceSize alignment = 256);

    /**
     * @brief Advance to next frame and reset offset
     */
    void next_frame();

    /**
     * @brief Get current frame index
     */
    int get_current_frame() const { return current_frame_; }

private:
    std::array<GPUBuffer, NUM_FRAMES> buffers_;
    std::array<VkDeviceSize, NUM_FRAMES> offsets_{};
    std::array<void*, NUM_FRAMES> mapped_ptrs_{};
    int current_frame_ = 0;
    VkDeviceSize size_per_frame_ = 0;
};

} // namespace manim

// Template implementations
namespace manim {

template<typename T>
void GPUBuffer::upload(std::span<const T> data) {
    if (data.size_bytes() > size_) {
        throw std::runtime_error("Data size exceeds buffer size");
    }

    void* ptr = map();
    std::memcpy(ptr, data.data(), data.size_bytes());
    unmap();
}

template<typename T>
void GPUBuffer::download(std::span<T> data) {
    if (data.size_bytes() > size_) {
        throw std::runtime_error("Data size exceeds buffer size");
    }

    void* ptr = map();
    std::memcpy(data.data(), ptr, data.size_bytes());
    unmap();
}

template<typename T>
void MemoryPool::upload_to_buffer(GPUBuffer& buffer, std::span<const T> data) {
    // For host-visible buffers, direct upload
    if (buffer.map()) {
        buffer.upload(data);
        return;
    }

    // For device-local buffers, use staging
    auto staging = create_staging_buffer(data.size_bytes());
    staging.upload(data);

    // TODO: Copy from staging to device buffer using command buffer
    // This would be done via a transfer command in a real implementation
}

template<typename T>
void MemoryPool::download_from_buffer(const GPUBuffer& buffer, std::span<T> data) {
    // Similar to upload but in reverse
    // TODO: Implement with staging buffer and transfer command
}

} // namespace manim
