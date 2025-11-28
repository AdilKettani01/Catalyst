#pragma once

#include "manim/core/memory_pool.hpp"
#include "manim/core/types.h"
#include "manim/utils/rate_functions.h"
#include <vector>
#include <string>

namespace manim {

enum class ColorSpace { RGB, HSV };

namespace GPUUtils {

enum class SpatialQueryType {
    Radius,
    AABB
};

void parallelSort(GPUBuffer& buffer);
void parallelReduce(GPUBuffer& buffer, const std::string& op = "sum");
void parallelScan(GPUBuffer& buffer);

void batchMatrixMultiply(GPUBuffer& matrices);
void batchBezierEval(GPUBuffer& curves, float t);
void batchInterpolate(GPUBuffer& from, GPUBuffer& to, float alpha);
void buildBVH(GPUBuffer& objects);
std::vector<int> spatialQuery(const std::vector<Vec3>& points,
                              const std::vector<Vec3>& sizes,
                              SpatialQueryType queryType = SpatialQueryType::Radius,
                              float radius = 1.0f,
                              bool use_gpu = true);

GPUBuffer createGPUBuffer(size_t size, VkBufferUsageFlags usage = 0);
void destroyGPUBuffer(GPUBuffer& buffer);

void uploadToGPU(GPUBuffer& buffer, const void* data, size_t size);
void downloadFromGPU(const GPUBuffer& buffer, void* data, size_t size);

void imageBlur(GPUBuffer& image, int width, int height, float sigma);
void imageConvolve(GPUBuffer& image, int width, int height,
                   const std::vector<float>& kernel, int kernelSize);
void imageResize(GPUBuffer& input, GPUBuffer& output,
                 int inputWidth, int inputHeight,
                 int outputWidth, int outputHeight);
void batchColorConvert(GPUBuffer& colors,
                       const std::string& fromSpace,
                       const std::string& toSpace);
std::vector<Vec4> batchColorConvert(const std::vector<Vec4>& colors,
                                    const std::string& fromSpace,
                                    const std::string& toSpace,
                                    bool use_gpu = true);

bool isGPUAvailable();
inline bool is_gpu_available() { return isGPUAvailable(); }
inline void batch_color_convert(GPUBuffer& buffer, ColorSpace /*from*/, ColorSpace /*to*/) {
    batchColorConvert(buffer, "RGB", "HSV");
}
inline void parallel_sort(GPUBuffer& buffer) { parallelSort(buffer); }
inline void batch_matrix_multiply(GPUBuffer& buffer) { batchMatrixMultiply(buffer); }

}  // namespace GPUUtils
}  // namespace manim
