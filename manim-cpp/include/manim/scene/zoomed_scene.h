#pragma once

#include "manim/scene/moving_camera_scene.h"
#include "manim/core/types.h"
#include <memory>

namespace manim {

/**
 * @brief Scene with zoomed-in viewport capability
 */
class ZoomedScene : public MovingCameraScene {
public:
    ZoomedScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~ZoomedScene() = default;

    /**
     * @brief Activate zoom
     * @param zoomFactor Zoom magnification
     * @param zoomedPosition Position to zoom into
     * @param zoomedDisplayPosition Where to show zoomed view
     * @param zoomedDisplayWidth Width of zoomed display
     * @param zoomedDisplayHeight Height of zoomed display
     */
    void activateZooming(
        double zoomFactor = 2.0,
        const Vec3& zoomedPosition = Vec3(0, 0, 0),
        const Vec3& zoomedDisplayPosition = Vec3(3, 2, 0),
        double zoomedDisplayWidth = 3.0,
        double zoomedDisplayHeight = 2.0
    );

    /**
     * @brief Deactivate zoom
     */
    void deactivateZooming();

    /**
     * @brief Get zoomed camera frame
     */
    std::shared_ptr<Mobject> getZoomedFrame() const { return zoomedFrame; }

    /**
     * @brief Get zoomed display frame
     */
    std::shared_ptr<Mobject> getZoomedDisplayFrame() const { return zoomedDisplayFrame; }

protected:
    std::shared_ptr<Mobject> zoomedFrame;
    std::shared_ptr<Mobject> zoomedDisplayFrame;
    std::shared_ptr<Camera> zoomedCamera;
    double zoomFactor;
    bool zoomActive;
};

} // namespace manim
