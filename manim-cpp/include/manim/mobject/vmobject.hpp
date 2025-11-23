#pragma once

#include "manim/mobject/mobject.hpp"
#include "manim/core/math.hpp"
#include <vector>
#include <optional>

namespace manim {

// Line joint types
enum class LineJointType {
    AUTO,
    MITER,
    BEVEL,
    ROUND
};

// Cap style types
enum class CapStyleType {
    AUTO,
    BUTT,
    ROUND,
    SQUARE
};

/**
 * @brief Vectorized Mobject - uses cubic Bezier curves for smooth rendering
 *
 * VMobject represents 2D/3D shapes using cubic Bezier curves. Each curve is defined
 * by 4 control points (anchor, handle, handle, anchor).
 *
 * GPU Optimizations:
 * - Bezier evaluation on GPU compute shaders
 * - Tessellation shaders for adaptive detail
 * - Batch curve rendering
 * - GPU stroke generation
 */
class VMobject : public Mobject {
public:
    VMobject();
    virtual ~VMobject() = default;

    // ========================================================================
    // Bezier Curve Management
    // ========================================================================

    /**
     * @brief Set the points as cubic Bezier curves
     * @param points Vector of points (size must be 1 mod 4 or 0)
     *
     * Points are organized as:
     * [anchor0, handle0, handle1, anchor1, handle1, handle2, anchor2, ...]
     */
    void set_points(const std::vector<math::Vec3>& points) override;

    /**
     * @brief Add a cubic Bezier curve
     * @param anchors Array of 2 anchor points
     * @param handles Array of 2 handle points
     */
    void add_cubic_bezier_curve(
        const std::array<math::Vec3, 2>& anchors,
        const std::array<math::Vec3, 2>& handles
    );

    /**
     * @brief Add a line segment (straight Bezier curve)
     */
    void add_line_to(const math::Vec3& point);

    /**
     * @brief Start a new subpath
     */
    void start_new_path(const math::Vec3& point);

    /**
     * @brief Close current path (connect last point to first)
     */
    void close_path();

    /**
     * @brief Make the curve smooth using cubic spline interpolation
     */
    void make_smooth();

    /**
     * @brief Make the curve jagged (straight lines between points)
     */
    void make_jagged();

    // ========================================================================
    // GPU Bezier Evaluation
    // ========================================================================

    /**
     * @brief Tessellate Bezier curves on GPU
     * @param segments_per_curve Number of line segments per curve
     * @param engine Compute engine for GPU operations
     *
     * Uses compute shader for parallel Bezier evaluation
     */
    void tessellate_on_gpu(uint32_t segments_per_curve, ComputeEngine& engine);

    /**
     * @brief Get tessellated vertices (after GPU evaluation)
     */
    const std::vector<math::Vec3>& get_tessellated_points() const;

    /**
     * @brief Update GPU buffers with current curve data
     */
    void update_bezier_gpu_buffers(MemoryPool& pool);

    // ========================================================================
    // Stroke and Fill Properties
    // ========================================================================

    // Fill
    VMobject& set_fill(const math::Vec4& color, float opacity = 1.0f);
    VMobject& set_fill_opacity(float opacity);
    math::Vec4 get_fill_color() const { return fill_color_; }
    float get_fill_opacity() const { return fill_opacity_; }

    // Stroke
    VMobject& set_stroke(const math::Vec4& color, float width = 2.0f, float opacity = 1.0f);
    VMobject& set_stroke_width(float width);
    VMobject& set_stroke_opacity(float opacity);
    math::Vec4 get_stroke_color() const { return stroke_color_; }
    float get_stroke_width() const { return stroke_width_; }
    float get_stroke_opacity() const { return stroke_opacity_; }

    // Background stroke (for text on textured backgrounds)
    VMobject& set_background_stroke(const math::Vec4& color, float width = 0.0f);
    math::Vec4 get_background_stroke_color() const { return background_stroke_color_; }
    float get_background_stroke_width() const { return background_stroke_width_; }

    // Sheen (gradient effect)
    VMobject& set_sheen(float factor, const math::Vec3& direction = math::Vec3{-1, 1, 0});
    float get_sheen_factor() const { return sheen_factor_; }
    math::Vec3 get_sheen_direction() const { return sheen_direction_; }

    // Joint and cap styles
    VMobject& set_joint_type(LineJointType type);
    VMobject& set_cap_style(CapStyleType style);
    LineJointType get_joint_type() const { return joint_type_; }
    CapStyleType get_cap_style() const { return cap_style_; }

    // ========================================================================
    // Curve Utilities
    // ========================================================================

    /**
     * @brief Get number of cubic Bezier curves
     */
    size_t get_num_curves() const;

    /**
     * @brief Get control points for a specific curve
     * @param index Curve index
     * @return Array of 4 control points [anchor0, handle0, handle1, anchor1]
     */
    std::array<math::Vec3, 4> get_curve_points(size_t index) const;

    /**
     * @brief Get all anchor points (every 4th point starting from 0)
     */
    std::vector<math::Vec3> get_anchors() const;

    /**
     * @brief Point at a specific position along the curve [0, 1]
     */
    math::Vec3 point_from_proportion(float alpha) const;

    /**
     * @brief Get curve length (approximate)
     */
    float get_arc_length() const;

    /**
     * @brief Subdivide curve into more segments
     */
    void insert_n_curves(size_t n);

    /**
     * @brief Align points with another VMobject (for morphing)
     */
    void align_points(const VMobject& other);

    // ========================================================================
    // Path Information
    // ========================================================================

    bool has_new_path_started() const { return has_new_path_started_; }
    bool is_closed() const;
    void set_shade_in_3d(bool shade) { shade_in_3d_ = shade; }
    bool get_shade_in_3d() const { return shade_in_3d_; }

    // ========================================================================
    // GPU Rendering Data
    // ========================================================================

    struct GPUStrokeData {
        math::Vec4 color;
        float width;
        float opacity;
        uint32_t joint_type;  // LineJointType as uint
        uint32_t cap_style;   // CapStyleType as uint
    };

    struct GPUFillData {
        math::Vec4 color;
        float opacity;
        float sheen_factor;
        math::Vec3 sheen_direction;
    };

    GPUStrokeData get_gpu_stroke_data() const;
    GPUFillData get_gpu_fill_data() const;

    // Override copy from base class
    Ptr copy() const override;  // Implemented in vmobject.cpp

protected:
    void generate_points() override;
    void init_colors() override;

    /**
     * @brief Compute smooth handles for cubic spline
     */
    void compute_smooth_handles();

    /**
     * @brief Ensure points array is valid (size = 1 mod 4 or 0)
     */
    void ensure_valid_points();

    // GPU tessellation members (accessible to derived classes)
    std::vector<math::Vec3> tessellated_points_;
    std::optional<GPUBuffer> bezier_control_points_buffer_;
    std::optional<GPUBuffer> tessellated_vertices_buffer_;
    bool tessellation_dirty_{true};

private:
    // Fill properties
    math::Vec4 fill_color_{0.5f, 0.5f, 0.5f, 1.0f};
    float fill_opacity_{0.0f};

    // Stroke properties
    math::Vec4 stroke_color_{1.0f, 1.0f, 1.0f, 1.0f};
    float stroke_width_{2.0f};
    float stroke_opacity_{1.0f};

    // Background stroke
    math::Vec4 background_stroke_color_{0.0f, 0.0f, 0.0f, 1.0f};
    float background_stroke_width_{0.0f};
    float background_stroke_opacity_{1.0f};

    // Sheen
    float sheen_factor_{0.0f};
    math::Vec3 sheen_direction_{-1.0f, 1.0f, 0.0f};

    // Style
    LineJointType joint_type_{LineJointType::AUTO};
    CapStyleType cap_style_{CapStyleType::AUTO};

    // Path state
    bool has_new_path_started_{false};
    bool shade_in_3d_{false};
    bool close_new_points_{false};

    // Bezier settings
    uint32_t n_points_per_cubic_curve_{4};
    float tolerance_for_point_equality_{1e-6f};
};

/**
 * @brief Group of VMobjects
 */
class VGroup : public VMobject {
public:
    VGroup() = default;
    explicit VGroup(const std::vector<std::shared_ptr<VMobject>>& vmobjects);

    VGroup& add(std::shared_ptr<VMobject> vmobject);

    Ptr copy() const override {
        auto copied = std::make_shared<VGroup>();
        for (const auto& submob : submobjects_) {
            if (auto vmob = std::dynamic_pointer_cast<VMobject>(submob)) {
                copied->add(std::dynamic_pointer_cast<VMobject>(vmob->copy()));
            }
        }
        return copied;
    }
};

}  // namespace manim
