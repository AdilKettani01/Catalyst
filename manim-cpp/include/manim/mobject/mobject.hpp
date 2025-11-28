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
#include <manim/core/types.h>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <optional>
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

    Mobject();
    virtual ~Mobject();

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
    virtual void generate_points();

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
     * @brief Clear all submobjects
     */
    void clear_submobjects();

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
     * @brief Align this mobject to another
     */
    Mobject& align_to(const Mobject& other);

    /**
     * @brief Move to center of screen
     */
    Mobject& to_edge(const math::Vec3& direction, float buff = 0.5f);


    /**
     * @brief Apply transform to points (GPU operation)
     */
    void apply_transform(const math::Mat4& transform);

    /**
     * @brief Apply matrix transformation to all points
     */
    void apply_matrix(const math::Mat4& matrix, bool about_point = false, const math::Vec3& point = math::Vec3(0.0f));

    /**
     * @brief Apply function to all points
     */
    template<typename Func>
    void apply_points_function(Func&& func) {
        sync_from_gpu();
        for (auto& point : points_cpu_) {
            point = func(point);
        }
        gpu_dirty_ = true;
    }

    /**
     * @brief Apply function to all points (non-template version)
     */
    void apply_points_function(const std::function<math::Vec3(math::Vec3)>& func);

    /**
     * @brief Get center point
     */
    math::Vec3 get_center() const;

    /**
     * @brief Get bounding box
     */
    std::pair<math::Vec3, math::Vec3> get_bounding_box() const;

    // ========================================================================
    // Appearance & Styling
    // ========================================================================

    /**
     * @brief Set color
     */
    Mobject& set_color(const math::Vec4& color);
    Mobject& set_color(const Color& color) { return set_color(color.toVec4()); }
    Mobject& set_color(std::initializer_list<float> components) {
        auto it = components.begin();
        float r = (it != components.end()) ? *it++ : 1.0f;
        float g = (it != components.end()) ? *it++ : 1.0f;
        float b = (it != components.end()) ? *it++ : 1.0f;
        float a = (it != components.end()) ? *it++ : 1.0f;
        return set_color(math::Vec4{r, g, b, a});
    }

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

    // Compatibility helpers (camelCase API)
    void moveTo(const math::Vec3& position) { move_to(position); }
    void nextTo(const Ptr& other, const math::Vec3& direction = math::Vec3{1, 0, 0}) { if (other) next_to(*other, direction); }
    void alignTo(const Ptr& other) { if (other) align_to(*other); }
    void setColor(const math::Vec4& color) { set_color(color); }
    void setOpacity(float opacity) { set_opacity(opacity); }
    void saveState() { save_state(); }
    math::Vec3 getCenter() const { return get_center(); }
    math::Vec4 getColor() const { return color_; }
    void addUpdater(UpdaterFunc updater) { add_updater(std::move(updater)); }
    void removeUpdater(size_t index) { remove_updater(index); }
    void save_state() { saved_states_.push_back(points_cpu_); }
    void restore() {
        if (!saved_states_.empty()) {
            points_cpu_ = saved_states_.back();
            num_points_ = points_cpu_.size();
            gpu_dirty_ = true;
            bbox_dirty_ = true;
            saved_states_.pop_back();
        }
    }
    void become(const Ptr& other) {
        if (other) {
            points_cpu_ = other->points_cpu_;
            num_points_ = points_cpu_.size();
        }
    }
    void matchPoints(const Ptr& other) { become(other); }
    std::vector<Ptr> getFamily() const { return get_family(); }

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
    virtual void set_points(std::span<const math::Vec3> points);
    virtual void set_points(const std::vector<math::Vec3>& points) {
        set_points(std::span<const math::Vec3>(points));
    }

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


    // ========================================================================
    // Updaters (Dynamic Animations)
    // ========================================================================

    /**
     * @brief Add updater function
     */
    void add_updater(UpdaterFunc updater);

    /**
     * @brief Remove updater by index
     */
    void remove_updater(size_t index);

    /**
     * @brief Clear all updaters
     */
    void clear_updaters();

    /**
     * @brief Update mobject (called every frame)
     */
    virtual void update(float dt, bool recurse_down = true);

    /**
     * @brief Suspend updating
     */
    void suspend_updating();

    /**
     * @brief Resume updating
     */
    void resume_updating();

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
     * @brief Mark GPU data as dirty
     */
    void mark_as_gpu_dirty();

    /**
     * @brief Check if GPU data is dirty
     */
    bool is_gpu_dirty() const;

    /**
     * @brief Clear GPU dirty flag
     */
    void clear_gpu_dirty_flag();

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
     * @brief Get world transform (updates if dirty)
     */
    const math::Mat4& get_world_transform() const;

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
    mutable std::vector<std::vector<math::Vec3>> saved_states_;
    size_t num_points_ = 0;

    // Transform
    math::Mat4 local_transform_ = math::Mat4::identity();  // Local transform matrix
    math::Mat4 world_transform_ = math::Mat4::identity();  // World transform matrix
    math::Mat4 prev_transform_ = math::Mat4::identity();   // Previous transform for animation
    bool transform_dirty_ = false;

    // Appearance
    math::Vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};  // White, opaque
    float opacity_ = 1.0f;
    float z_index_ = 0.0f;
    int dim_ = 3;  // Dimension

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
    mutable std::optional<size_t> point_hash_;  // Hash of points for caching

    // Helper functions
    void update_submobjects(float dt);
    void sync_to_gpu();
    void sync_from_gpu() const;
    void update_world_transform();
    // (Previously protected) world transform helper
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

    Ptr copy() const override {
        auto copied = std::make_shared<Group>();
        for (const auto& submob : submobjects_) {
            copied->add(submob->copy());
        }
        return copied;
    }

    void render(Renderer& renderer) override;
};

} // namespace manim
