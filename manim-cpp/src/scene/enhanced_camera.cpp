#include "manim/scene/enhanced_camera.hpp"
#include "manim/scene/scene.hpp"
#include <iostream>
#include <cmath>

namespace manim {

EnhancedCamera::EnhancedCamera()
    : renderMode(CameraRenderMode::MONO)
    , ipd(0.064f)
    , convergenceDistance(2.0f)
    , vrEnabled(false)
    , projectionType(ProjectionType::PERSPECTIVE)
    , fov(60.0f)
    , nearPlane(0.1f)
    , farPlane(1000.0f)
    , hdrEnabled(true)
    , exposure(1.0f)
    , autoExposure(false)
    , toneMappingType("ACES")
{
    std::cout << "Enhanced camera initialized" << std::endl;
}

EnhancedCamera::~EnhancedCamera() {
}

// ==================== Rendering Mode ====================

void EnhancedCamera::setRenderMode(CameraRenderMode mode) {
    renderMode = mode;

    std::string modeStr;
    switch (mode) {
        case CameraRenderMode::MONO: modeStr = "Mono"; break;
        case CameraRenderMode::STEREO: modeStr = "Stereo"; break;
        case CameraRenderMode::MULTI_VIEW: modeStr = "Multi-View"; break;
        case CameraRenderMode::VR: modeStr = "VR"; break;
        case CameraRenderMode::PANORAMIC: modeStr = "Panoramic"; break;
        case CameraRenderMode::FISHEYE: modeStr = "Fisheye"; break;
    }

    std::cout << "Camera render mode set to: " << modeStr << std::endl;
}

// ==================== Multi-View Rendering ====================

void EnhancedCamera::addViewport(const Viewport& viewport) {
    viewports.push_back(viewport);
    std::cout << "Viewport added (total: " << viewports.size() << ")" << std::endl;
}

void EnhancedCamera::removeViewport(size_t index) {
    if (index < viewports.size()) {
        viewports.erase(viewports.begin() + index);
        std::cout << "Viewport removed (remaining: " << viewports.size() << ")" << std::endl;
    }
}

void EnhancedCamera::setViewportEnabled(size_t index, bool enabled) {
    if (index < viewports.size()) {
        viewports[index].enabled = enabled;
    }
}

void EnhancedCamera::renderMultiView(Scene& scene) {
    std::cout << "Rendering multi-view with " << viewports.size() << " viewports" << std::endl;

    for (size_t i = 0; i < viewports.size(); ++i) {
        if (!viewports[i].enabled) continue;

        // Setup viewport
        // TODO: Set viewport rectangle
        // TODO: Render with viewport camera

        std::cout << "  Rendering viewport " << i << std::endl;
    }
}

// ==================== Stereoscopic 3D ====================

void EnhancedCamera::setupStereo(float ipd_, float convergenceDistance_) {
    ipd = ipd_;
    convergenceDistance = convergenceDistance_;

    // Create left and right eye cameras
    // TODO: Initialize camera objects
    leftEyeCamera = nullptr; // std::make_shared<Camera>();
    rightEyeCamera = nullptr; // std::make_shared<Camera>();

    updateStereoOffset();

    std::cout << "Stereoscopic 3D setup:" << std::endl;
    std::cout << "  IPD: " << ipd << " m" << std::endl;
    std::cout << "  Convergence distance: " << convergenceDistance << " m" << std::endl;
}

void EnhancedCamera::renderStereo(Scene& scene, StereoEye eye) {
    switch (eye) {
        case StereoEye::LEFT:
            std::cout << "Rendering left eye" << std::endl;
            // TODO: Render with left eye camera
            break;

        case StereoEye::RIGHT:
            std::cout << "Rendering right eye" << std::endl;
            // TODO: Render with right eye camera
            break;

        case StereoEye::BOTH:
            std::cout << "Rendering both eyes" << std::endl;
            renderStereo(scene, StereoEye::LEFT);
            renderStereo(scene, StereoEye::RIGHT);
            break;
    }
}

void EnhancedCamera::updateStereoOffset() {
    // Calculate eye separation based on IPD
    float separation = ipd / 2.0f;

    // Apply convergence
    float convergenceAngle = std::atan(separation / convergenceDistance);

    // TODO: Update left and right eye camera positions and rotations
    std::cout << "Updated stereo offset (separation: " << separation << ")" << std::endl;
}

// ==================== VR Support ====================

void EnhancedCamera::setupVR(const VRConfig& config) {
    vrConfig = config;
    vrEnabled = true;

    // Setup stereo rendering for VR
    setupStereo(vrConfig.ipd, vrConfig.convergenceDistance);

    std::cout << "VR setup complete:" << std::endl;
    std::cout << "  IPD: " << vrConfig.ipd << " m" << std::endl;
    std::cout << "  Tracking: " << (vrConfig.trackingEnabled ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::updateVRTracking(const Vec3& headPosition, const Vec3& headRotation) {
    vrConfig.headPosition = headPosition;
    vrConfig.headRotation = headRotation;

    // Update camera position and rotation based on tracking
    // TODO: Apply tracking data to camera

    if (vrConfig.trackingEnabled) {
        updateStereoOffset();
    }
}

void EnhancedCamera::renderVR(Scene& scene) {
    if (!vrEnabled) {
        std::cerr << "VR not enabled!" << std::endl;
        return;
    }

    std::cout << "Rendering VR scene" << std::endl;

    // Update tracking if enabled
    if (vrConfig.trackingEnabled) {
        // TODO: Query VR system for latest tracking data
    }

    // Render left and right eyes
    renderStereo(scene, StereoEye::BOTH);
}

// ==================== Cinematic Controls ====================

void EnhancedCamera::applyCinematicSettings(const CinematicSettings& settings) {
    cinematicSettings = settings;

    std::cout << "Applied cinematic settings:" << std::endl;
    std::cout << "  Focal length: " << settings.focalLength << "mm" << std::endl;
    std::cout << "  Aperture: f/" << settings.aperture << std::endl;
    std::cout << "  Focus distance: " << settings.focusDistance << "m" << std::endl;
    std::cout << "  Motion blur: " << (settings.motionBlur ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::setFocalLength(float focalLength) {
    cinematicSettings.focalLength = focalLength;

    // Update FOV based on focal length
    // FOV = 2 * atan(sensorSize / (2 * focalLength))
    // Assuming 35mm full-frame sensor (36mm width)
    float sensorWidth = 36.0f;
    fov = 2.0f * std::atan(sensorWidth / (2.0f * focalLength)) * 180.0f / M_PI;

    std::cout << "Focal length set to " << focalLength << "mm (FOV: " << fov << "°)" << std::endl;
}

void EnhancedCamera::setAperture(float aperture) {
    cinematicSettings.aperture = aperture;
    std::cout << "Aperture set to f/" << aperture << std::endl;
}

void EnhancedCamera::setFocusDistance(float distance) {
    cinematicSettings.focusDistance = distance;
    std::cout << "Focus distance set to " << distance << "m" << std::endl;
}

void EnhancedCamera::setDepthOfField(bool enabled, float intensity) {
    cinematicSettings.bokehIntensity = enabled ? intensity : 0.0f;
    std::cout << "Depth of field " << (enabled ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::setMotionBlur(bool enabled, float shutterSpeed) {
    cinematicSettings.motionBlur = enabled;
    cinematicSettings.shutterSpeed = shutterSpeed;
    std::cout << "Motion blur " << (enabled ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::setFilmGrain(bool enabled, float intensity) {
    cinematicSettings.filmGrain = enabled ? intensity : 0.0f;
    std::cout << "Film grain " << (enabled ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::setVignette(bool enabled, float intensity) {
    cinematicSettings.vignette = enabled;
    cinematicSettings.vignetteIntensity = intensity;
    std::cout << "Vignette " << (enabled ? "enabled" : "disabled") << std::endl;
}

// ==================== Camera Animation ====================

void EnhancedCamera::dollyZoom(float targetFOV, double duration) {
    std::cout << "Dolly zoom to FOV " << targetFOV << "° over " << duration << "s" << std::endl;
    // TODO: Animate camera position and FOV simultaneously
    // to maintain subject size while changing perspective
}

void EnhancedCamera::pan(const Vec3& direction, double duration) {
    std::cout << "Panning camera over " << duration << "s" << std::endl;
    // TODO: Rotate camera horizontally
}

void EnhancedCamera::tilt(float angle, double duration) {
    std::cout << "Tilting camera " << angle << "° over " << duration << "s" << std::endl;
    // TODO: Rotate camera vertically
}

void EnhancedCamera::truck(const Vec3& direction, double duration) {
    std::cout << "Trucking camera over " << duration << "s" << std::endl;
    // TODO: Move camera sideways
}

void EnhancedCamera::pedestal(float distance, double duration) {
    std::cout << "Pedestal camera " << distance << " units over " << duration << "s" << std::endl;
    // TODO: Move camera up/down
}

void EnhancedCamera::orbit(const Vec3& center, float angle, double duration) {
    std::cout << "Orbiting camera " << angle << "° around point over " << duration << "s" << std::endl;
    // TODO: Rotate camera around point
}

// ==================== Projection ====================

void EnhancedCamera::setProjectionType(ProjectionType type) {
    projectionType = type;

    std::string typeStr;
    switch (type) {
        case ProjectionType::PERSPECTIVE: typeStr = "Perspective"; break;
        case ProjectionType::ORTHOGRAPHIC: typeStr = "Orthographic"; break;
        case ProjectionType::FISHEYE: typeStr = "Fisheye"; break;
        case ProjectionType::PANORAMIC: typeStr = "Panoramic"; break;
    }

    std::cout << "Projection type set to: " << typeStr << std::endl;
}

void EnhancedCamera::setFOV(float fov_) {
    fov = fov_;
    std::cout << "FOV set to " << fov << "°" << std::endl;
}

// ==================== Advanced Features ====================

void EnhancedCamera::setHDR(bool enabled) {
    hdrEnabled = enabled;
    std::cout << "HDR " << (enabled ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::setExposure(float exposure_) {
    exposure = exposure_;
    std::cout << "Exposure set to " << exposure << std::endl;
}

void EnhancedCamera::setAutoExposure(bool enabled) {
    autoExposure = enabled;
    std::cout << "Auto-exposure " << (enabled ? "enabled" : "disabled") << std::endl;
}

void EnhancedCamera::setToneMapping(const std::string& type) {
    toneMappingType = type;
    std::cout << "Tone mapping set to: " << type << std::endl;
}

// ==================== Helper Methods ====================

void EnhancedCamera::applyDepthOfField() {
    if (cinematicSettings.bokehIntensity <= 0.0f) return;

    // TODO: Implement depth of field post-processing
    // - Calculate circle of confusion
    // - Apply bokeh blur based on depth
}

void EnhancedCamera::applyMotionBlur() {
    if (!cinematicSettings.motionBlur) return;

    // TODO: Implement motion blur
    // - Track object velocities
    // - Apply directional blur
}

void EnhancedCamera::applyFilmGrain() {
    if (cinematicSettings.filmGrain <= 0.0f) return;

    // TODO: Implement film grain
    // - Generate noise texture
    // - Blend with image
}

void EnhancedCamera::applyVignette() {
    if (!cinematicSettings.vignette) return;

    // TODO: Implement vignette effect
    // - Darken edges based on distance from center
}

// ==================== EnhancedCameraScene Implementation ====================

EnhancedCameraScene::EnhancedCameraScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
)
    : Scene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations)
    , enhancedCamera(std::make_shared<EnhancedCamera>())
{
    name = "EnhancedCameraScene";
}

void EnhancedCameraScene::setupForVR(const VRConfig& config) {
    enhancedCamera->setupVR(config);
    enhancedCamera->setRenderMode(CameraRenderMode::VR);
}

void EnhancedCameraScene::setupForStereo(float ipd, float convergenceDistance) {
    enhancedCamera->setupStereo(ipd, convergenceDistance);
    enhancedCamera->setRenderMode(CameraRenderMode::STEREO);
}

void EnhancedCameraScene::setupMultiView(const std::vector<Viewport>& viewports) {
    for (const auto& viewport : viewports) {
        enhancedCamera->addViewport(viewport);
    }
    enhancedCamera->setRenderMode(CameraRenderMode::MULTI_VIEW);
}

void EnhancedCameraScene::applyCinematicLook(const CinematicSettings& settings) {
    enhancedCamera->applyCinematicSettings(settings);
}

} // namespace manim
