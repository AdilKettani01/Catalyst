#include "manim/scene/scene.hpp"
#include "manim/renderer/renderer.hpp"
#include "manim/animation/animation.hpp"
#include <algorithm>
#include <random>
#include <iostream>

namespace manim {

Scene::Scene(
    std::shared_ptr<Renderer> renderer_,
    bool alwaysUpdateMobjects_,
    std::optional<uint32_t> randomSeed_,
    bool skipAnimations_
)
    : renderer(renderer_)
    , camera(nullptr)
    , alwaysUpdateMobjects(alwaysUpdateMobjects_)
    , randomSeed(randomSeed_)
    , skipAnimationsFlag(skipAnimations_)
    , time(0.0)
    , duration(0.0)
    , lastT(0.0)
    , interactiveMode(false)
    , name("Scene")
{
    // Initialize random seed if provided
    if (randomSeed.has_value()) {
        std::srand(randomSeed.value());
    }

    // Create default renderer if none provided
    if (!renderer) {
        // renderer = std::make_shared<HybridRenderer>();
        // Will be implemented with renderer system
    }

    // Initialize camera from renderer
    if (renderer) {
        camera = renderer->getCamera();
    }
}

Scene::~Scene() {
    // Cleanup
    mobjects.clear();
    foregroundMobjects.clear();
    movingMobjects.clear();
    staticMobjects.clear();
    updaters.clear();
}

bool Scene::render(bool preview) {
    setup();

    try {
        construct();
    } catch (const std::exception& e) {
        std::cerr << "Exception during scene construction: " << e.what() << std::endl;
        tearDown();
        return false;
    }

    tearDown();

    if (renderer) {
        renderer->finishScene();
    }

    std::cout << "Rendered " << name << std::endl;
    std::cout << "Played " << duration << "s of animations" << std::endl;

    return false;
}

void Scene::setup() {
    // Override in subclasses
}

void Scene::construct() {
    // Override in subclasses
}

void Scene::tearDown() {
    // Override in subclasses
}

// ==================== Mobject Management ====================

Scene& Scene::add(const std::vector<std::shared_ptr<Mobject>>& mobjects_) {
    for (const auto& mobject : mobjects_) {
        addMobjectInternal(mobject);
    }
    return *this;
}

Scene& Scene::add(std::shared_ptr<Mobject> mobject) {
    addMobjectInternal(mobject);
    return *this;
}

Scene& Scene::remove(const std::vector<std::shared_ptr<Mobject>>& mobjects_) {
    for (const auto& mobject : mobjects_) {
        removeMobjectInternal(mobject);
    }
    return *this;
}

Scene& Scene::remove(std::shared_ptr<Mobject> mobject) {
    removeMobjectInternal(mobject);
    return *this;
}

void Scene::addMobjectInternal(std::shared_ptr<Mobject> mobject) {
    if (!mobject) return;

    // Check if already in scene
    auto it = std::find(mobjects.begin(), mobjects.end(), mobject);
    if (it != mobjects.end()) {
        return;
    }

    // Add to scene
    mobjects.push_back(mobject);

    // Register with renderer
    if (renderer) {
        renderer->addMobject(mobject);
    }
}

void Scene::removeMobjectInternal(std::shared_ptr<Mobject> mobject) {
    if (!mobject) return;

    // Remove from mobjects
    auto it = std::find(mobjects.begin(), mobjects.end(), mobject);
    if (it != mobjects.end()) {
        mobjects.erase(it);
    }

    // Remove from renderer
    if (renderer) {
        renderer->removeMobject(mobject);
    }
}

Scene& Scene::addForegroundMobjects(const std::vector<std::shared_ptr<Mobject>>& mobjects_) {
    for (const auto& mobject : mobjects_) {
        foregroundMobjects.push_back(mobject);
        addMobjectInternal(mobject);
    }
    return *this;
}

Scene& Scene::removeForegroundMobjects(const std::vector<std::shared_ptr<Mobject>>& mobjects_) {
    for (const auto& mobject : mobjects_) {
        auto it = std::find(foregroundMobjects.begin(), foregroundMobjects.end(), mobject);
        if (it != foregroundMobjects.end()) {
            foregroundMobjects.erase(it);
        }
        removeMobjectInternal(mobject);
    }
    return *this;
}

void Scene::bringToFront(std::shared_ptr<Mobject> mobject) {
    auto it = std::find(mobjects.begin(), mobjects.end(), mobject);
    if (it != mobjects.end()) {
        mobjects.erase(it);
        mobjects.push_back(mobject);
    }
}

void Scene::bringToBack(std::shared_ptr<Mobject> mobject) {
    auto it = std::find(mobjects.begin(), mobjects.end(), mobject);
    if (it != mobjects.end()) {
        mobjects.erase(it);
        mobjects.insert(mobjects.begin(), mobject);
    }
}

void Scene::clear() {
    mobjects.clear();
    foregroundMobjects.clear();
    movingMobjects.clear();
    staticMobjects.clear();

    if (renderer) {
        renderer->clear();
    }
}

// ==================== Animation Management ====================

void Scene::play(const std::vector<std::shared_ptr<Animation>>& animations) {
    if (animations.empty()) return;

    beginAnimations(animations);

    // Calculate total run time
    double runTime = 0.0;
    for (const auto& anim : animations) {
        if (anim) {
            runTime = std::max(runTime, anim->getRunTime());
        }
    }

    progressThroughAnimations(runTime);
    finishAnimations();
}

void Scene::play(std::shared_ptr<Animation> animation) {
    if (animation) {
        play(std::vector<std::shared_ptr<Animation>>{animation});
    }
}

void Scene::wait(double duration_, std::function<bool()> stopCondition_) {
    this->stopCondition = stopCondition_;

    // Create wait animation
    // auto waitAnim = std::make_shared<Wait>(duration_);
    // play(waitAnim);

    // For now, just update time
    time += duration_;
    duration += duration_;
}

void Scene::pause() {
    // Interactive pause - wait for user input
    if (interactiveMode) {
        // Implementation depends on interactive system
    }
}

void Scene::skipAnimation() {
    if (!currentAnimations.empty()) {
        // Skip to end of current animations
        for (auto& anim : currentAnimations) {
            if (anim) {
                anim->finish();
            }
        }
    }
}

void Scene::beginAnimations(const std::vector<std::shared_ptr<Animation>>& animations) {
    currentAnimations = animations;

    // Begin each animation
    for (auto& anim : currentAnimations) {
        if (anim) {
            anim->begin();
        }
    }

    // Identify moving mobjects
    movingMobjects.clear();
    for (const auto& anim : currentAnimations) {
        if (anim) {
            auto animMobject = anim->getMobject();
            if (animMobject) {
                movingMobjects.push_back(animMobject);
            }
        }
    }
}

void Scene::progressThroughAnimations(double runTime) {
    double t = 0.0;
    const double dt = 1.0 / 60.0; // 60 FPS

    while (t < runTime) {
        double alpha = t / runTime;

        // Update animations
        for (auto& anim : currentAnimations) {
            if (anim) {
                anim->interpolate(alpha);
            }
        }

        // Update mobjects
        updateMobjects(dt);
        updateSelf(dt);

        // Render frame
        if (renderer && !skipAnimationsFlag) {
            renderer->render();
        }

        t += dt;
        time += dt;
    }

    duration += runTime;
}

void Scene::finishAnimations() {
    // Finish each animation
    for (auto& anim : currentAnimations) {
        if (anim) {
            anim->finish();
        }
    }

    currentAnimations.clear();
    movingMobjects.clear();
}

// ==================== Update Functions ====================

void Scene::updateMobjects(double dt) {
    for (auto& mobject : mobjects) {
        if (mobject) {
            mobject->update(dt);
        }
    }
}

void Scene::updateSelf(double dt) {
    for (auto& updater : updaters) {
        if (updater) {
            updater(dt);
        }
    }
}

bool Scene::shouldUpdateMobjects() const {
    if (alwaysUpdateMobjects) return true;
    if (!updaters.empty()) return true;
    if (stopCondition) return true;

    // Check if any mobject has time-based updaters
    for (const auto& mob : getMobjectFamilyMembers()) {
        if (mob && mob->hasTimeBasedUpdater()) {
            return true;
        }
    }

    return false;
}

void Scene::addUpdater(std::function<void(double)> updater) {
    if (updater) {
        updaters.push_back(updater);
    }
}

void Scene::removeUpdater(const std::function<void(double)>& updater) {
    // Note: function comparison is tricky in C++
    // This is a simplified version
    // TODO: Implement proper function comparison or use IDs
}

// ==================== Mobject Queries ====================

std::vector<std::shared_ptr<Mobject>> Scene::getTopLevelMobjects() const {
    std::vector<std::shared_ptr<Mobject>> topLevel;

    for (const auto& mob : mobjects) {
        if (!mob) continue;

        // Check if this mobject is in another mobject's family
        bool isSubmobject = false;
        for (const auto& other : mobjects) {
            if (other == mob || !other) continue;

            auto family = other->getFamily();
            if (std::find(family.begin(), family.end(), mob) != family.end()) {
                isSubmobject = true;
                break;
            }
        }

        if (!isSubmobject) {
            topLevel.push_back(mob);
        }
    }

    return topLevel;
}

std::vector<std::shared_ptr<Mobject>> Scene::getMobjectFamilyMembers() const {
    std::vector<std::shared_ptr<Mobject>> familyMembers;

    for (const auto& mob : mobjects) {
        if (mob) {
            auto family = mob->getFamily();
            familyMembers.insert(familyMembers.end(), family.begin(), family.end());
        }
    }

    return familyMembers;
}

std::vector<std::shared_ptr<Mobject>> Scene::getMobjectsByName(const std::string& name_) const {
    std::vector<std::shared_ptr<Mobject>> result;

    for (const auto& mob : getMobjectFamilyMembers()) {
        if (mob && mob->getName() == name_) {
            result.push_back(mob);
        }
    }

    return result;
}

std::shared_ptr<Mobject> Scene::getMobjectByName(const std::string& name_) const {
    auto mobs = getMobjectsByName(name_);
    return mobs.empty() ? nullptr : mobs[0];
}

// ==================== Section Management ====================

void Scene::nextSection(const std::string& name_, bool skipAnimations_) {
    currentSection = name_;
    // TODO: Integrate with file writer for section management
}

// ==================== Interactive Mode ====================

void Scene::interact() {
    interactiveMode = true;
    // TODO: Implement interactive mode
}

void Scene::registerKeyboardCallback(const std::string& key, std::function<void()> callback) {
    keyCallbacks[key] = callback;
}

void Scene::registerMouseCallback(std::function<void()> callback) {
    mouseCallbacks.push_back(callback);
}

// ==================== GPUScene Implementation ====================

GPUScene::GPUScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
)
    : Scene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations)
    , gpuResourcesInitialized(false)
{
}

void GPUScene::setupGPUResources() {
    if (gpuResourcesInitialized) return;

    // Setup GPU resources through renderer
    if (renderer) {
        // renderer->initializeGPUResources();
    }

    gpuResourcesInitialized = true;
}

void GPUScene::cleanupGPUResources() {
    if (!gpuResourcesInitialized) return;

    // Cleanup GPU resources through renderer
    if (renderer) {
        // renderer->cleanupGPUResources();
    }

    gpuResourcesInitialized = false;
}

} // namespace manim
