# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains two implementations of Manim (Mathematical Animation Engine):

1. **Python Manim** (`manim/`): The original Manim Community Edition Python library for creating mathematical animations
2. **Manim C++** (`manim-cpp/`): A GPU-accelerated C++ rewrite using Vulkan, targeting 100-1000x performance improvement

The C++ port is the active development focus. See `manim-cpp/CLAUDE.md` for detailed C++ architecture and patterns.

## Build Commands (C++ - Primary)

```bash
# Build from manim-cpp directory
cd manim-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMANIM_BUILD_TESTS=ON -DMANIM_ENABLE_VULKAN=ON
cmake --build build --config Release -j$(nproc)

# Run tests
cd manim-cpp/build && ctest --output-on-failure

# Run specific test filter
./manim-cpp/build/bin/manim_tests --gtest_filter="*BezierTessellation*"

# GPU-specific tests
./manim-cpp/build/bin/manim_tests --gtest_filter="GPUComputeTest.*:GPUMemoryTest.*"

# Benchmarks
./manim-cpp/build/bin/manim_benchmarks
```

### GPU Troubleshooting

If Vulkan selects software renderer (llvmpipe) instead of GPU:
```bash
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json  # adjust for your GPU
```

Enable validation layers for debugging:
```bash
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
```

## Python Bindings

```bash
# Setup virtual environment
python3 -m venv .venv && source .venv/bin/activate
pip install numpy pytest

# Run Python binding tests
PYTHONPATH=manim-cpp/build/python pytest manim-cpp/tests/validation/test_python_compatibility.py

# Quick import test
cd manim-cpp/build/python && python3 -c "import manim_cpp as m; print(m.GPUMesh.create_sphere())"
```

## Architecture Overview

### C++ (`manim-cpp/`)

- **Core Layer** (`src/core/`): GPU memory pool (VMA), compute engine, math types
- **Mobject Hierarchy** (`src/mobject/`): Base Mobject → VMobject → Geometry/3D shapes
- **Renderer** (`src/renderer/`): VulkanRenderer, GPU3DRenderer, ShaderSystem with hot-reload
- **Scene/Animation** (`src/scene/`, `src/animation/`): Scene graph, animation timeline
- **Shaders** (`shaders/`): GLSL compute/vertex/fragment/raytracing → compiled to SPIR-V at build time
- **Culling** (`src/culling/`): LBVH, frustum culling, Hi-Z occlusion, indirect draw generation

### Python (`manim/`)

Standard Manim Community structure:
- `manim/mobject/`: Mobject classes (geometry, text, graphing, 3D)
- `manim/animation/`: Animation classes
- `manim/scene/`: Scene management
- `manim/renderer/`: Cairo and OpenGL renderers
- `manim/camera/`: Camera systems

## Key CMake Options

```bash
-DMANIM_ENABLE_VULKAN=ON        # Primary renderer
-DMANIM_ENABLE_RAYTRACING=ON    # RTX support
-DMANIM_BUILD_TESTS=ON
-DMANIM_BUILD_PYTHON_BINDINGS=ON
```

## Testing

- **C++ unit tests**: GoogleTest in `manim-cpp/tests/unit/`
- **C++ validation/integration**: `manim-cpp/tests/validation/`
- **C++ benchmarks**: Google Benchmark in `manim-cpp/tests/benchmarks/`
- **Python tests**: `tests/` directory (for Python Manim)

All 270 C++ tests pass on NVIDIA RTX 3060 Ti; CPU fallbacks available for non-GPU environments.

## Dependencies

**C++ (via vcpkg)**: Vulkan, GLFW3, GLM, Eigen3, spdlog, CLI11, tomlplusplus, FFmpeg, FreeType, HarfBuzz, GoogleTest, benchmark, pybind11

**Python**: Standard Manim dependencies (numpy, scipy, Pillow, cairo, pango, etc.)
