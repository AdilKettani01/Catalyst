/**
 * @file mobject.hpp
 * @brief Base Mathematical Object class - Foundation of all renderable content
 *
 * The Mobject (Mathematical Object) is the core abstraction in Manim.
 * All visual elements inherit from this class.
 *
 * GPU-first design:
 * - Points and transforms stored on GPU
 * - CPU copy only for debugging/inspection
 * - Automatic CPU-GPU sync
 */

#pragma once

#include <manim/core/memory_pool.hpp>
#include <manim/core/math.hpp>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <glm/glm.hpp>

namespace manim {

// Forward declarations
class Animation;
class Scene;
class Renderer;

/**
 * @brief Base class for all mathematical objects
 *
 * Key features:
 * - Hierarchical structure (parent/children submobjects)
 * - GPU-resident geometry data
 * - Transform matrices
 * - Color and style properties
 * - Updater functions
 * - Bounding volume for culling
 */
class Mobject : public std::enable_shared_from_this<Mobject> {
public:
    using Ptr = std::shared_ptr<Mobject>;
    using UpdaterFunc = std::function<void(Mobject&, float dt)>;

    virtual ~Mobject() = default;

    // ========================================================================
    // Construction & Initialization
    // ========================================================================

    /**
     * @brief Initialize GPU resources
     */
    virtual void initialize(MemoryPool& pool, ComputeEngine& engine);

    /**
     * @brief Generate geometry points
     * Override this to define mobject shape
     */
    virtual void generate_points() = 0;

    /**
     * @brief Initialize colors based on style
     */
    virtual void init_colors();

    // ========================================================================
    // Hierarchy Management
    // ========================================================================

    /**
     * @brief Add child submobject
     */
    void add(Ptr submobject);

    /**
     * @brief Add multiple submobjects
     */
    template<typename... Args>
    void add(Ptr first, Args... rest) {
        add(first);
        add(rest...);
    }

    /**
     * @brief Remove submobject
     */
    void remove(Ptr submobject);

    /**
     * @brief Get all submobjects
     */
    const std::vector<Ptr>& get_submobjects() const { return submobjects_; }

    /**
     * @brief Get parent mobject
     */
    Ptr get_parent() const { return parent_.lock(); }

    /**
     * @brief Get all family members (self + all descendants)
     */
    std::vector<Ptr> get_family() const;

    // ========================================================================
    // Transform Operations (GPU-accelerated)
    // ========================================================================

    /**
     * @brief Move to position
     */
    Mobject& move_to(const math::Vec3& position);

    /**
     * @brief Shift by vector
     */
    Mobject& shift(const math::Vec3& offset);

    /**
     * @brief Scale uniformly
     */
    Mobject& scale(float factor);

    /**
     * @brief Scale non-uniformly
     */
    Mobject& scale(const math::Vec3& factors);

    /**
     * @brief Rotate around axis
     */
    Mobject& rotate(float angle, const math::Vec3& axis = math::UP);

    /**
     * @brief Rotate around point
     */
    Mobject& rotate_about_point(float angle, const math::Vec3& point,
                                const math::Vec3& axis = math::UP);

    /**
     * @brief Position next to another mobject
     */
    Mobject& next_to(const Mobject& other, const math::Vec3& direction,
                     float buff = 0.25f);

    /**
     * @brief Move to center of screen
     */
    Mobject& to_edge(const math::Vec3& direction, float buff = 0.5f);

    /**
     * @brief Get world transform matrix
     */
    math::Mat4 get_world_transform() const;

    /**
     * @brief Apply transform to points (GPU operation)
     */
    void apply_transform(const math::Mat4& transform);

    // ========================================================================
    // Appearance & Styling
    // ========================================================================

    /**
     * @brief Set color
     */
    Mobject& set_color(const math::Vec4& color);

    /**
     * @brief Set opacity
     */
    Mobject& set_opacity(float opacity);

    /**
     * @brief Set z-index for rendering order
     */
    Mobject& set_z_index(float z_index);

    /**
     * @brief Get color
     */
    const math::Vec4& get_color() const { return color_; }

    /**
     * @brief Get opacity
     */
    float get_opacity() const { return opacity_; }

    /**
     * @brief Get z-index
     */
    float get_z_index() const { return z_index_; }

    // ========================================================================
    // Geometry Access (GPU-accelerated)
    // ========================================================================

    /**
     * @brief Get points (downloads from GPU if needed)
     */
    std::span<const math::Vec3> get_points() const;

    /**
     * @brief Set points (uploads to GPU)
     */
    void set_points(std::span<const math::Vec3> points);

    /**
     * @brief Get GPU buffer for points
     */
    const GPUBuffer& get_points_buffer() const { return points_buffer_; }

    /**
     * @brief Get number of points
     */
    size_t get_num_points() const { return num_points_; }

    /**
     * @brief Compute bounding box
     */
    struct BoundingBox {
        math::Vec3 min;
        math::Vec3 max;

        math::Vec3 center() const { return (min + max) * 0.5f; }
        math::Vec3 size() const { return max - min; }
    };

    BoundingBox compute_bounding_box() const;

    /**
     * @brief Get cached bounding box
     */
    const BoundingBox& get_bounding_box() const;

    // ========================================================================
    // Updaters (Dynamic Animations)
    // ========================================================================

    /**
     * @brief Add updater function
     */
    void add_updater(UpdaterFunc updater);

    /**
     * @brief Remove updater
     */
    void remove_updater(UpdaterFunc updater);

    /**
     * @brief Clear all updaters
     */
    void clear_updaters();

    /**
     * @brief Update mobject (called every frame)
     */
    virtual void update(float dt);

    /**
     * @brief Suspend/resume updating
     */
    void suspend_updating(bool suspend = true);

    // ========================================================================
    // Rendering
    // ========================================================================

    /**
     * @brief Render mobject
     */
    virtual void render(Renderer& renderer);

    /**
     * @brief Mark GPU data as dirty (needs re-upload)
     */
    void mark_dirty();

    /**
     * @brief Update GPU buffers if dirty
     */
    void update_gpu_buffers();

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Copy mobject
     */
    virtual Ptr copy() const;

    /**
     * @brief Get/set name
     */
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    /**
     * @brief Print debug info
     */
    void print_debug() const;

protected:
    // Hierarchy
    std::vector<Ptr> submobjects_;
    std::weak_ptr<Mobject> parent_;

    // GPU data
    GPUBuffer points_buffer_;
    GPUBuffer color_buffer_;
    mutable std::vector<math::Vec3> points_cpu_;  // CPU cache
    size_t num_points_ = 0;

    // Transform
    glm::mat4 local_transform_{1.0f};  // Local transform matrix

    // Appearance
    math::Vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};  // White, opaque
    float opacity_ = 1.0f;
    float z_index_ = 0.0f;

    // Updaters
    std::vector<UpdaterFunc> updaters_;
    bool updating_suspended_ = false;

    // Bounding volume
    mutable BoundingBox bounding_box_;
    mutable bool bbox_dirty_ = true;

    // Flags
    bool gpu_dirty_ = true;  // GPU needs update
    mutable bool cpu_dirty_ = true;  // CPU needs update

    // Resources
    MemoryPool* memory_pool_ = nullptr;
    ComputeEngine* compute_engine_ = nullptr;

    // Metadata
    std::string name_;

    // Helper functions
    void update_submobjects(float dt);
    void sync_to_gpu();
    void sync_from_gpu() const;
};

/**
 * @brief Group of mobjects
 *
 * A container for multiple mobjects that can be transformed together.
 */
class Group : public Mobject {
public:
    Group() = default;

    template<typename... Args>
    explicit Group(Args... mobjects) {
        add(mobjects...);
    }

    void generate_points() override {
        // Groups don't have their own points
    }

    void render(Renderer& renderer) override;
};

} // namespace manim
