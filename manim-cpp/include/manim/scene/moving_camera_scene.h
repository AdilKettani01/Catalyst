#pragma once

#include "manim/scene/scene.h"
#include "manim/core/types.h"
#include <memory>

namespace manim {

// Forward declaration
class MovingCamera;

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

    /**
     * @brief Pan camera to position
     * @param position Target position
     * @param duration Duration of pan
     */
    void panTo(const Vec3& position, double duration = 1.0);

    /**
     * @brief Set camera frame width
     * @param width New frame width
     * @param duration Animation duration
     */
    void setCameraFrameWidth(double width, double duration = 0.0);

    /**
     * @brief Set camera frame height
     * @param height New frame height
     * @param duration Animation duration
     */
    void setCameraFrameHeight(double height, double duration = 0.0);

    /**
     * @brief Reset camera to default position
     * @param duration Animation duration
     */
    void resetCamera(double duration = 1.0);

    /**
     * @brief Save camera state
     */
    void saveCameraState();

    /**
     * @brief Restore camera state
     * @param duration Animation duration
     */
    void restoreCameraState(double duration = 1.0);

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
