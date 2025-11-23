// Shader system implementation
#include "manim/renderer/shader_system.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace manim {

// ============================================================================
// ShaderModule Implementation
// ============================================================================

ShaderModule::~ShaderModule() {
    if (module_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, module_, nullptr);
    }
}

void ShaderModule::create_from_glsl(
    VkDevice device,
    const std::string& source,
    ShaderStage stage,
    const ShaderCompileOptions& options) {

    device_ = device;
    stage_ = stage;

    // TODO: Implement GLSL compilation to SPIR-V
    spdlog::warn("GLSL compilation not yet implemented");
}

void ShaderModule::create_from_spirv(
    VkDevice device,
    const std::vector<uint32_t>& spirv) {

    device_ = device;
    spirv_ = spirv;

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = spirv.size() * sizeof(uint32_t);
    create_info.pCode = spirv.data();

    if (vkCreateShaderModule(device, &create_info, nullptr, &module_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
}

void ShaderModule::load_from_file(
    VkDevice device,
    const std::filesystem::path& path,
    ShaderStage stage,
    const ShaderCompileOptions& options) {

    device_ = device;
    stage_ = stage;

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Shader file not found: " + path.string());
    }

    // Check if it's a SPIR-V file
    if (path.extension() == ".spv") {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        size_t file_size = file.tellg();
        std::vector<uint32_t> spirv(file_size / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(spirv.data()), file_size);
        create_from_spirv(device, spirv);
    } else {
        // Load as GLSL
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        create_from_glsl(device, buffer.str(), stage, options);
    }
}

VkShaderModule ShaderModule::get_module() const {
    return module_;
}

ShaderStage ShaderModule::get_stage() const {
    return stage_;
}

const std::vector<uint32_t>& ShaderModule::get_spirv() const {
    return spirv_;
}

VkPipelineShaderStageCreateInfo ShaderModule::get_stage_info() const {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = static_cast<VkShaderStageFlagBits>(get_vulkan_stage());
    info.module = module_;
    info.pName = "main";
    return info;
}

VkShaderStageFlagBits ShaderModule::get_vulkan_stage() const {
    switch (stage_) {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::TessellationControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStage::TessellationEvaluation:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderStage::Mesh:
            return VK_SHADER_STAGE_MESH_BIT_NV;
        case ShaderStage::Task:
            return VK_SHADER_STAGE_TASK_BIT_NV;
        case ShaderStage::RayGen:
            return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case ShaderStage::Intersection:
            return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case ShaderStage::AnyHit:
            return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case ShaderStage::ClosestHit:
            return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case ShaderStage::Miss:
            return VK_SHADER_STAGE_MISS_BIT_KHR;
        case ShaderStage::Callable:
            return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        default:
            return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

// ============================================================================
// ShaderPipeline Implementation
// ============================================================================

ShaderPipeline::~ShaderPipeline() {
    if (pipeline_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }
    if (layout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, layout_, nullptr);
    }
}

void ShaderPipeline::add_shader(std::shared_ptr<ShaderModule> shader) {
    shaders_.push_back(shader);
}

void ShaderPipeline::create_compute_pipeline(
    VkDevice device,
    const ComputePipelineConfig& config) {

    device_ = device;

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = config.descriptor_layouts.size();
    layout_info.pSetLayouts = config.descriptor_layouts.data();
    layout_info.pushConstantRangeCount = config.push_constants.size();
    layout_info.pPushConstantRanges = config.push_constants.data();

    if (vkCreatePipelineLayout(device, &layout_info, nullptr, &layout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = shaders_[0]->get_stage_info();
    pipeline_info.layout = layout_;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline");
    }
}

void ShaderPipeline::create_graphics_pipeline(
    VkDevice device,
    const GraphicsPipelineConfig& config) {

    device_ = device;

    // Collect shader stages
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (const auto& shader : shaders_) {
        stages.push_back(shader->get_stage_info());
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = config.descriptor_layouts.size();
    layout_info.pSetLayouts = config.descriptor_layouts.data();
    layout_info.pushConstantRangeCount = config.push_constants.size();
    layout_info.pPushConstantRanges = config.push_constants.data();

    if (vkCreatePipelineLayout(device, &layout_info, nullptr, &layout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = stages.size();
    pipeline_info.pStages = stages.data();
    pipeline_info.pVertexInputState = &config.vertex_input;
    pipeline_info.pInputAssemblyState = &config.input_assembly;
    pipeline_info.pViewportState = &config.viewport_state;
    pipeline_info.pRasterizationState = &config.rasterization;
    pipeline_info.pMultisampleState = &config.multisampling;
    pipeline_info.pDepthStencilState = &config.depth_stencil;
    pipeline_info.pColorBlendState = &config.color_blending;
    pipeline_info.pDynamicState = &config.dynamic_state;
    pipeline_info.layout = layout_;
    pipeline_info.renderPass = config.render_pass;
    pipeline_info.subpass = config.subpass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }
}

void ShaderPipeline::create_ray_tracing_pipeline(
    VkDevice device,
    const RayTracingPipelineConfig& config) {

    device_ = device;

    // TODO: Implement ray tracing pipeline creation
    spdlog::warn("Ray tracing pipeline creation not yet implemented");
}

void ShaderPipeline::create_mesh_pipeline(
    VkDevice device,
    const MeshPipelineConfig& config) {

    device_ = device;

    // TODO: Implement mesh pipeline creation
    spdlog::warn("Mesh pipeline creation not yet implemented");
}

void ShaderPipeline::bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, bind_point_, pipeline_);
}

void ShaderPipeline::bind_descriptor_sets(
    VkCommandBuffer cmd,
    const std::vector<VkDescriptorSet>& sets,
    const std::vector<uint32_t>& dynamic_offsets) {

    vkCmdBindDescriptorSets(
        cmd,
        bind_point_,
        layout_,
        0,
        sets.size(),
        sets.data(),
        dynamic_offsets.size(),
        dynamic_offsets.data()
    );
}

void ShaderPipeline::push_constants(
    VkCommandBuffer cmd,
    VkShaderStageFlags stages,
    uint32_t offset,
    uint32_t size,
    const void* data) {

    vkCmdPushConstants(cmd, layout_, stages, offset, size, data);
}

// ============================================================================
// ShaderManager Implementation
// ============================================================================

ShaderManager::ShaderManager(VkDevice device) : device_(device) {
    spdlog::info("Initializing shader manager");
}

ShaderManager::~ShaderManager() = default;

std::shared_ptr<ShaderModule> ShaderManager::load_shader(
    const std::filesystem::path& path,
    ShaderStage stage,
    const ShaderCompileOptions& options) {

    auto shader = std::make_shared<ShaderModule>();
    shader->load_from_file(device_, path, stage, options);
    shaders_.push_back(shader);
    return shader;
}

std::shared_ptr<ShaderModule> ShaderManager::create_shader(
    const std::string& source,
    ShaderStage stage,
    const ShaderCompileOptions& options) {

    auto shader = std::make_shared<ShaderModule>();
    shader->create_from_glsl(device_, source, stage, options);
    shaders_.push_back(shader);
    return shader;
}

std::shared_ptr<ShaderPipeline> ShaderManager::create_compute_pipeline(
    std::shared_ptr<ShaderModule> compute_shader,
    const ComputePipelineConfig& config) {

    auto pipeline = std::make_shared<ShaderPipeline>();
    pipeline->add_shader(compute_shader);
    pipeline->create_compute_pipeline(device_, config);
    pipelines_.push_back(pipeline);
    return pipeline;
}

std::shared_ptr<ShaderPipeline> ShaderManager::create_graphics_pipeline(
    const std::vector<std::shared_ptr<ShaderModule>>& shaders,
    const GraphicsPipelineConfig& config) {

    auto pipeline = std::make_shared<ShaderPipeline>();
    for (const auto& shader : shaders) {
        pipeline->add_shader(shader);
    }
    pipeline->create_graphics_pipeline(device_, config);
    pipelines_.push_back(pipeline);
    return pipeline;
}

void ShaderManager::reload_all() {
    // TODO: Implement shader hot-reloading
    spdlog::info("Shader hot-reload requested");
}

void ShaderManager::clear() {
    shaders_.clear();
    pipelines_.clear();
}

// ============================================================================
// BuiltInShaders Implementation
// ============================================================================

void BuiltInShaders::initialize(VkDevice device) {
    // TODO: Load built-in shaders from embedded resources
    spdlog::info("Initializing built-in shaders");
}

std::shared_ptr<ShaderPipeline> BuiltInShaders::get_pbr_pipeline() {
    // TODO: Return PBR pipeline
    return nullptr;
}

std::shared_ptr<ShaderPipeline> BuiltInShaders::get_unlit_pipeline() {
    // TODO: Return unlit pipeline
    return nullptr;
}

std::shared_ptr<ShaderPipeline> BuiltInShaders::get_wireframe_pipeline() {
    // TODO: Return wireframe pipeline
    return nullptr;
}

std::shared_ptr<ShaderPipeline> BuiltInShaders::get_shadow_pipeline() {
    // TODO: Return shadow pipeline
    return nullptr;
}

std::shared_ptr<ShaderPipeline> BuiltInShaders::get_post_process_pipeline(const std::string& effect) {
    // TODO: Return post-process pipeline
    return nullptr;
}

}  // namespace manim