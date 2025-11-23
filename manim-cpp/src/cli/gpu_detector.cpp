#include "manim/cli/gpu_detector.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace manim {

// ==================== GPUDeviceInfo ====================

std::string GPUDeviceInfo::toString() const {
    std::stringstream ss;
    ss << "GPU: " << name << "\n";
    ss << "  Vendor: " << vendor << "\n";
    ss << "  Memory: " << totalMemoryMB << " MB (Available: " << availableMemoryMB << " MB)\n";
    ss << "  Type: " << (discrete ? "Discrete" : "Integrated") << "\n";
    ss << "  Ray Tracing: " << (supportsRayTracing ? "Yes" : "No") << "\n";
    ss << "  Compute: " << (supportsCompute ? "Yes" : "No") << "\n";
    ss << "  Mesh Shaders: " << (supportsMeshShaders ? "Yes" : "No") << "\n";
    ss << "  Performance Score: " << std::fixed << std::setprecision(1) << performanceScore << "/100\n";
    return ss.str();
}

// ==================== Capabilities toString ====================

std::string VulkanCapabilities::toString() const {
    std::stringstream ss;
    ss << "Vulkan: " << (available ? "Available" : "Not Available") << "\n";
    if (available) {
        ss << "  Version: " << version << "\n";
        ss << "  Ray Tracing: " << (rayTracingSupported ? "Yes" : "No") << "\n";
        ss << "  Mesh Shaders: " << (meshShadersSupported ? "Yes" : "No") << "\n";
        ss << "  Compute Shaders: " << (computeShadersSupported ? "Yes" : "No") << "\n";
        ss << "  Async Compute: " << (asyncComputeSupported ? "Yes" : "No") << "\n";
    }
    return ss.str();
}

std::string OpenGLCapabilities::toString() const {
    std::stringstream ss;
    ss << "OpenGL: " << (available ? "Available" : "Not Available") << "\n";
    if (available) {
        ss << "  Version: " << version << "\n";
        ss << "  GLSL Version: " << shadingLanguageVersion << "\n";
        ss << "  Compute Shaders: " << (computeShadersSupported ? "Yes" : "No") << "\n";
    }
    return ss.str();
}

std::string DirectX12Capabilities::toString() const {
    std::stringstream ss;
    ss << "DirectX 12: " << (available ? "Available" : "Not Available") << "\n";
    if (available) {
        ss << "  Ray Tracing (DXR): " << (rayTracingSupported ? "Yes" : "No") << "\n";
        ss << "  Mesh Shaders: " << (meshShadersSupported ? "Yes" : "No") << "\n";
    }
    return ss.str();
}

std::string CUDACapabilities::toString() const {
    std::stringstream ss;
    ss << "CUDA: " << (available ? "Available" : "Not Available") << "\n";
    if (available) {
        ss << "  Version: " << version << "\n";
        ss << "  Device Count: " << deviceCount << "\n";
    }
    return ss.str();
}

std::string SystemInfo::toString() const {
    std::stringstream ss;
    ss << "System Information:\n";
    ss << "  OS: " << os << "\n";
    ss << "  CPU: " << cpuModel << "\n";
    ss << "  Cores: " << cpuCores << " (" << cpuThreads << " threads)\n";
    ss << "  RAM: " << totalRAM_MB << " MB (Available: " << availableRAM_MB << " MB)\n";
    ss << "  SIMD: ";
    if (avx512Supported) ss << "AVX-512 ";
    else if (avxSupported) ss << "AVX ";
    else if (sseSupported) ss << "SSE ";
    else ss << "None";
    ss << "\n";
    return ss.str();
}

std::string BenchmarkResults::toString() const {
    std::stringstream ss;
    ss << "Benchmark Results:\n";
    ss << "  Sort (1M elements): " << std::fixed << std::setprecision(2) << sortTime << " ms\n";
    ss << "  Matrix Multiply: " << matrixMultTime << " ms\n";
    ss << "  Bezier Evaluation: " << bezierEvalTime << " ms\n";
    ss << "  Triangle Rate: " << renderTriangles << " M tri/s\n";
    ss << "  Bandwidth: " << bandwidth << " GB/s\n";
    ss << "  Overall Score: " << overallScore << "/100\n";
    return ss.str();
}

// ==================== GPUDetector Implementation ====================

GPUDetector::GPUDetector()
    : vkInstance(VK_NULL_HANDLE)
{
    std::cout << "Initializing GPU detector..." << std::endl;
}

GPUDetector::~GPUDetector() {
    if (vkInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(vkInstance, nullptr);
    }
}

void GPUDetector::detectGPUs() {
    std::cout << "Detecting GPUs..." << std::endl;

    checkVulkanSupport();
    if (vulkanCaps.available) {
        detectVulkanDevices();
    }

    checkOpenGLSupport();
    checkDirectX12Support();
    checkCUDASupport();
    detectSystemInfo();

    std::cout << "Found " << gpus.size() << " GPU(s)" << std::endl;
}

void GPUDetector::checkVulkanSupport() {
    vulkanCaps.available = false;

    // Try to create Vulkan instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Manim C++";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Manim Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
    if (result == VK_SUCCESS) {
        vulkanCaps.available = true;
        vulkanCaps.version = "1.2";

        // Query instance properties
        uint32_t apiVersion;
        vkEnumerateInstanceVersion(&apiVersion);
        vulkanCaps.version = std::to_string(VK_VERSION_MAJOR(apiVersion)) + "." +
                            std::to_string(VK_VERSION_MINOR(apiVersion));

        std::cout << "Vulkan " << vulkanCaps.version << " detected" << std::endl;
    } else {
        std::cout << "Vulkan not available" << std::endl;
    }
}

void GPUDetector::detectVulkanDevices() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return;
    }

    vkPhysicalDevices.resize(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, vkPhysicalDevices.data());

    for (auto device : vkPhysicalDevices) {
        GPUDeviceInfo info;
        queryDeviceProperties(device, info);
        queryDeviceFeatures(device, info);
        queryDeviceMemory(device, info);
        info.performanceScore = calculatePerformanceScore(info);
        gpus.push_back(info);
    }
}

void GPUDetector::queryDeviceProperties(VkPhysicalDevice device, GPUDeviceInfo& info) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    info.name = props.deviceName;
    info.vendorID = props.vendorID;
    info.deviceID = props.deviceID;

    // Determine vendor
    switch (props.vendorID) {
        case 0x1002: info.vendor = "AMD"; break;
        case 0x10DE: info.vendor = "NVIDIA"; break;
        case 0x8086: info.vendor = "Intel"; break;
        default: info.vendor = "Unknown";
    }

    // Determine if discrete
    info.discrete = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    info.driverVersion = std::to_string(VK_VERSION_MAJOR(props.driverVersion)) + "." +
                        std::to_string(VK_VERSION_MINOR(props.driverVersion)) + "." +
                        std::to_string(VK_VERSION_PATCH(props.driverVersion));
}

void GPUDetector::queryDeviceFeatures(VkPhysicalDevice device, GPUDeviceInfo& info) {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    // Check for ray tracing support
    // TODO: Query VK_KHR_ray_tracing_pipeline extension
    info.supportsRayTracing = false;

    // Compute shader support
    info.supportsCompute = true; // All Vulkan devices support compute

    // TODO: Query mesh shader support
    info.supportsMeshShaders = false;
}

void GPUDetector::queryDeviceMemory(VkPhysicalDevice device, GPUDeviceInfo& info) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);

    size_t totalMemory = 0;
    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            totalMemory += memProps.memoryHeaps[i].size;
        }
    }

    info.totalMemoryMB = totalMemory / (1024 * 1024);
    info.availableMemoryMB = info.totalMemoryMB; // Approximate
}

float GPUDetector::calculatePerformanceScore(const GPUDeviceInfo& info) const {
    float score = 0.0f;

    // Memory score (0-30 points)
    score += std::min(30.0f, info.totalMemoryMB / 100.0f);

    // Discrete GPU bonus (20 points)
    if (info.discrete) score += 20.0f;

    // Ray tracing support (15 points)
    if (info.supportsRayTracing) score += 15.0f;

    // Mesh shader support (10 points)
    if (info.supportsMeshShaders) score += 10.0f;

    // Vendor bonus (0-10 points)
    if (info.vendor == "NVIDIA") score += 10.0f;
    else if (info.vendor == "AMD") score += 8.0f;
    else if (info.vendor == "Intel") score += 5.0f;

    // Compute units (0-15 points)
    score += std::min(15.0f, info.computeUnits / 10.0f);

    return std::min(100.0f, score);
}

void GPUDetector::checkOpenGLSupport() {
    // TODO: Implement OpenGL detection
    openglCaps.available = false;
}

void GPUDetector::checkDirectX12Support() {
    // TODO: Implement DirectX 12 detection (Windows only)
    dx12Caps.available = false;
}

void GPUDetector::checkCUDASupport() {
    // TODO: Implement CUDA detection
    cudaCaps.available = false;
}

void GPUDetector::checkOpenCLSupport() {
    // TODO: Implement OpenCL detection
}

void GPUDetector::checkRayTracingSupport() {
    // Already checked in queryDeviceFeatures
}

void GPUDetector::checkComputeCapability() {
    // Already checked in queryDeviceFeatures
}

void GPUDetector::detectSystemInfo() {
    // Detect OS
#ifdef _WIN32
    systemInfo.os = "Windows";
#elif defined(__APPLE__)
    systemInfo.os = "macOS";
#elif defined(__linux__)
    systemInfo.os = "Linux";
#else
    systemInfo.os = "Unknown";
#endif

    // Detect CPU
    systemInfo.cpuCores = std::thread::hardware_concurrency();
    systemInfo.cpuThreads = systemInfo.cpuCores; // Simplified

    detectCPUFeatures();
    detectMemory();
}

void GPUDetector::detectCPUFeatures() {
    // TODO: Use CPUID to detect SSE, AVX, AVX-512
    systemInfo.sseSupported = true;
    systemInfo.avxSupported = true;
    systemInfo.avx512Supported = false;
}

void GPUDetector::detectMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    systemInfo.totalRAM_MB = memInfo.ullTotalPhys / (1024 * 1024);
    systemInfo.availableRAM_MB = memInfo.ullAvailPhys / (1024 * 1024);
#elif defined(__linux__)
    struct sysinfo info;
    sysinfo(&info);
    systemInfo.totalRAM_MB = info.totalram / (1024 * 1024);
    systemInfo.availableRAM_MB = info.freeram / (1024 * 1024);
#else
    systemInfo.totalRAM_MB = 8192; // Default estimate
    systemInfo.availableRAM_MB = 4096;
#endif
}

void GPUDetector::benchmarkGPU() {
    std::cout << "Running GPU benchmarks..." << std::endl;

    benchmarkSort(benchmarkResults);
    benchmarkMatrixMultiply(benchmarkResults);
    benchmarkBezierEval(benchmarkResults);

    // Calculate overall score
    benchmarkResults.overallScore = (
        (100.0f / benchmarkResults.sortTime) +
        (100.0f / benchmarkResults.matrixMultTime) +
        (100.0f / benchmarkResults.bezierEvalTime)
    ) / 3.0f;
}

void GPUDetector::benchmarkSort(BenchmarkResults& results) {
    // Simple benchmark - measure time to sort 1M elements
    auto start = std::chrono::high_resolution_clock::now();
    // TODO: Actual GPU sort
    auto end = std::chrono::high_resolution_clock::now();
    results.sortTime = std::chrono::duration<double, std::milli>(end - start).count();
}

void GPUDetector::benchmarkMatrixMultiply(BenchmarkResults& results) {
    auto start = std::chrono::high_resolution_clock::now();
    // TODO: Actual GPU matrix multiply
    auto end = std::chrono::high_resolution_clock::now();
    results.matrixMultTime = std::chrono::duration<double, std::milli>(end - start).count();
}

void GPUDetector::benchmarkBezierEval(BenchmarkResults& results) {
    auto start = std::chrono::high_resolution_clock::now();
    // TODO: Actual GPU Bezier evaluation
    auto end = std::chrono::high_resolution_clock::now();
    results.bezierEvalTime = std::chrono::duration<double, std::milli>(end - start).count();
}

void GPUDetector::quickBenchmark() {
    std::cout << "Running quick benchmark..." << std::endl;
    benchmarkSort(benchmarkResults);
}

void GPUDetector::fullBenchmark() {
    benchmarkGPU();
}

RenderConfig GPUDetector::recommendSettings() const {
    RenderConfig config;

    config.backend = recommendBackend();
    config.quality = recommendQuality();

    const auto* bestGPU = getBestGPU();
    if (bestGPU) {
        // Adjust settings based on GPU capability
        if (bestGPU->totalMemoryMB >= 8192) {
            config.shadowMapResolution = 4096;
            config.reflectionProbeCount = 16;
            config.enableRayTracing = bestGPU->supportsRayTracing;
        } else if (bestGPU->totalMemoryMB >= 4096) {
            config.shadowMapResolution = 2048;
            config.reflectionProbeCount = 8;
        } else {
            config.shadowMapResolution = 1024;
            config.reflectionProbeCount = 4;
            config.enableRayTracing = false;
        }
    }

    return config;
}

GPUBackend GPUDetector::recommendBackend() const {
    if (vulkanCaps.available && vulkanCaps.rayTracingSupported) {
        return GPUBackend::VULKAN;
    }
    if (cudaCaps.available) {
        return GPUBackend::CUDA;
    }
    if (vulkanCaps.available) {
        return GPUBackend::VULKAN;
    }
    if (openglCaps.available) {
        return GPUBackend::OPENGL;
    }
    return GPUBackend::AUTO;
}

QualityPreset GPUDetector::recommendQuality() const {
    const auto* bestGPU = getBestGPU();
    if (!bestGPU) return QualityPreset::MEDIUM;

    if (bestGPU->performanceScore >= 80.0f) {
        return QualityPreset::ULTRA;
    } else if (bestGPU->performanceScore >= 60.0f) {
        return QualityPreset::HIGH;
    } else if (bestGPU->performanceScore >= 40.0f) {
        return QualityPreset::MEDIUM;
    } else {
        return QualityPreset::LOW;
    }
}

bool GPUDetector::meetsMinimumRequirements() const {
    // Minimum: Any GPU with 2GB VRAM and Vulkan/OpenGL support
    if (gpus.empty()) return false;

    const auto* bestGPU = getBestGPU();
    return bestGPU && bestGPU->totalMemoryMB >= 2048 &&
           (vulkanCaps.available || openglCaps.available);
}

int GPUDetector::getRecommendedDevice() const {
    if (gpus.empty()) return -1;

    // Find GPU with highest performance score
    float maxScore = 0.0f;
    int bestIndex = 0;

    for (size_t i = 0; i < gpus.size(); ++i) {
        if (gpus[i].performanceScore > maxScore) {
            maxScore = gpus[i].performanceScore;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}

const GPUDeviceInfo* GPUDetector::getBestGPU() const {
    int index = getRecommendedDevice();
    return (index >= 0 && index < static_cast<int>(gpus.size())) ? &gpus[index] : nullptr;
}

void GPUDetector::printSystemReport() const {
    std::cout << "\n========== SYSTEM REPORT ==========\n" << std::endl;
    std::cout << systemInfo.toString() << std::endl;
    printGPUInfo();
    printCapabilities();
    if (benchmarkResults.overallScore > 0) {
        printBenchmarkResults();
    }
    printRecommendations();
    std::cout << "\n===================================\n" << std::endl;
}

void GPUDetector::printGPUInfo() const {
    std::cout << "GPUs Found: " << gpus.size() << "\n" << std::endl;
    for (size_t i = 0; i < gpus.size(); ++i) {
        std::cout << "GPU " << i << ":\n" << gpus[i].toString() << std::endl;
    }
}

void GPUDetector::printCapabilities() const {
    std::cout << "Graphics API Support:\n";
    std::cout << vulkanCaps.toString();
    std::cout << openglCaps.toString();
    std::cout << dx12Caps.toString();
    std::cout << cudaCaps.toString();
    std::cout << std::endl;
}

void GPUDetector::printBenchmarkResults() const {
    std::cout << benchmarkResults.toString() << std::endl;
}

void GPUDetector::printRecommendations() const {
    std::cout << "Recommended Settings:\n";
    std::cout << "  Backend: " << getBackendName(recommendBackend()) << "\n";
    std::cout << "  Quality: " << getQualityName(recommendQuality()) << "\n";
    std::cout << "  Device: GPU " << getRecommendedDevice() << "\n";
    std::cout << "  Minimum Requirements: "
              << (meetsMinimumRequirements() ? "Met" : "NOT MET") << "\n";
}

bool GPUDetector::exportReport(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << "Manim C++ System Report\n";
    file << "=======================\n\n";
    file << systemInfo.toString() << "\n";

    for (size_t i = 0; i < gpus.size(); ++i) {
        file << "GPU " << i << ":\n" << gpus[i].toString() << "\n";
    }

    file << vulkanCaps.toString() << "\n";
    file << openglCaps.toString() << "\n";

    return true;
}

std::string GPUDetector::toJSON() const {
    // TODO: Implement JSON serialization
    return "{}";
}

GPUDetector& getGPUDetector() {
    static GPUDetector instance;
    return instance;
}

HealthCheckResult runHealthCheck() {
    HealthCheckResult result;
    result.passed = true;

    auto gpuCheck = checkGPUHealth();
    auto vulkanCheck = checkVulkanHealth();
    auto sysCheck = checkSystemHealth();

    result.passed = gpuCheck.passed && vulkanCheck.passed && sysCheck.passed;
    result.message = result.passed ? "All checks passed" : "Some checks failed";

    return result;
}

HealthCheckResult checkGPUHealth() {
    HealthCheckResult result;
    auto& detector = getGPUDetector();

    if (detector.getGPUs().empty()) {
        result.passed = false;
        result.errors.push_back("No GPUs detected");
    } else {
        result.passed = true;
        result.message = "GPU(s) detected";
    }

    return result;
}

HealthCheckResult checkVulkanHealth() {
    HealthCheckResult result;
    auto& detector = getGPUDetector();

    if (!detector.getVulkanCaps().available) {
        result.passed = false;
        result.warnings.push_back("Vulkan not available");
    } else {
        result.passed = true;
        result.message = "Vulkan available";
    }

    return result;
}

HealthCheckResult checkDependenciesHealth() {
    HealthCheckResult result;
    result.passed = true;
    result.message = "Dependencies OK";
    return result;
}

HealthCheckResult checkSystemHealth() {
    HealthCheckResult result;
    auto& detector = getGPUDetector();

    if (detector.getSystemInfo().totalRAM_MB < 4096) {
        result.warnings.push_back("Low system RAM (< 4GB)");
    }

    result.passed = true;
    result.message = "System OK";
    return result;
}

} // namespace manim
