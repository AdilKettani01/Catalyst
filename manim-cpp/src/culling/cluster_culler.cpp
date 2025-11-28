#include "manim/culling/cluster_culler.hpp"
#include "manim/culling/frustum_culler.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <unordered_set>

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

ClusterCuller::ClusterCuller() = default;

ClusterCuller::~ClusterCuller() {
    cleanup();
}

void ClusterCuller::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t compute_queue_family
) {
    if (initialized_) {
        spdlog::warn("ClusterCuller already initialized");
        return;
    }

    device_ = device;
    physical_device_ = physical_device;
    memory_pool_ = &memory_pool;
    queue_family_ = compute_queue_family;

    // Detect mesh shading support
    detect_mesh_shading_support();

    // Allocate initial buffers
    allocate_buffers(config_.max_meshlets);

    // Create compute pipeline
    create_pipeline();

    initialized_ = true;
    spdlog::info("ClusterCuller initialized (mesh shading: {})",
                 mesh_shading_supported_ ? "supported" : "not supported");
}

void ClusterCuller::cleanup() {
    if (device_ == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device_);

    if (cull_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, cull_pipeline_, nullptr);
        cull_pipeline_ = VK_NULL_HANDLE;
    }

    if (cull_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, cull_pipeline_layout_, nullptr);
        cull_pipeline_layout_ = VK_NULL_HANDLE;
    }

    if (cull_desc_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, cull_desc_pool_, nullptr);
        cull_desc_pool_ = VK_NULL_HANDLE;
    }

    if (cull_desc_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, cull_desc_layout_, nullptr);
        cull_desc_layout_ = VK_NULL_HANDLE;
    }

    device_ = VK_NULL_HANDLE;
    initialized_ = false;
}

void ClusterCuller::detect_mesh_shading_support() {
    // Check for VK_EXT_mesh_shader or Vulkan 1.3+ mesh shading
    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    VkPhysicalDeviceMeshShaderPropertiesEXT mesh_props{};
    mesh_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
    props2.pNext = &mesh_props;

    vkGetPhysicalDeviceProperties2(physical_device_, &props2);

    // Check if mesh shader features are available
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_features{};
    mesh_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    features2.pNext = &mesh_features;

    vkGetPhysicalDeviceFeatures2(physical_device_, &features2);

    mesh_shading_supported_ = mesh_features.meshShader && mesh_features.taskShader;

    if (mesh_shading_supported_) {
        spdlog::info("Mesh shading supported - max output vertices: {}, max output primitives: {}",
                    mesh_props.maxMeshOutputVertices, mesh_props.maxMeshOutputPrimitives);
    } else {
        spdlog::info("Mesh shading not supported - cluster culling will use fallback");
    }
}

void ClusterCuller::allocate_buffers(uint32_t max_meshlets) {
    if (!memory_pool_) return;

    // Meshlet buffer
    VkDeviceSize meshlet_size = max_meshlets * sizeof(Meshlet);
    meshlet_buffer_ = memory_pool_->allocate_buffer(
        meshlet_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::DeviceLocal,
        MemoryUsage::Dynamic
    );

    // Object info buffer
    VkDeviceSize object_size = (max_meshlets / 64 + 1) * sizeof(MeshletObjectInfo);
    object_info_buffer_ = memory_pool_->allocate_buffer(
        object_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Visibility bitfield buffer
    VkDeviceSize visibility_size = ((max_meshlets + 31) / 32) * sizeof(uint32_t);
    visibility_buffer_ = memory_pool_->allocate_buffer(
        visibility_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Visible indices buffer
    VkDeviceSize indices_size = max_meshlets * sizeof(uint32_t);
    visible_indices_buffer_ = memory_pool_->allocate_buffer(
        indices_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    // Atomic counter
    atomic_counter_buffer_ = memory_pool_->allocate_buffer(
        sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::HostVisible,
        MemoryUsage::Dynamic
    );

    spdlog::debug("Allocated cluster culling buffers for {} meshlets", max_meshlets);
}

void ClusterCuller::create_pipeline() {
    // Load shader
    auto spirv = load_spirv_file("compute/culling/cluster_cull.comp.spv");
    if (spirv.empty()) {
        spdlog::debug("Cluster cull compute shader not found - using CPU fallback");
        return;
    }

    // Create shader module
    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
        spdlog::warn("Failed to create cluster cull shader module");
        return;
    }

    // Descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    // Binding 0: Meshlet data
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Visibility buffer
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 2: Visible indices
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 3: Atomic counter
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &cull_desc_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
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

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &cull_desc_pool_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = cull_desc_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &cull_desc_layout_;

    if (vkAllocateDescriptorSets(device_, &alloc_info, &cull_desc_set_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    // Update descriptor set
    update_descriptor_set();

    // Push constant range
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(ClusterCullPushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &cull_desc_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &cull_pipeline_layout_) != VK_SUCCESS) {
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
    pipeline_info.layout = cull_pipeline_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &cull_pipeline_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, shader_module, nullptr);
        return;
    }

    vkDestroyShaderModule(device_, shader_module, nullptr);
    spdlog::info("Cluster culling compute pipeline created");
}

void ClusterCuller::update_descriptor_set() {
    if (!meshlet_buffer_.get_buffer()) return;

    std::array<VkDescriptorBufferInfo, 4> buffer_infos{};

    buffer_infos[0].buffer = meshlet_buffer_.get_buffer();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = VK_WHOLE_SIZE;

    buffer_infos[1].buffer = visibility_buffer_.get_buffer();
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
        writes[i].dstSet = cull_desc_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void ClusterCuller::upload_meshlets(const std::vector<Meshlet>& meshlets) {
    if (meshlets.empty()) {
        meshlet_count_ = 0;
        return;
    }

    meshlet_count_ = static_cast<uint32_t>(meshlets.size());

    void* ptr = meshlet_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, meshlets.data(), meshlets.size() * sizeof(Meshlet));
        meshlet_buffer_.unmap();
    }

    stats_.total_meshlets = meshlet_count_;
}

void ClusterCuller::upload_object_info(const std::vector<MeshletObjectInfo>& objects) {
    if (objects.empty()) {
        object_count_ = 0;
        return;
    }

    object_count_ = static_cast<uint32_t>(objects.size());

    void* ptr = object_info_buffer_.map();
    if (ptr) {
        std::memcpy(ptr, objects.data(), objects.size() * sizeof(MeshletObjectInfo));
        object_info_buffer_.unmap();
    }
}

void ClusterCuller::cull(
    const math::Mat4& view_proj,
    const math::Vec3& camera_pos,
    VkCommandBuffer cmd_buffer
) {
    if (!initialized_ || meshlet_count_ == 0) return;

    if (cull_pipeline_ == VK_NULL_HANDLE) {
        // No GPU pipeline - CPU fallback would be called by user
        return;
    }

    // Clear atomic counter
    vkCmdFillBuffer(cmd_buffer, atomic_counter_buffer_.get_buffer(), 0, sizeof(uint32_t), 0);

    // Memory barrier
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
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, cull_pipeline_);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           cull_pipeline_layout_, 0, 1, &cull_desc_set_, 0, nullptr);

    // Extract frustum planes
    FrustumPlanes frustum = FrustumCuller::extract_frustum_planes(view_proj);

    // Push constants
    ClusterCullPushConstants constants{};
    constants.view_proj = view_proj;
    constants.camera_position = math::Vec4(camera_pos, 0.0f);

    for (int i = 0; i < 6; ++i) {
        constants.frustum_planes[i] = frustum.planes[i];
    }

    constants.meshlet_count = meshlet_count_;
    constants.flags = 0;
    if (config_.enable_cone_culling) constants.flags |= 0x01;
    if (config_.enable_frustum_culling) constants.flags |= 0x02;
    if (config_.enable_occlusion_culling) constants.flags |= 0x04;

    vkCmdPushConstants(cmd_buffer, cull_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                      0, sizeof(constants), &constants);

    // Dispatch
    uint32_t workgroups = (meshlet_count_ + 255) / 256;
    vkCmdDispatch(cmd_buffer, workgroups, 1, 1);

    // Barrier for output
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr
    );
}

uint32_t ClusterCuller::get_visible_count() {
    void* ptr = atomic_counter_buffer_.map();
    if (!ptr) return 0;

    uint32_t count = *static_cast<uint32_t*>(ptr);
    atomic_counter_buffer_.unmap();

    stats_.visible_meshlets = count;
    return count;
}

std::vector<uint32_t> ClusterCuller::get_visible_indices() {
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

bool ClusterCuller::test_sphere_frustum(
    const math::Vec4& sphere,
    const FrustumPlanes& frustum
) {
    math::Vec3 center(sphere.x, sphere.y, sphere.z);
    float radius = sphere.w;

    for (int i = 0; i < 6; ++i) {
        const math::Vec4& plane = frustum.planes[i];
        float dist = glm::dot(math::Vec3(plane), center) + plane.w;

        if (dist < -radius) {
            return false;  // Completely outside
        }
    }

    return true;
}

bool ClusterCuller::test_cone_backface(
    const math::Vec4& cone_apex,
    const math::Vec4& cone_axis_cutoff,
    const math::Vec3& camera_pos
) {
    math::Vec3 apex(cone_apex);
    math::Vec3 axis(cone_axis_cutoff);
    float cutoff = cone_axis_cutoff.w;

    // Direction from cone apex to camera
    math::Vec3 to_camera = glm::normalize(camera_pos - apex);

    // Dot product with cone axis
    float dot = glm::dot(to_camera, axis);

    // If camera is outside the cone (dot < cutoff), meshlet is backfacing
    return dot < cutoff;
}

std::vector<uint32_t> ClusterCuller::cull_cpu(
    const math::Mat4& view_proj,
    const math::Vec3& camera_pos,
    const std::vector<Meshlet>& meshlets
) {
    std::vector<uint32_t> visible;
    visible.reserve(meshlets.size());

    FrustumPlanes frustum = FrustumCuller::extract_frustum_planes(view_proj);

    for (uint32_t i = 0; i < meshlets.size(); ++i) {
        const auto& m = meshlets[i];

        // Cone culling (backface)
        if (config_.enable_cone_culling) {
            if (test_cone_backface(m.cone_apex, m.cone_axis_cutoff, camera_pos)) {
                continue;  // Backfacing
            }
        }

        // Frustum culling
        if (config_.enable_frustum_culling) {
            if (!test_sphere_frustum(m.bounding_sphere, frustum)) {
                continue;  // Outside frustum
            }
        }

        visible.push_back(i);
    }

    return visible;
}

// ============================================================================
// Meshlet Generation Utilities
// ============================================================================

std::vector<Meshlet> ClusterCuller::generate_meshlets(
    const std::vector<math::Vec3>& vertices,
    const std::vector<uint32_t>& indices,
    uint32_t max_vertices,
    uint32_t max_triangles
) {
    std::vector<Meshlet> meshlets;

    if (indices.size() < 3) return meshlets;

    // Simple greedy meshlet generation
    // For production, use meshoptimizer or similar library

    std::unordered_set<uint32_t> current_vertices;
    std::vector<uint32_t> current_indices;

    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;

    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        // Check if adding this triangle would exceed limits
        size_t new_vertex_count = current_vertices.size();
        if (current_vertices.find(i0) == current_vertices.end()) new_vertex_count++;
        if (current_vertices.find(i1) == current_vertices.end()) new_vertex_count++;
        if (current_vertices.find(i2) == current_vertices.end()) new_vertex_count++;

        bool exceeds_vertices = new_vertex_count > max_vertices;
        bool exceeds_triangles = (current_indices.size() / 3 + 1) > max_triangles;

        if (exceeds_vertices || exceeds_triangles) {
            // Finalize current meshlet
            if (!current_indices.empty()) {
                Meshlet m{};
                m.vertex_offset = vertex_offset;
                m.vertex_count = static_cast<uint32_t>(current_vertices.size());
                m.index_offset = index_offset;
                m.triangle_count = static_cast<uint32_t>(current_indices.size() / 3);

                // Compute bounding sphere
                m.bounding_sphere = compute_bounding_sphere(vertices, current_indices);

                // Cone - simplified, set to always visible
                m.cone_apex = math::Vec4(m.bounding_sphere.x, m.bounding_sphere.y, m.bounding_sphere.z, 0.0f);
                m.cone_axis_cutoff = math::Vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Always visible

                meshlets.push_back(m);

                vertex_offset += m.vertex_count;
                index_offset += m.triangle_count * 3;
            }

            current_vertices.clear();
            current_indices.clear();
        }

        // Add triangle to current meshlet
        current_vertices.insert(i0);
        current_vertices.insert(i1);
        current_vertices.insert(i2);
        current_indices.push_back(i0);
        current_indices.push_back(i1);
        current_indices.push_back(i2);
    }

    // Finalize last meshlet
    if (!current_indices.empty()) {
        Meshlet m{};
        m.vertex_offset = vertex_offset;
        m.vertex_count = static_cast<uint32_t>(current_vertices.size());
        m.index_offset = index_offset;
        m.triangle_count = static_cast<uint32_t>(current_indices.size() / 3);

        m.bounding_sphere = compute_bounding_sphere(vertices, current_indices);
        m.cone_apex = math::Vec4(m.bounding_sphere.x, m.bounding_sphere.y, m.bounding_sphere.z, 0.0f);
        m.cone_axis_cutoff = math::Vec4(0.0f, 0.0f, 1.0f, 1.0f);

        meshlets.push_back(m);
    }

    return meshlets;
}

math::Vec4 ClusterCuller::compute_bounding_sphere(
    const std::vector<math::Vec3>& vertices,
    const std::vector<uint32_t>& meshlet_indices
) {
    if (meshlet_indices.empty()) {
        return math::Vec4(0.0f);
    }

    // Compute center (average of all vertices)
    math::Vec3 center(0.0f);
    for (uint32_t idx : meshlet_indices) {
        if (idx < vertices.size()) {
            center += vertices[idx];
        }
    }
    center /= static_cast<float>(meshlet_indices.size());

    // Compute radius (max distance from center)
    float radius = 0.0f;
    for (uint32_t idx : meshlet_indices) {
        if (idx < vertices.size()) {
            float dist = glm::length(vertices[idx] - center);
            radius = std::max(radius, dist);
        }
    }

    return math::Vec4(center, radius);
}

void ClusterCuller::compute_cone(
    const std::vector<math::Vec3>& vertices,
    const std::vector<math::Vec3>& normals,
    const std::vector<uint32_t>& meshlet_indices,
    math::Vec4& out_apex,
    math::Vec4& out_axis_cutoff
) {
    if (meshlet_indices.empty() || normals.empty()) {
        out_apex = math::Vec4(0.0f);
        out_axis_cutoff = math::Vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Always visible
        return;
    }

    // Compute average normal
    math::Vec3 avg_normal(0.0f);
    for (uint32_t idx : meshlet_indices) {
        if (idx < normals.size()) {
            avg_normal += normals[idx];
        }
    }

    float len = glm::length(avg_normal);
    if (len < 1e-6f) {
        out_apex = math::Vec4(0.0f);
        out_axis_cutoff = math::Vec4(0.0f, 0.0f, 1.0f, 1.0f);
        return;
    }

    avg_normal /= len;

    // Compute cone angle (max deviation from average)
    float min_dot = 1.0f;
    for (uint32_t idx : meshlet_indices) {
        if (idx < normals.size()) {
            float dot = glm::dot(normals[idx], avg_normal);
            min_dot = std::min(min_dot, dot);
        }
    }

    // Compute center for apex
    math::Vec3 center(0.0f);
    for (uint32_t idx : meshlet_indices) {
        if (idx < vertices.size()) {
            center += vertices[idx];
        }
    }
    center /= static_cast<float>(meshlet_indices.size());

    out_apex = math::Vec4(center, 0.0f);
    out_axis_cutoff = math::Vec4(avg_normal, min_dot);
}

} // namespace culling
} // namespace manim
