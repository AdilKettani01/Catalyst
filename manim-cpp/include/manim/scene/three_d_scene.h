#pragma once

#include "manim/scene/scene.h"
#include "manim/scene/camera.hpp"
#include "manim/core/types.h"
#include "manim/mobject/three_d/mesh.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace manim {

// Forward declarations
class Light;
class EnvironmentMap;

namespace culling {
    class GPUCullingPipeline;
    struct CullingStats;
}
class AccelerationStructure;

class ThreeDCamera : public Camera {
public:
    void setPhi(double v) { phi_ = v; }
    void setTheta(double v) { theta_ = v; }
    void setGamma(double v) { gamma_ = v; }
    void setZoom(double v) { zoom_ = v; }
    void setPosition(const Vec3& pos) { position_ = pos; }
    void setFocalDistance(double v) { focal_distance_ = v; }
    void setFrameCenter(const Vec3& c) { frame_center_ = c; }

    double getPhi() const { return phi_; }
    double getTheta() const { return theta_; }
    double getGamma() const { return gamma_; }
    double getZoom() const { return zoom_; }

private:
    double phi_ = 0.0;
    double theta_ = 0.0;
    double gamma_ = 0.0;
    double zoom_ = 1.0;
    Vec3 position_{0.0f, 0.0f, 0.0f};
    double focal_distance_ = 1.0;
    Vec3 frame_center_{0.0f, 0.0f, 0.0f};
};

/**
 * @brief Light types for 3D scenes
 */
enum class LightType {
    POINT,
    DIRECTIONAL,
    SPOT,
    AREA,
    AMBIENT
};

enum class ShadowType {
    NONE,
    BASIC,
    CASCADED,
    RAY_TRACED
};

enum class ToneMappingType {
    NONE,
    REINHARD,
    ACES,
    FILMIC
};

/**
 * @brief Light representation
 */
struct Light {
    LightType type;
    Vec3 position;
    Vec3 direction;
    Vec4 color;
    float intensity;
    float radius;
    float innerConeAngle;  // For spot lights
    float outerConeAngle;  // For spot lights
    bool castsShadows;
    float inner_angle = 0.0f;
    float outer_angle = 0.0f;

    Light()
        : type(LightType::POINT)
        , position(0.0f, 0.0f, 0.0f)
        , direction(0.0f, -1.0f, 0.0f)
        , color(1.0f, 1.0f, 1.0f, 1.0f)
        , intensity(1.0f)
        , radius(10.0f)
        , innerConeAngle(0.0f)
        , outerConeAngle(0.0f)
        , castsShadows(true)
    {}
};

/**
 * @brief Environment map for image-based lighting
 */
class EnvironmentMap {
public:
    EnvironmentMap();
    ~EnvironmentMap();

    void loadFromFile(const std::string& path);
    void generateIrradianceMap();
    void generateSpecularMap();
    void generateBRDFLUT();

    VkImageView getCubemapView() const { return cubemapView; }
    VkImageView getIrradianceView() const { return irradianceView; }
    VkImageView getSpecularView() const { return specularView; }
    VkImageView getBRDFLUTView() const { return brdfLUTView; }

private:
    VkImage cubemap;
    VkImageView cubemapView;
    VkDeviceMemory cubemapMemory;

    VkImage irradianceMap;
    VkImageView irradianceView;
    VkDeviceMemory irradianceMemory;

    VkImage specularMap;
    VkImageView specularView;
    VkDeviceMemory specularMemory;

    VkImage brdfLUT;
    VkImageView brdfLUTView;
    VkDeviceMemory brdfLUTMemory;
};

struct PBRMaterial {
    Vec4 albedo{1.0f, 1.0f, 1.0f, 1.0f};
    float roughness{1.0f};
    float metallic{0.0f};
    float ao{1.0f};
};

struct RenderStats {
    uint32_t lod_switches = 0;
    float fps = 0.0f;
};

/**
 * @brief Traditional 3D Scene with basic camera controls
 */
class ThreeDScene : public Scene {
public:
    ThreeDScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~ThreeDScene() = default;

    /**
     * @brief Set camera orientation
     * @param phi Polar angle (angle from Z axis)
     * @param theta Azimuthal angle (rotation around Z axis)
     * @param gamma Roll angle
     * @param zoom Zoom factor
     * @param focalDistance Focal distance
     * @param frameCenter Center of camera frame
     */
    void setCameraOrientation(
        std::optional<double> phi = std::nullopt,
        std::optional<double> theta = std::nullopt,
        std::optional<double> gamma = std::nullopt,
        std::optional<double> zoom = std::nullopt,
        std::optional<double> focalDistance = std::nullopt,
        std::optional<Vec3> frameCenter = std::nullopt
    );
    void set_camera_orientation(
        std::optional<double> phi = std::nullopt,
        std::optional<double> theta = std::nullopt,
        std::optional<double> gamma = std::nullopt,
        std::optional<double> zoom = std::nullopt,
        std::optional<double> focalDistance = std::nullopt,
        std::optional<Vec3> frameCenter = std::nullopt
    ) {
        setCameraOrientation(phi, theta, gamma, zoom, focalDistance, frameCenter);
    }

    /**
     * @brief Begin ambient camera rotation
     * @param rate Rotation rate (radians per second)
     * @param about Axis to rotate about ("theta", "phi", or "gamma")
     */
    void beginAmbientCameraRotation(double rate = 0.02, const std::string& about = "theta");
    void begin_ambient_camera_rotation(double rate = 0.02, const std::string& about = "theta") {
        beginAmbientCameraRotation(rate, about);
    }

    /**
     * @brief Stop ambient camera rotation
     * @param about Axis to stop rotating about
     */
    void stopAmbientCameraRotation(const std::string& about = "theta");
    void stop_ambient_camera_rotation(const std::string& about = "theta") { stopAmbientCameraRotation(about); }

    /**
     * @brief Move camera to position
     */
    void moveCameraTo(const Vec3& position, double duration = 1.0);
    void move_camera_to(const Vec3& position, double duration = 1.0) { moveCameraTo(position, duration); }
    void move_camera_to(double phi, double theta, double distance) {
        moveCameraTo(Vec3(static_cast<float>(phi), static_cast<float>(theta), static_cast<float>(distance)));
    }

    /**
     * @brief Get 3D camera
     */
    std::shared_ptr<ThreeDCamera> get3DCamera() const;

protected:
    double defaultPhi;
    double defaultTheta;
    double ambientRotationRate;
    std::string ambientRotationAxis;
    bool isAmbientRotating;
};

/**
 * @brief GPU-accelerated 3D Scene with advanced rendering
 *
 * This scene type provides:
 * - Deferred rendering pipeline
 * - Physically-Based Rendering (PBR)
 * - Real-time ray tracing
 * - Global illumination
 * - Volumetric effects
 * - GPU-based culling
 * - Advanced lighting
 */
class GPU3DScene : public ThreeDScene {
public:
    GPU3DScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~GPU3DScene();

    // ==================== Setup & Initialization ====================

    /**
     * @brief Setup deferred rendering pipeline
     */
    void setupDeferredPipeline();
    void setup_deferred_pipeline() { setupDeferredPipeline(); }

    /**
     * @brief Initialize PBR materials
     */
    void initializePBRMaterials();
    void initialize_pbr_materials() { initializePBRMaterials(); }

    /**
     * @brief Setup ray tracing
     */
    void setupRayTracing();
    void setup_ray_tracing() { setupRayTracing(); }

    // ==================== Lighting ====================

    /**
     * @brief Add light to scene
     */
    void addLight(const Light& light);
    void add_light(const Light& light) { addLight(light); }
    void addDirectionalLight(const Vec3& /*direction*/) { addLight(Light{}); }
    void add_directional_light(const Vec3& direction) { addDirectionalLight(direction); }

    /**
     * @brief Remove light from scene
     */
    void removeLight(size_t index);
    void remove_light(size_t index) { removeLight(index); }

    /**
     * @brief Set environment map
     */
    void setEnvironmentMap(std::shared_ptr<EnvironmentMap> envMap);
    void set_environment_map(std::shared_ptr<EnvironmentMap> envMap) { setEnvironmentMap(std::move(envMap)); }
    void set_environment_map(const std::string& /*path*/) {}

    /**
     * @brief Update lighting system
     */
    void updateLighting();

    // ==================== Rendering ====================

    /**
     * @brief Render with PBR
     */
    void renderWithPBR();
    void render_with_pbr() { renderWithPBR(); }

    /**
     * @brief Compute global illumination
     */
    void computeGlobalIllumination();
    void compute_global_illumination() { computeGlobalIllumination(); }

    /**
     * @brief Render volumetric effects
     */
    void renderVolumetrics();
    void render_volumetrics() { renderVolumetrics(); }

    /**
     * @brief Render with ray tracing
     */
    void rayTraceScene();
    void ray_trace_scene() { rayTraceScene(); }
    void render_frame() {}
    void enable_shadows(ShadowType /*type*/) {}
    bool supports_ray_tracing() const { return rayTracingEnabled; }
    void render_ray_traced_frame() {}
    void enable_global_illumination() { computeGlobalIllumination(); }
    void clear_lights() { lights.clear(); }
    void enable_volumetric_fog(bool /*enable*/) {}
    void set_fog_density(float /*density*/) {}
    void set_fog_color(const Vec4& /*color*/) {}
    void enable_volumetric_lighting(bool enable);
    void enable_bloom(bool enable);
    void set_bloom_intensity(float intensity);
    void enable_taa(bool enable);
    void enable_ssao(bool enable);
    void set_ssao_radius(float radius);
    void set_tone_mapping(ToneMappingType type);

    // ==================== GPU Culling ====================

    /**
     * @brief Frustum culling on GPU
     */
    void frustumCullGPU();
    void enable_frustum_culling(bool enable);

    /**
     * @brief Occlusion culling on GPU
     */
    void occlusionCullGPU();
    void enable_occlusion_culling(bool enable);

    /**
     * @brief LOD selection on GPU
     */
    void LODSelectionGPU();
    void enable_lod(bool enable);

    // ==================== Acceleration Structures ====================

    /**
     * @brief Build acceleration structures for ray tracing
     */
    void buildAccelerationStructures();

    /**
     * @brief Update acceleration structures
     */
    void updateAccelerationStructures();

    // ==================== Post-Processing ====================

    /**
     * @brief Apply screen-space ambient occlusion (SSAO)
     */
    void applySSAO();
    void apply_ssao() { applySSAO(); }

    /**
     * @brief Apply screen-space reflections (SSR)
     */
    void applySSR();
    void apply_ssr() { applySSR(); }

    /**
     * @brief Apply bloom effect
     */
    void applyBloom();
    void apply_bloom() { applyBloom(); }

    /**
     * @brief Apply tone mapping
     */
    void applyToneMapping();
    void apply_tone_mapping() { applyToneMapping(); }

    // ==================== Getters ====================

    const std::vector<Light>& getLights() const { return lights; }
    std::shared_ptr<EnvironmentMap> getEnvironmentMap() const { return environmentMap; }
    bool isRayTracingEnabled() const { return rayTracingEnabled; }
    bool isDeferredRenderingEnabled() const { return deferredRenderingEnabled; }
    RenderStats get_render_stats() const { return renderStats_; }

protected:
    // Lighting
    std::vector<Light> lights;
    std::shared_ptr<EnvironmentMap> environmentMap;
    Vec3 ambientColor;
    float ambientIntensity;

    // Ray tracing
    VkAccelerationStructureKHR TLAS;  // Top-level acceleration structure
    std::vector<VkAccelerationStructureKHR> BLAS;  // Bottom-level acceleration structures
    VkBuffer tlasBuffer;
    VkDeviceMemory tlasMemory;
    bool rayTracingEnabled;

    // Deferred rendering
    VkFramebuffer gBuffer;
    VkImage gBufferPosition;
    VkImage gBufferNormal;
    VkImage gBufferAlbedo;
    VkImage gBufferRoughness;
    VkImage gBufferMetallic;
    VkImage gBufferDepth;
    bool deferredRenderingEnabled;

    // Post-processing
    bool ssaoEnabled;
    bool ssrEnabled;
    bool bloomEnabled;
    bool toneMappingEnabled;
    bool volumetricLightingEnabled;
    bool taaEnabled;
    float bloomIntensity;
    float ssaoRadius;
    ToneMappingType toneMappingType;
    float exposure;

    // GPU culling
    VkBuffer visibilityBuffer;
    VkDeviceMemory visibilityMemory;
    bool gpuCullingEnabled;
    bool frustumCullingEnabled;
    bool occlusionCullingEnabled;
    bool lodEnabled;
    RenderStats renderStats_;

    // GPU culling pipeline (new system)
    std::unique_ptr<culling::GPUCullingPipeline> culling_pipeline_;

    // Helper methods
    void createGBuffer();
    void createRayTracingResources();
    void createCullingResources();
    void initializeCullingPipeline();
    void geometryPass();
    void lightingPass();
    void postProcessingPass();
};

/**
 * @brief Special 3D Scene with additional features
 */
class SpecialThreeDScene : public GPU3DScene {
public:
    SpecialThreeDScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~SpecialThreeDScene() = default;

    /**
     * @brief Get default camera position
     */
    Vec3 getDefaultCameraPosition() const;

    /**
     * @brief Set default camera position
     */
    void setDefaultCameraPosition(const Vec3& position);

protected:
    Vec3 defaultCameraPosition;
};

} // namespace manim
