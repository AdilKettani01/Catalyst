#include "manim/utils/hybrid_compute.hpp"
#include <spdlog/spdlog.h>

namespace manim {

void HybridCompute::detect_hardware() {
    gpuAvailable = GPUUtils::is_gpu_available();
    cpuAvailable = true;
}

bool HybridCompute::should_use_gpu(size_t dataSize, OperationType op) const {
    if (!GPUUtils::is_gpu_available()) {
        return false;
    }

    switch (op) {
        case OperationType::MATRIX_MULTIPLY:
            return dataSize >= 128;
        case OperationType::SORT:
            return dataSize > 10000;
        default:
            return dataSize > 2000;
    }
}

void HybridCompute::executeHybrid(ComputeTask& task) {
    auto backend = selectBackend(task);
    if (backend == ExecutionBackend::GPU && task.gpuFunc) {
        (*task.gpuFunc)();
    } else if (task.cpuFunc) {
        (*task.cpuFunc)();
    }
    updatePerformanceHistory(task.type, backend, 0.0);
}

ExecutionBackend HybridCompute::selectBackend(const ComputeTask& task) const {
    return should_use_gpu(task.dataSize, task.type) ? ExecutionBackend::GPU : ExecutionBackend::CPU;
}

void HybridCompute::updatePerformanceHistory(OperationType op, ExecutionBackend backend, double time) {
    performanceHistory[op][backend] = time;
}

double HybridCompute::getAverageTime(OperationType op, ExecutionBackend backend) const {
    auto it = performanceHistory.find(op);
    if (it != performanceHistory.end()) {
        auto jt = it->second.find(backend);
        if (jt != it->second.end()) {
            return jt->second;
        }
    }
    return 0.0;
}

}  // namespace manim
