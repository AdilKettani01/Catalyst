#pragma once

#include "manim/animation/animation.hpp"

namespace manim {

/**
 * @brief Transform animation - morphs one mobject into another
 *
 * GPU Optimizations:
 * - Parallel point interpolation on compute shaders
 * - Bezier curve morphing on GPU
 * - Batch processing for multiple transforms
 * - Path function evaluation on GPU
 */
class Transform : public Animation {
public:
    Transform(
        std::shared_ptr<Mobject> mobject,
        std::shared_ptr<Mobject> target_mobject,
        float run_time = 1.0f
    );

    virtual ~Transform() = default;

    // GPU support
    bool can_run_on_gpu() const override { return true; }

    void upload_to_gpu(GPUAnimationEngine& engine) override;
    void interpolate_on_gpu(float t, VkCommandBuffer cmd) override;

protected:
    void begin() override;
    void interpolate_mobject(float alpha) override;

    std::shared_ptr<Mobject> target_mobject_;
    std::shared_ptr<Mobject> starting_mobject_;
};

/**
 * @brief Replace transform - simply replaces mobject
 */
class ReplacementTransform : public Transform {
public:
    using Transform::Transform;

    void finish() override;
};

/**
 * @brief Transform from copy
 */
class TransformFromCopy : public Transform {
public:
    TransformFromCopy(
        std::shared_ptr<Mobject> mobject,
        std::shared_ptr<Mobject> target_mobject,
        float run_time = 1.0f
    );

protected:
    void begin() override;
};

/**
 * @brief Clockwise transform
 */
class ClockwiseTransform : public Transform {
public:
    using Transform::Transform;

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Counter-clockwise transform
 */
class CounterclockwiseTransform : public Transform {
public:
    using Transform::Transform;

protected:
    void interpolate_mobject(float alpha) override;
};

/**
 * @brief Move to target
 */
class MoveToTarget : public Transform {
public:
    explicit MoveToTarget(
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f
    );

protected:
    void begin() override;
};

/**
 * @brief Apply function animation
 */
class ApplyFunction : public Transform {
public:
    using TransformFunc = std::function<std::shared_ptr<Mobject>(std::shared_ptr<Mobject>)>;

    ApplyFunction(
        TransformFunc func,
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f
    );

protected:
    void begin() override;

    TransformFunc func_;
};

/**
 * @brief Apply matrix animation
 */
class ApplyMatrix : public ApplyFunction {
public:
    ApplyMatrix(
        const math::Mat4& matrix,
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f
    );

    bool can_run_on_gpu() const override { return true; }
};

/**
 * @brief Apply complex function (for complex plane)
 */
class ApplyComplexFunction : public ApplyFunction {
public:
    using ComplexFunc = std::function<std::complex<float>(std::complex<float>)>;

    ApplyComplexFunction(
        ComplexFunc func,
        std::shared_ptr<Mobject> mobject,
        float run_time = 1.0f
    );
};

}  // namespace manim
