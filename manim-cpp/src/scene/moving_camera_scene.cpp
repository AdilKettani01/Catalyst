#include "manim/scene/moving_camera_scene.h"
#include "manim/renderer/renderer.h"
#include <iostream>

namespace manim {

MovingCameraScene::MovingCameraScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
)
    : Scene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations)
{
    name = "MovingCameraScene";

    // Initialize default camera state
    defaultState.position = Vec3(0.0f, 0.0f, 0.0f);
    defaultState.width = 14.0;
    defaultState.height = 8.0;
    defaultState.zoom = 1.0;
}

std::shared_ptr<MovingCamera> MovingCameraScene::getMovingCamera() const {
    // TODO: Implement proper camera casting
    return nullptr; // std::dynamic_pointer_cast<MovingCamera>(camera);
}

void MovingCameraScene::zoomToMobject(std::shared_ptr<Mobject> mobject,
                                      double zoomFactor,
                                      double duration) {
    if (!mobject) return;

    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    // Get mobject bounding box
    // TODO: Get actual bounding box from mobject
    // For now, use placeholder logic

    // Calculate target camera position and width
    // Vec3 targetPos = mobject->getCenter();
    // double targetWidth = mobject->getWidth() * zoomFactor;

    // Animate camera to target
    // TODO: Create camera animation
    std::cout << "Zooming to mobject with factor " << zoomFactor << std::endl;
}

void MovingCameraScene::autoZoom(const std::vector<std::shared_ptr<Mobject>>& mobjects,
                                 double margin,
                                 bool animate,
                                 double duration) {
    if (mobjects.empty()) return;

    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    // Calculate bounding box of all mobjects
    // TODO: Compute combined bounding box

    // Calculate target camera settings
    // TODO: Compute camera position and size to fit all mobjects

    if (animate) {
        std::cout << "Auto-zooming to " << mobjects.size() << " mobjects with animation" << std::endl;
        // TODO: Animate camera
    } else {
        std::cout << "Auto-zooming to " << mobjects.size() << " mobjects instantly" << std::endl;
        // TODO: Set camera immediately
    }
}

void MovingCameraScene::panTo(const Vec3& position, double duration) {
    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    if (duration > 0.0) {
        std::cout << "Panning camera to (" << position.x << ", " << position.y << ", " << position.z
                  << ") over " << duration << "s" << std::endl;
        // TODO: Animate camera position
    } else {
        movingCam->setPosition(position);
    }
}

void MovingCameraScene::setCameraFrameWidth(double width, double duration) {
    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    if (duration > 0.0) {
        std::cout << "Animating camera width to " << width << " over " << duration << "s" << std::endl;
        // TODO: Animate camera width
    } else {
        movingCam->setFrameWidth(width);
    }
}

void MovingCameraScene::setCameraFrameHeight(double height, double duration) {
    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    if (duration > 0.0) {
        std::cout << "Animating camera height to " << height << " over " << duration << "s" << std::endl;
        // TODO: Animate camera height
    } else {
        movingCam->setFrameHeight(height);
    }
}

void MovingCameraScene::resetCamera(double duration) {
    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    if (duration > 0.0) {
        std::cout << "Resetting camera over " << duration << "s" << std::endl;
        // TODO: Animate camera to default state
    } else {
        movingCam->setPosition(defaultState.position);
        movingCam->setFrameWidth(defaultState.width);
        movingCam->setFrameHeight(defaultState.height);
        movingCam->setZoom(defaultState.zoom);
    }
}

void MovingCameraScene::saveCameraState() {
    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    CameraState state;
    state.position = movingCam->getPosition();
    state.width = movingCam->getFrameWidth();
    state.height = movingCam->getFrameHeight();
    state.zoom = movingCam->getZoom();

    savedState = state;
    std::cout << "Camera state saved" << std::endl;
}

void MovingCameraScene::restoreCameraState(double duration) {
    if (!savedState.has_value()) {
        std::cerr << "No saved camera state to restore!" << std::endl;
        return;
    }

    auto movingCam = getMovingCamera();
    if (!movingCam) return;

    const auto& state = savedState.value();

    if (duration > 0.0) {
        std::cout << "Restoring camera state over " << duration << "s" << std::endl;
        // TODO: Animate camera to saved state
    } else {
        movingCam->setPosition(state.position);
        movingCam->setFrameWidth(state.width);
        movingCam->setFrameHeight(state.height);
        movingCam->setZoom(state.zoom);
    }
}

} // namespace manim
