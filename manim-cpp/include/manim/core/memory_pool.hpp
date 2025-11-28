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
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <span>
#include <cstring>
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
    GPUBuffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size, VmaAllocator allocator);
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
    bool is_initialized() const { return initialized_; }

    // Map/unmap for host-visible memory
    void* map();
    const void* map() const;
    void unmap() const;

    // Upload data to buffer
    template<typename T>
    void upload(std::span<const T> data);
    template<typename T>
    void upload(const std::vector<T>& data) { upload(std::span<const T>(data)); }

    // Download data from buffer
    template<typename T>
    void download(std::span<T> data);

    /**
     * @brief Allocate buffer with validation
     * @param size Size in bytes to allocate
     * @throws std::invalid_argument if size is 0 or negative
     * @throws std::runtime_error if size exceeds available GPU memory
     */
    void allocate(VkDeviceSize size);

    void free() {
        size_ = 0;
        initialized_ = false;
        buffer_ = VK_NULL_HANDLE;
        allocation_ = VK_NULL_HANDLE;
        allocator_ = VK_NULL_HANDLE;
        cpu_shadow_.clear();
    }

    /**
     * @brief Download float data from buffer
     * @throws std::runtime_error if buffer is not initialized
     */
    std::vector<float> download() const;

    void upload_colors(const std::vector<glm::vec4>& colors) {
        size_ = colors.size() * sizeof(glm::vec4);
        initialized_ = true;
        // Store in CPU shadow for fallback
        cpu_shadow_.resize(size_);
        std::memcpy(cpu_shadow_.data(), colors.data(), size_);
    }
    std::vector<glm::vec4> download_colors() const;

    template<typename MatType>
    void upload_matrices(const std::vector<MatType>& mats) {
        size_ = mats.size() * sizeof(MatType);
        initialized_ = true;
        // Store in CPU shadow for fallback
        cpu_shadow_.resize(size_);
        std::memcpy(cpu_shadow_.data(), mats.data(), size_);
    }

    /**
     * @brief Download matrix data from buffer
     * @throws std::runtime_error if buffer is not initialized
     */
    template<typename MatType = glm::mat4>
    std::vector<MatType> download_matrices() const;

    /// Query available GPU memory (static utility)
    static VkDeviceSize get_available_gpu_memory();

private:
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    void* mapped_ptr_ = nullptr;
    VmaAllocator allocator_ = VK_NULL_HANDLE;  // Non-owning
    mutable std::vector<uint8_t> cpu_shadow_;  // Fallback storage when not mapped
    bool initialized_ = false;                 // Track if buffer has valid data
};

/**
 * @brief GPU texture/image handle
 */
class GPUImage {
public:
    GPUImage() = default;
    GPUImage(VkImage image, VmaAllocation allocation, VkImageView view,
             uint32_t width, uint32_t height, VkFormat format, VmaAllocator allocator, VkDevice device);
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
    VmaAllocator allocator_ = VK_NULL_HANDLE;  // Non-owning
    VkDevice device_ = VK_NULL_HANDLE;         // Non-owning
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
    void initialize(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkInstance instance = VK_NULL_HANDLE
    );

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
    GPUBuffer allocate_buffer(VkDeviceSize size) {
        GPUBuffer buffer;
        buffer.allocate(size);
        return buffer;
    }
    void free_buffer(GPUBuffer& /*buffer*/) {}

    /**
     * @brief Allocate GPU image/texture
     * @param samples Sample count for MSAA (default VK_SAMPLE_COUNT_1_BIT)
     */
    GPUImage allocate_image(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        MemoryType memory_type = MemoryType::DeviceLocal,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
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

    /**
     * @brief Set command pool and queue for transfer operations
     */
    void set_transfer_resources(VkCommandPool pool, VkQueue queue, uint32_t queue_family);

    /**
     * @brief Upload data to GPU image with automatic staging
     *
     * Creates a staging buffer, copies data to it, then submits commands
     * to transition image layout and copy buffer to image.
     *
     * @param image Target GPU image
     * @param data Pointer to pixel data
     * @param size Size of pixel data in bytes
     */
    void upload_to_image(GPUImage& image, const void* data, VkDeviceSize size);

    /**
     * @brief Get Vulkan device
     */
    VkDevice get_device() const { return device_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VmaAllocator allocator_ = VK_NULL_HANDLE;

    // Transfer command resources
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkQueue transfer_queue_ = VK_NULL_HANDLE;
    uint32_t queue_family_index_ = 0;

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
        size_ = data.size_bytes();
    }

    initialized_ = true;

    // Store in CPU shadow for fallback
    cpu_shadow_.resize(data.size_bytes());
    std::memcpy(cpu_shadow_.data(), data.data(), data.size_bytes());

    void* ptr = map();
    if (ptr && ptr != cpu_shadow_.data()) {
        std::memcpy(ptr, data.data(), data.size_bytes());
        unmap();
    }
}

template<typename T>
void GPUBuffer::download(std::span<T> data) {
    if (!initialized_) {
        throw std::runtime_error("Cannot download from uninitialized buffer");
    }

    if (data.size_bytes() > size_) {
        data = data.first(size_ / sizeof(T));
    }

    void* ptr = map();
    if (ptr) {
        std::memcpy(data.data(), ptr, data.size_bytes());
        unmap();
    }
}

template<typename MatType>
std::vector<MatType> GPUBuffer::download_matrices() const {
    if (!initialized_) {
        throw std::runtime_error("Cannot download from uninitialized buffer");
    }

    size_t count = static_cast<size_t>(size_ / sizeof(MatType));
    std::vector<MatType> result(count);

    // Try to map GPU memory first
    const void* ptr = map();
    if (ptr) {
        std::memcpy(result.data(), ptr, count * sizeof(MatType));
        unmap();
    } else if (!cpu_shadow_.empty()) {
        // Fall back to CPU shadow storage
        size_t copy_size = std::min(static_cast<size_t>(size_), cpu_shadow_.size());
        std::memcpy(result.data(), cpu_shadow_.data(), copy_size);
    }

    return result;
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

    // TODO.md: Copy from staging to device buffer using command buffer
    // This would be done via a transfer command in a real implementation
}

template<typename T>
void MemoryPool::download_from_buffer(const GPUBuffer& buffer, std::span<T> data) {
    // Similar to upload but in reverse
    // TODO.md: Implement with staging buffer and transfer command
}

using GPUMemoryPool = MemoryPool;

} // namespace manim
