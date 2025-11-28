# Catalyst GPU Library - Project Status

Updated: 2025-11-28

---

## Current Status

| Component | Status |
|-----------|--------|
| Build | Release build clean |
| Tests | 270/270 passing |
| GPU Rendering | Vulkan stable, MSAA enabled |
| Text Rendering | SDF text implemented |
| Animations | FadeIn/FadeOut working |

---

## Quick Start

```bash
cd manim-cpp/build

# Run tests
./bin/manim_tests

# Render shapes to image
./bin/gpu_render_to_file --gpu output.ppm

# Run text animation
./bin/text_fade_animation
```

---

## Recent Completions

- [x] Repository cleanup (Python Manim removed)
- [x] C++ only project structure
- [x] Text animation example added
- [x] Backface culling (CW front face)
- [x] Alpha/opacity through shaders
- [x] Documentation updated

---

## Examples

| Example | Command | Description |
|---------|---------|-------------|
| GPU Render | `./bin/gpu_render_to_file --gpu out.ppm` | Renders shapes to PPM |
| Text Animation | `./bin/text_fade_animation` | TEXT1/2/3 fade sequence |

---

## Build Commands

```bash
cd manim-cpp

# Configure
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DMANIM_ENABLE_VULKAN=ON \
  -DMANIM_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build -j$(nproc)

# Test
cd build && ctest --output-on-failure
```

---

## Future Work

- [ ] Video export (MP4 via FFmpeg)
- [ ] More animation types (Transform, MoveTo, etc.)
- [ ] LaTeX/MathTex rendering
- [ ] Interactive window mode
- [ ] More geometry primitives
