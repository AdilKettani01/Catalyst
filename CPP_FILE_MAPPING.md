# Manim Python to C++ File Mapping

## Overview
Complete mapping of all 166 Python files in the Manim codebase to their C++ equivalents.
**Total Files**: 166 Python → 166+ C++ (additional headers)

---

## Directory Structure Mapping

```
manim/                          → src/manim/
├── __init__.py                 → manim.hpp (main header)
├── __main__.py                 → main.cpp (CLI entry point)
├── constants.py                → core/constants.hpp
├── data_structures.py          → core/data_structures.hpp
├── typing.py                   → core/types.hpp (using C++20 concepts)
```

---

## 1. Configuration System (_config/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `_config/__init__.py` | `config/config.hpp` | `config/config.cpp` | Global config singleton |
| `_config/cli_colors.py` | `config/cli_colors.hpp` | `config/cli_colors.cpp` | Terminal color codes |
| `_config/logger_utils.py` | `config/logger.hpp` | `config/logger.cpp` | Use spdlog library |
| `_config/utils.py` (1,877 lines) | `config/config_parser.hpp` | `config/config_parser.cpp` | Use TOML++ for parsing |

**C++ Libraries**:
- **spdlog**: Fast logging
- **toml++**: Config file parsing
- **CLI11**: Command-line parsing

---

## 2. Animation System (animation/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `animation/__init__.py` | `animation/animation.hpp` | - | Exports all animation types |
| `animation/animation.py` | `animation/base_animation.hpp` | `animation/base_animation.cpp` | Abstract base class |
| `animation/composition.py` | `animation/composition.hpp` | `animation/composition.cpp` | AnimationGroup, Succession |
| `animation/creation.py` | `animation/creation.hpp` | `animation/creation.cpp` | Create, Write, SpiralIn |
| `animation/changing.py` | `animation/changing.hpp` | `animation/changing.cpp` | TracedPath, AnimatedBoundary |
| `animation/fading.py` | `animation/fading.hpp` | `animation/fading.cpp` | FadeIn, FadeOut |
| `animation/growing.py` | `animation/growing.hpp` | `animation/growing.cpp` | Grow animations |
| `animation/indication.py` | `animation/indication.hpp` | `animation/indication.cpp` | Indicate, Flash, Wiggle |
| `animation/movement.py` | `animation/movement.hpp` | `animation/movement.cpp` | Homotopy, MoveAlongPath |
| `animation/numbers.py` | `animation/numbers.hpp` | `animation/numbers.cpp` | Number animations |
| `animation/rotation.py` | `animation/rotation.hpp` | `animation/rotation.cpp` | Rotating, Rotate |
| `animation/specialized.py` | `animation/specialized.hpp` | `animation/specialized.cpp` | Broadcast, etc. |
| `animation/speedmodifier.py` | `animation/speed_modifier.hpp` | `animation/speed_modifier.cpp` | Speed adjustments |
| `animation/transform.py` | `animation/transform.hpp` | `animation/transform.cpp` | Transform base |
| `animation/transform_matching_parts.py` | `animation/transform_matching.hpp` | `animation/transform_matching.cpp` | Part matching |
| `animation/updaters/update.py` | `animation/updaters.hpp` | `animation/updaters.cpp` | UpdateFromFunc |
| `animation/updaters/mobject_update_utils.py` | `animation/update_utils.hpp` | `animation/update_utils.cpp` | Utilities |

**Key C++ Design Patterns**:
- **Abstract Factory**: For animation creation
- **Strategy Pattern**: For rate functions
- **Template Method**: For interpolation
- **Observer Pattern**: For updaters

**C++ Features**:
```cpp
// Modern C++20 design
template<typename MobjectT>
concept AnimatableMobject = requires(MobjectT m) {
    { m.interpolate(0.5f) } -> std::convertible_to<void>;
};

class Animation {
    virtual void interpolate(float alpha) = 0;
    std::function<float(float)> rate_func;
    float run_time;
};
```

---

## 3. Camera System (camera/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `camera/__init__.py` | `camera/camera.hpp` | - | Camera exports |
| `camera/camera.py` (1,408 lines) | `camera/base_camera.hpp` | `camera/base_camera.cpp` | Base Camera class |
| `camera/moving_camera.py` | `camera/moving_camera.hpp` | `camera/moving_camera.cpp` | MovingCamera |
| `camera/three_d_camera.py` | `camera/three_d_camera.hpp` | `camera/three_d_camera.cpp` | 3D perspective |
| `camera/mapping_camera.py` | `camera/mapping_camera.hpp` | `camera/mapping_camera.cpp` | Coordinate mapping |
| `camera/multi_camera.py` | `camera/multi_camera.hpp` | `camera/multi_camera.cpp` | Multi-viewport |

**C++ Libraries**:
- **GLM**: OpenGL Mathematics for camera transforms
- **Eigen**: Matrix operations for projection

**Camera Architecture**:
```cpp
class Camera {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::vec3 position, target, up;

    virtual glm::mat4 get_view_matrix() const = 0;
    virtual void update(float dt) = 0;
};

class ThreeDCamera : public Camera {
    float theta, phi, gamma;  // Euler angles
    float focal_distance;
    glm::vec3 light_position;
};
```

---

## 4. CLI System (cli/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `cli/__init__.py` | `cli/cli.hpp` | - | CLI exports |
| `cli/default_group.py` | `cli/commands.hpp` | `cli/commands.cpp` | Main CLI |
| `cli/render/commands.py` | `cli/render_commands.hpp` | `cli/render_commands.cpp` | Render command |
| `cli/render/global_options.py` | `cli/global_options.hpp` | `cli/global_options.cpp` | Global opts |
| `cli/render/output_options.py` | `cli/output_options.hpp` | `cli/output_options.cpp` | Output opts |
| `cli/render/render_options.py` | `cli/render_options.hpp` | `cli/render_options.cpp` | Render opts |
| `cli/render/ease_of_access_options.py` | `cli/accessibility.hpp` | `cli/accessibility.cpp` | Accessibility |
| `cli/checkhealth/checks.py` | `cli/health_checks.hpp` | `cli/health_checks.cpp` | System checks |
| `cli/checkhealth/commands.py` | `cli/health_commands.hpp` | `cli/health_commands.cpp` | Health CLI |
| `cli/cfg/group.py` | `cli/config_commands.hpp` | `cli/config_commands.cpp` | Config CLI |
| `cli/init/commands.py` | `cli/init_commands.hpp` | `cli/init_commands.cpp` | Init CLI |
| `cli/plugins/commands.py` | `cli/plugin_commands.hpp` | `cli/plugin_commands.cpp` | Plugin CLI |

**C++ Library**:
- **CLI11**: Modern C++ command-line parser

---

## 5. Mobject Core System (mobject/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `mobject/__init__.py` | `mobject/mobject.hpp` | - | Main mobject exports |
| `mobject/mobject.py` (3,354 lines) | `mobject/base_mobject.hpp` | `mobject/base_mobject.cpp` | Base Mobject class |
| `mobject/frame.py` | `mobject/frame.hpp` | `mobject/frame.cpp` | Screen rectangles |
| `mobject/graph.py` (1,786 lines) | `mobject/graph.hpp` | `mobject/graph.cpp` | Graph structures |
| `mobject/logo.py` | `mobject/logo.hpp` | `mobject/logo.cpp` | ManimBanner |
| `mobject/matrix.py` | `mobject/matrix.hpp` | `mobject/matrix.cpp` | Matrix mobjects |
| `mobject/table.py` (1,165 lines) | `mobject/table.hpp` | `mobject/table.cpp` | Table mobjects |
| `mobject/value_tracker.py` | `mobject/value_tracker.hpp` | `mobject/value_tracker.cpp` | Value tracking |
| `mobject/vector_field.py` (1,087 lines) | `mobject/vector_field.hpp` | `mobject/vector_field.cpp` | Vector fields |
| `mobject/utils.py` | `mobject/utils.hpp` | `mobject/utils.cpp` | Utilities |

**Core Mobject Architecture**:
```cpp
// GPU-friendly design with ECS pattern
class Mobject {
protected:
    // GPU buffer handles
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    VkBuffer uniform_buffer;

    // Hierarchy
    std::vector<std::shared_ptr<Mobject>> submobjects;
    Mobject* parent = nullptr;

    // Transform
    glm::mat4 transform_matrix;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale{1.0f};

    // Appearance
    Color color;
    float opacity;
    float z_index;

    // Points (host-side cache)
    std::vector<glm::vec3> points;
    bool gpu_dirty = true;

public:
    virtual void update_gpu_buffers() = 0;
    virtual void render(VkCommandBuffer cmd) = 0;
    virtual BoundingBox compute_bounds() const = 0;

    // Updaters (C++ function objects)
    std::vector<std::function<void(Mobject&, float)>> updaters;
};
```

---

## 6. Mobject Types (mobject/types/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `types/__init__.py` | `mobject/types/types.hpp` | - | Type exports |
| `types/vectorized_mobject.py` (2,909 lines) | `mobject/types/vmobject.hpp` | `mobject/types/vmobject.cpp` | VMobject, VGroup |
| `types/point_cloud_mobject.py` | `mobject/types/point_cloud.hpp` | `mobject/types/point_cloud.cpp` | PMobject |
| `types/image_mobject.py` | `mobject/types/image.hpp` | `mobject/types/image.cpp` | Image rendering |

**VMobject GPU Design**:
```cpp
class VMobject : public Mobject {
    // Bezier curve data (GPU-resident)
    struct BezierCurve {
        glm::vec3 control_points[4];
        float stroke_width;
        Color stroke_color;
        Color fill_color;
    };

    std::vector<BezierCurve> curves;

    // GPU tessellation
    VkPipeline tessellation_pipeline;
    uint32_t tessellation_level = 16;

    void update_gpu_buffers() override;
    void render(VkCommandBuffer cmd) override;
};

class VGroup : public VMobject {
    // Manages multiple VMobjects as a batch
    void optimize_draw_calls();  // Batch similar objects
};
```

---

## 7. Geometry System (mobject/geometry/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `geometry/__init__.py` | `geometry/geometry.hpp` | - | Geometry exports |
| `geometry/arc.py` (1,352 lines) | `geometry/arc.hpp` | `geometry/arc.cpp` | Arc, Circle, Ellipse |
| `geometry/line.py` (1,210 lines) | `geometry/line.hpp` | `geometry/line.cpp` | Line, Arrow, Vector |
| `geometry/polygram.py` | `geometry/polygon.hpp` | `geometry/polygon.cpp` | Polygons |
| `geometry/tips.py` | `geometry/arrow_tips.hpp` | `geometry/arrow_tips.cpp` | Arrow tips |
| `geometry/labeled.py` | `geometry/labeled.hpp` | `geometry/labeled.cpp` | Labeled objects |
| `geometry/boolean_ops.py` | `geometry/boolean_ops.hpp` | `geometry/boolean_ops.cpp` | CSG operations |
| `geometry/shape_matchers.py` | `geometry/shape_matchers.hpp` | `geometry/shape_matchers.cpp` | Bounding shapes |

**C++ Library for CSG**:
- **CGAL**: Computational Geometry Algorithms Library
- **Clipper2**: Fast polygon clipping

**GPU Geometry Generation**:
```cpp
class Circle : public VMobject {
    float radius;
    uint32_t num_segments;

    void generate_points() override {
        // Generate on GPU using compute shader
        dispatch_compute_shader("circle_generation.comp");
    }
};
```

---

## 8. Graphing System (mobject/graphing/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `graphing/__init__.py` | `graphing/graphing.hpp` | - | Graphing exports |
| `graphing/coordinate_systems.py` (3,491 lines) | `graphing/coordinate_systems.hpp` | `graphing/coordinate_systems.cpp` | Axes, planes |
| `graphing/functions.py` | `graphing/functions.hpp` | `graphing/functions.cpp` | Function graphs |
| `graphing/number_line.py` | `graphing/number_line.hpp` | `graphing/number_line.cpp` | Number lines |
| `graphing/probability.py` | `graphing/probability.hpp` | `graphing/probability.cpp` | Charts |
| `graphing/scale.py` | `graphing/scale.hpp` | `graphing/scale.cpp` | Scale types |

**GPU Function Plotting**:
```cpp
class FunctionGraph : public VMobject {
    std::function<float(float)> function;
    float x_min, x_max;

    // GPU-accelerated sampling
    void compute_points_gpu() {
        // Parallel evaluation on GPU
        // Adaptive sampling based on curvature
    }
};
```

---

## 9. Text System (mobject/text/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `text/__init__.py` | `text/text.hpp` | - | Text exports |
| `text/text_mobject.py` (1,545 lines) | `text/text_mobject.hpp` | `text/text_mobject.cpp` | Text rendering |
| `text/tex_mobject.py` | `text/tex_mobject.hpp` | `text/tex_mobject.cpp` | LaTeX rendering |
| `text/code_mobject.py` | `text/code_mobject.hpp` | `text/code_mobject.cpp` | Code highlighting |
| `text/numbers.py` | `text/numbers.hpp` | `text/numbers.cpp` | Number display |

**C++ Libraries for Text**:
- **FreeType**: Font rasterization
- **HarfBuzz**: Text shaping
- **msdfgen**: Multi-channel signed distance field for high-quality text
- **tinylatex** or call external LaTeX → SVG pipeline

**GPU Text Rendering**:
```cpp
class Text : public VMobject {
    std::string text;
    Font* font;

    // Use MSDF atlas for GPU-accelerated rendering
    Texture2D* msdf_atlas;

    void generate_glyphs() {
        // HarfBuzz for shaping
        // FreeType for glyph metrics
        // msdfgen for SDF generation
    }
};
```

---

## 10. 3D System (mobject/three_d/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `three_d/__init__.py` | `three_d/three_d.hpp` | - | 3D exports |
| `three_d/three_dimensions.py` (1,275 lines) | `three_d/primitives.hpp` | `three_d/primitives.cpp` | Sphere, Cube, etc. |
| `three_d/polyhedra.py` | `three_d/polyhedra.hpp` | `three_d/polyhedra.cpp` | Platonic solids |
| `three_d/three_d_utils.py` | `three_d/utils.hpp` | `three_d/utils.cpp` | 3D utilities |

**GPU Mesh Generation**:
```cpp
class Sphere : public Mobject {
    float radius;
    uint32_t u_segments, v_segments;

    // GPU procedural generation
    VkPipeline procedural_pipeline;

    void generate_mesh() {
        // Compute shader for mesh generation
        // Normal/tangent computation
        // UV mapping
    }
};
```

---

## 11. SVG System (mobject/svg/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `svg/__init__.py` | `svg/svg.hpp` | - | SVG exports |
| `svg/svg_mobject.py` | `svg/svg_mobject.hpp` | `svg/svg_mobject.cpp` | SVG parsing |
| `svg/brace.py` | `svg/brace.hpp` | `svg/brace.cpp` | Brace objects |

**C++ Library for SVG**:
- **NanoSVG**: Lightweight SVG parser
- **LunaSVG**: More complete SVG support

---

## 12. OpenGL Mobject System (mobject/opengl/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `opengl/__init__.py` | - | - | **DEPRECATED - absorbed into main rendering** |
| `opengl/opengl_mobject.py` (3,175 lines) | - | - | Concepts merged into base Mobject |
| `opengl/opengl_vectorized_mobject.py` (1,907 lines) | - | - | Merged into VMobject |
| `opengl/opengl_point_cloud_mobject.py` | - | - | Merged into PMobject |
| `opengl/opengl_geometry.py` | - | - | Merged into geometry |
| `opengl/opengl_surface.py` | - | - | Merged into 3D system |
| `opengl/opengl_three_dimensions.py` | - | - | Merged into 3D system |
| `opengl/opengl_image_mobject.py` | - | - | Merged into image system |
| `opengl/dot_cloud.py` | `effects/dot_cloud.hpp` | `effects/dot_cloud.cpp` | Special effects |
| `opengl/opengl_compatibility.py` | - | - | **Not needed - C++ is GPU-first** |

**Design Note**: In C++, we eliminate the dual Cairo/OpenGL architecture. All rendering is GPU-first with Vulkan/OpenGL backend.

---

## 13. Renderer System (renderer/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `renderer/__init__.py` | `renderer/renderer.hpp` | - | Renderer exports |
| `renderer/cairo_renderer.py` | - | - | **REMOVED - GPU-only** |
| `renderer/opengl_renderer.py` (1,900 lines) | `renderer/vulkan_renderer.hpp` | `renderer/vulkan_renderer.cpp` | Primary renderer |
| `renderer/opengl_renderer_window.py` | `renderer/window.hpp` | `renderer/window.cpp` | Window management |
| `renderer/shader.py` | `renderer/shader.hpp` | `renderer/shader.cpp` | Shader abstraction |
| `renderer/shader_wrapper.py` | `renderer/shader_manager.hpp` | `renderer/shader_manager.cpp` | Shader management |
| `renderer/vectorized_mobject_rendering.py` | `renderer/vmobject_renderer.hpp` | `renderer/vmobject_renderer.cpp` | VMobject rendering |

**Additional C++ Files for Rendering**:
- `renderer/render_pass.hpp/cpp` - Render pass abstraction
- `renderer/frame_buffer.hpp/cpp` - FBO management
- `renderer/texture.hpp/cpp` - Texture management
- `renderer/buffer.hpp/cpp` - VBO/UBO/SSBO management
- `renderer/pipeline.hpp/cpp` - Graphics pipeline
- `renderer/compute_pipeline.hpp/cpp` - Compute pipeline
- `renderer/ray_tracer.hpp/cpp` - Ray tracing (RTX/DXR)

**C++ Libraries**:
- **Vulkan/VMA**: Primary rendering API + memory allocator
- **GLFW**: Window creation
- **VulkanHPP**: C++ bindings for Vulkan
- **Shader Compiler**: glslang, SPIRV-Cross

---

## 14. Shader System (renderer/shaders/)

All GLSL shaders remain largely the same but upgraded:
- Convert to **SPIR-V** for Vulkan
- Add **ray tracing shaders** (.rgen, .rchit, .rmiss)
- Add **compute shaders** for parallel processing

| Shader Category | Files | C++ Integration |
|----------------|-------|-----------------|
| Default | `default/*.{vert,frag}` | `shaders/default/` |
| Bezier Fill | `quadratic_bezier_fill/*.glsl` | `shaders/bezier/fill/` |
| Bezier Stroke | `quadratic_bezier_stroke/*.glsl` | `shaders/bezier/stroke/` |
| Surface | `surface/*.glsl` | `shaders/surface/` |
| Image | `image/*.glsl` | `shaders/image/` |
| **Ray Tracing (NEW)** | - | `shaders/rt/*.{rgen,rchit,rmiss}` |
| **Compute (NEW)** | - | `shaders/compute/*.comp` |

---

## 15. Scene System (scene/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `scene/__init__.py` | `scene/scene.hpp` | - | Scene exports |
| `scene/scene.py` (1,932 lines) | `scene/base_scene.hpp` | `scene/base_scene.cpp` | Base Scene class |
| `scene/scene_file_writer.py` | `scene/video_writer.hpp` | `scene/video_writer.cpp` | Video output |
| `scene/section.py` | `scene/section.hpp` | `scene/section.cpp` | Scene sections |
| `scene/moving_camera_scene.py` | `scene/moving_camera_scene.hpp` | `scene/moving_camera_scene.cpp` | Moving camera |
| `scene/three_d_scene.py` | `scene/three_d_scene.hpp` | `scene/three_d_scene.cpp` | 3D scenes |
| `scene/vector_space_scene.py` | `scene/vector_scene.hpp` | `scene/vector_scene.cpp` | Vector scenes |
| `scene/zoomed_scene.py` | `scene/zoomed_scene.hpp` | `scene/zoomed_scene.cpp` | Zoom scenes |

**C++ Libraries for Video**:
- **FFmpeg**: Video encoding (libavcodec, libavformat, libavutil)
- **x264/x265**: H.264/H.265 encoding

**Scene Architecture**:
```cpp
class Scene {
    std::vector<std::shared_ptr<Mobject>> mobjects;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Camera> camera;

    // Animation queue (parallel execution)
    std::vector<std::unique_ptr<Animation>> animation_queue;

    // Thread pool for parallel updates
    ThreadPool update_thread_pool;

    void play(std::vector<Animation*> animations);
    void add(Mobject* mob);
    void remove(Mobject* mob);
    void render_frame();
};
```

---

## 16. Utils System (utils/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `utils/__init__.py` | `utils/utils.hpp` | - | Utils exports |
| `utils/bezier.py` (2,079 lines) | `utils/bezier.hpp` | `utils/bezier.cpp` | Bezier operations |
| `utils/caching.py` | `utils/cache.hpp` | `utils/cache.cpp` | Animation caching |
| `utils/commands.py` | `utils/commands.hpp` | `utils/commands.cpp` | System commands |
| `utils/config_ops.py` | `utils/config_ops.hpp` | `utils/config_ops.cpp` | Config operations |
| `utils/debug.py` | `utils/debug.hpp` | `utils/debug.cpp` | Debug utilities |
| `utils/deprecation.py` | `utils/deprecation.hpp` | `utils/deprecation.cpp` | Deprecation warnings |
| `utils/exceptions.py` | `utils/exceptions.hpp` | - | Exception classes |
| `utils/family.py` | `utils/family.hpp` | `utils/family.cpp` | Hierarchy utilities |
| `utils/family_ops.py` | `utils/family_ops.hpp` | `utils/family_ops.cpp` | Family operations |
| `utils/file_ops.py` | `utils/file_ops.hpp` | `utils/file_ops.cpp` | File operations |
| `utils/hashing.py` | `utils/hash.hpp` | `utils/hash.cpp` | Hashing (use xxHash) |
| `utils/images.py` | `utils/image_utils.hpp` | `utils/image_utils.cpp` | Image utilities |
| `utils/ipython_magic.py` | - | - | **Not needed in C++** |
| `utils/iterables.py` | `utils/iterables.hpp` | - | Template-based (header-only) |
| `utils/module_ops.py` | `utils/module_ops.hpp` | `utils/module_ops.cpp` | Module operations |
| `utils/opengl.py` | `utils/vulkan_utils.hpp` | `utils/vulkan_utils.cpp` | GPU utilities |
| `utils/parameter_parsing.py` | `utils/params.hpp` | `utils/params.cpp` | Parameter parsing |
| `utils/paths.py` | `utils/path_utils.hpp` | `utils/path_utils.cpp` | Path utilities |
| `utils/polylabel.py` | `utils/polylabel.hpp` | `utils/polylabel.cpp` | Polygon labeling |
| `utils/qhull.py` | `utils/convex_hull.hpp` | `utils/convex_hull.cpp` | QuickHull |
| `utils/rate_functions.py` | `utils/rate_functions.hpp` | - | Template-based (header-only) |
| `utils/simple_functions.py` | `utils/math_utils.hpp` | - | Template-based (header-only) |
| `utils/sounds.py` | `utils/audio.hpp` | `utils/audio.cpp` | Audio utilities |
| `utils/space_ops.py` | `utils/space_ops.hpp` | - | Template-based with GLM |
| `utils/tex.py` | `utils/latex.hpp` | `utils/latex.cpp` | LaTeX utilities |
| `utils/tex_file_writing.py` | `utils/latex_writer.hpp` | `utils/latex_writer.cpp` | LaTeX generation |
| `utils/tex_templates.py` (1,051 lines) | `utils/latex_templates.hpp` | `utils/latex_templates.cpp` | LaTeX templates |
| `utils/unit.py` | `utils/units.hpp` | - | Constexpr units (header-only) |

**C++ Libraries for Utils**:
- **GLM**: Vector/matrix operations (replaces numpy)
- **xxHash**: Fast hashing
- **stb_image**: Image loading/saving
- **filesystem**: C++17 filesystem

---

## 17. Color System (utils/color/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `color/__init__.py` | `color/color.hpp` | - | Color exports |
| `color/core.py` (1,667 lines) | `color/color_core.hpp` | `color/color_core.cpp` | Color class |
| `color/manim_colors.py` | `color/manim_colors.hpp` | - | Manim palette (constexpr) |
| `color/AS2700.py` | `color/as2700.hpp` | - | AS2700 standard |
| `color/BS381.py` | `color/bs381.hpp` | - | BS381 standard |
| `color/DVIPSNAMES.py` | `color/dvipsnames.hpp` | - | DVIPS names |
| `color/SVGNAMES.py` | `color/svgnames.hpp` | - | SVG names |
| `color/X11.py` | `color/x11.hpp` | - | X11 names |
| `color/XKCD.py` | `color/xkcd.hpp` | - | XKCD colors |

**Modern C++ Color Design**:
```cpp
class Color {
    float r, g, b, a;  // Linear space

    static Color from_rgb(uint8_t r, uint8_t g, uint8_t b);
    static Color from_hsv(float h, float s, float v);
    static Color from_hex(std::string_view hex);

    Color interpolate(const Color& other, float t) const;

    // GPU-friendly representation
    glm::vec4 to_vec4() const;
};

// Constexpr color constants
namespace Colors {
    constexpr Color RED{1.0f, 0.0f, 0.0f, 1.0f};
    constexpr Color BLUE{0.0f, 0.0f, 1.0f, 1.0f};
    // ...
}
```

---

## 18. Testing System (utils/testing/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `testing/__init__.py` | - | - | Use GoogleTest/Catch2 |
| `testing/frames_comparison.py` | `test/frame_comparison.hpp` | `test/frame_comparison.cpp` | Frame comparison |
| `testing/_frames_testers.py` | `test/test_utils.hpp` | `test/test_utils.cpp` | Test utilities |
| `testing/_show_diff.py` | `test/diff_viewer.hpp` | `test/diff_viewer.cpp` | Diff visualization |
| `testing/_test_class_makers.py` | - | - | Use GoogleTest fixtures |

**C++ Testing Framework**:
- **GoogleTest**: Unit testing
- **GoogleBenchmark**: Performance testing
- **Catch2**: Alternative testing framework

---

## 19. Documentation System (utils/docbuild/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `docbuild/__init__.py` | - | - | Use Doxygen |
| `docbuild/autoaliasattr_directive.py` | - | - | Doxygen custom commands |
| `docbuild/autocolor_directive.py` | - | - | Doxygen plugins |
| `docbuild/manim_directive.py` | - | - | Sphinx → Doxygen |
| `docbuild/module_parsing.py` | - | - | Use libclang for parsing |

**Documentation Tools**:
- **Doxygen**: API documentation
- **Sphinx (Breathe)**: Integration with Python docs

---

## 20. Plugin System (plugins/)

| Python File | C++ Header | C++ Implementation | Notes |
|-------------|------------|-------------------|-------|
| `plugins/__init__.py` | `plugins/plugin_manager.hpp` | `plugins/plugin_manager.cpp` | Plugin system |
| `plugins/plugins_flags.py` | `plugins/plugin_interface.hpp` | - | Plugin interface |

**C++ Plugin Architecture**:
```cpp
// Dynamic library loading
class Plugin {
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual const char* get_name() const = 0;
    virtual const char* get_version() const = 0;
};

class PluginManager {
    void load_plugin(const std::filesystem::path& path);
    std::vector<std::unique_ptr<Plugin>> plugins;
};
```

---

## Additional C++ Files (Not in Python)

### GPU Acceleration Layer
| C++ File | Purpose |
|----------|---------|
| `gpu/vulkan_context.hpp/cpp` | Vulkan initialization |
| `gpu/device.hpp/cpp` | GPU device management |
| `gpu/memory_allocator.hpp/cpp` | VMA wrapper |
| `gpu/command_buffer.hpp/cpp` | Command buffer management |
| `gpu/descriptor_set.hpp/cpp` | Descriptor management |
| `gpu/pipeline_cache.hpp/cpp` | PSO caching |
| `gpu/sync.hpp/cpp` | Fences, semaphores |

### Ray Tracing Layer
| C++ File | Purpose |
|----------|---------|
| `rt/acceleration_structure.hpp/cpp` | BLAS/TLAS management |
| `rt/ray_tracing_pipeline.hpp/cpp` | RT pipeline |
| `rt/shader_binding_table.hpp/cpp` | SBT management |

### Compute Layer
| C++ File | Purpose |
|----------|---------|
| `compute/compute_pass.hpp/cpp` | Compute pipeline |
| `compute/parallel_for.hpp/cpp` | Parallel algorithms |

### Platform Abstraction
| C++ File | Purpose |
|----------|---------|
| `platform/window.hpp/cpp` | Window abstraction (GLFW) |
| `platform/filesystem.hpp/cpp` | Filesystem utilities |
| `platform/threading.hpp/cpp` | Thread pool, async |

---

## Summary Statistics

| Category | Python Files | C++ Headers | C++ Implementation Files | Total C++ Files |
|----------|-------------|-------------|-------------------------|----------------|
| Core | 4 | 4 | 3 | 7 |
| Config | 4 | 4 | 4 | 8 |
| Animation | 17 | 17 | 17 | 34 |
| Camera | 6 | 6 | 6 | 12 |
| CLI | 12 | 12 | 12 | 24 |
| Mobject Core | 10 | 10 | 10 | 20 |
| Mobject Types | 4 | 4 | 4 | 8 |
| Geometry | 8 | 8 | 8 | 16 |
| Graphing | 6 | 6 | 6 | 12 |
| Text | 5 | 5 | 5 | 10 |
| 3D | 4 | 4 | 4 | 8 |
| SVG | 3 | 3 | 3 | 6 |
| OpenGL (merged) | 10 | - | - | - |
| Renderer | 7 | 12 | 12 | 24 |
| Scene | 7 | 7 | 7 | 14 |
| Utils | 28 | 28 | 20 | 48 |
| Color | 9 | 9 | 1 | 10 |
| Testing | 4 | 3 | 3 | 6 |
| Plugins | 2 | 2 | 2 | 4 |
| **GPU Layer (NEW)** | - | 8 | 8 | 16 |
| **RT Layer (NEW)** | - | 3 | 3 | 6 |
| **Compute (NEW)** | - | 2 | 2 | 4 |
| **Platform (NEW)** | - | 3 | 3 | 6 |
| **TOTAL** | **166** | **160** | **143** | **303** |

---

## Build System

### CMake Structure
```
CMakeLists.txt              # Root CMake
cmake/
├── FindVulkan.cmake        # Vulkan detection
├── FindGLFW.cmake          # GLFW detection
└── CompileShaders.cmake    # SPIR-V compilation

src/
├── CMakeLists.txt          # Main library
├── core/CMakeLists.txt
├── animation/CMakeLists.txt
├── mobject/CMakeLists.txt
└── ...
```

### External Dependencies (managed via vcpkg/Conan)
- **Vulkan SDK**: Graphics API
- **GLFW**: Windowing
- **GLM**: Math library
- **Eigen**: Linear algebra
- **FreeType/HarfBuzz**: Text rendering
- **FFmpeg**: Video encoding
- **spdlog**: Logging
- **CLI11**: Command-line parsing
- **toml++**: Config parsing
- **CGAL**: Computational geometry
- **GoogleTest**: Testing

---

## Compilation Targets

```bash
# Main library
libmanim.so / manim.dll

# Main executable
manim (CLI)

# Test suite
manim_tests

# Benchmark suite
manim_benchmarks

# Plugins
plugins/*.so / plugins/*.dll
```

---

## Next Steps
See `ARCHITECTURE_DESIGN.md` for GPU-first architecture design.
See `RENDERING_PIPELINE.md` for advanced 3D rendering pipeline.
See `CONVERSION_CHECKLIST.md` for detailed conversion tracking.
