#pragma once

#include "manim/core/types.h"
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

namespace manim {

// Forward declarations
class Camera;
class Scene;

/**
 * @brief Camera rendering mode
 */
enum class CameraRenderMode {
    MONO,           // Standard single-view
    STEREO,         // Stereoscopic 3D
    MULTI_VIEW,     // Multiple viewports
    VR,             // Virtual Reality
    PANORAMIC,      // 360-degree panoramic
    FISHEYE         // Fisheye lens
};

/**
 * @brief Stereoscopic eye
 */
enum class StereoEye {
    LEFT,
    RIGHT,
    BOTH
};

/**
 * @brief Camera projection type
 */
enum class ProjectionType {
    PERSPECTIVE,
    ORTHOGRAPHIC,
    FISHEYE,
    PANORAMIC
};

/**
 * @brief Multi-view camera viewport
 */
struct Viewport {
    Vec2 position;      // Normalized position (0-1)
    Vec2 size;          // Normalized size (0-1)
    std::shared_ptr<Camera> camera;
    bool enabled;

    Viewport()
        : position(0.0f, 0.0f)
        , size(1.0f, 1.0f)
        , camera(nullptr)
        , enabled(true)
    {}
};

/**
 * @brief VR camera configuration
 */
struct VRConfig {
    float ipd;                  // Interpupillary distance
    float convergenceDistance;  // Convergence plane distance
    Vec3 headPosition;
    Vec3 headRotation;
    bool trackingEnabled;

    VRConfig()
        : ipd(0.064f)  // 64mm average IPD
        , convergenceDistance(2.0f)
        , headPosition(0.0f, 0.0f, 0.0f)
        , headRotation(0.0f, 0.0f, 0.0f)
        , trackingEnabled(false)
    {}
};

/**
 * @brief Cinematic camera settings
 */
struct CinematicSettings {
    float focalLength;          // in mm
    float aperture;             // f-stop
    float focusDistance;        // Distance to focus plane
    float bokehIntensity;       // Depth of field intensity
    bool motionBlur;
    float shutterSpeed;
    float filmGrain;
    bool vignette;
    float vignetteIntensity;

    CinematicSettings()
        : focalLength(50.0f)
        , aperture(2.8f)
        , focusDistance(5.0f)
        , bokehIntensity(1.0f)
        , motionBlur(false)
        , shutterSpeed(1.0f / 60.0f)
        , filmGrain(0.0f)
        , vignette(false)
        , vignetteIntensity(0.5f)
    {}
};

/**
 * @brief Enhanced camera system with advanced features
 */
class EnhancedCamera {
public:
    EnhancedCamera();
    virtual ~EnhancedCamera();

    // ==================== Rendering Mode ====================

    /**
     * @brief Set camera rendering mode
     */
    void setRenderMode(CameraRenderMode mode);

    /**
     * @brief Get current rendering mode
     */
    CameraRenderMode getRenderMode() const { return renderMode; }

    // ==================== Multi-View Rendering ====================

    /**
     * @brief Add viewport for multi-view rendering
     */
    void addViewport(const Viewport& viewport);

    /**
     * @brief Remove viewport
     */
    void removeViewport(size_t index);

    /**
     * @brief Get all viewports
     */
    const std::vector<Viewport>& getViewports() const { return viewports; }

    /**
     * @brief Enable/disable viewport
     */
    void setViewportEnabled(size_t index, bool enabled);

    /**
     * @brief Render all viewports
     */
    void renderMultiView(Scene& scene);

    // ==================== Stereoscopic 3D ====================

    /**
     * @brief Setup stereoscopic rendering
     * @param ipd Interpupillary distance
     * @param convergenceDistance Distance to convergence plane
     */
    void setupStereo(float ipd = 0.064f, float convergenceDistance = 2.0f);

    /**
     * @brief Render stereoscopic view
     * @param eye Which eye to render
     */
    void renderStereo(Scene& scene, StereoEye eye);

    /**
     * @brief Get left eye camera
     */
    std::shared_ptr<Camera> getLeftEyeCamera() const { return leftEyeCamera; }

    /**
     * @brief Get right eye camera
     */
    std::shared_ptr<Camera> getRightEyeCamera() const { return rightEyeCamera; }

    // ==================== VR Support ====================

    /**
     * @brief Setup VR rendering
     */
    void setupVR(const VRConfig& config);

    /**
     * @brief Update VR tracking
     * @param headPosition Position from VR tracker
     * @param headRotation Rotation from VR tracker
     */
    void updateVRTracking(const Vec3& headPosition, const Vec3& headRotation);

    /**
     * @brief Render VR scene
     */
    void renderVR(Scene& scene);

    /**
     * @brief Get VR configuration
     */
    const VRConfig& getVRConfig() const { return vrConfig; }

    // ==================== Cinematic Controls ====================

    /**
     * @brief Apply cinematic settings
     */
    void applyCinematicSettings(const CinematicSettings& settings);

    /**
     * @brief Set focal length (in mm)
     */
    void setFocalLength(float focalLength);

    /**
     * @brief Set aperture (f-stop)
     */
    void setAperture(float aperture);

    /**
     * @brief Set focus distance
     */
    void setFocusDistance(float distance);

    /**
     * @brief Enable/disable depth of field
     */
    void setDepthOfField(bool enabled, float intensity = 1.0f);

    /**
     * @brief Enable/disable motion blur
     */
    void setMotionBlur(bool enabled, float shutterSpeed = 1.0f / 60.0f);

    /**
     * @brief Enable/disable film grain
     */
    void setFilmGrain(bool enabled, float intensity = 0.1f);

    /**
     * @brief Enable/disable vignette
     */
    void setVignette(bool enabled, float intensity = 0.5f);

    /**
     * @brief Get cinematic settings
     */
    const CinematicSettings& getCinematicSettings() const { return cinematicSettings; }

    // ==================== Camera Animation ====================

    /**
     * @brief Dolly zoom (Hitchcock effect)
     * @param targetFOV Target field of view
     * @param duration Animation duration
     */
    void dollyZoom(float targetFOV, double duration);

    /**
     * @brief Pan camera
     */
    void pan(const Vec3& direction, double duration);

    /**
     * @brief Tilt camera
     */
    void tilt(float angle, double duration);

    /**
     * @brief Truck camera (move sideways)
     */
    void truck(const Vec3& direction, double duration);

    /**
     * @brief Pedestal camera (move up/down)
     */
    void pedestal(float distance, double duration);

    /**
     * @brief Orbit around point
     */
    void orbit(const Vec3& center, float angle, double duration);

    // ==================== Projection ====================

    /**
     * @brief Set projection type
     */
    void setProjectionType(ProjectionType type);

    /**
     * @brief Get projection type
     */
    ProjectionType getProjectionType() const { return projectionType; }

    /**
     * @brief Set field of view
     */
    void setFOV(float fov);

    /**
     * @brief Get field of view
     */
    float getFOV() const { return fov; }

    // ==================== Advanced Features ====================

    /**
     * @brief Enable/disable HDR rendering
     */
    void setHDR(bool enabled);

    /**
     * @brief Set exposure
     */
    void setExposure(float exposure);

    /**
     * @brief Enable/disable auto-exposure
     */
    void setAutoExposure(bool enabled);

    /**
     * @brief Set tone mapping type
     */
    void setToneMapping(const std::string& type);

protected:
    // Rendering mode
    CameraRenderMode renderMode;

    // Multi-view
    std::vector<Viewport> viewports;

    // Stereoscopic
    std::shared_ptr<Camera> leftEyeCamera;
    std::shared_ptr<Camera> rightEyeCamera;
    float ipd;
    float convergenceDistance;

    // VR
    VRConfig vrConfig;
    bool vrEnabled;

    // Cinematic
    CinematicSettings cinematicSettings;

    // Projection
    ProjectionType projectionType;
    float fov;
    float nearPlane;
    float farPlane;

    // HDR
    bool hdrEnabled;
    float exposure;
    bool autoExposure;
    std::string toneMappingType;

    // Helper methods
    void updateStereoOffset();
    void applyDepthOfField();
    void applyMotionBlur();
    void applyFilmGrain();
    void applyVignette();
};

/**
 * @brief Scene with enhanced camera support
 */
class EnhancedCameraScene : public Scene {
public:
    EnhancedCameraScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~EnhancedCameraScene() = default;

    /**
     * @brief Get enhanced camera
     */
    std::shared_ptr<EnhancedCamera> getEnhancedCamera() const {
        return enhancedCamera;
    }

    /**
     * @brief Setup for VR rendering
     */
    void setupForVR(const VRConfig& config);

    /**
     * @brief Setup for stereoscopic rendering
     */
    void setupForStereo(float ipd = 0.064f, float convergenceDistance = 2.0f);

    /**
     * @brief Setup multi-view rendering
     */
    void setupMultiView(const std::vector<Viewport>& viewports);

    /**
     * @brief Apply cinematic look
     */
    void applyCinematicLook(const CinematicSettings& settings);

protected:
    std::shared_ptr<EnhancedCamera> enhancedCamera;
};

} // namespace manim
