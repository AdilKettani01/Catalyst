#include "manim/scene/hybrid_scene_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace manim {

HybridSceneManager::HybridSceneManager() = default;
HybridSceneManager::~HybridSceneManager() = default;

SmartScene::SmartScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
) : Scene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations) {
    hybridManager = std::make_shared<HybridSceneManager>();
}

SmartScene::~SmartScene() = default;

bool SmartScene::render(bool preview) {
    if (hybridManager) {
        hybridManager->render();
    }
    return Scene::render(preview);
}

void HybridSceneManager::add_scene(std::shared_ptr<Scene> scene) {
    if (scene) {
        scenes_.push_back(scene);
    }
}

void HybridSceneManager::remove_scene(std::shared_ptr<Scene> scene) {
    scenes_.erase(
        std::remove(scenes_.begin(), scenes_.end(), scene),
        scenes_.end()
    );
}

void HybridSceneManager::update(float dt) {
    (void)dt;
    distribute_workload();
    process_cpu_tasks();
    process_gpu_tasks();
}

void HybridSceneManager::render() {
    for (auto& scene : scenes_) {
        if (scene) {
            scene->render();
        }
    }
}

void HybridSceneManager::set_distribution_strategy(DistributionStrategy strategy) {
    strategy_ = strategy;
}

HybridSceneManager::PerformanceMetrics HybridSceneManager::get_metrics() const {
    return metrics_;
}

void HybridSceneManager::distribute_workload() {
    // Stub workload distribution
}

void HybridSceneManager::process_cpu_tasks() {
    // Stub CPU processing
}

void HybridSceneManager::process_gpu_tasks() {
    // Stub GPU processing
}

}  // namespace manim
