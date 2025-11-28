#include "manim/culling/indirect_renderer.hpp"
#include "manim/mobject/mobject.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstring>
#include <fstream>

namespace manim {
namespace culling {

// Push constants for generate draws shader
struct GenerateDrawsPushConstants {
    uint32_t num_visible;
    uint32_t max_draws;
};

IndirectRenderer::IndirectRenderer() = default;

IndirectRenderer::~IndirectRenderer() {
    cleanup();
}

IndirectRenderer::IndirectRenderer(IndirectRenderer&& other) noexcept
    : config_(std::move(other.config_))
    , initialized_(other.initialized_)
    , device_(other.device_)
    , physical_device_(other.physical_device_)
    , memory_pool_(other.memory_pool_)
    , graphics_queue_family_(other.graphics_queue_family_)
    , compute_queue_family_(other.compute_queue_family_)
    , culling_pipeline_(std::move(other.culling_pipeline_))
    , generate_draws_pipeline_(other.generate_draws_pipeline_)
    , generate_draws_layout_(other.generate_draws_layout_)
    , generate_draws_desc_layout_(other.generate_draws_desc_layout_)
    , generate_draws_desc_pool_(other.generate_draws_desc_pool_)
    , generate_draws_desc_set_(other.generate_draws_desc_set_)
    , vertex_buffer_(std::move(other.vertex_buffer_))
    , index_buffer_(std::move(other.index_buffer_))
    , object_data_buffer_(std::move(other.object_data_buffer_))
    , object_draw_info_buffer_(std::move(other.object_draw_info_buffer_))
    , draw_command_buffer_(std::move(other.draw_command_buffer_))
    , draw_count_buffer_(std::move(other.draw_count_buffer_))
    , object_count_(other.object_count_)
    , vertex_count_(other.vertex_count_)
    , index_count_(other.index_count_)
    , stats_(other.stats_)
    , object_draw_info_(std::move(other.object_draw_info_))
    , object_bounds_(std::move(other.object_bounds_))
{
    other.initialized_ = false;
    other.device_ = VK_NULL_HANDLE;
}

IndirectRenderer& IndirectRenderer::operator=(IndirectRenderer&& other) noexcept {
    if (this != &other) {
        cleanup();

        config_ = std::move(other.config_);
        initialized_ = other.initialized_;
        device_ = other.device_;
        physical_device_ = other.physical_device_;
        memory_pool_ = other.memory_pool_;
        graphics_queue_family_ = other.graphics_queue_family_;
        compute_queue_family_ = other.compute_queue_family_;
        culling_pipeline_ = std::move(other.culling_pipeline_);
        generate_draws_pipeline_ = other.generate_draws_pipeline_;
        generate_draws_layout_ = other.generate_draws_layout_;
        generate_draws_desc_layout_ = other.generate_draws_desc_layout_;
        generate_draws_desc_pool_ = other.generate_draws_desc_pool_;
        generate_draws_desc_set_ = other.generate_draws_desc_set_;
        vertex_buffer_ = std::move(other.vertex_buffer_);
        index_buffer_ = std::move(other.index_buffer_);
        object_data_buffer_ = std::move(other.object_data_buffer_);
        object_draw_info_buffer_ = std::move(other.object_draw_info_buffer_);
        draw_command_buffer_ = std::move(other.draw_command_buffer_);
        draw_count_buffer_ = std::move(other.draw_count_buffer_);
        object_count_ = other.object_count_;
        vertex_count_ = other.vertex_count_;
        index_count_ = other.index_count_;
        stats_ = other.stats_;
        object_draw_info_ = std::move(other.object_draw_info_);
        object_bounds_ = std::move(other.object_bounds_);

        other.initialized_ = false;
        other.device_ = VK_NULL_HANDLE;
    }
    return *this;
}

void IndirectRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t graphics_queue_family,
    uint32_t compute_queue_family,
    const Config& config
) {
    if (initialized_) {
        spdlog::warn("IndirectRenderer already initialized");
        return;
    }

    device_ = device;
    physical_device_ = physical_device;
    memory_pool_ = &memory_pool;
    graphics_queue_family_ = graphics_queue_family;
    compute_queue_family_ = compute_queue_family;
    config_ = config;

    // Initialize culling pipeline
    culling_pipeline_ = std::make_unique<GPUCullingPipeline>();
    CullingConfig cull_config;
    cull_config.max_objects = config_.max_objects;
    culling_pipeline_->initialize(device, physical_device, memory_pool, compute_queue_family, cull_config);

    if (!culling_pipeline_->is_initialized()) {
        spdlog::error("Failed to initialize culling pipeline for indirect renderer");
        return;
    }

    // Detect indirect count support
    detect_indirect_count_support();

    // Create generate draws pipeline
    create_generate_draws_pipeline();

    // Allocate buffers
    allocate_buffers();

    initialized_ = true;
    spdlog::info("IndirectRenderer initialized (max {} objects, {} draws, indirect_count={})",
                 config_.max_objects, config_.max_draw_calls, supports_indirect_count_ ? "yes" : "no");
}

void IndirectRenderer::cleanup() {
    if (!device_) return;

    vkDeviceWaitIdle(device_);

    if (culling_pipeline_) {
        culling_pipeline_->cleanup();
        culling_pipeline_.reset();
    }

    if (generate_draws_pipeline_) {
        vkDestroyPipeline(device_, generate_draws_pipeline_, nullptr);
        generate_draws_pipeline_ = VK_NULL_HANDLE;
    }
    if (generate_draws_layout_) {
        vkDestroyPipelineLayout(device_, generate_draws_layout_, nullptr);
        generate_draws_layout_ = VK_NULL_HANDLE;
    }
    if (generate_draws_desc_layout_) {
        vkDestroyDescriptorSetLayout(device_, generate_draws_desc_layout_, nullptr);
        generate_draws_desc_layout_ = VK_NULL_HANDLE;
    }
    if (generate_draws_desc_pool_) {
        vkDestroyDescriptorPool(device_, generate_draws_desc_pool_, nullptr);
        generate_draws_desc_pool_ = VK_NULL_HANDLE;
    }

    // GPU buffers are cleaned up by MemoryPool or their destructors
    vertex_buffer_ = GPUBuffer{};
    index_buffer_ = GPUBuffer{};
    object_data_buffer_ = GPUBuffer{};
    object_draw_info_buffer_ = GPUBuffer{};
    draw_command_buffer_ = GPUBuffer{};
    draw_count_buffer_ = GPUBuffer{};

    initialized_ = false;
    device_ = VK_NULL_HANDLE;
}

void IndirectRenderer::create_generate_draws_pipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[4] = {};

    // Binding 0: Visible indices buffer (from culling)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Object draw info buffer
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Draw commands buffer (output)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 3: Draw count buffer (output)
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 4;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &generate_draws_desc_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create generate draws descriptor set layout");
        return;
    }

    // Create pipeline layout with push constants
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(GenerateDrawsPushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &generate_draws_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &generate_draws_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create generate draws pipeline layout");
        return;
    }

    // Create descriptor pool
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = 4;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &generate_draws_desc_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create generate draws descriptor pool");
        return;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = generate_draws_desc_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &generate_draws_desc_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &generate_draws_desc_set_) != VK_SUCCESS) {
        spdlog::error("Failed to allocate generate draws descriptor set");
        return;
    }

    // Load shader
    std::string shader_path = "shaders/compute/culling/generate_draws.comp.spv";
    std::ifstream file(shader_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::warn("Generate draws shader not found at {}", shader_path);
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
        spdlog::error("Failed to create generate draws shader module");
        return;
    }

    // Create pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = generate_draws_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &generate_draws_pipeline_) != VK_SUCCESS) {
        spdlog::error("Failed to create generate draws pipeline");
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);

    spdlog::debug("Generate draws pipeline created");
}

void IndirectRenderer::allocate_buffers() {
    if (!memory_pool_) return;

    // Vertex buffer (position + normal + uv = ~32 bytes per vertex)
    vertex_buffer_ = memory_pool_->allocate_buffer(
        config_.max_vertices * 32,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Static
    );

    // Index buffer (32-bit indices)
    index_buffer_ = memory_pool_->allocate_buffer(
        config_.max_indices * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Static
    );

    // Object data buffer (transforms, materials - ~128 bytes per object)
    object_data_buffer_ = memory_pool_->allocate_buffer(
        config_.max_objects * 128,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Object draw info buffer
    object_draw_info_buffer_ = memory_pool_->allocate_buffer(
        config_.max_objects * sizeof(ObjectDrawInfo),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Indirect draw command buffer
    draw_command_buffer_ = memory_pool_->allocate_buffer(
        config_.max_draw_calls * sizeof(DrawIndexedIndirectCommand),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Draw count buffer
    draw_count_buffer_ = memory_pool_->allocate_buffer(
        sizeof(uint32_t),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    spdlog::debug("IndirectRenderer buffers allocated");
}

void IndirectRenderer::update_descriptor_set() {
    if (!generate_draws_desc_set_) return;

    const auto& visible_buffer = culling_pipeline_->get_visible_indices_buffer();

    VkDescriptorBufferInfo visible_info{};
    visible_info.buffer = visible_buffer.get_buffer();
    visible_info.offset = 0;
    visible_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo draw_info_info{};
    draw_info_info.buffer = object_draw_info_buffer_.get_buffer();
    draw_info_info.offset = 0;
    draw_info_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo commands_info{};
    commands_info.buffer = draw_command_buffer_.get_buffer();
    commands_info.offset = 0;
    commands_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo count_info{};
    count_info.buffer = draw_count_buffer_.get_buffer();
    count_info.offset = 0;
    count_info.range = sizeof(uint32_t);

    VkWriteDescriptorSet writes[4] = {};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = generate_draws_desc_set_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &visible_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = generate_draws_desc_set_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &draw_info_info;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = generate_draws_desc_set_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &commands_info;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = generate_draws_desc_set_;
    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &count_info;

    vkUpdateDescriptorSets(device_, 4, writes, 0, nullptr);
}

void IndirectRenderer::begin_frame() {
    // Reset draw count to 0
    uint32_t zero = 0;
    void* ptr = draw_count_buffer_.map();
    if (ptr) {
        memcpy(ptr, &zero, sizeof(uint32_t));
        draw_count_buffer_.unmap();
    }

    stats_ = Stats{};
}

void IndirectRenderer::submit_objects(const std::vector<std::shared_ptr<Mobject>>& mobjects) {
    if (!initialized_) return;

    object_count_ = static_cast<uint32_t>(mobjects.size());
    stats_.total_objects = object_count_;

    // Build bounds and draw info
    object_bounds_.clear();
    object_bounds_.reserve(mobjects.size());
    object_draw_info_.clear();
    object_draw_info_.reserve(mobjects.size());

    uint32_t current_vertex_offset = 0;
    uint32_t current_index_offset = 0;

    for (size_t i = 0; i < mobjects.size(); ++i) {
        const auto& mob = mobjects[i];
        if (!mob) continue;

        // Compute bounds
        auto bbox = mob->compute_bounding_box();
        object_bounds_.emplace_back(bbox.min, bbox.max, static_cast<uint32_t>(i));

        // Create draw info
        ObjectDrawInfo info{};
        // In a full implementation, we'd get actual vertex/index counts
        // For now, assume simple triangulated geometry
        info.index_count = 36;  // Placeholder (e.g., cube = 36 indices)
        info.first_index = current_index_offset;
        info.vertex_offset = static_cast<int32_t>(current_vertex_offset);
        info.material_id = 0;
        info.lod_level = 0;

        object_draw_info_.push_back(info);

        // In full implementation, update offsets based on actual geometry
        current_vertex_offset += 24;  // Placeholder
        current_index_offset += 36;   // Placeholder
    }

    vertex_count_ = current_vertex_offset;
    index_count_ = current_index_offset;
    stats_.total_vertices = vertex_count_;
    stats_.total_indices = index_count_;

    // Upload bounds to culling pipeline
    culling_pipeline_->update_object_bounds(object_bounds_);

    // Upload draw info
    upload_object_draw_info();

    // Update descriptor set
    update_descriptor_set();
}

void IndirectRenderer::update_object_transforms(const std::vector<std::shared_ptr<Mobject>>& mobjects) {
    if (!initialized_ || mobjects.size() != object_count_) {
        // If object count changed, do full resubmit
        submit_objects(mobjects);
        return;
    }

    // Update bounds only
    for (size_t i = 0; i < mobjects.size(); ++i) {
        const auto& mob = mobjects[i];
        if (!mob) continue;

        auto bbox = mob->compute_bounding_box();
        object_bounds_[i] = ObjectBounds(bbox.min, bbox.max, static_cast<uint32_t>(i));
    }

    // Re-upload bounds
    culling_pipeline_->update_object_bounds(object_bounds_);
}

void IndirectRenderer::upload_object_draw_info() {
    if (object_draw_info_.empty()) return;

    void* ptr = object_draw_info_buffer_.map();
    if (ptr) {
        memcpy(ptr, object_draw_info_.data(), object_draw_info_.size() * sizeof(ObjectDrawInfo));
        object_draw_info_buffer_.unmap();
    }
}

void IndirectRenderer::cull_and_generate_draws(
    const math::Mat4& view_proj,
    VkCommandBuffer cmd_buffer
) {
    if (!initialized_ || object_count_ == 0) {
        stats_.visible_objects = 0;
        stats_.draw_calls = 0;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Reset draw count
    uint32_t zero = 0;
    void* ptr = draw_count_buffer_.map();
    if (ptr) {
        memcpy(ptr, &zero, sizeof(uint32_t));
        draw_count_buffer_.unmap();
    }

    // Execute culling
    culling_pipeline_->execute(view_proj, cmd_buffer);

    // Get visible count
    uint32_t visible_count = culling_pipeline_->get_visible_count();
    stats_.visible_objects = visible_count;

    if (visible_count == 0) {
        stats_.draw_calls = 0;
        return;
    }

    // Generate draw commands
    if (generate_draws_pipeline_) {
        // Memory barrier after culling
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &barrier, 0, nullptr, 0, nullptr);

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, generate_draws_pipeline_);
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            generate_draws_layout_, 0, 1, &generate_draws_desc_set_, 0, nullptr);

        GenerateDrawsPushConstants pc{};
        pc.num_visible = visible_count;
        pc.max_draws = config_.max_draw_calls;

        vkCmdPushConstants(cmd_buffer, generate_draws_layout_,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

        uint32_t num_groups = (visible_count + 255) / 256;
        vkCmdDispatch(cmd_buffer, num_groups, 1, 1);

        // Memory barrier for indirect buffer
        VkBufferMemoryBarrier buffer_barrier{};
        buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        buffer_barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        buffer_barrier.buffer = draw_command_buffer_.get_buffer();
        buffer_barrier.offset = 0;
        buffer_barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            0, 0, nullptr, 1, &buffer_barrier, 0, nullptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats_.generate_draws_time_ms = std::chrono::duration<float, std::milli>(end - start).count();

    // Note: draw_calls will be updated after GPU execution completes
}

void IndirectRenderer::record_draws(VkCommandBuffer cmd_buffer) {
    if (!initialized_ || object_count_ == 0) {
        return;
    }

    VkBuffer draw_cmd_buffer = draw_command_buffer_.get_buffer();
    VkBuffer count_buffer = draw_count_buffer_.get_buffer();

    if (!draw_cmd_buffer || !count_buffer) {
        spdlog::warn("Draw command or count buffer not available");
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Use indirect count if supported and enabled
    if (supports_indirect_count_ && use_indirect_count_ && vkCmdDrawIndexedIndirectCountFn_) {
        // GPU-driven rendering: draw count comes from GPU
        vkCmdDrawIndexedIndirectCountFn_(
            cmd_buffer,
            draw_cmd_buffer,
            0,  // offset into draw command buffer
            count_buffer,
            0,  // offset into count buffer
            config_.max_draw_calls,  // maximum draw count
            sizeof(DrawIndexedIndirectCommand)
        );

        spdlog::debug("Indirect draw recorded with GPU-driven count");
    } else {
        // Fallback: use vkCmdDrawIndexedIndirect with fixed max count
        // This draws up to max_draw_calls but GPU will skip empty commands
        // Less efficient but works on older Vulkan / llvmpipe

        // Actually use the object count we know about for safety
        uint32_t max_draws = std::min(object_count_, config_.max_draw_calls);
        if (max_draws > 0) {
            vkCmdDrawIndexedIndirect(
                cmd_buffer,
                draw_cmd_buffer,
                0,  // offset
                max_draws,
                sizeof(DrawIndexedIndirectCommand)
            );

            spdlog::debug("Indirect draw recorded with fallback (max {} draws)", max_draws);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats_.render_time_ms = std::chrono::duration<float, std::milli>(end - start).count();
}

void IndirectRenderer::bind_geometry_buffers(VkCommandBuffer cmd_buffer) {
    if (!initialized_) {
        return;
    }

    VkBuffer vertex_buf = vertex_buffer_.get_buffer();
    VkBuffer index_buf = index_buffer_.get_buffer();

    if (!vertex_buf || !index_buf) {
        spdlog::warn("Geometry buffers not available for binding");
        return;
    }

    // Bind merged vertex buffer
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertex_buf, &offset);

    // Bind merged index buffer
    vkCmdBindIndexBuffer(cmd_buffer, index_buf, 0, VK_INDEX_TYPE_UINT32);

    spdlog::debug("Geometry buffers bound for indirect rendering");
}

void IndirectRenderer::detect_indirect_count_support() {
    if (!device_) {
        supports_indirect_count_ = false;
        return;
    }

    // Try to get the function pointer
    // First try Vulkan 1.2 core function
    vkCmdDrawIndexedIndirectCountFn_ = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCount>(
        vkGetDeviceProcAddr(device_, "vkCmdDrawIndexedIndirectCount")
    );

    if (vkCmdDrawIndexedIndirectCountFn_) {
        supports_indirect_count_ = true;
        spdlog::debug("Vulkan 1.2 vkCmdDrawIndexedIndirectCount supported");
        return;
    }

    // Try the KHR extension version
    vkCmdDrawIndexedIndirectCountFn_ = reinterpret_cast<PFN_vkCmdDrawIndexedIndirectCount>(
        vkGetDeviceProcAddr(device_, "vkCmdDrawIndexedIndirectCountKHR")
    );

    if (vkCmdDrawIndexedIndirectCountFn_) {
        supports_indirect_count_ = true;
        spdlog::debug("VK_KHR_draw_indirect_count extension supported");
        return;
    }

    // No indirect count support
    supports_indirect_count_ = false;
    spdlog::info("vkCmdDrawIndexedIndirectCount not available, using fallback");
}

uint32_t IndirectRenderer::get_draw_count() const {
    if (!draw_count_buffer_.get_buffer()) return 0;

    // Map and read draw count
    const void* ptr = draw_count_buffer_.map();
    if (!ptr) return 0;

    uint32_t count;
    memcpy(&count, ptr, sizeof(uint32_t));
    draw_count_buffer_.unmap();

    return count;
}

} // namespace culling
} // namespace manim
