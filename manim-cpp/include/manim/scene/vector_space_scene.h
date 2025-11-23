#pragma once

#include "manim/scene/scene.h"
#include "manim/core/types.h"
#include <memory>
#include <string>

namespace manim {

// Forward declarations
class CoordinateSystem;
class Axes;
class NumberPlane;
class Vector;

/**
 * @brief Scene specialized for vector space visualizations
 */
class VectorSpaceScene : public Scene {
public:
    VectorSpaceScene(
        std::shared_ptr<Renderer> renderer = nullptr,
        bool alwaysUpdateMobjects = false,
        std::optional<uint32_t> randomSeed = std::nullopt,
        bool skipAnimations = false
    );

    virtual ~VectorSpaceScene() = default;

    /**
     * @brief Add coordinate system
     */
    void addCoordinateSystem(std::shared_ptr<CoordinateSystem> coordSystem);

    /**
     * @brief Add vector to scene
     */
    void addVector(std::shared_ptr<Vector> vector);

    /**
     * @brief Add basis vectors
     */
    void addBasisVectors();

    /**
     * @brief Show linear transformation
     */
    void applyLinearTransformation(const Mat4& matrix, double duration = 1.0);

    /**
     * @brief Get coordinate system
     */
    std::shared_ptr<CoordinateSystem> getCoordinateSystem() const {
        return coordinateSystem;
    }

protected:
    std::shared_ptr<CoordinateSystem> coordinateSystem;
    std::vector<std::shared_ptr<Vector>> vectors;
};

} // namespace manim
