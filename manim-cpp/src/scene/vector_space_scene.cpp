#include "manim/scene/vector_space_scene.h"
#include <iostream>

namespace manim {

VectorSpaceScene::VectorSpaceScene(
    std::shared_ptr<Renderer> renderer,
    bool alwaysUpdateMobjects,
    std::optional<uint32_t> randomSeed,
    bool skipAnimations
)
    : Scene(renderer, alwaysUpdateMobjects, randomSeed, skipAnimations)
{
    name = "VectorSpaceScene";
}

void VectorSpaceScene::addCoordinateSystem(std::shared_ptr<CoordinateSystem> coordSystem) {
    coordinateSystem = coordSystem;
    // add(coordSystem);
}

void VectorSpaceScene::addVector(std::shared_ptr<Vector> vector) {
    vectors.push_back(vector);
    // add(vector);
}

void VectorSpaceScene::addBasisVectors() {
    std::cout << "Adding basis vectors" << std::endl;
    // TODO: Create and add i, j, k basis vectors
}

void VectorSpaceScene::applyLinearTransformation(const Mat4& matrix, double duration) {
    std::cout << "Applying linear transformation" << std::endl;
    // TODO: Animate transformation of coordinate system and vectors
}

} // namespace manim
