# Catalyst Documentation

**GPU-First Vulkan Renderer for Mathematical Animations**

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Installation & Setup](#2-installation--setup)
3. [Quick Start](#3-quick-start)
4. [Core Concepts](#4-core-concepts)
5. [Text & Typography](#5-text--typography)
6. [LaTeX Formulas](#6-latex-formulas)
7. [Shapes & Geometry](#7-shapes--geometry)
8. [Animation System](#8-animation-system)
9. [Graphs & Data Visualization](#9-graphs--data-visualization)
10. [Grouping & Layout](#10-grouping--layout)
11. [Camera & Scenes](#11-camera--scenes)
12. [3D Rendering](#12-3d-rendering)
13. [SVG Import](#13-svg-import)
14. [API Reference](#14-api-reference)
15. [Examples Gallery](#15-examples-gallery)
16. [Troubleshooting](#16-troubleshooting)

---

## 1. Introduction

### What is Catalyst?

Catalyst is a high-performance GPU-first Vulkan renderer designed for creating mathematical animations and visualizations. Inspired by [Manim](https://www.manim.community/) (the animation engine used by 3Blue1Brown), Catalyst provides a simple C++ API for creating beautiful, smooth animations with text, shapes, graphs, and 3D objects.

### Key Features

| Feature | Description |
|---------|-------------|
| **GPU-First Architecture** | Leverages Vulkan for maximum performance with automatic hybrid CPU fallback |
| **SDF Text Rendering** | Crisp text at any size using Signed Distance Field technology |
| **LaTeX Support** | Render mathematical formulas with MicroTeX |
| **3D Rendering** | Full 3D scene support with surfaces, primitives, and camera controls |
| **Animation System** | 31 easing functions, chaining, looping, and animation groups |
| **Shape Morphing** | Smooth vertex interpolation between shapes |
| **SVG Import** | Load and animate vector graphics |

### Design Philosophy

- **Simple API**: Create complex animations with minimal code
- **Performance First**: GPU-accelerated rendering with intelligent fallback
- **Manim-Inspired**: Familiar concepts for those coming from Manim
- **Modern C++**: Clean, type-safe API using modern C++ features

---

## 2. Installation & Setup

### System Requirements

- **Operating System**: Linux (Ubuntu 20.04+ / Debian 11+ recommended)
- **GPU**: Vulkan 1.3 compatible graphics card
- **Compiler**: GCC 11+ or Clang 14+ with C++17 support
- **CMake**: Version 3.20 or higher

### Dependencies Installation

#### Ubuntu/Debian

```bash
# Update package lists
sudo apt update

# Install Vulkan SDK
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools

# Install GLFW3
sudo apt install libglfw3-dev

# Install Cairo and Pango (for LaTeX rendering)
sudo apt install libcairo2-dev libpango1.0-dev

# Install shader compiler
sudo apt install glslang-tools

# Install build tools
sudo apt install build-essential cmake git
```

#### Verify Vulkan Installation

```bash
# Check Vulkan is working
vulkaninfo | head -20

# You should see your GPU listed
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/your-repo/Catalyst.git
cd Catalyst

# Initialize submodules (for MicroTeX)
git submodule update --init --recursive

# Build
./build.sh

# Run the demo
./build/Catalyst
```

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `CATALYST_RESOURCE_PATH` | Override resource search path | Auto-detected from executable |

---

## 3. Quick Start

### Hello World

Create a file called `hello.cpp`:

```cpp
#include "catalyst.h"

int main() {
    // Create a 1920x1080 window
    Catalyst window(1920, 1080);

    // Create a text element
    auto hello = window.setText("Hello, Catalyst!");
    hello.setSize(72);
    hello.setPosition(Position::CENTER);
    hello.setColor("#FFFFFF");

    // Animate it in
    hello.show(1.0f);

    // Wait, then fade out
    window.wait(2.0f);
    hello.hide(1.0f);

    // Run the render loop
    window.run();
    return 0;
}
```

### Building Your Project

#### Option 1: Add to animations/ directory

Place your `.cpp` file in the `animations/` directory. The build system automatically compiles all files there:

```bash
cp hello.cpp animations/
./build.sh
./build/hello
```

#### Option 2: Link against CatalystLib

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(MyAnimation)

find_package(CatalystLib REQUIRED)
add_executable(my_animation main.cpp)
target_link_libraries(my_animation CatalystLib::CatalystLib)
```

### Project Structure

```
my_project/
├── CMakeLists.txt
├── main.cpp
└── assets/          # Optional: SVGs, custom fonts, etc.
    └── logo.svg
```

---

## 4. Core Concepts

### The Catalyst Window

Every Catalyst program starts with creating a window:

```cpp
Catalyst window(1920, 1080);  // Width x Height in pixels
```

The window manages:
- Vulkan rendering context
- Animation timeline
- All visual elements
- Scene management

### Elements

Catalyst provides several element types:

| Element | Created With | Purpose |
|---------|--------------|---------|
| TextElement | `setText()` | Styled text with stroke, gradient |
| MathElement | `setMath()` | LaTeX formulas |
| ShapeElement | `setCircle()`, `setRectangle()`, etc. | 2D geometric shapes |
| AxesElement | `setAxes()` | 2D coordinate systems |
| GraphElement | `setGraph()` | Function and data plots |
| Axes3DElement | `setAxes3D()` | 3D coordinate systems |
| Surface3DElement | `setSurface()` | 3D surface plots |
| Shape3DElement | `setSphere()`, `setCube3D()`, etc. | 3D primitives |
| Group | `createGroup()` | Container for multiple elements |

### The Show/Hide Paradigm

**Important**: All elements are hidden by default when created. You must call `show()` to make them visible.

```cpp
auto text = window.setText("I'm invisible!");
// Text exists but is not rendered

text.show(1.0f);  // Now it fades in over 1 second
```

This design allows you to set up elements before they appear, enabling complex choreographed animations.

### Animation Timeline

Catalyst uses a timeline-based animation system. Animations are scheduled relative to the current time:

```cpp
auto a = window.setText("First");
a.show(1.0f);              // Shows at t=0, completes at t=1

window.wait(0.5f);         // Advance timeline to t=1.5

auto b = window.setText("Second");
b.show(1.0f);              // Shows at t=1.5, completes at t=2.5
```

### Coordinate Systems

Catalyst supports two positioning modes:

#### Pixel Coordinates
Top-left origin (0,0), X increases rightward, Y increases downward (like CSS):

```cpp
text.setPosition(100, 200);  // 100px from left, 200px from top
```

#### Anchor Presets
Resolution-independent positioning:

```cpp
text.setPosition(Position::CENTER);   // Center of screen
text.setPosition(Position::TLEFT);    // Top-left corner
text.setPosition(Position::BRIGHT);   // Bottom-right corner
```

**Available Anchors:**

| Anchor | Description |
|--------|-------------|
| `CENTER` | Center of screen (default) |
| `TOP` | Top center |
| `BOTTOM` | Bottom center |
| `LEFT` | Left center |
| `RIGHT` | Right center |
| `TLEFT` | Top-left corner |
| `TRIGHT` | Top-right corner |
| `BLEFT` | Bottom-left corner |
| `BRIGHT` | Bottom-right corner |

---

## 5. Text & Typography

### Creating Text

```cpp
auto text = window.setText("Your text here");
```

### Font Size

Size is specified in points (like word processors):

```cpp
text.setSize(24);   // Small text
text.setSize(48);   // Medium text
text.setSize(72);   // Large text
text.setSize(128);  // Very large text
```

The text renderer uses a 128pt reference atlas with SDF, so text remains crisp at any size.

### Positioning

```cpp
// Pixel position (top-left origin)
text.setPosition(100, 200);

// Anchor position
text.setPosition(Position::CENTER);
```

### Colors

Three color formats are supported:

```cpp
// Hex string (with or without #)
text.setColor("#FF5733");
text.setColor("00FF00");

// RGB (0-255 each)
text.setColor(255, 87, 51);

// HSL (hue: 0-360, saturation: 0-1, lightness: 0-1)
text.setColor(14.0f, 0.8f, 0.6f);
```

### Stroke (Outline)

Add an outline to text:

```cpp
text.setStroke(3.0f);              // 3px stroke width
text.setStrokeColor("#000000");    // Black outline
text.setStrokeColor(0, 0, 0);      // RGB format
```

### Gradients

Apply color gradients:

```cpp
// Horizontal gradient (left to right)
text.setGradient("#FF0000", "#0000FF");

// Angled gradient
text.setGradient("#FF0000", "#0000FF", 45.0f);   // 45 degrees
text.setGradient("#FF0000", "#0000FF", 90.0f);   // Vertical

// Remove gradient
text.clearGradient();
```

### Multi-line Text

```cpp
// Explicit line breaks
auto multiline = window.setText("Line 1\nLine 2\nLine 3");

// Line spacing (1.0 = 100% of font height)
multiline.setLineHeight(1.4f);  // 140% spacing

// Text alignment for multi-line
multiline.setAlignment(TextAlignment::LEFT);    // Default
multiline.setAlignment(TextAlignment::CENTER);
multiline.setAlignment(TextAlignment::RIGHT);
```

### Word Wrapping

```cpp
auto paragraph = window.setText("This is a long paragraph that will automatically wrap to the next line when it exceeds the maximum width.");
paragraph.setMaxWidth(400);  // Wrap at 400 pixels
paragraph.setMaxWidth(0);    // Disable wrapping
```

### Text Animation

```cpp
// Simple fade
text.show(1.0f);   // Fade in over 1 second
text.hide(0.5f);   // Fade out over 0.5 seconds

// Directional animation
text.show(1.0f, Direction::UP);     // Slide up while fading in
text.show(1.0f, Direction::LEFT);   // Slide left while fading in
text.hide(1.0f, Direction::DOWN);   // Slide down while fading out

// Custom shift amount (default is 0.1 = 10% of screen)
text.show(1.0f, Direction::UP, 0.2f);  // 20% shift
```

### Text Morphing

Transform one text into another:

```cpp
auto title1 = window.setText("Hello");
title1.setPosition(Position::CENTER);
title1.show(0.5f);

auto title2 = window.setText("World");
title2.setPosition(Position::CENTER);
// title2 stays hidden

window.wait(1.0f);
title1.morphTo(title2, 1.5f);  // Cross-fade with position interpolation
```

---

## 6. LaTeX Formulas

Catalyst uses MicroTeX to render LaTeX mathematical notation.

### Creating Math Elements

```cpp
auto formula = window.setMath("E = mc^2");
formula.setSize(48);
formula.setPosition(Position::CENTER);
formula.setColor("#FFFFFF");
formula.show(1.0f);
```

### Common LaTeX Examples

```cpp
// Fractions
window.setMath("\\frac{a}{b}");

// Square roots
window.setMath("\\sqrt{x^2 + y^2}");

// Integrals
window.setMath("\\int_0^\\infty e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}");

// Summations
window.setMath("\\sum_{n=1}^{\\infty} \\frac{1}{n^2} = \\frac{\\pi^2}{6}");

// Matrices
window.setMath("\\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}");

// Greek letters
window.setMath("\\alpha + \\beta = \\gamma");

// Limits
window.setMath("\\lim_{x \\to 0} \\frac{\\sin x}{x} = 1");
```

### Styling Math

```cpp
auto eq = window.setMath("f(x) = x^2");
eq.setSize(64);
eq.setPosition(Position::CENTER);
eq.setColor("#FFD700");  // Gold color
eq.show(1.0f);
```

### Animation

Math elements support the same animations as text:

```cpp
formula.show(1.0f);
formula.show(1.0f, Direction::UP);
formula.hide(0.5f, Direction::LEFT);
```

---

## 7. Shapes & Geometry

### Basic Shapes

```cpp
// Circle
auto circle = window.setCircle(100);  // radius in pixels

// Rectangle
auto rect = window.setRectangle(200, 100);  // width, height

// Line
auto line = window.setLine(100, 100, 500, 400);  // x1, y1, x2, y2

// Equilateral Triangle
auto triangle = window.setTriangle(150);  // side length

// Arrow
auto arrow = window.setArrow(100, 300, 500, 300);  // x1, y1, x2, y2
```

### Positioning

```cpp
shape.setPosition(400, 300);           // Pixel position
shape.setPosition(Position::CENTER);   // Anchor position
```

### Fill Color

```cpp
shape.setFill("#FF5733");        // Hex
shape.setFill(255, 87, 51);      // RGB
```

### Stroke (Outline)

```cpp
shape.setStroke(3.0f);               // 3px outline
shape.setStrokeColor("#000000");     // Black outline
shape.setStrokeColor(0, 0, 0);       // RGB
```

### Complete Shape Example

```cpp
auto circle = window.setCircle(80);
circle.setPosition(Position::CENTER);
circle.setFill("#4CAF50");           // Green fill
circle.setStroke(4.0f);              // 4px stroke
circle.setStrokeColor("#2E7D32");    // Darker green outline
circle.show(0.5f);
```

### Shape Transformations

Animate shapes with transform methods:

```cpp
// Move to position over 1.5 seconds
shape.MoveTo(1.5f, 960, 540);

// Scale (1.0 = original size)
shape.Scale(1.0f, 2.0f);    // Scale to 2x over 1 second
shape.Scale(0.5f, 0.5f);    // Scale to 50% over 0.5 seconds

// Rotate (degrees)
shape.Rotate(1.5f, 360.0f);  // Full rotation over 1.5 seconds
shape.Rotate(1.0f, -90.0f);  // Rotate -90 degrees over 1 second
```

### Shape Morphing

Morph one shape into another with smooth vertex interpolation:

```cpp
auto circle = window.setCircle(100);
circle.setPosition(Position::LEFT);
circle.setFill("#FF5733");
circle.show(0.5f);

auto rect = window.setRectangle(200, 150);
rect.setPosition(Position::RIGHT);
rect.setFill("#33FF57");
// rect stays hidden

window.wait(1.0f);
circle.morphTo(rect, 2.0f);  // Smooth morph over 2 seconds
```

The morph interpolates:
- Vertex positions
- Fill color
- Position
- Scale

---

## 8. Animation System

### Basic Show/Hide

```cpp
element.show(1.0f);   // Fade in over 1 second
element.hide(0.5f);   // Fade out over 0.5 seconds
```

### Directional Animations

Add movement to show/hide:

```cpp
// Show with direction
element.show(1.0f, Direction::UP);      // Slide up while appearing
element.show(1.0f, Direction::DOWN);    // Slide down
element.show(1.0f, Direction::LEFT);    // Slide left
element.show(1.0f, Direction::RIGHT);   // Slide right

// Hide with direction
element.hide(1.0f, Direction::UP);      // Slide up while disappearing

// Custom shift amount (default 0.1 = 10% of screen in NDC)
element.show(1.0f, Direction::UP, 0.2f);  // 20% shift
```

### Easing Functions

Control the animation curve with 31 easing options:

```cpp
// Apply to show/hide
element.show(1.0f, Easing::EaseOutElastic);
element.show(1.0f, Direction::UP, 0.1f, Easing::EaseOutBack);

// Apply to transforms
shape.MoveTo(1.0f, 500, 300, Easing::EaseInOutCubic);
shape.Scale(0.5f, 2.0f, Easing::EaseOutBounce);
shape.Rotate(1.0f, 360.0f, Easing::EaseInOutQuad);
```

**Available Easing Functions:**

| Category | Ease In | Ease Out | Ease In-Out |
|----------|---------|----------|-------------|
| **Linear** | `Linear` | - | - |
| **Quadratic** | `EaseInQuad` | `EaseOutQuad` | `EaseInOutQuad` |
| **Cubic** | `EaseInCubic` | `EaseOutCubic` (default) | `EaseInOutCubic` |
| **Quartic** | `EaseInQuart` | `EaseOutQuart` | `EaseInOutQuart` |
| **Quintic** | `EaseInQuint` | `EaseOutQuint` | `EaseInOutQuint` |
| **Sinusoidal** | `EaseInSine` | `EaseOutSine` | `EaseInOutSine` |
| **Exponential** | `EaseInExpo` | `EaseOutExpo` | `EaseInOutExpo` |
| **Circular** | `EaseInCirc` | `EaseOutCirc` | `EaseInOutCirc` |
| **Back** | `EaseInBack` | `EaseOutBack` | `EaseInOutBack` |
| **Elastic** | `EaseInElastic` | `EaseOutElastic` | `EaseInOutElastic` |
| **Bounce** | `EaseInBounce` | `EaseOutBounce` | `EaseInOutBounce` |

**Special Easing:**

| Easing | Description |
|--------|-------------|
| `Smooth` | Smoothstep (3t² - 2t³) |
| `Smoother` | Smootherstep (6t⁵ - 15t⁴ + 10t³) |
| `ThereAndBack` | 0 → 1 → 0 |
| `ThereAndBackWithPause` | 0 → 1 (pause) → 0 |

### Setting Default Easing

```cpp
element.setEasing(Easing::EaseOutElastic);
element.show(1.0f);  // Uses EaseOutElastic
element.hide(0.5f);  // Also uses EaseOutElastic
```

### Animation Chaining

#### Using `then()`

Chain animations to run sequentially:

```cpp
shape.show(1.0f);
shape.then().MoveTo(1.0f, 500, 300);
shape.then().Rotate(1.0f, 180.0f);
shape.then().hide(0.5f);
```

#### Using `delay()`

Add delays between animations:

```cpp
element.show(1.0f);
element.delay(2.0f).hide(0.5f);  // Wait 2 seconds, then hide
```

### Looping

```cpp
// Repeat a specific number of times
element.repeat(3).Rotate(1.0f, 360.0f);  // Rotate 3 times

// Loop forever
element.repeatForever().Rotate(2.0f, 360.0f);  // Continuous rotation

// Ping-pong (alternate direction each loop)
element.repeatForever().pingPong().Scale(1.0f, 1.5f);  // Pulse effect

// Stop looping
element.stopLoop();
```

### Animation Groups

Run multiple animations in parallel or sequence:

```cpp
// Parallel group (all at once)
auto parallel = window.createAnimationGroup();
parallel.addShow(element1, 1.0f);
parallel.addShow(element2, 1.0f);
parallel.addShow(element3, 1.0f);
parallel.play();  // All three show simultaneously

// Sequential group (one after another)
auto sequence = window.createSequence();
sequence.addShow(element1, 0.5f);
sequence.addShow(element2, 0.5f);
sequence.addShow(element3, 0.5f);
sequence.play();  // Shows one at a time

// Staggered (partial overlap)
auto staggered = window.createAnimationGroup();
staggered.setLagRatio(0.3f);  // 30% overlap
staggered.addShow(element1, 1.0f);
staggered.addShow(element2, 1.0f);
staggered.addShow(element3, 1.0f);
staggered.play();
```

### Timeline Control

```cpp
window.wait(2.0f);   // Pause timeline for 2 seconds
window.clear();      // Remove all elements and reset timeline
```

---

## 9. Graphs & Data Visualization

### Creating 2D Axes

```cpp
auto axes = window.setAxes(
    -5.0f, 5.0f,    // X range: -5 to 5
    -2.0f, 2.0f     // Y range: -2 to 2
);
axes.setTickSpacing(1.0f, 0.5f);  // X ticks every 1, Y ticks every 0.5
axes.setColor("#FFFFFF");
axes.showLabels(true);   // Show tick labels
axes.showArrows(true);   // Show arrow tips
axes.show(1.0f);
```

### Plotting Functions

Plot mathematical functions using lambdas:

```cpp
// Sine wave
auto sinGraph = window.setGraph(axes, [](float x) {
    return std::sin(x);
}, 200);  // 200 sample points
sinGraph.setColor("#FF5733");
sinGraph.setThickness(3.0f);
sinGraph.show(1.5f);

// Cosine wave
auto cosGraph = window.setGraph(axes, [](float x) {
    return std::cos(x);
}, 200);
cosGraph.setColor("#33FF57");
cosGraph.show(1.0f);

// Parabola
auto parabola = window.setGraph(axes, [](float x) {
    return x * x / 5.0f;  // Scale to fit
}, 100);
parabola.setColor("#5733FF");
parabola.show(1.0f);
```

### Plotting Data Points

```cpp
std::vector<float> dataX = {-4.0f, -2.0f, 0.0f, 2.0f, 4.0f};
std::vector<float> dataY = {1.5f, -1.0f, 0.5f, 1.0f, -0.5f};

auto dataGraph = window.setGraph(axes, dataX, dataY);
dataGraph.setColor("#FFFF00");
dataGraph.setThickness(4.0f);
dataGraph.show(1.0f);
```

### Coordinate Conversion

Convert between data coordinates and pixel coordinates:

```cpp
float pixelX = axes.toPixelX(2.5f);   // Data X → Pixel X
float pixelY = axes.toPixelY(1.0f);   // Data Y → Pixel Y
```

### Complete Graphing Example

```cpp
Catalyst window(1920, 1080);
window.setBackground("#1a1a2e");

// Create axes
auto axes = window.setAxes(-3.14159f, 3.14159f, -1.5f, 1.5f);
axes.setTickSpacing(1.57f, 0.5f);
axes.setColor("#666666");
axes.show(0.5f);

window.wait(0.5f);

// Plot sin and cos
auto sin = window.setGraph(axes, [](float x) { return std::sin(x); }, 200);
sin.setColor("#FF6B6B");
sin.setThickness(3.0f);
sin.show(1.5f);

auto cos = window.setGraph(axes, [](float x) { return std::cos(x); }, 200);
cos.setColor("#4ECDC4");
cos.setThickness(3.0f);
cos.show(1.5f);

// Add labels
auto sinLabel = window.setText("sin(x)");
sinLabel.setColor("#FF6B6B");
sinLabel.setPosition(axes.toPixelX(2.0f), axes.toPixelY(0.9f));
sinLabel.show(0.5f);

window.run();
```

---

## 10. Grouping & Layout

### Creating Groups

Groups allow you to organize multiple elements and manipulate them together:

```cpp
auto group = window.createGroup();
```

### Adding Elements

```cpp
auto circle = window.setCircle(30);
auto label = window.setText("Item");

group.add(circle);
group.add(label);

// Or chain:
group.add(circle).add(label);
```

### Removing Elements

```cpp
group.remove(circle);
group.clear();  // Remove all
```

### Arranging Elements

#### Row Layout

```cpp
auto row = window.createGroup();
for (int i = 0; i < 5; i++) {
    auto item = window.setCircle(25);
    item.setFill("#4CAF50");
    row.add(item);
}
row.arrange(Direction::RIGHT, 15.0f);  // 15px horizontal spacing
row.setPosition(Position::CENTER);
row.show(1.0f);
```

#### Column Layout

```cpp
auto column = window.createGroup();
column.add(window.setText("Header"));
column.add(window.setText("Body"));
column.add(window.setText("Footer"));
column.arrange(Direction::DOWN, 20.0f);  // 20px vertical spacing
column.setPosition(Position::LEFT);
column.show(0.5f);
```

#### Grid Layout

```cpp
auto grid = window.createGroup();
for (int i = 0; i < 9; i++) {
    auto cell = window.setRectangle(80, 80);
    cell.setFill(i % 2 == 0 ? "#2196F3" : "#FF9800");
    grid.add(cell);
}
grid.arrangeInGrid(3, 10.0f, 10.0f);  // 3 columns, 10px spacing
grid.setPosition(Position::CENTER);
grid.show(1.0f);
```

### Nested Groups

Groups can contain other groups:

```cpp
auto innerGroup = window.createGroup();
innerGroup.add(window.setCircle(20));
innerGroup.add(window.setCircle(20));
innerGroup.arrange(Direction::RIGHT, 10.0f);

auto outerGroup = window.createGroup();
outerGroup.add(innerGroup);
outerGroup.add(window.setText("Label"));
outerGroup.arrange(Direction::DOWN, 15.0f);
outerGroup.show(1.0f);
```

### Z-Ordering

Control render order (higher z-index renders on top):

```cpp
auto background = window.setRectangle(400, 300);
background.setFill("#333333");
background.setPosition(Position::CENTER);
// background.setZIndex(0);  // Render first (behind)
background.show(0.5f);

auto foreground = window.setText("On Top");
foreground.setSize(48);
foreground.setColor("#FFFFFF");
foreground.setPosition(Position::CENTER);
// foreground.setZIndex(1);  // Render after (in front)
foreground.show(0.5f);

// For groups:
group.setZIndex(10);  // Sets z-index for all group elements
```

### Group Styling

Apply styles to all elements in a group:

```cpp
group.setColor("#FF5733");           // Set color recursively
group.setOpacity(0.8f);              // Set opacity where supported
group.setFill("#00FF00");            // Set fill for shapes
group.setStroke(2.0f);               // Set stroke width for shapes
group.setStrokeColor("#000000");     // Set stroke color for shapes
```

### Group Animations

Animate entire groups:

```cpp
group.show(1.0f);                        // Show all elements
group.show(1.0f, Direction::UP);         // Show with slide
group.hide(0.5f);                        // Hide all elements
group.MoveTo(2.0f, 500, 300);            // Move entire group
group.Scale(1.0f, 1.5f);                 // Scale all elements
group.Rotate(1.0f, 45.0f);               // Rotate all elements
```

### Group Bounds

Query group dimensions:

```cpp
float width = group.getWidth();
float height = group.getHeight();

float minX, minY, maxX, maxY;
group.getBounds(minX, minY, maxX, maxY);
```

---

## 11. Camera & Scenes

### Background Color

```cpp
window.setBackground("#1a1a2e");      // Hex color
window.setBackground(26, 26, 46);     // RGB
```

### 2D Camera Controls

#### Instant Camera Changes

```cpp
window.setCameraZoom(2.0f);           // 2x zoom
window.setCameraPan(0.2f, 0.0f);      // Pan right (NDC units)
window.setCameraRotate(45.0f);        // Rotate 45 degrees
window.resetCamera();                  // Reset to default
```

#### Animated Camera Changes

```cpp
window.setCameraZoom(1.5f, 1.0f);           // Zoom over 1 second
window.setCameraPan(0.2f, 0.0f, 0.5f);      // Pan over 0.5 seconds
window.setCameraRotate(30.0f, 1.0f);        // Rotate over 1 second
window.resetCamera(0.5f);                    // Reset over 0.5 seconds
```

### HUD Elements (Fixed in Frame)

Keep elements fixed regardless of camera movement:

```cpp
auto label = window.setText("Score: 100");
label.setPosition(Position::TRIGHT);
label.fix_in_frame();  // Ignores camera pan/zoom/rotate
label.show(0.5f);
```

### Multiple Scenes

Create separate scenes with independent elements and cameras:

```cpp
// Create scenes
auto scene1 = window.createScene();
scene1.setBackground("#1a1a2e");

auto title1 = scene1.setText("Scene 1");
title1.setPosition(Position::CENTER);
title1.show(1.0f);

auto scene2 = window.createScene();
scene2.setBackground("#2e1a1a");

auto title2 = scene2.setText("Scene 2");
title2.setPosition(Position::CENTER);
title2.show(1.0f);

// Switch between scenes
window.setActiveScene(scene1);
window.wait(3.0f);
window.setActiveScene(scene2);

window.run();
```

### Scene Methods

Each scene has its own element creation methods:

```cpp
auto text = scene.setText("Scene-specific text");
auto shape = scene.setCircle(50);
scene.setBackground("#000000");
scene.setCameraZoom(1.5f, 1.0f);
scene.wait(2.0f);
```

---

## 12. 3D Rendering

### Enabling 3D Mode

```cpp
window.set3DMode(true);
```

This activates:
- Depth buffer for proper occlusion
- 3D projection matrix
- 3D camera controls
- 3D lighting system

### 3D Coordinate Axes

```cpp
auto axes3D = window.setAxes3D(
    -2.0f, 2.0f,   // X range
    -2.0f, 2.0f,   // Y range
    -1.0f, 1.0f    // Z range
);
axes3D.setColor("#AAAAAA");
axes3D.setTickSpacing(1.0f, 1.0f, 0.5f);
axes3D.showLabels(true);
axes3D.showArrows(true);
axes3D.setOpacity(0.8f);
axes3D.show(0.5f);
```

### 3D Primitives

```cpp
// Sphere
auto sphere = window.setSphere(0.5f);  // radius
sphere.setPosition(0.0f, 1.0f, 0.0f);
sphere.setColor("#00FFAA");
sphere.show(0.5f);

// Cube
auto cube = window.setCube3D(1.0f);    // size
cube.setPosition(-1.5f, 0.5f, 0.0f);
cube.setColor("#FF5733");
cube.show(0.5f);

// Cylinder
auto cylinder = window.setCylinder(0.3f, 1.0f);  // radius, height
cylinder.setPosition(1.5f, 0.5f, 0.0f);
cylinder.setColor("#33FF57");
cylinder.show(0.5f);

// Cone
auto cone = window.setCone(0.4f, 1.0f);  // radius, height
cone.setPosition(0.0f, 0.0f, 1.5f);
cone.setColor("#FF33AA");
cone.show(0.5f);

// 3D Arrow
auto arrow3D = window.setArrow3D(
    0.0f, 0.0f, 0.0f,    // Start point
    1.0f, 1.0f, 1.0f     // End point
);
arrow3D.setColor("#FFFF00");
arrow3D.show(0.5f);
```

### 3D Surfaces

Plot z = f(x, y) surfaces:

```cpp
auto surface = window.setSurface(
    [](float x, float y) {
        return std::sin(x) * std::cos(y);
    },
    -2.0f, 2.0f,   // X range
    -2.0f, 2.0f    // Y range
);
surface.setColor("#5599FF");
surface.setOpacity(0.8f);
surface.setWireframe(false);        // Solid surface
surface.setResolution(50, 50);      // Tessellation detail
surface.show(1.0f);
```

### 3D Shape Properties

```cpp
// Position
shape3D.setPosition(1.0f, 2.0f, 0.5f);

// Rotation (Euler angles in degrees)
shape3D.setRotation(45.0f, 0.0f, 30.0f);  // rx, ry, rz

// Scale
shape3D.setScale(1.5f);                    // Uniform
shape3D.setScale(1.0f, 2.0f, 0.5f);        // Non-uniform

// Color and opacity
shape3D.setColor("#FF5733");
shape3D.setOpacity(0.7f);
```

### 3D Shape Animations

```cpp
// Move to position
sphere.MoveTo(1.5f, 1.0f, 2.0f, 0.0f);  // duration, x, y, z

// Scale animation
sphere.ScaleTo(1.0f, 2.0f);              // duration, scale

// Rotation animation
sphere.RotateTo(2.0f, 0.0f, 180.0f, 0.0f);  // duration, rx, ry, rz
```

### 3D Camera Controls

#### Basic Camera Setup

```cpp
// Set camera position and target
window.setCamera3D(
    5.0f, 3.0f, 5.0f,    // Eye position (x, y, z)
    0.0f, 0.0f, 0.0f     // Target (look at)
);

// Animated camera movement
window.setCamera3D(
    7.0f, 4.0f, 0.0f,    // New eye position
    0.0f, 0.0f, 0.0f,    // Target
    2.0f                  // Duration in seconds
);
```

#### Field of View

```cpp
window.setCameraFOV(45.0f);           // Instant
window.setCameraFOV(30.0f, 1.5f);     // Animated (zoom in)
window.setCameraFOV(60.0f, 1.5f);     // Animated (zoom out)
```

#### Orbit Camera

Position camera using spherical coordinates:

```cpp
// theta: azimuthal angle, phi: polar angle, distance: from target
window.orbitCamera(0.785f, 1.047f, 6.0f);           // Instant
window.orbitCamera(1.57f, 0.785f, 8.0f, 2.0f);      // Animated
```

#### Advanced Camera Controls

```cpp
// Move camera target (camera follows)
window.setCamera3DTarget(0.0f, 1.0f, 0.0f, 1.0f);

// Shift camera and target together
window.shiftCamera3D(1.0f, 0.0f, 0.0f, 0.5f);  // dx, dy, dz, duration

// Distance controls
window.setCamera3DDistance(10.0f, 1.0f);       // Set absolute distance
window.scaleCamera3DDistance(0.8f, 1.0f);      // Zoom in (scale < 1)
window.scaleCamera3DDistance(1.5f, 1.0f);      // Zoom out (scale > 1)

// Rotate around target
window.rotateCamera3D(0.0f, 1.0f, 0.0f, 0.5f, 1.0f);  // axis, angle, duration

// Reorient camera (degrees)
window.reorientCamera(-30.0f, 75.0f, 1.0f);    // theta, phi, duration

// Reset camera
window.resetCamera3D(1.5f);
```

#### Ambient Camera Rotation

Continuous orbit animation:

```cpp
window.beginAmbientCameraRotation(0.1f);  // radians per second
// ... later ...
window.stopAmbientCameraRotation();
```

### Mixed 2D/3D Rendering

2D elements render as HUD overlays on top of 3D content:

```cpp
window.set3DMode(true);

// 3D content
auto sphere = window.setSphere(1.0f);
sphere.show(0.5f);

// 2D overlay
auto label = window.setText("3D Demo");
label.setPosition(Position::TOP);
label.setSize(48);
label.setColor("#FFFFFF");
label.fix_in_frame();  // HUD mode
label.show(0.5f);
```

---

## 13. SVG Import

Load and animate vector graphics:

### Basic SVG Loading

```cpp
auto logo = window.SVGMobject("assets/logo.svg");
logo.setPosition(Position::CENTER);
logo.show(1.0f);
```

### Scaling to Target Height

```cpp
auto icon = window.SVGMobject("assets/icon.svg", 200);  // Scale to 200px tall
icon.setPosition(Position::LEFT);
icon.show(0.5f);
```

### Styling SVGs

SVGs are loaded as Groups, so you can style them:

```cpp
auto svg = window.SVGMobject("assets/graphic.svg", 150);
svg.setFill("#FFFFFF");          // Set fill color
svg.setStroke(2.0f);             // Add stroke
svg.setStrokeColor("#000000");
svg.setOpacity(0.9f);
svg.show(1.0f);
```

### Animating SVGs

```cpp
auto svg = window.SVGMobject("assets/arrow.svg", 100);
svg.setPosition(Position::LEFT);
svg.show(0.5f);

svg.MoveTo(2.0f, 960, 540);  // Move to center
svg.Scale(1.0f, 1.5f);       // Scale up
svg.Rotate(1.0f, 360.0f);    // Full rotation
```

---

## 14. API Reference

### Catalyst Class

| Method | Description |
|--------|-------------|
| `Catalyst(width, height)` | Create window with resolution |
| `setText(text)` | Create TextElement |
| `setMath(latex)` | Create MathElement |
| `setCircle(radius)` | Create circle ShapeElement |
| `setRectangle(w, h)` | Create rectangle ShapeElement |
| `setLine(x1, y1, x2, y2)` | Create line ShapeElement |
| `setTriangle(size)` | Create triangle ShapeElement |
| `setArrow(x1, y1, x2, y2)` | Create arrow ShapeElement |
| `setAxes(xMin, xMax, yMin, yMax)` | Create 2D AxesElement |
| `setGraph(axes, func, points)` | Create function GraphElement |
| `setGraph(axes, xData, yData)` | Create data GraphElement |
| `createGroup()` | Create empty Group |
| `createAnimationGroup()` | Create parallel AnimationGroup |
| `createSequence()` | Create sequential AnimationGroup |
| `SVGMobject(path)` | Import SVG as Group |
| `SVGMobject(path, height)` | Import and scale SVG |
| `wait(seconds)` | Delay timeline |
| `clear()` | Clear all elements |
| `run()` | Start render loop |
| `setBackground(hex)` | Set background color |
| `setBackground(r, g, b)` | Set background color RGB |
| `setCameraZoom(zoom)` | Set 2D camera zoom |
| `setCameraPan(x, y)` | Set 2D camera pan |
| `setCameraRotate(degrees)` | Set 2D camera rotation |
| `resetCamera()` | Reset 2D camera |
| `createScene()` | Create new Scene |
| `setActiveScene(scene)` | Switch to scene |
| `set3DMode(enable)` | Enable/disable 3D |
| `setAxes3D(...)` | Create 3D axes |
| `setSurface(func, ...)` | Create 3D surface |
| `setSphere(radius)` | Create 3D sphere |
| `setCube3D(size)` | Create 3D cube |
| `setCylinder(r, h)` | Create 3D cylinder |
| `setCone(r, h)` | Create 3D cone |
| `setArrow3D(...)` | Create 3D arrow |
| `setCamera3D(...)` | Set 3D camera |
| `orbitCamera(...)` | Spherical camera position |

### TextElement Methods

| Method | Description |
|--------|-------------|
| `setSize(points)` | Set font size |
| `setPosition(x, y)` | Set pixel position |
| `setPosition(anchor)` | Set anchor position |
| `setColor(hex/rgb/hsl)` | Set text color |
| `setStroke(width)` | Set stroke width |
| `setStrokeColor(...)` | Set stroke color |
| `setGradient(start, end)` | Set gradient |
| `setGradient(start, end, angle)` | Set angled gradient |
| `clearGradient()` | Remove gradient |
| `setLineHeight(mult)` | Set line spacing |
| `setMaxWidth(pixels)` | Enable word wrap |
| `setAlignment(align)` | Set text alignment |
| `show(duration)` | Fade in |
| `show(duration, dir)` | Show with direction |
| `hide(duration)` | Fade out |
| `hide(duration, dir)` | Hide with direction |
| `morphTo(target, duration)` | Morph to another text |
| `setEasing(easing)` | Set default easing |
| `delay(seconds)` | Delay next animation |
| `then()` | Chain animation |
| `repeat(count)` | Repeat N times |
| `repeatForever()` | Loop indefinitely |
| `pingPong()` | Alternate direction |
| `stopLoop()` | Cancel looping |
| `fix_in_frame()` | HUD mode (ignore camera) |

### ShapeElement Methods

| Method | Description |
|--------|-------------|
| `setPosition(x, y)` | Set pixel position |
| `setPosition(anchor)` | Set anchor position |
| `setFill(hex/rgb)` | Set fill color |
| `setStroke(width)` | Set stroke width |
| `setStrokeColor(...)` | Set stroke color |
| `show(duration)` | Fade in |
| `hide(duration)` | Fade out |
| `MoveTo(dur, x, y)` | Animate to position |
| `Scale(dur, scale)` | Animate scale |
| `Rotate(dur, degrees)` | Animate rotation |
| `morphTo(target, dur)` | Morph to another shape |

### Shape3DElement Methods

| Method | Description |
|--------|-------------|
| `setPosition(x, y, z)` | Set 3D position |
| `setRotation(rx, ry, rz)` | Set rotation (degrees) |
| `setScale(s)` | Set uniform scale |
| `setScale(sx, sy, sz)` | Set non-uniform scale |
| `setColor(hex/rgb)` | Set color |
| `setOpacity(opacity)` | Set opacity (0-1) |
| `show(duration)` | Fade in |
| `hide(duration)` | Fade out |
| `MoveTo(dur, x, y, z)` | Animate to position |
| `ScaleTo(dur, scale)` | Animate scale |
| `RotateTo(dur, rx, ry, rz)` | Animate rotation |

### Group Methods

| Method | Description |
|--------|-------------|
| `add(element)` | Add element to group |
| `remove(element)` | Remove element from group |
| `clear()` | Remove all elements |
| `size()` | Get element count |
| `empty()` | Check if empty |
| `setPosition(x, y)` | Set group center |
| `setPosition(anchor)` | Set group anchor |
| `arrange(dir, spacing)` | Arrange in row/column |
| `arrangeInGrid(cols, h, v)` | Arrange in grid |
| `setZIndex(z)` | Set render order |
| `setColor(...)` | Set color recursively |
| `setFill(...)` | Set fill for shapes |
| `setStroke(width)` | Set stroke for shapes |
| `show(duration)` | Show all elements |
| `hide(duration)` | Hide all elements |
| `MoveTo(dur, x, y)` | Move group |
| `Scale(dur, scale)` | Scale group |
| `Rotate(dur, degrees)` | Rotate group |
| `getWidth()` | Get group width |
| `getHeight()` | Get group height |
| `getBounds(...)` | Get bounding box |

### Enums

#### Position
`CENTER`, `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, `TLEFT`, `TRIGHT`, `BLEFT`, `BRIGHT`

#### Direction
`NONE`, `UP`, `DOWN`, `LEFT`, `RIGHT`

#### TextAlignment
`LEFT`, `CENTER`, `RIGHT`

#### Easing
`Linear`, `EaseInQuad`, `EaseOutQuad`, `EaseInOutQuad`, `EaseInCubic`, `EaseOutCubic`, `EaseInOutCubic`, `EaseInQuart`, `EaseOutQuart`, `EaseInOutQuart`, `EaseInQuint`, `EaseOutQuint`, `EaseInOutQuint`, `EaseInSine`, `EaseOutSine`, `EaseInOutSine`, `EaseInExpo`, `EaseOutExpo`, `EaseInOutExpo`, `EaseInCirc`, `EaseOutCirc`, `EaseInOutCirc`, `EaseInBack`, `EaseOutBack`, `EaseInOutBack`, `EaseInElastic`, `EaseOutElastic`, `EaseInOutElastic`, `EaseInBounce`, `EaseOutBounce`, `EaseInOutBounce`, `Smooth`, `Smoother`, `ThereAndBack`, `ThereAndBackWithPause`

---

## 15. Examples Gallery

### Example 1: Text Animation Showcase

```cpp
#include "catalyst.h"

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#0a0a1a");

    // Title with gradient
    auto title = window.setText("Welcome to Catalyst");
    title.setSize(72);
    title.setPosition(Position::TOP);
    title.setGradient("#FF6B6B", "#4ECDC4");
    title.show(1.0f, Direction::DOWN);

    window.wait(0.5f);

    // Subtitle
    auto subtitle = window.setText("GPU-First Vulkan Renderer");
    subtitle.setSize(36);
    subtitle.setPosition(Position::CENTER);
    subtitle.setColor("#888888");
    subtitle.show(1.0f, Direction::UP);

    window.wait(2.0f);

    // Morph text
    auto newTitle = window.setText("Let's Create!");
    newTitle.setSize(72);
    newTitle.setPosition(Position::TOP);
    newTitle.setGradient("#FFD93D", "#6BCB77");

    title.morphTo(newTitle, 1.5f);

    window.run();
    return 0;
}
```

### Example 2: Mathematical Visualization

```cpp
#include "catalyst.h"
#include <cmath>

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#1a1a2e");

    // Create coordinate axes
    auto axes = window.setAxes(-3.14159f, 3.14159f, -1.5f, 1.5f);
    axes.setColor("#444444");
    axes.setTickSpacing(1.57f, 0.5f);
    axes.show(0.5f);

    window.wait(0.3f);

    // Formula
    auto formula = window.setMath("f(x) = \\sin(x)");
    formula.setSize(48);
    formula.setPosition(Position::TRIGHT);
    formula.setColor("#FF6B6B");
    formula.show(0.5f);

    window.wait(0.3f);

    // Animated graph drawing
    auto graph = window.setGraph(axes, [](float x) {
        return std::sin(x);
    }, 200);
    graph.setColor("#FF6B6B");
    graph.setThickness(3.0f);
    graph.show(2.0f);  // Draw animation

    window.wait(1.0f);

    // Add second function
    auto formula2 = window.setMath("g(x) = \\cos(x)");
    formula2.setSize(48);
    formula2.setPosition(axes.toPixelX(2.5f), axes.toPixelY(1.2f));
    formula2.setColor("#4ECDC4");
    formula2.show(0.5f);

    auto graph2 = window.setGraph(axes, [](float x) {
        return std::cos(x);
    }, 200);
    graph2.setColor("#4ECDC4");
    graph2.setThickness(3.0f);
    graph2.show(2.0f);

    window.run();
    return 0;
}
```

### Example 3: Shape Morphing Demo

```cpp
#include "catalyst.h"

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#16213e");

    // Create shapes
    auto circle = window.setCircle(80);
    circle.setPosition(Position::LEFT);
    circle.setFill("#e94560");
    circle.show(0.5f);

    auto square = window.setRectangle(140, 140);
    square.setPosition(Position::CENTER);
    square.setFill("#0f3460");

    auto triangle = window.setTriangle(160);
    triangle.setPosition(Position::RIGHT);
    triangle.setFill("#533483");

    // Label
    auto label = window.setText("Shape Morphing");
    label.setSize(48);
    label.setPosition(Position::TOP);
    label.setColor("#ffffff");
    label.show(0.5f);

    window.wait(1.0f);

    // Morph circle to square
    circle.morphTo(square, 1.5f);

    window.wait(2.0f);

    // Morph square to triangle
    square.morphTo(triangle, 1.5f);

    window.wait(2.0f);

    // Hide
    triangle.hide(1.0f, Direction::DOWN);
    label.hide(0.5f, Direction::UP);

    window.run();
    return 0;
}
```

### Example 4: 3D Surface Plot

```cpp
#include "catalyst.h"
#include <cmath>

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#0d1117");
    window.set3DMode(true);

    // 3D axes
    auto axes = window.setAxes3D(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    axes.setColor("#30363d");
    axes.show(0.5f);

    // Surface: z = sin(sqrt(x² + y²))
    auto surface = window.setSurface(
        [](float x, float y) {
            float r = std::sqrt(x*x + y*y);
            return std::sin(r * 2.0f) * std::exp(-r * 0.3f);
        },
        -2.0f, 2.0f, -2.0f, 2.0f
    );
    surface.setColor("#58a6ff");
    surface.setOpacity(0.85f);
    surface.setResolution(60, 60);
    surface.show(1.5f);

    // Title overlay
    auto title = window.setText("Damped Ripple: z = sin(2r) * e^(-0.3r)");
    title.setSize(36);
    title.setPosition(Position::TOP);
    title.setColor("#ffffff");
    title.fix_in_frame();
    title.show(0.5f);

    // Animate camera
    window.setCamera3D(6.0f, 4.0f, 6.0f, 0.0f, 0.0f, 0.0f);
    window.wait(1.0f);

    // Orbit
    window.beginAmbientCameraRotation(0.15f);

    window.run();
    return 0;
}
```

### Example 5: Complex Scene

```cpp
#include "catalyst.h"

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#1e1e2e");

    // Header group
    auto header = window.createGroup();
    auto title = window.setText("Catalyst Features");
    title.setSize(64);
    title.setColor("#cdd6f4");
    auto line = window.setLine(0, 0, 400, 0);
    line.setFill("#89b4fa");
    header.add(title).add(line);
    header.arrange(Direction::DOWN, 20.0f);
    header.setPosition(Position::TOP);
    header.show(1.0f, Direction::DOWN);

    window.wait(0.5f);

    // Feature cards
    std::vector<std::string> features = {
        "GPU-First", "SDF Text", "LaTeX", "3D Scenes"
    };

    auto grid = window.createGroup();
    for (const auto& feat : features) {
        auto card = window.createGroup();

        auto bg = window.setRectangle(200, 120);
        bg.setFill("#313244");
        bg.setStroke(2.0f);
        bg.setStrokeColor("#45475a");

        auto label = window.setText(feat);
        label.setSize(24);
        label.setColor("#cdd6f4");

        card.add(bg).add(label);
        grid.add(card);
    }
    grid.arrangeInGrid(2, 30.0f, 30.0f);
    grid.setPosition(Position::CENTER);

    // Staggered animation
    auto anim = window.createAnimationGroup();
    anim.setLagRatio(0.2f);
    for (size_t i = 0; i < grid.size(); i++) {
        // Animate each card
    }
    grid.show(1.5f);

    window.wait(2.0f);

    // Camera effect
    window.setCameraZoom(1.1f, 1.0f);
    window.wait(0.5f);
    window.resetCamera(0.5f);

    window.run();
    return 0;
}
```

---

## 16. Troubleshooting

### Build Issues

#### Missing Vulkan SDK

```
Error: Could not find Vulkan
```

**Solution:**
```bash
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers-dev
```

#### Missing glslangValidator

```
Error: glslangValidator not found
```

**Solution:**
```bash
sudo apt install glslang-tools
```

#### MicroTeX Build Fails

```
Error: MicroTeX compilation error
```

**Solution:**
```bash
# Ensure submodules are initialized
git submodule update --init --recursive

# Install Cairo/Pango dependencies
sudo apt install libcairo2-dev libpango1.0-dev
```

### Runtime Issues

#### Vulkan Validation Errors

```
Validation Error: [VUID-...]
```

**Solution:** These are often warnings. For release builds, validation layers are disabled. If errors persist, ensure your GPU drivers are up to date.

#### Font Not Found

```
Error: Failed to load font
```

**Solution:** Ensure the font directory is accessible:
```bash
# Check font path
ls build/font/

# Or set custom resource path
export CATALYST_RESOURCE_PATH=/path/to/resources
```

#### Black Screen / No Output

**Possible causes:**
1. Elements not shown (forgot to call `show()`)
2. Color same as background
3. Position off-screen

**Debug:**
```cpp
// Check element is visible
element.setPosition(Position::CENTER);  // Known good position
element.setColor("#FFFFFF");            // High contrast
element.show(1.0f);                     // Required!
```

### Performance Tips

#### Optimize for Large Scenes

1. **Reuse elements** instead of creating new ones
2. **Use groups** to batch similar elements
3. **Limit animation complexity** during intensive scenes
4. **Reduce 3D surface resolution** if not needed:
   ```cpp
   surface.setResolution(30, 30);  // Lower = faster
   ```

#### GPU Monitoring

Catalyst automatically monitors GPU load and enables hybrid CPU fallback when overloaded (>33ms for 10+ consecutive frames).

To check GPU status:
```bash
# Monitor GPU usage
nvidia-smi -l 1  # For NVIDIA
```

### Getting Help

- **GitHub Issues:** Report bugs and request features
- **Documentation:** Check this guide and API reference
- **Examples:** Study the TD1.cpp demo (1,597 lines of examples)

---

## License

Catalyst is open source software. See the LICENSE file for details.

---

*Generated for Catalyst v1.0 - GPU-First Vulkan Renderer*
