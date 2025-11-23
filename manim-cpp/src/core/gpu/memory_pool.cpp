/**
 * @file memory_pool.cpp
 * @brief Implementation of GPU memory pool management
 */

#include <manim/core/memory_pool.hpp>
#include <stdexcept>
#include <algorithm>

namespace manim {

// ============================================================================
// GPUBuffer Implementation
// ============================================================================

GPUBuffer::GPUBuffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size)
    : buffer_(buffer), allocation_(allocation), size_(size) {}

GPUBuffer::~GPUBuffer() {
    if (mapped_ptr_) {
        unmap();
    }
    // Note: Actual cleanup should be done via MemoryPool
}

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
    : buffer_(other.buffer_)
    , allocation_(other.allocation_)
    , size_(other.size_)
    , mapped_ptr_(other.mapped_ptr_) {
    other.buffer_ = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.size_ = 0;
    other.mapped_ptr_ = nullptr;
}

GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept {
    if (this != &other) {
        if (mapped_ptr_) {
            unmap();
        }

        buffer_ = other.buffer_;
        allocation_ = other.allocation_;
        size_ = other.size_;
        mapped_ptr_ = other.mapped_ptr_;

        other.buffer_ = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.size_ = 0;
        other.mapped_ptr_ = nullptr;
    }
    return *this;
}

VkDeviceAddress GPUBuffer::get_device_address() const {
    // Would require VkDevice to be passed
    // Simplified for now
    return 0;
}

void* GPUBuffer::map() {
    if (!mapped_ptr_) {
        // Would call vmaMapMemory here
        // Simplified for now
    }
    return mapped_ptr_;
}

void GPUBuffer::unmap() {
    if (mapped_ptr_) {
        // Would call vmaUnmapMemory here
        mapped_ptr_ = nullptr;
    }
}

// ============================================================================
// GPUImage Implementation
// ============================================================================

GPUImage::GPUImage(VkImage image, VmaAllocation allocation, VkImageView view,
                   uint32_t width, uint32_t height, VkFormat format)
    : image_(image)
    , allocation_(allocation)
    , view_(view)
    , width_(width)
    , height_(height)
    , format_(format) {}

GPUImage::~GPUImage() {
    // Cleanup done via MemoryPool
}

GPUImage::GPUImage(GPUImage&& other) noexcept
    : image_(other.image_)
    , allocation_(other.allocation_)
    , view_(other.view_)
    , width_(other.width_)
    , height_(other.height_)
    , format_(other.format_) {
    other.image_ = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.view_ = VK_NULL_HANDLE;
}

GPUImage& GPUImage::operator=(GPUImage&& other) noexcept {
    if (this != &other) {
        image_ = other.image_;
        allocation_ = other.allocation_;
        view_ = other.view_;
        width_ = other.width_;
        height_ = other.height_;
        format_ = other.format_;

        other.image_ = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.view_ = VK_NULL_HANDLE;
    }
    return *this;
}

// ============================================================================
// MemoryPool Implementation
// ============================================================================

void MemoryPool::initialize(VkDevice device, VkPhysicalDevice physical_device) {
    device_ = device;
    physical_device_ = physical_device;

    // Create VMA allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = device;
    allocator_info.instance = VK_NULL_HANDLE;  // Would need to pass instance

    VkResult result = vmaCreateAllocator(&allocator_info, &allocator_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
}

void MemoryPool::shutdown() {
    if (allocator_) {
        vmaDestroyAllocator(allocator_);
        allocator_ = VK_NULL_HANDLE;
    }
}

GPUBuffer MemoryPool::allocate_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    MemoryType memory_type,
    MemoryUsage memory_usage
) {
    // Add shader device address usage for GPU access
    usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    // Add transfer flags if needed
    usage = add_transfer_flags(usage, memory_type);

    // Create buffer
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Set up allocation
    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = get_vma_usage(memory_type, memory_usage);

    // For host-visible memory, request persistent mapping
    if (memory_type == MemoryType::HostVisible ||
        memory_type == MemoryType::Unified) {
        alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    VkBuffer buffer;
    VmaAllocation allocation;
    VkResult result = vmaCreateBuffer(
        allocator_,
        &buffer_info,
        &alloc_info,
        &buffer,
        &allocation,
        nullptr
    );

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer");
    }

    // Update statistics
    total_allocated_ += size;
    num_allocations_++;

    return GPUBuffer(buffer, allocation, size);
}

GPUImage MemoryPool::allocate_image(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    MemoryType memory_type
) {
    // Create image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = {width, height, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = get_vma_usage(memory_type, MemoryUsage::Static);

    VkImage image;
    VmaAllocation allocation;
    VkResult result = vmaCreateImage(
        allocator_,
        &image_info,
        &alloc_info,
        &image,
        &allocation,
        nullptr
    );

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image");
    }

    // Create image view
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView view;
    result = vkCreateImageView(device_, &view_info, nullptr, &view);
    if (result != VK_SUCCESS) {
        vmaDestroyImage(allocator_, image, allocation);
        throw std::runtime_error("Failed to create image view");
    }

    return GPUImage(image, allocation, view, width, height, format);
}

GPUBuffer MemoryPool::create_staging_buffer(VkDeviceSize size) {
    return allocate_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::Staging,
        MemoryUsage::Streaming
    );
}

MemoryPool::Stats MemoryPool::get_stats() const {
    Stats stats;
    stats.total_allocated = total_allocated_.load();
    stats.num_allocations = num_allocations_.load();

    // Would query VMA for more detailed stats
    VmaTotalStatistics vma_stats;
    vmaCalculateStatistics(allocator_, &vma_stats);

    stats.device_local_allocated = vma_stats.memoryType[0].statistics.allocationBytes;
    stats.host_visible_allocated = vma_stats.memoryType[1].statistics.allocationBytes;
    stats.num_buffers = vma_stats.total.statistics.blockCount;
    stats.num_images = 0;  // Would need to track separately

    return stats;
}

void MemoryPool::defragment() {
    // VMA defragmentation
    VmaDefragmentationInfo defrag_info{};
    defrag_info.flags = VMA_DEFRAGMENTATION_FLAG_ALGORITHM_FAST_BIT;

    VmaDefragmentationContext defrag_ctx;
    VkResult result = vmaBeginDefragmentation(allocator_, &defrag_info, &defrag_ctx);

    if (result == VK_SUCCESS) {
        // Perform defragmentation passes
        VmaDefragmentationPassMoveInfo pass_info{};
        while (true) {
            result = vmaBeginDefragmentationPass(allocator_, defrag_ctx, &pass_info);
            if (result == VK_SUCCESS) {
                // Move completed
                vmaEndDefragmentationPass(allocator_, defrag_ctx, &pass_info);
            } else if (result == VK_INCOMPLETE) {
                // More passes needed
                vmaEndDefragmentationPass(allocator_, defrag_ctx, &pass_info);
                continue;
            } else {
                break;
            }
        }

        vmaEndDefragmentation(allocator_, defrag_ctx, nullptr);
    }
}

VmaMemoryUsage MemoryPool::get_vma_usage(MemoryType type, MemoryUsage usage) const {
    switch (type) {
        case MemoryType::DeviceLocal:
            return VMA_MEMORY_USAGE_GPU_ONLY;

        case MemoryType::HostVisible:
            return (usage == MemoryUsage::Dynamic)
                ? VMA_MEMORY_USAGE_CPU_TO_GPU
                : VMA_MEMORY_USAGE_AUTO;

        case MemoryType::HostCached:
            return VMA_MEMORY_USAGE_GPU_TO_CPU;

        case MemoryType::Staging:
            return VMA_MEMORY_USAGE_CPU_ONLY;

        case MemoryType::Unified:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        default:
            return VMA_MEMORY_USAGE_AUTO;
    }
}

VkBufferUsageFlags MemoryPool::add_transfer_flags(
    VkBufferUsageFlags usage,
    MemoryType type
) const {
    // Add transfer flags for non-device-local memory
    if (type != MemoryType::DeviceLocal) {
        usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    return usage;
}

// ============================================================================
// RingBuffer Implementation
// ============================================================================

void RingBuffer::initialize(MemoryPool& pool, VkDeviceSize size_per_frame) {
    size_per_frame_ = size_per_frame;

    for (int i = 0; i < NUM_FRAMES; ++i) {
        buffers_[i] = pool.allocate_buffer(
            size_per_frame,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );

        mapped_ptrs_[i] = buffers_[i].map();
        offsets_[i] = 0;
    }
}

void RingBuffer::shutdown() {
    for (auto& buffer : buffers_) {
        buffer.unmap();
    }
}

RingBuffer::Allocation RingBuffer::allocate(VkDeviceSize size, VkDeviceSize alignment) {
    int frame = current_frame_;

    // Align offset
    VkDeviceSize aligned_offset = (offsets_[frame] + alignment - 1) & ~(alignment - 1);

    if (aligned_offset + size > size_per_frame_) {
        throw std::runtime_error("Ring buffer overflow");
    }

    Allocation alloc;
    alloc.buffer = buffers_[frame].get_buffer();
    alloc.offset = aligned_offset;
    alloc.size = size;
    alloc.mapped_ptr = static_cast<char*>(mapped_ptrs_[frame]) + aligned_offset;

    offsets_[frame] = aligned_offset + size;

    return alloc;
}

void RingBuffer::next_frame() {
    current_frame_ = (current_frame_ + 1) % NUM_FRAMES;
    offsets_[current_frame_] = 0;  // Reset offset for next frame
}

} // namespace manim
