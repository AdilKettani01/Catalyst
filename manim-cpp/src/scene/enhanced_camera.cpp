#include "manim/scene/enhanced_camera.hpp"
#include <spdlog/spdlog.h>

namespace manim {

EnhancedCamera::EnhancedCamera() = default;

void EnhancedCamera::move_to(const math::Vec3& /*position*/, float /*duration*/) {}
void EnhancedCamera::look_at(const math::Vec3& /*target*/, float /*duration*/) {}
void EnhancedCamera::orbit(const math::Vec3& /*center*/, float /*angle*/, float /*duration*/) {}
void EnhancedCamera::zoom(float /*factor*/, float /*duration*/) {}

void EnhancedCamera::set_depth_of_field(float focal_distance, float aperture) {
    focal_distance_ = focal_distance;
    aperture_ = aperture;
}

void EnhancedCamera::set_motion_blur(float strength) {
    motion_blur_strength_ = strength;
}

void EnhancedCamera::set_lens_distortion(float k1, float k2) {
    lens_k1_ = k1;
    lens_k2_ = k2;
}

void EnhancedCamera::update(float /*dt*/) {
    spdlog::trace("Updating enhanced camera");
}

}  // namespace manim
