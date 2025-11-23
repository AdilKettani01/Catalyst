#include "manim/scene/zoomed_scene.hpp"
#include <iostream>

namespace manim {

ZoomedScene::ZoomedScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
)
    : MovingCameraScene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations)
    , zoomFactor(2.0)
    , zoomActive(false)
{
    name = "ZoomedScene";
}

void ZoomedScene::activateZooming(
    double zoomFactor_,
    const Vec3& zoomedPosition,
    const Vec3& zoomedDisplayPosition,
    double zoomedDisplayWidth,
    double zoomedDisplayHeight
) {
    zoomFactor = zoomFactor_;
    zoomActive = true;

    std::cout << "Activating zoom:" << std::endl;
    std::cout << "  Zoom factor: " << zoomFactor << std::endl;
    std::cout << "  Position: (" << zoomedPosition.x << ", " << zoomedPosition.y << ")" << std::endl;

    // TODO: Create zoomed frame rectangle
    // TODO: Create zoomed display frame
    // TODO: Create zoomed camera
    // TODO: Add frames to scene
}

void ZoomedScene::deactivateZooming() {
    zoomActive = false;
    std::cout << "Deactivating zoom" << std::endl;

    // TODO: Remove zoom frames
    // TODO: Cleanup zoomed camera
}

} // namespace manim
