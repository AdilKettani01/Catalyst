#include "manim/culling/contribution_culler.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace manim {
namespace culling {

// Shader search paths
static const std::vector<std::string> SHADER_SEARCH_PATHS = {
    "./build/shaders",
    "../build/shaders",
    "../../build/shaders",
    "./shaders",
    "/usr/share/manim/shaders",
    "/usr/local/share/manim/shaders"
};

static std::vector<uint32_t> load_spirv_file(const std::string& filename) {
    std::string full_path;

    for (const auto& search_path : SHADER_SEARCH_PATHS) {
        std::string candidate = search_path + "/" + filename;
        if (std::filesystem::exists(candidate)) {
            full_path = candidate;
            break;
        }
    }

    if (full_path.empty()) {
        spdlog::error("Could not find shader: {}", filename);
        return {};
    }

    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::error("Failed to open shader file: {}", full_path);
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> spirv(file_size / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(spirv.data()), file_size);

    spdlog::debug("Loaded shader: {} ({} bytes)", full_path, file_size);
    return spirv;
}

ContributionCuller::ContributionCuller() = default;

ContributionCuller::~ContributionCuller() {
    cleanup();
}

ContributionCuller::ContributionCuller(ContributionCuller&& other) noexcept
    : config_(other.config_)
    , initialized_(other.initialized_)
    , viewport_width_(other.viewport_width_)
    , viewport_height_(other.viewport_height_)
    , device_(other.device_)
    , physical_device_(other.physical_device_)
    , memory_pool_(other.memory_pool_)
    , queue_family_(other.queue_family_)
    , pipeline_(other.pipeline_)
    , pipeline_layout_(other.pipeline_layout_)
    , desc_set_layout_(other.desc_set_layout_)
    , desc_pool_(other.desc_pool_)
    , desc_set_(other.desc_set_)
    , bounds_buffer_(std::move(other.bounds_buffer_))
    , frustum_visible_buffer_(std::move(other.frustum_visible_buffer_))
    , visible_indices_buffer_(std::move(other.visible_indices_buffer_))
    , atomic_counter_buffer_(std::move(other.atomic_counter_buffer_))
    , object_count_(other.object_count_)
    , using_external_bounds_(other.using_external_bounds_)
    , stats_(other.stats_)
{
    other.device_ = VK_NULL_HANDLE;
    other.initialized_ = false;
}

ContributionCuller& ContributionCuller::operator=(ContributionCuller&& other) noexcept {
    if (this != &other) {
        cleanup();

        config_ = other.config_;
        initialized_ = other.initialized_;
        viewport_width_ = other.viewport_width_;
        viewport_height_ = other.viewport_height_;
        device_ = other.device_;
        physical_device_ = other.physical_device_;
        memory_pool_ = other.memory_pool_;
        queue_family_ = other.queue_family_;
        pipeline_ = other.pipeline_;
        pipeline_layout_ = other.pipeline_layout_;
        desc_set_layout_ = other.desc_set_layout_;
        desc_pool_ = other.desc_pool_;
        desc_set_ = other.desc_set_;
        bounds_buffer_ = std::move(other.bounds_buffer_);
        frustum_visible_buffer_ = std::move(other.frustum_visible_buffer_);
        visible_indices_buffer_ = std::move(other.visible_indices_buffer_);
        atomic_counter_buffer_ = std::move(other.atomic_counter_buffer_);
        object_count_ = other.object_count_;
        using_external_bounds_ = other.using_external_bounds_;
        stats_ = other.stats_;

        other.device_ = VK_NULL_HANDLE;
        other.initialized_ = false;
    }
    return *this;
}

void ContributionCuller::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t queue_family
) {
    if (initialized_) {
        spdlog::warn("ContributionCuller already initialized");
        return;
    }

    device_ = device;
    physical_device_ = physical_device;
    memory_pool_ = &memory_pool;
    queue_family_ = queue_family;

    // Create descriptor resources
    create_descriptor_set();

    // Create pipeline
    create_pipeline();

    // Allocate initial buffers
    allocate_buffers(1024);

    initialized_ = true;
    spdlog::info("ContributionCuller initialized successfully");
}

void ContributionCuller::cleanup() {
    if (device_ == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device_);

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }

    if (desc_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, desc_pool_, nullptr);
        desc_pool_ = VK_NULL_HANDLE;
    }

    if (desc_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, desc_set_layout_, nullptr);
        desc_set_layout_ = VK_NULL_HANDLE;
    }

    device_ = VK_NULL_HANDLE;
    initialized_ = false;
}

void ContributionCuller::create_descriptor_set() {
    // Descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    // Binding 0: Object bounds (storage buffer, read-only)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Frustum visible indices (storage buffer, read-only)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Output visible indices (storage buffer, write)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 3: Atomic counter (storage buffer, read-write)
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &desc_set_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor set layout for contribution culler");
        return;
    }

    // Descriptor pool
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = 4;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &desc_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor pool for contribution culler");
        return;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = desc_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &desc_set_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &desc_set_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate descriptor set for contribution culler");
        return;
    }
}

void ContributionCuller::create_pipeline() {
    // Load shader
    auto spirv = load_spirv_file("compute/culling/contribution_cull.comp.spv");
    if (spirv.empty()) {
        spdlog::error("Failed to load compute/culling/contribution_cull.comp.spv");
        return;
    }

    // Create shader module
    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module for contribution culler");
        return;
    }

    // Push constant range
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(ContributionCullPushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &desc_set_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        spdlog::error("Failed to create pipeline layout for contribution culler");
        return;
    }

    // Compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = pipeline_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        spdlog::error("Failed to create compute pipeline for contribution culler");
        return;
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);
    spdlog::info("Contribution culling pipeline created successfully");
}

void ContributionCuller::allocate_buffers(uint32_t max_objects) {
    if (!memory_pool_) return;

    // Object bounds buffer
    VkDeviceSize bounds_size = max_objects * sizeof(ObjectBounds);
    bounds_buffer_ = memory_pool_->allocate_buffer(
        bounds_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Frustum visible indices buffer (input from frustum cull)
    VkDeviceSize indices_size = max_objects * sizeof(uint32_t);
    frustum_visible_buffer_ = memory_pool_->allocate_buffer(
        indices_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Output visible indices buffer
    visible_indices_buffer_ = memory_pool_->allocate_buffer(
        indices_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Atomic counter buffer
    atomic_counter_buffer_ = memory_pool_->allocate_buffer(
        sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Update descriptor set with new buffers
    update_descriptor_set();

    spdlog::debug("Allocated contribution culling buffers for {} objects", max_objects);
}

void ContributionCuller::update_descriptor_set() {
    if (!bounds_buffer_.get_buffer() || !frustum_visible_buffer_.get_buffer() ||
        !visible_indices_buffer_.get_buffer() || !atomic_counter_buffer_.get_buffer()) {
        return;
    }

    std::array<VkDescriptorBufferInfo, 4> buffer_infos{};

    buffer_infos[0].buffer = bounds_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = frustum_visible_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    buffer_infos[2].buffer = visible_indices_buffer_.get_buffer();
    buffer_infos[2].offset = 0;
    buffer_infos[2].range = VK_WHOLE_SIZE;

    buffer_infos[3].buffer = atomic_counter_buffer_.get_buffer();
    buffer_infos[3].offset = 0;
    buffer_infos[3].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = desc_set_;
        writes[i].dstBinding = i;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void ContributionCuller::set_viewport_size(uint32_t width, uint32_t height) {
    viewport_width_ = width;
    viewport_height_ = height;
}

void ContributionCuller::update_bounds(const std::vector<ObjectBounds>& bounds) {
    if (bounds.empty()) {
        object_count_ = 0;
        return;
    }

    object_count_ = static_cast<uint32_t>(bounds.size());
    using_external_bounds_ = false;

    // Upload bounds to GPU
    void* ptr = bounds_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, bounds.data(), bounds.size() * sizeof(ObjectBounds));
        bounds_buffer_.unmap();
    } else {
        spdlog::warn("Failed to map bounds buffer for upload");
    }
}

void ContributionCuller::set_bounds_buffer(const GPUBuffer& buffer, uint32_t object_count) {
    object_count_ = object_count;
    using_external_bounds_ = true;

    // Update descriptor set to point to external buffer
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer.get_buffer();
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = desc_set_;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void ContributionCuller::cull(
    const math::Mat4& view_proj,
    uint32_t frustum_visible_count,
    VkCommandBuffer cmd_buffer
) {
    if (!initialized_ || frustum_visible_count == 0) return;

    stats_.input_count = frustum_visible_count;

    // Clear atomic counter
    vkCmdFillBuffer(cmd_buffer, atomic_counter_buffer_.get_buffer(), 0, sizeof(uint32_t), 0);

    // Memory barrier after clear
    VkMemoryBarrier clear_barrier{};
    clear_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    clear_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    clear_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &clear_barrier, 0, nullptr, 0, nullptr
    );

    // Bind pipeline and descriptor set
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           pipeline_layout_, 0, 1, &desc_set_, 0, nullptr);

    // Push constants
    ContributionCullPushConstants constants{};
    constants.view_proj = view_proj;
    constants.screen_width = static_cast<float>(viewport_width_);
    constants.screen_height = static_cast<float>(viewport_height_);
    constants.min_screen_size = config_.min_screen_size;
    constants.max_screen_distance = config_.max_screen_distance;
    constants.num_frustum_visible = frustum_visible_count;
    constants.flags = config_.use_distance_scaling ? 0x01 : 0x00;

    vkCmdPushConstants(cmd_buffer, pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                      0, sizeof(constants), &constants);

    // Dispatch
    uint32_t workgroup_size = 256;
    uint32_t num_workgroups = (frustum_visible_count + workgroup_size - 1) / workgroup_size;
    vkCmdDispatch(cmd_buffer, num_workgroups, 1, 1);

    // Memory barrier for output
    VkMemoryBarrier compute_barrier{};
    compute_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    compute_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    compute_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
        0, 1, &compute_barrier, 0, nullptr, 0, nullptr
    );
}

std::vector<uint32_t> ContributionCuller::get_visible_indices() {
    std::vector<uint32_t> result;

    uint32_t count = get_visible_count();
    if (count == 0) return result;

    result.resize(count);

    void* ptr = visible_indices_buffer_.map();
    if (ptr) {
        std::memcpy(result.data(), ptr, count * sizeof(uint32_t));
        visible_indices_buffer_.unmap();
    }

    return result;
}

uint32_t ContributionCuller::get_visible_count() {
    void* ptr = atomic_counter_buffer_.map();
    if (!ptr) return 0;

    uint32_t count = *static_cast<uint32_t*>(ptr);
    atomic_counter_buffer_.unmap();

    stats_.output_count = count;
    return count;
}

std::vector<uint32_t> ContributionCuller::cull_cpu(
    const math::Mat4& view_proj,
    const std::vector<ObjectBounds>& bounds,
    const std::vector<uint32_t>& frustum_visible
) {
    std::vector<uint32_t> visible;
    visible.reserve(frustum_visible.size());

    float screen_height = static_cast<float>(viewport_height_);

    for (uint32_t idx : frustum_visible) {
        if (idx >= bounds.size()) continue;

        const auto& obj = bounds[idx];
        math::Vec3 center(obj.sphere.x, obj.sphere.y, obj.sphere.z);
        float radius = obj.sphere.w;

        // Transform center to clip space
        math::Vec4 clip = view_proj * math::Vec4(center, 1.0f);

        // Object behind camera - keep visible
        if (clip.w <= 0.0f) {
            visible.push_back(idx);
            continue;
        }

        // Calculate approximate screen size (diameter)
        float inv_w = 1.0f / clip.w;
        float screen_radius = (radius * inv_w) * screen_height * 0.5f;
        float screen_size = screen_radius * 2.0f;

        // Calculate adaptive threshold if enabled
        float threshold = config_.min_screen_size;
        if (config_.use_distance_scaling) {
            float distance = clip.w;
            float distance_factor = std::clamp(distance / config_.max_screen_distance, 0.1f, 1.0f);
            threshold = config_.min_screen_size * distance_factor;
        }

        if (screen_size >= threshold) {
            visible.push_back(idx);
        }
    }

    return visible;
}

} // namespace culling
} // namespace manim
