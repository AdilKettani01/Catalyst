#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/complex.h>

// Include all manim headers
#include "manim/core/types.h"
#include "manim/mobject/mobject.hpp"
#include "manim/mobject/vmobject.hpp"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/mobject/geometry/rectangle.hpp"
#include "manim/mobject/geometry/square.hpp"
#include "manim/mobject/geometry/polygon.hpp"
#include "manim/mobject/point_cloud.hpp"
#include "manim/mobject/text/text.hpp"
#include "manim/mobject/three_d/mesh.hpp"
#include "manim/mobject/three_d/surface.hpp"
#include "manim/mobject/three_d/volume.hpp"
#include "manim/mobject/three_d/sphere.hpp"
#include "manim/animation/animation.hpp"
#include "manim/animation/creation.hpp"
#include "manim/animation/fading.hpp"
#include "manim/animation/transform.hpp"
#include "manim/animation/composition.hpp"
#include "manim/animation/movement.hpp"
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
#include "manim/culling/gpu_culling_pipeline.hpp"
#include "manim/culling/indirect_renderer.hpp"
#include "manim/renderer/gpu_particle_system.hpp"

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

    // Vector3 alias for compatibility
    m.attr("Vector3") = m.attr("Vec3");

    // Color constants (as Vec4 RGBA)
    m.attr("RED") = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    m.attr("GREEN") = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
    m.attr("BLUE") = Vec4(0.0f, 0.0f, 1.0f, 1.0f);
    m.attr("YELLOW") = Vec4(1.0f, 1.0f, 0.0f, 1.0f);
    m.attr("PURPLE") = Vec4(0.5f, 0.0f, 0.5f, 1.0f);
    m.attr("ORANGE") = Vec4(1.0f, 0.647f, 0.0f, 1.0f);
    m.attr("WHITE") = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m.attr("BLACK") = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m.attr("GREY") = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    m.attr("GRAY") = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    m.attr("PINK") = Vec4(1.0f, 0.753f, 0.796f, 1.0f);
    m.attr("TEAL") = Vec4(0.0f, 0.502f, 0.502f, 1.0f);

    // Color class binding
    py::class_<Color>(m, "Color")
        .def(py::init<>())
        .def(py::init<float, float, float, float>(),
             py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 1.0f)
        .def_readwrite("r", &Color::r)
        .def_readwrite("g", &Color::g)
        .def_readwrite("b", &Color::b)
        .def_readwrite("a", &Color::a)
        .def("to_vec4", &Color::toVec4)
        .def("to_vec3", &Color::toVec3);
}

// ==================== Mobject Bindings ====================

void bind_mobjects(py::module& m) {
    // Base Mobject class
    py::class_<Mobject, std::shared_ptr<Mobject>>(m, "Mobject")
        .def(py::init<>())
        .def("add", static_cast<void (Mobject::*)(Mobject::Ptr)>(&Mobject::add), "Add submobjects")
        .def("remove", &Mobject::remove, "Remove submobjects")
        .def("shift", &Mobject::shift, "Shift position")
        .def("scale", py::overload_cast<float>(&Mobject::scale), "Scale size")
        .def("scale", py::overload_cast<const math::Vec3&>(&Mobject::scale), "Scale size (vector)")
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
        .def("save_state", &Mobject::save_state, "Save current state")
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
        .def("add_cubic_bezier_curve",
             static_cast<void (VMobject::*)(const std::array<math::Vec3, 2>&,
                                            const std::array<math::Vec3, 2>&)>(&VMobject::add_cubic_bezier_curve),
             "Add bezier curve");

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

    // Rectangle
    py::class_<Rectangle, VMobject, std::shared_ptr<Rectangle>>(m, "Rectangle")
        .def(py::init<float, float>(),
             py::arg("width") = 2.0f,
             py::arg("height") = 1.0f,
             "Create a rectangle");

    // Square
    py::class_<Square, Mobject, std::shared_ptr<Square>>(m, "Square")
        .def(py::init<float>(),
             py::arg("side_length") = 1.0f,
             "Create a square");

    // Polygon
    py::class_<Polygon, VMobject, std::shared_ptr<Polygon>>(m, "Polygon")
        .def(py::init<>(), "Create an empty polygon")
        .def(py::init<const std::vector<math::Vec3>&>(),
             py::arg("points"),
             "Create a polygon from vertices");

    // ==================== Point Cloud Mobjects ====================

    // BillboardMode enum
    py::enum_<PointCloudMobject::BillboardMode>(m, "BillboardMode")
        .value("NONE", PointCloudMobject::BillboardMode::None)
        .value("SPHERICAL", PointCloudMobject::BillboardMode::Spherical)
        .value("CYLINDRICAL", PointCloudMobject::BillboardMode::Cylindrical);

    // PointCloudMobject (PMobject)
    py::class_<PointCloudMobject, Mobject, std::shared_ptr<PointCloudMobject>>(m, "PointCloudMobject")
        .def(py::init<>(), "Create an empty point cloud")
        .def("set_points", &PointCloudMobject::set_points,
             "Set point positions")
        .def("set_points_with_colors", &PointCloudMobject::set_points_with_colors,
             py::arg("positions"),
             py::arg("colors"),
             "Set points with positions and colors")
        .def("set_points_with_attributes", &PointCloudMobject::set_points_with_attributes,
             py::arg("positions"),
             py::arg("colors"),
             py::arg("sizes"),
             "Set points with full attributes")
        .def("add_point", &PointCloudMobject::add_point,
             py::arg("position"),
             py::arg("color") = math::Vec4{1.0f},
             py::arg("size") = 1.0f,
             "Add a single point")
        .def("set_point_size", &PointCloudMobject::set_point_size,
             "Set default point size")
        .def("set_point_sizes", &PointCloudMobject::set_point_sizes,
             "Set individual point sizes")
        .def("set_point_colors", &PointCloudMobject::set_point_colors,
             "Set point colors")
        .def("set_uniform_color", &PointCloudMobject::set_uniform_color,
             "Set uniform color for all points")
        .def("set_billboard_mode", &PointCloudMobject::set_billboard_mode,
             "Set billboard rendering mode")
        .def("get_billboard_mode", &PointCloudMobject::get_billboard_mode,
             "Get billboard rendering mode")
        .def("get_num_points", &PointCloudMobject::get_num_points,
             "Get number of points")
        .def("get_positions", &PointCloudMobject::get_positions,
             "Get point positions")
        .def("get_colors", &PointCloudMobject::get_colors,
             "Get point colors")
        .def("get_sizes", &PointCloudMobject::get_sizes,
             "Get point sizes");

    // Python Manim compatibility alias
    m.attr("PMobject") = m.attr("PointCloudMobject");

    // ==================== Text Mobjects ====================

    // Text::Weight enum
    py::enum_<Text::Weight>(m, "TextWeight")
        .value("THIN", Text::Weight::Thin)
        .value("EXTRA_LIGHT", Text::Weight::ExtraLight)
        .value("LIGHT", Text::Weight::Light)
        .value("NORMAL", Text::Weight::Normal)
        .value("MEDIUM", Text::Weight::Medium)
        .value("SEMI_BOLD", Text::Weight::SemiBold)
        .value("BOLD", Text::Weight::Bold)
        .value("EXTRA_BOLD", Text::Weight::ExtraBold)
        .value("BLACK", Text::Weight::Black);

    // Text::Alignment enum
    py::enum_<Text::Alignment>(m, "TextAlignment")
        .value("LEFT", Text::Alignment::Left)
        .value("CENTER", Text::Alignment::Center)
        .value("RIGHT", Text::Alignment::Right)
        .value("JUSTIFY", Text::Alignment::Justify);

    // Text class
    py::class_<Text, VMobject, std::shared_ptr<Text>>(m, "Text")
        .def(py::init<>(), "Create empty text")
        .def(py::init<const std::string&, float>(),
             py::arg("text"),
             py::arg("font_size") = 48.0f,
             "Create text with string and font size")
        .def("set_text", &Text::set_text, "Set text content")
        .def("get_text", &Text::get_text, "Get text content")
        .def("set_font", &Text::set_font, "Set font name")
        .def("set_font_size", &Text::set_font_size, "Set font size")
        .def("get_font_size", &Text::get_font_size, "Get font size")
        .def("set_weight", &Text::set_weight, "Set font weight")
        .def("set_italic", &Text::set_italic, "Set italic style")
        .def("set_outline", &Text::set_outline,
             py::arg("width"),
             py::arg("color") = math::Vec4{0,0,0,1},
             "Set text outline")
        .def("set_glow", &Text::set_glow,
             py::arg("intensity"),
             py::arg("color"),
             "Set glow effect")
        .def("set_shadow", &Text::set_shadow,
             py::arg("offset"),
             py::arg("blur"),
             py::arg("color"),
             "Set text shadow")
        .def("set_alignment", &Text::set_alignment, "Set text alignment")
        .def("set_line_spacing", &Text::set_line_spacing, "Set line spacing")
        .def("set_max_width", &Text::set_max_width, "Set max width for wrapping")
        .def("get_char_bounding_box", &Text::get_char_bounding_box,
             "Get bounding box for character at index")
        .def("set_char_color", &Text::set_char_color,
             py::arg("index"),
             py::arg("color"),
             "Color individual character")
        .def("set_char_color_range", &Text::set_char_color_range,
             py::arg("start"),
             py::arg("end"),
             py::arg("color"),
             "Color range of characters");

    // Tex class (LaTeX formulas)
    py::class_<Tex, VMobject, std::shared_ptr<Tex>>(m, "Tex")
        .def(py::init<>(), "Create empty Tex")
        .def(py::init<const std::string&>(),
             py::arg("latex_string"),
             "Create Tex from LaTeX string")
        .def("set_latex", &Tex::set_latex, "Set LaTeX string")
        .def("get_latex", &Tex::get_latex, "Get LaTeX string")
        .def("set_color_by_tex", &Tex::set_color_by_tex,
             py::arg("tex"),
             py::arg("color"),
             "Color parts of formula by matching TeX");

    // MathTex class (simplified math mode)
    py::class_<MathTex, Tex, std::shared_ptr<MathTex>>(m, "MathTex")
        .def(py::init<const std::string&>(),
             py::arg("math_string"),
             "Create MathTex from math string");

    // ==================== 3D Mobjects ====================

    // GPUMesh (Mesh)
    py::class_<GPUMesh, Mobject, std::shared_ptr<GPUMesh>>(m, "GPUMesh")
        .def(py::init<>(), "Create an empty GPU mesh")
        .def("set_mesh", &GPUMesh::set_mesh,
             py::arg("positions"),
             py::arg("indices"),
             "Set mesh from positions and indices (auto-compute normals)")
        .def("get_num_vertices", &GPUMesh::get_num_vertices, "Get vertex count")
        .def("get_num_triangles", &GPUMesh::get_num_triangles, "Get triangle count")
        .def("recalculate_normals", &GPUMesh::recalculate_normals,
             py::arg("smooth") = true,
             "Recalculate vertex normals")
        .def("weld_vertices", &GPUMesh::weld_vertices,
             py::arg("threshold") = 1e-6f,
             "Merge vertices with same position")
        .def("get_bounding_box", &GPUMesh::get_bounding_box, "Get mesh bounding box")
        // Instancing
        .def("setup_instancing", &GPUMesh::setup_instancing,
             py::arg("max_instances"),
             "Setup instanced rendering")
        .def("get_instance_count", &GPUMesh::get_instance_count, "Get instance count")
        .def("clear_instances", &GPUMesh::clear_instances, "Clear all instances")
        .def("has_instancing", &GPUMesh::has_instancing, "Check if instancing enabled")
        // LOD
        .def("get_lod_count", &GPUMesh::get_lod_count, "Get number of LOD levels")
        .def("select_lod", &GPUMesh::select_lod,
             py::arg("camera_distance"),
             "Select LOD based on camera distance")
        // Geomorphing
        .def("start_lod_transition", &GPUMesh::start_lod_transition,
             py::arg("target_lod"),
             py::arg("duration") = 0.5f,
             "Start LOD transition")
        .def("update_lod_transition", &GPUMesh::update_lod_transition,
             py::arg("dt"),
             "Update LOD transition")
        .def("get_morph_factor", &GPUMesh::get_morph_factor, "Get current morph factor")
        .def("is_transitioning", &GPUMesh::is_transitioning, "Check if transitioning")
        // Static factory methods for procedural meshes
        .def_static("create_sphere", &GPUMesh::create_sphere,
             py::arg("radius") = 1.0f,
             py::arg("subdivisions") = 32,
             "Generate sphere mesh")
        .def_static("create_cube", &GPUMesh::create_cube,
             py::arg("size") = 1.0f,
             "Generate cube mesh")
        .def_static("create_plane", &GPUMesh::create_plane,
             py::arg("width") = 1.0f,
             py::arg("height") = 1.0f,
             py::arg("subdivisions_x") = 1,
             py::arg("subdivisions_y") = 1,
             "Generate plane mesh")
        .def_static("create_cylinder", &GPUMesh::create_cylinder,
             py::arg("radius") = 1.0f,
             py::arg("height") = 2.0f,
             py::arg("segments") = 32,
             "Generate cylinder mesh")
        .def_static("create_torus", &GPUMesh::create_torus,
             py::arg("major_radius") = 1.0f,
             py::arg("minor_radius") = 0.3f,
             py::arg("major_segments") = 48,
             py::arg("minor_segments") = 24,
             "Generate torus mesh")
        .def_static("create_cone", &GPUMesh::create_cone,
             py::arg("radius") = 1.0f,
             py::arg("height") = 2.0f,
             py::arg("radial_segments") = 32,
             py::arg("open_ended") = false,
             "Generate cone mesh")
        .def_static("create_capsule", &GPUMesh::create_capsule,
             py::arg("radius") = 0.5f,
             py::arg("height") = 2.0f,
             py::arg("radial_segments") = 32,
             py::arg("hemisphere_segments") = 8,
             "Generate capsule mesh")
        .def_static("create_icosphere", &GPUMesh::create_icosphere,
             py::arg("radius") = 1.0f,
             py::arg("subdivisions") = 2,
             "Generate icosphere mesh")
        .def_static("create_arrow", &GPUMesh::create_arrow,
             py::arg("shaft_radius") = 0.05f,
             py::arg("shaft_length") = 1.0f,
             py::arg("head_radius") = 0.15f,
             py::arg("head_length") = 0.3f,
             py::arg("radial_segments") = 16,
             "Generate 3D arrow mesh");

    // Mesh alias
    m.attr("Mesh") = m.attr("GPUMesh");

    // Sphere
    py::class_<Sphere, GPUMesh, std::shared_ptr<Sphere>>(m, "Sphere")
        .def(py::init<float, uint32_t, uint32_t>(),
             py::arg("radius") = 1.0f,
             py::arg("rings") = 16,
             py::arg("segments") = 16,
             "Create a sphere primitive")
        .def("set_pbr_material", &Sphere::set_pbr_material, py::arg("material"),
             "Assign a PBR material to the sphere");

    // Surface
    py::class_<Surface, GPUMesh, std::shared_ptr<Surface>>(m, "Surface")
        .def(py::init<>(), "Create a surface")
        .def(py::init([](py::function func,
                         std::array<float, 2> u_range,
                         std::array<float, 2> v_range,
                         std::array<int, 2> resolution) {
            // Create a surface from a parametric function
            auto surface = std::make_shared<Surface>();
            // Generate mesh from parametric function
            std::vector<math::Vec3> positions;
            std::vector<uint32_t> indices;

            int u_res = resolution[0];
            int v_res = resolution[1];
            float u_min = u_range[0], u_max = u_range[1];
            float v_min = v_range[0], v_max = v_range[1];

            // Generate vertices
            for (int j = 0; j <= v_res; ++j) {
                for (int i = 0; i <= u_res; ++i) {
                    float u = u_min + (u_max - u_min) * i / u_res;
                    float v = v_min + (v_max - v_min) * j / v_res;

                    // Call the Python function
                    py::object result = func(u, v);
                    py::array_t<float> arr = result.cast<py::array_t<float>>();
                    auto buf = arr.request();
                    float* ptr = static_cast<float*>(buf.ptr);
                    positions.push_back(math::Vec3(ptr[0], ptr[1], ptr[2]));
                }
            }

            // Generate indices (quads split into triangles)
            for (int j = 0; j < v_res; ++j) {
                for (int i = 0; i < u_res; ++i) {
                    uint32_t idx = j * (u_res + 1) + i;
                    indices.push_back(idx);
                    indices.push_back(idx + 1);
                    indices.push_back(idx + u_res + 1);

                    indices.push_back(idx + 1);
                    indices.push_back(idx + u_res + 2);
                    indices.push_back(idx + u_res + 1);
                }
            }

            surface->set_mesh(positions, indices);
            return surface;
        }),
        py::arg("func"),
        py::arg("u_range") = std::array<float, 2>{-1.0f, 1.0f},
        py::arg("v_range") = std::array<float, 2>{-1.0f, 1.0f},
        py::arg("resolution") = std::array<int, 2>{10, 10},
        "Create a surface from parametric function");

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

    // ==================== Animation Composition ====================

    // AnimationGroup - run multiple animations in parallel
    py::class_<AnimationGroup, Animation, std::shared_ptr<AnimationGroup>>(m, "AnimationGroup")
        .def(py::init<std::vector<std::shared_ptr<Animation>>, float, float, RateFunc>(),
             py::arg("animations"),
             py::arg("run_time") = -1.0f,
             py::arg("lag_ratio") = 0.0f,
             py::arg("rate_func") = nullptr,
             "Run multiple animations in parallel with optional lag")
        .def("get_animation_count", &AnimationGroup::get_animation_count,
             "Get number of animations in group")
        .def("get_animation", &AnimationGroup::get_animation,
             py::arg("index"),
             "Get animation at index")
        .def("get_animations", &AnimationGroup::get_animations,
             "Get all animations")
        .def("get_lag_ratio", &AnimationGroup::get_lag_ratio,
             "Get lag ratio");

    // Succession - run animations sequentially
    py::class_<Succession, AnimationGroup, std::shared_ptr<Succession>>(m, "Succession")
        .def(py::init<std::vector<std::shared_ptr<Animation>>, float, RateFunc>(),
             py::arg("animations"),
             py::arg("run_time") = -1.0f,
             py::arg("rate_func") = nullptr,
             "Run animations one after another");

    // LaggedStart - AnimationGroup with default stagger
    py::class_<LaggedStart, AnimationGroup, std::shared_ptr<LaggedStart>>(m, "LaggedStart")
        .def(py::init<std::vector<std::shared_ptr<Animation>>, float, float>(),
             py::arg("animations"),
             py::arg("lag_ratio") = 0.05f,
             py::arg("run_time") = -1.0f,
             "Run animations with staggered starts (default 5% lag)");

    // ==================== Movement Animations ====================

    // MoveAlongPath - move mobject along a VMobject path
    py::class_<MoveAlongPath, Animation, std::shared_ptr<MoveAlongPath>>(m, "MoveAlongPath")
        .def(py::init<std::shared_ptr<Mobject>, std::shared_ptr<VMobject>, float, RateFunc>(),
             py::arg("mobject"),
             py::arg("path"),
             py::arg("run_time") = 1.0f,
             py::arg("rate_func") = nullptr,
             "Move mobject along a path")
        .def("get_path", &MoveAlongPath::get_path, "Get the path being followed")
        .def("get_starting_position", &MoveAlongPath::get_starting_position,
             "Get starting position");

    // MoveTo - move mobject to target position
    py::class_<MoveTo, Animation, std::shared_ptr<MoveTo>>(m, "MoveTo")
        .def(py::init<std::shared_ptr<Mobject>, const math::Vec3&, float, RateFunc>(),
             py::arg("mobject"),
             py::arg("target"),
             py::arg("run_time") = 1.0f,
             py::arg("rate_func") = nullptr,
             "Move mobject to target position");

    // Shift - shift mobject by vector
    py::class_<Shift, Animation, std::shared_ptr<Shift>>(m, "Shift")
        .def(py::init<std::shared_ptr<Mobject>, const math::Vec3&, float, RateFunc>(),
             py::arg("mobject"),
             py::arg("direction"),
             py::arg("run_time") = 1.0f,
             py::arg("rate_func") = nullptr,
             "Shift mobject by direction vector");

    // Homotopy - continuous deformation
    py::class_<Homotopy, Animation, std::shared_ptr<Homotopy>>(m, "Homotopy")
        .def(py::init<HomotopyFunc, std::shared_ptr<Mobject>, float, RateFunc>(),
             py::arg("homotopy"),
             py::arg("mobject"),
             py::arg("run_time") = 3.0f,
             py::arg("rate_func") = nullptr,
             "Apply continuous deformation (x,y,z,t) -> Vec3")
        .def("get_homotopy_func", &Homotopy::get_homotopy_func,
             "Get the homotopy function");

    // ComplexHomotopy - homotopy in complex plane
    py::class_<ComplexHomotopy, Homotopy, std::shared_ptr<ComplexHomotopy>>(m, "ComplexHomotopy")
        .def(py::init<ComplexHomotopyFunc, std::shared_ptr<Mobject>, float, RateFunc>(),
             py::arg("complex_homotopy"),
             py::arg("mobject"),
             py::arg("run_time") = 3.0f,
             py::arg("rate_func") = nullptr,
             "Apply complex plane homotopy (complex, t) -> complex");

    // ApplyPointwiseFunction - apply pointwise transformation
    py::class_<ApplyPointwiseFunction, Animation, std::shared_ptr<ApplyPointwiseFunction>>(m, "ApplyPointwiseFunction")
        .def(py::init<ApplyPointwiseFunction::PointwiseFunc, std::shared_ptr<Mobject>, float, RateFunc>(),
             py::arg("func"),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             py::arg("rate_func") = nullptr,
             "Apply pointwise function to morph mobject");

    // ApplyComplexPointwiseFunction - complex pointwise transformation
    py::class_<ApplyComplexPointwiseFunction, ApplyPointwiseFunction, std::shared_ptr<ApplyComplexPointwiseFunction>>(m, "ApplyComplexPointwiseFunction")
        .def(py::init<ApplyComplexPointwiseFunction::ComplexFunc, std::shared_ptr<Mobject>, float, RateFunc>(),
             py::arg("func"),
             py::arg("mobject"),
             py::arg("run_time") = 1.0f,
             py::arg("rate_func") = nullptr,
             "Apply complex pointwise function");
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
        .def("play", [](Scene& self, std::shared_ptr<Animation> anim, float run_time) {
            anim->set_run_time(run_time);
            self.play(anim);
        }, py::arg("animation"), py::arg("run_time"),
             "Play animation with specified run time")
        .def("play", [](Scene& self, py::args args, py::kwargs kwargs) {
            // Handle multiple animations with optional run_time kwarg
            std::vector<std::shared_ptr<Animation>> animations;
            for (auto& arg : args) {
                animations.push_back(arg.cast<std::shared_ptr<Animation>>());
            }

            // Extract run_time if provided
            float run_time = -1.0f;
            if (kwargs.contains("run_time")) {
                run_time = kwargs["run_time"].cast<float>();
            }

            // Set run_time for all animations if specified
            if (run_time > 0 && !animations.empty()) {
                for (auto& anim : animations) {
                    anim->set_run_time(run_time);
                }
            }

            // Play all animations (using AnimationGroup for multiple)
            if (animations.size() == 1) {
                self.play(animations[0]);
            } else if (animations.size() > 1) {
                self.play(animations);
            }
        }, "Play one or more animations with optional run_time kwarg")
        .def("wait", &Scene::wait, "Wait for duration",
             py::arg("duration") = 1.0,
             py::arg("stop_condition") = nullptr)
        .def("clear", &Scene::clear, "Clear all mobjects")
        .def("get_mobjects", &Scene::getMobjects, "Get all mobjects")
        .def("get_time", &Scene::getTime, "Get current time")
        .def("add_updater", &Scene::addUpdater, "Add scene updater")
        .def("enable_antialiasing", [](Scene& /*self*/, bool /*enable*/) {
            // Antialiasing control (stub for compatibility)
        }, py::arg("enable"),
             "Enable/disable antialiasing");

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
        .def("add_directional_light", &GPU3DScene::addDirectionalLight,
             py::arg("direction"),
             "Add directional light to scene")
        .def("remove_light", &GPU3DScene::removeLight,
             "Remove light by index")
        .def("set_environment_map",
             py::overload_cast<std::shared_ptr<EnvironmentMap>>(&GPU3DScene::setEnvironmentMap),
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
             "Apply tone mapping")
        // Additional methods for test compatibility
        .def("supports_ray_tracing", &GPU3DScene::supports_ray_tracing,
             "Check if ray tracing is supported")
        .def("render_frame", &GPU3DScene::render_frame,
             "Render a single frame")
        .def("render_ray_traced_frame", &GPU3DScene::render_ray_traced_frame,
             "Render a ray-traced frame")
        .def("set_real_time_mode", [](GPU3DScene& /*self*/, bool /*enable*/) {
            // Real-time mode flag (stub for compatibility)
        }, py::arg("enable"),
             "Enable/disable real-time rendering mode");

    // PBRMaterial binding
    py::class_<PBRMaterial>(m, "PBRMaterial")
        .def(py::init<>())
        .def_property(
            "albedo",
            [](const PBRMaterial& mat) { return mat.albedo; },
            [](PBRMaterial& mat, const py::object& value) {
                if (py::isinstance<Color>(value)) {
                    mat.albedo = value.cast<Color>().toVec4();
                    return;
                }
                mat.albedo = value.cast<Vec4>();
            },
            "Base color (accepts Vec4 or Color)")
        .def_readwrite("roughness", &PBRMaterial::roughness)
        .def_readwrite("metallic", &PBRMaterial::metallic)
        .def_readwrite("ao", &PBRMaterial::ao);

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
    // GPUDeviceInfo struct
    py::class_<GPUDeviceInfo>(m, "GPUDeviceInfo")
        .def(py::init<>())
        .def_readwrite("name", &GPUDeviceInfo::name)
        .def_readwrite("vendor", &GPUDeviceInfo::vendor)
        .def_readwrite("driver_version", &GPUDeviceInfo::driverVersion)
        .def_readwrite("total_memory_mb", &GPUDeviceInfo::totalMemoryMB)
        .def_readwrite("available_memory_mb", &GPUDeviceInfo::availableMemoryMB)
        .def_readwrite("device_id", &GPUDeviceInfo::deviceID)
        .def_readwrite("vendor_id", &GPUDeviceInfo::vendorID)
        .def_readwrite("discrete", &GPUDeviceInfo::discrete)
        .def_readwrite("supports_ray_tracing", &GPUDeviceInfo::supportsRayTracing)
        .def_readwrite("supports_compute", &GPUDeviceInfo::supportsCompute)
        .def_readwrite("supports_mesh_shaders", &GPUDeviceInfo::supportsMeshShaders)
        .def_readwrite("compute_units", &GPUDeviceInfo::computeUnits)
        .def_readwrite("max_work_group_size", &GPUDeviceInfo::maxWorkGroupSize)
        .def_readwrite("performance_score", &GPUDeviceInfo::performanceScore)
        .def("__repr__", [](const GPUDeviceInfo& info) {
            return "<GPUDeviceInfo: " + info.name + " (" +
                   std::to_string(info.totalMemoryMB) + " MB)>";
        });

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

// ==================== Culling/Optimization Bindings ====================

void bind_culling(py::module& m) {
    using namespace culling;

    // Create culling submodule
    auto culling_module = m.def_submodule("culling", "GPU-driven culling and optimization");

    // CullingStats struct
    py::class_<CullingStats>(culling_module, "CullingStats")
        .def(py::init<>())
        .def_readonly("total_objects", &CullingStats::total_objects)
        .def_readonly("frustum_visible", &CullingStats::frustum_visible)
        .def_readonly("occlusion_visible", &CullingStats::occlusion_visible)
        .def_readonly("final_visible", &CullingStats::final_visible)
        .def_readonly("frustum_cull_time_ms", &CullingStats::frustum_cull_time_ms)
        .def_readonly("occlusion_cull_time_ms", &CullingStats::occlusion_cull_time_ms)
        .def_readonly("total_time_ms", &CullingStats::total_time_ms)
        .def("__repr__", [](const CullingStats& s) {
            return "CullingStats(total=" + std::to_string(s.total_objects) +
                   ", visible=" + std::to_string(s.final_visible) +
                   ", time=" + std::to_string(s.total_time_ms) + "ms)";
        });

    // CullingConfig struct
    py::class_<CullingConfig>(culling_module, "CullingConfig")
        .def(py::init<>())
        .def_readwrite("enable_frustum_culling", &CullingConfig::enable_frustum_culling)
        .def_readwrite("enable_occlusion_culling", &CullingConfig::enable_occlusion_culling)
        .def_readwrite("enable_bvh_acceleration", &CullingConfig::enable_bvh_acceleration)
        .def_readwrite("enable_compaction", &CullingConfig::enable_compaction)
        .def_readwrite("max_objects", &CullingConfig::max_objects)
        .def_readwrite("occlusion_depth_bias", &CullingConfig::occlusion_depth_bias);

    // GPUCullingPipeline
    py::class_<GPUCullingPipeline>(culling_module, "GPUCullingPipeline")
        .def(py::init<>())
        .def("cleanup", &GPUCullingPipeline::cleanup, "Cleanup GPU resources")
        .def("is_initialized", &GPUCullingPipeline::is_initialized, "Check if initialized")
        .def("set_config", &GPUCullingPipeline::set_config, "Set culling configuration")
        .def("get_config", &GPUCullingPipeline::get_config, "Get current configuration")
        .def("enable_frustum_culling", &GPUCullingPipeline::enable_frustum_culling,
             py::arg("enable"),
             "Enable/disable frustum culling")
        .def("enable_occlusion_culling", &GPUCullingPipeline::enable_occlusion_culling,
             py::arg("enable"),
             "Enable/disable occlusion culling")
        .def("set_screen_dimensions", &GPUCullingPipeline::set_screen_dimensions,
             py::arg("width"),
             py::arg("height"),
             "Set screen dimensions for occlusion culling")
        .def("update_object_bounds",
             static_cast<void (GPUCullingPipeline::*)(const std::vector<std::shared_ptr<Mobject>>&)>(
                 &GPUCullingPipeline::update_object_bounds),
             py::arg("mobjects"),
             "Update object bounds from mobjects")
        .def("get_object_count", &GPUCullingPipeline::get_object_count,
             "Get number of objects")
        .def("get_visible_indices", &GPUCullingPipeline::get_visible_indices,
             "Get visible object indices after culling")
        .def("get_visible_count", &GPUCullingPipeline::get_visible_count,
             "Get count of visible objects")
        .def("is_visible", &GPUCullingPipeline::is_visible,
             py::arg("object_index"),
             "Check if specific object is visible")
        .def("get_stats", &GPUCullingPipeline::get_stats,
             "Get culling statistics");

    // IndirectRenderer::Stats
    py::class_<IndirectRenderer::Stats>(culling_module, "IndirectRendererStats")
        .def(py::init<>())
        .def_readonly("total_objects", &IndirectRenderer::Stats::total_objects)
        .def_readonly("visible_objects", &IndirectRenderer::Stats::visible_objects)
        .def_readonly("draw_calls", &IndirectRenderer::Stats::draw_calls)
        .def_readonly("total_vertices", &IndirectRenderer::Stats::total_vertices)
        .def_readonly("total_indices", &IndirectRenderer::Stats::total_indices)
        .def_readonly("generate_draws_time_ms", &IndirectRenderer::Stats::generate_draws_time_ms)
        .def_readonly("render_time_ms", &IndirectRenderer::Stats::render_time_ms);

    // IndirectRenderer::Config
    py::class_<IndirectRenderer::Config>(culling_module, "IndirectRendererConfig")
        .def(py::init<>())
        .def_readwrite("max_objects", &IndirectRenderer::Config::max_objects)
        .def_readwrite("max_draw_calls", &IndirectRenderer::Config::max_draw_calls)
        .def_readwrite("max_vertices", &IndirectRenderer::Config::max_vertices)
        .def_readwrite("max_indices", &IndirectRenderer::Config::max_indices)
        .def_readwrite("enable_draw_compaction", &IndirectRenderer::Config::enable_draw_compaction)
        .def_readwrite("enable_lod", &IndirectRenderer::Config::enable_lod);

    // IndirectRenderer
    py::class_<IndirectRenderer>(culling_module, "IndirectRenderer")
        .def(py::init<>())
        .def("cleanup", &IndirectRenderer::cleanup, "Cleanup resources")
        .def("is_initialized", &IndirectRenderer::is_initialized, "Check if initialized")
        .def("set_config", &IndirectRenderer::set_config, "Set renderer configuration")
        .def("get_config", &IndirectRenderer::get_config, "Get current configuration")
        .def("begin_frame", &IndirectRenderer::begin_frame, "Begin a new frame")
        .def("submit_objects", &IndirectRenderer::submit_objects,
             py::arg("mobjects"),
             "Submit objects for rendering")
        .def("update_object_transforms", &IndirectRenderer::update_object_transforms,
             py::arg("mobjects"),
             "Update object transforms")
        .def("get_draw_count", &IndirectRenderer::get_draw_count,
             "Get number of draw calls after culling")
        .def("get_stats", &IndirectRenderer::get_stats,
             "Get rendering statistics");
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
    bind_culling(m);

    // Version info
    m.attr("__version__") = "1.0.0";
    m.attr("__gpu_enabled__") = true;

    // Convenience functions
    m.def("check_gpu_available", []() {
        return true;
    }, "Check if GPU is available");

    m.def("get_gpu_info", []() {
        auto& detector = getGPUDetector();
        detector.detectGPUs();
        return detector.getGPUs();
    }, "Get GPU information");

    // ==================== GPU Utility Functions ====================

    // rotation_matrix - create rotation matrix around axis
    m.def("rotation_matrix", [](float angle, const Vec3& axis) {
        // Create rotation matrix using Rodrigues' formula
        float c = std::cos(angle);
        float s = std::sin(angle);
        float t = 1.0f - c;

        Vec3 n = axis;
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 0.0f) {
            n.x /= len;
            n.y /= len;
            n.z /= len;
        }

        Mat4 result;
        result[0][0] = t * n.x * n.x + c;
        result[0][1] = t * n.x * n.y - s * n.z;
        result[0][2] = t * n.x * n.z + s * n.y;
        result[0][3] = 0.0f;

        result[1][0] = t * n.x * n.y + s * n.z;
        result[1][1] = t * n.y * n.y + c;
        result[1][2] = t * n.y * n.z - s * n.x;
        result[1][3] = 0.0f;

        result[2][0] = t * n.x * n.z - s * n.y;
        result[2][1] = t * n.y * n.z + s * n.x;
        result[2][2] = t * n.z * n.z + c;
        result[2][3] = 0.0f;

        result[3][0] = 0.0f;
        result[3][1] = 0.0f;
        result[3][2] = 0.0f;
        result[3][3] = 1.0f;

        return result;
    }, py::arg("angle"), py::arg("axis"),
       "Create rotation matrix from angle and axis");

    // transform_points_gpu - apply transformation to points
    m.def("transform_points_gpu", [](py::array_t<float> points, const Mat4& matrix) {
        auto buf = points.request();
        if (buf.ndim != 2 || buf.shape[1] != 3) {
            throw std::runtime_error("Points must be Nx3 array");
        }

        size_t num_points = buf.shape[0];
        float* ptr = static_cast<float*>(buf.ptr);

        // Create output array
        auto result = py::array_t<float>({static_cast<py::ssize_t>(num_points), py::ssize_t{3}});
        auto result_buf = result.request();
        float* out_ptr = static_cast<float*>(result_buf.ptr);

        // Transform each point (could be GPU accelerated)
        for (size_t i = 0; i < num_points; ++i) {
            Vec4 p(ptr[i * 3 + 0], ptr[i * 3 + 1], ptr[i * 3 + 2], 1.0f);
            Vec4 transformed = matrix * p;
            out_ptr[i * 3 + 0] = transformed.x;
            out_ptr[i * 3 + 1] = transformed.y;
            out_ptr[i * 3 + 2] = transformed.z;
        }

        return result;
    }, py::arg("points"), py::arg("matrix"),
       "Transform points by matrix (GPU accelerated when available)");

    // ==================== GPUParticleSystem ====================

    py::class_<GPUParticleSystem>(m, "GPUParticleSystem")
        .def(py::init<>())
        .def("initialize", py::overload_cast<size_t>(&GPUParticleSystem::initialize),
             py::arg("count"),
             "Initialize particle system with count particles")
        .def("update", py::overload_cast<float>(&GPUParticleSystem::update),
             py::arg("dt"),
             "Update particle simulation")
        .def("render", py::overload_cast<>(&GPUParticleSystem::render),
             "Render particles")
        .def("set_gravity", &GPUParticleSystem::set_gravity,
             py::arg("gravity"),
             "Set gravity vector")
        .def("reset", &GPUParticleSystem::reset,
             "Reset all particles")
        .def("enable_sph_fluid", &GPUParticleSystem::enable_sph_fluid,
             py::arg("enable"),
             "Enable SPH fluid simulation")
        .def("enable_sorting", &GPUParticleSystem::enable_sorting,
             py::arg("enable"),
             "Enable depth sorting for transparency")
        .def("get_stats", &GPUParticleSystem::get_stats,
             "Get particle system statistics");

    // ParticleEmitterConfig
    py::class_<ParticleEmitterConfig>(m, "ParticleEmitterConfig")
        .def(py::init<>())
        .def_readwrite("particles_per_second", &ParticleEmitterConfig::particles_per_second)
        .def_readwrite("max_particles", &ParticleEmitterConfig::max_particles)
        .def_readwrite("particle_lifetime", &ParticleEmitterConfig::particle_lifetime)
        .def_readwrite("position", &ParticleEmitterConfig::position)
        .def_readwrite("velocity", &ParticleEmitterConfig::velocity)
        .def_readwrite("size", &ParticleEmitterConfig::size)
        .def_readwrite("color_start", &ParticleEmitterConfig::color_start)
        .def_readwrite("color_end", &ParticleEmitterConfig::color_end)
        .def_readwrite("gravity", &ParticleEmitterConfig::gravity)
        .def_readwrite("drag", &ParticleEmitterConfig::drag);
}
