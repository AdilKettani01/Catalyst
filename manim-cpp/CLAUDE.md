# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Catalyst** is a GPU-accelerated C++ animation and rendering library. It uses Vulkan as the primary rendering backend with a GPU-first architecture where geometry data lives on the GPU with automatic CPU-GPU synchronization.

## Build Commands

```bash
# Configure and build (from manim-cpp directory)
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DMANIM_ENABLE_VULKAN=ON \
  -DMANIM_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

cmake --build build -j$(nproc)

# Build specific targets
make catalyst       # Main library
make manim-cli      # CLI executable
make manim_tests    # Unit tests
make manim_cpp      # Python bindings

# Run all tests
./bin/manim_tests

# Run specific test suite
./bin/manim_tests --gtest_filter="*BezierTessellation*"

# Run benchmarks
./bin/manim_benchmarks
```

## Running Examples

```bash
cd build

# Render shapes to image
./bin/gpu_render_to_file --gpu output.ppm

# Run text animation (TEXT1/2/3 fade in/out)
./bin/text_fade_animation
```

## Architecture

### Core Layer (`src/core/`, `include/manim/core/`)
- **MemoryPool** (`memory_pool.hpp`): Vulkan Memory Allocator wrapper for GPU buffer management
- **ComputeEngine** (`compute_engine.hpp`): GPU compute pipeline for parallel operations
- **Math** (`math.hpp`): GLM-based math types (Vec3, Vec4, Mat4)

### Mobject Hierarchy (`src/mobject/`, `include/manim/mobject/`)
- **Mobject**: Base class for all renderable objects
- **VMobject**: Vectorized mobject for 2D curves with GPU tessellation
- **Geometry**: Circle, Rectangle, Polygon inherit from VMobject
- **Text**: GPU-accelerated SDF text rendering
- **3D**: Mesh, Sphere, Surface for 3D primitives

### Renderer Layer (`src/renderer/`, `include/manim/renderer/`)
- **VulkanRenderer**: Primary renderer with Vulkan instance, device, swapchain
- **GPU3DRenderer**: High-level 3D rendering with MSAA
- **ShaderSystem**: Shader loading with hot-reload support

### Scene/Animation (`src/scene/`, `src/animation/`)
- **Scene**: Container for mobjects with play/wait animation timeline
- **Animation**: Base animation class with rate functions
- **FadeIn/FadeOut**: Opacity animations
- **Transform**: Morphing animations

### Shaders (`shaders/`)
- `compute/`: GPU compute shaders (bezier tessellation, vector ops)
- `vertex/`: Vertex shaders
- `fragment/`: Fragment shaders
- `raytracing/`: RTX ray tracing shaders

Shaders compile to SPIR-V at build time via glslangValidator.

## Key Patterns

### Creating a Scene
```cpp
#include "manim/scene/scene.h"
#include "manim/mobject/text/text.hpp"
#include "manim/animation/fading.hpp"

class MyScene : public manim::Scene {
public:
    void construct() override {
        auto text = std::make_shared<manim::Text>("Hello", 72.0f);
        play(std::make_shared<manim::FadeIn>(text, 1.0f));
        wait(2.0);
        play(std::make_shared<manim::FadeOut>(text, 1.0f));
    }
};

int main() {
    MyScene scene;
    scene.render();
    return 0;
}
```

### GPU Buffer Usage
```cpp
auto buffer = memory_pool.allocate_buffer(
    size,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    MemoryType::HostVisible,
    MemoryUsage::Dynamic
);
void* ptr = buffer.map();
memcpy(ptr, data, size);
buffer.unmap();
```

### Mobject Points
```cpp
vmobject->set_points({Vec3(0,0,0), Vec3(1,0,0), ...});
auto points = mobject->get_points(); // Downloads from GPU if needed
```

## Testing

Tests use GoogleTest. Key test files:
- `tests/unit/test_gpu.cpp`: GPU compute, Bezier tessellation
- `tests/unit/test_mobjects.cpp`: Mobject hierarchy tests
- `tests/unit/test_animations.cpp`: Animation system tests
- `tests/validation/`: Integration and performance tests

All 270 tests pass.

## Dependencies (via vcpkg)

vulkan-memory-allocator, spdlog, cli11, tomlplusplus, glfw3, glm, eigen3, benchmark, gtest, ffmpeg, freetype, harfbuzz, pybind11

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `MANIM_ENABLE_VULKAN` | ON | Primary renderer |
| `MANIM_BUILD_TESTS` | OFF | Build unit tests |
| `MANIM_BUILD_PYTHON_BINDINGS` | OFF | Python bindings |

## Project Structure

```
manim-cpp/
├── include/manim/     # Public headers
├── src/               # Implementation
├── shaders/           # Vulkan GLSL shaders
├── examples/          # Example applications
│   ├── gpu_render_to_file.cpp
│   └── text_fade_animation.cpp
├── tests/             # Unit & integration tests
└── bindings/python/   # Python bindings
```
