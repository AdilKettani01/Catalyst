#pragma once

#include "manim/core/math.hpp"
#include <cstdint>
#include <vulkan/vulkan.h>

namespace manim {
namespace culling {

/**
 * @brief Object bounds for GPU culling (48 bytes, aligned)
 *
 * Stores AABB and bounding sphere for efficient culling tests.
 * Layout optimized for GPU vec4 alignment.
 */
struct alignas(16) ObjectBounds {
    math::Vec4 aabb_min;    // xyz = min corner, w = flags/padding
    math::Vec4 aabb_max;    // xyz = max corner, w = object_id
    math::Vec4 sphere;      // xyz = center, w = radius

    ObjectBounds() = default;

    ObjectBounds(const math::Vec3& min, const math::Vec3& max, uint32_t object_id = 0)
        : aabb_min(min.x, min.y, min.z, 0.0f)
        , aabb_max(max.x, max.y, max.z, *reinterpret_cast<const float*>(&object_id))
        , sphere(0.0f, 0.0f, 0.0f, 0.0f)
    {
        // Compute bounding sphere
        math::Vec3 center = (min + max) * 0.5f;
        float radius = glm::length(max - center);
        sphere = math::Vec4(center.x, center.y, center.z, radius);
    }

    uint32_t get_object_id() const {
        return *reinterpret_cast<const uint32_t*>(&aabb_max.w);
    }

    void set_object_id(uint32_t id) {
        aabb_max.w = *reinterpret_cast<const float*>(&id);
    }
};
static_assert(sizeof(ObjectBounds) == 48, "ObjectBounds must be 48 bytes");

/**
 * @brief Frustum planes for GPU culling (96 bytes + padding)
 *
 * 6 planes extracted from view-projection matrix.
 * Each plane: xyz = normal, w = distance from origin
 */
struct FrustumPlanes {
    math::Vec4 planes[6];   // Left, Right, Bottom, Top, Near, Far
    uint32_t num_objects;
    uint32_t padding[3];    // Align to 16 bytes

    enum PlaneIndex : uint32_t {
        LEFT = 0,
        RIGHT = 1,
        BOTTOM = 2,
        TOP = 3,
        NEAR = 4,
        FAR = 5
    };
};
static_assert(sizeof(FrustumPlanes) == 112, "FrustumPlanes alignment check");

/**
 * @brief Visibility buffer header for compacted output
 */
struct VisibilityBufferHeader {
    uint32_t visible_count;     // Number of visible objects
    uint32_t total_objects;     // Total objects tested
    uint32_t padding[2];        // Align to 16 bytes
};

/**
 * @brief Push constants for frustum culling shader
 */
struct FrustumCullPushConstants {
    uint32_t num_objects;
    uint32_t output_mode;       // 0 = bitfield, 1 = compacted indices
    uint32_t padding[2];
};

/**
 * @brief Push constants for Hi-Z pyramid build shader
 */
struct HiZBuildPushConstants {
    int32_t src_width;
    int32_t src_height;
    int32_t dst_width;
    int32_t dst_height;
    uint32_t mip_level;
    uint32_t padding[3];
};

/**
 * @brief Push constants for occlusion culling shader
 */
struct OcclusionCullPushConstants {
    uint32_t num_candidates;
    uint32_t viewport_width;
    uint32_t viewport_height;
    float near_plane;
    float far_plane;
    uint32_t padding[3];
};

/**
 * @brief Camera data for occlusion culling
 */
struct CameraData {
    math::Mat4 view_proj;
    math::Vec4 viewport_size;   // xy = size, zw = 1/size
    float near_plane;
    float far_plane;
    uint32_t padding[2];
};

/**
 * @brief BVH node structure for spatial indexing (32 bytes)
 */
struct alignas(16) BVHNode {
    math::Vec3 aabb_min;
    uint32_t left;          // Left child index or first primitive (for leaves)
    math::Vec3 aabb_max;
    uint32_t right;         // Right child index or primitive count (for leaves)

    bool is_leaf() const {
        // Convention: if right MSB is set, it's a leaf
        return (right & 0x80000000u) != 0;
    }

    uint32_t get_primitive_count() const {
        return right & 0x7FFFFFFFu;
    }

    void set_leaf(uint32_t first_primitive, uint32_t count) {
        left = first_primitive;
        right = count | 0x80000000u;
    }
};
static_assert(sizeof(BVHNode) == 32, "BVHNode must be 32 bytes");

/**
 * @brief Morton code with object index for BVH construction
 */
struct MortonPrimitive {
    uint32_t morton_code;
    uint32_t object_index;
};

/**
 * @brief BVH update strategy
 */
enum class BVHUpdateStrategy {
    None,           // No update needed (static scene)
    Refit,          // Only update AABBs (small movements)
    FullRebuild     // Complete reconstruction (major changes)
};

/**
 * @brief Indirect draw command (matches VkDrawIndexedIndirectCommand)
 */
struct DrawIndexedIndirectCommand {
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t  vertex_offset;
    uint32_t first_instance;
};

/**
 * @brief Object draw info for indirect rendering
 */
struct ObjectDrawInfo {
    uint32_t index_count;
    uint32_t first_index;
    int32_t  vertex_offset;
    uint32_t material_id;
    uint32_t lod_level;
    uint32_t padding[3];
};

} // namespace culling
} // namespace manim
