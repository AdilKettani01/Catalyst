#pragma once

#include "manim/core/types.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace manim {

// Forward declarations
class Scene;
class Mobject;
class Animation;

/**
 * @brief Task types for work distribution
 */
enum class TaskType {
    RENDER,
    PHYSICS,
    ANIMATION,
    LOGIC,
    INPUT
};

/**
 * @brief Complexity metrics for a scene or task
 */
struct ComplexityMetrics {
    size_t numMobjects;
    size_t numVertices;
    size_t numTriangles;
    size_t numAnimations;
    size_t numLights;
    size_t numEffects;
    float computeIntensity;  // 0.0 to 1.0
    float memoryUsage;       // In MB
    float estimatedTime;     // In ms

    ComplexityMetrics()
        : numMobjects(0)
        , numVertices(0)
        , numTriangles(0)
        , numAnimations(0)
        , numLights(0)
        , numEffects(0)
        , computeIntensity(0.0f)
        , memoryUsage(0.0f)
        , estimatedTime(0.0f)
    {}
};

/**
 * @brief Work partition between CPU and GPU
 */
struct WorkPartition {
    std::vector<std::shared_ptr<Mobject>> cpuObjects;
    std::vector<std::shared_ptr<Mobject>> gpuObjects;

    std::vector<std::shared_ptr<Animation>> cpuAnimations;
    std::vector<std::shared_ptr<Animation>> gpuAnimations;

    TaskType primaryTaskType;
    float cpuLoad;  // 0.0 to 1.0
    float gpuLoad;  // 0.0 to 1.0
};

/**
 * @brief Hybrid Scene Manager for intelligent CPU/GPU distribution
 *
 * This manager analyzes scene complexity and intelligently distributes
 * work between CPU and GPU to maximize performance.
 */
class HybridSceneManager {
public:
    HybridSceneManager();
    ~HybridSceneManager();

    // Disable copy, enable move
    HybridSceneManager(const HybridSceneManager&) = delete;
    HybridSceneManager& operator=(const HybridSceneManager&) = delete;
    HybridSceneManager(HybridSceneManager&&) = default;
    HybridSceneManager& operator=(HybridSceneManager&&) = default;

    // ==================== Analysis ====================

    /**
     * @brief Analyze scene complexity
     * @param scene Scene to analyze
     * @return Complexity metrics
     */
    ComplexityMetrics analyzeSceneComplexity(const Scene& scene);

    /**
     * @brief Analyze mobject complexity
     * @param mobject Mobject to analyze
     * @return Complexity metrics
     */
    ComplexityMetrics analyzeMobjectComplexity(const Mobject& mobject);

    /**
     * @brief Partition scene objects for optimal CPU/GPU distribution
     * @param scene Scene to partition
     * @return Work partition
     */
    WorkPartition partitionSceneObjects(const Scene& scene);

    // ==================== CPU Tasks ====================

    /**
     * @brief Update game logic on CPU
     * @param dt Time delta
     */
    void updateGameLogic(double dt);

    /**
     * @brief Process user input on CPU
     */
    void processUserInput();

    /**
     * @brief Manage scene graph on CPU
     * @param scene Scene to manage
     */
    void manageSceneGraph(Scene& scene);

    /**
     * @brief Perform CPU-side animation updates
     * @param animations Animations to update
     * @param dt Time delta
     */
    void updateCPUAnimations(const std::vector<std::shared_ptr<Animation>>& animations, double dt);

    // ==================== GPU Tasks ====================

    /**
     * @brief Render all objects on GPU
     * @param objects Objects to render
     */
    void renderAllObjects(const std::vector<std::shared_ptr<Mobject>>& objects);

    /**
     * @brief Simulate physics on GPU
     * @param objects Objects with physics
     * @param dt Time delta
     */
    void simulatePhysics(const std::vector<std::shared_ptr<Mobject>>& objects, double dt);

    /**
     * @brief Process effects on GPU
     * @param objects Objects with effects
     */
    void processEffects(const std::vector<std::shared_ptr<Mobject>>& objects);

    /**
     * @brief Perform GPU-side animation updates
     * @param animations Animations to update
     * @param dt Time delta
     */
    void updateGPUAnimations(const std::vector<std::shared_ptr<Animation>>& animations, double dt);

    // ==================== Synchronization ====================

    /**
     * @brief Synchronize CPU and GPU state
     */
    void syncCPUGPUState();

    /**
     * @brief Wait for GPU to finish current work
     */
    void waitForGPU();

    /**
     * @brief Wait for CPU to finish current work
     */
    void waitForCPU();

    /**
     * @brief Check if CPU and GPU are synchronized
     */
    bool isSynchronized() const;

    // ==================== Performance Monitoring ====================

    /**
     * @brief Get CPU utilization (0.0 to 1.0)
     */
    float getCPUUtilization() const;

    /**
     * @brief Get GPU utilization (0.0 to 1.0)
     */
    float getGPUUtilization() const;

    /**
     * @brief Get current frame time in milliseconds
     */
    float getFrameTime() const;

    /**
     * @brief Get current FPS
     */
    float getFPS() const;

    /**
     * @brief Print performance statistics
     */
    void printPerformanceStats() const;

    // ==================== Configuration ====================

    /**
     * @brief Set target CPU load (0.0 to 1.0)
     */
    void setTargetCPULoad(float load);

    /**
     * @brief Set target GPU load (0.0 to 1.0)
     */
    void setTargetGPULoad(float load);

    /**
     * @brief Enable/disable automatic load balancing
     */
    void setAutoLoadBalancing(bool enable);

    /**
     * @brief Set complexity threshold for GPU offloading
     */
    void setGPUOffloadThreshold(float threshold);

private:
    // Performance tracking
    struct PerformanceStats {
        float cpuUtilization;
        float gpuUtilization;
        float frameTime;
        float fps;
        size_t framesRendered;

        PerformanceStats()
            : cpuUtilization(0.0f)
            , gpuUtilization(0.0f)
            , frameTime(0.0f)
            , fps(0.0f)
            , framesRendered(0)
        {}
    };

    PerformanceStats performanceStats;

    // Configuration
    float targetCPULoad;
    float targetGPULoad;
    bool autoLoadBalancing;
    float gpuOffloadThreshold;

    // Synchronization
    std::atomic<bool> cpuBusy;
    std::atomic<bool> gpuBusy;
    std::mutex syncMutex;
    std::condition_variable syncCV;

    // Worker threads
    std::vector<std::thread> workerThreads;
    std::atomic<bool> running;

    // Helper methods
    void updatePerformanceStats();
    void balanceLoad();
    bool shouldOffloadToGPU(const ComplexityMetrics& metrics) const;
    void initializeWorkerThreads();
    void shutdownWorkerThreads();
};

/**
 * @brief Smart Scene with automatic hybrid management
 */
class SmartScene : public Scene {
public:
    SmartScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~SmartScene();

    /**
     * @brief Override render to use hybrid manager
     */
    bool render(bool preview = false) override;

    /**
     * @brief Get hybrid scene manager
     */
    std::shared_ptr<HybridSceneManager> getHybridManager() const {
        return hybridManager;
    }

protected:
    std::shared_ptr<HybridSceneManager> hybridManager;

    void optimizeScene();
    void distributeWork();
};

} // namespace manim
