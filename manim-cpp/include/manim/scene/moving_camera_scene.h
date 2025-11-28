#pragma once

#include "manim/scene/scene.h"
#include "manim/core/types.h"
#include "manim/scene/camera.hpp"
#include <memory>

namespace manim {

// Simple moving camera stub
class MovingCamera : public Camera {
public:
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }

    void setFrameWidth(double w) { frame_width_ = w; }
    double getFrameWidth() const { return frame_width_; }

    void setFrameHeight(double h) { frame_height_ = h; }
    double getFrameHeight() const { return frame_height_; }

    void setZoom(double z) { zoom_ = z; }
    double getZoom() const { return zoom_; }

private:
    Vec3 position_{0.0f, 0.0f, 0.0f};
    double frame_width_ = 1.0;
    double frame_height_ = 1.0;
    double zoom_ = 1.0;
};

/**
 * @brief Scene with a camera that can be moved around
 *
 * This scene type allows the camera to be animated and moved,
 * enabling zoom effects, panning, and dynamic framing.
 */
class MovingCameraScene : public Scene {
public:
    MovingCameraScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~MovingCameraScene() = default;

    /**
     * @brief Get the moving camera
     */
    std::shared_ptr<MovingCamera> getMovingCamera() const;

    /**
     * @brief Zoom to mobject
     * @param mobject Target mobject to zoom to
     * @param zoomFactor Zoom factor (1.0 = fit exactly, >1.0 = leave margin)
     * @param duration Duration of zoom animation
     */
    void zoomToMobject(std::shared_ptr<Mobject> mobject,
                       double zoomFactor = 1.2,
                       double duration = 1.0);

    /**
     * @brief Auto zoom to mobjects with animation
     * @param mobjects Mobjects to frame
     * @param margin Margin around mobjects
     * @param animate Whether to animate the zoom
     * @param duration Duration of zoom animation
     */
    void autoZoom(const std::vector<std::shared_ptr<Mobject>>& mobjects,
                  double margin = 0.5,
                  bool animate = true,
                  double duration = 1.0);
    void zoom_to_mobject(std::shared_ptr<Mobject> mobject,
                         double zoomFactor = 1.2,
                         double duration = 1.0) {
        zoomToMobject(std::move(mobject), zoomFactor, duration);
    }

    /**
     * @brief Pan camera to position
     * @param position Target position
     * @param duration Duration of pan
     */
    void panTo(const Vec3& position, double duration = 1.0);
    void pan_to(const Vec3& position, double duration = 1.0) { panTo(position, duration); }

    /**
     * @brief Set camera frame width
     * @param width New frame width
     * @param duration Animation duration
     */
    void setCameraFrameWidth(double width, double duration = 0.0);
    void set_camera_frame_width(double width, double duration = 0.0) { setCameraFrameWidth(width, duration); }

    /**
     * @brief Set camera frame height
     * @param height New frame height
     * @param duration Animation duration
     */
    void setCameraFrameHeight(double height, double duration = 0.0);
    void set_camera_frame_height(double height, double duration = 0.0) { setCameraFrameHeight(height, duration); }

    /**
     * @brief Reset camera to default position
     * @param duration Animation duration
     */
    void resetCamera(double duration = 1.0);
    void reset_camera(double duration = 1.0) { resetCamera(duration); }

    /**
     * @brief Save camera state
     */
    void saveCameraState();
    void save_camera_state() { saveCameraState(); }

    /**
     * @brief Restore camera state
     * @param duration Animation duration
     */
    void restoreCameraState(double duration = 1.0);
    void restore_camera_state(double duration = 1.0) { restoreCameraState(duration); }

protected:
    // Saved camera state
    struct CameraState {
        Vec3 position;
        double width;
        double height;
        double zoom;
    };

    std::optional<CameraState> savedState;
    CameraState defaultState;
};

} // namespace manim
