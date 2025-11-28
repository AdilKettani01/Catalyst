#pragma once

#include <vector>
#include <functional>
#include <optional>
#include <map>
#include "manim/core/types.h"
#include "manim/utils/gpu_utils.h"

namespace manim {

enum class OperationType {
    GENERIC_COMPUTE,
    SORT,
    REDUCE,
    SCAN,
    MATRIX_MULTIPLY,
    BEZIER_EVAL,
    IMAGE_PROCESS
};

enum class ExecutionBackend {
    CPU,
    GPU
};

struct ComputeTask {
    size_t dataSize = 0;
    OperationType type = OperationType::GENERIC_COMPUTE;
    std::optional<std::function<void()>> cpuFunc;
    std::optional<std::function<void()>> gpuFunc;
};

class HybridCompute {
public:
    using OperationType = manim::OperationType;

    HybridCompute() = default;
    ~HybridCompute() = default;

    bool shouldUseGPU(size_t dataSize, OperationType op) const { return should_use_gpu(dataSize, op); }
    bool should_use_gpu(size_t dataSize, OperationType op) const;
    void detect_hardware();

    void executeHybrid(ComputeTask& task);
    ExecutionBackend selectBackend(const ComputeTask& task) const;

    double estimateExecutionTime(size_t /*dataSize*/, OperationType /*op*/,
                                 ExecutionBackend /*backend*/) const { return 0.0; }
    void autoTune() {}
    void auto_tune() { autoTune(); }

private:
    bool gpuAvailable = true;
    bool cpuAvailable = true;
    std::map<OperationType, std::map<ExecutionBackend, double>> performanceHistory;

    void updatePerformanceHistory(OperationType op, ExecutionBackend backend, double time);
    double getAverageTime(OperationType op, ExecutionBackend backend) const;
};

namespace HybridOps {
    template<typename T>
    void sort(std::vector<T>& data) {
        ComputeTask task;
        task.dataSize = data.size() * sizeof(T);
        HybridCompute hybrid;
        hybrid.executeHybrid(task);
    }

    template<typename T>
    T reduce(const std::vector<T>& data, std::function<T(T, T)> op = std::plus<T>(), T init = T{}) {
        ComputeTask task;
        task.dataSize = data.size() * sizeof(T);
        HybridCompute hybrid;
        hybrid.executeHybrid(task);
        return init;
    }

    template<typename T>
    std::vector<T> scan(const std::vector<T>& data, std::function<T(T, T)> op = std::plus<T>(), T init = T{}) {
        ComputeTask task;
        task.dataSize = data.size() * sizeof(T);
        HybridCompute hybrid;
        hybrid.executeHybrid(task);
        return data;
    }
}

}  // namespace manim
