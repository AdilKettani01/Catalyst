#include "manim/culling/software_rasterizer.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <cmath>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace manim {
namespace culling {

// Shader search paths
static const std::vector<std::string> SHADER_SEARCH_PATHS = {
    "./build/shaders",
    "../build/shaders",
    "../../build/shaders",
    "./shaders"
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
        spdlog::debug("Could not find shader: {}", filename);
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

SoftwareRasterizer::SoftwareRasterizer() = default;

SoftwareRasterizer::~SoftwareRasterizer() {
    cleanup();
}

void SoftwareRasterizer::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t queue_family
) {
    if (initialized_) {
        spdlog::warn("SoftwareRasterizer already initialized");
        return;
    }

    device_ = device;
    physical_device_ = physical_device;
    memory_pool_ = &memory_pool;
    queue_family_ = queue_family;

    // Create CPU depth buffer
    create_depth_buffer();

    // Create hierarchical mips
    if (config_.enable_hierarchical) {
        create_hierarchical_mips();
    }

    // Create GPU resources (optional)
    create_gpu_resources();

    initialized_ = true;
    spdlog::info("SoftwareRasterizer initialized ({}x{} depth buffer)",
                 config_.depth_buffer_width, config_.depth_buffer_height);
}

void SoftwareRasterizer::cleanup() {
    if (device_ == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device_);

    if (raster_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, raster_pipeline_, nullptr);
        raster_pipeline_ = VK_NULL_HANDLE;
    }

    if (raster_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, raster_pipeline_layout_, nullptr);
        raster_pipeline_layout_ = VK_NULL_HANDLE;
    }

    if (raster_desc_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, raster_desc_pool_, nullptr);
        raster_desc_pool_ = VK_NULL_HANDLE;
    }

    if (raster_desc_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, raster_desc_layout_, nullptr);
        raster_desc_layout_ = VK_NULL_HANDLE;
    }

    device_ = VK_NULL_HANDLE;
    initialized_ = false;
}

void SoftwareRasterizer::set_config(const SoftwareRasterizerConfig& config) {
    bool size_changed = (config_.depth_buffer_width != config.depth_buffer_width ||
                        config_.depth_buffer_height != config.depth_buffer_height);

    config_ = config;

    if (initialized_ && size_changed) {
        create_depth_buffer();
        if (config_.enable_hierarchical) {
            create_hierarchical_mips();
        }
    }
}

void SoftwareRasterizer::create_depth_buffer() {
    uint32_t size = config_.depth_buffer_width * config_.depth_buffer_height;
    depth_buffer_.resize(size);

    // Initialize to max depth (far plane)
    std::fill(depth_buffer_.begin(), depth_buffer_.end(), 1.0f);
}

void SoftwareRasterizer::create_hierarchical_mips() {
    hierarchical_mips_.clear();

    uint32_t w = config_.depth_buffer_width;
    uint32_t h = config_.depth_buffer_height;

    while (w > 1 || h > 1) {
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);

        hierarchical_mips_.emplace_back(w * h, 1.0f);
    }

    spdlog::debug("Created {} hierarchical mip levels", hierarchical_mips_.size());
}

void SoftwareRasterizer::update_hierarchical_mips() {
    if (hierarchical_mips_.empty()) return;

    const float* src = depth_buffer_.data();
    uint32_t src_w = config_.depth_buffer_width;
    uint32_t src_h = config_.depth_buffer_height;

    for (auto& mip : hierarchical_mips_) {
        uint32_t dst_w = std::max(1u, src_w / 2);
        uint32_t dst_h = std::max(1u, src_h / 2);

        float* dst = mip.data();

        for (uint32_t y = 0; y < dst_h; ++y) {
            for (uint32_t x = 0; x < dst_w; ++x) {
                uint32_t sx = x * 2;
                uint32_t sy = y * 2;

                // Sample 2x2 region and take MAX (conservative)
                float d00 = src[sy * src_w + sx];
                float d10 = (sx + 1 < src_w) ? src[sy * src_w + sx + 1] : d00;
                float d01 = (sy + 1 < src_h) ? src[(sy + 1) * src_w + sx] : d00;
                float d11 = (sx + 1 < src_w && sy + 1 < src_h) ?
                            src[(sy + 1) * src_w + sx + 1] : d00;

                dst[y * dst_w + x] = std::max({d00, d10, d01, d11});
            }
        }

        src = mip.data();
        src_w = dst_w;
        src_h = dst_h;
    }
}

void SoftwareRasterizer::create_gpu_resources() {
    if (!memory_pool_) return;

    // GPU depth buffer
    VkDeviceSize depth_size = config_.depth_buffer_width * config_.depth_buffer_height * sizeof(float);
    gpu_depth_buffer_ = memory_pool_->allocate_buffer(
        depth_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Triangle buffer (initial size, will grow as needed)
    VkDeviceSize triangle_size = 1024 * sizeof(RasterTriangle);
    triangle_buffer_ = memory_pool_->allocate_buffer(
        triangle_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Try to create compute pipeline (optional - may fail if shader not compiled)
    create_compute_pipeline();
}

void SoftwareRasterizer::create_compute_pipeline() {
    // Load shader
    auto spirv = load_spirv_file("compute/culling/software_raster.comp.spv");
    if (spirv.empty()) {
        spdlog::debug("Software raster compute shader not found - GPU rasterization disabled");
        return;
    }

    // Create shader module
    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::warn("Failed to create software raster shader module");
        return;
    }

    // Descriptor set layout
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

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &raster_desc_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    // Descriptor pool
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = 2;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &raster_desc_pool_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = raster_desc_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &raster_desc_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &raster_desc_set_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    // Push constant range
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(SoftwareRasterPushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &raster_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &raster_pipeline_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    // Compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = shader_module;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = raster_pipeline_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &raster_pipeline_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);

    // Update descriptor set
    std::array<VkDescriptorBufferInfo, 2> buffer_infos{};
    buffer_infos[0].buffer = triangle_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = gpu_depth_buffer_.get_buffer();
    buffer_infos[1].offset = 0;
    buffer_infos[1].range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> writes{};
    for (uint32_t i = 0; i < 2; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = raster_desc_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    spdlog::info("Software raster compute pipeline created");
}

// ============================================================================
// CPU Rasterization
// ============================================================================

void SoftwareRasterizer::begin_frame(
    const math::Mat4& view_proj,
    uint32_t viewport_width,
    uint32_t viewport_height
) {
    view_proj_ = view_proj;
    viewport_width_ = viewport_width;
    viewport_height_ = viewport_height;

    // Calculate scale from viewport to depth buffer
    depth_scale_x_ = static_cast<float>(config_.depth_buffer_width) / viewport_width;
    depth_scale_y_ = static_cast<float>(config_.depth_buffer_height) / viewport_height;

    // Clear depth buffer to far plane
    std::fill(depth_buffer_.begin(), depth_buffer_.end(), 1.0f);

    // Clear hierarchical mips
    for (auto& mip : hierarchical_mips_) {
        std::fill(mip.begin(), mip.end(), 1.0f);
    }

    // Reset stats
    stats_.triangles_rasterized = 0;
    stats_.aabbs_tested = 0;
    stats_.aabbs_culled = 0;
}

math::Vec4 SoftwareRasterizer::project_vertex(const math::Vec3& world_pos) const {
    math::Vec4 clip = view_proj_ * math::Vec4(world_pos, 1.0f);

    // Perspective divide
    if (clip.w <= 0.0f) {
        // Behind camera
        return math::Vec4(-1.0f, -1.0f, -1.0f, 0.0f);
    }

    float inv_w = 1.0f / clip.w;
    math::Vec4 ndc(clip.x * inv_w, clip.y * inv_w, clip.z * inv_w, inv_w);

    // Convert to depth buffer coordinates
    float screen_x = (ndc.x * 0.5f + 0.5f) * config_.depth_buffer_width;
    float screen_y = (ndc.y * 0.5f + 0.5f) * config_.depth_buffer_height;

    return math::Vec4(screen_x, screen_y, ndc.z, inv_w);
}

float SoftwareRasterizer::edge_function(const math::Vec2& a, const math::Vec2& b, const math::Vec2& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void SoftwareRasterizer::rasterize_triangles_cpu(const RasterTriangle* triangles, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        const auto& tri = triangles[i];

        // Project vertices
        math::Vec4 sv0 = project_vertex(math::Vec3(tri.v0));
        math::Vec4 sv1 = project_vertex(math::Vec3(tri.v1));
        math::Vec4 sv2 = project_vertex(math::Vec3(tri.v2));

        // Skip if any vertex is behind camera
        if (sv0.w <= 0.0f || sv1.w <= 0.0f || sv2.w <= 0.0f) continue;

#ifdef __AVX2__
        if (config_.use_simd) {
            rasterize_triangle_simd(sv0, sv1, sv2);
        } else {
            rasterize_triangle_scalar(sv0, sv1, sv2);
        }
#else
        rasterize_triangle_scalar(sv0, sv1, sv2);
#endif
    }

    stats_.triangles_rasterized += count;

    // Update hierarchical mips after rasterization
    if (config_.enable_hierarchical) {
        update_hierarchical_mips();
    }
}

void SoftwareRasterizer::rasterize_triangle_scalar(
    const math::Vec4& v0,
    const math::Vec4& v1,
    const math::Vec4& v2
) {
    // Get bounding box
    float min_x = std::max(0.0f, std::min({v0.x, v1.x, v2.x}));
    float max_x = std::min(static_cast<float>(config_.depth_buffer_width - 1),
                          std::max({v0.x, v1.x, v2.x}));
    float min_y = std::max(0.0f, std::min({v0.y, v1.y, v2.y}));
    float max_y = std::min(static_cast<float>(config_.depth_buffer_height - 1),
                          std::max({v0.y, v1.y, v2.y}));

    // Skip if degenerate
    if (min_x >= max_x || min_y >= max_y) return;

    // Triangle area (for barycentric interpolation)
    math::Vec2 p0(v0.x, v0.y);
    math::Vec2 p1(v1.x, v1.y);
    math::Vec2 p2(v2.x, v2.y);

    float area = edge_function(p0, p1, p2);
    if (std::abs(area) < 1e-6f) return;  // Degenerate triangle

    float inv_area = 1.0f / area;

    // Rasterize
    for (int y = static_cast<int>(min_y); y <= static_cast<int>(max_y); ++y) {
        for (int x = static_cast<int>(min_x); x <= static_cast<int>(max_x); ++x) {
            math::Vec2 p(x + 0.5f, y + 0.5f);

            // Barycentric coordinates
            float w0 = edge_function(p1, p2, p) * inv_area;
            float w1 = edge_function(p2, p0, p) * inv_area;
            float w2 = edge_function(p0, p1, p) * inv_area;

            // Check if inside triangle
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                // Interpolate depth
                float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;

                // Depth test (closer = smaller depth value)
                uint32_t idx = y * config_.depth_buffer_width + x;
                if (depth < depth_buffer_[idx]) {
                    depth_buffer_[idx] = depth;
                }
            }
        }
    }
}

#ifdef __AVX2__
void SoftwareRasterizer::rasterize_triangle_simd(
    const math::Vec4& v0,
    const math::Vec4& v1,
    const math::Vec4& v2
) {
    // Get bounding box
    float min_x = std::max(0.0f, std::min({v0.x, v1.x, v2.x}));
    float max_x = std::min(static_cast<float>(config_.depth_buffer_width - 1),
                          std::max({v0.x, v1.x, v2.x}));
    float min_y = std::max(0.0f, std::min({v0.y, v1.y, v2.y}));
    float max_y = std::min(static_cast<float>(config_.depth_buffer_height - 1),
                          std::max({v0.y, v1.y, v2.y}));

    // Skip if degenerate or too small
    if (min_x >= max_x || min_y >= max_y) return;

    // For small triangles, use scalar
    float width = max_x - min_x;
    if (width < 8.0f) {
        rasterize_triangle_scalar(v0, v1, v2);
        return;
    }

    // Triangle area
    math::Vec2 p0(v0.x, v0.y);
    math::Vec2 p1(v1.x, v1.y);
    math::Vec2 p2(v2.x, v2.y);

    float area = edge_function(p0, p1, p2);
    if (std::abs(area) < 1e-6f) return;

    float inv_area = 1.0f / area;

    // Edge equation coefficients
    // E01(x,y) = (p1.y - p0.y) * x - (p1.x - p0.x) * y + (p1.x * p0.y - p0.x * p1.y)
    float a01 = p1.y - p0.y;
    float b01 = -(p1.x - p0.x);
    float c01 = p1.x * p0.y - p0.x * p1.y;

    float a12 = p2.y - p1.y;
    float b12 = -(p2.x - p1.x);
    float c12 = p2.x * p1.y - p1.x * p2.y;

    float a20 = p0.y - p2.y;
    float b20 = -(p0.x - p2.x);
    float c20 = p0.x * p2.y - p2.x * p0.y;

    // SIMD constants
    __m256 va01 = _mm256_set1_ps(a01);
    __m256 vb01 = _mm256_set1_ps(b01);
    __m256 va12 = _mm256_set1_ps(a12);
    __m256 vb12 = _mm256_set1_ps(b12);
    __m256 va20 = _mm256_set1_ps(a20);
    __m256 vb20 = _mm256_set1_ps(b20);
    __m256 vinv_area = _mm256_set1_ps(inv_area);

    __m256 vz0 = _mm256_set1_ps(v0.z);
    __m256 vz1 = _mm256_set1_ps(v1.z);
    __m256 vz2 = _mm256_set1_ps(v2.z);

    __m256 vzero = _mm256_setzero_ps();
    __m256 x_offset = _mm256_set_ps(7.5f, 6.5f, 5.5f, 4.5f, 3.5f, 2.5f, 1.5f, 0.5f);

    int start_x = static_cast<int>(min_x) & ~7;  // Align to 8
    int end_x = static_cast<int>(max_x);

    for (int y = static_cast<int>(min_y); y <= static_cast<int>(max_y); ++y) {
        float fy = y + 0.5f;
        __m256 vy = _mm256_set1_ps(fy);

        // Edge equation y components
        float e01_y = b01 * fy + c01;
        float e12_y = b12 * fy + c12;
        float e20_y = b20 * fy + c20;

        for (int x = start_x; x <= end_x; x += 8) {
            __m256 vx = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(x)), x_offset);

            // Evaluate edge equations
            __m256 e01 = _mm256_add_ps(_mm256_mul_ps(va01, vx), _mm256_set1_ps(e01_y));
            __m256 e12 = _mm256_add_ps(_mm256_mul_ps(va12, vx), _mm256_set1_ps(e12_y));
            __m256 e20 = _mm256_add_ps(_mm256_mul_ps(va20, vx), _mm256_set1_ps(e20_y));

            // Inside test
            __m256 inside = _mm256_and_ps(
                _mm256_and_ps(_mm256_cmp_ps(e01, vzero, _CMP_GE_OQ),
                             _mm256_cmp_ps(e12, vzero, _CMP_GE_OQ)),
                _mm256_cmp_ps(e20, vzero, _CMP_GE_OQ)
            );

            int mask = _mm256_movemask_ps(inside);
            if (mask == 0) continue;

            // Barycentric coordinates
            __m256 w0 = _mm256_mul_ps(e12, vinv_area);
            __m256 w1 = _mm256_mul_ps(e20, vinv_area);
            __m256 w2 = _mm256_mul_ps(e01, vinv_area);

            // Interpolate depth
            __m256 depth = _mm256_add_ps(
                _mm256_add_ps(_mm256_mul_ps(w0, vz0), _mm256_mul_ps(w1, vz1)),
                _mm256_mul_ps(w2, vz2)
            );

            // Load current depth values
            uint32_t base_idx = y * config_.depth_buffer_width + x;
            __m256 current_depth = _mm256_loadu_ps(&depth_buffer_[base_idx]);

            // Depth test
            __m256 closer = _mm256_cmp_ps(depth, current_depth, _CMP_LT_OQ);
            __m256 update_mask = _mm256_and_ps(inside, closer);

            // Blend and store
            __m256 new_depth = _mm256_blendv_ps(current_depth, depth, update_mask);
            _mm256_storeu_ps(&depth_buffer_[base_idx], new_depth);
        }
    }
}
#else
void SoftwareRasterizer::rasterize_triangle_simd(
    const math::Vec4& v0,
    const math::Vec4& v1,
    const math::Vec4& v2
) {
    // Fallback to scalar if SIMD not available
    rasterize_triangle_scalar(v0, v1, v2);
}
#endif

void SoftwareRasterizer::rasterize_aabb_cpu(const math::Vec3& aabb_min, const math::Vec3& aabb_max) {
    // Create 12 triangles from AABB (2 per face)
    RasterTriangle triangles[12];

    // Front face (-Z)
    triangles[0] = {{aabb_min.x, aabb_min.y, aabb_min.z, 1}, {aabb_max.x, aabb_min.y, aabb_min.z, 1}, {aabb_max.x, aabb_max.y, aabb_min.z, 1}, 0, 0, {}};
    triangles[1] = {{aabb_min.x, aabb_min.y, aabb_min.z, 1}, {aabb_max.x, aabb_max.y, aabb_min.z, 1}, {aabb_min.x, aabb_max.y, aabb_min.z, 1}, 0, 0, {}};

    // Back face (+Z)
    triangles[2] = {{aabb_max.x, aabb_min.y, aabb_max.z, 1}, {aabb_min.x, aabb_min.y, aabb_max.z, 1}, {aabb_min.x, aabb_max.y, aabb_max.z, 1}, 0, 0, {}};
    triangles[3] = {{aabb_max.x, aabb_min.y, aabb_max.z, 1}, {aabb_min.x, aabb_max.y, aabb_max.z, 1}, {aabb_max.x, aabb_max.y, aabb_max.z, 1}, 0, 0, {}};

    // Left face (-X)
    triangles[4] = {{aabb_min.x, aabb_min.y, aabb_max.z, 1}, {aabb_min.x, aabb_min.y, aabb_min.z, 1}, {aabb_min.x, aabb_max.y, aabb_min.z, 1}, 0, 0, {}};
    triangles[5] = {{aabb_min.x, aabb_min.y, aabb_max.z, 1}, {aabb_min.x, aabb_max.y, aabb_min.z, 1}, {aabb_min.x, aabb_max.y, aabb_max.z, 1}, 0, 0, {}};

    // Right face (+X)
    triangles[6] = {{aabb_max.x, aabb_min.y, aabb_min.z, 1}, {aabb_max.x, aabb_min.y, aabb_max.z, 1}, {aabb_max.x, aabb_max.y, aabb_max.z, 1}, 0, 0, {}};
    triangles[7] = {{aabb_max.x, aabb_min.y, aabb_min.z, 1}, {aabb_max.x, aabb_max.y, aabb_max.z, 1}, {aabb_max.x, aabb_max.y, aabb_min.z, 1}, 0, 0, {}};

    // Bottom face (-Y)
    triangles[8] = {{aabb_min.x, aabb_min.y, aabb_max.z, 1}, {aabb_max.x, aabb_min.y, aabb_max.z, 1}, {aabb_max.x, aabb_min.y, aabb_min.z, 1}, 0, 0, {}};
    triangles[9] = {{aabb_min.x, aabb_min.y, aabb_max.z, 1}, {aabb_max.x, aabb_min.y, aabb_min.z, 1}, {aabb_min.x, aabb_min.y, aabb_min.z, 1}, 0, 0, {}};

    // Top face (+Y)
    triangles[10] = {{aabb_min.x, aabb_max.y, aabb_min.z, 1}, {aabb_max.x, aabb_max.y, aabb_min.z, 1}, {aabb_max.x, aabb_max.y, aabb_max.z, 1}, 0, 0, {}};
    triangles[11] = {{aabb_min.x, aabb_max.y, aabb_min.z, 1}, {aabb_max.x, aabb_max.y, aabb_max.z, 1}, {aabb_min.x, aabb_max.y, aabb_max.z, 1}, 0, 0, {}};

    rasterize_triangles_cpu(triangles, 12);
}

float SoftwareRasterizer::sample_depth(float x, float y, uint32_t mip_level) const {
    if (mip_level == 0) {
        int ix = static_cast<int>(x);
        int iy = static_cast<int>(y);
        ix = std::clamp(ix, 0, static_cast<int>(config_.depth_buffer_width - 1));
        iy = std::clamp(iy, 0, static_cast<int>(config_.depth_buffer_height - 1));
        return depth_buffer_[iy * config_.depth_buffer_width + ix];
    }

    if (mip_level > hierarchical_mips_.size()) {
        return 1.0f;
    }

    uint32_t mip_idx = mip_level - 1;
    uint32_t mip_w = config_.depth_buffer_width >> mip_level;
    uint32_t mip_h = config_.depth_buffer_height >> mip_level;

    float mx = x / static_cast<float>(1 << mip_level);
    float my = y / static_cast<float>(1 << mip_level);

    int ix = std::clamp(static_cast<int>(mx), 0, static_cast<int>(mip_w - 1));
    int iy = std::clamp(static_cast<int>(my), 0, static_cast<int>(mip_h - 1));

    return hierarchical_mips_[mip_idx][iy * mip_w + ix];
}

bool SoftwareRasterizer::is_occluded_cpu(const math::Vec3& aabb_min, const math::Vec3& aabb_max) const {
    stats_.aabbs_tested++;

    // Project all 8 corners
    math::Vec3 corners[8] = {
        {aabb_min.x, aabb_min.y, aabb_min.z},
        {aabb_max.x, aabb_min.y, aabb_min.z},
        {aabb_min.x, aabb_max.y, aabb_min.z},
        {aabb_max.x, aabb_max.y, aabb_min.z},
        {aabb_min.x, aabb_min.y, aabb_max.z},
        {aabb_max.x, aabb_min.y, aabb_max.z},
        {aabb_min.x, aabb_max.y, aabb_max.z},
        {aabb_max.x, aabb_max.y, aabb_max.z}
    };

    float screen_min_x = 1e10f;
    float screen_max_x = -1e10f;
    float screen_min_y = 1e10f;
    float screen_max_y = -1e10f;
    float nearest_depth = 1.0f;
    bool any_valid = false;

    for (int i = 0; i < 8; ++i) {
        math::Vec4 sv = project_vertex(corners[i]);
        if (sv.w <= 0.0f) continue;

        any_valid = true;
        screen_min_x = std::min(screen_min_x, sv.x);
        screen_max_x = std::max(screen_max_x, sv.x);
        screen_min_y = std::min(screen_min_y, sv.y);
        screen_max_y = std::max(screen_max_y, sv.y);
        nearest_depth = std::min(nearest_depth, sv.z);
    }

    // If no corners are valid, conservatively assume visible
    if (!any_valid) return false;

    // Clamp to depth buffer bounds
    screen_min_x = std::max(0.0f, screen_min_x);
    screen_max_x = std::min(static_cast<float>(config_.depth_buffer_width - 1), screen_max_x);
    screen_min_y = std::max(0.0f, screen_min_y);
    screen_max_y = std::min(static_cast<float>(config_.depth_buffer_height - 1), screen_max_y);

    // Select mip level based on screen extent
    float extent = std::max(screen_max_x - screen_min_x, screen_max_y - screen_min_y);
    uint32_t mip = 0;
    if (config_.enable_hierarchical && extent > 1.0f) {
        mip = std::min(static_cast<uint32_t>(std::log2(extent)),
                      static_cast<uint32_t>(hierarchical_mips_.size()));
    }

    // Sample depth buffer (use MAX depth for conservative test)
    float buffer_depth = 0.0f;
    buffer_depth = std::max(buffer_depth, sample_depth(screen_min_x, screen_min_y, mip));
    buffer_depth = std::max(buffer_depth, sample_depth(screen_max_x, screen_min_y, mip));
    buffer_depth = std::max(buffer_depth, sample_depth(screen_min_x, screen_max_y, mip));
    buffer_depth = std::max(buffer_depth, sample_depth(screen_max_x, screen_max_y, mip));
    buffer_depth = std::max(buffer_depth, sample_depth(
        (screen_min_x + screen_max_x) * 0.5f,
        (screen_min_y + screen_max_y) * 0.5f, mip));

    // Occluded if nearest object point is farther than depth buffer
    bool occluded = nearest_depth > buffer_depth;

    if (occluded) {
        stats_.aabbs_culled++;
    }

    return occluded;
}

bool SoftwareRasterizer::is_occluded_sphere_cpu(const math::Vec3& center, float radius) const {
    // Approximate sphere with AABB
    math::Vec3 aabb_min = center - math::Vec3(radius);
    math::Vec3 aabb_max = center + math::Vec3(radius);
    return is_occluded_cpu(aabb_min, aabb_max);
}

const std::vector<float>& SoftwareRasterizer::get_hierarchical_mip(uint32_t level) const {
    static std::vector<float> empty;

    if (level == 0) return depth_buffer_;
    if (level > hierarchical_mips_.size()) return empty;

    return hierarchical_mips_[level - 1];
}

// ============================================================================
// GPU Rasterization
// ============================================================================

void SoftwareRasterizer::upload_triangles(const std::vector<RasterTriangle>& triangles) {
    if (triangles.empty()) {
        triangle_count_ = 0;
        return;
    }

    triangle_count_ = static_cast<uint32_t>(triangles.size());

    void* ptr = triangle_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, triangles.data(), triangles.size() * sizeof(RasterTriangle));
        triangle_buffer_.unmap();
    }
}

void SoftwareRasterizer::rasterize_triangles_gpu(
    const math::Mat4& view_proj,
    VkCommandBuffer cmd_buffer
) {
    if (raster_pipeline_ == VK_NULL_HANDLE || triangle_count_ == 0) return;

    // Clear GPU depth buffer
    vkCmdFillBuffer(cmd_buffer, gpu_depth_buffer_.get_buffer(), 0, VK_WHOLE_SIZE, 0x3F800000);  // 1.0f as bits

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr
    );

    // Bind pipeline
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, raster_pipeline_);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           raster_pipeline_layout_, 0, 1, &raster_desc_set_, 0, nullptr);

    // Push constants
    SoftwareRasterPushConstants constants{};
    constants.view_proj = view_proj;
    constants.depth_width = config_.depth_buffer_width;
    constants.depth_height = config_.depth_buffer_height;
    constants.num_triangles = triangle_count_;
    constants.flags = 0;

    vkCmdPushConstants(cmd_buffer, raster_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                      0, sizeof(constants), &constants);

    // Dispatch
    uint32_t workgroups = (triangle_count_ + 255) / 256;
    vkCmdDispatch(cmd_buffer, workgroups, 1, 1);

    // Barrier for readback
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr
    );
}

void SoftwareRasterizer::download_depth_buffer() {
    void* ptr = gpu_depth_buffer_.map();
    if (ptr) {
        std::memcpy(depth_buffer_.data(), ptr, depth_buffer_.size() * sizeof(float));
        gpu_depth_buffer_.unmap();

        // Update hierarchical mips
        if (config_.enable_hierarchical) {
            update_hierarchical_mips();
        }
    }
}

} // namespace culling
} // namespace manim
