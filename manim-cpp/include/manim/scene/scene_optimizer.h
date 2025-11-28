#pragma once

#include "manim/core/types.h"
#include "manim/scene/scene.h"
#include "manim/renderer/renderer.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace manim {

// Forward declarations
class Scene;
class Mobject;
class Material;

/**
 * @brief Batch of similar objects for efficient rendering
 */
struct RenderBatch {
    std::vector<std::shared_ptr<Mobject>> objects;
    std::shared_ptr<Material> material;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory vertexMemory;
    VkDeviceMemory indexMemory;
    size_t vertexCount;
    size_t indexCount;
    bool isDirty;

    RenderBatch()
        : vertexBuffer(VK_NULL_HANDLE)
        , indexBuffer(VK_NULL_HANDLE)
        , vertexMemory(VK_NULL_HANDLE)
        , indexMemory(VK_NULL_HANDLE)
        , vertexCount(0)
        , indexCount(0)
        , isDirty(true)
    {}
};

/**
 * @brief Instance data for instanced rendering
 */
struct InstanceData {
    Mat4 transform;
    Vec4 color;
    Vec4 customData;  // Application-specific data
};

/**
 * @brief Instanced render group
 */
struct InstancedGroup {
    std::shared_ptr<Mobject> prototype;  // Base mesh
    std::vector<InstanceData> instances;
    VkBuffer instanceBuffer;
    VkDeviceMemory instanceMemory;
    bool isDirty;

    InstancedGroup()
        : instanceBuffer(VK_NULL_HANDLE)
        , instanceMemory(VK_NULL_HANDLE)
        , isDirty(true)
    {}
};

/**
 * @brief Draw call information
 */
struct DrawCall {
    VkPipeline pipeline;
    VkDescriptorSet descriptorSet;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t vertexOffset;
    uint32_t firstInstance;

    DrawCall()
        : pipeline(VK_NULL_HANDLE)
        , descriptorSet(VK_NULL_HANDLE)
        , vertexBuffer(VK_NULL_HANDLE)
        , indexBuffer(VK_NULL_HANDLE)
        , indexCount(0)
        , instanceCount(1)
        , firstIndex(0)
        , vertexOffset(0)
        , firstInstance(0)
    {}
};

/**
 * @brief Render state for state change minimization
 */
struct RenderState {
    VkPipeline pipeline;
    VkDescriptorSet descriptorSet;
    std::shared_ptr<Material> material;
    bool depthTest;
    bool depthWrite;
    bool blending;

    bool operator==(const RenderState& other) const {
        return pipeline == other.pipeline &&
               descriptorSet == other.descriptorSet &&
               material == other.material &&
               depthTest == other.depthTest &&
               depthWrite == other.depthWrite &&
               blending == other.blending;
    }
};

/**
 * @brief Scene Optimizer for automatic batching, instancing, and draw call optimization
 */
class SceneOptimizer {
public:
    SceneOptimizer();
    ~SceneOptimizer();

    // Disable copy, enable move
    SceneOptimizer(const SceneOptimizer&) = delete;
    SceneOptimizer& operator=(const SceneOptimizer&) = delete;
    SceneOptimizer(SceneOptimizer&&) = default;
    SceneOptimizer& operator=(SceneOptimizer&&) = default;

    // ==================== Automatic Batching ====================

    /**
     * @brief Analyze scene and create batches
     * @param scene Scene to analyze
     */
    void analyzeBatching(const Scene& scene);

    /**
     * @brief Create batches from mobjects
     * @param mobjects Mobjects to batch
     * @return Vector of render batches
     */
    std::vector<RenderBatch> createBatches(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Merge geometries into a single batch
     * @param batch Batch to merge
     */
    void mergeGeometry(RenderBatch& batch);

    /**
     * @brief Update batch buffers
     * @param batch Batch to update
     */
    void updateBatchBuffers(RenderBatch& batch);

    /**
     * @brief Get all render batches
     */
    const std::vector<RenderBatch>& getBatches() const { return batches; }

    // ==================== Instance Rendering ====================

    /**
     * @brief Detect instancing opportunities
     * @param mobjects Mobjects to analyze
     * @return Vector of instanced groups
     */
    std::vector<InstancedGroup> detectInstancing(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Create instance group
     * @param prototype Base mobject
     * @param instances Instance data
     * @return Instanced group
     */
    InstancedGroup createInstanceGroup(std::shared_ptr<Mobject> prototype,
                                       const std::vector<InstanceData>& instances);

    /**
     * @brief Update instance buffer
     * @param group Instance group to update
     */
    void updateInstanceBuffer(InstancedGroup& group);

    /**
     * @brief Get all instanced groups
     */
    const std::vector<InstancedGroup>& getInstancedGroups() const { return instancedGroups; }

    // ==================== Draw Call Optimization ====================

    /**
     * @brief Optimize draw calls for scene
     * @param scene Scene to optimize
     * @return Optimized draw calls
     */
    std::vector<DrawCall> optimizeDrawCalls(const Scene& scene);

    /**
     * @brief Sort draw calls to minimize state changes
     * @param drawCalls Draw calls to sort
     */
    void sortDrawCalls(std::vector<DrawCall>& drawCalls);

    /**
     * @brief Merge compatible draw calls
     * @param drawCalls Draw calls to merge
     * @return Merged draw calls
     */
    std::vector<DrawCall> mergeDrawCalls(const std::vector<DrawCall>& drawCalls);

    /**
     * @brief Get optimized draw calls
     */
    const std::vector<DrawCall>& getDrawCalls() const { return drawCalls; }

    // ==================== State Change Minimization ====================

    /**
     * @brief Build render state graph
     * @param scene Scene to analyze
     */
    void buildStateGraph(const Scene& scene);

    /**
     * @brief Find optimal render order
     * @return Ordered mobject indices
     */
    std::vector<size_t> findOptimalRenderOrder();

    /**
     * @brief Calculate state change cost
     * @param from Source state
     * @param to Target state
     * @return Cost value
     */
    float calculateStateChangeCost(const RenderState& from, const RenderState& to);

    /**
     * @brief Get current render state
     */
    const RenderState& getCurrentState() const { return currentState; }

    // ==================== Statistics ====================

    struct OptimizationStats {
        size_t originalDrawCalls;
        size_t optimizedDrawCalls;
        size_t batchCount;
        size_t instancedGroupCount;
        size_t totalInstances;
        size_t stateChanges;
        float optimizationRatio;  // optimized / original

        OptimizationStats()
            : originalDrawCalls(0)
            , optimizedDrawCalls(0)
            , batchCount(0)
            , instancedGroupCount(0)
            , totalInstances(0)
            , stateChanges(0)
            , optimizationRatio(0.0f)
        {}
    };

    /**
     * @brief Get optimization statistics
     */
    const OptimizationStats& getStats() const { return stats; }

    /**
     * @brief Print optimization report
     */
    void printOptimizationReport() const;

    // ==================== Configuration ====================

    /**
     * @brief Enable/disable automatic batching
     */
    void setAutoBatching(bool enable) { autoBatching = enable; }

    /**
     * @brief Enable/disable automatic instancing
     */
    void setAutoInstancing(bool enable) { autoInstancing = enable; }

    /**
     * @brief Set batching threshold (minimum objects to batch)
     */
    void setBatchingThreshold(size_t threshold) { batchingThreshold = threshold; }

    /**
     * @brief Set instancing threshold (minimum instances to use instancing)
     */
    void setInstancingThreshold(size_t threshold) { instancingThreshold = threshold; }

    /**
     * @brief Clear all optimizations
     */
    void clear();

private:
    // Optimization data
    std::vector<RenderBatch> batches;
    std::vector<InstancedGroup> instancedGroups;
    std::vector<DrawCall> drawCalls;
    RenderState currentState;

    // Configuration
    bool autoBatching;
    bool autoInstancing;
    size_t batchingThreshold;
    size_t instancingThreshold;

    // Statistics
    OptimizationStats stats;

    // Helper methods
    bool canBatch(const Mobject& obj1, const Mobject& obj2) const;
    bool canInstance(const Mobject& obj1, const Mobject& obj2) const;
    size_t hashRenderState(const RenderState& state) const;
    void updateStatistics();
};

/**
 * @brief Optimized scene with automatic optimization
 */
class OptimizedScene : public Scene {
public:
    OptimizedScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~OptimizedScene() = default;

    /**
     * @brief Override render to apply optimizations
     */
    bool render(bool preview = false);

    /**
     * @brief Get scene optimizer
     */
    std::shared_ptr<SceneOptimizer> getOptimizer() const {
        return optimizer;
    }

    /**
     * @brief Enable/disable auto optimization
     */
    void setAutoOptimize(bool enable) { autoOptimize = enable; }

    /**
     * @brief Manually trigger optimization
     */
    void optimize();

protected:
    std::shared_ptr<SceneOptimizer> optimizer;
    bool autoOptimize;
};

} // namespace manim
