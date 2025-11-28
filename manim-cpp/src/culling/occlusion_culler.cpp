#include "manim/culling/occlusion_culler.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <glm/gtc/matrix_inverse.hpp>

namespace manim {
namespace culling {

// Push constants for occlusion test shader (local definition matching shader)
struct OcclusionPushConstants {
    math::Mat4 view_proj;
    float screen_size[2];
    uint32_t num_frustum_visible;
    uint32_t hiz_mip_levels;
};

// Push constants for depth-to-HiZ copy shader
struct DepthCopyPushConstants {
    int32_t width;
    int32_t height;
    uint32_t padding[2];
};

// Push constants for temporal reprojection shader
struct ReprojectPushConstants {
    math::Mat4 prev_view_proj_inv;
    math::Mat4 curr_view_proj;
    int32_t width;
    int32_t height;
    float depth_threshold;
    uint32_t use_current_fallback;
};

OcclusionCuller::OcclusionCuller() = default;

OcclusionCuller::~OcclusionCuller() {
    cleanup();
}

OcclusionCuller::OcclusionCuller(OcclusionCuller&& other) noexcept {
    *this = std::move(other);
}

OcclusionCuller& OcclusionCuller::operator=(OcclusionCuller&& other) noexcept {
    if (this != &other) {
        cleanup();

        device_ = other.device_;
        physical_device_ = other.physical_device_;
        memory_pool_ = other.memory_pool_;

        hiz_pyramid_image_ = other.hiz_pyramid_image_;
        hiz_pyramid_memory_ = other.hiz_pyramid_memory_;
        hiz_pyramid_view_ = other.hiz_pyramid_view_;
        hiz_mip_views_ = std::move(other.hiz_mip_views_);
        hiz_sampler_ = other.hiz_sampler_;
        hiz_width_ = other.hiz_width_;
        hiz_height_ = other.hiz_height_;
        current_mip_levels_ = other.current_mip_levels_;

        hiz_build_pipeline_ = other.hiz_build_pipeline_;
        hiz_build_layout_ = other.hiz_build_layout_;
        hiz_build_desc_layout_ = other.hiz_build_desc_layout_;
        hiz_build_desc_pool_ = other.hiz_build_desc_pool_;
        hiz_build_desc_sets_ = std::move(other.hiz_build_desc_sets_);

        occlusion_pipeline_ = other.occlusion_pipeline_;
        occlusion_layout_ = other.occlusion_layout_;
        occlusion_desc_layout_ = other.occlusion_desc_layout_;
        occlusion_desc_pool_ = other.occlusion_desc_pool_;
        occlusion_desc_set_ = other.occlusion_desc_set_;

        command_pool_ = other.command_pool_;
        command_buffer_ = other.command_buffer_;
        compute_queue_ = other.compute_queue_;
        fence_ = other.fence_;

        bounds_staging_buffer_ = std::move(other.bounds_staging_buffer_);
        frustum_visible_staging_buffer_ = std::move(other.frustum_visible_staging_buffer_);
        visible_indices_buffer_ = std::move(other.visible_indices_buffer_);
        atomic_counter_buffer_ = std::move(other.atomic_counter_buffer_);

        initialized_ = other.initialized_;
        max_objects_ = other.max_objects_;
        config_ = other.config_;
        stats_ = other.stats_;

        // Invalidate source
        other.device_ = VK_NULL_HANDLE;
        other.initialized_ = false;
    }
    return *this;
}

void OcclusionCuller::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t compute_queue_family
) {
    if (initialized_) {
        spdlog::warn("OcclusionCuller already initialized");
        return;
    }

    device_ = device;
    physical_device_ = physical_device;
    memory_pool_ = &memory_pool;

    // Get compute queue
    vkGetDeviceQueue(device_, compute_queue_family, 0, &compute_queue_);

    // Create command pool
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = compute_queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion culler command pool");
        return;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate occlusion culler command buffer");
        return;
    }

    // Create fence
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(device_, &fence_info, nullptr, &fence_) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion culler fence");
        return;
    }

    // Create Hi-Z sampler
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = static_cast<float>(config_.max_mip_levels);

    if (vkCreateSampler(device_, &sampler_info, nullptr, &hiz_sampler_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z sampler");
        return;
    }

    // Create pipelines
    create_hiz_build_pipeline();
    create_occlusion_pipeline();
    create_depth_copy_pipeline();
    create_reproject_pipeline();

    // Create descriptor pool for depth copy and reproject pipelines
    {
        VkDescriptorPoolSize pool_sizes[2] = {};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[0].descriptorCount = 5;  // 1 for depth copy, 2 for reproject, buffer
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        pool_sizes[1].descriptorCount = 3;  // 1 for depth copy, 1 for reproject, buffer

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.maxSets = 2;
        pool_info.poolSizeCount = 2;
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool extra_pool;
        if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &extra_pool) == VK_SUCCESS) {
            // Allocate depth copy descriptor set
            if (depth_copy_desc_layout_) {
                VkDescriptorSetAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                alloc_info.descriptorPool = extra_pool;
                alloc_info.descriptorSetCount = 1;
                alloc_info.pSetLayouts = &depth_copy_desc_layout_;
                vkAllocateDescriptorSets(device_, &alloc_info, &depth_copy_desc_set_);
            }

            // Allocate reproject descriptor set
            if (reproject_desc_layout_) {
                VkDescriptorSetAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                alloc_info.descriptorPool = extra_pool;
                alloc_info.descriptorSetCount = 1;
                alloc_info.pSetLayouts = &reproject_desc_layout_;
                vkAllocateDescriptorSets(device_, &alloc_info, &reproject_desc_set_);
            }
        }
    }

    // Allocate initial buffers
    allocate_buffers(10000);  // Initial capacity

    initialized_ = true;
    spdlog::info("OcclusionCuller initialized");
}

void OcclusionCuller::cleanup() {
    if (!device_) return;

    vkDeviceWaitIdle(device_);

    cleanup_hiz_pyramid_resources();
    cleanup_temporal_resources();

    if (hiz_sampler_) {
        vkDestroySampler(device_, hiz_sampler_, nullptr);
        hiz_sampler_ = VK_NULL_HANDLE;
    }

    if (hiz_build_pipeline_) {
        vkDestroyPipeline(device_, hiz_build_pipeline_, nullptr);
        hiz_build_pipeline_ = VK_NULL_HANDLE;
    }
    if (hiz_build_layout_) {
        vkDestroyPipelineLayout(device_, hiz_build_layout_, nullptr);
        hiz_build_layout_ = VK_NULL_HANDLE;
    }
    if (hiz_build_desc_layout_) {
        vkDestroyDescriptorSetLayout(device_, hiz_build_desc_layout_, nullptr);
        hiz_build_desc_layout_ = VK_NULL_HANDLE;
    }
    if (hiz_build_desc_pool_) {
        vkDestroyDescriptorPool(device_, hiz_build_desc_pool_, nullptr);
        hiz_build_desc_pool_ = VK_NULL_HANDLE;
    }

    if (occlusion_pipeline_) {
        vkDestroyPipeline(device_, occlusion_pipeline_, nullptr);
        occlusion_pipeline_ = VK_NULL_HANDLE;
    }
    if (occlusion_layout_) {
        vkDestroyPipelineLayout(device_, occlusion_layout_, nullptr);
        occlusion_layout_ = VK_NULL_HANDLE;
    }
    if (occlusion_desc_layout_) {
        vkDestroyDescriptorSetLayout(device_, occlusion_desc_layout_, nullptr);
        occlusion_desc_layout_ = VK_NULL_HANDLE;
    }
    if (occlusion_desc_pool_) {
        vkDestroyDescriptorPool(device_, occlusion_desc_pool_, nullptr);
        occlusion_desc_pool_ = VK_NULL_HANDLE;
    }

    // Depth copy pipeline
    if (depth_copy_pipeline_) {
        vkDestroyPipeline(device_, depth_copy_pipeline_, nullptr);
        depth_copy_pipeline_ = VK_NULL_HANDLE;
    }
    if (depth_copy_layout_) {
        vkDestroyPipelineLayout(device_, depth_copy_layout_, nullptr);
        depth_copy_layout_ = VK_NULL_HANDLE;
    }
    if (depth_copy_desc_layout_) {
        vkDestroyDescriptorSetLayout(device_, depth_copy_desc_layout_, nullptr);
        depth_copy_desc_layout_ = VK_NULL_HANDLE;
    }

    // Reproject pipeline
    if (reproject_pipeline_) {
        vkDestroyPipeline(device_, reproject_pipeline_, nullptr);
        reproject_pipeline_ = VK_NULL_HANDLE;
    }
    if (reproject_layout_) {
        vkDestroyPipelineLayout(device_, reproject_layout_, nullptr);
        reproject_layout_ = VK_NULL_HANDLE;
    }
    if (reproject_desc_layout_) {
        vkDestroyDescriptorSetLayout(device_, reproject_desc_layout_, nullptr);
        reproject_desc_layout_ = VK_NULL_HANDLE;
    }

    if (fence_) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }
    if (command_pool_) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }

    // GPU buffers are cleaned up by MemoryPool or their destructors
    bounds_staging_buffer_ = GPUBuffer{};
    frustum_visible_staging_buffer_ = GPUBuffer{};
    visible_indices_buffer_ = GPUBuffer{};
    atomic_counter_buffer_ = GPUBuffer{};

    initialized_ = false;
    device_ = VK_NULL_HANDLE;
}

void OcclusionCuller::create_hiz_build_pipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[2] = {};

    // Binding 0: Source texture (sampler)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Destination image (storage)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &hiz_build_desc_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z build descriptor set layout");
        return;
    }

    // Create pipeline layout with push constants
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(HiZBuildPushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &hiz_build_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &hiz_build_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z build pipeline layout");
        return;
    }

    // Load shader
    std::string shader_path = "shaders/compute/culling/hiz_build.comp.spv";
    std::ifstream file(shader_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::warn("Hi-Z build shader not found at {}, using CPU fallback", shader_path);
        return;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> shader_code(file_size);
    file.seekg(0);
    file.read(shader_code.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = shader_code.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &shader_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z build shader module");
        return;
    }

    // Create pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = hiz_build_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &hiz_build_pipeline_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z build pipeline");
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Hi-Z build pipeline created");
}

void OcclusionCuller::create_occlusion_pipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[5] = {};

    // Binding 0: Object bounds buffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Frustum visible indices
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Final visible indices (output)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 3: Atomic counter
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 4: Hi-Z pyramid texture
    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 5;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &occlusion_desc_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion descriptor set layout");
        return;
    }

    // Create pipeline layout with push constants
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(OcclusionPushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &occlusion_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &occlusion_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion pipeline layout");
        return;
    }

    // Create descriptor pool
    VkDescriptorPoolSize pool_sizes[2] = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[0].descriptorCount = 4;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &occlusion_desc_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion descriptor pool");
        return;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = occlusion_desc_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &occlusion_desc_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &occlusion_desc_set_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate occlusion descriptor set");
        return;
    }

    // Load shader
    std::string shader_path = "shaders/compute/culling/occlusion_cull.comp.spv";
    std::ifstream file(shader_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::warn("Occlusion cull shader not found at {}, using CPU fallback", shader_path);
        return;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> shader_code(file_size);
    file.seekg(0);
    file.read(shader_code.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = shader_code.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &shader_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion shader module");
        return;
    }

    // Create pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = occlusion_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &occlusion_pipeline_) != VK_SUCCESS) {
        spdlog::error("Failed to create occlusion pipeline");
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Occlusion pipeline created");
}

void OcclusionCuller::create_hiz_pyramid_resources(uint32_t width, uint32_t height) {
    // Cleanup existing resources
    cleanup_hiz_pyramid_resources();

    hiz_width_ = width;
    hiz_height_ = height;

    // Calculate mip levels
    current_mip_levels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    current_mip_levels_ = std::min(current_mip_levels_, config_.max_mip_levels);

    // Create Hi-Z image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R32_SFLOAT;
    image_info.extent = {width, height, 1};
    image_info.mipLevels = current_mip_levels_;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device_, &image_info, nullptr, &hiz_pyramid_image_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z pyramid image");
        return;
    }

    // Allocate memory
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_, hiz_pyramid_image_, &mem_reqs);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_props);

    uint32_t memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((mem_reqs.memoryTypeBits & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memory_type_index = i;
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = memory_type_index;

    if (vkAllocateMemory(device_, &alloc_info, nullptr, &hiz_pyramid_memory_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate Hi-Z pyramid memory");
        return;
    }

    vkBindImageMemory(device_, hiz_pyramid_image_, hiz_pyramid_memory_, 0);

    // Create image view for all mip levels
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = hiz_pyramid_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = current_mip_levels_;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &view_info, nullptr, &hiz_pyramid_view_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z pyramid image view");
        return;
    }

    // Create individual mip level views
    hiz_mip_views_.resize(current_mip_levels_);
    for (uint32_t i = 0; i < current_mip_levels_; i++) {
        view_info.subresourceRange.baseMipLevel = i;
        view_info.subresourceRange.levelCount = 1;

        if (vkCreateImageView(device_, &view_info, nullptr, &hiz_mip_views_[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create Hi-Z mip {} image view", i);
            return;
        }
    }

    // Create descriptor pool for Hi-Z build (one set per mip level)
    if (hiz_build_desc_pool_) {
        vkDestroyDescriptorPool(device_, hiz_build_desc_pool_, nullptr);
    }

    VkDescriptorPoolSize pool_sizes[2] = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount = current_mip_levels_;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_sizes[1].descriptorCount = current_mip_levels_;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = current_mip_levels_;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &hiz_build_desc_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create Hi-Z build descriptor pool");
        return;
    }

    // Allocate descriptor sets
    hiz_build_desc_sets_.resize(current_mip_levels_);
    std::vector<VkDescriptorSetLayout> layouts(current_mip_levels_, hiz_build_desc_layout_);

    VkDescriptorSetAllocateInfo desc_alloc{};
    desc_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_alloc.descriptorPool = hiz_build_desc_pool_;
    desc_alloc.descriptorSetCount = current_mip_levels_;
    desc_alloc.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device_, &desc_alloc, hiz_build_desc_sets_.data()) != VK_SUCCESS) {
        spdlog::error("Failed to allocate Hi-Z build descriptor sets");
        return;
    }

    spdlog::debug("Created Hi-Z pyramid: {}x{}, {} mip levels", width, height, current_mip_levels_);
}

void OcclusionCuller::cleanup_hiz_pyramid_resources() {
    if (!device_) return;

    for (auto view : hiz_mip_views_) {
        if (view) vkDestroyImageView(device_, view, nullptr);
    }
    hiz_mip_views_.clear();

    if (hiz_pyramid_view_) {
        vkDestroyImageView(device_, hiz_pyramid_view_, nullptr);
        hiz_pyramid_view_ = VK_NULL_HANDLE;
    }
    if (hiz_pyramid_image_) {
        vkDestroyImage(device_, hiz_pyramid_image_, nullptr);
        hiz_pyramid_image_ = VK_NULL_HANDLE;
    }
    if (hiz_pyramid_memory_) {
        vkFreeMemory(device_, hiz_pyramid_memory_, nullptr);
        hiz_pyramid_memory_ = VK_NULL_HANDLE;
    }

    hiz_width_ = 0;
    hiz_height_ = 0;
    current_mip_levels_ = 0;
}

void OcclusionCuller::allocate_buffers(uint32_t max_objects) {
    if (!memory_pool_) return;

    max_objects_ = max_objects;

    // Visible indices buffer
    visible_indices_buffer_ = memory_pool_->allocate_buffer(
        max_objects * sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Static
    );

    // Atomic counter buffer
    atomic_counter_buffer_ = memory_pool_->allocate_buffer(
        sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    spdlog::debug("OcclusionCuller buffers allocated for {} objects", max_objects);
}

void OcclusionCuller::build_hiz_pyramid(
    VkImage depth_image,
    VkImageView depth_image_view,
    uint32_t width,
    uint32_t height,
    VkCommandBuffer cmd_buffer
) {
    if (!initialized_ || !hiz_build_pipeline_) {
        spdlog::warn("OcclusionCuller not initialized or Hi-Z pipeline not available");
        return;
    }

    // Recreate pyramid if size changed
    if (width != hiz_width_ || height != hiz_height_) {
        create_hiz_pyramid_resources(width, height);
    }

    bool own_cmd_buffer = (cmd_buffer == VK_NULL_HANDLE);
    if (own_cmd_buffer) {
        begin_commands();
        cmd_buffer = command_buffer_;
    }

    // Transition depth image for reading (if needed - assume it's already in right state)
    // Transition Hi-Z mip 0 to transfer dst
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = hiz_pyramid_image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy depth buffer to mip level 0
    // Note: This requires a blit if formats differ, or manual copy
    // For simplicity, we'll use a compute shader to copy from depth
    // In production, you'd want to blit or use a proper depth-to-R32F conversion

    // For now, transition mip 0 to general for shader write
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

    vkCmdPipelineBarrier(cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Build mip chain
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, hiz_build_pipeline_);

    uint32_t src_width = width;
    uint32_t src_height = height;

    for (uint32_t mip = 1; mip < current_mip_levels_; mip++) {
        uint32_t dst_width = std::max(1u, src_width / 2);
        uint32_t dst_height = std::max(1u, src_height / 2);

        // Update descriptor set for this mip level
        VkDescriptorImageInfo src_info{};
        src_info.sampler = hiz_sampler_;
        src_info.imageView = hiz_mip_views_[mip - 1];
        src_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo dst_info{};
        dst_info.imageView = hiz_mip_views_[mip];
        dst_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = hiz_build_desc_sets_[mip - 1];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &src_info;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = hiz_build_desc_sets_[mip - 1];
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageInfo = &dst_info;

        vkUpdateDescriptorSets(device_, 2, writes, 0, nullptr);

        // Transition source mip to shader read
        if (mip > 1) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.subresourceRange.baseMipLevel = mip - 1;

            vkCmdPipelineBarrier(cmd_buffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        // Transition dest mip to general
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.subresourceRange.baseMipLevel = mip;

        vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Bind descriptor set and push constants
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            hiz_build_layout_, 0, 1, &hiz_build_desc_sets_[mip - 1], 0, nullptr);

        HiZBuildPushConstants pc{};
        pc.src_width = static_cast<int32_t>(src_width);
        pc.src_height = static_cast<int32_t>(src_height);
        pc.dst_width = static_cast<int32_t>(dst_width);
        pc.dst_height = static_cast<int32_t>(dst_height);
        pc.mip_level = mip;

        vkCmdPushConstants(cmd_buffer, hiz_build_layout_,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

        // Dispatch
        uint32_t group_x = (dst_width + 15) / 16;
        uint32_t group_y = (dst_height + 15) / 16;
        vkCmdDispatch(cmd_buffer, group_x, group_y, 1);

        src_width = dst_width;
        src_height = dst_height;
    }

    // Transition all mip levels to shader read for occlusion testing
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = current_mip_levels_;

    vkCmdPipelineBarrier(cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    if (own_cmd_buffer) {
        end_commands();
        submit_and_wait();
    }

    stats_.hiz_mip_levels = current_mip_levels_;
}

std::vector<uint32_t> OcclusionCuller::cull(
    const std::vector<ObjectBounds>& bounds,
    const std::vector<uint32_t>& frustum_visible_indices,
    const math::Mat4& view_proj,
    uint32_t screen_width,
    uint32_t screen_height,
    VkCommandBuffer cmd_buffer
) {
    if (frustum_visible_indices.empty()) {
        return {};
    }

    // If GPU pipeline not available, use CPU fallback
    if (!occlusion_pipeline_ || !hiz_pyramid_view_) {
        spdlog::debug("Using CPU occlusion culling fallback");
        // For now, just pass through frustum visible (no occlusion)
        return frustum_visible_indices;
    }

    // Upload bounds and indices
    // ... (GPU path would upload to staging buffers and dispatch)

    // For now, return frustum visible indices
    // Full GPU implementation would dispatch compute shader
    return frustum_visible_indices;
}

void OcclusionCuller::cull_gpu(
    const GPUBuffer& bounds_buffer,
    const GPUBuffer& frustum_visible_buffer,
    uint32_t num_frustum_visible,
    const math::Mat4& view_proj,
    uint32_t screen_width,
    uint32_t screen_height,
    VkCommandBuffer cmd_buffer
) {
    if (!occlusion_pipeline_ || !hiz_pyramid_view_ || num_frustum_visible == 0) {
        return;
    }

    // Reset atomic counter
    uint32_t zero = 0;
    void* ptr = atomic_counter_buffer_.map();
    if (ptr) {
        memcpy(ptr, &zero, sizeof(uint32_t));
        atomic_counter_buffer_.unmap();
    }

    // Update descriptor set
    VkDescriptorBufferInfo bounds_info{};
    bounds_info.buffer = bounds_buffer.get_buffer();
    bounds_info.offset = 0;
    bounds_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo frustum_visible_info{};
    frustum_visible_info.buffer = frustum_visible_buffer.get_buffer();
    frustum_visible_info.offset = 0;
    frustum_visible_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo visible_info{};
    visible_info.buffer = visible_indices_buffer_.get_buffer();
    visible_info.offset = 0;
    visible_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo counter_info{};
    counter_info.buffer = atomic_counter_buffer_.get_buffer();
    counter_info.offset = 0;
    counter_info.range = sizeof(uint32_t);

    VkDescriptorImageInfo hiz_info{};
    hiz_info.sampler = hiz_sampler_;
    hiz_info.imageView = hiz_pyramid_view_;
    hiz_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writes[5] = {};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = occlusion_desc_set_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &bounds_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = occlusion_desc_set_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &frustum_visible_info;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = occlusion_desc_set_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &visible_info;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = occlusion_desc_set_;
    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &counter_info;

    writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[4].dstSet = occlusion_desc_set_;
    writes[4].dstBinding = 4;
    writes[4].descriptorCount = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[4].pImageInfo = &hiz_info;

    vkUpdateDescriptorSets(device_, 5, writes, 0, nullptr);

    // Bind pipeline
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, occlusion_pipeline_);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
        occlusion_layout_, 0, 1, &occlusion_desc_set_, 0, nullptr);

    // Push constants
    OcclusionPushConstants pc{};
    pc.view_proj = view_proj;
    pc.screen_size[0] = static_cast<float>(screen_width);
    pc.screen_size[1] = static_cast<float>(screen_height);
    pc.num_frustum_visible = num_frustum_visible;
    pc.hiz_mip_levels = current_mip_levels_;

    vkCmdPushConstants(cmd_buffer, occlusion_layout_,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

    // Dispatch
    uint32_t num_groups = (num_frustum_visible + 255) / 256;
    vkCmdDispatch(cmd_buffer, num_groups, 1, 1);

    // Memory barrier for results
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr);

    stats_.objects_tested = num_frustum_visible;
}

uint32_t OcclusionCuller::get_visible_count() {
    if (!atomic_counter_buffer_.get_buffer()) return 0;

    void* ptr = atomic_counter_buffer_.map();
    if (!ptr) return 0;

    uint32_t count;
    memcpy(&count, ptr, sizeof(uint32_t));
    atomic_counter_buffer_.unmap();

    stats_.objects_occluded = stats_.objects_tested - count;
    return count;
}

std::vector<uint32_t> OcclusionCuller::download_visible_indices() {
    uint32_t count = get_visible_count();
    if (count == 0) return {};

    std::vector<uint32_t> result(count);

    // Would need staging buffer for device-local memory
    // For now, assume buffer is host-visible or add staging

    return result;
}

void OcclusionCuller::begin_commands() {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer_, &begin_info);
}

void OcclusionCuller::end_commands() {
    vkEndCommandBuffer(command_buffer_);
}

void OcclusionCuller::submit_and_wait() {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    vkQueueSubmit(compute_queue_, 1, &submit_info, fence_);
    vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &fence_);
    vkResetCommandBuffer(command_buffer_, 0);
}

std::vector<uint32_t> OcclusionCuller::cull_cpu(
    const std::vector<ObjectBounds>& bounds,
    const std::vector<uint32_t>& frustum_visible_indices,
    const math::Mat4& view_proj,
    const std::vector<float>& depth_buffer,
    uint32_t width,
    uint32_t height
) {
    std::vector<uint32_t> result;
    result.reserve(frustum_visible_indices.size());

    for (uint32_t idx : frustum_visible_indices) {
        if (idx >= bounds.size()) continue;

        const auto& obj = bounds[idx];

        // Project AABB corners to screen
        math::Vec3 corners[8] = {
            {obj.aabb_min.x, obj.aabb_min.y, obj.aabb_min.z},
            {obj.aabb_max.x, obj.aabb_min.y, obj.aabb_min.z},
            {obj.aabb_min.x, obj.aabb_max.y, obj.aabb_min.z},
            {obj.aabb_max.x, obj.aabb_max.y, obj.aabb_min.z},
            {obj.aabb_min.x, obj.aabb_min.y, obj.aabb_max.z},
            {obj.aabb_max.x, obj.aabb_min.y, obj.aabb_max.z},
            {obj.aabb_min.x, obj.aabb_max.y, obj.aabb_max.z},
            {obj.aabb_max.x, obj.aabb_max.y, obj.aabb_max.z}
        };

        float min_x = 1e10f, min_y = 1e10f, max_x = -1e10f, max_y = -1e10f;
        float nearest_depth = 1.0f;
        bool any_visible = false;

        for (int i = 0; i < 8; i++) {
            math::Vec4 clip = view_proj * math::Vec4(corners[i], 1.0f);
            if (clip.w <= 0.0f) continue;

            math::Vec3 ndc = math::Vec3(clip) / clip.w;
            if (ndc.z < 0.0f || ndc.z > 1.0f) continue;

            any_visible = true;
            float screen_x = (ndc.x * 0.5f + 0.5f) * width;
            float screen_y = (ndc.y * 0.5f + 0.5f) * height;

            min_x = std::min(min_x, screen_x);
            min_y = std::min(min_y, screen_y);
            max_x = std::max(max_x, screen_x);
            max_y = std::max(max_y, screen_y);
            nearest_depth = std::min(nearest_depth, ndc.z);
        }

        if (!any_visible) {
            result.push_back(idx);  // Conservatively visible
            continue;
        }

        // Sample depth buffer at object's screen bounds
        int ix_min = std::max(0, static_cast<int>(min_x));
        int iy_min = std::max(0, static_cast<int>(min_y));
        int ix_max = std::min(static_cast<int>(width) - 1, static_cast<int>(max_x));
        int iy_max = std::min(static_cast<int>(height) - 1, static_cast<int>(max_y));

        float max_depth = 0.0f;
        for (int y = iy_min; y <= iy_max; y++) {
            for (int x = ix_min; x <= ix_max; x++) {
                max_depth = std::max(max_depth, depth_buffer[y * width + x]);
            }
        }

        // Object is visible if nearest point is closer than depth buffer
        if (nearest_depth <= max_depth) {
            result.push_back(idx);
        }
    }

    return result;
}

void OcclusionCuller::create_depth_copy_pipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[2] = {};

    // Binding 0: Source depth texture (sampler)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Destination Hi-Z image (storage)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &depth_copy_desc_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create depth copy descriptor set layout");
        return;
    }

    // Create pipeline layout with push constants
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(DepthCopyPushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &depth_copy_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &depth_copy_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create depth copy pipeline layout");
        return;
    }

    // Load shader
    std::string shader_path = "shaders/compute/culling/depth_to_hiz.comp.spv";
    std::ifstream file(shader_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::warn("Depth copy shader not found at {}", shader_path);
        return;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> shader_code(file_size);
    file.seekg(0);
    file.read(shader_code.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = shader_code.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &shader_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::error("Failed to create depth copy shader module");
        return;
    }

    // Create pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = depth_copy_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &depth_copy_pipeline_) != VK_SUCCESS) {
        spdlog::error("Failed to create depth copy pipeline");
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Depth copy pipeline created");
}

void OcclusionCuller::create_reproject_pipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[3] = {};

    // Binding 0: Previous frame depth (sampler)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Current frame depth (sampler)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Output reprojected depth (storage)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 3;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &reproject_desc_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create reproject descriptor set layout");
        return;
    }

    // Create pipeline layout with push constants
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(ReprojectPushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &reproject_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &reproject_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create reproject pipeline layout");
        return;
    }

    // Load shader
    std::string shader_path = "shaders/compute/culling/depth_reproject.comp.spv";
    std::ifstream file(shader_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::warn("Reproject shader not found at {}", shader_path);
        return;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> shader_code(file_size);
    file.seekg(0);
    file.read(shader_code.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = shader_code.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &shader_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::error("Failed to create reproject shader module");
        return;
    }

    // Create pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = reproject_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &reproject_pipeline_) != VK_SUCCESS) {
        spdlog::error("Failed to create reproject pipeline");
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Temporal reprojection pipeline created");
}

void OcclusionCuller::create_temporal_resources(uint32_t width, uint32_t height) {
    cleanup_temporal_resources();

    // Create previous frame depth image (R32_SFLOAT for Hi-Z compatibility)
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R32_SFLOAT;
    image_info.extent = {width, height, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device_, &image_info, nullptr, &prev_depth_image_) != VK_SUCCESS) {
        spdlog::error("Failed to create previous depth image");
        return;
    }

    // Allocate memory
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_, prev_depth_image_, &mem_reqs);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_props);

    uint32_t memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((mem_reqs.memoryTypeBits & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memory_type_index = i;
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = memory_type_index;

    if (vkAllocateMemory(device_, &alloc_info, nullptr, &prev_depth_memory_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate previous depth memory");
        return;
    }

    vkBindImageMemory(device_, prev_depth_image_, prev_depth_memory_, 0);

    // Create image view
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = prev_depth_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &view_info, nullptr, &prev_depth_view_) != VK_SUCCESS) {
        spdlog::error("Failed to create previous depth image view");
        return;
    }

    has_previous_frame_ = false;

    spdlog::debug("Temporal depth resources created: {}x{}", width, height);
}

void OcclusionCuller::cleanup_temporal_resources() {
    if (!device_) return;

    if (prev_depth_view_) {
        vkDestroyImageView(device_, prev_depth_view_, nullptr);
        prev_depth_view_ = VK_NULL_HANDLE;
    }
    if (prev_depth_image_) {
        vkDestroyImage(device_, prev_depth_image_, nullptr);
        prev_depth_image_ = VK_NULL_HANDLE;
    }
    if (prev_depth_memory_) {
        vkFreeMemory(device_, prev_depth_memory_, nullptr);
        prev_depth_memory_ = VK_NULL_HANDLE;
    }

    has_previous_frame_ = false;
}

void OcclusionCuller::copy_depth_to_hiz(VkCommandBuffer cmd, VkImageView depth_view) {
    if (!depth_copy_pipeline_ || hiz_mip_views_.empty()) {
        spdlog::warn("Depth copy pipeline or Hi-Z views not available");
        return;
    }

    // Transition Hi-Z mip 0 to general for writing
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = hiz_pyramid_image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update descriptor set
    VkDescriptorImageInfo src_info{};
    src_info.sampler = hiz_sampler_;
    src_info.imageView = depth_view;
    src_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo dst_info{};
    dst_info.imageView = hiz_mip_views_[0];
    dst_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = depth_copy_desc_set_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &src_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = depth_copy_desc_set_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].pImageInfo = &dst_info;

    vkUpdateDescriptorSets(device_, 2, writes, 0, nullptr);

    // Bind pipeline and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, depth_copy_pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        depth_copy_layout_, 0, 1, &depth_copy_desc_set_, 0, nullptr);

    DepthCopyPushConstants pc{};
    pc.width = static_cast<int32_t>(hiz_width_);
    pc.height = static_cast<int32_t>(hiz_height_);

    vkCmdPushConstants(cmd, depth_copy_layout_,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

    uint32_t group_x = (hiz_width_ + 15) / 16;
    uint32_t group_y = (hiz_height_ + 15) / 16;
    vkCmdDispatch(cmd, group_x, group_y, 1);

    // Transition mip 0 to shader read for mipmap generation
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void OcclusionCuller::reproject_temporal_depth(VkCommandBuffer cmd, VkImageView curr_depth_view) {
    if (!reproject_pipeline_ || !prev_depth_view_ || !has_previous_frame_) {
        // No previous frame, skip reprojection
        return;
    }

    // Update descriptor set for reprojection
    VkDescriptorImageInfo prev_info{};
    prev_info.sampler = hiz_sampler_;
    prev_info.imageView = prev_depth_view_;
    prev_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo curr_info{};
    curr_info.sampler = hiz_sampler_;
    curr_info.imageView = curr_depth_view;
    curr_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo out_info{};
    out_info.imageView = hiz_mip_views_[0];
    out_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[3] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = reproject_desc_set_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &prev_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = reproject_desc_set_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &curr_info;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = reproject_desc_set_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].pImageInfo = &out_info;

    vkUpdateDescriptorSets(device_, 3, writes, 0, nullptr);

    // Bind pipeline and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, reproject_pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        reproject_layout_, 0, 1, &reproject_desc_set_, 0, nullptr);

    ReprojectPushConstants pc{};
    pc.prev_view_proj_inv = prev_view_proj_inv_;
    pc.curr_view_proj = prev_view_proj_;  // Will be updated in build_hiz_pyramid_temporal
    pc.width = static_cast<int32_t>(hiz_width_);
    pc.height = static_cast<int32_t>(hiz_height_);
    pc.depth_threshold = config_.depth_threshold;
    pc.use_current_fallback = 1;

    vkCmdPushConstants(cmd, reproject_layout_,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

    uint32_t group_x = (hiz_width_ + 15) / 16;
    uint32_t group_y = (hiz_height_ + 15) / 16;
    vkCmdDispatch(cmd, group_x, group_y, 1);
}

void OcclusionCuller::build_hiz_pyramid_temporal(
    VkImage depth_image,
    VkImageView depth_image_view,
    uint32_t width,
    uint32_t height,
    const math::Mat4& curr_view_proj,
    VkCommandBuffer cmd_buffer
) {
    if (!initialized_) {
        spdlog::warn("OcclusionCuller not initialized");
        return;
    }

    // Recreate resources if size changed
    if (width != hiz_width_ || height != hiz_height_) {
        create_hiz_pyramid_resources(width, height);
        create_temporal_resources(width, height);
    }

    bool own_cmd_buffer = (cmd_buffer == VK_NULL_HANDLE);
    if (own_cmd_buffer) {
        begin_commands();
        cmd_buffer = command_buffer_;
    }

    // Step 1: Copy or reproject depth to Hi-Z mip 0
    if (config_.use_temporal_reprojection && has_previous_frame_ && reproject_pipeline_) {
        // Use temporal reprojection
        reproject_temporal_depth(cmd_buffer, depth_image_view);
    } else if (depth_copy_pipeline_) {
        // Direct copy from depth buffer
        copy_depth_to_hiz(cmd_buffer, depth_image_view);
    } else {
        spdlog::warn("No depth copy or reproject pipeline available");
        if (own_cmd_buffer) {
            end_commands();
        }
        return;
    }

    // Step 2: Build mipmap chain (same as existing build_hiz_pyramid but starting from mip 1)
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, hiz_build_pipeline_);

    uint32_t src_width = width;
    uint32_t src_height = height;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = hiz_pyramid_image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    for (uint32_t mip = 1; mip < current_mip_levels_; mip++) {
        uint32_t dst_width = std::max(1u, src_width / 2);
        uint32_t dst_height = std::max(1u, src_height / 2);

        // Update descriptor set for this mip level
        VkDescriptorImageInfo src_info{};
        src_info.sampler = hiz_sampler_;
        src_info.imageView = hiz_mip_views_[mip - 1];
        src_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo dst_info{};
        dst_info.imageView = hiz_mip_views_[mip];
        dst_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = hiz_build_desc_sets_[mip - 1];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &src_info;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = hiz_build_desc_sets_[mip - 1];
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageInfo = &dst_info;

        vkUpdateDescriptorSets(device_, 2, writes, 0, nullptr);

        // Transition dest mip to general
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.subresourceRange.baseMipLevel = mip;
        barrier.subresourceRange.levelCount = 1;

        vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Bind and dispatch
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            hiz_build_layout_, 0, 1, &hiz_build_desc_sets_[mip - 1], 0, nullptr);

        HiZBuildPushConstants pc{};
        pc.src_width = static_cast<int32_t>(src_width);
        pc.src_height = static_cast<int32_t>(src_height);
        pc.dst_width = static_cast<int32_t>(dst_width);
        pc.dst_height = static_cast<int32_t>(dst_height);
        pc.mip_level = mip;

        vkCmdPushConstants(cmd_buffer, hiz_build_layout_,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

        uint32_t group_x = (dst_width + 15) / 16;
        uint32_t group_y = (dst_height + 15) / 16;
        vkCmdDispatch(cmd_buffer, group_x, group_y, 1);

        // Transition this mip to shader read for next iteration
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        src_width = dst_width;
        src_height = dst_height;
    }

    // Store current view-proj for next frame
    prev_view_proj_ = curr_view_proj;
    prev_view_proj_inv_ = glm::inverse(curr_view_proj);

    if (own_cmd_buffer) {
        end_commands();
        submit_and_wait();
    }

    stats_.hiz_mip_levels = current_mip_levels_;
}

void OcclusionCuller::end_frame() {
    if (!initialized_ || hiz_pyramid_image_ == VK_NULL_HANDLE) {
        return;
    }

    // Copy current Hi-Z mip 0 to previous depth for next frame
    // This is done via a command buffer copy
    begin_commands();

    // Transition Hi-Z mip 0 to transfer src
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = hiz_pyramid_image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Transition previous depth to transfer dst
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = has_previous_frame_ ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = prev_depth_image_;

    vkCmdPipelineBarrier(command_buffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy Hi-Z mip 0 to previous depth
    VkImageCopy copy_region{};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcSubresource.baseArrayLayer = 0;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource = copy_region.srcSubresource;
    copy_region.extent = {hiz_width_, hiz_height_, 1};

    vkCmdCopyImage(command_buffer_,
        hiz_pyramid_image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        prev_depth_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region);

    // Transition both back to shader read
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.image = hiz_pyramid_image_;

    vkCmdPipelineBarrier(command_buffer_,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = prev_depth_image_;

    vkCmdPipelineBarrier(command_buffer_,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    end_commands();
    submit_and_wait();

    has_previous_frame_ = true;
}

} // namespace culling
} // namespace manim
