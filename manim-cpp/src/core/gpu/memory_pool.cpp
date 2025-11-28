/**
 * @file memory_pool.cpp
 * @brief Implementation of GPU memory pool management
 */

#define VMA_IMPLEMENTATION
#include <manim/core/memory_pool.hpp>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <spdlog/spdlog.h>

namespace manim {

// ============================================================================
// GPUBuffer Implementation
// ============================================================================

GPUBuffer::GPUBuffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size, VmaAllocator allocator)
    : buffer_(buffer), allocation_(allocation), size_(size), allocator_(allocator), initialized_(size > 0) {}

GPUBuffer::~GPUBuffer() {
    if (mapped_ptr_) {
        unmap();
    }
    // GPUBuffer does not own allocator lifetime; MemoryPool manages destruction.
}

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
    : buffer_(other.buffer_)
    , allocation_(other.allocation_)
    , size_(other.size_)
    , mapped_ptr_(other.mapped_ptr_)
    , allocator_(other.allocator_)
    , cpu_shadow_(std::move(other.cpu_shadow_))
    , initialized_(other.initialized_) {
    other.buffer_ = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.size_ = 0;
    other.mapped_ptr_ = nullptr;
    other.allocator_ = VK_NULL_HANDLE;
    other.initialized_ = false;
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
        allocator_ = other.allocator_;
        cpu_shadow_ = std::move(other.cpu_shadow_);
        initialized_ = other.initialized_;

        other.buffer_ = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.size_ = 0;
        other.mapped_ptr_ = nullptr;
        other.allocator_ = VK_NULL_HANDLE;
        other.initialized_ = false;
    }
    return *this;
}

VkDeviceAddress GPUBuffer::get_device_address() const {
    // Would require VkDevice to be passed
    // Simplified for now
    return 0;
}

void* GPUBuffer::map() {
    if (mapped_ptr_) {
        return mapped_ptr_;
    }
    if (allocator_ && allocation_) {
        if (vmaMapMemory(allocator_, allocation_, &mapped_ptr_) != VK_SUCCESS) {
            mapped_ptr_ = nullptr;
        }
    }
    if (!mapped_ptr_) {
        // Fallback to CPU shadow storage
        if (cpu_shadow_.size() < static_cast<size_t>(size_)) {
            cpu_shadow_.resize(static_cast<size_t>(size_));
        }
        mapped_ptr_ = cpu_shadow_.data();
    }
    return mapped_ptr_;
}

const void* GPUBuffer::map() const {
    return const_cast<GPUBuffer*>(this)->map();
}

void GPUBuffer::unmap() const {
    auto* self = const_cast<GPUBuffer*>(this);
    if (self->mapped_ptr_ && self->allocator_ && self->allocation_) {
        vmaUnmapMemory(self->allocator_, self->allocation_);
    }
    self->mapped_ptr_ = nullptr;
}

// ============================================================================
// GPUImage Implementation
// ============================================================================

GPUImage::GPUImage(VkImage image, VmaAllocation allocation, VkImageView view,
                   uint32_t width, uint32_t height, VkFormat format, VmaAllocator allocator, VkDevice device)
    : image_(image)
    , allocation_(allocation)
    , view_(view)
    , width_(width)
    , height_(height)
    , format_(format)
    , allocator_(allocator)
    , device_(device) {}

GPUImage::~GPUImage() {
    // GPUImage does not own allocator/device lifetime; MemoryPool manages destruction.
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

void MemoryPool::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkInstance instance
) {
    device_ = device;
    physical_device_ = physical_device;

    if (device_ == VK_NULL_HANDLE || physical_device_ == VK_NULL_HANDLE) {
        spdlog::warn("MemoryPool initialized without a valid Vulkan device/physical device; allocations will be placeholders.");
        allocator_ = VK_NULL_HANDLE;
        return;
    }

    // Create VMA allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = device;
    allocator_info.instance = instance;
    // Enable buffer device address support for GPU-side buffer access
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

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

std::vector<float> GPUBuffer::download() const {
    if (!initialized_) {
        throw std::runtime_error("Cannot download from uninitialized buffer");
    }

    size_t count = static_cast<size_t>(size_ / sizeof(float));
    std::vector<float> out(count);
    const void* mapped = map();
    if (mapped) {
        std::memcpy(out.data(), mapped, count * sizeof(float));
        unmap();
    }
    return out;
}

void GPUBuffer::allocate(VkDeviceSize size) {
    // Validate size - reject 0 or negative (when interpreted as signed)
    if (size == 0) {
        throw std::invalid_argument("Invalid buffer size: cannot allocate 0 bytes");
    }

    // Check for negative size (size is unsigned, but -1 becomes a huge value)
    // VkDeviceSize is uint64_t, so values > INT64_MAX are likely negative inputs
    constexpr VkDeviceSize MAX_SANE_SIZE = static_cast<VkDeviceSize>(std::numeric_limits<int64_t>::max());
    if (size > MAX_SANE_SIZE) {
        throw std::invalid_argument("Invalid buffer size: negative size detected");
    }

    // Check against available GPU memory to prevent OOM
    VkDeviceSize available = get_available_gpu_memory();
    if (available > 0 && size > available) {
        throw std::runtime_error("Out of memory: requested size exceeds available GPU memory");
    }

    // For very large allocations (>16GB), reject even if we can't query memory
    constexpr VkDeviceSize MAX_REASONABLE_SIZE = 16ULL * 1024 * 1024 * 1024;  // 16GB
    if (available == 0 && size > MAX_REASONABLE_SIZE) {
        throw std::runtime_error("Out of memory: requested size exceeds reasonable GPU memory limit");
    }

    size_ = size;
    initialized_ = true;

    // Prepare CPU shadow storage
    cpu_shadow_.resize(static_cast<size_t>(size));
}

VkDeviceSize GPUBuffer::get_available_gpu_memory() {
    // Query GPU memory using Vulkan API
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ManimMemCheck";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Manim";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.enabledExtensionCount = 0;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS) {
        return 0;  // Can't query, return 0 to indicate unknown
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        vkDestroyInstance(instance, nullptr);
        return 0;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkDeviceSize maxMemory = 0;

    for (const auto& device : devices) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(device, &memProps);

        // Sum up device-local memory heaps
        for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                maxMemory = std::max(maxMemory, memProps.memoryHeaps[i].size);
            }
        }
    }

    vkDestroyInstance(instance, nullptr);

    return maxMemory;
}

std::vector<glm::vec4> GPUBuffer::download_colors() const {
    size_t count = static_cast<size_t>(size_ / sizeof(glm::vec4));
    std::vector<glm::vec4> out(count);
    const void* mapped = map();
    if (mapped) {
        std::memcpy(out.data(), mapped, count * sizeof(glm::vec4));
        unmap();
    }
    return out;
}

GPUBuffer MemoryPool::allocate_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    MemoryType memory_type,
    MemoryUsage memory_usage
) {
    if (!allocator_) {
        spdlog::warn("MemoryPool allocate_buffer called without a valid VMA allocator; returning placeholder buffer (size={} bytes).", size);
        return GPUBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE, size, allocator_);
    }

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

    return GPUBuffer(buffer, allocation, size, allocator_);
}

GPUImage MemoryPool::allocate_image(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    MemoryType memory_type,
    VkSampleCountFlagBits samples
) {
    if (!allocator_) {
        spdlog::warn("MemoryPool allocate_image called without a valid VMA allocator; returning placeholder image ({}x{}, format {}).", width, height, static_cast<int>(format));
        return GPUImage(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, width, height, format, allocator_, device_);
    }

    // Create image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = {width, height, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = samples;  // Use provided sample count for MSAA
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

    // Create image view with correct aspect mask based on format
    VkImageAspectFlags aspect_mask;
    switch (format) {
        // Depth-only formats
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        // Depth + Stencil formats
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;  // Use only DEPTH for view, not both
            break;
        // Stencil-only format
        case VK_FORMAT_S8_UINT:
            aspect_mask = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        // All other formats are color
        default:
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
    }

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_mask;
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

    return GPUImage(image, allocation, view, width, height, format, allocator_, device_);
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

    if (!allocator_) {
        stats.device_local_allocated = 0;
        stats.host_visible_allocated = 0;
        stats.num_buffers = 0;
        stats.num_images = 0;
        return stats;
    }

    VmaTotalStatistics vma_stats;
    vmaCalculateStatistics(allocator_, &vma_stats);

    stats.device_local_allocated = vma_stats.memoryType[0].statistics.allocationBytes;
    stats.host_visible_allocated = vma_stats.memoryType[1].statistics.allocationBytes;
    stats.num_buffers = vma_stats.total.statistics.blockCount;
    stats.num_images = 0;  // Would need to track separately

    return stats;
}

void MemoryPool::defragment() {
    if (!allocator_) {
        spdlog::warn("MemoryPool defragment called without a valid allocator; skipping.");
        return;
    }

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

void MemoryPool::set_transfer_resources(VkCommandPool pool, VkQueue queue, uint32_t queue_family) {
    command_pool_ = pool;
    transfer_queue_ = queue;
    queue_family_index_ = queue_family;
}

void MemoryPool::upload_to_image(GPUImage& image, const void* data, VkDeviceSize size) {
    if (!allocator_ || !device_ || !command_pool_ || !transfer_queue_) {
        spdlog::warn("MemoryPool::upload_to_image called without proper initialization");
        return;
    }

    if (!image.get_image()) {
        spdlog::warn("MemoryPool::upload_to_image called with invalid image");
        return;
    }

    // Create staging buffer
    GPUBuffer staging = create_staging_buffer(size);

    // Copy data to staging buffer
    void* mapped = staging.map();
    if (mapped) {
        std::memcpy(mapped, data, size);
        staging.unmap();
    } else {
        spdlog::error("Failed to map staging buffer for image upload");
        return;
    }

    // Create a one-shot command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buffer;
    if (vkAllocateCommandBuffers(device_, &alloc_info, &cmd_buffer) != VK_SUCCESS) {
        spdlog::error("Failed to allocate command buffer for image upload");
        return;
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd_buffer, &begin_info);

    // Transition image from UNDEFINED to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.get_image();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;   // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {image.get_width(), image.get_height(), 1};

    vkCmdCopyBufferToImage(
        cmd_buffer,
        staging.get_buffer(),
        image.get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // Transition image from TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(cmd_buffer);

    // Submit and wait
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;

    vkQueueSubmit(transfer_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(transfer_queue_);

    // Cleanup
    vkFreeCommandBuffers(device_, command_pool_, 1, &cmd_buffer);

    spdlog::debug("Uploaded {} bytes to GPU image {}x{}", size, image.get_width(), image.get_height());
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
