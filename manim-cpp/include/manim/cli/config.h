#pragma once

#include "manim/core/types.h"
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <thread>

namespace manim {

/**
 * @brief GPU backend selection
 */
enum class GPUBackend {
    AUTO,       // Automatic selection
    VULKAN,     // Vulkan API
    OPENGL,     // OpenGL (compatibility)
    DIRECTX12,  // DirectX 12 (Windows)
    METAL,      // Metal (macOS/iOS)
    CUDA,       // NVIDIA CUDA
    OPENCL      // OpenCL (cross-platform)
};

/**
 * @brief Rendering quality presets
 */
enum class QualityPreset {
    LOW,           // Fast preview
    MEDIUM,        // Balanced
    HIGH,          // Production quality
    ULTRA,         // Maximum quality
    CUSTOM         // User-defined
};

/**
 * @brief Anti-aliasing modes
 */
enum class AAMode {
    NONE,
    FXAA,
    MSAA_2X,
    MSAA_4X,
    MSAA_8X,
    TAA,           // Temporal AA
    SMAA           // Enhanced SMAA
};

/**
 * @brief Comprehensive render configuration
 */
struct RenderConfig {
    // ==================== Output Settings ====================
    std::string outputFile = "output.mp4";
    std::string outputDirectory = "./media";
    std::string sceneName = "";
    int width = 1920;
    int height = 1080;
    int frameRate = 60;
    QualityPreset quality = QualityPreset::HIGH;

    // ==================== GPU Settings ====================
    GPUBackend backend = GPUBackend::AUTO;
    int gpuDevice = 0;                      // GPU device index
    bool enableRayTracing = true;           // Hardware ray tracing
    bool enableCompute = true;              // Compute shaders
    bool enableMeshShaders = false;         // Mesh shaders (if available)
    bool enableVariableRateShading = false; // VRS
    int maxGPUMemoryMB = 4096;             // Max GPU memory usage

    // ==================== Rendering Features ====================
    AAMode antiAliasing = AAMode::TAA;
    bool enableShadows = true;
    bool enableGI = true;                   // Global illumination
    bool enableSSAO = true;                 // Screen-space ambient occlusion
    bool enableSSR = true;                  // Screen-space reflections
    bool enableBloom = true;
    bool enableMotionBlur = false;
    bool enableDepthOfField = false;
    bool enableVolumetrics = false;
    int shadowMapResolution = 2048;
    int reflectionProbeCount = 8;

    // ==================== Hybrid Compute Settings ====================
    float gpuThreshold = 0.7f;              // GPU usage target (0-1)
    int cpuThreads = std::thread::hardware_concurrency();
    bool dynamicScheduling = true;          // Dynamic CPU/GPU balancing
    bool enableAsyncCompute = true;         // Async compute queues
    bool enableMultiGPU = false;            // Multi-GPU rendering

    // ==================== Performance Settings ====================
    bool enableOcclusionCulling = true;
    bool enableFrustumCulling = true;
    bool enableLOD = true;                  // Level of detail
    bool enableInstancing = true;
    bool enableBatching = true;
    int maxDrawCalls = 10000;
    int targetFPS = 60;

    // ==================== Debug Settings ====================
    bool enableDebugOverlay = false;
    bool enableWireframe = false;
    bool showBoundingBoxes = false;
    bool showNormals = false;
    bool enableGPUValidation = false;       // Vulkan validation layers
    bool profileGPU = false;
    bool verboseLogging = false;

    // ==================== Advanced Settings ====================
    bool enableHDR = true;
    float exposure = 1.0f;
    bool autoExposure = false;
    std::string toneMapping = "ACES";       // ACES, Reinhard, Filmic
    float gamma = 2.2f;

    // ==================== Scene Settings ====================
    Vec3 backgroundColor = Vec3(0.0f, 0.0f, 0.0f);
    bool transparentBackground = false;
    float ambientIntensity = 0.1f;

    // ==================== File Paths ====================
    std::filesystem::path configFile;
    std::filesystem::path sceneFile;
    std::filesystem::path assetDirectory = "./assets";
    std::filesystem::path shaderDirectory = "./shaders";
    std::filesystem::path cacheDirectory = "./cache";

    /**
     * @brief Load configuration from file
     */
    bool loadFromFile(const std::filesystem::path& path);

    /**
     * @brief Save configuration to file
     */
    bool saveToFile(const std::filesystem::path& path) const;

    /**
     * @brief Apply quality preset
     */
    void applyQualityPreset(QualityPreset preset);

    /**
     * @brief Validate configuration
     */
    bool validate() const;

    /**
     * @brief Get configuration as string
     */
    std::string toString() const;

    /**
     * @brief Merge with another config (other takes precedence)
     */
    void merge(const RenderConfig& other);
};

/**
 * @brief Global configuration instance
 */
RenderConfig& getGlobalConfig();

/**
 * @brief Configuration builder for fluent API
 */
class ConfigBuilder {
public:
    ConfigBuilder& setOutput(const std::string& file);
    ConfigBuilder& setResolution(int w, int h);
    ConfigBuilder& setFrameRate(int fps);
    ConfigBuilder& setQuality(QualityPreset preset);
    ConfigBuilder& setGPUBackend(GPUBackend backend);
    ConfigBuilder& enableFeature(const std::string& feature);
    ConfigBuilder& disableFeature(const std::string& feature);
    ConfigBuilder& setGPUThreshold(float threshold);
    ConfigBuilder& setCPUThreads(int threads);

    RenderConfig build() const;

private:
    RenderConfig config;
};

/**
 * @brief Parse command-line arguments into config
 */
RenderConfig parseCommandLine(int argc, char** argv);

/**
 * @brief Get backend name as string
 */
std::string getBackendName(GPUBackend backend);

/**
 * @brief Get quality preset name
 */
std::string getQualityName(QualityPreset preset);

} // namespace manim
