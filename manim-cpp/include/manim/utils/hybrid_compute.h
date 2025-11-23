#pragma once

#include "manim/utils/gpu_utils.h"
#include <functional>
#include <string>
#include <memory>
#include <chrono>

namespace manim {

/**
 * @brief Task descriptor for hybrid compute
 */
struct ComputeTask {
    OperationType type;
    size_t dataSize;
    void* inputData;
    void* outputData;
    std::function<void()> cpuFunction;
    std::function<void()> gpuFunction;

    ComputeTask()
        : type(OperationType::GENERIC_COMPUTE)
        , dataSize(0)
        , inputData(nullptr)
        , outputData(nullptr)
    {}
};

/**
 * @brief Execution backend
 */
enum class ExecutionBackend {
    AUTO,        // Automatic selection
    CPU_SCALAR,  // Scalar CPU
    CPU_SSE,     // SSE vectorized
    CPU_AVX,     // AVX vectorized
    CPU_AVX512,  // AVX-512 vectorized
    GPU_VULKAN,  // Vulkan compute
    GPU_CUDA,    // CUDA
    GPU_OPENCL   // OpenCL
};

/**
 * @brief Performance statistics for hybrid compute
 */
struct ComputeStats {
    size_t totalTasks;
    size_t cpuTasks;
    size_t gpuTasks;
    double totalCPUTime;
    double totalGPUTime;
    double avgCPUTime;
    double avgGPUTime;

    ComputeStats()
        : totalTasks(0)
        , cpuTasks(0)
        , gpuTasks(0)
        , totalCPUTime(0.0)
        , totalGPUTime(0.0)
        , avgCPUTime(0.0)
        , avgGPUTime(0.0)
    {}
};

/**
 * @brief Hybrid Compute System for intelligent CPU/GPU selection
 *
 * This system automatically chooses the best execution backend based on:
 * - Data size
 * - Operation type
 * - Hardware availability
 * - Historical performance
 */
class HybridCompute {
public:
    HybridCompute();
    ~HybridCompute();

    // Disable copy, enable move
    HybridCompute(const HybridCompute&) = delete;
    HybridCompute& operator=(const HybridCompute&) = delete;
    HybridCompute(HybridCompute&&) = default;
    HybridCompute& operator=(HybridCompute&&) = default;

    // ==================== Automatic Selection ====================

    /**
     * @brief Determine if GPU should be used for given task
     * @param dataSize Size of data in bytes
     * @param op Operation type
     * @return True if GPU should be used
     */
    bool shouldUseGPU(size_t dataSize, OperationType op) const;

    /**
     * @brief Execute task with automatic backend selection
     * @param task Task to execute
     */
    void executeHybrid(ComputeTask& task);

    /**
     * @brief Select optimal backend for task
     * @param task Task descriptor
     * @return Selected backend
     */
    ExecutionBackend selectBackend(const ComputeTask& task) const;

    // ==================== CPU Execution ====================

    /**
     * @brief Execute on CPU (scalar)
     * @param task Task to execute
     */
    void executeCPU(ComputeTask& task);

    /**
     * @brief Execute on CPU with SSE vectorization
     * @param task Task to execute
     */
    void executeSSE(ComputeTask& task);

    /**
     * @brief Execute on CPU with AVX vectorization
     * @param task Task to execute
     */
    void executeAVX(ComputeTask& task);

    /**
     * @brief Execute on CPU with AVX-512 vectorization
     * @param task Task to execute
     */
    void executeAVX512(ComputeTask& task);

    // ==================== GPU Execution ====================

    /**
     * @brief Execute on GPU (Vulkan compute)
     * @param task Task to execute
     */
    void executeGPU(ComputeTask& task);

    /**
     * @brief Execute on GPU using CUDA
     * @param task Task to execute
     */
    void executeCUDA(ComputeTask& task);

    /**
     * @brief Execute on GPU using OpenCL
     * @param task Task to execute
     */
    void executeOpenCL(ComputeTask& task);

    // ==================== Backend Capabilities ====================

    /**
     * @brief Check if SSE is available
     */
    bool hasSSE() const { return sseAvailable; }

    /**
     * @brief Check if AVX is available
     */
    bool hasAVX() const { return avxAvailable; }

    /**
     * @brief Check if AVX-512 is available
     */
    bool hasAVX512() const { return avx512Available; }

    /**
     * @brief Check if Vulkan compute is available
     */
    bool hasVulkan() const { return vulkanAvailable; }

    /**
     * @brief Check if CUDA is available
     */
    bool hasCUDA() const { return cudaAvailable; }

    /**
     * @brief Check if OpenCL is available
     */
    bool hasOpenCL() const { return openclAvailable; }

    // ==================== Configuration ====================

    /**
     * @brief Set GPU threshold (minimum data size for GPU)
     * @param bytes Minimum bytes for GPU execution
     */
    void setGPUThreshold(size_t bytes);

    /**
     * @brief Set preferred backend
     * @param backend Preferred execution backend
     */
    void setPreferredBackend(ExecutionBackend backend);

    /**
     * @brief Enable/disable auto-tuning
     * @param enable Enable auto-tuning
     */
    void setAutoTuning(bool enable);

    /**
     * @brief Force specific backend
     * @param backend Backend to force (or AUTO for automatic)
     */
    void forceBackend(ExecutionBackend backend);

    // ==================== Performance Tracking ====================

    /**
     * @brief Get performance statistics
     */
    const ComputeStats& getStats() const { return stats; }

    /**
     * @brief Reset performance statistics
     */
    void resetStats();

    /**
     * @brief Print performance report
     */
    void printPerformanceReport() const;

    /**
     * @brief Get estimated execution time
     * @param dataSize Data size in bytes
     * @param op Operation type
     * @param backend Execution backend
     * @return Estimated time in seconds
     */
    double estimateExecutionTime(size_t dataSize, OperationType op,
                                  ExecutionBackend backend) const;

    // ==================== Benchmarking ====================

    /**
     * @brief Benchmark operation on different backends
     * @param dataSize Data size to benchmark
     * @param op Operation type
     * @return Map of backend to execution time
     */
    std::map<ExecutionBackend, double> benchmark(size_t dataSize,
                                                  OperationType op);

    /**
     * @brief Auto-tune thresholds based on hardware
     */
    void autoTune();

private:
    // Backend availability
    bool sseAvailable;
    bool avxAvailable;
    bool avx512Available;
    bool vulkanAvailable;
    bool cudaAvailable;
    bool openclAvailable;

    // Configuration
    size_t gpuThreshold;              // Minimum data size for GPU
    ExecutionBackend preferredBackend;
    ExecutionBackend forcedBackend;
    bool autoTuningEnabled;

    // Performance tracking
    ComputeStats stats;
    std::map<OperationType, std::map<ExecutionBackend, double>> performanceHistory;

    // Helper methods
    void detectCapabilities();
    void initializeBackends();
    void shutdownBackends();
    void updatePerformanceHistory(OperationType op, ExecutionBackend backend, double time);
    double getAverageTime(OperationType op, ExecutionBackend backend) const;
};

/**
 * @brief Global hybrid compute instance
 */
HybridCompute& getHybridCompute();

/**
 * @brief Helper functions for common operations
 */
namespace HybridOps {

/**
 * @brief Sort array with hybrid compute
 */
template<typename T>
void sort(std::vector<T>& data) {
    auto& hybrid = getHybridCompute();
    ComputeTask task;
    task.type = OperationType::SORT;
    task.dataSize = data.size() * sizeof(T);
    task.inputData = data.data();
    task.outputData = data.data();
    task.cpuFunction = [&data]() { std::sort(data.begin(), data.end()); };
    hybrid.executeHybrid(task);
}

/**
 * @brief Reduce array with hybrid compute
 */
template<typename T>
T reduce(const std::vector<T>& data, std::function<T(T, T)> op, T init = T()) {
    auto& hybrid = getHybridCompute();

    if (hybrid.shouldUseGPU(data.size() * sizeof(T), OperationType::REDUCE)) {
        // GPU path
        return GPUUtils::parallelReduce(data, op, init, true);
    } else {
        // CPU path
        return std::accumulate(data.begin(), data.end(), init, op);
    }
}

/**
 * @brief Scan array with hybrid compute
 */
template<typename T>
std::vector<T> scan(const std::vector<T>& data, std::function<T(T, T)> op, T init = T()) {
    auto& hybrid = getHybridCompute();

    if (hybrid.shouldUseGPU(data.size() * sizeof(T), OperationType::SCAN)) {
        return GPUUtils::parallelScan(data, op, init, true);
    } else {
        return GPUUtils::parallelScan(data, op, init, false);
    }
}

} // namespace HybridOps

} // namespace manim
