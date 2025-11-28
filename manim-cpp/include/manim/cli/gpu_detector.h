#pragma once

#include "manim/cli/config.h"
#include <vector>
#include <string>
#include <map>
#include <vulkan/vulkan.h>

namespace manim {

/**
 * @brief GPU device information
 */
struct GPUDeviceInfo {
    std::string name;
    std::string vendor;
    std::string driverVersion;
    size_t totalMemoryMB;
    size_t availableMemoryMB;
    int deviceID;
    int vendorID;
    bool discrete;              // Discrete vs integrated
    bool supportsRayTracing;
    bool supportsCompute;
    bool supportsMeshShaders;
    bool supportsVRS;
    int computeUnits;
    int maxWorkGroupSize;
    float performanceScore;     // Relative performance (0-100)
    // snake_case aliases for tests
    size_t total_memory_mb = 0;
    size_t available_memory_mb = 0;
    float performance_score = 0.0f;

    std::string toString() const;
};

/**
 * @brief Vulkan capabilities
 */
struct VulkanCapabilities {
    bool available;
    std::string version;
    bool validationLayersAvailable;
    bool rayTracingSupported;
    bool meshShadersSupported;
    bool computeShadersSupported;
    bool asyncComputeSupported;
    bool multiGPUSupported;
    std::vector<std::string> supportedExtensions;
    std::vector<std::string> supportedLayers;

    std::string toString() const;
};

/**
 * @brief OpenGL capabilities
 */
struct OpenGLCapabilities {
    bool available;
    std::string version;
    std::string shadingLanguageVersion;
    bool computeShadersSupported;
    bool tessellationSupported;
    bool geometryShadersSupported;
    int maxTextureSize;
    int maxTextureUnits;

    std::string toString() const;
};

/**
 * @brief DirectX 12 capabilities (Windows only)
 */
struct DirectX12Capabilities {
    bool available;
    std::string version;
    bool rayTracingSupported;
    bool meshShadersSupported;
    bool variableRateShadingSupported;
    int featureLevel;

    std::string toString() const;
};

/**
 * @brief CUDA capabilities
 */
struct CUDACapabilities {
    bool available;
    std::string version;
    int computeCapability;
    int deviceCount;
    std::vector<std::string> deviceNames;

    std::string toString() const;
};

/**
 * @brief System information
 */
struct SystemInfo {
    std::string os;
    std::string cpuModel;
    int cpuCores;
    int cpuThreads;
    size_t totalRAM_MB;
    size_t availableRAM_MB;
    bool sseSupported;
    bool avxSupported;
    bool avx512Supported;

    std::string toString() const;
};

/**
 * @brief Benchmark results
 */
struct BenchmarkResults {
    double sortTime;            // Time to sort 1M elements (ms)
    double matrixMultTime;      // Time for matrix multiply (ms)
    double bezierEvalTime;      // Time to evaluate 10K curves (ms)
    double renderTriangles;     // Triangles per second (millions)
    double bandwidth;           // GB/s
    float overallScore;         // Composite score (0-100)

    std::string toString() const;
};

/**
 * @brief Comprehensive GPU capability detector
 */
class GPUDetector {
public:
    GPUDetector();
    ~GPUDetector();

    // ==================== Detection ====================

    /**
     * @brief Detect all GPUs in the system
     */
    void detectGPUs();
    void detect_gpus() { detectGPUs(); }

    /**
     * @brief Check Vulkan support
     */
    void checkVulkanSupport();
    void check_vulkan_support() { checkVulkanSupport(); }

    /**
     * @brief Check OpenGL support
     */
    void checkOpenGLSupport();

    /**
     * @brief Check DirectX 12 support (Windows only)
     */
    void checkDirectX12Support();

    /**
     * @brief Check CUDA support
     */
    void checkCUDASupport();

    /**
     * @brief Check OpenCL support
     */
    void checkOpenCLSupport();

    /**
     * @brief Check ray tracing support
     */
    void checkRayTracingSupport();

    /**
     * @brief Check compute capability
     */
    void checkComputeCapability();

    /**
     * @brief Detect system information
     */
    void detectSystemInfo();

    // ==================== Benchmarking ====================

    /**
     * @brief Run GPU benchmark
     */
    void benchmarkGPU();

    /**
     * @brief Benchmark specific GPU
     */
    BenchmarkResults benchmarkDevice(int deviceIndex);
    void benchmark_gpu(int deviceIndex) { benchmarkDevice(deviceIndex); }

    /**
     * @brief Quick benchmark (< 1 second)
     */
    void quickBenchmark();

    /**
     * @brief Full benchmark (may take several seconds)
     */
    void fullBenchmark();

    // ==================== Recommendations ====================

    /**
     * @brief Recommend optimal settings based on hardware
     */
    RenderConfig recommendSettings() const;
    RenderConfig recommend_settings() const { return recommendSettings(); }

    /**
     * @brief Recommend GPU backend
     */
    GPUBackend recommendBackend() const;

    /**
     * @brief Recommend quality preset
     */
    QualityPreset recommendQuality() const;

    /**
     * @brief Check if hardware meets minimum requirements
     */
    bool meetsMinimumRequirements() const;

    /**
     * @brief Get recommended GPU device
     */
    int getRecommendedDevice() const;
    int get_recommended_device() const { return getRecommendedDevice(); }

    // ==================== Getters ====================

    const std::vector<GPUDeviceInfo>& getGPUs() const { return gpus; }
    const std::vector<GPUDeviceInfo>& get_gpus() const { return gpus; }
    const VulkanCapabilities& getVulkanCaps() const { return vulkanCaps; }
    const OpenGLCapabilities& getOpenGLCaps() const { return openglCaps; }
    const DirectX12Capabilities& getDX12Caps() const { return dx12Caps; }
    const CUDACapabilities& getCUDACaps() const { return cudaCaps; }
    const SystemInfo& getSystemInfo() const { return systemInfo; }
    const BenchmarkResults& getBenchmarkResults() const { return benchmarkResults; }

    /**
     * @brief Get best available GPU
     */
    const GPUDeviceInfo* getBestGPU() const;

    // ==================== Reporting ====================

    /**
     * @brief Print full system report
     */
    void printSystemReport() const;
    void print_system_report() const { printSystemReport(); }

    /**
     * @brief Print GPU information
     */
    void printGPUInfo() const;

    /**
     * @brief Print capabilities summary
     */
    void printCapabilities() const;

    /**
     * @brief Print benchmark results
     */
    void printBenchmarkResults() const;
    void print_benchmark_results() const { printBenchmarkResults(); }

    /**
     * @brief Print recommendations
     */
    void printRecommendations() const;

    /**
     * @brief Export report to file
     */
    bool exportReport(const std::string& filename) const;

    /**
     * @brief Get report as JSON
     */
    std::string toJSON() const;

private:
    // Detected hardware
    std::vector<GPUDeviceInfo> gpus;
    VulkanCapabilities vulkanCaps;
    OpenGLCapabilities openglCaps;
    DirectX12Capabilities dx12Caps;
    CUDACapabilities cudaCaps;
    SystemInfo systemInfo;
    BenchmarkResults benchmarkResults;

    // Vulkan handles
    VkInstance vkInstance;
    std::vector<VkPhysicalDevice> vkPhysicalDevices;

    // Helper methods
    void detectVulkanDevices();
    void queryDeviceProperties(VkPhysicalDevice device, GPUDeviceInfo& info);
    void queryDeviceFeatures(VkPhysicalDevice device, GPUDeviceInfo& info);
    void queryDeviceMemory(VkPhysicalDevice device, GPUDeviceInfo& info);
    float calculatePerformanceScore(const GPUDeviceInfo& info) const;
    void detectCPUFeatures();
    void detectMemory();

    // Benchmark helpers
    void benchmarkSort(BenchmarkResults& results);
    void benchmarkMatrixMultiply(BenchmarkResults& results);
    void benchmarkBezierEval(BenchmarkResults& results);
    void benchmarkTriangleRate(BenchmarkResults& results);
    void benchmarkBandwidth(BenchmarkResults& results);
};

/**
 * @brief Perform health check
 */
struct HealthCheckResult {
    bool passed;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    bool hasWarnings() const { return !warnings.empty(); }
    bool hasErrors() const { return !errors.empty(); }
};

/**
 * @brief Run comprehensive health check
 */
HealthCheckResult runHealthCheck();

/**
 * @brief Check specific component
 */
HealthCheckResult checkGPUHealth();
HealthCheckResult checkVulkanHealth();
HealthCheckResult checkDependenciesHealth();
HealthCheckResult checkSystemHealth();

GPUDetector& getGPUDetector();
HealthCheckResult runHealthCheck();

} // namespace manim
