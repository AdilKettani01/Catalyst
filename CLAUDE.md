# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Catalyst** is a GPU-accelerated C++ animation and rendering library built on Vulkan. It provides a Manim-like API for creating mathematical animations with native C++ performance.

The main library code is in `manim-cpp/`. See `manim-cpp/CLAUDE.md` for detailed architecture and patterns.

## Build Commands

```bash
# Build from manim-cpp directory
cd manim-cpp
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DMANIM_BUILD_TESTS=ON \
  -DMANIM_ENABLE_VULKAN=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release -j$(nproc)

# Run all tests
cd build && ctest --output-on-failure

# Run specific test filter
./bin/manim_tests --gtest_filter="*BezierTessellation*"

# GPU-specific tests
./bin/manim_tests --gtest_filter="GPUComputeTest.*:GPUMemoryTest.*"

# Benchmarks
./bin/manim_benchmarks
```

## Running Examples

```bash
cd manim-cpp/build

# Render shapes to image
./bin/gpu_render_to_file --gpu output.ppm
ffmpeg -y -i output.ppm output.png

# Run text animation
./bin/text_fade_animation
```

## GPU Troubleshooting

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
# Build with Python bindings
cmake -S . -B build -DMANIM_BUILD_PYTHON_BINDINGS=ON ...

# Setup virtual environment
python3 -m venv .venv && source .venv/bin/activate
pip install numpy

# Quick import test
PYTHONPATH=build python -c "import manim_cpp; print('Ready!')"
```

## Architecture Overview

- **Core Layer** (`src/core/`): GPU memory pool (VMA), compute engine, math types
- **Mobject Hierarchy** (`src/mobject/`): Base Mobject -> VMobject -> Geometry/3D/Text
- **Renderer** (`src/renderer/`): VulkanRenderer, GPU3DRenderer, ShaderSystem
- **Scene/Animation** (`src/scene/`, `src/animation/`): Scene graph, animation timeline
- **Shaders** (`shaders/`): GLSL compute/vertex/fragment -> compiled to SPIR-V
- **Examples** (`examples/`): gpu_render_to_file, text_fade_animation

## Key CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `MANIM_ENABLE_VULKAN` | ON | Primary GPU renderer |
| `MANIM_BUILD_TESTS` | OFF | Build unit tests |
| `MANIM_BUILD_PYTHON_BINDINGS` | OFF | Python module |

## Testing

- **Unit tests**: `tests/unit/` (GoogleTest)
- **Integration tests**: `tests/validation/`
- **Benchmarks**: `tests/benchmarks/` (Google Benchmark)

All 270 C++ tests pass. CPU fallbacks available for non-GPU environments.

## Dependencies (via vcpkg)

vulkan-memory-allocator, spdlog, cli11, tomlplusplus, glfw3, glm, eigen3, benchmark, gtest, ffmpeg, freetype, harfbuzz, pybind11

## Project Structure

```
Catalyst/
├── manim-cpp/          # Main C++ library
│   ├── include/        # Public headers
│   ├── src/            # Implementation
│   ├── shaders/        # Vulkan shaders
│   ├── examples/       # Example apps
│   └── tests/          # Test suites
├── logo/               # Project logos
├── scripts/            # Utility scripts
└── README.md           # Documentation
```
