/**
 * @file test_mobjects.cpp
 * @brief Comprehensive unit tests for all mobject types
 *
 * Tests ported from Python Manim test suite with GPU enhancements
 */

#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#include "manim/mobject/mobject.hpp"
#include "manim/mobject/vmobject.hpp"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/mobject/three_d/mesh.hpp"
#include "manim/mobject/three_d/surface.hpp"
#include "manim/mobject/three_d/volume.hpp"
#include "manim/core/types.h"

using namespace manim;

// ==================== Base Mobject Tests ====================

class MobjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        mobject = std::make_shared<Mobject>();
    }

    std::shared_ptr<Mobject> mobject;
};

TEST_F(MobjectTest, DefaultConstruction) {
    EXPECT_NE(mobject, nullptr);
    EXPECT_EQ(mobject->get_center(), math::Vec3(0, 0, 0));
}

TEST_F(MobjectTest, Shift) {
    mobject->shift(math::Vec3(1, 2, 3));
    auto center = mobject->get_center();
    EXPECT_FLOAT_EQ(center.x, 1.0f);
    EXPECT_FLOAT_EQ(center.y, 2.0f);
    EXPECT_FLOAT_EQ(center.z, 3.0f);
}

TEST_F(MobjectTest, Scale) {
    mobject->shift(math::Vec3(1, 1, 1));
    mobject->scale(2.0f);
    auto center = mobject->get_center();
    EXPECT_FLOAT_EQ(center.x, 2.0f);
    EXPECT_FLOAT_EQ(center.y, 2.0f);
    EXPECT_FLOAT_EQ(center.z, 2.0f);
}

TEST_F(MobjectTest, MoveTo) {
    mobject->move_to(math::Vec3(5, 6, 7));
    auto center = mobject->get_center();
    EXPECT_FLOAT_EQ(center.x, 5.0f);
    EXPECT_FLOAT_EQ(center.y, 6.0f);
    EXPECT_FLOAT_EQ(center.z, 7.0f);
}

TEST_F(MobjectTest, Rotate) {
    // Test rotation doesn't crash
    EXPECT_NO_THROW(mobject->rotate(math::PI / 2, math::Vec3(0, 0, 1)));
}

TEST_F(MobjectTest, ColorManipulation) {
    math::Vec4 red(1, 0, 0, 1);
    mobject->set_color(red);
    auto color = mobject->get_color();
    EXPECT_FLOAT_EQ(color.x, 1.0f);
    EXPECT_FLOAT_EQ(color.y, 0.0f);
    EXPECT_FLOAT_EQ(color.z, 0.0f);
    EXPECT_FLOAT_EQ(color.w, 1.0f);
}

TEST_F(MobjectTest, OpacityManipulation) {
    mobject->set_opacity(0.5f);
    auto color = mobject->get_color();
    EXPECT_FLOAT_EQ(color.w, 0.5f);
}

TEST_F(MobjectTest, AddSubmobjects) {
    auto child1 = std::make_shared<Mobject>();
    auto child2 = std::make_shared<Mobject>();

    mobject->add(child1);
    mobject->add(child2);

    auto family = mobject->get_family();
    EXPECT_EQ(family.size(), 3); // parent + 2 children
}

TEST_F(MobjectTest, RemoveSubmobjects) {
    auto child = std::make_shared<Mobject>();
    mobject->add(child);
    EXPECT_EQ(mobject->get_family().size(), 2);

    mobject->remove(child);
    EXPECT_EQ(mobject->get_family().size(), 1);
}

TEST_F(MobjectTest, SaveAndRestoreState) {
    mobject->shift(math::Vec3(1, 2, 3));
    mobject->save_state();

    mobject->shift(math::Vec3(10, 10, 10));
    auto center_before = mobject->get_center();
    EXPECT_FLOAT_EQ(center_before.x, 11.0f);

    mobject->restore();
    auto center_after = mobject->get_center();
    EXPECT_FLOAT_EQ(center_after.x, 1.0f);
    EXPECT_FLOAT_EQ(center_after.y, 2.0f);
}

TEST_F(MobjectTest, Copy) {
    mobject->shift(math::Vec3(1, 2, 3));
    mobject->set_color(math::Vec4(1, 0, 0, 1));

    auto copy = mobject->copy();
    EXPECT_NE(copy, mobject);

    auto center = copy->get_center();
    EXPECT_FLOAT_EQ(center.x, 1.0f);
    EXPECT_FLOAT_EQ(center.y, 2.0f);
}

// ==================== Circle Tests ====================

class CircleTest : public ::testing::Test {
protected:
    void SetUp() override {
        circle = std::make_shared<Circle>(1.0f);
    }

    std::shared_ptr<Circle> circle;
};

TEST_F(CircleTest, DefaultRadius) {
    auto c = std::make_shared<Circle>();
    EXPECT_FLOAT_EQ(c->get_radius(), 1.0f);
}

TEST_F(CircleTest, CustomRadius) {
    auto c = std::make_shared<Circle>(2.5f);
    EXPECT_FLOAT_EQ(c->get_radius(), 2.5f);
}

TEST_F(CircleTest, SetRadius) {
    circle->set_radius(3.0f);
    EXPECT_FLOAT_EQ(circle->get_radius(), 3.0f);
}

TEST_F(CircleTest, InheritsFromVMobject) {
    // Circle should have VMobject methods
    EXPECT_NO_THROW(circle->set_fill(math::Vec4(1, 0, 0, 1), 1.0f));
    EXPECT_NO_THROW(circle->set_stroke(math::Vec4(0, 1, 0, 1), 2.0f));
}

TEST_F(CircleTest, PointGeneration) {
    auto points = circle->get_points();
    EXPECT_GT(points.size(), 0);
}

TEST_F(CircleTest, SurroundMobject) {
    auto target = std::make_shared<Mobject>();
    target->shift(math::Vec3(2, 2, 0));

    EXPECT_NO_THROW(circle->surround(*target, 0.2f));

    // Circle center should be near target center
    auto circle_center = circle->get_center();
    auto target_center = target->get_center();

    float dx = std::abs(circle_center.x - target_center.x);
    float dy = std::abs(circle_center.y - target_center.y);

    EXPECT_LT(dx, 0.1f);
    EXPECT_LT(dy, 0.1f);
}

// ==================== Dot Tests ====================

TEST(DotTest, DefaultDotRadius) {
    auto dot = std::make_shared<Dot>();
    EXPECT_FLOAT_EQ(dot->get_radius(), Dot::DEFAULT_DOT_RADIUS);
}

TEST(DotTest, CustomPosition) {
    math::Vec3 pos(1, 2, 3);
    auto dot = std::make_shared<Dot>(pos);

    auto center = dot->get_center();
    EXPECT_FLOAT_EQ(center.x, 1.0f);
    EXPECT_FLOAT_EQ(center.y, 2.0f);
    EXPECT_FLOAT_EQ(center.z, 3.0f);
}

TEST(DotTest, CustomRadius) {
    auto dot = std::make_shared<Dot>(math::Vec3(0, 0, 0), 0.2f);
    EXPECT_FLOAT_EQ(dot->get_radius(), 0.2f);
}

// ==================== Ellipse Tests ====================

TEST(EllipseTest, DefaultDimensions) {
    auto ellipse = std::make_shared<Ellipse>();
    // Default width=2, height=1
    EXPECT_NO_THROW(ellipse->get_points());
}

TEST(EllipseTest, CustomDimensions) {
    auto ellipse = std::make_shared<Ellipse>(4.0f, 2.0f);
    EXPECT_NO_THROW(ellipse->set_width(5.0f));
    EXPECT_NO_THROW(ellipse->set_height(3.0f));
}

TEST(EllipseTest, SetDimensions) {
    auto ellipse = std::make_shared<Ellipse>();
    EXPECT_NO_THROW(ellipse->set_width(3.0f));
    EXPECT_NO_THROW(ellipse->set_height(1.5f));
}

// ==================== Arc Tests ====================

TEST(ArcTest, DefaultArc) {
    auto arc = std::make_shared<Arc>();
    EXPECT_FLOAT_EQ(arc->get_angle(), math::PI / 2.0f);
}

TEST(ArcTest, CustomAngle) {
    auto arc = std::make_shared<Arc>(1.0f, 0.0f, math::PI);
    EXPECT_FLOAT_EQ(arc->get_angle(), math::PI);
}

TEST(ArcTest, SetAngle) {
    auto arc = std::make_shared<Arc>();
    arc->set_angle(math::PI);
    EXPECT_FLOAT_EQ(arc->get_angle(), math::PI);
}

TEST(ArcTest, FullCircle) {
    auto arc = std::make_shared<Arc>(1.0f, 0.0f, 2.0f * math::PI);
    EXPECT_FLOAT_EQ(arc->get_angle(), 2.0f * math::PI);
}

// ==================== Annulus Tests ====================

TEST(AnnulusTest, DefaultDimensions) {
    auto annulus = std::make_shared<Annulus>();
    EXPECT_NO_THROW(annulus->get_points());
}

TEST(AnnulusTest, CustomRadii) {
    auto annulus = std::make_shared<Annulus>(0.3f, 1.5f);
    EXPECT_NO_THROW(annulus->get_points());
}

// ==================== VMobject Tests ====================

class VMobjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        vmobject = std::make_shared<VMobject>();
    }

    std::shared_ptr<VMobject> vmobject;
};

TEST_F(VMobjectTest, SetFill) {
    EXPECT_NO_THROW(vmobject->set_fill(math::Vec4(1, 0, 0, 1), 1.0f));
}

TEST_F(VMobjectTest, SetStroke) {
    EXPECT_NO_THROW(vmobject->set_stroke(math::Vec4(0, 1, 0, 1), 2.0f));
}

TEST_F(VMobjectTest, BezierPoints) {
    std::vector<math::Vec3> points = {
        math::Vec3(0, 0, 0),
        math::Vec3(1, 1, 0),
        math::Vec3(2, 1, 0),
        math::Vec3(3, 0, 0)
    };

    EXPECT_NO_THROW(vmobject->set_points(points));
    auto retrieved = vmobject->get_points();
    EXPECT_EQ(retrieved.size(), points.size());
}

TEST_F(VMobjectTest, AddCubicBezierCurve) {
    math::Vec3 p0(0, 0, 0);
    math::Vec3 p1(1, 1, 0);
    math::Vec3 p2(2, 1, 0);
    math::Vec3 p3(3, 0, 0);

    EXPECT_NO_THROW(vmobject->add_cubic_bezier_curve(p0, p1, p2, p3));

    auto points = vmobject->get_points();
    EXPECT_GE(points.size(), 4);
}

// ==================== 3D Mobject Tests ====================

TEST(MeshTest, DefaultConstruction) {
    auto mesh = std::make_shared<Mesh>();
    EXPECT_NE(mesh, nullptr);
}

TEST(SurfaceTest, InheritsFromMesh) {
    auto surface = std::make_shared<Surface>();
    EXPECT_NE(surface, nullptr);
}

TEST(VolumeTest, DefaultConstruction) {
    auto volume = std::make_shared<Volume>();
    EXPECT_NE(volume, nullptr);
}

// ==================== Procedural Mesh Generator Tests ====================

TEST(MeshGeneratorTest, CylinderGeneration) {
    auto cylinder = GPUMesh::create_cylinder(1.0f, 2.0f, 32);
    EXPECT_NE(cylinder, nullptr);
    EXPECT_GT(cylinder->get_num_vertices(), 0);
    EXPECT_GT(cylinder->get_num_triangles(), 0);

    // Verify proper vertex/triangle counts for cylinder
    // 32 segments * 2 rings (sides) + 2 caps with center + ring
    EXPECT_GE(cylinder->get_num_vertices(), 64); // At minimum
}

TEST(MeshGeneratorTest, TorusGeneration) {
    auto torus = GPUMesh::create_torus(1.0f, 0.3f, 32, 24);
    EXPECT_NE(torus, nullptr);
    EXPECT_GT(torus->get_num_vertices(), 0);
    EXPECT_GT(torus->get_num_triangles(), 0);

    // Torus should have (major_segments+1) * (minor_segments+1) vertices
    EXPECT_GE(torus->get_num_vertices(), 33 * 25);
}

TEST(MeshGeneratorTest, ConeGeneration) {
    auto cone = GPUMesh::create_cone(1.0f, 2.0f, 32, false);
    EXPECT_NE(cone, nullptr);
    EXPECT_GT(cone->get_num_vertices(), 0);
    EXPECT_GT(cone->get_num_triangles(), 0);

    // Open-ended cone
    auto cone_open = GPUMesh::create_cone(1.0f, 2.0f, 32, true);
    EXPECT_NE(cone_open, nullptr);
    // Open cone should have fewer vertices (no cap)
    EXPECT_LT(cone_open->get_num_vertices(), cone->get_num_vertices());
}

TEST(MeshGeneratorTest, CapsuleGeneration) {
    auto capsule = GPUMesh::create_capsule(0.5f, 2.0f, 32, 8);
    EXPECT_NE(capsule, nullptr);
    EXPECT_GT(capsule->get_num_vertices(), 0);
    EXPECT_GT(capsule->get_num_triangles(), 0);
}

TEST(MeshGeneratorTest, IcosphereSubdivision) {
    auto ico0 = GPUMesh::create_icosphere(1.0f, 0);
    auto ico1 = GPUMesh::create_icosphere(1.0f, 1);
    auto ico2 = GPUMesh::create_icosphere(1.0f, 2);

    EXPECT_NE(ico0, nullptr);
    EXPECT_NE(ico1, nullptr);
    EXPECT_NE(ico2, nullptr);

    // Icosahedron (level 0) has 12 vertices
    EXPECT_EQ(ico0->get_num_vertices(), 12);

    // Each subdivision multiplies faces by 4
    EXPECT_GT(ico1->get_num_vertices(), ico0->get_num_vertices());
    EXPECT_GT(ico2->get_num_vertices(), ico1->get_num_vertices());
}

TEST(MeshGeneratorTest, ArrowGeneration) {
    auto arrow = GPUMesh::create_arrow(0.05f, 1.0f, 0.15f, 0.3f, 16);
    EXPECT_NE(arrow, nullptr);
    EXPECT_GT(arrow->get_num_vertices(), 0);
    EXPECT_GT(arrow->get_num_triangles(), 0);
}

TEST(MeshGeneratorTest, SphereGeneration) {
    auto sphere = GPUMesh::create_sphere(1.0f, 32);
    EXPECT_NE(sphere, nullptr);
    EXPECT_GT(sphere->get_num_vertices(), 0);
    EXPECT_GT(sphere->get_num_triangles(), 0);

    // UV sphere: (rings+1) * (sectors+1) vertices
    EXPECT_GE(sphere->get_num_vertices(), 33 * 65);
}

TEST(MeshGeneratorTest, CubeGeneration) {
    auto cube = GPUMesh::create_cube(1.0f);
    EXPECT_NE(cube, nullptr);

    // Cube has 24 vertices (4 per face * 6 faces for proper normals)
    EXPECT_EQ(cube->get_num_vertices(), 24);

    // 36 indices = 12 triangles (2 per face * 6 faces)
    EXPECT_EQ(cube->get_num_triangles(), 12);
}

TEST(MeshGeneratorTest, PlaneGeneration) {
    auto plane = GPUMesh::create_plane(2.0f, 2.0f, 4, 4);
    EXPECT_NE(plane, nullptr);

    // Subdivided plane: (subdivisions_x+1) * (subdivisions_y+1) vertices
    EXPECT_EQ(plane->get_num_vertices(), 25);
}

// ==================== Instancing Tests ====================

TEST(InstancedRenderingTest, SetupInstancing) {
    auto mesh = GPUMesh::create_cube(1.0f);
    mesh->setup_instancing(100);

    EXPECT_TRUE(mesh->has_instancing());
    EXPECT_EQ(mesh->get_instance_count(), 0);
}

TEST(InstancedRenderingTest, AddInstances) {
    auto mesh = GPUMesh::create_cube(1.0f);
    mesh->setup_instancing(100);

    for (int i = 0; i < 10; ++i) {
        GPUMesh::InstanceData instance;
        instance.model_matrix = math::Mat4(1.0f);
        instance.model_matrix[3][0] = static_cast<float>(i) * 2.0f;  // Translate X
        instance.color = math::Vec4(1, 1, 1, 1);
        instance.metallic = 0.5f;
        instance.roughness = 0.5f;
        instance.ao = 1.0f;
        mesh->add_instance(instance);
    }

    EXPECT_EQ(mesh->get_instance_count(), 10);
}

TEST(InstancedRenderingTest, ClearInstances) {
    auto mesh = GPUMesh::create_cube(1.0f);
    mesh->setup_instancing(100);

    GPUMesh::InstanceData instance;
    instance.model_matrix = math::Mat4(1.0f);
    instance.color = math::Vec4(1, 1, 1, 1);
    mesh->add_instance(instance);
    mesh->add_instance(instance);
    mesh->add_instance(instance);

    EXPECT_EQ(mesh->get_instance_count(), 3);

    mesh->clear_instances();
    EXPECT_EQ(mesh->get_instance_count(), 0);
}

// ==================== LOD System Tests ====================

TEST(LODSystemTest, AddLODLevel) {
    auto mesh = GPUMesh::create_sphere(1.0f, 32);

    GPUMesh::LOD lod1;
    lod1.distance_threshold = 10.0f;
    mesh->add_lod_level(lod1);

    GPUMesh::LOD lod2;
    lod2.distance_threshold = 25.0f;
    mesh->add_lod_level(lod2);

    // Base mesh + 2 LOD levels
    EXPECT_EQ(mesh->get_lod_count(), 3);
}

TEST(LODSystemTest, DistanceSelection) {
    auto mesh = GPUMesh::create_sphere(1.0f, 32);

    GPUMesh::LOD lod1;
    lod1.distance_threshold = 10.0f;
    mesh->add_lod_level(lod1);

    GPUMesh::LOD lod2;
    lod2.distance_threshold = 25.0f;
    mesh->add_lod_level(lod2);

    // Close distance should select LOD 0 (highest detail)
    EXPECT_EQ(mesh->select_lod(5.0f), 0);

    // Medium distance
    EXPECT_EQ(mesh->select_lod(15.0f), 1);

    // Far distance
    EXPECT_EQ(mesh->select_lod(30.0f), 1);  // Last LOD
}

TEST(LODSystemTest, AutomaticGeneration) {
    auto sphere = GPUMesh::create_sphere(1.0f, 32);

    GPUMesh::LODGenerationConfig config;
    config.distance_thresholds = {10.0f, 25.0f, 50.0f};
    config.target_ratios = {0.5f, 0.25f, 0.1f};

    sphere->generate_lod_levels(config);

    // Should have 3 LOD levels + base
    EXPECT_EQ(sphere->get_lod_count(), 4);

    // Each LOD should have fewer or equal vertices
    size_t prev_vertices = sphere->get_num_vertices();
    for (size_t i = 0; i < 3; ++i) {
        const GPUMesh::LOD* lod = sphere->get_lod(i);
        EXPECT_NE(lod, nullptr);
        EXPECT_LE(lod->vertices.size(), prev_vertices);
        prev_vertices = lod->vertices.size();
    }
}

// ==================== Geomorphing Tests ====================

TEST(GeomorphingTest, StartTransition) {
    auto mesh = GPUMesh::create_sphere(1.0f, 32);

    GPUMesh::LOD lod;
    lod.distance_threshold = 10.0f;
    mesh->add_lod_level(lod);

    mesh->start_lod_transition(1, 0.5f);

    EXPECT_TRUE(mesh->is_transitioning());
    EXPECT_FLOAT_EQ(mesh->get_morph_factor(), 0.0f);
}

TEST(GeomorphingTest, UpdateTransition) {
    auto mesh = GPUMesh::create_sphere(1.0f, 32);

    GPUMesh::LODGenerationConfig config;
    config.distance_thresholds = {10.0f};
    config.target_ratios = {0.5f};
    mesh->generate_lod_levels(config);

    mesh->start_lod_transition(1, 1.0f);  // 1 second transition

    // Update by 0.5 seconds
    mesh->update_lod_transition(0.5f);
    EXPECT_FLOAT_EQ(mesh->get_morph_factor(), 0.5f);
    EXPECT_TRUE(mesh->is_transitioning());

    // Update by another 0.5 seconds - should complete
    mesh->update_lod_transition(0.5f);
    EXPECT_FLOAT_EQ(mesh->get_morph_factor(), 1.0f);
    EXPECT_FALSE(mesh->is_transitioning());
}

TEST(GeomorphingTest, GeomorphVertexStructureSize) {
    // Verify GeomorphVertex is 96 bytes for cache alignment
    EXPECT_EQ(sizeof(GPUMesh::GeomorphVertex), 96);
}

// ==================== Performance Tests ====================

TEST(MobjectPerformance, CreateThousandCircles) {
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::shared_ptr<Circle>> circles;
    circles.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
        circles.push_back(std::make_shared<Circle>(1.0f));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should create 1000 circles in less than 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST(MobjectPerformance, TransformOperations) {
    auto start = std::chrono::high_resolution_clock::now();

    auto circle = std::make_shared<Circle>();

    for (int i = 0; i < 10000; ++i) {
        circle->shift(math::Vec3(0.01f, 0.01f, 0));
        circle->rotate(0.01f, math::Vec3(0, 0, 1));
        circle->scale(1.001f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 10,000 operations should complete in less than 500ms
    EXPECT_LT(duration.count(), 500);
}

// ==================== Thread Safety Tests ====================

TEST(MobjectThreadSafety, ConcurrentCreation) {
    const int num_threads = 4;
    const int circles_per_thread = 100;

    std::vector<std::thread> threads;
    std::vector<std::vector<std::shared_ptr<Circle>>> all_circles(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < circles_per_thread; ++i) {
                all_circles[t].push_back(std::make_shared<Circle>(1.0f + t));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all circles were created
    for (int t = 0; t < num_threads; ++t) {
        EXPECT_EQ(all_circles[t].size(), circles_per_thread);
    }
}

// ==================== Main ====================

#if 0
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
