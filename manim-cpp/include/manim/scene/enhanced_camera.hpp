/**
 * @file enhanced_camera.hpp
 * @brief Enhanced camera with advanced features
 */

#pragma once

#include <manim/scene/camera.hpp>
#include <manim/core/math.hpp>

namespace manim {

/**
 * @brief Enhanced camera with advanced features
 */
class EnhancedCamera : public Camera {
public:
    EnhancedCamera();
    virtual ~EnhancedCamera() = default;

    // Camera movement
    void move_to(const math::Vec3& position, float duration = 1.0f);
    void look_at(const math::Vec3& target, float duration = 1.0f);
    void orbit(const math::Vec3& center, float angle, float duration = 1.0f);
    void zoom(float factor, float duration = 1.0f);

    // Camera effects
    void set_depth_of_field(float focal_distance, float aperture);
    void set_motion_blur(float strength);
    void set_lens_distortion(float k1, float k2);

    // Update camera
    void update(float dt) override;

private:
    // Animation state
    struct AnimationState {
        math::Vec3 start_position;
        math::Vec3 target_position;
        math::Vec3 start_rotation;
        math::Vec3 target_rotation;
        float progress = 0.0f;
        float duration = 0.0f;
        bool active = false;
    } animation_;

    // Camera effects
    float focal_distance_ = 10.0f;
    float aperture_ = 0.1f;
    float motion_blur_strength_ = 0.0f;
    float lens_k1_ = 0.0f;
    float lens_k2_ = 0.0f;
};

}  // namespace manim