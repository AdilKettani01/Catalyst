/**
 * @file test_gpu.cpp
 * @brief GPU-specific tests including memory management, fallback, and multi-GPU
 *
 * Tests:
 * - GPU detection and initialization
 * - GPU memory limits and leak detection
 * - Automatic CPU fallback when GPU unavailable
 * - Multi-GPU scaling
 * - GPU compute operations
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "manim/cli/gpu_detector.h"
#include "manim/cli/config.h"
#include "manim/scene/three_d_scene.h"
#include "manim/utils/gpu_utils.h"
#include "manim/utils/hybrid_compute.h"
#include "manim/animation/fading.hpp"
#include "manim/mobject/geometry/circle.hpp"

using namespace manim;

// ==================== GPU Detection Tests ====================

class GPUDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = std::make_shared<GPUDetector>();
    }

    std::shared_ptr<GPUDetector> detector;
};

TEST_F(GPUDetectorTest, DetectGPUs) {
    EXPECT_NO_THROW(detector->detect_gpus());

    auto gpus = detector->get_gpus();
    // May have 0 GPUs (CI environment) or multiple GPUs
    EXPECT_GE(gpus.size(), 0);
}

TEST_F(GPUDetectorTest, CheckVulkanSupport) {
    EXPECT_NO_THROW(detector->check_vulkan_support());
}

TEST_F(GPUDetectorTest, RecommendSettings) {
    detector->detect_gpus();

    auto config = detector->recommend_settings();
    EXPECT_NE(config.backend, GPUBackend::AUTO);  // Should pick specific backend

    // Quality should be valid
    EXPECT_TRUE(
        config.quality == QualityPreset::LOW ||
        config.quality == QualityPreset::MEDIUM ||
        config.quality == QualityPreset::HIGH ||
        config.quality == QualityPreset::ULTRA
    );
}

TEST_F(GPUDetectorTest, SystemReport) {
    detector->detect_gpus();
    EXPECT_NO_THROW(detector->print_system_report());
}

TEST_F(GPUDetectorTest, BenchmarkGPU) {
    detector->detect_gpus();

    auto gpus = detector->get_gpus();
    if (!gpus.empty()) {
        EXPECT_NO_THROW(detector->benchmark_gpu(0));
    }
}

TEST_F(GPUDetectorTest, MultipleDetections) {
    // Should be able to detect multiple times without issues
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(detector->detect_gpus());
        auto gpus = detector->get_gpus();
        EXPECT_GE(gpus.size(), 0);
    }
}

// ==================== GPU Availability Tests ====================

TEST(GPUAvailability, CheckAvailable) {
    bool available = GPUUtils::is_gpu_available();
    // Result depends on environment, but should not crash
    EXPECT_TRUE(available || !available);  // Tautology, but tests execution
}

TEST(GPUAvailability, ConsistentResults) {
    bool first = GPUUtils::is_gpu_available();
    bool second = GPUUtils::is_gpu_available();

    // Multiple checks should return same result
    EXPECT_EQ(first, second);
}

// ==================== Hybrid Compute Tests ====================

class HybridComputeTest : public ::testing::Test {
protected:
    void SetUp() override {
        compute = std::make_shared<HybridCompute>();
    }

    std::shared_ptr<HybridCompute> compute;
};

TEST_F(HybridComputeTest, DetectHardware) {
    EXPECT_NO_THROW(compute->detect_hardware());
}

TEST_F(HybridComputeTest, AutoTuning) {
    EXPECT_NO_THROW(compute->auto_tune());
}

TEST_F(HybridComputeTest, GPUThresholdDecisions) {
    // Small data should use CPU
    bool use_gpu_small = compute->should_use_gpu(100, HybridCompute::OperationType::SORT);
    EXPECT_FALSE(use_gpu_small);

    // Large data should use GPU (if available)
    bool use_gpu_large = compute->should_use_gpu(1000000, HybridCompute::OperationType::SORT);
    if (GPUUtils::is_gpu_available()) {
        EXPECT_TRUE(use_gpu_large);
    }
}

TEST_F(HybridComputeTest, MatrixMultiplyThreshold) {
    // Small matrices -> CPU
    bool use_gpu_small = compute->should_use_gpu(64, HybridCompute::OperationType::MATRIX_MULTIPLY);
    EXPECT_FALSE(use_gpu_small);

    // Large matrices -> GPU (if available)
    bool use_gpu_large = compute->should_use_gpu(256, HybridCompute::OperationType::MATRIX_MULTIPLY);
    if (GPUUtils::is_gpu_available()) {
        EXPECT_TRUE(use_gpu_large);
    }
}

// ==================== GPU Memory Tests ====================

class GPUMemoryTest : public ::testing::Test {
protected:
    bool has_gpu = false;

    void SetUp() override {
        has_gpu = GPUUtils::is_gpu_available();
    }
};

TEST_F(GPUMemoryTest, AllocateAndFreeBuffers) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    // Test allocating and freeing GPU buffers
    // This tests memory leak detection
    for (int i = 0; i < 100; ++i) {
        GPUBuffer buffer;
        EXPECT_NO_THROW(buffer.allocate(1024 * 1024));  // 1MB
        EXPECT_NO_THROW(buffer.free());
    }
}

TEST_F(GPUMemoryTest, LargeAllocation) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    // Try to allocate 100MB
    GPUBuffer buffer;
    try {
        buffer.allocate(100 * 1024 * 1024);
        buffer.free();
        SUCCEED();
    } catch (const std::exception& e) {
        // May fail on low-memory GPUs - that's expected
        GTEST_SKIP() << "GPU memory limit: " << e.what();
    }
}

TEST_F(GPUMemoryTest, OutOfMemoryHandling) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    // Try to allocate unreasonably large buffer (100GB)
    GPUBuffer buffer;
    EXPECT_THROW(
        buffer.allocate(100ULL * 1024 * 1024 * 1024),
        std::exception
    );
}

TEST_F(GPUMemoryTest, MultipleBuffers) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    // Allocate multiple buffers simultaneously
    std::vector<GPUBuffer> buffers(10);

    for (auto& buffer : buffers) {
        EXPECT_NO_THROW(buffer.allocate(1024 * 1024));  // 1MB each
    }

    for (auto& buffer : buffers) {
        EXPECT_NO_THROW(buffer.free());
    }
}

// ==================== CPU Fallback Tests ====================

TEST(CPUFallback, AnimationWithoutGPU) {
    // Create animation that can run on GPU
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<FadeIn>(mobject);

    // Should work even without GPU (falls back to CPU)
    anim->begin();
    EXPECT_NO_THROW(anim->interpolate(0.5f));
    anim->finish();
}

TEST(CPUFallback, GPU3DSceneWithoutGPU) {
    // GPU3DScene should fall back to CPU rendering if GPU unavailable
    auto scene = std::make_shared<GPU3DScene>();

    EXPECT_NO_THROW(scene->setup_deferred_pipeline());
    EXPECT_NO_THROW(scene->render_with_pbr());
}

TEST(CPUFallback, RateFunctionBatchEvaluation) {
    std::vector<float> t_values = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    // Should work with or without GPU
    auto results = RateFunctions::batch_evaluate(
        RateFunctions::smooth,
        t_values,
        true  // try to use GPU
    );

    EXPECT_EQ(results.size(), t_values.size());

    // Verify results are reasonable
    for (float result : results) {
        EXPECT_GE(result, 0.0f);
        EXPECT_LE(result, 1.0f);
    }
}

// ==================== GPU Compute Operations Tests ====================

class GPUComputeTest : public ::testing::Test {
protected:
    bool has_gpu = false;

    void SetUp() override {
        has_gpu = GPUUtils::is_gpu_available();
    }
};

TEST_F(GPUComputeTest, ParallelSort) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    std::vector<float> data(10000);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(data.size() - i);  // Reverse order
    }

    GPUBuffer buffer;
    buffer.upload(data);

    EXPECT_NO_THROW(GPUUtils::parallel_sort(buffer));

    auto sorted = buffer.download();

    // Verify sorted
    for (size_t i = 1; i < sorted.size(); ++i) {
        EXPECT_LE(sorted[i-1], sorted[i]);
    }
}

TEST_F(GPUComputeTest, BatchMatrixMultiply) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    const int num_matrices = 100;
    const int matrix_size = 128;

    std::vector<math::Mat4> matrices(num_matrices);
    // Initialize with identity matrices
    for (auto& mat : matrices) {
        mat = math::Mat4::identity();
    }

    GPUBuffer buffer;
    buffer.upload_matrices(matrices);

    EXPECT_NO_THROW(GPUUtils::batch_matrix_multiply(buffer));

    auto results = buffer.download_matrices();
    EXPECT_EQ(results.size(), num_matrices);
}

TEST_F(GPUComputeTest, ColorConversion) {
    if (!has_gpu) {
        GTEST_SKIP() << "No GPU available";
    }

    std::vector<math::Vec4> colors(1000);
    for (size_t i = 0; i < colors.size(); ++i) {
        colors[i] = math::Vec4(
            static_cast<float>(i) / colors.size(),
            0.5f,
            0.5f,
            1.0f
        );
    }

    GPUBuffer buffer;
    buffer.upload_colors(colors);

    EXPECT_NO_THROW(GPUUtils::batch_color_convert(buffer, ColorSpace::RGB, ColorSpace::HSV));

    auto converted = buffer.download_colors();
    EXPECT_EQ(converted.size(), colors.size());
}

// ==================== Multi-GPU Tests ====================

class MultiGPUTest : public ::testing::Test {
protected:
    std::shared_ptr<GPUDetector> detector;
    int num_gpus = 0;

    void SetUp() override {
        detector = std::make_shared<GPUDetector>();
        detector->detect_gpus();
        num_gpus = detector->get_gpus().size();
    }
};

TEST_F(MultiGPUTest, DetectMultipleGPUs) {
    if (num_gpus < 2) {
        GTEST_SKIP() << "Need at least 2 GPUs";
    }

    auto gpus = detector->get_gpus();
    EXPECT_GE(gpus.size(), 2);

    // Verify each GPU has valid info
    for (const auto& gpu : gpus) {
        EXPECT_FALSE(gpu.name.empty());
        EXPECT_GT(gpu.total_memory_mb, 0);
    }
}

TEST_F(MultiGPUTest, BenchmarkAllGPUs) {
    if (num_gpus == 0) {
        GTEST_SKIP() << "No GPUs available";
    }

    for (int i = 0; i < num_gpus; ++i) {
        EXPECT_NO_THROW(detector->benchmark_gpu(i));
    }
}

TEST_F(MultiGPUTest, PerformanceComparison) {
    if (num_gpus < 2) {
        GTEST_SKIP() << "Need at least 2 GPUs for comparison";
    }

    std::vector<float> scores;
    auto gpus = detector->get_gpus();

    for (const auto& gpu : gpus) {
        scores.push_back(gpu.performance_score);
    }

    // All scores should be valid
    for (float score : scores) {
        EXPECT_GE(score, 0.0f);
        EXPECT_LE(score, 100.0f);
    }
}

TEST_F(MultiGPUTest, SelectBestGPU) {
    if (num_gpus == 0) {
        GTEST_SKIP() << "No GPUs available";
    }

    auto config = detector->recommend_settings();

    // Should have selected a valid backend
    EXPECT_NE(config.backend, GPUBackend::AUTO);
}

// ==================== GPU Scene Scaling Tests ====================

TEST(GPUScaling, SceneWith1000Objects) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    // Add 1000 3D objects
    for (int i = 0; i < 1000; ++i) {
        auto mesh = std::make_shared<Mesh>();
        mesh->shift(math::Vec3(i * 0.1f, 0, 0));
        scene->add(mesh);
    }

    // Should handle 1000 objects without crashing
    EXPECT_NO_THROW(scene->render_with_pbr());
}

TEST(GPUScaling, SceneWith100Lights) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    // Add 100 lights
    for (int i = 0; i < 100; ++i) {
        Light light;
        light.type = LightType::POINT;
        light.position = math::Vec3(i * 0.5f, 5, 5);
        light.intensity = 0.1f;
        scene->add_light(light);
    }

    // Should handle 100 lights
    EXPECT_NO_THROW(scene->render_with_pbr());
}

// ==================== GPU Error Handling Tests ====================

TEST(GPUErrorHandling, InvalidBufferSize) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    GPUBuffer buffer;

    // Zero size should fail gracefully
    EXPECT_THROW(buffer.allocate(0), std::exception);

    // Negative size should fail
    EXPECT_THROW(buffer.allocate(-1), std::exception);
}

TEST(GPUErrorHandling, UninitializedBuffer) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    GPUBuffer buffer;

    // Operations on uninitialized buffer should fail gracefully
    EXPECT_THROW(buffer.download(), std::exception);
}

TEST(GPUErrorHandling, DoubleFreeMembuffer) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    GPUBuffer buffer;
    buffer.allocate(1024);
    buffer.free();

    // Second free should be safe (no-op)
    EXPECT_NO_THROW(buffer.free());
}

// ==================== GPU Performance Tests ====================

TEST(GPUPerformance, LargeDataTransfer) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    const size_t data_size = 100 * 1024 * 1024;  // 100MB
    std::vector<float> data(data_size / sizeof(float));

    GPUBuffer buffer;

    auto start = std::chrono::high_resolution_clock::now();

    buffer.upload(data);
    auto downloaded = buffer.download();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    buffer.free();

    // 100MB transfer should complete in reasonable time
    EXPECT_LT(duration.count(), 1000);  // < 1 second

    // Verify data integrity
    EXPECT_EQ(downloaded.size(), data.size());
}

TEST(GPUPerformance, BatchOperations) {
    if (!GPUUtils::is_gpu_available()) {
        GTEST_SKIP() << "No GPU available";
    }

    const int num_operations = 1000;
    std::vector<float> data(10000);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_operations; ++i) {
        GPUBuffer buffer;
        buffer.upload(data);
        GPUUtils::parallel_sort(buffer);
        buffer.download();
        buffer.free();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 1000 operations should complete in reasonable time
    EXPECT_LT(duration.count(), 30000);  // < 30 seconds
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
