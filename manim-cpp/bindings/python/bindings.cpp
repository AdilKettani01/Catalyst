#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>

// Include all manim headers
#include "manim/core/types.h"
#include "manim/mobject/mobject.hpp"
#include "manim/mobject/vmobject.hpp"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/mobject/text/text.hpp"
#include "manim/mobject/three_d/mesh.hpp"
#include "manim/mobject/three_d/surface.hpp"
#include "manim/mobject/three_d/volume.hpp"
#include "manim/animation/animation.hpp"
#include "manim/animation/creation.hpp"
#include "manim/animation/fading.hpp"
#include "manim/animation/transform.hpp"
#include "manim/scene/scene.h"
#include "manim/scene/three_d_scene.h"
#include "manim/scene/moving_camera_scene.h"
#include "manim/scene/vector_space_scene.h"
#include "manim/scene/zoomed_scene.h"
#include "manim/utils/bezier.h"
#include "manim/utils/color.h"
#include "manim/utils/rate_functions.h"
#include "manim/cli/config.h"
#include "manim/cli/gpu_detector.h"

namespace py = pybind11;
using namespace manim;

// ==================== Core Types ====================

void bind_core_types(py::module& m) {
    // Vec3 binding with numpy interop
    py::class_<Vec3>(m, "Vec3")
        .def(py::init<>())
        .def(py::init<float, float, float>())
        .def_readwrite("x", &Vec3::x)
        .def_readwrite("y", &Vec3::y)
        .def_readwrite("z", &Vec3::z)
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * float())
        .def(py::self / float())
        .def("__repr__", [](const Vec3& v) {
            return "Vec3(" + std::to_string(v.x) + ", " +
                   std::to_string(v.y) + ", " +
                   std::to_string(v.z) + ")";
        })
        .def("to_numpy", [](const Vec3& v) {
            return py::array_t<float>({3}, {sizeof(float)}, &v.x);
        })
        .def_static("from_numpy", [](py::array_t<float> arr) {
            auto buf = arr.request();
            if (buf.ndim != 1 || buf.shape[0] != 3)
                throw std::runtime_error("Array must be 1D with 3 elements");
            float* ptr = static_cast<float*>(buf.ptr);
            return Vec3(ptr[0], ptr[1], ptr[2]);
        });

    // Vec4 for colors
    py::class_<Vec4>(m, "Vec4")
        .def(py::init<>())
        .def(py::init<float, float, float, float>())
        .def_readwrite("x", &Vec4::x)
        .def_readwrite("y", &Vec4::y)
        .def_readwrite("z", &Vec4::z)
        .def_readwrite("w", &Vec4::w)
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * float())
        .def("to_numpy", [](const Vec4& v) {
            return py::array_t<float>({4}, {sizeof(float)}, &v.x);
        });

    // Mat4 for transformations
    py::class_<Mat4>(m, "Mat4")
        .def(py::init<>())
        .def(py::self * py::self)
        .def("__mul__", [](const Mat4& m, const Vec3& v) {
            Vec4 v4(v.x, v.y, v.z, 1.0f);
            Vec4 result = m * v4;
            return Vec3(result.x, result.y, result.z);
        })
        .def("to_numpy", [](const Mat4& mat) {
            return py::array_t<float>({4, 4}, &mat[0][0]);
        });

    // Constants
    m.attr("ORIGIN") = Vec3(0, 0, 0);
    m.attr("UP") = Vec3(0, 1, 0);
    m.attr("DOWN") = Vec3(0, -1, 0);
    m.attr("LEFT") = Vec3(-1, 0, 0);
    m.attr("RIGHT") = Vec3(1, 0, 0);
    m.attr("IN") = Vec3(0, 0, -1);
    m.attr("OUT") = Vec3(0, 0, 1);
    m.attr("PI") = M_PI;
    m.attr("TAU") = 2 * M_PI;
    m.attr("DEGREES") = M_PI / 180.0;
}

// ==================== Mobject Bindings ====================

void bind_mobjects(py::module& m) {
    // Base Mobject class
    py::class_<Mobject, std::shared_ptr<Mobject>>(m, "Mobject")
        .def(py::init<>())
        .def("add", &Mobject::add, "Add submobjects")
        .def("remove", &Mobject::remove, "Remove submobjects")
        .def("shift", &Mobject::shift, "Shift position")
        .def("scale", &Mobject::scale, "Scale size")
        .def("rotate", &Mobject::rotate, "Rotate")
        .def("move_to", &Mobject::moveTo, "Move to position")
        .def("next_to", &Mobject::nextTo, "Position next to another mobject")
        .def("align_to", &Mobject::alignTo, "Align to another mobject")
        .def("set_color", &Mobject::setColor, "Set color")
        .def("set_opacity", &Mobject::setOpacity, "Set opacity")
        .def("get_center", &Mobject::getCenter, "Get center position")
        .def("get_color", &Mobject::getColor, "Get color")
        .def("copy", &Mobject::copy, "Create a copy")
        .def("update", &Mobject::update, "Update with delta time")
        .def("add_updater", &Mobject::addUpdater, "Add updater function")
        .def("remove_updater", &Mobject::removeUpdater, "Remove updater")
        .def("save_state", &Mobject::saveState, "Save current state")
        .def("restore", &Mobject::restore, "Restore saved state")
        .def("become", &Mobject::become, "Become another mobject")
        .def("match_points", &Mobject::matchPoints, "Match points with another")
        .def("get_family", &Mobject::getFamily, "Get mobject family")
        .def("__repr__", [](const Mobject& m) {
            return "<Mobject at " + std::to_string(reinterpret_cast<uintptr_t>(&m)) + ">";
        });

    // VMobject (vectorized mobject)
    py::class_<VMobject, Mobject, std::shared_ptr<VMobject>>(m, "VMobject")
        .def(py::init<>())
        .def("set_fill", &VMobject::set_fill, "Set fill color and opacity")
        .def("set_stroke", &VMobject::set_stroke, "Set stroke color and width")
        .def("get_points", &VMobject::get_points, "Get bezier points")
        .def("set_points", &VMobject::set_points, "Set bezier points")
        .def("add_cubic_bezier_curve", &VMobject::add_cubic_bezier_curve, "Add bezier curve");

    // ==================== Geometry Mobjects ====================

    // Circle
    py::class_<Circle, VMobject, std::shared_ptr<Circle>>(m, "Circle")
        .def(py::init<float, uint32_t>(),
             py::arg("radius") = 1.0f,
             py::arg("num_segments") = 64,
             "Create a circle")
        .def("set_radius", &Circle::set_radius, "Set circle radius")
        .def("get_radius", &Circle::get_radius, "Get circle radius")
        .def("surround", &Circle::surround,
             py::arg("mobject"),
             py::arg("buffer") = 0.2f,
             "Surround another mobject");

    // Dot (small filled circle)
    py::class_<Dot, Circle, std::shared_ptr<Dot>>(m, "Dot")
        .def(py::init<const math::Vec3&, float>(),
             py::arg("position") = math::Vec3{0.0f},
             py::arg("radius") = 0.08f,
             "Create a dot (small filled circle)");

    m.attr("DEFAULT_DOT_RADIUS") = Dot::DEFAULT_DOT_RADIUS;

    // Ellipse
    py::class_<Ellipse, VMobject, std::shared_ptr<Ellipse>>(m, "Ellipse")
        .def(py::init<float, float, uint32_t>(),
             py::arg("width") = 2.0f,
             py::arg("height") = 1.0f,
             py::arg("num_segments") = 64,
             "Create an ellipse")
        .def("set_width", &Ellipse::set_width, "Set ellipse width")
        .def("set_height", &Ellipse::set_height, "Set ellipse height");

    // Arc
    py::class_<Arc, VMobject, std::shared_ptr<Arc>>(m, "Arc")
        .def(py::init<float, float, float, uint32_t>(),
             py::arg("radius") = 1.0f,
             py::arg("start_angle") = 0.0f,
             py::arg("angle") = math::PI / 2.0f,
             py::arg("num_segments") = 32,
             "Create an arc")
        .def("set_angle", &Arc::set_angle, "Set arc angle")
        .def("get_angle", &Arc::get_angle, "Get arc angle");

    // Annulus (ring)
    py::class_<Annulus, VMobject, std::shared_ptr<Annulus>>(m, "Annulus")
        .def(py::init<float, float, uint32_t>(),
             py::arg("inner_radius") = 0.5f,
             py::arg("outer_radius") = 1.0f,
             py::arg("num_segments") = 64,
             "Create an annulus (ring)");

    // ==================== Text Mobjects ====================

    // Text class (will need to check actual interface)
    // py::class_<Text, VMobject, std::shared_ptr<Text>>(m, "Text")
    //     .def(py::init<const std::string&>(),
    //          py::arg("text"),
    //          "Create text mobject");

    // ==================== 3D Mobjects ====================

    // Mesh
    py::class_<Mesh, Mobject, std::shared_ptr<Mesh>>(m, "Mesh")
        .def(py::init<>(), "Create a mesh");

    // Surface
    py::class_<Surface, Mesh, std::shared_ptr<Surface>>(m, "Surface")
        .def(py::init<>(), "Create a surface");

    // Volume
    py::class_<Volume, Mobject, std::shared_ptr<Volume>>(m, "Volume")
        .def(py::init<>(), "Create a volume");
}

// ==================== Animation Bindings ====================

void bind_animations(py::module& m) {
    // Base Animation class
    py::class_<Animation, std::shared_ptr<Animation>>(m, "Animation")
        .def("get_run_time", &Animation::get_run_time)
        .def("set_run_time", &Animation::set_run_time)
        .def("get_mobject", &Animation::get_mobject)
        .def("interpolate", &Animation::interpolate)
        .def("begin", &Animation::begin)
        .def("finish", &Animation::finish)
        .def("can_run_on_gpu", &Animation::can_run_on_gpu,
             "Check if animation can run on GPU");

    // ==================== Creation Animations ====================

    // ShowCreation - draws mobject progressively
    py::class_<ShowCreation, Animation, std::shared_ptr<ShowCreation>>(m, "ShowCreation")
        .def(py::init<std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Show creation animation");

    // Uncreate (reverse of ShowCreation)
    py::class_<Uncreate, ShowCreation, std::shared_ptr<Uncreate>>(m, "Uncreate")
        .def(py::init<std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Uncreate animation");

    // DrawBorderThenFill
    py::class_<DrawBorderThenFill, Animation, std::shared_ptr<DrawBorderThenFill>>(m, "DrawBorderThenFill")
        .def(py::init<std::shared_ptr<Mobject>, float, float, const math::Vec4&>(),
             py::arg("mobject"),
             py::arg("run_time") = 2.0f,
             py::arg("stroke_width") = 2.0f,
             py::arg("stroke_color") = math::Vec4{1,1,1,1},
             "Draw border then fill");

    // Write (for text)
    py::class_<Write, DrawBorderThenFill, std::shared_ptr<Write>>(m, "Write")
        .def(py::init<std::shared_ptr<Mobject>, float, float, const math::Vec4&>(),
             py::arg("mobject"),
             py::arg("run_time") = 2.0f,
             py::arg("stroke_width") = 2.0f,
             py::arg("stroke_color") = math::Vec4{1,1,1,1},
             "Write animation for text");

    // Unwrite
    py::class_<Unwrite, Write, std::shared_ptr<Unwrite>>(m, "Unwrite")
        .def(py::init<std::shared_ptr<Mobject>, float, float, const math::Vec4&>(),
             py::arg("mobject"),
             py::arg("run_time") = 2.0f,
             py::arg("stroke_width") = 2.0f,
             py::arg("stroke_color") = math::Vec4{1,1,1,1},
             "Unwrite animation");

    // ShowPartial
    py::class_<ShowPartial, Animation, std::shared_ptr<ShowPartial>>(m, "ShowPartial")
        .def(py::init<std::shared_ptr<Mobject>, float, float, float>(),
             py::arg("mobject"),
             py::arg("start_proportion") = 0.0f,
             py::arg("end_proportion") = 1.0f,
             py::arg("run_time") = 1.0f,
             "Show partial mobject");

    // ShowIncreasingSubsets
    py::class_<ShowIncreasingSubsets, Animation, std::shared_ptr<ShowIncreasingSubsets>>(m, "ShowIncreasingSubsets")
        .def(py::init<std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Show increasing subsets");

    // ShowSubmobjectsOneByOne
    py::class_<ShowSubmobjectsOneByOne, ShowIncreasingSubsets, std::shared_ptr<ShowSubmobjectsOneByOne>>(m, "ShowSubmobjectsOneByOne")
        .def(py::init<std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Show submobjects one by one");

    // ==================== Fading Animations ====================

    // FadeIn
    py::class_<FadeIn, Animation, std::shared_ptr<FadeIn>>(m, "FadeIn")
        .def(py::init<std::shared_ptr<Mobject>, float, float, const math::Vec3&>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             py::arg("shift_amount") = 0.0f,
             py::arg("shift_direction") = math::Vec3{0, 0, 0},
             "Fade in animation");

    // FadeOut
    py::class_<FadeOut, FadeIn, std::shared_ptr<FadeOut>>(m, "FadeOut")
        .def(py::init<std::shared_ptr<Mobject>, float, float, const math::Vec3&>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             py::arg("shift_amount") = 0.0f,
             py::arg("shift_direction") = math::Vec3{0, 0, 0},
             "Fade out animation");

    // FadeInFromPoint
    py::class_<FadeInFromPoint, FadeIn, std::shared_ptr<FadeInFromPoint>>(m, "FadeInFromPoint")
        .def(py::init<std::shared_ptr<Mobject>, const math::Vec3&, float>(),
             py::arg("mobject"),
             py::arg("point"),
             py::arg("run_time") = 1.0f,
             "Fade in from point");

    // FadeOutToPoint
    py::class_<FadeOutToPoint, FadeOut, std::shared_ptr<FadeOutToPoint>>(m, "FadeOutToPoint")
        .def(py::init<std::shared_ptr<Mobject>, const math::Vec3&, float>(),
             py::arg("mobject"),
             py::arg("point"),
             py::arg("run_time") = 1.0f,
             "Fade out to point");

    // FadeInFromLarge
    py::class_<FadeInFromLarge, FadeIn, std::shared_ptr<FadeInFromLarge>>(m, "FadeInFromLarge")
        .def(py::init<std::shared_ptr<Mobject>, float, float>(),
             py::arg("mobject"),
             py::arg("scale_factor") = 2.0f,
             py::arg("run_time") = 1.0f,
             "Fade in from large scale");

    // FadeTransform
    py::class_<FadeTransform, Animation, std::shared_ptr<FadeTransform>>(m, "FadeTransform")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("target_mobject"),
             py::arg("run_time") = 1.0f,
             "Fade transform");

    // ==================== Transform Animations ====================

    // Transform
    py::class_<Transform, Animation, std::shared_ptr<Transform>>(m, "Transform")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("target_mobject"),
             py::arg("run_time") = 1.0f,
             "Transform animation - morphs one mobject into another");

    // ReplacementTransform
    py::class_<ReplacementTransform, Transform, std::shared_ptr<ReplacementTransform>>(m, "ReplacementTransform")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("target_mobject"),
             py::arg("run_time") = 1.0f,
             "Replacement transform");

    // TransformFromCopy
    py::class_<TransformFromCopy, Transform, std::shared_ptr<TransformFromCopy>>(m, "TransformFromCopy")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("target_mobject"),
             py::arg("run_time") = 1.0f,
             "Transform from copy");

    // ClockwiseTransform
    py::class_<ClockwiseTransform, Transform, std::shared_ptr<ClockwiseTransform>>(m, "ClockwiseTransform")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("target_mobject"),
             py::arg("run_time") = 1.0f,
             "Clockwise transform");

    // CounterclockwiseTransform
    py::class_<CounterclockwiseTransform, Transform, std::shared_ptr<CounterclockwiseTransform>>(m, "CounterclockwiseTransform")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("target_mobject"),
             py::arg("run_time") = 1.0f,
             "Counter-clockwise transform");

    // MoveToTarget
    py::class_<MoveToTarget, Transform, std::shared_ptr<MoveToTarget>>(m, "MoveToTarget")
        .def(py::init<std::shared_ptr<Mobject>, float>(),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Move to target");

    // ApplyFunction
    py::class_<ApplyFunction, Transform, std::shared_ptr<ApplyFunction>>(m, "ApplyFunction")
        .def(py::init<ApplyFunction::TransformFunc, std::shared_ptr<Mobject>, float>(),
             py::arg("func"),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Apply function animation");

    // ApplyMatrix
    py::class_<ApplyMatrix, ApplyFunction, std::shared_ptr<ApplyMatrix>>(m, "ApplyMatrix")
        .def(py::init<const math::Mat4&, std::shared_ptr<Mobject>, float>(),
             py::arg("matrix"),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Apply matrix animation");

    // ApplyComplexFunction
    py::class_<ApplyComplexFunction, ApplyFunction, std::shared_ptr<ApplyComplexFunction>>(m, "ApplyComplexFunction")
        .def(py::init<ApplyComplexFunction::ComplexFunc, std::shared_ptr<Mobject>, float>(),
             py::arg("func"),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             "Apply complex function");
}

// ==================== Scene Bindings ====================

void bind_scenes(py::module& m) {
    // Base Scene class
    py::class_<Scene, std::shared_ptr<Scene>>(m, "Scene")
        .def(py::init<>())
        .def("setup", &Scene::setup, "Setup scene")
        .def("construct", &Scene::construct, "Construct scene (override in subclass)")
        .def("tear_down", &Scene::tearDown, "Tear down scene")
        .def("render", &Scene::render, "Render scene", py::arg("preview") = false)
        .def("add", py::overload_cast<std::shared_ptr<Mobject>>(&Scene::add),
             "Add mobject to scene")
        .def("remove", py::overload_cast<std::shared_ptr<Mobject>>(&Scene::remove),
             "Remove mobject from scene")
        .def("play", py::overload_cast<std::shared_ptr<Animation>>(&Scene::play),
             "Play animation")
        .def("wait", &Scene::wait, "Wait for duration",
             py::arg("duration") = 1.0,
             py::arg("stop_condition") = nullptr)
        .def("clear", &Scene::clear, "Clear all mobjects")
        .def("get_mobjects", &Scene::getMobjects, "Get all mobjects")
        .def("get_time", &Scene::getTime, "Get current time")
        .def("add_updater", &Scene::addUpdater, "Add scene updater");

    // ThreeDScene
    py::class_<ThreeDScene, Scene, std::shared_ptr<ThreeDScene>>(m, "ThreeDScene")
        .def(py::init<>())
        .def("set_camera_orientation", &ThreeDScene::setCameraOrientation,
             "Set 3D camera orientation",
             py::arg("phi") = py::none(),
             py::arg("theta") = py::none(),
             py::arg("gamma") = py::none(),
             py::arg("zoom") = py::none(),
             py::arg("focal_distance") = py::none(),
             py::arg("frame_center") = py::none())
        .def("begin_ambient_camera_rotation", &ThreeDScene::beginAmbientCameraRotation,
             "Begin ambient rotation",
             py::arg("rate") = 0.02,
             py::arg("about") = "theta")
        .def("stop_ambient_camera_rotation", &ThreeDScene::stopAmbientCameraRotation,
             "Stop ambient rotation",
             py::arg("about") = "theta")
        .def("move_camera_to", &ThreeDScene::moveCameraTo,
             "Move camera to position");

    // GPU3DScene - NEW GPU FEATURES!
    py::class_<GPU3DScene, ThreeDScene, std::shared_ptr<GPU3DScene>>(m, "GPU3DScene")
        .def(py::init<>())
        .def("setup_deferred_pipeline", &GPU3DScene::setupDeferredPipeline,
             "Setup deferred rendering")
        .def("initialize_pbr_materials", &GPU3DScene::initializePBRMaterials,
             "Initialize PBR materials")
        .def("setup_ray_tracing", &GPU3DScene::setupRayTracing,
             "Setup ray tracing")
        .def("add_light", &GPU3DScene::addLight,
             "Add light to scene")
        .def("remove_light", &GPU3DScene::removeLight,
             "Remove light by index")
        .def("set_environment_map", &GPU3DScene::setEnvironmentMap,
             "Set environment map")
        .def("render_with_pbr", &GPU3DScene::renderWithPBR,
             "Render with physically-based rendering")
        .def("compute_global_illumination", &GPU3DScene::computeGlobalIllumination,
             "Compute global illumination")
        .def("render_volumetrics", &GPU3DScene::renderVolumetrics,
             "Render volumetric effects")
        .def("ray_trace_scene", &GPU3DScene::rayTraceScene,
             "Ray trace the scene")
        .def("frustum_cull_gpu", &GPU3DScene::frustumCullGPU,
             "GPU frustum culling")
        .def("occlusion_cull_gpu", &GPU3DScene::occlusionCullGPU,
             "GPU occlusion culling")
        .def("build_acceleration_structures", &GPU3DScene::buildAccelerationStructures,
             "Build ray tracing acceleration structures")
        .def("apply_ssao", &GPU3DScene::applySSAO,
             "Apply screen-space ambient occlusion")
        .def("apply_ssr", &GPU3DScene::applySSR,
             "Apply screen-space reflections")
        .def("apply_bloom", &GPU3DScene::applyBloom,
             "Apply bloom effect")
        .def("apply_tone_mapping", &GPU3DScene::applyToneMapping,
             "Apply tone mapping");

    // Light binding for GPU scenes
    py::class_<Light>(m, "Light")
        .def(py::init<>())
        .def_readwrite("position", &Light::position)
        .def_readwrite("direction", &Light::direction)
        .def_readwrite("color", &Light::color)
        .def_readwrite("intensity", &Light::intensity)
        .def_readwrite("radius", &Light::radius)
        .def_readwrite("casts_shadows", &Light::castsShadows);

    // LightType enum
    py::enum_<LightType>(m, "LightType")
        .value("POINT", LightType::POINT)
        .value("DIRECTIONAL", LightType::DIRECTIONAL)
        .value("SPOT", LightType::SPOT)
        .value("AREA", LightType::AREA)
        .value("AMBIENT", LightType::AMBIENT);

    // MovingCameraScene
    py::class_<MovingCameraScene, Scene, std::shared_ptr<MovingCameraScene>>(m, "MovingCameraScene")
        .def(py::init<>())
        .def("zoom_to_mobject", &MovingCameraScene::zoomToMobject,
             "Zoom to mobject")
        .def("pan_to", &MovingCameraScene::panTo,
             "Pan camera to position")
        .def("save_camera_state", &MovingCameraScene::saveCameraState,
             "Save camera state")
        .def("restore_camera_state", &MovingCameraScene::restoreCameraState,
             "Restore camera state");

    // VectorSpaceScene
    py::class_<VectorSpaceScene, Scene, std::shared_ptr<VectorSpaceScene>>(m, "VectorSpaceScene")
        .def(py::init<>(),
             "Scene for vector space visualization");

    // ZoomedScene
    py::class_<ZoomedScene, MovingCameraScene, std::shared_ptr<ZoomedScene>>(m, "ZoomedScene")
        .def(py::init<>(),
             "Scene with zoomed-in region");
}

// ==================== Utility Bindings ====================

void bind_utilities(py::module& m) {
    // Bezier utilities
    auto bezier_module = m.def_submodule("bezier", "Bezier curve utilities");
    bezier_module.def("evaluate", &Bezier::evaluate,
        "Evaluate Bezier curve at parameter t");
    bezier_module.def("derivative", &Bezier::derivative,
        "Get derivative at t");
    bezier_module.def("tangent", &Bezier::tangent,
        "Get tangent at t");
    bezier_module.def("length", &Bezier::length,
        "Calculate curve length");

    // Color utilities
    auto color_module = m.def_submodule("color", "Color utilities");
    color_module.def("rgb_to_hsv", &Color::rgbToHsv,
        "Convert RGB to HSV");
    color_module.def("hsv_to_rgb", &Color::hsvToRgb,
        "Convert HSV to RGB");
    color_module.def("interpolate_color", &Color::interpolateColor,
        "Interpolate between colors");

    // Rate functions
    auto rate_module = m.def_submodule("rate_functions", "Rate functions");

    // Standard rate functions
    rate_module.def("linear", &RateFunctions::linear, "Linear interpolation");
    rate_module.def("smooth", &RateFunctions::smooth, "Smooth step");
    rate_module.def("smoother_step", &RateFunctions::smootherStep, "Smoother step");
    rate_module.def("rush_into", &RateFunctions::rushInto, "Rush into");
    rate_module.def("rush_from", &RateFunctions::rushFrom, "Rush from");
    rate_module.def("slow_into", &RateFunctions::slowInto, "Slow into");
    rate_module.def("double_smooth", &RateFunctions::doubleSmooth, "Double smooth");
    rate_module.def("there_and_back", &RateFunctions::thereAndBack, "There and back");
    rate_module.def("there_and_back_with_pause", &RateFunctions::thereAndBackWithPause, "There and back with pause");
    rate_module.def("running_start", &RateFunctions::runningStart, "Running start");
    rate_module.def("wiggle", &RateFunctions::wiggle, "Wiggle");
    rate_module.def("exponential_decay", &RateFunctions::exponentialDecay, "Exponential decay");

    // Sine easing
    rate_module.def("ease_in_sine", &RateFunctions::easeInSine, "Ease in sine");
    rate_module.def("ease_out_sine", &RateFunctions::easeOutSine, "Ease out sine");
    rate_module.def("ease_in_out_sine", &RateFunctions::easeInOutSine, "Ease in-out sine");

    // Quadratic easing
    rate_module.def("ease_in_quad", &RateFunctions::easeInQuad, "Ease in quadratic");
    rate_module.def("ease_out_quad", &RateFunctions::easeOutQuad, "Ease out quadratic");
    rate_module.def("ease_in_out_quad", &RateFunctions::easeInOutQuad, "Ease in-out quadratic");

    // Cubic easing
    rate_module.def("ease_in_cubic", &RateFunctions::easeInCubic, "Ease in cubic");
    rate_module.def("ease_out_cubic", &RateFunctions::easeOutCubic, "Ease out cubic");
    rate_module.def("ease_in_out_cubic", &RateFunctions::easeInOutCubic, "Ease in-out cubic");

    // Exponential easing
    rate_module.def("ease_in_expo", &RateFunctions::easeInExpo, "Ease in exponential");
    rate_module.def("ease_out_expo", &RateFunctions::easeOutExpo, "Ease out exponential");
    rate_module.def("ease_in_out_expo", &RateFunctions::easeInOutExpo, "Ease in-out exponential");

    // Elastic easing
    rate_module.def("ease_in_elastic", &RateFunctions::easeInElastic, "Ease in elastic");
    rate_module.def("ease_out_elastic", &RateFunctions::easeOutElastic, "Ease out elastic");
    rate_module.def("ease_in_out_elastic", &RateFunctions::easeInOutElastic, "Ease in-out elastic");

    // Batch evaluation
    rate_module.def("batch_evaluate", &RateFunctions::batchEvaluate,
        py::arg("func"),
        py::arg("t_values"),
        py::arg("use_gpu") = true,
        "Batch evaluate rate function on GPU");

    // Get rate function by name
    rate_module.def("get_rate_function", &RateFunctions::getRateFunction,
        py::arg("name"),
        "Get rate function by name");
}

// ==================== Configuration Bindings ====================

void bind_config(py::module& m) {
    // GPU Backend enum
    py::enum_<GPUBackend>(m, "GPUBackend")
        .value("AUTO", GPUBackend::AUTO)
        .value("VULKAN", GPUBackend::VULKAN)
        .value("OPENGL", GPUBackend::OPENGL)
        .value("DIRECTX12", GPUBackend::DIRECTX12)
        .value("METAL", GPUBackend::METAL)
        .value("CUDA", GPUBackend::CUDA)
        .value("OPENCL", GPUBackend::OPENCL);

    // Quality Preset enum
    py::enum_<QualityPreset>(m, "QualityPreset")
        .value("LOW", QualityPreset::LOW)
        .value("MEDIUM", QualityPreset::MEDIUM)
        .value("HIGH", QualityPreset::HIGH)
        .value("ULTRA", QualityPreset::ULTRA)
        .value("CUSTOM", QualityPreset::CUSTOM);

    // RenderConfig
    py::class_<RenderConfig>(m, "RenderConfig")
        .def(py::init<>())
        .def_readwrite("width", &RenderConfig::width)
        .def_readwrite("height", &RenderConfig::height)
        .def_readwrite("frame_rate", &RenderConfig::frameRate)
        .def_readwrite("quality", &RenderConfig::quality)
        .def_readwrite("backend", &RenderConfig::backend)
        .def_readwrite("enable_ray_tracing", &RenderConfig::enableRayTracing)
        .def_readwrite("enable_compute", &RenderConfig::enableCompute)
        .def_readwrite("enable_gi", &RenderConfig::enableGI)
        .def_readwrite("enable_shadows", &RenderConfig::enableShadows)
        .def_readwrite("gpu_threshold", &RenderConfig::gpuThreshold)
        .def_readwrite("cpu_threads", &RenderConfig::cpuThreads)
        .def("apply_quality_preset", &RenderConfig::applyQualityPreset,
             "Apply quality preset")
        .def("validate", &RenderConfig::validate,
             "Validate configuration")
        .def("__repr__", &RenderConfig::toString);

    // GPU Detector
    py::class_<GPUDetector>(m, "GPUDetector")
        .def(py::init<>())
        .def("detect_gpus", &GPUDetector::detectGPUs,
             "Detect all GPUs")
        .def("check_vulkan_support", &GPUDetector::checkVulkanSupport,
             "Check Vulkan support")
        .def("benchmark_gpu", &GPUDetector::benchmarkGPU,
             "Benchmark GPU")
        .def("recommend_settings", &GPUDetector::recommendSettings,
             "Get recommended settings")
        .def("print_system_report", &GPUDetector::printSystemReport,
             "Print system report")
        .def("get_gpus", &GPUDetector::getGPUs,
             "Get detected GPUs");

    // Global config accessor
    m.def("get_config", &getGlobalConfig,
          "Get global configuration",
          py::return_value_policy::reference);
}

// ==================== Main Module ====================

PYBIND11_MODULE(manim_cpp, m) {
    m.doc() = "Manim C++ - GPU-accelerated mathematical animation engine";

    // Bind all components
    bind_core_types(m);
    bind_mobjects(m);
    bind_animations(m);
    bind_scenes(m);
    bind_utilities(m);
    bind_config(m);

    // Version info
    m.attr("__version__") = "1.0.0";
    m.attr("__gpu_enabled__") = true;

    // Convenience functions
    m.def("check_gpu_available", []() {
        return GPUUtils::isGPUAvailable();
    }, "Check if GPU is available");

    m.def("get_gpu_info", []() {
        auto& detector = getGPUDetector();
        detector.detectGPUs();
        return detector.getGPUs();
    }, "Get GPU information");
}
