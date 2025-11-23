#include "manim/utils/hybrid_compute.h"
#include <iostream>
#include <immintrin.h> // For SIMD intrinsics
#include <cpuid.h>     // For CPU capability detection

namespace manim {

HybridCompute::HybridCompute()
    : sseAvailable(false)
    , avxAvailable(false)
    , avx512Available(false)
    , vulkanAvailable(false)
    , cudaAvailable(false)
    , openclAvailable(false)
    , gpuThreshold(10000) // 10KB default
    , preferredBackend(ExecutionBackend::AUTO)
    , forcedBackend(ExecutionBackend::AUTO)
    , autoTuningEnabled(true)
{
    std::cout << "Initializing Hybrid Compute System..." << std::endl;
    detectCapabilities();
    initializeBackends();

    if (autoTuningEnabled) {
        autoTune();
    }
}

HybridCompute::~HybridCompute() {
    shutdownBackends();
}

// ==================== Automatic Selection ====================

bool HybridCompute::shouldUseGPU(size_t dataSize, OperationType op) const {
    if (forcedBackend != ExecutionBackend::AUTO) {
        return forcedBackend >= ExecutionBackend::GPU_VULKAN;
    }

    // Check GPU availability
    if (!vulkanAvailable && !cudaAvailable && !openclAvailable) {
        return false;
    }

    // Size threshold check
    if (dataSize < gpuThreshold) {
        return false;
    }

    // Operation-specific thresholds
    switch (op) {
        case OperationType::SORT:
            return dataSize > 100000; // 100K elements

        case OperationType::MATRIX_MULTIPLY:
            return dataSize > 16384; // ~128x128 matrices

        case OperationType::BEZIER_EVAL:
            return dataSize > 10000; // 10K curves

        case OperationType::IMAGE_PROCESS:
            return dataSize > 262144; // 512x512 pixels

        case OperationType::REDUCE:
        case OperationType::SCAN:
            return dataSize > 50000;

        default:
            return dataSize > gpuThreshold;
    }
}

void HybridCompute::executeHybrid(ComputeTask& task) {
    auto start = std::chrono::high_resolution_clock::now();

    ExecutionBackend backend = (forcedBackend != ExecutionBackend::AUTO)
        ? forcedBackend
        : selectBackend(task);

    // Execute on selected backend
    switch (backend) {
        case ExecutionBackend::CPU_SCALAR:
            executeCPU(task);
            break;

        case ExecutionBackend::CPU_SSE:
            executeSSE(task);
            break;

        case ExecutionBackend::CPU_AVX:
            executeAVX(task);
            break;

        case ExecutionBackend::CPU_AVX512:
            executeAVX512(task);
            break;

        case ExecutionBackend::GPU_VULKAN:
            executeGPU(task);
            break;

        case ExecutionBackend::GPU_CUDA:
            executeCUDA(task);
            break;

        case ExecutionBackend::GPU_OPENCL:
            executeOpenCL(task);
            break;

        default:
            executeCPU(task);
            break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double>(end - start).count();

    // Update statistics
    stats.totalTasks++;
    if (backend >= ExecutionBackend::GPU_VULKAN) {
        stats.gpuTasks++;
        stats.totalGPUTime += duration;
        stats.avgGPUTime = stats.totalGPUTime / stats.gpuTasks;
    } else {
        stats.cpuTasks++;
        stats.totalCPUTime += duration;
        stats.avgCPUTime = stats.totalCPUTime / stats.cpuTasks;
    }

    updatePerformanceHistory(task.type, backend, duration);
}

ExecutionBackend HybridCompute::selectBackend(const ComputeTask& task) const {
    if (shouldUseGPU(task.dataSize, task.type)) {
        // Prefer CUDA if available, then Vulkan, then OpenCL
        if (cudaAvailable) return ExecutionBackend::GPU_CUDA;
        if (vulkanAvailable) return ExecutionBackend::GPU_VULKAN;
        if (openclAvailable) return ExecutionBackend::GPU_OPENCL;
    }

    // CPU backend selection
    if (avx512Available && task.dataSize > 1000000) {
        return ExecutionBackend::CPU_AVX512;
    }
    if (avxAvailable && task.dataSize > 100000) {
        return ExecutionBackend::CPU_AVX;
    }
    if (sseAvailable && task.dataSize > 10000) {
        return ExecutionBackend::CPU_SSE;
    }

    return ExecutionBackend::CPU_SCALAR;
}

// ==================== CPU Execution ====================

void HybridCompute::executeCPU(ComputeTask& task) {
    if (task.cpuFunction) {
        task.cpuFunction();
    }
}

void HybridCompute::executeSSE(ComputeTask& task) {
    if (!sseAvailable) {
        executeCPU(task);
        return;
    }

    // TODO: Implement SSE-optimized execution
    std::cout << "Executing with SSE" << std::endl;
    executeCPU(task);
}

void HybridCompute::executeAVX(ComputeTask& task) {
    if (!avxAvailable) {
        executeSSE(task);
        return;
    }

    // TODO: Implement AVX-optimized execution
    std::cout << "Executing with AVX" << std::endl;
    executeCPU(task);
}

void HybridCompute::executeAVX512(ComputeTask& task) {
    if (!avx512Available) {
        executeAVX(task);
        return;
    }

    // TODO: Implement AVX-512-optimized execution
    std::cout << "Executing with AVX-512" << std::endl;
    executeCPU(task);
}

// ==================== GPU Execution ====================

void HybridCompute::executeGPU(ComputeTask& task) {
    if (!vulkanAvailable) {
        executeCPU(task);
        return;
    }

    if (task.gpuFunction) {
        task.gpuFunction();
    } else {
        std::cout << "GPU execution via Vulkan compute" << std::endl;
        // TODO: Implement Vulkan compute execution
        executeCPU(task);
    }
}

void HybridCompute::executeCUDA(ComputeTask& task) {
    if (!cudaAvailable) {
        executeGPU(task);
        return;
    }

    std::cout << "Executing with CUDA" << std::endl;
    // TODO: Implement CUDA execution
    executeCPU(task);
}

void HybridCompute::executeOpenCL(ComputeTask& task) {
    if (!openclAvailable) {
        executeGPU(task);
        return;
    }

    std::cout << "Executing with OpenCL" << std::endl;
    // TODO: Implement OpenCL execution
    executeCPU(task);
}

// ==================== Configuration ====================

void HybridCompute::setGPUThreshold(size_t bytes) {
    gpuThreshold = bytes;
    std::cout << "GPU threshold set to " << bytes << " bytes" << std::endl;
}

void HybridCompute::setPreferredBackend(ExecutionBackend backend) {
    preferredBackend = backend;
}

void HybridCompute::setAutoTuning(bool enable) {
    autoTuningEnabled = enable;
}

void HybridCompute::forceBackend(ExecutionBackend backend) {
    forcedBackend = backend;
}

// ==================== Performance Tracking ====================

void HybridCompute::resetStats() {
    stats = ComputeStats();
}

void HybridCompute::printPerformanceReport() const {
    std::cout << "\n=== Hybrid Compute Performance Report ===" << std::endl;
    std::cout << "Total tasks: " << stats.totalTasks << std::endl;
    std::cout << "CPU tasks: " << stats.cpuTasks
              << " (avg time: " << stats.avgCPUTime << "s)" << std::endl;
    std::cout << "GPU tasks: " << stats.gpuTasks
              << " (avg time: " << stats.avgGPUTime << "s)" << std::endl;

    if (stats.avgGPUTime > 0) {
        double speedup = stats.avgCPUTime / stats.avgGPUTime;
        std::cout << "GPU speedup: " << speedup << "x" << std::endl;
    }

    std::cout << "========================================\n" << std::endl;
}

double HybridCompute::estimateExecutionTime(size_t dataSize, OperationType op,
                                            ExecutionBackend backend) const {
    double avgTime = getAverageTime(op, backend);
    if (avgTime > 0) {
        return avgTime;
    }

    // Rough estimates if no history
    if (backend >= ExecutionBackend::GPU_VULKAN) {
        return dataSize * 1e-9; // ~1ns per element on GPU
    } else {
        return dataSize * 1e-8; // ~10ns per element on CPU
    }
}

// ==================== Benchmarking ====================

std::map<ExecutionBackend, double> HybridCompute::benchmark(size_t dataSize,
                                                             OperationType op) {
    std::map<ExecutionBackend, double> results;

    std::cout << "Benchmarking operation with data size " << dataSize << std::endl;

    // TODO: Implement comprehensive benchmarking
    // Run each available backend and measure performance

    return results;
}

void HybridCompute::autoTune() {
    std::cout << "Auto-tuning hybrid compute system..." << std::endl;

    // Benchmark different operation types at various data sizes
    std::vector<size_t> testSizes = {1000, 10000, 100000, 1000000};
    std::vector<OperationType> testOps = {
        OperationType::SORT,
        OperationType::REDUCE,
        OperationType::MATRIX_MULTIPLY
    };

    for (auto op : testOps) {
        for (auto size : testSizes) {
            benchmark(size, op);
        }
    }

    std::cout << "Auto-tuning complete" << std::endl;
}

// ==================== Private Helper Methods ====================

void HybridCompute::detectCapabilities() {
    std::cout << "Detecting CPU capabilities..." << std::endl;

#if defined(__x86_64__) || defined(_M_X64)
    unsigned int eax, ebx, ecx, edx;

    // Check for SSE
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        sseAvailable = (edx & (1 << 25)) != 0; // SSE
        avxAvailable = (ecx & (1 << 28)) != 0; // AVX

        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
            avx512Available = (ebx & (1 << 16)) != 0; // AVX-512F
        }
    }
#endif

    std::cout << "  SSE: " << (sseAvailable ? "Yes" : "No") << std::endl;
    std::cout << "  AVX: " << (avxAvailable ? "Yes" : "No") << std::endl;
    std::cout << "  AVX-512: " << (avx512Available ? "Yes" : "No") << std::endl;

    // Check GPU capabilities
    vulkanAvailable = GPUUtils::isGPUAvailable();
    // TODO: Check CUDA and OpenCL availability

    std::cout << "  Vulkan: " << (vulkanAvailable ? "Yes" : "No") << std::endl;
    std::cout << "  CUDA: " << (cudaAvailable ? "Yes" : "No") << std::endl;
    std::cout << "  OpenCL: " << (openclAvailable ? "Yes" : "No") << std::endl;
}

void HybridCompute::initializeBackends() {
    // TODO: Initialize GPU backends
}

void HybridCompute::shutdownBackends() {
    // TODO: Shutdown GPU backends
}

void HybridCompute::updatePerformanceHistory(OperationType op,
                                             ExecutionBackend backend,
                                             double time) {
    performanceHistory[op][backend] = time;
}

double HybridCompute::getAverageTime(OperationType op, ExecutionBackend backend) const {
    auto opIt = performanceHistory.find(op);
    if (opIt != performanceHistory.end()) {
        auto backendIt = opIt->second.find(backend);
        if (backendIt != opIt->second.end()) {
            return backendIt->second;
        }
    }
    return 0.0;
}

// ==================== Global Instance ====================

HybridCompute& getHybridCompute() {
    static HybridCompute instance;
    return instance;
}

} // namespace manim
