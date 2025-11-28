// ComputeEngine implementation with GPU Bezier tessellation and CPU fallbacks.
#include "manim/core/compute_engine.hpp"
#include "manim/core/math.hpp"
#include "manim/renderer/vulkan_utils.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace manim {

void ComputePipeline::create(VkDevice device, const std::string& /*shader_path*/) {
    device_ = device;
}

void ComputePipeline::create_from_spirv(VkDevice device, const std::vector<uint32_t>& spirv,
                                        VkDescriptorSetLayout descriptor_layout,
                                        const std::vector<VkPushConstantRange>& push_constants) {
    device_ = device;
    descriptor_layout_ = descriptor_layout;
    owns_descriptor_layout_ = false;  // Layout is managed externally

    // Create shader module
    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    if (vkCreateShaderModule(device, &module_info, nullptr, &shader_module_) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module");
        return;
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_layout;
    layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size());
    layout_info.pPushConstantRanges = push_constants.empty() ? nullptr : push_constants.data();

    if (vkCreatePipelineLayout(device, &layout_info, nullptr, &layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create pipeline layout");
        vkDestroyShaderModule(device, shader_module_, nullptr);
        shader_module_ = VK_NULL_HANDLE;
        return;
    }

    // Create compute pipeline
    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = shader_module_;
    stage_info.pName = "main";

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = stage_info;
    pipeline_info.layout = layout_;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS) {
        spdlog::error("Failed to create compute pipeline");
        vkDestroyPipelineLayout(device, layout_, nullptr);
        vkDestroyShaderModule(device, shader_module_, nullptr);
        layout_ = VK_NULL_HANDLE;
        shader_module_ = VK_NULL_HANDLE;
        return;
    }

    spdlog::info("Successfully created compute pipeline");
}

void ComputePipeline::destroy() {
    if (pipeline_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }
    if (layout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, layout_, nullptr);
    }
    if (owns_descriptor_layout_ && descriptor_layout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptor_layout_, nullptr);
    }
    if (shader_module_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, shader_module_, nullptr);
    }
    pipeline_ = VK_NULL_HANDLE;
    layout_ = VK_NULL_HANDLE;
    descriptor_layout_ = VK_NULL_HANDLE;
    shader_module_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
}

void ComputeEngine::initialize(VkDevice device, VkQueue compute_queue, MemoryPool& memory_pool) {
    device_ = device;
    compute_queue_ = compute_queue;
    memory_pool_ = &memory_pool;

    if (device_ == VK_NULL_HANDLE) {
        return;
    }

    // Try to determine shader directory from common locations
    std::vector<std::string> shader_search_paths = {
        "./build/shaders",
        "../build/shaders",
        "../../build/shaders",
        "./shaders",
        "/usr/share/manim/shaders",
        "/usr/local/share/manim/shaders"
    };

    for (const auto& path : shader_search_paths) {
        if (std::filesystem::exists(path)) {
            shader_dir_ = path;
            spdlog::info("Found shader directory: {}", shader_dir_);
            break;
        }
    }

    create_command_pool();
    create_descriptor_pool();
    create_bezier_tessellation_pipeline();
    create_radix_sort_pipelines();
    create_matrix_multiply_pipeline();
    load_built_in_shaders();
}

void ComputeEngine::shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        // Wait for all operations to complete
        vkDeviceWaitIdle(device_);

        // Destroy Bezier pipeline
        bezier_pipeline_.destroy();
        if (bezier_descriptor_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, bezier_descriptor_layout_, nullptr);
            bezier_descriptor_layout_ = VK_NULL_HANDLE;
        }

        // Destroy radix sort pipelines
        float_convert_pipeline_.destroy();
        radix_histogram_pipeline_.destroy();
        prefix_sum_pipeline_.destroy();
        radix_scatter_pipeline_.destroy();
        if (radix_descriptor_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, radix_descriptor_layout_, nullptr);
            radix_descriptor_layout_ = VK_NULL_HANDLE;
        }

        // Destroy matrix multiply pipeline
        matrix_multiply_pipeline_.destroy();
        if (matrix_multiply_descriptor_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, matrix_multiply_descriptor_layout_, nullptr);
            matrix_multiply_descriptor_layout_ = VK_NULL_HANDLE;
        }

        if (descriptor_pool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        }
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
        }
        if (fence_ != VK_NULL_HANDLE) {
            vkDestroyFence(device_, fence_, nullptr);
        }
    }
    device_ = VK_NULL_HANDLE;
    compute_queue_ = VK_NULL_HANDLE;
    command_pool_ = VK_NULL_HANDLE;
    command_buffer_ = VK_NULL_HANDLE;
    fence_ = VK_NULL_HANDLE;
    descriptor_pool_ = VK_NULL_HANDLE;
    bezier_descriptor_set_ = VK_NULL_HANDLE;
    matrix_multiply_descriptor_set_ = VK_NULL_HANDLE;
    for (int i = 0; i < 4; ++i) radix_descriptor_sets_[i] = VK_NULL_HANDLE;
    pipelines_.clear();
}

template<typename Op>
void cpu_binary_op(const GPUBuffer& a, const GPUBuffer& b, GPUBuffer& result, uint32_t count, Op op) {
    const float* ap = static_cast<const float*>(a.map());
    const float* bp = static_cast<const float*>(b.map());
    float* rp = static_cast<float*>(result.map());
    if (!ap || !bp || !rp) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        rp[i] = op(ap[i], bp[i]);
    }
    result.unmap();
    a.unmap();
    b.unmap();
}

void ComputeEngine::vector_add(const GPUBuffer& a, const GPUBuffer& b, GPUBuffer& result, uint32_t count) {
    cpu_binary_op(a, b, result, count, [](float x, float y) { return x + y; });
}

void ComputeEngine::vector_sub(const GPUBuffer& a, const GPUBuffer& b, GPUBuffer& result, uint32_t count) {
    cpu_binary_op(a, b, result, count, [](float x, float y) { return x - y; });
}

void ComputeEngine::vector_mul(const GPUBuffer& a, const GPUBuffer& b, GPUBuffer& result, uint32_t count) {
    cpu_binary_op(a, b, result, count, [](float x, float y) { return x * y; });
}

void ComputeEngine::vector_scale(const GPUBuffer& a, float scalar, GPUBuffer& result, uint32_t count) {
    const float* ap = static_cast<const float*>(a.map());
    float* rp = static_cast<float*>(result.map());
    if (!ap || !rp) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        rp[i] = ap[i] * scalar;
    }
    result.unmap();
    a.unmap();
}

float ComputeEngine::vector_dot(const GPUBuffer& a, const GPUBuffer& b, uint32_t count) {
    const float* ap = static_cast<const float*>(a.map());
    const float* bp = static_cast<const float*>(b.map());
    if (!ap || !bp) {
        return 0.0f;
    }
    float acc = 0.0f;
    for (uint32_t i = 0; i < count; ++i) {
        acc += ap[i] * bp[i];
    }
    a.unmap();
    b.unmap();
    return acc;
}

void ComputeEngine::vector_cross(const GPUBuffer& a, const GPUBuffer& b, GPUBuffer& result, uint32_t count) {
    const math::Vec3* ap = static_cast<const math::Vec3*>(a.map());
    const math::Vec3* bp = static_cast<const math::Vec3*>(b.map());
    math::Vec3* rp = static_cast<math::Vec3*>(result.map());
    if (!ap || !bp || !rp) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        rp[i] = glm::cross(ap[i], bp[i]);
    }
    result.unmap();
    a.unmap();
    b.unmap();
}

void ComputeEngine::vector_normalize(const GPUBuffer& a, GPUBuffer& result, uint32_t count) {
    const math::Vec3* ap = static_cast<const math::Vec3*>(a.map());
    math::Vec3* rp = static_cast<math::Vec3*>(result.map());
    if (!ap || !rp) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        float len = glm::length(ap[i]);
        rp[i] = (len > 1e-6f) ? ap[i] / len : ap[i];
    }
    result.unmap();
    a.unmap();
}

void ComputeEngine::matrix_multiply(const GPUBuffer& a, const GPUBuffer& b, GPUBuffer& result, uint32_t m, uint32_t k, uint32_t n) {
    const float* ap = static_cast<const float*>(a.map());
    const float* bp = static_cast<const float*>(b.map());
    float* rp = static_cast<float*>(result.map());
    if (!ap || !bp || !rp) {
        return;
    }
    for (uint32_t row = 0; row < m; ++row) {
        for (uint32_t col = 0; col < n; ++col) {
            float sum = 0.0f;
            for (uint32_t i = 0; i < k; ++i) {
                sum += ap[row * k + i] * bp[i * n + col];
            }
            rp[row * n + col] = sum;
        }
    }
    result.unmap();
    a.unmap();
    b.unmap();
}

void ComputeEngine::transform_points(const glm::mat4& matrix, const GPUBuffer& points, GPUBuffer& result, uint32_t count) {
    const math::Vec3* src = static_cast<const math::Vec3*>(points.map());
    math::Vec3* dst = static_cast<math::Vec3*>(result.map());
    if (!src || !dst) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        dst[i] = math::Vec3(matrix * math::Vec4(src[i], 1.0f));
    }
    result.unmap();
    points.unmap();
}

void ComputeEngine::batch_transform(const GPUBuffer& matrices, const GPUBuffer& points, GPUBuffer& result, uint32_t num_objects, uint32_t points_per_object) {
    const glm::mat4* mats = static_cast<const glm::mat4*>(matrices.map());
    const math::Vec3* src = static_cast<const math::Vec3*>(points.map());
    math::Vec3* dst = static_cast<math::Vec3*>(result.map());
    if (!mats || !src || !dst) {
        return;
    }
    for (uint32_t obj = 0; obj < num_objects; ++obj) {
        for (uint32_t p = 0; p < points_per_object; ++p) {
            uint32_t idx = obj * points_per_object + p;
            dst[idx] = math::Vec3(mats[obj] * math::Vec4(src[idx], 1.0f));
        }
    }
    result.unmap();
    matrices.unmap();
    points.unmap();
}

void ComputeEngine::eval_bezier_curves(const GPUBuffer& control_points, const GPUBuffer& t_values, GPUBuffer& result, uint32_t num_curves, uint32_t samples_per_curve) {
    const math::Vec4* cp = static_cast<const math::Vec4*>(control_points.map());
    const float* tvals = static_cast<const float*>(t_values.map());
    math::Vec3* out = static_cast<math::Vec3*>(result.map());
    if (!cp || !tvals || !out) {
        return;
    }
    for (uint32_t curve = 0; curve < num_curves; ++curve) {
        const math::Vec3 c[4] = {
            math::Vec3(cp[curve * 4 + 0]),
            math::Vec3(cp[curve * 4 + 1]),
            math::Vec3(cp[curve * 4 + 2]),
            math::Vec3(cp[curve * 4 + 3]),
        };
        for (uint32_t s = 0; s < samples_per_curve; ++s) {
            float t = tvals[s];
            math::Vec3 p01 = glm::mix(c[0], c[1], t);
            math::Vec3 p12 = glm::mix(c[1], c[2], t);
            math::Vec3 p23 = glm::mix(c[2], c[3], t);
            math::Vec3 p012 = glm::mix(p01, p12, t);
            math::Vec3 p123 = glm::mix(p12, p23, t);
            out[curve * samples_per_curve + s] = glm::mix(p012, p123, t);
        }
    }
    result.unmap();
    control_points.unmap();
    t_values.unmap();
}

void ComputeEngine::tessellate_bezier(const GPUBuffer& control_points, GPUBuffer& vertices, GPUBuffer& indices, uint32_t num_curves, uint32_t segments_per_curve) {
    const math::Vec4* cp = static_cast<const math::Vec4*>(control_points.map());
    math::Vec3* vtx = static_cast<math::Vec3*>(vertices.map());
    uint32_t* idx = static_cast<uint32_t*>(indices.map());
    if (!cp || !vtx || !idx) {
        return;
    }
    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;
    for (uint32_t curve = 0; curve < num_curves; ++curve) {
        math::Vec3 c[4] = {
            math::Vec3(cp[curve * 4 + 0]),
            math::Vec3(cp[curve * 4 + 1]),
            math::Vec3(cp[curve * 4 + 2]),
            math::Vec3(cp[curve * 4 + 3]),
        };
        for (uint32_t s = 0; s <= segments_per_curve; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(segments_per_curve);
            math::Vec3 p01 = glm::mix(c[0], c[1], t);
            math::Vec3 p12 = glm::mix(c[1], c[2], t);
            math::Vec3 p23 = glm::mix(c[2], c[3], t);
            math::Vec3 p012 = glm::mix(p01, p12, t);
            math::Vec3 p123 = glm::mix(p12, p23, t);
            vtx[vertex_offset + s] = glm::mix(p012, p123, t);
        }
        for (uint32_t s = 0; s < segments_per_curve; ++s) {
            idx[index_offset + s * 2 + 0] = vertex_offset + s;
            idx[index_offset + s * 2 + 1] = vertex_offset + s + 1;
        }
        vertex_offset += segments_per_curve + 1;
        index_offset += segments_per_curve * 2;
    }
    vertices.unmap();
    indices.unmap();
    control_points.unmap();
}

std::vector<math::Vec3> ComputeEngine::tessellate_bezier_cpu(const std::vector<std::array<math::Vec3, 4>>& curves, uint32_t segments_per_curve) {
    if (curves.empty() || segments_per_curve == 0) {
        return {};
    }
    std::vector<math::Vec3> out;
    out.reserve(curves.size() * (segments_per_curve + 1));
    for (const auto& c : curves) {
        for (uint32_t s = 0; s <= segments_per_curve; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(segments_per_curve);
            math::Vec3 p01 = glm::mix(c[0], c[1], t);
            math::Vec3 p12 = glm::mix(c[1], c[2], t);
            math::Vec3 p23 = glm::mix(c[2], c[3], t);
            math::Vec3 p012 = glm::mix(p01, p12, t);
            math::Vec3 p123 = glm::mix(p12, p23, t);
            out.push_back(glm::mix(p012, p123, t));
        }
    }
    return out;
}

void ComputeEngine::copy_buffer(const GPUBuffer& src, GPUBuffer& dst, VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset) {
    const std::byte* sp = static_cast<const std::byte*>(src.map());
    std::byte* dp = static_cast<std::byte*>(dst.map());
    if (!sp || !dp) {
        return;
    }
    std::memcpy(dp + dst_offset, sp + src_offset, static_cast<size_t>(size));
    dst.unmap();
    src.unmap();
}

void ComputeEngine::dispatch(const ComputePipeline& /*pipeline*/, VkDescriptorSet /*descriptor_set*/, uint32_t /*group_count_x*/, uint32_t /*group_count_y*/, uint32_t /*group_count_z*/) {
    // Stub: real GPU compute not implemented
}

void ComputeEngine::submit_and_wait() {
    if (device_ == VK_NULL_HANDLE || command_buffer_ == VK_NULL_HANDLE || compute_queue_ == VK_NULL_HANDLE) {
        return;
    }
    vkEndCommandBuffer(command_buffer_);
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffer_;
    vkQueueSubmit(compute_queue_, 1, &submit, fence_);
    vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &fence_);
}

void ComputeEngine::begin_commands() {
    if (command_buffer_ == VK_NULL_HANDLE) {
        return;
    }
    vkResetCommandBuffer(command_buffer_, 0);
    auto begin_info = vulkan_utils::one_time_begin_info();
    vkBeginCommandBuffer(command_buffer_, &begin_info);
}

void ComputeEngine::end_commands() {
    // No-op; handled in submit_and_wait
}

void ComputeEngine::create_command_pool() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    VkCommandPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.queueFamilyIndex = 0;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device_, &pool, nullptr, &command_pool_);

    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = command_pool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(device_, &alloc, &command_buffer_);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device_, &fence_info, nullptr, &fence_);
}

void ComputeEngine::load_built_in_shaders() {
    // Built-in shaders are now loaded in create_bezier_tessellation_pipeline
}

std::vector<uint32_t> ComputeEngine::load_spirv_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::warn("Failed to open SPIR-V file: {}", path);
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    if (file_size == 0 || file_size % sizeof(uint32_t) != 0) {
        spdlog::warn("Invalid SPIR-V file size: {}", path);
        return {};
    }

    std::vector<uint32_t> spirv(file_size / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(spirv.data()), file_size);

    spdlog::info("Loaded SPIR-V shader: {} ({} bytes)", path, file_size);
    return spirv;
}

void ComputeEngine::create_descriptor_pool() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }

    // Pool sizes for compute shaders (SSBOs and push constants)
    // Increased capacity for radix sort and matrix multiply pipelines
    std::array<VkDescriptorPoolSize, 1> pool_sizes = {{
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128 }  // Enough for all pipelines
    }};

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 32;  // Increased for radix sort + matrix multiply
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor pool");
        descriptor_pool_ = VK_NULL_HANDLE;
    }
}

void ComputeEngine::create_bezier_tessellation_pipeline() {
    if (device_ == VK_NULL_HANDLE || shader_dir_.empty()) {
        spdlog::warn("Cannot create Bezier pipeline: device or shader dir not available");
        return;
    }

    std::string shader_path = shader_dir_ + "/bezier_tessellation.comp.spv";
    auto spirv = load_spirv_file(shader_path);
    if (spirv.empty()) {
        spdlog::warn("Bezier tessellation shader not found at: {}", shader_path);
        return;
    }

    // Create descriptor set layout (4 storage buffers)
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    // Binding 0: Control points (input)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Output vertices
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Output normals
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 3: Output tangents
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &bezier_descriptor_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create Bezier descriptor set layout");
        return;
    }

    // Allocate descriptor set
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool_;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &bezier_descriptor_layout_;

        if (vkAllocateDescriptorSets(device_, &alloc_info, &bezier_descriptor_set_) != VK_SUCCESS) {
            spdlog::error("Failed to allocate Bezier descriptor set");
            bezier_descriptor_set_ = VK_NULL_HANDLE;
        }
    }

    // Push constants for Bezier tessellation
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(BezierTessellationPushConstants);

    bezier_pipeline_.create_from_spirv(device_, spirv, bezier_descriptor_layout_, { push_constant });

    if (bezier_pipeline_.is_valid()) {
        spdlog::info("Bezier tessellation GPU pipeline created successfully");
    }
}

void ComputeEngine::tessellate_bezier_gpu(
    const GPUBuffer& control_points,
    GPUBuffer& vertices,
    uint32_t num_curves,
    uint32_t segments_per_curve,
    bool compute_normals,
    bool compute_tangents,
    GPUBuffer* normals,
    GPUBuffer* tangents
) {
    if (!bezier_pipeline_.is_valid() || device_ == VK_NULL_HANDLE) {
        spdlog::warn("GPU Bezier tessellation not available, falling back to CPU");
        return;
    }

    if (num_curves == 0 || segments_per_curve == 0) {
        return;
    }

    // Update descriptor set with buffer bindings
    std::array<VkDescriptorBufferInfo, 4> buffer_infos{};

    buffer_infos[0].buffer = control_points.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = vertices.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    // For normals and tangents, use the same buffer if not provided (will be written as dummy)
    buffer_infos[2].buffer = (normals && normals->get_buffer()) ? normals->get_buffer() : vertices.get_buffer();
    buffer_infos[2].offset = 0;
    buffer_infos[2].range = VK_WHOLE_SIZE;

    buffer_infos[3].buffer = (tangents && tangents->get_buffer()) ? tangents->get_buffer() : vertices.get_buffer();
    buffer_infos[3].offset = 0;
    buffer_infos[3].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = bezier_descriptor_set_;
        writes[i].dstBinding = i;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Begin command buffer
    begin_commands();

    // Bind pipeline
    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, bezier_pipeline_.get_pipeline());

    // Bind descriptor set
    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                           bezier_pipeline_.get_layout(), 0, 1, &bezier_descriptor_set_, 0, nullptr);

    // Push constants
    BezierTessellationPushConstants constants{};
    constants.num_curves = num_curves;
    constants.segments_per_curve = segments_per_curve;
    constants.compute_normals = compute_normals ? 1 : 0;
    constants.compute_tangents = compute_tangents ? 1 : 0;

    vkCmdPushConstants(command_buffer_, bezier_pipeline_.get_layout(),
                      VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);

    // Dispatch compute
    uint32_t total_vertices = num_curves * segments_per_curve;
    uint32_t workgroup_size = 256;
    uint32_t num_workgroups = (total_vertices + workgroup_size - 1) / workgroup_size;

    vkCmdDispatch(command_buffer_, num_workgroups, 1, 1);

    // Memory barrier to ensure compute writes are visible
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

    vkCmdPipelineBarrier(command_buffer_,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        0, 1, &barrier, 0, nullptr, 0, nullptr);

    // Submit and wait
    submit_and_wait();

    spdlog::debug("GPU tessellated {} curves with {} segments each", num_curves, segments_per_curve);
}

ComputePipeline& ComputeEngine::get_pipeline(const std::string& name) {
    return pipelines_[name];
}

template<typename T>
void ComputeEngine::fill_buffer(GPUBuffer& buffer, const T& value, uint32_t count) {
    T* ptr = static_cast<T*>(buffer.map());
    if (!ptr) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        ptr[i] = value;
    }
    buffer.unmap();
}

// Explicit template instantiation for fill_buffer
template void ComputeEngine::fill_buffer<float>(GPUBuffer& buffer, const float& value, uint32_t count);

// ========================================================================
// Radix Sort Pipeline Creation
// ========================================================================

void ComputeEngine::create_radix_sort_pipelines() {
    if (device_ == VK_NULL_HANDLE || shader_dir_.empty()) {
        spdlog::warn("Cannot create radix sort pipelines: device or shader dir not available");
        return;
    }

    // Create descriptor set layout for radix sort shaders
    // Most shaders need 1-3 storage buffers
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

    // Binding 0: Primary data buffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Secondary buffer (histogram, output, etc.)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Tertiary buffer (prefix sums for scatter)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &radix_descriptor_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create radix sort descriptor set layout");
        return;
    }

    // Allocate descriptor sets
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        std::array<VkDescriptorSetLayout, 4> layouts = {
            radix_descriptor_layout_, radix_descriptor_layout_,
            radix_descriptor_layout_, radix_descriptor_layout_
        };
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool_;
        alloc_info.descriptorSetCount = 4;
        alloc_info.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device_, &alloc_info, radix_descriptor_sets_) != VK_SUCCESS) {
            spdlog::error("Failed to allocate radix sort descriptor sets");
            for (int i = 0; i < 4; ++i) radix_descriptor_sets_[i] = VK_NULL_HANDLE;
        }
    }

    // Push constant ranges for each shader type
    VkPushConstantRange float_convert_push{};
    float_convert_push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    float_convert_push.offset = 0;
    float_convert_push.size = sizeof(FloatConvertPushConstants);

    VkPushConstantRange radix_push{};
    radix_push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    radix_push.offset = 0;
    radix_push.size = sizeof(RadixSortPushConstants);

    VkPushConstantRange prefix_push{};
    prefix_push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    prefix_push.offset = 0;
    prefix_push.size = sizeof(PrefixSumPushConstants);

    // Load and create float_to_sortable pipeline
    std::string float_convert_path = shader_dir_ + "/float_to_sortable.comp.spv";
    auto float_convert_spirv = load_spirv_file(float_convert_path);
    if (!float_convert_spirv.empty()) {
        float_convert_pipeline_.create_from_spirv(device_, float_convert_spirv,
                                                  radix_descriptor_layout_, { float_convert_push });
        if (float_convert_pipeline_.is_valid()) {
            spdlog::info("Float-to-sortable GPU pipeline created successfully");
        }
    }

    // Load and create radix_histogram pipeline
    std::string histogram_path = shader_dir_ + "/radix_histogram.comp.spv";
    auto histogram_spirv = load_spirv_file(histogram_path);
    if (!histogram_spirv.empty()) {
        radix_histogram_pipeline_.create_from_spirv(device_, histogram_spirv,
                                                    radix_descriptor_layout_, { radix_push });
        if (radix_histogram_pipeline_.is_valid()) {
            spdlog::info("Radix histogram GPU pipeline created successfully");
        }
    }

    // Load and create prefix_sum pipeline
    std::string prefix_path = shader_dir_ + "/prefix_sum.comp.spv";
    auto prefix_spirv = load_spirv_file(prefix_path);
    if (!prefix_spirv.empty()) {
        prefix_sum_pipeline_.create_from_spirv(device_, prefix_spirv,
                                               radix_descriptor_layout_, { prefix_push });
        if (prefix_sum_pipeline_.is_valid()) {
            spdlog::info("Prefix sum GPU pipeline created successfully");
        }
    }

    // Load and create radix_scatter pipeline
    std::string scatter_path = shader_dir_ + "/radix_scatter.comp.spv";
    auto scatter_spirv = load_spirv_file(scatter_path);
    if (!scatter_spirv.empty()) {
        radix_scatter_pipeline_.create_from_spirv(device_, scatter_spirv,
                                                  radix_descriptor_layout_, { radix_push });
        if (radix_scatter_pipeline_.is_valid()) {
            spdlog::info("Radix scatter GPU pipeline created successfully");
        }
    }
}

// ========================================================================
// Matrix Multiply Pipeline Creation
// ========================================================================

void ComputeEngine::create_matrix_multiply_pipeline() {
    if (device_ == VK_NULL_HANDLE || shader_dir_.empty()) {
        spdlog::warn("Cannot create matrix multiply pipeline: device or shader dir not available");
        return;
    }

    // Create descriptor set layout (2 storage buffers: input and output)
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

    // Binding 0: Input matrices
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Output results
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &matrix_multiply_descriptor_layout_) != VK_SUCCESS) {
        spdlog::error("Failed to create matrix multiply descriptor set layout");
        return;
    }

    // Allocate descriptor set
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool_;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &matrix_multiply_descriptor_layout_;

        if (vkAllocateDescriptorSets(device_, &alloc_info, &matrix_multiply_descriptor_set_) != VK_SUCCESS) {
            spdlog::error("Failed to allocate matrix multiply descriptor set");
            matrix_multiply_descriptor_set_ = VK_NULL_HANDLE;
        }
    }

    // Push constants
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(MatrixMultiplyPushConstants);

    // Load and create pipeline
    std::string shader_path = shader_dir_ + "/matrix_multiply.comp.spv";
    auto spirv = load_spirv_file(shader_path);
    if (!spirv.empty()) {
        matrix_multiply_pipeline_.create_from_spirv(device_, spirv,
                                                    matrix_multiply_descriptor_layout_, { push_constant });
        if (matrix_multiply_pipeline_.is_valid()) {
            spdlog::info("Matrix multiply GPU pipeline created successfully");
        }
    }
}

// ========================================================================
// GPU Radix Sort Implementation
// ========================================================================

void ComputeEngine::radix_sort_gpu(GPUBuffer& buffer, uint32_t num_elements) {
    if (!float_convert_pipeline_.is_valid() || !radix_histogram_pipeline_.is_valid() ||
        !prefix_sum_pipeline_.is_valid() || !radix_scatter_pipeline_.is_valid()) {
        spdlog::warn("Radix sort GPU pipelines not available");
        return;
    }

    if (num_elements == 0 || device_ == VK_NULL_HANDLE) {
        return;
    }

    spdlog::debug("GPU radix sort: {} elements", num_elements);

    // Allocate temporary buffers for ping-pong and histogram
    const size_t data_size = num_elements * sizeof(uint32_t);
    const uint32_t workgroup_size = 256;
    const uint32_t num_workgroups = (num_elements + workgroup_size - 1) / workgroup_size;
    const size_t histogram_size = num_workgroups * 16 * sizeof(uint32_t);
    const size_t prefix_size = 16 * sizeof(uint32_t);  // Global prefix sums (16 buckets)

    // Create temp buffer for ping-pong
    GPUBuffer temp_buffer;
    if (memory_pool_) {
        temp_buffer = memory_pool_->allocate_buffer(data_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            MemoryType::HostVisible, MemoryUsage::Dynamic);
    }

    // Create histogram buffer
    GPUBuffer histogram_buffer;
    if (memory_pool_) {
        histogram_buffer = memory_pool_->allocate_buffer(histogram_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::HostVisible, MemoryUsage::Dynamic);
    }

    // Create prefix sum buffer
    GPUBuffer prefix_buffer;
    if (memory_pool_) {
        prefix_buffer = memory_pool_->allocate_buffer(prefix_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::HostVisible, MemoryUsage::Dynamic);
    }

    if (temp_buffer.get_buffer() == VK_NULL_HANDLE ||
        histogram_buffer.get_buffer() == VK_NULL_HANDLE ||
        prefix_buffer.get_buffer() == VK_NULL_HANDLE) {
        spdlog::error("Failed to allocate temporary buffers for radix sort");
        return;
    }

    // Step 1: Convert floats to sortable uints
    {
        // Update descriptor set
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer.get_buffer();
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = radix_descriptor_sets_[0];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

        begin_commands();

        vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                         float_convert_pipeline_.get_pipeline());
        vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                               float_convert_pipeline_.get_layout(), 0, 1,
                               &radix_descriptor_sets_[0], 0, nullptr);

        FloatConvertPushConstants convert_push{};
        convert_push.num_elements = num_elements;
        convert_push.direction = 0;  // float -> sortable

        vkCmdPushConstants(command_buffer_, float_convert_pipeline_.get_layout(),
                          VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(convert_push), &convert_push);

        vkCmdDispatch(command_buffer_, num_workgroups, 1, 1);

        // Memory barrier
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command_buffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        submit_and_wait();
    }

    // Step 2: Radix sort passes (8 passes for 32-bit, 4 bits per pass)
    GPUBuffer* src = &buffer;
    GPUBuffer* dst = &temp_buffer;

    for (uint32_t pass = 0; pass < 8; ++pass) {
        uint32_t bit_shift = pass * 4;

        // 2a: Build histogram
        {
            std::array<VkDescriptorBufferInfo, 2> buffer_infos{};
            buffer_infos[0].buffer = src->get_buffer();
            buffer_infos[0].offset = 0;
            buffer_infos[0].range = VK_WHOLE_SIZE;
            buffer_infos[1].buffer = histogram_buffer.get_buffer();
            buffer_infos[1].offset = 0;
            buffer_infos[1].range = VK_WHOLE_SIZE;

            std::array<VkWriteDescriptorSet, 2> writes{};
            for (int i = 0; i < 2; ++i) {
                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = radix_descriptor_sets_[1];
                writes[i].dstBinding = i;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].descriptorCount = 1;
                writes[i].pBufferInfo = &buffer_infos[i];
            }
            vkUpdateDescriptorSets(device_, 2, writes.data(), 0, nullptr);

            begin_commands();

            vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                             radix_histogram_pipeline_.get_pipeline());
            vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                                   radix_histogram_pipeline_.get_layout(), 0, 1,
                                   &radix_descriptor_sets_[1], 0, nullptr);

            RadixSortPushConstants hist_push{};
            hist_push.num_elements = num_elements;
            hist_push.bit_shift = bit_shift;
            hist_push.num_workgroups = num_workgroups;

            vkCmdPushConstants(command_buffer_, radix_histogram_pipeline_.get_layout(),
                              VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(hist_push), &hist_push);

            vkCmdDispatch(command_buffer_, num_workgroups, 1, 1);

            VkMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(command_buffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

            submit_and_wait();
        }

        // 2b: Compute global prefix sums from histogram (CPU for simplicity)
        {
            uint32_t* hist = static_cast<uint32_t*>(histogram_buffer.map());
            uint32_t* prefix = static_cast<uint32_t*>(prefix_buffer.map());

            // Sum up all workgroup histograms into global counts
            std::array<uint32_t, 16> global_counts{};
            for (uint32_t wg = 0; wg < num_workgroups; ++wg) {
                for (uint32_t d = 0; d < 16; ++d) {
                    global_counts[d] += hist[wg * 16 + d];
                }
            }

            // Exclusive prefix sum
            uint32_t running_sum = 0;
            for (uint32_t d = 0; d < 16; ++d) {
                prefix[d] = running_sum;
                running_sum += global_counts[d];
            }

            prefix_buffer.unmap();
            histogram_buffer.unmap();
        }

        // 2c: Scatter elements to sorted positions
        {
            std::array<VkDescriptorBufferInfo, 3> buffer_infos{};
            buffer_infos[0].buffer = src->get_buffer();
            buffer_infos[0].offset = 0;
            buffer_infos[0].range = VK_WHOLE_SIZE;
            buffer_infos[1].buffer = dst->get_buffer();
            buffer_infos[1].offset = 0;
            buffer_infos[1].range = VK_WHOLE_SIZE;
            buffer_infos[2].buffer = prefix_buffer.get_buffer();
            buffer_infos[2].offset = 0;
            buffer_infos[2].range = VK_WHOLE_SIZE;

            std::array<VkWriteDescriptorSet, 3> writes{};
            for (int i = 0; i < 3; ++i) {
                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = radix_descriptor_sets_[3];
                writes[i].dstBinding = i;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].descriptorCount = 1;
                writes[i].pBufferInfo = &buffer_infos[i];
            }
            vkUpdateDescriptorSets(device_, 3, writes.data(), 0, nullptr);

            begin_commands();

            vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                             radix_scatter_pipeline_.get_pipeline());
            vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                                   radix_scatter_pipeline_.get_layout(), 0, 1,
                                   &radix_descriptor_sets_[3], 0, nullptr);

            RadixSortPushConstants scatter_push{};
            scatter_push.num_elements = num_elements;
            scatter_push.bit_shift = bit_shift;

            vkCmdPushConstants(command_buffer_, radix_scatter_pipeline_.get_layout(),
                              VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(scatter_push), &scatter_push);

            vkCmdDispatch(command_buffer_, num_workgroups, 1, 1);

            VkMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(command_buffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

            submit_and_wait();
        }

        // Swap src and dst for next pass
        std::swap(src, dst);
    }

    // After 8 passes, data is in src. If src != &buffer, copy back
    if (src != &buffer) {
        // Copy temp_buffer back to buffer
        copy_buffer(*src, buffer, data_size, 0, 0);
    }

    // Step 3: Convert sortable uints back to floats
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer.get_buffer();
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = radix_descriptor_sets_[0];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

        begin_commands();

        vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                         float_convert_pipeline_.get_pipeline());
        vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                               float_convert_pipeline_.get_layout(), 0, 1,
                               &radix_descriptor_sets_[0], 0, nullptr);

        FloatConvertPushConstants convert_push{};
        convert_push.num_elements = num_elements;
        convert_push.direction = 1;  // sortable -> float

        vkCmdPushConstants(command_buffer_, float_convert_pipeline_.get_layout(),
                          VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(convert_push), &convert_push);

        vkCmdDispatch(command_buffer_, num_workgroups, 1, 1);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command_buffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        submit_and_wait();
    }

    spdlog::debug("GPU radix sort completed");
}

// ========================================================================
// GPU Matrix Multiply Implementation
// ========================================================================

void ComputeEngine::dispatch_matrix_multiply(GPUBuffer& matrices, GPUBuffer& results,
                                             uint32_t num_matrices, uint32_t operation) {
    if (!matrix_multiply_pipeline_.is_valid()) {
        spdlog::warn("Matrix multiply GPU pipeline not available");
        return;
    }

    if (num_matrices == 0 || device_ == VK_NULL_HANDLE) {
        return;
    }

    spdlog::debug("GPU matrix multiply: {} matrices, op={}", num_matrices, operation);

    // Update descriptor set
    std::array<VkDescriptorBufferInfo, 2> buffer_infos{};
    buffer_infos[0].buffer = matrices.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;
    buffer_infos[1].buffer = results.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> writes{};
    for (int i = 0; i < 2; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = matrix_multiply_descriptor_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(device_, 2, writes.data(), 0, nullptr);

    begin_commands();

    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                     matrix_multiply_pipeline_.get_pipeline());
    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                           matrix_multiply_pipeline_.get_layout(), 0, 1,
                           &matrix_multiply_descriptor_set_, 0, nullptr);

    MatrixMultiplyPushConstants push{};
    push.num_matrices = num_matrices;
    push.operation = operation;

    vkCmdPushConstants(command_buffer_, matrix_multiply_pipeline_.get_layout(),
                      VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);

    const uint32_t workgroup_size = 64;
    uint32_t num_workgroups = (num_matrices + workgroup_size - 1) / workgroup_size;
    vkCmdDispatch(command_buffer_, num_workgroups, 1, 1);

    // Memory barrier
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        0, 1, &barrier, 0, nullptr, 0, nullptr);

    submit_and_wait();

    spdlog::debug("GPU matrix multiply completed");
}

// ========================================================================
// Global ComputeEngine Accessor Implementation
// ========================================================================

static std::unique_ptr<ComputeEngine> g_compute_engine;
static std::mutex g_compute_engine_mutex;

ComputeEngine& getGlobalComputeEngine() {
    std::lock_guard<std::mutex> lock(g_compute_engine_mutex);
    if (!g_compute_engine) {
        throw std::runtime_error("ComputeEngine not initialized - call initializeGlobalComputeEngine first");
    }
    return *g_compute_engine;
}

void initializeGlobalComputeEngine(VkDevice device, VkQueue queue, MemoryPool& pool) {
    std::lock_guard<std::mutex> lock(g_compute_engine_mutex);
    if (g_compute_engine) {
        spdlog::warn("Global ComputeEngine already initialized, shutting down old instance");
        g_compute_engine->shutdown();
    }
    g_compute_engine = std::make_unique<ComputeEngine>();
    g_compute_engine->initialize(device, queue, pool);
    spdlog::info("Global ComputeEngine initialized");
}

void shutdownGlobalComputeEngine() {
    std::lock_guard<std::mutex> lock(g_compute_engine_mutex);
    if (g_compute_engine) {
        g_compute_engine->shutdown();
        g_compute_engine.reset();
        spdlog::info("Global ComputeEngine shut down");
    }
}

bool isComputeEngineInitialized() {
    std::lock_guard<std::mutex> lock(g_compute_engine_mutex);
    return g_compute_engine != nullptr && g_compute_engine->is_gpu_compute_available();
}

} // namespace manim
