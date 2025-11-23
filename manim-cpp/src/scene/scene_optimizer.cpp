#include "manim/scene/scene_optimizer.h"
#include "manim/scene/scene.h"
#include "manim/mobject/mobject.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace manim {

SceneOptimizer::SceneOptimizer()
    : autoBatching(true)
    , autoInstancing(true)
    , batchingThreshold(10)
    , instancingThreshold(5)
{
    std::cout << "Scene optimizer initialized" << std::endl;
}

SceneOptimizer::~SceneOptimizer() {
    clear();
}

// ==================== Automatic Batching ====================

void SceneOptimizer::analyzeBatching(const Scene& scene) {
    std::cout << "Analyzing scene for batching opportunities..." << std::endl;

    const auto& mobjects = scene.getMobjects();
    batches = createBatches(mobjects);

    std::cout << "Created " << batches.size() << " batches from " << mobjects.size() << " objects" << std::endl;
}

std::vector<RenderBatch> SceneOptimizer::createBatches(
    const std::vector<std::shared_ptr<Mobject>>& mobjects
) {
    std::vector<RenderBatch> result;

    // Group objects by material and other properties
    std::unordered_map<size_t, std::vector<std::shared_ptr<Mobject>>> groups;

    for (const auto& obj : mobjects) {
        if (!obj) continue;

        // Calculate hash based on batchable properties
        size_t hash = 0;  // TODO: Calculate proper hash
        groups[hash].push_back(obj);
    }

    // Create batches from groups
    for (auto& [hash, objects] : groups) {
        if (objects.size() >= batchingThreshold) {
            RenderBatch batch;
            batch.objects = objects;
            // batch.material = objects[0]->getMaterial();

            mergeGeometry(batch);
            result.push_back(batch);

            std::cout << "  Created batch with " << objects.size() << " objects" << std::endl;
        }
    }

    return result;
}

void SceneOptimizer::mergeGeometry(RenderBatch& batch) {
    std::cout << "Merging geometry for batch..." << std::endl;

    // Merge all vertex and index buffers into one
    std::vector<float> mergedVertices;
    std::vector<uint32_t> mergedIndices;

    uint32_t vertexOffset = 0;

    for (const auto& obj : batch.objects) {
        if (!obj) continue;

        // TODO: Get vertices and indices from object
        // TODO: Transform vertices by object transform
        // TODO: Append to merged buffers with offset

        // Placeholder
        // auto vertices = obj->getVertices();
        // auto indices = obj->getIndices();
        // mergedVertices.insert(mergedVertices.end(), vertices.begin(), vertices.end());
        // for (auto idx : indices) {
        //     mergedIndices.push_back(idx + vertexOffset);
        // }
        // vertexOffset += vertices.size() / vertexStride;
    }

    batch.vertexCount = mergedVertices.size();
    batch.indexCount = mergedIndices.size();

    updateBatchBuffers(batch);
}

void SceneOptimizer::updateBatchBuffers(RenderBatch& batch) {
    if (!batch.isDirty) return;

    // TODO: Create/update Vulkan buffers
    // - Create vertex buffer
    // - Create index buffer
    // - Upload data

    batch.isDirty = false;
}

// ==================== Instance Rendering ====================

std::vector<InstancedGroup> SceneOptimizer::detectInstancing(
    const std::vector<std::shared_ptr<Mobject>>& mobjects
) {
    std::vector<InstancedGroup> result;

    std::cout << "Detecting instancing opportunities..." << std::endl;

    // Group identical objects
    std::unordered_map<size_t, std::vector<std::shared_ptr<Mobject>>> groups;

    for (const auto& obj : mobjects) {
        if (!obj) continue;

        // Calculate hash based on mesh/geometry
        size_t hash = 0;  // TODO: Calculate proper hash
        groups[hash].push_back(obj);
    }

    // Create instance groups
    for (auto& [hash, objects] : groups) {
        if (objects.size() >= instancingThreshold) {
            std::vector<InstanceData> instances;

            for (const auto& obj : objects) {
                InstanceData data;
                // data.transform = obj->getTransform();
                // data.color = obj->getColor();
                instances.push_back(data);
            }

            auto group = createInstanceGroup(objects[0], instances);
            result.push_back(group);

            std::cout << "  Created instance group with " << instances.size() << " instances" << std::endl;
        }
    }

    return result;
}

InstancedGroup SceneOptimizer::createInstanceGroup(
    std::shared_ptr<Mobject> prototype,
    const std::vector<InstanceData>& instances
) {
    InstancedGroup group;
    group.prototype = prototype;
    group.instances = instances;

    updateInstanceBuffer(group);

    return group;
}

void SceneOptimizer::updateInstanceBuffer(InstancedGroup& group) {
    if (!group.isDirty) return;

    // TODO: Create/update instance buffer
    // - Create buffer
    // - Upload instance data

    group.isDirty = false;
}

// ==================== Draw Call Optimization ====================

std::vector<DrawCall> SceneOptimizer::optimizeDrawCalls(const Scene& scene) {
    std::cout << "Optimizing draw calls..." << std::endl;

    std::vector<DrawCall> calls;

    // Add draw calls for batches
    for (auto& batch : batches) {
        DrawCall call;
        call.vertexBuffer = batch.vertexBuffer;
        call.indexBuffer = batch.indexBuffer;
        call.indexCount = static_cast<uint32_t>(batch.indexCount);
        call.instanceCount = 1;
        calls.push_back(call);
    }

    // Add draw calls for instanced groups
    for (auto& group : instancedGroups) {
        DrawCall call;
        // call.vertexBuffer = group.prototype->getVertexBuffer();
        // call.indexBuffer = group.prototype->getIndexBuffer();
        // call.indexCount = group.prototype->getIndexCount();
        call.instanceCount = static_cast<uint32_t>(group.instances.size());
        calls.push_back(call);
    }

    stats.originalDrawCalls = scene.getMobjects().size();
    stats.optimizedDrawCalls = calls.size();

    // Sort to minimize state changes
    sortDrawCalls(calls);

    // Merge compatible calls
    calls = mergeDrawCalls(calls);

    stats.optimizedDrawCalls = calls.size();
    stats.optimizationRatio = stats.originalDrawCalls > 0
        ? static_cast<float>(stats.optimizedDrawCalls) / stats.originalDrawCalls
        : 1.0f;

    std::cout << "Reduced draw calls from " << stats.originalDrawCalls
              << " to " << stats.optimizedDrawCalls << std::endl;

    return calls;
}

void SceneOptimizer::sortDrawCalls(std::vector<DrawCall>& drawCalls) {
    // Sort by pipeline, then descriptor set to minimize state changes
    std::sort(drawCalls.begin(), drawCalls.end(),
        [](const DrawCall& a, const DrawCall& b) {
            if (a.pipeline != b.pipeline) return a.pipeline < b.pipeline;
            return a.descriptorSet < b.descriptorSet;
        }
    );
}

std::vector<DrawCall> SceneOptimizer::mergeDrawCalls(const std::vector<DrawCall>& drawCalls) {
    if (drawCalls.empty()) return {};

    std::vector<DrawCall> merged;
    DrawCall current = drawCalls[0];

    for (size_t i = 1; i < drawCalls.size(); ++i) {
        const auto& next = drawCalls[i];

        // Check if we can merge with current
        bool canMerge =
            current.pipeline == next.pipeline &&
            current.descriptorSet == next.descriptorSet &&
            current.vertexBuffer == next.vertexBuffer &&
            current.indexBuffer == next.indexBuffer;

        if (canMerge) {
            // Merge by extending index/instance count
            // This is simplified - actual merging is more complex
            current.indexCount += next.indexCount;
        } else {
            merged.push_back(current);
            current = next;
        }
    }

    merged.push_back(current);
    return merged;
}

// ==================== State Change Minimization ====================

void SceneOptimizer::buildStateGraph(const Scene& scene) {
    std::cout << "Building render state graph..." << std::endl;

    // Build graph of render state transitions
    // TODO: Implement state graph construction
}

std::vector<size_t> SceneOptimizer::findOptimalRenderOrder() {
    std::cout << "Finding optimal render order..." << std::endl;

    // Use greedy algorithm or more sophisticated optimization
    // TODO: Implement optimal ordering algorithm

    return {};
}

float SceneOptimizer::calculateStateChangeCost(const RenderState& from, const RenderState& to) {
    float cost = 0.0f;

    // Different state changes have different costs
    if (from.pipeline != to.pipeline) cost += 100.0f;  // Pipeline change is expensive
    if (from.descriptorSet != to.descriptorSet) cost += 10.0f;
    if (from.material != to.material) cost += 5.0f;
    if (from.depthTest != to.depthTest) cost += 1.0f;
    if (from.depthWrite != to.depthWrite) cost += 1.0f;
    if (from.blending != to.blending) cost += 2.0f;

    return cost;
}

// ==================== Statistics ====================

void SceneOptimizer::printOptimizationReport() const {
    std::cout << "\n=== Scene Optimization Report ===" << std::endl;
    std::cout << "Original draw calls: " << stats.originalDrawCalls << std::endl;
    std::cout << "Optimized draw calls: " << stats.optimizedDrawCalls << std::endl;
    std::cout << "Optimization ratio: " << (stats.optimizationRatio * 100.0f) << "%" << std::endl;
    std::cout << "Batch count: " << stats.batchCount << std::endl;
    std::cout << "Instanced groups: " << stats.instancedGroupCount << std::endl;
    std::cout << "Total instances: " << stats.totalInstances << std::endl;
    std::cout << "State changes: " << stats.stateChanges << std::endl;
    std::cout << "=================================\n" << std::endl;
}

// ==================== Configuration ====================

void SceneOptimizer::clear() {
    batches.clear();
    instancedGroups.clear();
    drawCalls.clear();
    stats = OptimizationStats();
}

// ==================== Private Helper Methods ====================

bool SceneOptimizer::canBatch(const Mobject& obj1, const Mobject& obj2) const {
    // Check if two objects can be batched together
    // TODO: Compare materials, shaders, etc.
    return false;  // Placeholder
}

bool SceneOptimizer::canInstance(const Mobject& obj1, const Mobject& obj2) const {
    // Check if two objects can be instanced together
    // TODO: Compare geometry/mesh
    return false;  // Placeholder
}

size_t SceneOptimizer::hashRenderState(const RenderState& state) const {
    // TODO: Implement proper hash function
    return 0;
}

void SceneOptimizer::updateStatistics() {
    stats.batchCount = batches.size();
    stats.instancedGroupCount = instancedGroups.size();

    stats.totalInstances = 0;
    for (const auto& group : instancedGroups) {
        stats.totalInstances += group.instances.size();
    }
}

// ==================== OptimizedScene Implementation ====================

OptimizedScene::OptimizedScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
)
    : Scene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations)
    , optimizer(std::make_shared<SceneOptimizer>())
    , autoOptimize(true)
{
    name = "OptimizedScene";
}

bool OptimizedScene::render(bool preview) {
    if (autoOptimize) {
        optimize();
    }

    return Scene::render(preview);
}

void OptimizedScene::optimize() {
    std::cout << "Running scene optimization..." << std::endl;

    // Analyze and create batches
    optimizer->analyzeBatching(*this);

    // Detect instancing opportunities
    auto instancedGroups = optimizer->detectInstancing(getMobjects());
    // TODO: Store instanced groups

    // Optimize draw calls
    auto drawCalls = optimizer->optimizeDrawCalls(*this);

    // Print report
    optimizer->printOptimizationReport();
}

} // namespace manim
