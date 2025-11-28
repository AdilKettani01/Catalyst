# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Manim C++ is a GPU-accelerated C++ port of the Python Manim mathematical animation library. It uses Vulkan as the primary rendering backend with a GPU-first architecture where geometry data lives on the GPU with automatic CPU-GPU synchronization.

## Build Commands

```bash
# Configure and build (from project root)
cd build && cmake .. && make -j4

# Build specific targets
make manim          # Main library
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

## Architecture

### Core Layer (`src/core/`, `include/manim/core/`)
- **MemoryPool** (`memory_pool.hpp`): Vulkan Memory Allocator wrapper for GPU buffer management. Handles host-visible vs device-local memory with automatic sub-allocation.
- **ComputeEngine** (`compute_engine.hpp`): GPU compute pipeline for parallel operations. Manages descriptor sets, command buffers, and compute shader dispatch. Includes real GPU Bezier tessellation with CPU fallback.
- **Math** (`math.hpp`): GLM-based math types (Vec3, Vec4, Mat4) with GPU-friendly layouts.

### Mobject Hierarchy (`src/mobject/`, `include/manim/mobject/`)
- **Mobject**: Base class for all renderable objects. Stores points in GPU buffers with lazy CPU sync. Supports hierarchical submobjects.
- **VMobject**: Vectorized mobject for 2D curves. Cubic Bezier representation with GPU tessellation support.
- **Geometry**: Circle, Rectangle, Polygon, etc. inherit from VMobject.
- **3D**: Mesh, Sphere, Surface for 3D primitives.

### Renderer Layer (`src/renderer/`, `include/manim/renderer/`)
- **VulkanRenderer**: Primary renderer. Manages Vulkan instance, device, swapchain, command buffers. Handles window resize and interactive render loop.
- **GPU3DRenderer**: High-level 3D rendering with deferred pipeline, PBR materials.
- **ShaderSystem**: Shader loading with hot-reload support via timestamp tracking.

### Scene/Animation (`src/scene/`, `src/animation/`)
- **Scene**: Container for mobjects with play/wait animation timeline.
- **Animation**: Base animation class with rate functions and interpolation.
- **Transform/Creation/Fading**: Animation subclasses.

### Shaders (`shaders/`)
- `compute/bezier_tessellation.comp`: GPU Bezier curve tessellation
- `compute/vector_ops.comp`: Parallel vector operations
- `vertex/`, `fragment/`: Standard rendering shaders
- `raytracing/`: RTX ray tracing shaders

Shaders are compiled to SPIR-V at build time via glslangValidator. Output goes to `build/shaders/*.spv`.

## Key Patterns

### GPU Buffer Usage
```cpp
// Allocate GPU buffer via MemoryPool
auto buffer = memory_pool.allocate_buffer(
    size,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    MemoryType::HostVisible,
    MemoryUsage::Dynamic
);
// Map, write, unmap for host-visible buffers
void* ptr = buffer.map();
memcpy(ptr, data, size);
buffer.unmap();
```

### Mobject Points
```cpp
// Points stored as Vec3, use set_points/get_points
vmobject->set_points({Vec3(0,0,0), Vec3(1,0,0), ...});
auto points = mobject->get_points(); // Downloads from GPU if needed
```

### ComputeEngine Dispatch
```cpp
engine.tessellate_bezier_gpu(control_points_buffer, output_buffer,
                             num_curves, segments_per_curve);
// Falls back to CPU via tessellate_bezier_cpu if GPU unavailable
```

## Testing

Tests use GoogleTest. Key test files:
- `tests/unit/test_gpu.cpp`: GPU compute, Bezier tessellation, swapchain tests
- `tests/unit/test_mobjects.cpp`: Mobject hierarchy tests
- `tests/unit/test_animations.cpp`: Animation system tests
- `tests/validation/`: Integration and performance tests

## Dependencies (via vcpkg)

Vulkan, GLFW3, GLM, Eigen3, spdlog, CLI11, tomlplusplus, FFmpeg, FreeType, HarfBuzz, GoogleTest, benchmark, pybind11

## CMake Options

```bash
-DMANIM_ENABLE_VULKAN=ON       # Primary renderer
-DMANIM_ENABLE_RAYTRACING=ON   # RTX support
-DMANIM_BUILD_TESTS=ON
-DMANIM_BUILD_PYTHON_BINDINGS=ON
```
