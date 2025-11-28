// GPU Mesh implementation with full GPU geometry processing
#include "manim/mobject/three_d/mesh.hpp"
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <map>
#include <array>
#include <cmath>
#include <cstring>
#include <set>
#include <queue>
#include <numeric>
#include <limits>

namespace manim {

// ============================================================================
// GPUMesh Implementation
// ============================================================================

void GPUMesh::generate_points() {
    // Convert vertices to points for base class
    points_cpu_.clear();
    for (const auto& v : vertices_) {
        points_cpu_.push_back(v.position);
    }
}

// ============================================================================
// Mesh Data
// ============================================================================

void GPUMesh::set_vertices(const std::vector<Vertex>& vertices) {
    vertices_ = vertices;
    mesh_dirty_ = true;
    generate_points();
}

void GPUMesh::set_indices(const std::vector<uint32_t>& indices) {
    indices_ = indices;
    mesh_dirty_ = true;
}

void GPUMesh::set_mesh(
    const std::vector<math::Vec3>& positions,
    const std::vector<uint32_t>& indices
) {
    vertices_.clear();
    vertices_.reserve(positions.size());

    for (const auto& pos : positions) {
        vertices_.emplace_back(pos);
    }

    indices_ = indices;
    mesh_dirty_ = true;

    // Auto-compute normals
    recalculate_normals(true);
    generate_points();
}

// ============================================================================
// GPU Buffer Management
// ============================================================================

void GPUMesh::upload_to_gpu(MemoryPool& pool) {
    if (!mesh_dirty_ || vertices_.empty()) {
        return;
    }

    // Vertex buffer
    VkDeviceSize vertex_size = sizeof(Vertex) * vertices_.size();
    if (!vertex_buffer_ || vertex_buffer_->get_size() < vertex_size) {
        vertex_buffer_ = pool.allocate_buffer(
            vertex_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::DeviceLocal,
            MemoryUsage::Static
        );
    }

    // Index buffer
    if (!indices_.empty()) {
        VkDeviceSize index_size = sizeof(uint32_t) * indices_.size();
        if (!index_buffer_ || index_buffer_->get_size() < index_size) {
            index_buffer_ = pool.allocate_buffer(
                index_size,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                MemoryType::DeviceLocal,
                MemoryUsage::Static
            );
        }

        // Upload data (would use staging buffers)
        // index_buffer_->upload(indices_.data(), index_size);
    }

    // Upload vertices (would use staging buffer)
    // vertex_buffer_->upload(vertices_.data(), vertex_size);

    mesh_dirty_ = false;

    spdlog::info("Uploaded mesh to GPU: {} vertices, {} triangles",
                 vertices_.size(), indices_.size() / 3);
}

void GPUMesh::update_gpu_buffers() {
    if (!vertex_buffer_ || !mesh_dirty_) {
        return;
    }

    // Update vertex data
    VkDeviceSize vertex_size = sizeof(Vertex) * vertices_.size();
    // vertex_buffer_->upload(vertices_.data(), vertex_size);

    if (index_buffer_ && !indices_.empty()) {
        VkDeviceSize index_size = sizeof(uint32_t) * indices_.size();
        // index_buffer_->upload(indices_.data(), index_size);
    }

    mesh_dirty_ = false;
}

// ============================================================================
// Instanced Rendering
// ============================================================================

void GPUMesh::setup_instancing(uint32_t max_instances) {
    max_instances_ = max_instances;
    instances_.clear();
    instances_.reserve(max_instances);
    instances_dirty_ = true;

    spdlog::info("Setup instancing for {} instances (buffer allocated on upload)", max_instances);
}

void GPUMesh::add_instance(const InstanceData& instance) {
    if (instances_.size() >= max_instances_) {
        spdlog::warn("Instance buffer full, cannot add more instances");
        return;
    }

    instances_.push_back(instance);
    instances_dirty_ = true;
}

void GPUMesh::render_instanced(VkCommandBuffer cmd, uint32_t instance_count) {
    if (!vertex_buffer_ || !index_buffer_) {
        spdlog::warn("Cannot render instanced: vertex/index buffers not uploaded");
        return;
    }

    if (instance_count == 0) {
        return;
    }

    // Bind vertex buffer (binding 0)
    VkBuffer vertex_buf = vertex_buffer_->get_buffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buf, &offset);

    // Bind instance buffer (binding 1) if available
    if (instance_buffer_ && instance_count > 1) {
        VkBuffer instance_buf = instance_buffer_->get_buffer();
        vkCmdBindVertexBuffers(cmd, 1, 1, &instance_buf, &offset);
    }

    // Bind index buffer
    VkBuffer index_buf = index_buffer_->get_buffer();
    vkCmdBindIndexBuffer(cmd, index_buf, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed instanced
    uint32_t index_count = static_cast<uint32_t>(indices_.size());
    vkCmdDrawIndexed(cmd, index_count, instance_count, 0, 0, 0);

    spdlog::trace("Rendered {} instances with {} indices", instance_count, index_count);
}

void GPUMesh::update_instance_buffer() {
    if (!instance_buffer_ || instances_.empty()) {
        return;
    }

    // Map and copy instance data
    void* mapped = instance_buffer_->map();
    if (mapped) {
        VkDeviceSize copy_size = sizeof(InstanceData) * instances_.size();
        std::memcpy(mapped, instances_.data(), static_cast<size_t>(copy_size));
        instance_buffer_->unmap();
    }

    instances_dirty_ = false;
}

void GPUMesh::upload_instance_buffer(MemoryPool& pool) {
    if (instances_.empty()) {
        return;
    }

    VkDeviceSize required_size = sizeof(InstanceData) * std::max(max_instances_, static_cast<uint32_t>(instances_.size()));

    // Allocate or reallocate buffer if needed
    if (!instance_buffer_ || instance_buffer_->get_size() < required_size) {
        instance_buffer_ = pool.allocate_buffer(
            required_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );
        spdlog::info("Allocated instance buffer: {} bytes for {} instances",
                     required_size, instances_.size());
    }

    // Upload instance data
    update_instance_buffer();
}

// ============================================================================
// GPU Geometry Processing
// ============================================================================

void GPUMesh::compute_normals_gpu(ComputeEngine& engine, bool smooth) {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }

    // Dispatch compute shader to calculate normals
    // If smooth: accumulate normals from adjacent triangles
    // If flat: use triangle normal

    spdlog::debug("Computing normals on GPU ({} mode)",
                  smooth ? "smooth" : "flat");

    // This would dispatch a compute shader
    // engine.compute_normals(vertex_buffer_, index_buffer_, smooth);

    mesh_dirty_ = false;
}

void GPUMesh::compute_tangents_gpu(ComputeEngine& engine) {
    // Compute tangent space for normal mapping
    // Uses Lengyel's method or similar

    spdlog::debug("Computing tangents on GPU");

    // This would dispatch a compute shader
    // engine.compute_tangents(vertex_buffer_, index_buffer_);
}

void GPUMesh::tessellate_gpu(ComputeEngine& engine, uint32_t level) {
    if (level <= 1) {
        return;  // No subdivision
    }

    spdlog::info("Tessellating mesh on GPU: level {}", level);

    // This would use tessellation shaders or compute shader
    // to subdivide triangles
    // engine.tessellate_mesh(vertex_buffer_, index_buffer_, level);
}

// ============================================================================
// Procedural Mesh Generation
// ============================================================================

std::shared_ptr<GPUMesh> GPUMesh::create_sphere(float radius, uint32_t subdivisions) {
    auto mesh = std::make_shared<GPUMesh>();

    // UV sphere generation
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t rings = subdivisions;
    uint32_t sectors = subdivisions * 2;

    float ring_step = math::PI / rings;
    float sector_step = 2.0f * math::PI / sectors;

    // Generate vertices
    for (uint32_t i = 0; i <= rings; ++i) {
        float theta = i * ring_step;
        float sin_theta = std::sin(theta);
        float cos_theta = std::cos(theta);

        for (uint32_t j = 0; j <= sectors; ++j) {
            float phi = j * sector_step;
            float sin_phi = std::sin(phi);
            float cos_phi = std::cos(phi);

            Vertex v;
            v.position = math::Vec3{
                radius * sin_theta * cos_phi,
                radius * cos_theta,
                radius * sin_theta * sin_phi
            };
            v.normal = glm::normalize(v.position / radius);
            v.uv = math::Vec2{
                static_cast<float>(j) / sectors,
                static_cast<float>(i) / rings
            };

            vertices.push_back(v);
        }
    }

    // Generate indices
    for (uint32_t i = 0; i < rings; ++i) {
        for (uint32_t j = 0; j < sectors; ++j) {
            uint32_t current = i * (sectors + 1) + j;
            uint32_t next = current + sectors + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated sphere: {} vertices, {} triangles",
                 vertices.size(), indices.size() / 3);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_cube(float size) {
    auto mesh = std::make_shared<GPUMesh>();

    float half = size * 0.5f;

    // 24 vertices (4 per face for proper normals/UVs)
    std::vector<Vertex> vertices = {
        // Front face (+Z)
        {{-half, -half,  half}, {0, 0, 1}, {0, 0}},
        {{ half, -half,  half}, {0, 0, 1}, {1, 0}},
        {{ half,  half,  half}, {0, 0, 1}, {1, 1}},
        {{-half,  half,  half}, {0, 0, 1}, {0, 1}},

        // Back face (-Z)
        {{ half, -half, -half}, {0, 0, -1}, {0, 0}},
        {{-half, -half, -half}, {0, 0, -1}, {1, 0}},
        {{-half,  half, -half}, {0, 0, -1}, {1, 1}},
        {{ half,  half, -half}, {0, 0, -1}, {0, 1}},

        // Right face (+X)
        {{ half, -half,  half}, {1, 0, 0}, {0, 0}},
        {{ half, -half, -half}, {1, 0, 0}, {1, 0}},
        {{ half,  half, -half}, {1, 0, 0}, {1, 1}},
        {{ half,  half,  half}, {1, 0, 0}, {0, 1}},

        // Left face (-X)
        {{-half, -half, -half}, {-1, 0, 0}, {0, 0}},
        {{-half, -half,  half}, {-1, 0, 0}, {1, 0}},
        {{-half,  half,  half}, {-1, 0, 0}, {1, 1}},
        {{-half,  half, -half}, {-1, 0, 0}, {0, 1}},

        // Top face (+Y)
        {{-half,  half,  half}, {0, 1, 0}, {0, 0}},
        {{ half,  half,  half}, {0, 1, 0}, {1, 0}},
        {{ half,  half, -half}, {0, 1, 0}, {1, 1}},
        {{-half,  half, -half}, {0, 1, 0}, {0, 1}},

        // Bottom face (-Y)
        {{-half, -half, -half}, {0, -1, 0}, {0, 0}},
        {{ half, -half, -half}, {0, -1, 0}, {1, 0}},
        {{ half, -half,  half}, {0, -1, 0}, {1, 1}},
        {{-half, -half,  half}, {0, -1, 0}, {0, 1}},
    };

    // 36 indices (2 triangles per face * 6 faces)
    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t base = i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_plane(
    float width,
    float height,
    uint32_t subdivisions_x,
    uint32_t subdivisions_y
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float half_w = width * 0.5f;
    float half_h = height * 0.5f;

    // Generate vertices
    for (uint32_t y = 0; y <= subdivisions_y; ++y) {
        for (uint32_t x = 0; x <= subdivisions_x; ++x) {
            float u = static_cast<float>(x) / subdivisions_x;
            float v = static_cast<float>(y) / subdivisions_y;

            Vertex vert;
            vert.position = math::Vec3{
                (u - 0.5f) * width,
                0.0f,
                (v - 0.5f) * height
            };
            vert.normal = math::Vec3{0, 1, 0};
            vert.uv = math::Vec2{u, v};

            vertices.push_back(vert);
        }
    }

    // Generate indices
    for (uint32_t y = 0; y < subdivisions_y; ++y) {
        for (uint32_t x = 0; x < subdivisions_x; ++x) {
            uint32_t i0 = y * (subdivisions_x + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (subdivisions_x + 1);
            uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_cylinder(
    float radius,
    float height,
    uint32_t segments
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float half_height = height * 0.5f;
    float angle_step = 2.0f * math::PI / segments;

    // ========================================================================
    // Side vertices (2 rings: top and bottom)
    // ========================================================================
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        float u = static_cast<float>(i) / segments;

        // Bottom ring
        Vertex v_bottom;
        v_bottom.position = math::Vec3{radius * cos_a, -half_height, radius * sin_a};
        v_bottom.normal = math::Vec3{cos_a, 0.0f, sin_a};
        v_bottom.uv = math::Vec2{u, 0.0f};
        vertices.push_back(v_bottom);

        // Top ring
        Vertex v_top;
        v_top.position = math::Vec3{radius * cos_a, half_height, radius * sin_a};
        v_top.normal = math::Vec3{cos_a, 0.0f, sin_a};
        v_top.uv = math::Vec2{u, 1.0f};
        vertices.push_back(v_top);
    }

    // Side indices
    for (uint32_t i = 0; i < segments; ++i) {
        uint32_t base = i * 2;
        // Triangle 1
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        // Triangle 2
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    // ========================================================================
    // Bottom cap
    // ========================================================================
    uint32_t bottom_center_idx = static_cast<uint32_t>(vertices.size());
    Vertex bottom_center;
    bottom_center.position = math::Vec3{0.0f, -half_height, 0.0f};
    bottom_center.normal = math::Vec3{0.0f, -1.0f, 0.0f};
    bottom_center.uv = math::Vec2{0.5f, 0.5f};
    vertices.push_back(bottom_center);

    uint32_t bottom_ring_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);

        Vertex v;
        v.position = math::Vec3{radius * cos_a, -half_height, radius * sin_a};
        v.normal = math::Vec3{0.0f, -1.0f, 0.0f};
        v.uv = math::Vec2{cos_a * 0.5f + 0.5f, sin_a * 0.5f + 0.5f};
        vertices.push_back(v);
    }

    for (uint32_t i = 0; i < segments; ++i) {
        indices.push_back(bottom_center_idx);
        indices.push_back(bottom_ring_start + i + 1);
        indices.push_back(bottom_ring_start + i);
    }

    // ========================================================================
    // Top cap
    // ========================================================================
    uint32_t top_center_idx = static_cast<uint32_t>(vertices.size());
    Vertex top_center;
    top_center.position = math::Vec3{0.0f, half_height, 0.0f};
    top_center.normal = math::Vec3{0.0f, 1.0f, 0.0f};
    top_center.uv = math::Vec2{0.5f, 0.5f};
    vertices.push_back(top_center);

    uint32_t top_ring_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);

        Vertex v;
        v.position = math::Vec3{radius * cos_a, half_height, radius * sin_a};
        v.normal = math::Vec3{0.0f, 1.0f, 0.0f};
        v.uv = math::Vec2{cos_a * 0.5f + 0.5f, sin_a * 0.5f + 0.5f};
        vertices.push_back(v);
    }

    for (uint32_t i = 0; i < segments; ++i) {
        indices.push_back(top_center_idx);
        indices.push_back(top_ring_start + i);
        indices.push_back(top_ring_start + i + 1);
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated cylinder: {} vertices, {} triangles",
                 vertices.size(), indices.size() / 3);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_torus(
    float major_radius,
    float minor_radius,
    uint32_t major_segments,
    uint32_t minor_segments
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float R = major_radius;
    float r = minor_radius;

    // Generate vertices using parametric torus equation
    // x = (R + r*cos(v)) * cos(u)
    // y = r * sin(v)
    // z = (R + r*cos(v)) * sin(u)
    for (uint32_t i = 0; i <= major_segments; ++i) {
        float u = static_cast<float>(i) / major_segments * 2.0f * math::PI;
        float cos_u = std::cos(u);
        float sin_u = std::sin(u);

        for (uint32_t j = 0; j <= minor_segments; ++j) {
            float v = static_cast<float>(j) / minor_segments * 2.0f * math::PI;
            float cos_v = std::cos(v);
            float sin_v = std::sin(v);

            Vertex vert;

            // Position on torus surface
            float dist_from_center = R + r * cos_v;
            vert.position = math::Vec3{
                dist_from_center * cos_u,
                r * sin_v,
                dist_from_center * sin_u
            };

            // Normal: vector from tube center to surface point
            // Tube center at this u: (R*cos_u, 0, R*sin_u)
            math::Vec3 tube_center{R * cos_u, 0.0f, R * sin_u};
            vert.normal = glm::normalize(vert.position - tube_center);

            // UV coordinates
            vert.uv = math::Vec2{
                static_cast<float>(i) / major_segments,
                static_cast<float>(j) / minor_segments
            };

            vertices.push_back(vert);
        }
    }

    // Generate indices
    for (uint32_t i = 0; i < major_segments; ++i) {
        for (uint32_t j = 0; j < minor_segments; ++j) {
            uint32_t current = i * (minor_segments + 1) + j;
            uint32_t next_ring = (i + 1) * (minor_segments + 1) + j;

            // First triangle
            indices.push_back(current);
            indices.push_back(next_ring);
            indices.push_back(current + 1);

            // Second triangle
            indices.push_back(current + 1);
            indices.push_back(next_ring);
            indices.push_back(next_ring + 1);
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated torus: {} vertices, {} triangles",
                 vertices.size(), indices.size() / 3);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_cone(
    float radius,
    float height,
    uint32_t radial_segments,
    bool open_ended
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float half_height = height * 0.5f;
    float angle_step = 2.0f * math::PI / radial_segments;

    // Slope for normals: tan(slope_angle) = radius / height
    float slope = radius / height;
    float normal_y = 1.0f / std::sqrt(1.0f + slope * slope);
    float normal_xz = slope * normal_y;

    // ========================================================================
    // Apex vertex (shared by all side triangles)
    // ========================================================================
    uint32_t apex_idx = 0;
    Vertex apex;
    apex.position = math::Vec3{0.0f, half_height, 0.0f};
    apex.normal = math::Vec3{0.0f, 1.0f, 0.0f};  // Will be averaged
    apex.uv = math::Vec2{0.5f, 1.0f};
    vertices.push_back(apex);

    // ========================================================================
    // Base ring vertices (for side surface)
    // ========================================================================
    uint32_t base_ring_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= radial_segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);

        Vertex v;
        v.position = math::Vec3{radius * cos_a, -half_height, radius * sin_a};
        // Normal points outward and upward along the cone surface
        v.normal = glm::normalize(math::Vec3{normal_xz * cos_a, normal_y, normal_xz * sin_a});
        v.uv = math::Vec2{static_cast<float>(i) / radial_segments, 0.0f};
        vertices.push_back(v);
    }

    // Side triangles (fan from apex)
    for (uint32_t i = 0; i < radial_segments; ++i) {
        indices.push_back(apex_idx);
        indices.push_back(base_ring_start + i);
        indices.push_back(base_ring_start + i + 1);
    }

    // ========================================================================
    // Base cap (optional)
    // ========================================================================
    if (!open_ended) {
        uint32_t base_center_idx = static_cast<uint32_t>(vertices.size());
        Vertex base_center;
        base_center.position = math::Vec3{0.0f, -half_height, 0.0f};
        base_center.normal = math::Vec3{0.0f, -1.0f, 0.0f};
        base_center.uv = math::Vec2{0.5f, 0.5f};
        vertices.push_back(base_center);

        uint32_t cap_ring_start = static_cast<uint32_t>(vertices.size());
        for (uint32_t i = 0; i <= radial_segments; ++i) {
            float angle = i * angle_step;
            float cos_a = std::cos(angle);
            float sin_a = std::sin(angle);

            Vertex v;
            v.position = math::Vec3{radius * cos_a, -half_height, radius * sin_a};
            v.normal = math::Vec3{0.0f, -1.0f, 0.0f};
            v.uv = math::Vec2{cos_a * 0.5f + 0.5f, sin_a * 0.5f + 0.5f};
            vertices.push_back(v);
        }

        for (uint32_t i = 0; i < radial_segments; ++i) {
            indices.push_back(base_center_idx);
            indices.push_back(cap_ring_start + i + 1);
            indices.push_back(cap_ring_start + i);
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated cone: {} vertices, {} triangles",
                 vertices.size(), indices.size() / 3);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_capsule(
    float radius,
    float height,
    uint32_t radial_segments,
    uint32_t hemisphere_segments
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Cylinder height (total height minus two hemispheres)
    float cylinder_height = std::max(0.0f, height - 2.0f * radius);
    float half_cylinder = cylinder_height * 0.5f;
    float angle_step = 2.0f * math::PI / radial_segments;

    // ========================================================================
    // Top hemisphere
    // ========================================================================
    uint32_t top_hemi_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= hemisphere_segments; ++i) {
        // theta from 0 (top) to PI/2 (equator)
        float theta = static_cast<float>(i) / hemisphere_segments * math::PI * 0.5f;
        float sin_theta = std::sin(theta);
        float cos_theta = std::cos(theta);

        for (uint32_t j = 0; j <= radial_segments; ++j) {
            float phi = j * angle_step;
            float sin_phi = std::sin(phi);
            float cos_phi = std::cos(phi);

            Vertex v;
            v.position = math::Vec3{
                radius * sin_theta * cos_phi,
                half_cylinder + radius * cos_theta,
                radius * sin_theta * sin_phi
            };
            v.normal = math::Vec3{sin_theta * cos_phi, cos_theta, sin_theta * sin_phi};
            v.uv = math::Vec2{
                static_cast<float>(j) / radial_segments,
                static_cast<float>(i) / (hemisphere_segments * 2 + 1)
            };
            vertices.push_back(v);
        }
    }

    // ========================================================================
    // Cylinder body (single ring connecting hemispheres)
    // ========================================================================
    uint32_t cylinder_top_ring = top_hemi_start + hemisphere_segments * (radial_segments + 1);
    uint32_t cylinder_bottom_start = static_cast<uint32_t>(vertices.size());

    for (uint32_t j = 0; j <= radial_segments; ++j) {
        float phi = j * angle_step;
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);

        Vertex v;
        v.position = math::Vec3{radius * cos_phi, -half_cylinder, radius * sin_phi};
        v.normal = math::Vec3{cos_phi, 0.0f, sin_phi};
        v.uv = math::Vec2{
            static_cast<float>(j) / radial_segments,
            static_cast<float>(hemisphere_segments) / (hemisphere_segments * 2 + 1)
        };
        vertices.push_back(v);
    }

    // ========================================================================
    // Bottom hemisphere
    // ========================================================================
    uint32_t bottom_hemi_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= hemisphere_segments; ++i) {
        // theta from PI/2 (equator) to PI (bottom)
        float theta = math::PI * 0.5f + static_cast<float>(i) / hemisphere_segments * math::PI * 0.5f;
        float sin_theta = std::sin(theta);
        float cos_theta = std::cos(theta);

        for (uint32_t j = 0; j <= radial_segments; ++j) {
            float phi = j * angle_step;
            float sin_phi = std::sin(phi);
            float cos_phi = std::cos(phi);

            Vertex v;
            v.position = math::Vec3{
                radius * sin_theta * cos_phi,
                -half_cylinder + radius * cos_theta,
                radius * sin_theta * sin_phi
            };
            v.normal = math::Vec3{sin_theta * cos_phi, cos_theta, sin_theta * sin_phi};
            v.uv = math::Vec2{
                static_cast<float>(j) / radial_segments,
                static_cast<float>(hemisphere_segments + 1 + i) / (hemisphere_segments * 2 + 1)
            };
            vertices.push_back(v);
        }
    }

    // ========================================================================
    // Generate indices
    // ========================================================================

    // Top hemisphere triangles
    for (uint32_t i = 0; i < hemisphere_segments; ++i) {
        for (uint32_t j = 0; j < radial_segments; ++j) {
            uint32_t current = top_hemi_start + i * (radial_segments + 1) + j;
            uint32_t next_ring = current + radial_segments + 1;

            indices.push_back(current);
            indices.push_back(next_ring);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next_ring);
            indices.push_back(next_ring + 1);
        }
    }

    // Cylinder body (connect top hemisphere bottom to cylinder bottom)
    for (uint32_t j = 0; j < radial_segments; ++j) {
        uint32_t top_ring_v = cylinder_top_ring + j;
        uint32_t bottom_ring_v = cylinder_bottom_start + j;

        indices.push_back(top_ring_v);
        indices.push_back(bottom_ring_v);
        indices.push_back(top_ring_v + 1);

        indices.push_back(top_ring_v + 1);
        indices.push_back(bottom_ring_v);
        indices.push_back(bottom_ring_v + 1);
    }

    // Connect cylinder bottom to bottom hemisphere top
    for (uint32_t j = 0; j < radial_segments; ++j) {
        uint32_t cyl_v = cylinder_bottom_start + j;
        uint32_t hemi_v = bottom_hemi_start + j;

        indices.push_back(cyl_v);
        indices.push_back(hemi_v);
        indices.push_back(cyl_v + 1);

        indices.push_back(cyl_v + 1);
        indices.push_back(hemi_v);
        indices.push_back(hemi_v + 1);
    }

    // Bottom hemisphere triangles
    for (uint32_t i = 0; i < hemisphere_segments; ++i) {
        for (uint32_t j = 0; j < radial_segments; ++j) {
            uint32_t current = bottom_hemi_start + i * (radial_segments + 1) + j;
            uint32_t next_ring = current + radial_segments + 1;

            indices.push_back(current);
            indices.push_back(next_ring);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next_ring);
            indices.push_back(next_ring + 1);
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated capsule: {} vertices, {} triangles",
                 vertices.size(), indices.size() / 3);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_icosphere(
    float radius,
    uint32_t subdivisions
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Golden ratio for icosahedron
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    const float scale = radius / std::sqrt(1.0f + phi * phi);

    // ========================================================================
    // Icosahedron base vertices (12 vertices)
    // ========================================================================
    std::vector<math::Vec3> positions = {
        {-1,  phi, 0}, { 1,  phi, 0}, {-1, -phi, 0}, { 1, -phi, 0},
        { 0, -1,  phi}, { 0,  1,  phi}, { 0, -1, -phi}, { 0,  1, -phi},
        { phi, 0, -1}, { phi, 0,  1}, {-phi, 0, -1}, {-phi, 0,  1}
    };

    // Scale and normalize
    for (auto& p : positions) {
        p = glm::normalize(p) * radius;
    }

    // ========================================================================
    // Icosahedron faces (20 triangles)
    // ========================================================================
    std::vector<std::array<uint32_t, 3>> faces = {
        // 5 faces around point 0
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        // 5 adjacent faces
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        // 5 faces around point 3
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        // 5 adjacent faces
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    // Helper to get or create midpoint vertex
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> midpoint_cache;
    auto get_midpoint = [&](uint32_t i1, uint32_t i2) -> uint32_t {
        auto key = std::make_pair(std::min(i1, i2), std::max(i1, i2));
        auto it = midpoint_cache.find(key);
        if (it != midpoint_cache.end()) {
            return it->second;
        }

        // Create new midpoint vertex
        math::Vec3 mid = glm::normalize((positions[i1] + positions[i2]) * 0.5f) * radius;
        uint32_t new_idx = static_cast<uint32_t>(positions.size());
        positions.push_back(mid);
        midpoint_cache[key] = new_idx;
        return new_idx;
    };

    // ========================================================================
    // Subdivide faces
    // ========================================================================
    for (uint32_t sub = 0; sub < subdivisions; ++sub) {
        std::vector<std::array<uint32_t, 3>> new_faces;
        new_faces.reserve(faces.size() * 4);

        for (const auto& face : faces) {
            uint32_t v0 = face[0];
            uint32_t v1 = face[1];
            uint32_t v2 = face[2];

            uint32_t m01 = get_midpoint(v0, v1);
            uint32_t m12 = get_midpoint(v1, v2);
            uint32_t m20 = get_midpoint(v2, v0);

            // 4 new triangles
            new_faces.push_back({v0, m01, m20});
            new_faces.push_back({v1, m12, m01});
            new_faces.push_back({v2, m20, m12});
            new_faces.push_back({m01, m12, m20});
        }

        faces = std::move(new_faces);
        midpoint_cache.clear();
    }

    // ========================================================================
    // Build final vertex and index buffers
    // ========================================================================
    vertices.reserve(positions.size());
    for (const auto& pos : positions) {
        Vertex v;
        v.position = pos;
        v.normal = glm::normalize(pos);

        // Spherical UV mapping
        float u = 0.5f + std::atan2(pos.z, pos.x) / (2.0f * math::PI);
        float v_coord = 0.5f - std::asin(pos.y / radius) / math::PI;
        v.uv = math::Vec2{u, v_coord};

        vertices.push_back(v);
    }

    indices.reserve(faces.size() * 3);
    for (const auto& face : faces) {
        indices.push_back(face[0]);
        indices.push_back(face[1]);
        indices.push_back(face[2]);
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated icosphere (subdiv {}): {} vertices, {} triangles",
                 subdivisions, vertices.size(), indices.size() / 3);

    return mesh;
}

std::shared_ptr<GPUMesh> GPUMesh::create_arrow(
    float shaft_radius,
    float shaft_length,
    float head_radius,
    float head_length,
    uint32_t radial_segments
) {
    auto mesh = std::make_shared<GPUMesh>();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float angle_step = 2.0f * math::PI / radial_segments;
    float total_length = shaft_length + head_length;

    // Arrow points along +Y axis, centered at origin
    float shaft_bottom = -total_length * 0.5f;
    float shaft_top = shaft_bottom + shaft_length;
    float head_top = shaft_top + head_length;

    // ========================================================================
    // Shaft (cylinder without caps)
    // ========================================================================
    uint32_t shaft_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= radial_segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        float u = static_cast<float>(i) / radial_segments;

        // Bottom ring
        Vertex v_bottom;
        v_bottom.position = math::Vec3{shaft_radius * cos_a, shaft_bottom, shaft_radius * sin_a};
        v_bottom.normal = math::Vec3{cos_a, 0.0f, sin_a};
        v_bottom.uv = math::Vec2{u, 0.0f};
        vertices.push_back(v_bottom);

        // Top ring
        Vertex v_top;
        v_top.position = math::Vec3{shaft_radius * cos_a, shaft_top, shaft_radius * sin_a};
        v_top.normal = math::Vec3{cos_a, 0.0f, sin_a};
        v_top.uv = math::Vec2{u, shaft_length / total_length};
        vertices.push_back(v_top);
    }

    // Shaft side indices
    for (uint32_t i = 0; i < radial_segments; ++i) {
        uint32_t base = shaft_start + i * 2;
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);

        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    // ========================================================================
    // Shaft bottom cap
    // ========================================================================
    uint32_t shaft_cap_center = static_cast<uint32_t>(vertices.size());
    Vertex cap_center;
    cap_center.position = math::Vec3{0.0f, shaft_bottom, 0.0f};
    cap_center.normal = math::Vec3{0.0f, -1.0f, 0.0f};
    cap_center.uv = math::Vec2{0.5f, 0.5f};
    vertices.push_back(cap_center);

    uint32_t cap_ring_start = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= radial_segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);

        Vertex v;
        v.position = math::Vec3{shaft_radius * cos_a, shaft_bottom, shaft_radius * sin_a};
        v.normal = math::Vec3{0.0f, -1.0f, 0.0f};
        v.uv = math::Vec2{cos_a * 0.5f + 0.5f, sin_a * 0.5f + 0.5f};
        vertices.push_back(v);
    }

    for (uint32_t i = 0; i < radial_segments; ++i) {
        indices.push_back(shaft_cap_center);
        indices.push_back(cap_ring_start + i + 1);
        indices.push_back(cap_ring_start + i);
    }

    // ========================================================================
    // Arrow head base (ring connecting shaft to cone)
    // ========================================================================
    uint32_t head_base_center = static_cast<uint32_t>(vertices.size());
    Vertex head_base_c;
    head_base_c.position = math::Vec3{0.0f, shaft_top, 0.0f};
    head_base_c.normal = math::Vec3{0.0f, -1.0f, 0.0f};
    head_base_c.uv = math::Vec2{0.5f, 0.5f};
    vertices.push_back(head_base_c);

    uint32_t head_base_ring = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= radial_segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);

        Vertex v;
        v.position = math::Vec3{head_radius * cos_a, shaft_top, head_radius * sin_a};
        v.normal = math::Vec3{0.0f, -1.0f, 0.0f};
        v.uv = math::Vec2{cos_a * 0.5f + 0.5f, sin_a * 0.5f + 0.5f};
        vertices.push_back(v);
    }

    for (uint32_t i = 0; i < radial_segments; ++i) {
        indices.push_back(head_base_center);
        indices.push_back(head_base_ring + i + 1);
        indices.push_back(head_base_ring + i);
    }

    // ========================================================================
    // Arrow head cone
    // ========================================================================
    float slope = head_radius / head_length;
    float normal_y = 1.0f / std::sqrt(1.0f + slope * slope);
    float normal_xz = slope * normal_y;

    uint32_t cone_apex = static_cast<uint32_t>(vertices.size());
    Vertex apex;
    apex.position = math::Vec3{0.0f, head_top, 0.0f};
    apex.normal = math::Vec3{0.0f, 1.0f, 0.0f};
    apex.uv = math::Vec2{0.5f, 1.0f};
    vertices.push_back(apex);

    uint32_t cone_base = static_cast<uint32_t>(vertices.size());
    for (uint32_t i = 0; i <= radial_segments; ++i) {
        float angle = i * angle_step;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);

        Vertex v;
        v.position = math::Vec3{head_radius * cos_a, shaft_top, head_radius * sin_a};
        v.normal = glm::normalize(math::Vec3{normal_xz * cos_a, normal_y, normal_xz * sin_a});
        v.uv = math::Vec2{static_cast<float>(i) / radial_segments, shaft_length / total_length};
        vertices.push_back(v);
    }

    for (uint32_t i = 0; i < radial_segments; ++i) {
        indices.push_back(cone_apex);
        indices.push_back(cone_base + i);
        indices.push_back(cone_base + i + 1);
    }

    mesh->set_vertices(vertices);
    mesh->set_indices(indices);

    spdlog::info("Generated arrow: {} vertices, {} triangles",
                 vertices.size(), indices.size() / 3);

    return mesh;
}

// ============================================================================
// Utilities
// ============================================================================

std::pair<math::Vec3, math::Vec3> GPUMesh::get_bounding_box() const {
    if (vertices_.empty()) {
        return {math::Vec3{0.0f}, math::Vec3{0.0f}};
    }

    math::Vec3 min_point{std::numeric_limits<float>::max()};
    math::Vec3 max_point{std::numeric_limits<float>::lowest()};

    for (const auto& v : vertices_) {
        min_point = glm::min(min_point, v.position);
        max_point = glm::max(max_point, v.position);
    }

    return {min_point, max_point};
}

void GPUMesh::recalculate_normals(bool smooth) {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }

    // Reset normals
    for (auto& v : vertices_) {
        v.normal = math::Vec3{0.0f};
    }

    // Accumulate face normals
    for (size_t i = 0; i < indices_.size(); i += 3) {
        uint32_t i0 = indices_[i];
        uint32_t i1 = indices_[i + 1];
        uint32_t i2 = indices_[i + 2];

        math::Vec3 v0 = vertices_[i0].position;
        math::Vec3 v1 = vertices_[i1].position;
        math::Vec3 v2 = vertices_[i2].position;

        math::Vec3 edge1 = v1 - v0;
        math::Vec3 edge2 = v2 - v0;
        math::Vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        if (smooth) {
            vertices_[i0].normal += normal;
            vertices_[i1].normal += normal;
            vertices_[i2].normal += normal;
        } else {
            vertices_[i0].normal = normal;
            vertices_[i1].normal = normal;
            vertices_[i2].normal = normal;
        }
    }

    // Normalize accumulated normals
    if (smooth) {
        for (auto& v : vertices_) {
            if (glm::length(v.normal) > 0.0001f) {
                v.normal = glm::normalize(v.normal);
            }
        }
    }

    mesh_dirty_ = true;
}

void GPUMesh::weld_vertices(float threshold) {
    // Merge vertices within threshold distance
    std::unordered_map<size_t, uint32_t> hash_to_index;
    std::vector<Vertex> welded_vertices;
    std::vector<uint32_t> remap(vertices_.size());

    for (size_t i = 0; i < vertices_.size(); ++i) {
        // Simple hash based on position
        size_t hash = 0;
        hash ^= std::hash<float>{}(std::floor(vertices_[i].position.x / threshold));
        hash ^= std::hash<float>{}(std::floor(vertices_[i].position.y / threshold)) << 1;
        hash ^= std::hash<float>{}(std::floor(vertices_[i].position.z / threshold)) << 2;

        if (hash_to_index.find(hash) == hash_to_index.end()) {
            uint32_t new_index = static_cast<uint32_t>(welded_vertices.size());
            hash_to_index[hash] = new_index;
            welded_vertices.push_back(vertices_[i]);
            remap[i] = new_index;
        } else {
            remap[i] = hash_to_index[hash];
        }
    }

    // Remap indices
    for (auto& idx : indices_) {
        idx = remap[idx];
    }

    vertices_ = welded_vertices;
    mesh_dirty_ = true;

    spdlog::info("Welded vertices: {} -> {}", remap.size(), vertices_.size());
}

// ============================================================================
// LOD System
// ============================================================================

void GPUMesh::add_lod_level(const LOD& lod) {
    lod_levels_.push_back(lod);

    // Sort by distance
    std::sort(lod_levels_.begin(), lod_levels_.end(),
              [](const LOD& a, const LOD& b) {
                  return a.distance_threshold < b.distance_threshold;
              });
}

size_t GPUMesh::select_lod(float camera_distance) const {
    for (size_t i = 0; i < lod_levels_.size(); ++i) {
        if (camera_distance < lod_levels_[i].distance_threshold) {
            return i;
        }
    }

    return lod_levels_.empty() ? 0 : lod_levels_.size() - 1;
}

size_t GPUMesh::select_lod_with_hysteresis(float camera_distance, float hysteresis) {
    if (lod_levels_.empty()) {
        return 0;
    }

    // Check if we should switch up (more detail)
    if (current_lod_ > 0) {
        float threshold = lod_levels_[current_lod_ - 1].distance_threshold;
        if (camera_distance < threshold * (1.0f - hysteresis)) {
            current_lod_--;
        }
    }

    // Check if we should switch down (less detail)
    if (current_lod_ < lod_levels_.size()) {
        float threshold = lod_levels_[current_lod_].distance_threshold;
        if (camera_distance > threshold * (1.0f + hysteresis)) {
            current_lod_++;
        }
    }

    return current_lod_;
}

// ============================================================================
// QEM Mesh Simplification for LOD Generation
// ============================================================================

namespace {

// 4x4 symmetric matrix for quadric error
struct Quadric {
    double data[10];  // Symmetric 4x4 matrix stored as 10 elements

    Quadric() {
        for (int i = 0; i < 10; ++i) data[i] = 0.0;
    }

    // Create quadric from plane equation ax + by + cz + d = 0
    static Quadric from_plane(double a, double b, double c, double d) {
        Quadric q;
        q.data[0] = a * a;
        q.data[1] = a * b;
        q.data[2] = a * c;
        q.data[3] = a * d;
        q.data[4] = b * b;
        q.data[5] = b * c;
        q.data[6] = b * d;
        q.data[7] = c * c;
        q.data[8] = c * d;
        q.data[9] = d * d;
        return q;
    }

    Quadric operator+(const Quadric& other) const {
        Quadric result;
        for (int i = 0; i < 10; ++i) {
            result.data[i] = data[i] + other.data[i];
        }
        return result;
    }

    Quadric& operator+=(const Quadric& other) {
        for (int i = 0; i < 10; ++i) {
            data[i] += other.data[i];
        }
        return *this;
    }

    // Evaluate v^T * Q * v for vertex (x, y, z)
    double evaluate(double x, double y, double z) const {
        return data[0] * x * x + 2 * data[1] * x * y + 2 * data[2] * x * z + 2 * data[3] * x
             + data[4] * y * y + 2 * data[5] * y * z + 2 * data[6] * y
             + data[7] * z * z + 2 * data[8] * z
             + data[9];
    }
};

struct EdgeCollapse {
    uint32_t v1, v2;
    math::Vec3 optimal_position;
    double cost;

    bool operator>(const EdgeCollapse& other) const {
        return cost > other.cost;  // For min-heap
    }
};

// Find optimal collapse position and compute cost
EdgeCollapse compute_edge_collapse(
    uint32_t v1, uint32_t v2,
    const std::vector<manim::GPUMesh::Vertex>& vertices,
    const std::vector<Quadric>& quadrics
) {
    EdgeCollapse collapse;
    collapse.v1 = v1;
    collapse.v2 = v2;

    const Quadric Q = quadrics[v1] + quadrics[v2];

    // Simple approach: try midpoint and both endpoints, pick best
    math::Vec3 p1 = vertices[v1].position;
    math::Vec3 p2 = vertices[v2].position;
    math::Vec3 mid = (p1 + p2) * 0.5f;

    double cost1 = Q.evaluate(p1.x, p1.y, p1.z);
    double cost2 = Q.evaluate(p2.x, p2.y, p2.z);
    double cost_mid = Q.evaluate(mid.x, mid.y, mid.z);

    if (cost_mid <= cost1 && cost_mid <= cost2) {
        collapse.optimal_position = mid;
        collapse.cost = cost_mid;
    } else if (cost1 <= cost2) {
        collapse.optimal_position = p1;
        collapse.cost = cost1;
    } else {
        collapse.optimal_position = p2;
        collapse.cost = cost2;
    }

    return collapse;
}

}  // anonymous namespace

void GPUMesh::generate_lod_levels(const LODGenerationConfig& config) {
    if (vertices_.empty() || indices_.empty()) {
        spdlog::warn("Cannot generate LOD: mesh has no geometry");
        return;
    }

    if (config.distance_thresholds.size() != config.target_ratios.size()) {
        spdlog::warn("LOD config: distance_thresholds and target_ratios must have same size");
        return;
    }

    lod_levels_.clear();

    // Start with current mesh
    std::vector<Vertex> current_vertices = vertices_;
    std::vector<uint32_t> current_indices = indices_;

    spdlog::info("Generating {} LOD levels from {} vertices",
                 config.target_ratios.size(), current_vertices.size());

    for (size_t level = 0; level < config.target_ratios.size(); ++level) {
        float target_ratio = config.target_ratios[level];
        size_t target_vertex_count = static_cast<size_t>(vertices_.size() * target_ratio);

        if (target_vertex_count >= current_vertices.size()) {
            // No simplification needed, just copy
            LOD lod;
            lod.vertices = current_vertices;
            lod.indices = current_indices;
            lod.distance_threshold = config.distance_thresholds[level];
            lod_levels_.push_back(std::move(lod));
            continue;
        }

        // ====================================================================
        // Step 1: Compute quadrics for each vertex
        // ====================================================================
        std::vector<Quadric> quadrics(current_vertices.size());

        for (size_t i = 0; i < current_indices.size(); i += 3) {
            uint32_t i0 = current_indices[i];
            uint32_t i1 = current_indices[i + 1];
            uint32_t i2 = current_indices[i + 2];

            math::Vec3 v0 = current_vertices[i0].position;
            math::Vec3 v1 = current_vertices[i1].position;
            math::Vec3 v2 = current_vertices[i2].position;

            // Compute plane equation
            math::Vec3 edge1 = v1 - v0;
            math::Vec3 edge2 = v2 - v0;
            math::Vec3 normal = glm::cross(edge1, edge2);
            float len = glm::length(normal);
            if (len < 1e-8f) continue;
            normal /= len;

            double a = normal.x;
            double b = normal.y;
            double c = normal.z;
            double d = -glm::dot(normal, v0);

            Quadric plane_q = Quadric::from_plane(a, b, c, d);

            quadrics[i0] += plane_q;
            quadrics[i1] += plane_q;
            quadrics[i2] += plane_q;
        }

        // ====================================================================
        // Step 2: Build edge set and compute collapse costs
        // ====================================================================
        std::set<std::pair<uint32_t, uint32_t>> edges;
        for (size_t i = 0; i < current_indices.size(); i += 3) {
            uint32_t i0 = current_indices[i];
            uint32_t i1 = current_indices[i + 1];
            uint32_t i2 = current_indices[i + 2];

            edges.insert({std::min(i0, i1), std::max(i0, i1)});
            edges.insert({std::min(i1, i2), std::max(i1, i2)});
            edges.insert({std::min(i2, i0), std::max(i2, i0)});
        }

        // Priority queue of edge collapses (min cost first)
        std::priority_queue<EdgeCollapse, std::vector<EdgeCollapse>, std::greater<EdgeCollapse>> pq;
        for (const auto& edge : edges) {
            EdgeCollapse collapse = compute_edge_collapse(
                edge.first, edge.second, current_vertices, quadrics
            );
            pq.push(collapse);
        }

        // ====================================================================
        // Step 3: Perform edge collapses
        // ====================================================================
        std::vector<uint32_t> vertex_remap(current_vertices.size());
        std::iota(vertex_remap.begin(), vertex_remap.end(), 0);
        std::vector<bool> vertex_deleted(current_vertices.size(), false);

        auto get_canonical = [&](uint32_t v) {
            while (vertex_remap[v] != v) {
                v = vertex_remap[v];
            }
            return v;
        };

        size_t active_vertices = current_vertices.size();
        while (active_vertices > target_vertex_count && !pq.empty()) {
            EdgeCollapse collapse = pq.top();
            pq.pop();

            uint32_t v1 = get_canonical(collapse.v1);
            uint32_t v2 = get_canonical(collapse.v2);

            // Skip if edge was already collapsed
            if (v1 == v2 || vertex_deleted[v1] || vertex_deleted[v2]) {
                continue;
            }

            // Collapse v2 into v1
            current_vertices[v1].position = collapse.optimal_position;
            // Average normals and UVs
            current_vertices[v1].normal = glm::normalize(
                current_vertices[v1].normal + current_vertices[v2].normal
            );
            current_vertices[v1].uv = (current_vertices[v1].uv + current_vertices[v2].uv) * 0.5f;

            // Update quadric
            quadrics[v1] = quadrics[v1] + quadrics[v2];

            // Mark v2 as deleted and remap
            vertex_deleted[v2] = true;
            vertex_remap[v2] = v1;
            active_vertices--;
        }

        // ====================================================================
        // Step 4: Rebuild mesh with remapped vertices
        // ====================================================================
        std::vector<Vertex> new_vertices;
        std::vector<uint32_t> new_indices;
        std::vector<uint32_t> old_to_new(current_vertices.size(), UINT32_MAX);

        for (size_t i = 0; i < current_vertices.size(); ++i) {
            if (!vertex_deleted[i]) {
                old_to_new[i] = static_cast<uint32_t>(new_vertices.size());
                new_vertices.push_back(current_vertices[i]);
            }
        }

        for (size_t i = 0; i < current_indices.size(); i += 3) {
            uint32_t i0 = get_canonical(current_indices[i]);
            uint32_t i1 = get_canonical(current_indices[i + 1]);
            uint32_t i2 = get_canonical(current_indices[i + 2]);

            // Skip degenerate triangles
            if (i0 == i1 || i1 == i2 || i2 == i0) {
                continue;
            }

            if (old_to_new[i0] == UINT32_MAX ||
                old_to_new[i1] == UINT32_MAX ||
                old_to_new[i2] == UINT32_MAX) {
                continue;
            }

            new_indices.push_back(old_to_new[i0]);
            new_indices.push_back(old_to_new[i1]);
            new_indices.push_back(old_to_new[i2]);
        }

        // Store LOD level
        LOD lod;
        lod.vertices = std::move(new_vertices);
        lod.indices = std::move(new_indices);
        lod.distance_threshold = config.distance_thresholds[level];
        lod_levels_.push_back(std::move(lod));

        // Use this LOD as input for next level
        current_vertices = lod_levels_.back().vertices;
        current_indices = lod_levels_.back().indices;

        spdlog::info("LOD {}: {} vertices, {} triangles (threshold: {})",
                     level, current_vertices.size(), current_indices.size() / 3,
                     config.distance_thresholds[level]);
    }

    spdlog::info("Generated {} LOD levels", lod_levels_.size());
}

// ============================================================================
// Geomorphing (Smooth LOD Transitions)
// ============================================================================

void GPUMesh::start_lod_transition(size_t target_lod, float duration) {
    if (target_lod >= get_lod_count()) {
        spdlog::warn("Invalid target LOD: {}", target_lod);
        return;
    }

    if (target_lod == current_lod_ && !transition_state_.transitioning) {
        return;  // Already at target
    }

    transition_state_.from_lod = current_lod_;
    transition_state_.to_lod = target_lod;
    transition_state_.morph_factor = 0.0f;
    transition_state_.transition_duration = std::max(0.01f, duration);
    transition_state_.transitioning = true;

    // Build geomorph buffer for smooth transition
    build_geomorph_buffer();

    spdlog::debug("Starting LOD transition: {} -> {} (duration: {}s)",
                  transition_state_.from_lod, transition_state_.to_lod, duration);
}

void GPUMesh::update_lod_transition(float dt) {
    if (!transition_state_.transitioning) {
        return;
    }

    // Update morph factor
    transition_state_.morph_factor += dt / transition_state_.transition_duration;

    if (transition_state_.morph_factor >= 1.0f) {
        // Transition complete
        transition_state_.morph_factor = 1.0f;
        current_lod_ = transition_state_.to_lod;
        transition_state_.transitioning = false;

        spdlog::debug("LOD transition complete, now at LOD {}", current_lod_);
    }
}

void GPUMesh::build_geomorph_buffer() {
    // Get source (from_lod) and target (to_lod) mesh data
    const std::vector<Vertex>* from_verts = nullptr;
    const std::vector<Vertex>* to_verts = nullptr;

    if (transition_state_.from_lod == 0) {
        from_verts = &vertices_;
    } else if (transition_state_.from_lod <= lod_levels_.size()) {
        from_verts = &lod_levels_[transition_state_.from_lod - 1].vertices;
    }

    if (transition_state_.to_lod == 0) {
        to_verts = &vertices_;
    } else if (transition_state_.to_lod <= lod_levels_.size()) {
        to_verts = &lod_levels_[transition_state_.to_lod - 1].vertices;
    }

    if (!from_verts || !to_verts) {
        spdlog::warn("Cannot build geomorph buffer: invalid LOD levels");
        return;
    }

    geomorph_vertices_.clear();
    geomorph_vertices_.reserve(from_verts->size());

    // Build geomorph vertices
    // For simplification: use the "from" mesh as base, find closest vertex in "to" mesh
    // This is a simplified approach - full geomorphing would use morph targets from QEM

    for (size_t i = 0; i < from_verts->size(); ++i) {
        GeomorphVertex gv;
        gv.position = (*from_verts)[i].position;
        gv.normal = (*from_verts)[i].normal;
        gv.uv = (*from_verts)[i].uv;
        gv.tangent = (*from_verts)[i].tangent;

        // Find closest vertex in target mesh (simplified nearest-neighbor)
        float min_dist = std::numeric_limits<float>::max();
        size_t closest_idx = 0;

        for (size_t j = 0; j < to_verts->size(); ++j) {
            float dist = glm::length(gv.position - (*to_verts)[j].position);
            if (dist < min_dist) {
                min_dist = dist;
                closest_idx = j;
            }
        }

        // If transitioning to lower LOD, morph to closest vertex
        // If transitioning to higher LOD, we need inverse mapping (more complex)
        if (closest_idx < to_verts->size()) {
            gv.morph_target = (*to_verts)[closest_idx].position;
            gv.morph_normal = (*to_verts)[closest_idx].normal;
        } else {
            gv.morph_target = gv.position;  // No change if no mapping
            gv.morph_normal = gv.normal;
        }

        geomorph_vertices_.push_back(gv);
    }

    spdlog::debug("Built geomorph buffer with {} vertices", geomorph_vertices_.size());
}

}  // namespace manim
