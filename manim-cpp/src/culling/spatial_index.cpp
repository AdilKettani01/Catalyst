#include "manim/culling/spatial_index.hpp"
#include "manim/mobject/mobject.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <fstream>
#include <filesystem>

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

    return spirv;
}

// Push constants for Morton code shader
struct MortonPushConstants {
    math::Vec3 scene_min;
    uint32_t num_objects;
    math::Vec3 scene_max;
    uint32_t padding;
};

// Push constants for refit shader
struct RefitPushConstants {
    uint32_t num_internal_nodes;
    uint32_t num_leaves;
};

// Push constants for Karras BVH shader
struct KarrasPushConstants {
    uint32_t num_leaves;
    uint32_t padding[3];
};

// Push constants for generate leaves shader
struct GenerateLeavesPushConstants {
    uint32_t num_leaves;
    uint32_t num_internal;
    uint32_t padding[2];
};

SpatialIndex::SpatialIndex() = default;

SpatialIndex::~SpatialIndex() {
    cleanup();
}

SpatialIndex::SpatialIndex(SpatialIndex&& other) noexcept
    : device_(other.device_)
    , physical_device_(other.physical_device_)
    , memory_pool_(other.memory_pool_)
    , morton_pipeline_(other.morton_pipeline_)
    , refit_pipeline_(other.refit_pipeline_)
    , morton_layout_(other.morton_layout_)
    , refit_layout_(other.refit_layout_)
    , morton_desc_layout_(other.morton_desc_layout_)
    , refit_desc_layout_(other.refit_desc_layout_)
    , descriptor_pool_(other.descriptor_pool_)
    , morton_desc_set_(other.morton_desc_set_)
    , refit_desc_set_(other.refit_desc_set_)
    , command_pool_(other.command_pool_)
    , command_buffer_(other.command_buffer_)
    , compute_queue_(other.compute_queue_)
    , fence_(other.fence_)
    , object_bounds_buffer_(std::move(other.object_bounds_buffer_))
    , morton_codes_buffer_(std::move(other.morton_codes_buffer_))
    , sorted_indices_buffer_(std::move(other.sorted_indices_buffer_))
    , bvh_nodes_buffer_(std::move(other.bvh_nodes_buffer_))
    , atomic_counters_buffer_(std::move(other.atomic_counters_buffer_))
    , initialized_(other.initialized_)
    , object_count_(other.object_count_)
    , max_objects_(other.max_objects_)
    , scene_min_(other.scene_min_)
    , scene_max_(other.scene_max_)
    , stats_(other.stats_)
    , prev_bounds_(std::move(other.prev_bounds_))
{
    other.device_ = VK_NULL_HANDLE;
    other.initialized_ = false;
}

SpatialIndex& SpatialIndex::operator=(SpatialIndex&& other) noexcept {
    if (this != &other) {
        cleanup();

        device_ = other.device_;
        physical_device_ = other.physical_device_;
        memory_pool_ = other.memory_pool_;
        morton_pipeline_ = other.morton_pipeline_;
        refit_pipeline_ = other.refit_pipeline_;
        morton_layout_ = other.morton_layout_;
        refit_layout_ = other.refit_layout_;
        morton_desc_layout_ = other.morton_desc_layout_;
        refit_desc_layout_ = other.refit_desc_layout_;
        descriptor_pool_ = other.descriptor_pool_;
        morton_desc_set_ = other.morton_desc_set_;
        refit_desc_set_ = other.refit_desc_set_;
        command_pool_ = other.command_pool_;
        command_buffer_ = other.command_buffer_;
        compute_queue_ = other.compute_queue_;
        fence_ = other.fence_;
        object_bounds_buffer_ = std::move(other.object_bounds_buffer_);
        morton_codes_buffer_ = std::move(other.morton_codes_buffer_);
        sorted_indices_buffer_ = std::move(other.sorted_indices_buffer_);
        bvh_nodes_buffer_ = std::move(other.bvh_nodes_buffer_);
        atomic_counters_buffer_ = std::move(other.atomic_counters_buffer_);
        initialized_ = other.initialized_;
        object_count_ = other.object_count_;
        max_objects_ = other.max_objects_;
        scene_min_ = other.scene_min_;
        scene_max_ = other.scene_max_;
        stats_ = other.stats_;
        prev_bounds_ = std::move(other.prev_bounds_);

        other.device_ = VK_NULL_HANDLE;
        other.initialized_ = false;
    }
    return *this;
}

void SpatialIndex::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t compute_queue_family
) {
    if (initialized_) {
        spdlog::warn("SpatialIndex already initialized");
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
        spdlog::error("Failed to create command pool for spatial index");
        return;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate command buffer for spatial index");
        return;
    }

    // Create fence
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(device_, &fence_info, nullptr, &fence_) != VK_SUCCESS) {
        spdlog::error("Failed to create fence for spatial index");
        return;
    }

    // Create descriptor resources and pipelines
    create_descriptor_resources();
    create_morton_pipeline();
    create_refit_pipeline();
    create_karras_pipeline();
    create_generate_leaves_pipeline();

    // Allocate initial buffers
    allocate_buffers(1024);

    initialized_ = true;
    spdlog::info("SpatialIndex initialized successfully");
}

void SpatialIndex::cleanup() {
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

    if (morton_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, morton_pipeline_, nullptr);
        morton_pipeline_ = VK_NULL_HANDLE;
    }

    if (refit_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, refit_pipeline_, nullptr);
        refit_pipeline_ = VK_NULL_HANDLE;
    }

    if (morton_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, morton_layout_, nullptr);
        morton_layout_ = VK_NULL_HANDLE;
    }

    if (refit_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, refit_layout_, nullptr);
        refit_layout_ = VK_NULL_HANDLE;
    }

    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }

    if (morton_desc_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, morton_desc_layout_, nullptr);
        morton_desc_layout_ = VK_NULL_HANDLE;
    }

    if (refit_desc_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, refit_desc_layout_, nullptr);
        refit_desc_layout_ = VK_NULL_HANDLE;
    }

    // Cleanup Karras BVH pipelines
    if (karras_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, karras_pipeline_, nullptr);
        karras_pipeline_ = VK_NULL_HANDLE;
    }

    if (generate_leaves_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, generate_leaves_pipeline_, nullptr);
        generate_leaves_pipeline_ = VK_NULL_HANDLE;
    }

    if (karras_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, karras_layout_, nullptr);
        karras_layout_ = VK_NULL_HANDLE;
    }

    if (generate_leaves_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, generate_leaves_layout_, nullptr);
        generate_leaves_layout_ = VK_NULL_HANDLE;
    }

    if (karras_desc_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, karras_desc_layout_, nullptr);
        karras_desc_layout_ = VK_NULL_HANDLE;
    }

    if (generate_leaves_desc_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, generate_leaves_desc_layout_, nullptr);
        generate_leaves_desc_layout_ = VK_NULL_HANDLE;
    }

    device_ = VK_NULL_HANDLE;
    initialized_ = false;
}

void SpatialIndex::create_descriptor_resources() {
    // Morton code shader: 2 bindings (input bounds, output morton pairs)
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &morton_desc_layout_);
    }

    // Refit shader: 5 bindings
    {
        std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
        for (uint32_t i = 0; i < 5; ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &refit_desc_layout_);
    }

    // Karras BVH shader: 3 bindings (sorted morton, BVH nodes, parent indices)
    {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
        for (uint32_t i = 0; i < 3; ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &karras_desc_layout_);
    }

    // Generate leaves shader: 4 bindings (object bounds, sorted morton, BVH nodes, sorted indices)
    {
        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
        for (uint32_t i = 0; i < 4; ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &generate_leaves_desc_layout_);
    }

    // Descriptor pool (increased capacity for new descriptor sets)
    std::array<VkDescriptorPoolSize, 1> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[0].descriptorCount = 24;  // Increased for new pipelines

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 6;  // morton, refit, karras, generate_leaves + spares
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();

    vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_);

    // Allocate descriptor sets
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;

    alloc_info.pSetLayouts = &morton_desc_layout_;
    vkAllocateDescriptorSets(device_, &alloc_info, &morton_desc_set_);

    alloc_info.pSetLayouts = &refit_desc_layout_;
    vkAllocateDescriptorSets(device_, &alloc_info, &refit_desc_set_);

    alloc_info.pSetLayouts = &karras_desc_layout_;
    vkAllocateDescriptorSets(device_, &alloc_info, &karras_desc_set_);

    alloc_info.pSetLayouts = &generate_leaves_desc_layout_;
    vkAllocateDescriptorSets(device_, &alloc_info, &generate_leaves_desc_set_);
}

void SpatialIndex::create_morton_pipeline() {
    auto spirv = load_spirv_file("compute/culling/morton_codes.comp.spv");
    if (spirv.empty()) {
        spdlog::warn("Morton codes shader not found, will use CPU fallback");
        return;
    }

    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(MortonPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &morton_desc_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    vkCreatePipelineLayout(device_, &layout_info, nullptr, &morton_layout_);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = morton_layout_;

    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &morton_pipeline_);
    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Morton codes pipeline created");
}

void SpatialIndex::create_refit_pipeline() {
    auto spirv = load_spirv_file("compute/culling/bvh_refit.comp.spv");
    if (spirv.empty()) {
        spdlog::warn("BVH refit shader not found, will use CPU fallback");
        return;
    }

    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(RefitPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &refit_desc_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    vkCreatePipelineLayout(device_, &layout_info, nullptr, &refit_layout_);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = refit_layout_;

    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &refit_pipeline_);
    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("BVH refit pipeline created");
}

void SpatialIndex::create_karras_pipeline() {
    auto spirv = load_spirv_file("compute/culling/karras_bvh.comp.spv");
    if (spirv.empty()) {
        spdlog::warn("Karras BVH shader not found, will use CPU fallback");
        return;
    }

    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(KarrasPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &karras_desc_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    vkCreatePipelineLayout(device_, &layout_info, nullptr, &karras_layout_);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = karras_layout_;

    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &karras_pipeline_);
    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Karras BVH pipeline created");
}

void SpatialIndex::create_generate_leaves_pipeline() {
    auto spirv = load_spirv_file("compute/culling/generate_leaves.comp.spv");
    if (spirv.empty()) {
        spdlog::warn("Generate leaves shader not found, will use CPU fallback");
        return;
    }

    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(GenerateLeavesPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &generate_leaves_desc_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    vkCreatePipelineLayout(device_, &layout_info, nullptr, &generate_leaves_layout_);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = generate_leaves_layout_;

    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &generate_leaves_pipeline_);
    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Generate leaves pipeline created");
}

void SpatialIndex::allocate_buffers(uint32_t max_objects) {
    if (!memory_pool_) return;

    max_objects_ = max_objects;

    // Object bounds buffer
    object_bounds_buffer_ = memory_pool_->allocate_buffer(
        max_objects * sizeof(ObjectBounds),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Morton codes buffer (pair of uint32_t)
    morton_codes_buffer_ = memory_pool_->allocate_buffer(
        max_objects * sizeof(MortonPrimitive),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Sorted indices buffer
    sorted_indices_buffer_ = memory_pool_->allocate_buffer(
        max_objects * sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // BVH nodes buffer: internal nodes (n-1) + leaf nodes (n) = 2n-1
    uint32_t max_nodes = 2 * max_objects - 1;
    bvh_nodes_buffer_ = memory_pool_->allocate_buffer(
        max_nodes * sizeof(BVHNode),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Atomic counters for refit (one per internal node)
    atomic_counters_buffer_ = memory_pool_->allocate_buffer(
        max_objects * sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Parent indices buffer (2n-1 nodes for tree traversal)
    parent_indices_buffer_ = memory_pool_->allocate_buffer(
        max_nodes * sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Sorted codes buffer for double-buffering during sort
    sorted_codes_buffer_ = memory_pool_->allocate_buffer(
        max_objects * sizeof(MortonPrimitive),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    update_descriptor_sets();
}

void SpatialIndex::update_descriptor_sets() {
    // Update Morton descriptor set
    if (object_bounds_buffer_.get_buffer() && morton_codes_buffer_.get_buffer()) {
        std::array<VkDescriptorBufferInfo, 2> morton_buffers{};
        morton_buffers[0].buffer = object_bounds_buffer_.get_buffer();
        morton_buffers[0].offset = 0;
        morton_buffers[0].range = VK_WHOLE_SIZE;

        morton_buffers[1].buffer = morton_codes_buffer_.get_buffer();
        morton_buffers[1].offset = 0;
        morton_buffers[1].range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 2> writes{};
        for (uint32_t i = 0; i < 2; ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = morton_desc_set_;
            writes[i].dstBinding = i;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[i].descriptorCount = 1;
            writes[i].pBufferInfo = &morton_buffers[i];
        }

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

// ============================================================================
// Morton Code Computation (CPU)
// ============================================================================

static uint32_t expand_bits_10(uint32_t v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

uint32_t SpatialIndex::compute_morton_code(const math::Vec3& normalized_pos) {
    math::Vec3 clamped = glm::clamp(normalized_pos, math::Vec3(0.0f), math::Vec3(1.0f));
    glm::uvec3 quantized = glm::uvec3(clamped * 1023.0f);

    uint32_t xx = expand_bits_10(quantized.x);
    uint32_t yy = expand_bits_10(quantized.y);
    uint32_t zz = expand_bits_10(quantized.z);

    return (zz << 2) | (yy << 1) | xx;
}

// ============================================================================
// Scene Bounds Computation
// ============================================================================

void SpatialIndex::compute_scene_bounds(const std::vector<ObjectBounds>& bounds) {
    if (bounds.empty()) {
        scene_min_ = math::Vec3(0.0f);
        scene_max_ = math::Vec3(0.0f);
        return;
    }

    scene_min_ = math::Vec3(bounds[0].aabb_min);
    scene_max_ = math::Vec3(bounds[0].aabb_max);

    for (size_t i = 1; i < bounds.size(); ++i) {
        scene_min_ = glm::min(scene_min_, math::Vec3(bounds[i].aabb_min));
        scene_max_ = glm::max(scene_max_, math::Vec3(bounds[i].aabb_max));
    }

    // Add small padding to prevent degenerate cases
    math::Vec3 padding = (scene_max_ - scene_min_) * 0.001f;
    padding = glm::max(padding, math::Vec3(0.001f));
    scene_min_ -= padding;
    scene_max_ += padding;
}

// ============================================================================
// CPU BVH Construction
// ============================================================================

void SpatialIndex::build_cpu(const std::vector<ObjectBounds>& bounds) {
    auto start = std::chrono::high_resolution_clock::now();

    if (bounds.empty()) {
        object_count_ = 0;
        return;
    }

    object_count_ = static_cast<uint32_t>(bounds.size());

    // Reallocate if needed
    if (object_count_ > max_objects_) {
        allocate_buffers(object_count_ * 2);
    }

    // Compute scene bounds
    compute_scene_bounds(bounds);

    math::Vec3 scene_size = scene_max_ - scene_min_;

    // Compute Morton codes and sort
    std::vector<MortonPrimitive> morton_primitives(object_count_);

    for (uint32_t i = 0; i < object_count_; ++i) {
        math::Vec3 centroid = (math::Vec3(bounds[i].aabb_min) + math::Vec3(bounds[i].aabb_max)) * 0.5f;
        math::Vec3 normalized = (centroid - scene_min_) / scene_size;

        morton_primitives[i].morton_code = compute_morton_code(normalized);
        morton_primitives[i].object_index = i;
    }

    // Sort by Morton code (CPU radix sort would be faster, but std::sort is simpler)
    std::sort(morton_primitives.begin(), morton_primitives.end(),
              [](const MortonPrimitive& a, const MortonPrimitive& b) {
                  return a.morton_code < b.morton_code;
              });

    // Build BVH using simple recursive construction
    // For LBVH, we would use Karras algorithm here
    // This is a simplified version that creates a balanced tree

    uint32_t num_internal = object_count_ - 1;
    uint32_t num_leaves = object_count_;
    uint32_t total_nodes = num_internal + num_leaves;

    std::vector<BVHNode> nodes(total_nodes);
    std::vector<uint32_t> sorted_indices(object_count_);
    std::vector<uint32_t> parent_indices(total_nodes, 0xFFFFFFFF);

    // Extract sorted indices
    for (uint32_t i = 0; i < object_count_; ++i) {
        sorted_indices[i] = morton_primitives[i].object_index;
    }

    // Initialize leaf nodes
    for (uint32_t i = 0; i < num_leaves; ++i) {
        uint32_t node_idx = num_internal + i;
        uint32_t obj_idx = sorted_indices[i];

        nodes[node_idx].aabb_min = math::Vec3(bounds[obj_idx].aabb_min);
        nodes[node_idx].aabb_max = math::Vec3(bounds[obj_idx].aabb_max);
        nodes[node_idx].set_leaf(i, 1);  // Single primitive per leaf
    }

    // Build internal nodes using simple binary tree
    // This is a placeholder - full LBVH would use Karras algorithm
    std::function<void(uint32_t, uint32_t, uint32_t)> build_recursive;
    build_recursive = [&](uint32_t node_idx, uint32_t first, uint32_t last) {
        if (first == last) {
            // Leaf
            return;
        }

        uint32_t mid = (first + last) / 2;
        uint32_t left_child = (first == mid) ? (num_internal + first) : node_idx + 1;
        uint32_t right_child = (mid + 1 == last) ? (num_internal + last) : node_idx + 1 + (mid - first);

        // For internal nodes, find next available slot
        // This is simplified - real LBVH computes child indices from Morton codes

        nodes[node_idx].left = left_child;
        nodes[node_idx].right = right_child;
        parent_indices[left_child] = node_idx;
        parent_indices[right_child] = node_idx;

        // Build children
        if (first != mid) {
            build_recursive(left_child, first, mid);
        }
        if (mid + 1 != last) {
            build_recursive(right_child, mid + 1, last);
        }

        // Compute AABB as union of children
        nodes[node_idx].aabb_min = glm::min(nodes[left_child].aabb_min, nodes[right_child].aabb_min);
        nodes[node_idx].aabb_max = glm::max(nodes[left_child].aabb_max, nodes[right_child].aabb_max);
    };

    if (object_count_ > 1) {
        // Simple linear tree for now
        for (uint32_t i = 0; i < num_internal; ++i) {
            uint32_t left = (i == num_internal - 1) ? (num_internal + object_count_ - 2) : (i + 1);
            uint32_t right = num_internal + i;

            nodes[i].left = left;
            nodes[i].right = right;
            parent_indices[left] = i;
            parent_indices[right] = i;
        }

        // Compute AABBs bottom-up
        for (int i = static_cast<int>(num_internal) - 1; i >= 0; --i) {
            uint32_t left = nodes[i].left;
            uint32_t right = nodes[i].right;
            nodes[i].aabb_min = glm::min(nodes[left].aabb_min, nodes[right].aabb_min);
            nodes[i].aabb_max = glm::max(nodes[left].aabb_max, nodes[right].aabb_max);
        }
    }

    // Upload to GPU
    void* nodes_ptr = bvh_nodes_buffer_.map();
    if (nodes_ptr) {
        std::memcpy(nodes_ptr, nodes.data(), nodes.size() * sizeof(BVHNode));
        bvh_nodes_buffer_.unmap();
    }

    void* indices_ptr = sorted_indices_buffer_.map();
    if (indices_ptr) {
        std::memcpy(indices_ptr, sorted_indices.data(), sorted_indices.size() * sizeof(uint32_t));
        sorted_indices_buffer_.unmap();
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats_.build_time_ms = std::chrono::duration<float, std::milli>(end - start).count();
    stats_.num_objects = object_count_;
    stats_.num_internal_nodes = num_internal;
    stats_.num_leaf_nodes = num_leaves;
    stats_.last_update = BVHUpdateStrategy::FullRebuild;

    // Store for next frame comparison
    prev_bounds_ = bounds;

    spdlog::debug("BVH built (CPU): {} objects, {} nodes, {:.2f}ms",
                 object_count_, total_nodes, stats_.build_time_ms);
}

// ============================================================================
// GPU BVH Construction (Karras Algorithm)
// ============================================================================

bool SpatialIndex::is_gpu_build_available() const {
    return initialized_ &&
           morton_pipeline_ != VK_NULL_HANDLE &&
           karras_pipeline_ != VK_NULL_HANDLE &&
           generate_leaves_pipeline_ != VK_NULL_HANDLE &&
           refit_pipeline_ != VK_NULL_HANDLE;
}

void SpatialIndex::upload_object_bounds(const std::vector<ObjectBounds>& bounds) {
    if (!object_bounds_buffer_.get_buffer()) return;

    // Use staging buffer if DeviceLocal, otherwise map directly
    void* ptr = object_bounds_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, bounds.data(), bounds.size() * sizeof(ObjectBounds));
        object_bounds_buffer_.unmap();
    }
}

void SpatialIndex::compute_morton_codes_gpu(VkCommandBuffer cmd) {
    if (morton_pipeline_ == VK_NULL_HANDLE) return;

    // Update descriptor set bindings
    std::array<VkDescriptorBufferInfo, 2> buffer_infos{};
    buffer_infos[0].buffer = object_bounds_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = morton_codes_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> writes{};
    for (uint32_t i = 0; i < 2; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = morton_desc_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, morton_pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                           morton_layout_, 0, 1, &morton_desc_set_, 0, nullptr);

    MortonPushConstants push{};
    push.scene_min = scene_min_;
    push.num_objects = object_count_;
    push.scene_max = scene_max_;

    vkCmdPushConstants(cmd, morton_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push), &push);

    uint32_t workgroups = (object_count_ + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);
}

void SpatialIndex::sort_morton_codes_cpu() {
    // Download Morton codes from GPU, sort on CPU, upload sorted codes
    std::vector<MortonPrimitive> morton_data(object_count_);

    void* ptr = morton_codes_buffer_.map();
    if (ptr) {
        std::memcpy(morton_data.data(), ptr, object_count_ * sizeof(MortonPrimitive));
        morton_codes_buffer_.unmap();
    }

    // Sort by Morton code
    std::sort(morton_data.begin(), morton_data.end(),
              [](const MortonPrimitive& a, const MortonPrimitive& b) {
                  return a.morton_code < b.morton_code;
              });

    // Upload sorted codes
    ptr = sorted_codes_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, morton_data.data(), object_count_ * sizeof(MortonPrimitive));
        sorted_codes_buffer_.unmap();
    }
}

void SpatialIndex::build_tree_gpu(VkCommandBuffer cmd) {
    if (karras_pipeline_ == VK_NULL_HANDLE || object_count_ < 2) return;

    // Update descriptor set bindings
    std::array<VkDescriptorBufferInfo, 3> buffer_infos{};
    buffer_infos[0].buffer = sorted_codes_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = bvh_nodes_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    buffer_infos[2].buffer = parent_indices_buffer_.get_buffer();
    buffer_infos[2].offset = 0;
    buffer_infos[2].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> writes{};
    for (uint32_t i = 0; i < 3; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = karras_desc_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, karras_pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                           karras_layout_, 0, 1, &karras_desc_set_, 0, nullptr);

    KarrasPushConstants push{};
    push.num_leaves = object_count_;

    vkCmdPushConstants(cmd, karras_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push), &push);

    uint32_t num_internal = object_count_ - 1;
    uint32_t workgroups = (num_internal + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);
}

void SpatialIndex::generate_leaves_gpu(VkCommandBuffer cmd) {
    if (generate_leaves_pipeline_ == VK_NULL_HANDLE) return;

    // Update descriptor set bindings
    std::array<VkDescriptorBufferInfo, 4> buffer_infos{};
    buffer_infos[0].buffer = object_bounds_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = sorted_codes_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    buffer_infos[2].buffer = bvh_nodes_buffer_.get_buffer();
    buffer_infos[2].offset = 0;
    buffer_infos[2].range = VK_WHOLE_SIZE;

    buffer_infos[3].buffer = sorted_indices_buffer_.get_buffer();
    buffer_infos[3].offset = 0;
    buffer_infos[3].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = generate_leaves_desc_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, generate_leaves_pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                           generate_leaves_layout_, 0, 1, &generate_leaves_desc_set_, 0, nullptr);

    GenerateLeavesPushConstants push{};
    push.num_leaves = object_count_;
    push.num_internal = object_count_ - 1;

    vkCmdPushConstants(cmd, generate_leaves_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push), &push);

    uint32_t workgroups = (object_count_ + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);
}

void SpatialIndex::compute_node_bounds_gpu(VkCommandBuffer cmd) {
    if (refit_pipeline_ == VK_NULL_HANDLE) return;

    // Update descriptor set bindings for refit shader
    std::array<VkDescriptorBufferInfo, 5> buffer_infos{};
    buffer_infos[0].buffer = object_bounds_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = sorted_indices_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    buffer_infos[2].buffer = bvh_nodes_buffer_.get_buffer();
    buffer_infos[2].offset = 0;
    buffer_infos[2].range = VK_WHOLE_SIZE;

    buffer_infos[3].buffer = atomic_counters_buffer_.get_buffer();
    buffer_infos[3].offset = 0;
    buffer_infos[3].range = VK_WHOLE_SIZE;

    buffer_infos[4].buffer = parent_indices_buffer_.get_buffer();
    buffer_infos[4].offset = 0;
    buffer_infos[4].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> writes{};
    for (uint32_t i = 0; i < 5; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = refit_desc_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, refit_pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                           refit_layout_, 0, 1, &refit_desc_set_, 0, nullptr);

    RefitPushConstants push{};
    push.num_internal_nodes = object_count_ - 1;
    push.num_leaves = object_count_;

    vkCmdPushConstants(cmd, refit_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push), &push);

    uint32_t workgroups = (object_count_ + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);
}

void SpatialIndex::build_gpu(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer) {
    auto start = std::chrono::high_resolution_clock::now();

    if (bounds.empty()) {
        object_count_ = 0;
        return;
    }

    object_count_ = static_cast<uint32_t>(bounds.size());

    // Reallocate if needed
    if (object_count_ > max_objects_) {
        allocate_buffers(object_count_ * 2);
    }

    // Step 1: Compute scene bounds (CPU)
    compute_scene_bounds(bounds);

    // Step 2: Upload object bounds
    upload_object_bounds(bounds);

    bool using_internal_cmd = (cmd_buffer == VK_NULL_HANDLE);
    if (using_internal_cmd) {
        begin_commands();
        cmd_buffer = command_buffer_;
    }

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // Step 3: Compute Morton codes on GPU
    compute_morton_codes_gpu(cmd_buffer);
    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Step 4: Sort Morton codes (CPU fallback for now)
    if (using_internal_cmd) {
        end_commands();
        submit_and_wait();
    }
    sort_morton_codes_cpu();
    if (using_internal_cmd) {
        begin_commands();
        cmd_buffer = command_buffer_;
    }

    // Step 5: Initialize parent indices to 0xFFFFFFFF
    uint32_t total_nodes = 2 * object_count_ - 1;
    vkCmdFillBuffer(cmd_buffer, parent_indices_buffer_.get_buffer(), 0,
                    total_nodes * sizeof(uint32_t), 0xFFFFFFFF);

    // Initialize atomic counters to 0
    vkCmdFillBuffer(cmd_buffer, atomic_counters_buffer_.get_buffer(), 0,
                    (object_count_ - 1) * sizeof(uint32_t), 0);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Step 6: Build tree structure (Karras algorithm)
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    build_tree_gpu(cmd_buffer);
    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Step 7: Generate leaf nodes
    generate_leaves_gpu(cmd_buffer);
    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Step 8: Compute internal node AABBs (refit)
    compute_node_bounds_gpu(cmd_buffer);

    if (using_internal_cmd) {
        end_commands();
        submit_and_wait();
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats_.build_time_ms = std::chrono::duration<float, std::milli>(end - start).count();
    stats_.num_objects = object_count_;
    stats_.num_internal_nodes = object_count_ - 1;
    stats_.num_leaf_nodes = object_count_;
    stats_.last_update = BVHUpdateStrategy::FullRebuild;

    prev_bounds_ = bounds;

    spdlog::debug("BVH built (GPU): {} objects, {:.2f}ms", object_count_, stats_.build_time_ms);
}

// ============================================================================
// Public Interface
// ============================================================================

void SpatialIndex::build(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer) {
    if (use_gpu_build_ && is_gpu_build_available()) {
        build_gpu(bounds, cmd_buffer);
    } else {
        build_cpu(bounds);
    }
}

void SpatialIndex::build(const std::vector<std::shared_ptr<Mobject>>& mobjects, VkCommandBuffer cmd_buffer) {
    std::vector<ObjectBounds> bounds;
    bounds.reserve(mobjects.size());

    for (size_t i = 0; i < mobjects.size(); ++i) {
        const auto& mob = mobjects[i];
        if (!mob) continue;

        auto bbox = mob->compute_bounding_box();
        bounds.emplace_back(bbox.min, bbox.max, static_cast<uint32_t>(i));
    }

    build(bounds, cmd_buffer);
}

BVHUpdateStrategy SpatialIndex::determine_update_strategy(const std::vector<ObjectBounds>& bounds) const {
    if (object_count_ == 0 || prev_bounds_.empty()) {
        return BVHUpdateStrategy::FullRebuild;
    }

    if (bounds.size() != prev_bounds_.size()) {
        return BVHUpdateStrategy::FullRebuild;
    }

    // Compute maximum displacement
    float max_displacement = 0.0f;
    float avg_size = 0.0f;

    for (size_t i = 0; i < bounds.size(); ++i) {
        math::Vec3 prev_center = (math::Vec3(prev_bounds_[i].aabb_min) + math::Vec3(prev_bounds_[i].aabb_max)) * 0.5f;
        math::Vec3 curr_center = (math::Vec3(bounds[i].aabb_min) + math::Vec3(bounds[i].aabb_max)) * 0.5f;

        float displacement = glm::length(curr_center - prev_center);
        max_displacement = std::max(max_displacement, displacement);

        math::Vec3 size = math::Vec3(bounds[i].aabb_max) - math::Vec3(bounds[i].aabb_min);
        avg_size += (size.x + size.y + size.z) / 3.0f;
    }

    avg_size /= bounds.size();

    // If displacement is significant relative to object size, rebuild
    if (max_displacement > avg_size * rebuild_threshold_) {
        return BVHUpdateStrategy::FullRebuild;
    }

    if (max_displacement > 0.0001f) {
        return BVHUpdateStrategy::Refit;
    }

    return BVHUpdateStrategy::None;
}

BVHUpdateStrategy SpatialIndex::update(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer) {
    BVHUpdateStrategy strategy = determine_update_strategy(bounds);

    switch (strategy) {
        case BVHUpdateStrategy::None:
            break;
        case BVHUpdateStrategy::Refit:
            refit(bounds, cmd_buffer);
            break;
        case BVHUpdateStrategy::FullRebuild:
            rebuild(bounds, cmd_buffer);
            break;
    }

    stats_.last_update = strategy;
    return strategy;
}

void SpatialIndex::refit(const std::vector<ObjectBounds>& bounds, VkCommandBuffer /*cmd_buffer*/) {
    auto start = std::chrono::high_resolution_clock::now();

    // For now, use CPU refit
    // TODO: Implement GPU refit using bvh_refit.comp

    if (bounds.size() != object_count_) {
        rebuild(bounds);
        return;
    }

    // Just update leaf bounds and propagate up
    // This requires the tree structure and parent indices

    prev_bounds_ = bounds;

    auto end = std::chrono::high_resolution_clock::now();
    stats_.refit_time_ms = std::chrono::duration<float, std::milli>(end - start).count();
    stats_.last_update = BVHUpdateStrategy::Refit;
}

void SpatialIndex::rebuild(const std::vector<ObjectBounds>& bounds, VkCommandBuffer cmd_buffer) {
    build(bounds, cmd_buffer);
}

std::vector<BVHNode> SpatialIndex::download_nodes() {
    std::vector<BVHNode> nodes;

    if (object_count_ == 0) return nodes;

    uint32_t total_nodes = 2 * object_count_ - 1;
    nodes.resize(total_nodes);

    void* ptr = bvh_nodes_buffer_.map();
    if (ptr) {
        std::memcpy(nodes.data(), ptr, total_nodes * sizeof(BVHNode));
        bvh_nodes_buffer_.unmap();
    }

    return nodes;
}

void SpatialIndex::begin_commands() {
    vkResetCommandBuffer(command_buffer_, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer_, &begin_info);
}

void SpatialIndex::end_commands() {
    vkEndCommandBuffer(command_buffer_);
}

void SpatialIndex::submit_and_wait() {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    vkQueueSubmit(compute_queue_, 1, &submit_info, fence_);
    vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &fence_);
}

} // namespace culling
} // namespace manim
