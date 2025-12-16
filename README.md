<p align="center">
  <img src="lib/logo_rounded.png" alt="Catalyst logo" width="600" />
</p>
# Catalyst

**GPU-First Vulkan Renderer with Manim-Style Animation API**

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)
![Linux](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)
![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)

Catalyst is a high-performance graphics library for creating animated mathematical and scientific visualizations. Inspired by [Manim](https://github.com/3b1b/manim), it provides a fluent C++ API for rendering text, shapes, LaTeX formulas, graphs, and 3D objects with smooth animations—all powered by Vulkan for GPU-accelerated rendering.

---

## Features

- **GPU-First Architecture** — Automatic hybrid CPU fallback when GPU is overloaded
- **2D & 3D Rendering** — Full support for both 2D shapes and 3D primitives with depth buffering
- **Text Rendering** — SDF-based text with stroke, gradient, and multi-line support
- **LaTeX Math** — Native LaTeX formula rendering via MicroTeX
- **Rich Animation System**
  - Fade: `show()`, `hide()` with directional slides
  - Transform: `MoveTo()`, `Scale()`, `Rotate()`, `morphTo()`
  - Emphasis: `Indicate()`, `Flash()`, `Wiggle()`, `Pulse()`, `Circumscribe()`
  - Special: `Write()`, `SpiralIn()`, `MoveAlongPath()`
- **31 Easing Functions** — Linear, Cubic, Bounce, Elastic, Back, Smooth, and more
- **Shapes** — Circle, Rectangle, Triangle, Arrow, Polygon, Star, Bezier curves
- **Graphs & Axes** — 2D/3D coordinate systems with function and data plotting
- **Groups & Layout** — Arrange elements in rows, columns, or grids with z-ordering
- **Scene Management** — Multiple independent scenes with transitions
- **Camera Controls** — Pan, zoom, rotate (2D) and orbit, FOV (3D)

---

## Demo

The comprehensive feature demo is located at [`animations/TD1.cpp`](animations/TD1.cpp), showcasing **83 features** across 12 sections.

```bash
# Build and run the demo
./build.sh
./build/anim_TD1
```

<!-- Add a GIF or screenshot here -->
<!-- ![Catalyst Demo](demo.gif) -->

---

## Installation

### Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
    vulkan-sdk \
    libglfw3-dev \
    libcairomm-1.0-dev \
    libpangomm-1.4-dev \
    glslang-tools \
    cmake \
    build-essential
```

### Build from Source

```bash
git clone https://github.com/yourusername/Catalyst.git
cd Catalyst
./build.sh
./build/Catalyst
```

### System-Wide Install

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

This installs:
- `libCatalyst.so` → `/usr/local/lib/`
- Headers (`catalyst.h`, `Catalyst`) → `/usr/local/include/`
- Runtime assets → `/usr/local/share/Catalyst/`
- CMake package config → `/usr/local/lib/cmake/Catalyst/`

---

## Quick Start

```cpp
#include <Catalyst>

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#1a1a2e");

    auto title = window.setText("Hello Catalyst!");
    title.setPosition(Position::CENTER);
    title.setSize(72);
    title.setColor("#FF5733");
    title.show(1.0f);

    window.wait(2.0f);

    title.hide(0.5f, Direction::UP);

    window.run();
    return 0;
}
```

Build with:
```bash
g++ -std=c++20 main.cpp -o app -lCatalyst -lvulkan -lglfw
./app
```

---

## Using as a Library

Catalyst exports a CMake package, making it easy to integrate into your projects.

### Your Project's CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Catalyst CONFIG REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE Catalyst::Catalyst)
```

### Your main.cpp

```cpp
#include <Catalyst>

int main() {
    Catalyst window(1280, 720);
    window.setBackground("#1a1a2e");

    auto hello = window.setText("Hello from an external project!");
    hello.setPosition(Position::CENTER);
    hello.setSize(48);
    hello.setColor("#FFFFFF");
    hello.show(0.5f);

    window.run();
    return 0;
}
```

### Build Your Project

```bash
cmake -S . -B build
cmake --build build
./build/myapp
```

See [`examples/consumer/`](examples/consumer/) for a complete example.

---

## API Overview

### Core Classes

| Class | Description |
|-------|-------------|
| `Catalyst` | Main window and element factory |
| `TextElement` | Text rendering with full animation support |
| `MathElement` | LaTeX formula rendering via MicroTeX |
| `ShapeElement` | 2D geometric shapes (circle, rect, line, etc.) |
| `AxesElement` | 2D coordinate system with tick marks and labels |
| `GraphElement` | Function and data point plotting |
| `Axes3DElement` | 3D coordinate system |
| `Surface3DElement` | 3D surface plots z = f(x, y) |
| `Shape3DElement` | 3D primitives (sphere, cube, cylinder, cone) |
| `Group` | Element grouping with layout and z-ordering |
| `AnimationGroup` | Parallel/sequential animation orchestration |
| `Scene` | Multi-scene support with independent state |

### Enums

| Enum | Values |
|------|--------|
| `Position` | `CENTER`, `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, `TLEFT`, `TRIGHT`, `BLEFT`, `BRIGHT` |
| `Direction` | `NONE`, `UP`, `DOWN`, `LEFT`, `RIGHT` |
| `TextAlignment` | `LEFT`, `CENTER`, `RIGHT` |
| `Easing` | `Linear`, `EaseInQuad`, `EaseOutQuad`, `EaseInOutQuad`, `EaseInCubic`, `EaseOutCubic`, `EaseInOutCubic`, `EaseInQuart`, `EaseOutQuart`, `EaseInOutQuart`, `EaseInQuint`, `EaseOutQuint`, `EaseInOutQuint`, `EaseInSine`, `EaseOutSine`, `EaseInOutSine`, `EaseInExpo`, `EaseOutExpo`, `EaseInOutExpo`, `EaseInCirc`, `EaseOutCirc`, `EaseInOutCirc`, `EaseInBack`, `EaseOutBack`, `EaseInOutBack`, `EaseInElastic`, `EaseOutElastic`, `EaseInOutElastic`, `EaseInBounce`, `EaseOutBounce`, `EaseInOutBounce`, `Smooth`, `Smoother`, `ThereAndBack`, `ThereAndBackWithPause` |

### Full API Documentation

See [`CLAUDE.md`](CLAUDE.md) for the complete API reference with all methods and examples.

---

## Feature Demo: TD1.cpp

The [`animations/TD1.cpp`](animations/TD1.cpp) file demonstrates all Catalyst features across 12 sections:

| Section | Features |
|---------|----------|
| **1. Text & Typography** | Font sizes, positioning (pixel & anchor), hex/RGB/HSL colors, LaTeX math, stroke, gradients, multiline |
| **2. Animations - Fade** | `show()`, `hide()` with directional slides (UP, DOWN, LEFT, RIGHT) |
| **3. Animations - Transform** | `MoveTo()`, `Scale()`, `Rotate()`, `morphTo()` |
| **4. Animations - Showing** | `Write()`, `AddLetterByLetter()`, `DrawBorderThenFill()` |
| **5. Animations - Emphasis** | `Indicate()`, `Flash()`, `Circumscribe()`, `Wiggle()`, `Pulse()`, `FocusOn()` |
| **6. Animations - Movement** | `SpiralIn()`, `SpiralOut()`, `MoveAlongPath()` |
| **7. Shapes - Basic** | Circle, Rectangle, Triangle, Line, Arrow, Polygon, Star, Ellipse, Arc, Dot |
| **8. Shapes - Advanced** | 2D Axes, Function graphs, Tables |
| **9. Scene & Camera** | `setCameraZoom()`, `setCameraPan()`, `setCameraRotate()`, `resetCamera()` |
| **10. 3D Features** | Axes3D, Surface plots, Sphere, Cube, Cylinder, Cone, `orbitCamera()` |
| **11. Timing & Control** | `setEasing()`, `delay()`, `then()`, `repeat()`, `AnimationGroup` |
| **12. Grouping & Hierarchy** | `createGroup()`, `arrange()`, `arrangeInGrid()`, `setZIndex()` |

**Total: 83 features demonstrated**

```bash
./build/anim_TD1
```

---

## Project Structure

```
Catalyst/
├── catalyst.h              # Public API header (1,091 lines)
├── Catalyst                 # Umbrella include header
├── main.cpp                 # Vulkan implementation (19,375 lines)
├── CMakeLists.txt           # Build configuration
├── build.sh                 # Build automation script
├── CLAUDE.md                # Full API documentation
├── animations/
│   ├── TD1.cpp              # Comprehensive feature demo (83 features)
│   └── simple_test.cpp      # Basic hello world example
├── shaders/
│   ├── shader.vert/frag     # Text rendering (SDF)
│   ├── shape.vert/frag      # 2D shapes
│   ├── shape3d.vert/frag    # 3D shapes with lighting
│   └── morph.vert/frag      # Shape morphing
├── font/
│   └── Raleway-*.ttf        # Font family (18 variants)
├── lib/
│   └── MicroTeX/            # LaTeX rendering library
├── examples/
│   └── consumer/            # External project usage example
├── cmake/
│   └── CatalystConfig.cmake.in  # CMake package config template
├── stb_truetype.h           # Font rasterization (bundled)
└── stb_image.h              # Image loading (bundled)
```

---

## Environment Variables

| Variable | Description |
|----------|-------------|
| `CATALYST_RESOURCE_PATH` | Override the runtime asset search path for shaders, fonts, and MicroTeX resources. By default, assets are found relative to `libCatalyst.so` at `share/Catalyst/`. |

Example:
```bash
export CATALYST_RESOURCE_PATH=/opt/catalyst/resources
./myapp
```

---

## Requirements

- **OS:** Linux (uses `/proc/self/exe` for path resolution)
- **Compiler:** GCC 10+ or Clang 12+ with C++20 support
- **CMake:** 3.20+
- **GPU:** Vulkan 1.3 capable graphics card with appropriate drivers

---

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

---

## License

This project is open source. See the LICENSE file for details.

---

## Keywords

`vulkan` `graphics` `renderer` `animation` `manim` `c++` `cpp` `gpu` `visualization` `math` `latex` `3d` `2d` `shapes` `text-rendering` `scientific-visualization` `gpu-accelerated` `real-time` `graphics-library` `animation-library`
