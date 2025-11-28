#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <chrono>
#include <optional>

#include "manim/core/types.h"
#include "manim/mobject/mobject.hpp"
#include "manim/animation/animation.hpp"
#include <any>

namespace manim {

// Forward declarations
class Camera;
class Renderer;
class Animation;
class Mobject;
class FileWriter;

/**
 * @brief Scene interaction action types
 */
enum class SceneInteractActionType {
    METHOD_CALL,
    CONTINUE,
    RERUN
};

/**
 * @brief Scene interaction action
 */
struct SceneInteractAction {
    SceneInteractActionType type;
    std::string sender;
    std::function<void()> method;
    std::unordered_map<std::string, std::any> kwargs;

    SceneInteractAction(SceneInteractActionType t, const std::string& s)
        : type(t), sender(s) {}
};

/**
 * @brief Base Scene class - canvas for animations
 *
 * The Scene class provides tools to manage mobjects and animations.
 * It is the fundamental building block of manim animations.
 */
class Scene {
public:
    /**
     * @brief Constructor
     *
     * @param renderer Custom renderer (nullptr for default)
     * @param cameraClass Type of camera to use
     * @param alwaysUpdateMobjects Always update mobjects even when static
     * @param randomSeed Random seed for reproducibility
     * @param skipAnimations Skip animation rendering
     */
    Scene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~Scene();

    // Disable copy, enable move
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;

    /**
     * @brief Render the scene
     * @param preview If true, opens scene in viewer
     * @return True if scene should be rerun
     */
    bool render(bool preview = false);

    /**
     * @brief Setup method called before construct
     * Override in subclasses for common setup code
     */
    virtual void setup();

    /**
     * @brief Main scene construction method
     * Override this method to define scene content
     */
    virtual void construct();

    /**
     * @brief Tear down method called after construct
     * Override in subclasses for cleanup code
     */
    virtual void tearDown();
    void tear_down() { tearDown(); }

    // ==================== Mobject Management ====================

    /**
     * @brief Add mobjects to the scene
     * @param mobjects Mobjects to add
     * @return Reference to this scene
     */
    Scene& add(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Add a single mobject to the scene
     */
    Scene& add(std::shared_ptr<Mobject> mobject);

    /**
     * @brief Remove mobjects from the scene
     * @param mobjects Mobjects to remove
     * @return Reference to this scene
     */
    Scene& remove(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Remove a single mobject from the scene
     */
    Scene& remove(std::shared_ptr<Mobject> mobject);

    /**
     * @brief Add mobjects to foreground
     */
    Scene& addForegroundMobjects(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Remove mobjects from foreground
     */
    Scene& removeForegroundMobjects(const std::vector<std::shared_ptr<Mobject>>& mobjects);

    /**
     * @brief Bring mobject to front
     */
    void bringToFront(std::shared_ptr<Mobject> mobject);

    /**
     * @brief Bring mobject to back
     */
    void bringToBack(std::shared_ptr<Mobject> mobject);

    /**
     * @brief Clear all mobjects from scene
     */
    void clear();

    // ==================== Animation Management ====================

    /**
     * @brief Play animations
     * @param animations List of animations to play
     */
    void play(const std::vector<std::shared_ptr<Animation>>& animations);

    /**
     * @brief Play a single animation
     */
    void play(std::shared_ptr<Animation> animation);

    /**
     * @brief Wait for a duration
     * @param duration Duration in seconds
     * @param stopCondition Optional stop condition function
     */
    void wait(double duration = 1.0,
              std::function<bool()> stopCondition = nullptr);

    /**
     * @brief Pause (wait for user input in interactive mode)
     */
    void pause();

    /**
     * @brief Skip current animation
     */
    void skipAnimation();

    // ==================== Update Functions ====================

    /**
     * @brief Update all mobjects
     * @param dt Time delta
     */
    void updateMobjects(double dt);

    /**
     * @brief Update scene-level updaters
     * @param dt Time delta
     */
    void updateSelf(double dt);

    /**
     * @brief Check if mobjects should be updated
     */
    bool shouldUpdateMobjects() const;

    /**
     * @brief Add scene updater function
     * @param updater Function to call each frame
     */
    void addUpdater(std::function<void(double)> updater);

    /**
     * @brief Remove scene updater function
     * @param updater Function to remove
     */
    void removeUpdater(const std::function<void(double)>& updater);

    // ==================== Mobject Queries ====================

    /**
     * @brief Get all top-level mobjects (not submobjects)
     */
    std::vector<std::shared_ptr<Mobject>> getTopLevelMobjects() const;

    /**
     * @brief Get all mobject family members including submobjects
     */
    std::vector<std::shared_ptr<Mobject>> getMobjectFamilyMembers() const;

    /**
     * @brief Get mobjects by name
     */
    std::vector<std::shared_ptr<Mobject>> getMobjectsByName(const std::string& name) const;

    /**
     * @brief Get mobject by name (first match)
     */
    std::shared_ptr<Mobject> getMobjectByName(const std::string& name) const;

    // ==================== Section Management ====================

    /**
     * @brief Start a new section
     * @param name Section name
     * @param skipAnimations Skip animations in this section
     */
    void nextSection(const std::string& name = "unnamed",
                     bool skipAnimations = false);

    // ==================== Interactive Mode ====================

    /**
     * @brief Enter interactive mode
     */
    void interact();

    /**
     * @brief Register keyboard callback
     */
    void registerKeyboardCallback(const std::string& key,
                                   std::function<void()> callback);

    /**
     * @brief Register mouse callback
     */
    void registerMouseCallback(std::function<void()> callback);

    // ==================== Getters ====================

    std::shared_ptr<Camera> getCamera() const { return camera; }
    std::shared_ptr<Renderer> getRenderer() const { return renderer; }
    const std::vector<std::shared_ptr<Mobject>>& getMobjects() const { return mobjects; }
    const std::vector<std::shared_ptr<Mobject>>& get_mobjects() const { return mobjects; }
    double getTime() const { return time; }
    double get_time() const { return getTime(); }
    double getDuration() const { return duration; }
    std::string getName() const { return name; }

    // ==================== Setters ====================

    void setCamera(std::shared_ptr<Camera> cam) { camera = cam; }
    void setName(const std::string& n) { name = n; }

protected:
    // Core components
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<FileWriter> fileWriter;

    // Mobject storage
    std::vector<std::shared_ptr<Mobject>> mobjects;
    std::vector<std::shared_ptr<Mobject>> foregroundMobjects;
    std::vector<std::shared_ptr<Mobject>> movingMobjects;
    std::vector<std::shared_ptr<Mobject>> staticMobjects;

    // Animation state
    std::vector<std::shared_ptr<Animation>> currentAnimations;
    std::function<bool()> stopCondition;
    bool skipAnimationsFlag;
    bool alwaysUpdateMobjects;

    // Timing
    double time;
    double duration;
    double lastT;

    // Scene properties
    std::string name;
    std::optional<uint32_t> randomSeed;

    // Updaters
    std::vector<std::function<void(double)>> updaters;

    // Interactive mode
    bool interactiveMode;
    std::queue<SceneInteractAction> interactQueue;
    std::unordered_map<std::string, std::function<void()>> keyCallbacks;
    std::vector<std::function<void()>> mouseCallbacks;

    // Helper methods
    void beginAnimations(const std::vector<std::shared_ptr<Animation>>& animations);
    void progressThroughAnimations(double runTime);
    void finishAnimations();
    void updateFrame(double dt);
    void addMobjectInternal(std::shared_ptr<Mobject> mobject);
    void removeMobjectInternal(std::shared_ptr<Mobject> mobject);

    // Section management
    std::string currentSection;
};

/**
 * @brief Scene with GPU-accelerated rendering pipeline
 */
class GPUScene : public Scene {
public:
    GPUScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~GPUScene() = default;

    /**
     * @brief Setup GPU resources
     */
    virtual void setupGPUResources();

    /**
     * @brief Cleanup GPU resources
     */
    virtual void cleanupGPUResources();

protected:
    // GPU resources managed by renderer
    bool gpuResourcesInitialized;
};

} // namespace manim
