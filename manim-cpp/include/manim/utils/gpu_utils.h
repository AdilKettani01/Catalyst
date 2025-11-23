#pragma once

#include "manim/core/types.h"
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

namespace manim {

/**
 * @brief GPU buffer wrapper for compute operations
 */
struct GPUBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    size_t size;
    void* mappedData;

    GPUBuffer()
        : buffer(VK_NULL_HANDLE)
        , memory(VK_NULL_HANDLE)
        , size(0)
        , mappedData(nullptr)
    {}
};

/**
 * @brief Operation types for hybrid compute
 */
enum class OperationType {
    SORT,
    REDUCE,
    SCAN,
    MATRIX_MULTIPLY,
    BEZIER_EVAL,
    INTERPOLATE,
    SPATIAL_QUERY,
    IMAGE_PROCESS,
    COLOR_CONVERT,
    GENERIC_COMPUTE
};

/**
 * @brief GPU utility functions namespace
 */
namespace GPUUtils {

// ==================== Parallel Algorithms ====================

/**
 * @brief Parallel sort on GPU
 * @param buffer GPU buffer containing data to sort
 */
void parallelSort(GPUBuffer& buffer);

/**
 * @brief Parallel sort with CPU fallback
 */
template<typename T>
void parallelSort(std::vector<T>& data, bool useGPU = true);

/**
 * @brief Parallel reduce operation
 * @param buffer GPU buffer containing data
 * @param operation Reduction operation (sum, min, max, etc.)
 */
void parallelReduce(GPUBuffer& buffer, const std::string& operation = "sum");

/**
 * @brief Parallel reduce with CPU fallback
 */
template<typename T>
T parallelReduce(const std::vector<T>& data,
                 std::function<T(T, T)> op,
                 T initialValue = T(),
                 bool useGPU = true);

/**
 * @brief Parallel scan (prefix sum)
 * @param buffer GPU buffer containing data
 */
void parallelScan(GPUBuffer& buffer);

/**
 * @brief Parallel scan with CPU fallback
 */
template<typename T>
std::vector<T> parallelScan(const std::vector<T>& data,
                            std::function<T(T, T)> op,
                            T initialValue = T(),
                            bool useGPU = true);

// ==================== Math Operations ====================

/**
 * @brief Batch matrix multiplication on GPU
 * @param matrices GPU buffer containing matrices
 */
void batchMatrixMultiply(GPUBuffer& matrices);

/**
 * @brief Batch matrix multiply with CPU fallback
 */
std::vector<Mat4> batchMatrixMultiply(const std::vector<Mat4>& matrices1,
                                      const std::vector<Mat4>& matrices2,
                                      bool useGPU = true);

/**
 * @brief Batch Bezier curve evaluation on GPU
 * @param curves GPU buffer containing curve control points
 * @param t Parameter value
 */
void batchBezierEval(GPUBuffer& curves, float t);

/**
 * @brief Batch Bezier evaluation with CPU fallback
 */
std::vector<Vec3> batchBezierEval(const std::vector<std::vector<Vec3>>& curves,
                                  const std::vector<float>& t_values,
                                  bool useGPU = true);

/**
 * @brief Batch interpolation on GPU
 * @param from Starting values
 * @param to Target values
 * @param alpha Interpolation factor
 */
void batchInterpolate(GPUBuffer& from, GPUBuffer& to, float alpha);

/**
 * @brief Batch interpolation with CPU fallback
 */
template<typename T>
std::vector<T> batchInterpolate(const std::vector<T>& from,
                                const std::vector<T>& to,
                                float alpha,
                                bool useGPU = true);

// ==================== Spatial Operations ====================

/**
 * @brief Build Bounding Volume Hierarchy on GPU
 * @param objects GPU buffer containing objects
 */
void buildBVH(GPUBuffer& objects);

/**
 * @brief BVH node structure
 */
struct BVHNode {
    Vec3 min_bound;
    Vec3 max_bound;
    int leftChild;   // -1 if leaf
    int rightChild;  // -1 if leaf
    int objectIndex; // -1 if internal node
};

/**
 * @brief Build BVH with CPU fallback
 */
std::vector<BVHNode> buildBVH(const std::vector<Vec3>& positions,
                              const std::vector<Vec3>& sizes,
                              bool useGPU = true);

/**
 * @brief Spatial query on GPU
 * @param queries GPU buffer containing query data
 * @param results GPU buffer for results
 */
void spatialQuery(GPUBuffer& queries, GPUBuffer& results);

/**
 * @brief Spatial query types
 */
enum class SpatialQueryType {
    NEAREST_NEIGHBOR,
    RADIUS_SEARCH,
    BOX_QUERY,
    RAY_INTERSECTION
};

/**
 * @brief Perform spatial query with CPU fallback
 */
std::vector<int> spatialQuery(const std::vector<Vec3>& queryPoints,
                              const std::vector<Vec3>& dataPoints,
                              SpatialQueryType queryType,
                              float radius = 1.0f,
                              bool useGPU = true);

// ==================== Image Processing ====================

/**
 * @brief GPU image blur
 */
void imageBlur(GPUBuffer& image, int width, int height, float sigma);

/**
 * @brief GPU image convolution
 */
void imageConvolve(GPUBuffer& image, int width, int height,
                   const std::vector<float>& kernel, int kernelSize);

/**
 * @brief GPU image resize
 */
void imageResize(GPUBuffer& input, GPUBuffer& output,
                 int srcWidth, int srcHeight,
                 int dstWidth, int dstHeight);

// ==================== Color Operations ====================

/**
 * @brief Batch color space conversion on GPU
 */
void batchColorConvert(GPUBuffer& colors,
                      const std::string& fromSpace,
                      const std::string& toSpace);

/**
 * @brief Batch color conversion with CPU fallback
 */
std::vector<Vec4> batchColorConvert(const std::vector<Vec4>& colors,
                                    const std::string& fromSpace,
                                    const std::string& toSpace,
                                    bool useGPU = true);

// ==================== Utility Functions ====================

/**
 * @brief Create GPU buffer
 */
GPUBuffer createGPUBuffer(size_t size, VkBufferUsageFlags usage);

/**
 * @brief Destroy GPU buffer
 */
void destroyGPUBuffer(GPUBuffer& buffer);

/**
 * @brief Upload data to GPU buffer
 */
void uploadToGPU(GPUBuffer& buffer, const void* data, size_t size);

/**
 * @brief Download data from GPU buffer
 */
void downloadFromGPU(const GPUBuffer& buffer, void* data, size_t size);

/**
 * @brief Check if GPU is available
 */
bool isGPUAvailable();

/**
 * @brief Get GPU memory info
 */
struct GPUMemoryInfo {
    size_t totalMemory;
    size_t availableMemory;
    size_t usedMemory;
};
GPUMemoryInfo getGPUMemoryInfo();

/**
 * @brief Get optimal workgroup size for compute shader
 */
Vec3i getOptimalWorkGroupSize(size_t dataSize);

} // namespace GPUUtils

// ==================== Template Implementations ====================

template<typename T>
void GPUUtils::parallelSort(std::vector<T>& data, bool useGPU) {
    if (!useGPU || !isGPUAvailable() || data.size() < 1000) {
        // CPU fallback
        std::sort(data.begin(), data.end());
        return;
    }

    // GPU implementation
    GPUBuffer buffer = createGPUBuffer(data.size() * sizeof(T),
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    uploadToGPU(buffer, data.data(), data.size() * sizeof(T));
    parallelSort(buffer);
    downloadFromGPU(buffer, data.data(), data.size() * sizeof(T));
    destroyGPUBuffer(buffer);
}

template<typename T>
T GPUUtils::parallelReduce(const std::vector<T>& data,
                           std::function<T(T, T)> op,
                           T initialValue,
                           bool useGPU) {
    if (!useGPU || !isGPUAvailable() || data.size() < 1000) {
        // CPU fallback
        return std::accumulate(data.begin(), data.end(), initialValue, op);
    }

    // GPU implementation
    // TODO: Implement GPU reduce
    return std::accumulate(data.begin(), data.end(), initialValue, op);
}

template<typename T>
std::vector<T> GPUUtils::parallelScan(const std::vector<T>& data,
                                      std::function<T(T, T)> op,
                                      T initialValue,
                                      bool useGPU) {
    std::vector<T> result(data.size());

    if (!useGPU || !isGPUAvailable() || data.size() < 1000) {
        // CPU fallback - sequential scan
        T accumulator = initialValue;
        for (size_t i = 0; i < data.size(); ++i) {
            accumulator = op(accumulator, data[i]);
            result[i] = accumulator;
        }
        return result;
    }

    // GPU implementation
    // TODO: Implement GPU scan
    T accumulator = initialValue;
    for (size_t i = 0; i < data.size(); ++i) {
        accumulator = op(accumulator, data[i]);
        result[i] = accumulator;
    }

    return result;
}

template<typename T>
std::vector<T> GPUUtils::batchInterpolate(const std::vector<T>& from,
                                          const std::vector<T>& to,
                                          float alpha,
                                          bool useGPU) {
    std::vector<T> result(from.size());

    if (!useGPU || !isGPUAvailable() || from.size() < 100) {
        // CPU fallback
        for (size_t i = 0; i < from.size(); ++i) {
            result[i] = from[i] * (1.0f - alpha) + to[i] * alpha;
        }
        return result;
    }

    // GPU implementation
    // TODO: Implement GPU interpolation
    for (size_t i = 0; i < from.size(); ++i) {
        result[i] = from[i] * (1.0f - alpha) + to[i] * alpha;
    }

    return result;
}

} // namespace manim
