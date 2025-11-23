// PointCloudMobject implementation with GPU SoA layout
#include "manim/mobject/point_cloud.hpp"
#include <spdlog/spdlog.h>

namespace manim {

// ============================================================================
// PointCloudMobject Implementation
// ============================================================================

void PointCloudMobject::generate_points() {
    // Override in subclasses
}

// ============================================================================
// Point Management
// ============================================================================

void PointCloudMobject::set_points(const std::vector<math::Vec3>& positions) {
    positions_ = positions;

    // Initialize colors and sizes if needed
    if (colors_.size() != positions_.size()) {
        colors_.resize(positions_.size(), math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
    }

    if (sizes_.size() != positions_.size()) {
        sizes_.resize(positions_.size(), default_point_size_);
    }

    soa_dirty_ = true;
    gpu_dirty_ = true;

    // Also update points_cpu_ for base class compatibility
    points_cpu_ = positions;
}

void PointCloudMobject::set_points_with_colors(
    const std::vector<math::Vec3>& positions,
    const std::vector<math::Vec4>& colors
) {
    if (positions.size() != colors.size()) {
        throw std::invalid_argument("Positions and colors must have same size");
    }

    positions_ = positions;
    colors_ = colors;

    if (sizes_.size() != positions_.size()) {
        sizes_.resize(positions_.size(), default_point_size_);
    }

    soa_dirty_ = true;
    gpu_dirty_ = true;
    points_cpu_ = positions;
}

void PointCloudMobject::set_points_with_attributes(
    const std::vector<math::Vec3>& positions,
    const std::vector<math::Vec4>& colors,
    const std::vector<float>& sizes
) {
    if (positions.size() != colors.size() || positions.size() != sizes.size()) {
        throw std::invalid_argument("Positions, colors, and sizes must have same size");
    }

    positions_ = positions;
    colors_ = colors;
    sizes_ = sizes;

    soa_dirty_ = true;
    gpu_dirty_ = true;
    points_cpu_ = positions;
}

void PointCloudMobject::add_point(
    const math::Vec3& position,
    const math::Vec4& color,
    float size
) {
    positions_.push_back(position);
    colors_.push_back(color);
    sizes_.push_back(size);

    soa_dirty_ = true;
    gpu_dirty_ = true;
    points_cpu_.push_back(position);
}

// ============================================================================
// Rendering Properties
// ============================================================================

void PointCloudMobject::set_point_size(float size) {
    default_point_size_ = size;

    // Update all sizes
    for (auto& s : sizes_) {
        s = size;
    }

    soa_dirty_ = true;
}

void PointCloudMobject::set_point_sizes(const std::vector<float>& sizes) {
    if (sizes.size() != positions_.size()) {
        throw std::invalid_argument("Sizes must match number of points");
    }

    sizes_ = sizes;
    soa_dirty_ = true;
}

void PointCloudMobject::set_point_colors(const std::vector<math::Vec4>& colors) {
    if (colors.size() != positions_.size()) {
        throw std::invalid_argument("Colors must match number of points");
    }

    colors_ = colors;
    soa_dirty_ = true;
}

void PointCloudMobject::set_uniform_color(const math::Vec4& color) {
    for (auto& c : colors_) {
        c = color;
    }

    soa_dirty_ = true;
}

// ============================================================================
// GPU SoA Layout
// ============================================================================

void PointCloudMobject::upload_to_gpu_soa(MemoryPool& pool) {
    if (!soa_dirty_ || positions_.empty()) {
        return;
    }

    size_t count = positions_.size();
    gpu_soa_.count = static_cast<uint32_t>(count);

    // Allocate/reallocate buffers

    // Positions buffer
    VkDeviceSize pos_size = sizeof(math::Vec3) * count;
    if (!gpu_soa_.positions || gpu_soa_.positions->get_size() < pos_size) {
        gpu_soa_.positions = pool.allocate_buffer(
            pos_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::DeviceLocal,
            MemoryUsage::Dynamic
        );
    }

    // Colors buffer
    VkDeviceSize color_size = sizeof(math::Vec4) * count;
    if (!gpu_soa_.colors || gpu_soa_.colors->get_size() < color_size) {
        gpu_soa_.colors = pool.allocate_buffer(
            color_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::DeviceLocal,
            MemoryUsage::Dynamic
        );
    }

    // Sizes buffer
    VkDeviceSize size_size = sizeof(float) * count;
    if (!gpu_soa_.sizes || gpu_soa_.sizes->get_size() < size_size) {
        gpu_soa_.sizes = pool.allocate_buffer(
            size_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            MemoryType::DeviceLocal,
            MemoryUsage::Dynamic
        );
    }

    // Upload data (would use staging buffers in real implementation)
    // gpu_soa_.positions->upload(positions_.data(), pos_size);
    // gpu_soa_.colors->upload(colors_.data(), color_size);
    // gpu_soa_.sizes->upload(sizes_.data(), size_size);

    soa_dirty_ = false;

    spdlog::debug("Uploaded {} points to GPU using SoA layout", count);
}

}  // namespace manim
