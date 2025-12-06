/**
 * @file text_renderer.cpp
 * @brief GPU text rendering implementation
 */

#include "manim/renderer/text_renderer.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>
#include <array>
#include <cstring>

namespace manim {

namespace {

// Quad vertices: position (x,y) and UV (u,v)
struct QuadVertex {
    float x, y;
    float u, v;
};

const std::array<QuadVertex, 6> QUAD_VERTICES = {{
    // Triangle 1
    {0.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    // Triangle 2
    {0.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f}
}};

std::vector<char> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    return buffer;
}

VkShaderModule create_shader_module(VkDevice device, const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

} // anonymous namespace

TextRenderer::~TextRenderer() {
    shutdown();
}

bool TextRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkRenderPass render_pass,
    MemoryPool& pool,
    uint32_t subpass,
    VkSampleCountFlagBits msaa_samples
) {
    if (initialized_) return true;

    device_ = device;
    physical_device_ = physical_device;
    render_pass_ = render_pass;
    pool_ = &pool;
    subpass_ = subpass;
    msaa_samples_ = msaa_samples;

    if (!device_) {
        spdlog::warn("TextRenderer: No Vulkan device provided");
        return false;
    }

    // Initialize default uniforms
    uniforms_.projection = math::Mat4(1.0f);
    uniforms_.view = math::Mat4(1.0f);
    uniforms_.model = math::Mat4(1.0f);
    uniforms_.atlas_size = math::Vec2(2048, 2048);
    uniforms_.sdf_range = 4.0f;
    uniforms_.time = 0.0f;

    // Initialize default effects
    effects_.outline_color = math::Vec4(0, 0, 0, 1);
    effects_.glow_color = math::Vec4(1, 1, 1, 1);
    effects_.shadow_color = math::Vec4(0, 0, 0, 0.5f);
    effects_.shadow_offset = math::Vec2(0.02f, -0.02f);
    effects_.smoothing = 0.1f;
    effects_.gamma = 2.2f;

    // Create resources
    if (!load_shaders()) {
        spdlog::warn("TextRenderer: Failed to load shaders");
        return false;
    }

    if (!create_descriptors()) {
        spdlog::warn("TextRenderer: Failed to create descriptors");
        return false;
    }

    if (!create_pipeline()) {
        spdlog::warn("TextRenderer: Failed to create pipeline");
        return false;
    }

    if (!create_quad_buffer()) {
        spdlog::warn("TextRenderer: Failed to create quad buffer");
        return false;
    }

    initialized_ = true;
    spdlog::info("TextRenderer initialized successfully");
    return true;
}

void TextRenderer::shutdown() {
    if (!device_) return;

    vkDeviceWaitIdle(device_);

    if (pipeline_) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipeline_layout_) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
    if (descriptor_pool_) {
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
    if (descriptor_layout_) {
        vkDestroyDescriptorSetLayout(device_, descriptor_layout_, nullptr);
        descriptor_layout_ = VK_NULL_HANDLE;
    }
    if (atlas_sampler_) {
        vkDestroySampler(device_, atlas_sampler_, nullptr);
        atlas_sampler_ = VK_NULL_HANDLE;
    }
    if (vert_shader_) {
        vkDestroyShaderModule(device_, vert_shader_, nullptr);
        vert_shader_ = VK_NULL_HANDLE;
    }
    if (frag_shader_) {
        vkDestroyShaderModule(device_, frag_shader_, nullptr);
        frag_shader_ = VK_NULL_HANDLE;
    }

    initialized_ = false;
}

bool TextRenderer::load_shaders() {
    // Try to load compiled SPIR-V shaders from build directory
    // CMake compiles shaders to ${CMAKE_BINARY_DIR}/shaders/ which is typically build/shaders/
    // Priority order:
    // 1. Installed/package locations (highest priority for deployed builds)
    // 2. Build directory locations for development builds
    // 3. Legacy flat shader directory as fallback
    std::vector<std::string> vert_paths = {
        // Installed/package locations (highest priority)
        "shaders/vertex/sdf_text.vert.spv",
        // Build directory (standard CMake output)
        "build/shaders/vertex/sdf_text.vert.spv",
        // Development: running from build directory
        "../build/shaders/vertex/sdf_text.vert.spv",
        "../../build/shaders/vertex/sdf_text.vert.spv",
        // Legacy flat shader directory
        "shaders/sdf_text.vert.spv",
        "../shaders/sdf_text.vert.spv",
        // Source directory (may work with runtime compilation)
        "../shaders/vertex/sdf_text.vert.spv"
    };
    std::vector<std::string> frag_paths = {
        // Installed/package locations (highest priority)
        "shaders/fragment/sdf_text.frag.spv",
        // Build directory (standard CMake output)
        "build/shaders/fragment/sdf_text.frag.spv",
        // Development: running from build directory
        "../build/shaders/fragment/sdf_text.frag.spv",
        "../../build/shaders/fragment/sdf_text.frag.spv",
        // Legacy flat shader directory
        "shaders/sdf_text.frag.spv",
        "../shaders/sdf_text.frag.spv",
        // Source directory (may work with runtime compilation)
        "../shaders/fragment/sdf_text.frag.spv"
    };

    std::vector<char> vert_code, frag_code;
    std::string vert_found_path, frag_found_path;
    for (const auto& path : vert_paths) {
        vert_code = read_file(path);
        if (!vert_code.empty()) {
            vert_found_path = path;
            break;
        }
    }
    for (const auto& path : frag_paths) {
        frag_code = read_file(path);
        if (!frag_code.empty()) {
            frag_found_path = path;
            break;
        }
    }

    if (vert_code.empty() || frag_code.empty()) {
        spdlog::warn("TextRenderer: SPIR-V shaders not found, text rendering disabled");
        if (vert_code.empty()) {
            spdlog::warn("TextRenderer: Vertex shader not found. Searched paths:");
            for (const auto& path : vert_paths) {
                spdlog::warn("  - {}", path);
            }
        }
        if (frag_code.empty()) {
            spdlog::warn("TextRenderer: Fragment shader not found. Searched paths:");
            for (const auto& path : frag_paths) {
                spdlog::warn("  - {}", path);
            }
        }
        return false;
    }

    spdlog::debug("TextRenderer: Loaded vertex shader from '{}'", vert_found_path);
    spdlog::debug("TextRenderer: Loaded fragment shader from '{}'", frag_found_path);

    vert_shader_ = create_shader_module(device_, vert_code);
    frag_shader_ = create_shader_module(device_, frag_code);

    if (!vert_shader_ || !frag_shader_) {
        spdlog::error("TextRenderer: Failed to create shader modules");
        return false;
    }

    spdlog::debug("TextRenderer: Loaded SDF text shaders");
    return true;
}

bool TextRenderer::create_descriptors() {
    // Create sampler for atlas texture
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device_, &sampler_info, nullptr, &atlas_sampler_) != VK_SUCCESS) {
        return false;
    }

    // Descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

    // Binding 0: Uniform buffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Binding 1: Atlas texture
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = &atlas_sampler_;

    // Binding 2: Effects buffer
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_layout_) != VK_SUCCESS) {
        return false;
    }

    // Descriptor pool
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 2;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        return false;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &descriptor_set_) != VK_SUCCESS) {
        return false;
    }

    // Create uniform buffers
    uniform_buffer_ = pool_->allocate_buffer(
        sizeof(TextUniformBuffer),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    effects_buffer_ = pool_->allocate_buffer(
        sizeof(TextEffectsBuffer),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Update descriptor set with uniform buffers
    std::array<VkWriteDescriptorSet, 2> writes{};

    VkDescriptorBufferInfo uniform_info{};
    uniform_info.buffer = uniform_buffer_.get_buffer();
    uniform_info.offset = 0;
    uniform_info.range = sizeof(TextUniformBuffer);

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptor_set_;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &uniform_info;

    VkDescriptorBufferInfo effects_info{};
    effects_info.buffer = effects_buffer_.get_buffer();
    effects_info.offset = 0;
    effects_info.range = sizeof(TextEffectsBuffer);

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptor_set_;
    writes[1].dstBinding = 2;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].descriptorCount = 1;
    writes[1].pBufferInfo = &effects_info;

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    return true;
}

bool TextRenderer::create_pipeline() {
    if (!render_pass_) {
        spdlog::warn("TextRenderer: No render pass provided, skipping pipeline creation");
        return false;
    }

    // Shader stages
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_shader_;
    shader_stages[0].pName = "main";

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_shader_;
    shader_stages[1].pName = "main";

    // Vertex input
    std::array<VkVertexInputBindingDescription, 2> binding_descs{};

    // Binding 0: Quad vertices
    binding_descs[0].binding = 0;
    binding_descs[0].stride = sizeof(QuadVertex);
    binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding 1: Instance data (GlyphInstance)
    binding_descs[1].binding = 1;
    binding_descs[1].stride = sizeof(Text::GlyphInstance);
    binding_descs[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    std::array<VkVertexInputAttributeDescription, 6> attr_descs{};

    // Per-vertex attributes
    attr_descs[0].binding = 0;
    attr_descs[0].location = 0;
    attr_descs[0].format = VK_FORMAT_R32G32_SFLOAT;  // position
    attr_descs[0].offset = offsetof(QuadVertex, x);

    attr_descs[1].binding = 0;
    attr_descs[1].location = 1;
    attr_descs[1].format = VK_FORMAT_R32G32_SFLOAT;  // uv
    attr_descs[1].offset = offsetof(QuadVertex, u);

    // Per-instance attributes
    attr_descs[2].binding = 1;
    attr_descs[2].location = 2;
    attr_descs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // pos_size (vec4: x,y = pos, z,w = size)
    attr_descs[2].offset = offsetof(Text::GlyphInstance, position);

    attr_descs[3].binding = 1;
    attr_descs[3].location = 3;
    attr_descs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // uv_rect
    attr_descs[3].offset = offsetof(Text::GlyphInstance, uv_rect);

    attr_descs[4].binding = 1;
    attr_descs[4].location = 4;
    attr_descs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // color
    attr_descs[4].offset = offsetof(Text::GlyphInstance, color);

    attr_descs[5].binding = 1;
    attr_descs[5].location = 5;
    attr_descs[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // effects (outline_width, glow, shadow_offset, shadow_blur)
    attr_descs[5].offset = offsetof(Text::GlyphInstance, effects);

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descs.size());
    vertex_input.pVertexBindingDescriptions = binding_descs.data();
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_descs.size());
    vertex_input.pVertexAttributeDescriptions = attr_descs.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport (dynamic)
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling - must match the render pass sample count
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaa_samples_;

    // Depth/stencil (disabled for 2D text)
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    // Blending (alpha blending for text)
    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.logicOpEnable = VK_FALSE;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &blend_attachment;

    // Dynamic state
    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // Pipeline layout
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_layout_;

    if (vkCreatePipelineLayout(device_, &layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        return false;
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = subpass_;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS) {
        return false;
    }

    spdlog::debug("TextRenderer: Created graphics pipeline");
    return true;
}

bool TextRenderer::create_quad_buffer() {
    quad_vertex_buffer_ = pool_->allocate_buffer(
        sizeof(QUAD_VERTICES),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Static
    );

    // Upload quad vertices
    void* mapped = quad_vertex_buffer_.map();
    if (mapped) {
        std::memcpy(mapped, QUAD_VERTICES.data(), sizeof(QUAD_VERTICES));
        quad_vertex_buffer_.unmap();
    }

    return true;
}

void TextRenderer::set_atlas(const GPUImage& atlas) {
    if (current_atlas_ != &atlas) {
        current_atlas_ = &atlas;
        atlas_dirty_ = true;
        uniforms_.atlas_size = math::Vec2(
            static_cast<float>(atlas.get_width()),
            static_cast<float>(atlas.get_height())
        );
    }
}

void TextRenderer::update_atlas_descriptor() {
    if (!current_atlas_ || !current_atlas_->get_view()) return;

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = current_atlas_->get_view();
    image_info.sampler = atlas_sampler_;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set_;
    write.dstBinding = 1;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    atlas_dirty_ = false;
}

void TextRenderer::set_matrices(const math::Mat4& projection, const math::Mat4& view) {
    uniforms_.projection = projection;
    uniforms_.view = view;
}

void TextRenderer::set_default_effects(const TextEffectsBuffer& effects) {
    effects_ = effects;
}

void TextRenderer::begin_batch(VkCommandBuffer cmd) {
    if (!initialized_ || !pipeline_) return;

    // Update atlas descriptor if needed
    if (atlas_dirty_) {
        update_atlas_descriptor();
    }

    // Upload uniforms
    void* mapped = uniform_buffer_.map();
    if (mapped) {
        std::memcpy(mapped, &uniforms_, sizeof(uniforms_));
        uniform_buffer_.unmap();
    }

    mapped = effects_buffer_.map();
    if (mapped) {
        std::memcpy(mapped, &effects_, sizeof(effects_));
        effects_buffer_.unmap();
    }

    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // Bind descriptor set
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);

    // Bind quad vertex buffer
    VkBuffer vertex_buffers[] = {quad_vertex_buffer_.get_buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
}

void TextRenderer::render_text(VkCommandBuffer cmd, Text& text, const math::Mat4& model) {
    if (!initialized_ || !pipeline_) return;

    // Upload glyphs if needed
    upload_text_to_gpu(text);

    // Update model matrix in uniform buffer
    uniforms_.model = model;
    void* mapped = uniform_buffer_.map();
    if (mapped) {
        std::memcpy(mapped, &uniforms_, sizeof(uniforms_));
        uniform_buffer_.unmap();
    }

    // Bind instance buffer and issue draw
    const GPUBuffer* instance_buffer = text.get_glyph_buffer();
    if (instance_buffer && instance_buffer->get_buffer() != VK_NULL_HANDLE) {
        VkBuffer buffers[] = {instance_buffer->get_buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 1, 1, buffers, offsets);  // Binding 1 for instances

        uint32_t glyph_count = static_cast<uint32_t>(text.get_glyph_count());
        if (glyph_count > 0) {
            vkCmdDraw(cmd, 6, glyph_count, 0, 0);  // 6 vertices per quad, N instances
            spdlog::debug("TextRenderer: Rendered text '{}' with {} glyphs",
                          text.get_text(), glyph_count);
        }
    } else {
        spdlog::debug("TextRenderer: No glyph buffer for text '{}'", text.get_text());
    }
}

void TextRenderer::upload_text_to_gpu(Text& text) {
    if (pool_) {
        text.upload_glyphs_to_gpu(*pool_);
    }
}

void TextRenderer::end_batch(VkCommandBuffer cmd) {
    (void)cmd;
    // Nothing special needed for now
}

} // namespace manim
