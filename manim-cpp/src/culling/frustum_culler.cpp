#include "manim/culling/frustum_culler.hpp"
#include "manim/mobject/mobject.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace manim {
namespace culling {

// Shader search paths (same as compute_engine.cpp)
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

FrustumCuller::FrustumCuller() = default;

FrustumCuller::~FrustumCuller() {
    cleanup();
}

FrustumCuller::FrustumCuller(FrustumCuller&& other) noexcept
    : device_(other.device_)
    , physical_device_(other.physical_device_)
    , memory_pool_(other.memory_pool_)
    , pipeline_(other.pipeline_)
    , pipeline_layout_(other.pipeline_layout_)
    , descriptor_layout_(other.descriptor_layout_)
    , descriptor_pool_(other.descriptor_pool_)
    , descriptor_set_(other.descriptor_set_)
    , command_pool_(other.command_pool_)
    , command_buffer_(other.command_buffer_)
    , compute_queue_(other.compute_queue_)
    , fence_(other.fence_)
    , object_bounds_buffer_(std::move(other.object_bounds_buffer_))
    , frustum_buffer_(std::move(other.frustum_buffer_))
    , visibility_buffer_(std::move(other.visibility_buffer_))
    , visible_indices_buffer_(std::move(other.visible_indices_buffer_))
    , counter_buffer_(std::move(other.counter_buffer_))
    , initialized_(other.initialized_)
    , object_count_(other.object_count_)
    , max_objects_(other.max_objects_)
    , visible_count_(other.visible_count_)
{
    other.device_ = VK_NULL_HANDLE;
    other.initialized_ = false;
}

FrustumCuller& FrustumCuller::operator=(FrustumCuller&& other) noexcept {
    if (this != &other) {
        cleanup();

        device_ = other.device_;
        physical_device_ = other.physical_device_;
        memory_pool_ = other.memory_pool_;
        pipeline_ = other.pipeline_;
        pipeline_layout_ = other.pipeline_layout_;
        descriptor_layout_ = other.descriptor_layout_;
        descriptor_pool_ = other.descriptor_pool_;
        descriptor_set_ = other.descriptor_set_;
        command_pool_ = other.command_pool_;
        command_buffer_ = other.command_buffer_;
        compute_queue_ = other.compute_queue_;
        fence_ = other.fence_;
        object_bounds_buffer_ = std::move(other.object_bounds_buffer_);
        frustum_buffer_ = std::move(other.frustum_buffer_);
        visibility_buffer_ = std::move(other.visibility_buffer_);
        visible_indices_buffer_ = std::move(other.visible_indices_buffer_);
        counter_buffer_ = std::move(other.counter_buffer_);
        initialized_ = other.initialized_;
        object_count_ = other.object_count_;
        max_objects_ = other.max_objects_;
        visible_count_ = other.visible_count_;

        other.device_ = VK_NULL_HANDLE;
        other.initialized_ = false;
    }
    return *this;
}

void FrustumCuller::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t compute_queue_family
) {
    if (initialized_) {
        spdlog::warn("FrustumCuller already initialized");
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
        spdlog::error("Failed to create command pool for frustum culler");
        return;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate command buffer for frustum culler");
        return;
    }

    // Create fence
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(device_, &fence_info, nullptr, &fence_) != VK_SUCCESS) {
        spdlog::error("Failed to create fence for frustum culler");
        return;
    }

    // Create descriptor resources
    create_descriptor_resources();

    // Create pipeline
    create_pipeline();

    // Allocate initial buffers for 1024 objects
    allocate_buffers(1024);

    initialized_ = true;
    spdlog::info("FrustumCuller initialized successfully");
}

void FrustumCuller::cleanup() {
    if (device_ == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device_);

    if (fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }

    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }

    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }

    if (descriptor_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptor_layout_, nullptr);
        descriptor_layout_ = VK_NULL_HANDLE;
    }

    // GPUBuffers will be cleaned up by memory_pool when destroyed

    device_ = VK_NULL_HANDLE;
    initialized_ = false;
}

void FrustumCuller::create_descriptor_resources() {
    // Descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    // Binding 0: Object bounds (storage buffer, read-only)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Frustum planes (uniform buffer)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Visibility bitfield (storage buffer, read-write)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 3: Visible indices (storage buffer, read-write)
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor set layout for frustum culler");
        return;
    }

    // Descriptor pool
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[0].descriptorCount = 3;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor pool for frustum culler");
        return;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &descriptor_set_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate descriptor set for frustum culler");
        return;
    }
}

void FrustumCuller::create_pipeline() {
    // Load shader (path preserves directory structure: compute/culling/...)
    auto spirv = load_spirv_file("compute/culling/frustum_cull.comp.spv");
    if (spirv.empty()) {
        spdlog::error("Failed to load compute/culling/frustum_cull.comp.spv");
        return;
    }

    // Create shader module
    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module for frustum culler");
        return;
    }

    // Push constant range
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(FrustumCullPushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        spdlog::error("Failed to create pipeline layout for frustum culler");
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
        spdlog::error("Failed to create compute pipeline for frustum culler");
        return;
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);
    spdlog::info("Frustum culling pipeline created successfully");
}

void FrustumCuller::allocate_buffers(uint32_t max_objects) {
    if (!memory_pool_) return;

    max_objects_ = max_objects;

    // Object bounds buffer
    VkDeviceSize bounds_size = max_objects * sizeof(ObjectBounds);
    object_bounds_buffer_ = memory_pool_->allocate_buffer(
        bounds_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Frustum planes buffer (uniform)
    frustum_buffer_ = memory_pool_->allocate_buffer(
        sizeof(FrustumPlanes),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Visibility bitfield buffer (ceil(max_objects / 32) uint32_t's)
    uint32_t bitfield_words = (max_objects + 31) / 32;
    visibility_buffer_ = memory_pool_->allocate_buffer(
        bitfield_words * sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Visible indices buffer (header + indices)
    VkDeviceSize indices_size = sizeof(uint32_t) + max_objects * sizeof(uint32_t);
    visible_indices_buffer_ = memory_pool_->allocate_buffer(
        indices_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Update descriptor set with new buffers
    update_descriptor_set();

    spdlog::debug("Allocated culling buffers for {} objects", max_objects);
}

void FrustumCuller::update_descriptor_set() {
    if (!object_bounds_buffer_.get_buffer() || !frustum_buffer_.get_buffer() ||
        !visibility_buffer_.get_buffer() || !visible_indices_buffer_.get_buffer()) {
        return;
    }

    std::array<VkDescriptorBufferInfo, 4> buffer_infos{};

    buffer_infos[0].buffer = object_bounds_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = frustum_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    buffer_infos[2].buffer = visibility_buffer_.get_buffer();
    buffer_infos[2].offset = 0;
    buffer_infos[2].range = VK_WHOLE_SIZE;

    buffer_infos[3].buffer = visible_indices_buffer_.get_buffer();
    buffer_infos[3].offset = 0;
    buffer_infos[3].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = descriptor_set_;
        writes[i].dstBinding = i;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = (i == 1) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                            : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

// ============================================================================
// Frustum Plane Extraction (Gribb-Hartmann method)
// ============================================================================

FrustumPlanes FrustumCuller::extract_frustum_planes(const math::Mat4& view_proj) {
    FrustumPlanes frustum{};

    // GLM uses column-major, so view_proj[col][row]
    // Left plane: row3 + row0
    frustum.planes[FrustumPlanes::LEFT] = math::Vec4(
        view_proj[0][3] + view_proj[0][0],
        view_proj[1][3] + view_proj[1][0],
        view_proj[2][3] + view_proj[2][0],
        view_proj[3][3] + view_proj[3][0]
    );

    // Right plane: row3 - row0
    frustum.planes[FrustumPlanes::RIGHT] = math::Vec4(
        view_proj[0][3] - view_proj[0][0],
        view_proj[1][3] - view_proj[1][0],
        view_proj[2][3] - view_proj[2][0],
        view_proj[3][3] - view_proj[3][0]
    );

    // Bottom plane: row3 + row1
    frustum.planes[FrustumPlanes::BOTTOM] = math::Vec4(
        view_proj[0][3] + view_proj[0][1],
        view_proj[1][3] + view_proj[1][1],
        view_proj[2][3] + view_proj[2][1],
        view_proj[3][3] + view_proj[3][1]
    );

    // Top plane: row3 - row1
    frustum.planes[FrustumPlanes::TOP] = math::Vec4(
        view_proj[0][3] - view_proj[0][1],
        view_proj[1][3] - view_proj[1][1],
        view_proj[2][3] - view_proj[2][1],
        view_proj[3][3] - view_proj[3][1]
    );

    // Near plane: row3 + row2
    frustum.planes[FrustumPlanes::NEAR] = math::Vec4(
        view_proj[0][3] + view_proj[0][2],
        view_proj[1][3] + view_proj[1][2],
        view_proj[2][3] + view_proj[2][2],
        view_proj[3][3] + view_proj[3][2]
    );

    // Far plane: row3 - row2
    frustum.planes[FrustumPlanes::FAR] = math::Vec4(
        view_proj[0][3] - view_proj[0][2],
        view_proj[1][3] - view_proj[1][2],
        view_proj[2][3] - view_proj[2][2],
        view_proj[3][3] - view_proj[3][2]
    );

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(math::Vec3(frustum.planes[i]));
        if (len > 0.0f) {
            frustum.planes[i] /= len;
        }
    }

    return frustum;
}

// ============================================================================
// Object Bounds Upload
// ============================================================================

void FrustumCuller::upload_object_bounds(const std::vector<ObjectBounds>& bounds) {
    if (bounds.empty()) {
        object_count_ = 0;
        return;
    }

    // Reallocate if needed
    if (bounds.size() > max_objects_) {
        allocate_buffers(static_cast<uint32_t>(bounds.size() * 2));
    }

    object_count_ = static_cast<uint32_t>(bounds.size());

    // Upload via staging buffer
    void* ptr = object_bounds_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, bounds.data(), bounds.size() * sizeof(ObjectBounds));
        object_bounds_buffer_.unmap();
    } else {
        spdlog::warn("Failed to map object bounds buffer for upload");
    }
}

void FrustumCuller::upload_object_bounds_from_mobjects(
    const std::vector<std::shared_ptr<Mobject>>& mobjects
) {
    std::vector<ObjectBounds> bounds;
    bounds.reserve(mobjects.size());

    for (size_t i = 0; i < mobjects.size(); ++i) {
        const auto& mob = mobjects[i];
        if (!mob) continue;

        auto bbox = mob->compute_bounding_box();
        bounds.emplace_back(bbox.min, bbox.max, static_cast<uint32_t>(i));
    }

    upload_object_bounds(bounds);
}

// ============================================================================
// Culling Operations
// ============================================================================

void FrustumCuller::begin_commands() {
    vkResetCommandBuffer(command_buffer_, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer_, &begin_info);
}

void FrustumCuller::end_commands() {
    vkEndCommandBuffer(command_buffer_);
}

void FrustumCuller::submit_and_wait() {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    vkQueueSubmit(compute_queue_, 1, &submit_info, fence_);
    vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &fence_);
}

void FrustumCuller::cull(const FrustumPlanes& frustum, VkCommandBuffer cmd_buffer) {
    if (!initialized_ || object_count_ == 0) return;

    bool immediate_mode = (cmd_buffer == VK_NULL_HANDLE);
    VkCommandBuffer cmd = immediate_mode ? command_buffer_ : cmd_buffer;

    if (immediate_mode) {
        begin_commands();
    }

    // Upload frustum planes
    FrustumPlanes frustum_data = frustum;
    frustum_data.num_objects = object_count_;

    void* frustum_ptr = frustum_buffer_.map();
    if (frustum_ptr) {
        std::memcpy(frustum_ptr, &frustum_data, sizeof(FrustumPlanes));
        frustum_buffer_.unmap();
    }

    // Clear visibility buffer
    uint32_t bitfield_words = (object_count_ + 31) / 32;
    vkCmdFillBuffer(cmd, visibility_buffer_.get_buffer(), 0, bitfield_words * sizeof(uint32_t), 0);

    // Clear visible count
    vkCmdFillBuffer(cmd, visible_indices_buffer_.get_buffer(), 0, sizeof(uint32_t), 0);

    // Memory barrier after clear
    VkMemoryBarrier clear_barrier{};
    clear_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    clear_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    clear_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &clear_barrier, 0, nullptr, 0, nullptr
    );

    // Bind pipeline and descriptor set
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                           pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);

    // Push constants
    FrustumCullPushConstants constants{};
    constants.num_objects = object_count_;
    constants.output_mode = 1;  // Enable compaction

    vkCmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                      0, sizeof(constants), &constants);

    // Dispatch
    uint32_t workgroup_size = 256;
    uint32_t num_workgroups = (object_count_ + workgroup_size - 1) / workgroup_size;
    vkCmdDispatch(cmd, num_workgroups, 1, 1);

    // Memory barrier for output
    VkMemoryBarrier compute_barrier{};
    compute_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    compute_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    compute_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
        0, 1, &compute_barrier, 0, nullptr, 0, nullptr
    );

    if (immediate_mode) {
        end_commands();
        submit_and_wait();

        // Read back visible count
        void* indices_ptr = visible_indices_buffer_.map();
        if (indices_ptr) {
            visible_count_ = *static_cast<uint32_t*>(indices_ptr);
            visible_indices_buffer_.unmap();
        }
    }
}

void FrustumCuller::cull(const math::Mat4& view_proj, VkCommandBuffer cmd_buffer) {
    FrustumPlanes frustum = extract_frustum_planes(view_proj);
    cull(frustum, cmd_buffer);
}

std::vector<uint32_t> FrustumCuller::download_visible_indices() {
    std::vector<uint32_t> result;

    void* ptr = visible_indices_buffer_.map();
    if (!ptr) return result;

    uint32_t count = *static_cast<uint32_t*>(ptr);
    visible_count_ = count;

    if (count > 0 && count <= object_count_) {
        result.resize(count);
        uint32_t* indices = static_cast<uint32_t*>(ptr) + 1;  // Skip count
        std::memcpy(result.data(), indices, count * sizeof(uint32_t));
    }

    visible_indices_buffer_.unmap();
    return result;
}

bool FrustumCuller::is_visible(uint32_t object_index) const {
    if (object_index >= object_count_) return false;

    // Read visibility bitfield if not cached
    if (visibility_bitfield_.empty()) {
        uint32_t bitfield_words = (object_count_ + 31) / 32;
        visibility_bitfield_.resize(bitfield_words);

        void* ptr = const_cast<GPUBuffer&>(visibility_buffer_).map();
        if (ptr) {
            std::memcpy(visibility_bitfield_.data(), ptr, bitfield_words * sizeof(uint32_t));
            const_cast<GPUBuffer&>(visibility_buffer_).unmap();
        }
    }

    uint32_t word_idx = object_index / 32;
    uint32_t bit_idx = object_index % 32;
    return (visibility_bitfield_[word_idx] & (1u << bit_idx)) != 0;
}

// ============================================================================
// CPU Fallback
// ============================================================================

bool FrustumCuller::test_aabb_frustum(
    const math::Vec3& aabb_min,
    const math::Vec3& aabb_max,
    const FrustumPlanes& frustum
) {
    for (int i = 0; i < 6; ++i) {
        const math::Vec4& plane = frustum.planes[i];
        math::Vec3 p_vertex;

        // Find P-vertex (most positive corner relative to plane normal)
        p_vertex.x = (plane.x >= 0.0f) ? aabb_max.x : aabb_min.x;
        p_vertex.y = (plane.y >= 0.0f) ? aabb_max.y : aabb_min.y;
        p_vertex.z = (plane.z >= 0.0f) ? aabb_max.z : aabb_min.z;

        // If P-vertex is on negative side, AABB is completely outside
        float dist = glm::dot(math::Vec3(plane), p_vertex) + plane.w;
        if (dist < 0.0f) {
            return false;
        }
    }
    return true;
}

std::vector<uint32_t> FrustumCuller::cull_cpu(
    const std::vector<ObjectBounds>& bounds,
    const FrustumPlanes& frustum
) {
    std::vector<uint32_t> visible;
    visible.reserve(bounds.size());

    for (uint32_t i = 0; i < bounds.size(); ++i) {
        const auto& obj = bounds[i];
        math::Vec3 aabb_min(obj.aabb_min);
        math::Vec3 aabb_max(obj.aabb_max);

        if (test_aabb_frustum(aabb_min, aabb_max, frustum)) {
            visible.push_back(i);
        }
    }

    return visible;
}

} // namespace culling
} // namespace manim
