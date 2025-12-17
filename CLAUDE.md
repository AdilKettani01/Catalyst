# Catalyst - GPU-First Vulkan Renderer

## Project Overview

Catalyst is a Vulkan-based graphics application featuring GPU-first architecture with automatic hybrid CPU fallback when the GPU becomes overloaded.

## Build & Run

```bash
# Build
./build.sh

# Run (from any directory)
./build/Catalyst
```

## Library API (catalyst.h)

### Basic Usage

```cpp
#include "catalyst.h"

int main() {
    Catalyst window(1920, 1080);

    // Create multiple independent text elements
    auto title = window.setText("Hello World");
    title.setSize(72);
    title.setPosition(Position::TOP);
    title.setColor("#FF5733");
    title.show(1.0f);

    auto subtitle = window.setText("Welcome to Catalyst");
    subtitle.setSize(36);
    subtitle.setPosition(Position::CENTER);
    subtitle.setColor(100, 200, 255);
    subtitle.show(2.0f);

    window.run();
    return 0;
}
```

### Catalyst Class

| Method | Description |
|--------|-------------|
| `Catalyst(uint32_t width, uint32_t height)` | Constructor - creates window with resolution |
| `TextElement setText(const std::string& text)` | Creates text element and returns it |
| `MathElement setMath(const std::string& latex)` | Creates LaTeX formula element and returns it |
| `ShapeElement setCircle(float radius)` | Creates circle with given radius in pixels |
| `ShapeElement setRectangle(float width, float height)` | Creates rectangle with given dimensions |
| `ShapeElement setLine(float x1, float y1, float x2, float y2)` | Creates line between two points |
| `ShapeElement setTriangle(float size)` | Creates equilateral triangle with given side length |
| `ShapeElement setArrow(float x1, float y1, float x2, float y2)` | Creates arrow from (x1,y1) to (x2,y2) |
| `AxesElement setAxes(float xMin, float xMax, float yMin, float yMax)` | Creates 2D coordinate axes |
| `GraphElement setGraph(AxesElement& axes, const std::vector<float>& x, const std::vector<float>& y)` | Creates graph from data points |
| `GraphElement setGraph(AxesElement& axes, std::function<float(float)> func, int numPoints)` | Creates graph from function |
| `void wait(float seconds)` | Schedule delay in animation timeline |
| `void clear()` | Clear all elements and reset timeline |
| `void run()` | Starts the Vulkan render loop |
| `~Catalyst()` | Destructor - cleans up resources |
| `void setBackground(const std::string& hex)` | Set background color via hex |
| `void setBackground(int r, int g, int b)` | Set background color via RGB (0-255) |
| `void setCameraZoom(float zoom)` | Set camera zoom instantly |
| `void setCameraZoom(float zoom, float duration)` | Animate camera zoom |
| `void setCameraPan(float x, float y)` | Set camera pan instantly (NDC units) |
| `void setCameraPan(float x, float y, float duration)` | Animate camera pan |
| `void setCameraRotate(float degrees)` | Set camera rotation instantly |
| `void setCameraRotate(float degrees, float duration)` | Animate camera rotation |
| `void resetCamera()` | Reset camera to default |
| `void resetCamera(float duration)` | Animate camera reset |
| `Scene createScene()` | Create a new scene |
| `void setActiveScene(Scene& scene)` | Switch to a scene |
| `void setActiveScene(size_t index)` | Switch to scene by index |
| `void set3DMode(bool enable)` | Enable 3D rendering for current scene |
| `Axes3DElement setAxes3D(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)` | Creates 3D coordinate axes |
| `Surface3DElement setSurface(func, xMin, xMax, yMin, yMax)` | Creates 3D surface from height function z=f(x,y) |
| `Shape3DElement setSphere(float radius)` | Creates 3D sphere |
| `Shape3DElement setCube3D(float size)` | Creates 3D cube |
| `Shape3DElement setCylinder(float radius, float height)` | Creates 3D cylinder |
| `Shape3DElement setCone(float radius, float height)` | Creates 3D cone |
| `Shape3DElement setArrow3D(x1, y1, z1, x2, y2, z2)` | Creates 3D arrow |
| `void setCamera3D(eyeX, eyeY, eyeZ, targetX, targetY, targetZ)` | Set 3D camera position and target |
| `void setCamera3D(eye..., target..., float duration)` | Animate 3D camera movement |
| `void setCameraFOV(float fov)` | Set field of view in degrees |
| `void setCameraFOV(float fov, float duration)` | Animate FOV change |
| `void orbitCamera(float theta, float phi, float distance)` | Position camera using spherical coordinates |
| `void orbitCamera(theta, phi, distance, float duration)` | Animate camera orbit |
| `void resetCamera3D()` | Reset 3D camera to default |
| `void resetCamera3D(float duration)` | Animate 3D camera reset |
| `void setCamera3DTarget(float x, float y, float z)` | Set 3D camera target (preserve offset) |
| `void setCamera3DTarget(x, y, z, float duration)` | Animate camera target move |
| `void shiftCamera3D(float dx, float dy, float dz)` | Shift camera + target by delta |
| `void shiftCamera3D(dx, dy, dz, float duration)` | Animate camera shift |
| `void setCamera3DDistance(float distance)` | Set camera distance to target |
| `void setCamera3DDistance(distance, float duration)` | Animate camera distance |
| `void scaleCamera3DDistance(float scale)` | Scale camera distance (zoom) |
| `void scaleCamera3DDistance(scale, float duration)` | Animate distance scaling |
| `void rotateCamera3D(axisX, axisY, axisZ, float angleRadians)` | Rotate camera around target |
| `void rotateCamera3D(axisX, axisY, axisZ, angleRadians, float duration)` | Animate camera rotation |
| `void reorientCamera(float thetaDeg, float phiDeg)` | Reorient camera (degrees) preserving distance |
| `void reorientCamera(thetaDeg, phiDeg, float duration)` | Animate reorientation |
| `void beginAmbientCameraRotation(float rateRadians)` | Start continuous orbit (rad/sec) |
| `void stopAmbientCameraRotation()` | Stop ambient orbit |

### TextElement Class

| Method | Description |
|--------|-------------|
| `void setSize(float points)` | Set font size in points (e.g., 72 for 72pt) |
| `void setPosition(float x, float y)` | Set position in pixels from top-left |
| `void setPosition(Position anchor)` | Set position using anchor preset |
| `void setColor(const std::string& hex)` | Set color via hex string ("#FF5733" or "FF5733") |
| `void setColor(int r, int g, int b)` | Set color via RGB (0-255 each) |
| `void setColor(float h, float s, float l)` | Set color via HSL (h: 0-360, s/l: 0-1) |
| `void setStroke(float width)` | Set stroke/outline width in pixels |
| `void setStrokeColor(const std::string& hex)` | Set stroke color via hex string |
| `void setStrokeColor(int r, int g, int b)` | Set stroke color via RGB (0-255 each) |
| `void setGradient(start, end)` | Set horizontal gradient (two hex colors) |
| `void setGradient(start, end, angle)` | Set gradient with angle in degrees (0=horizontal, 90=vertical) |
| `void clearGradient()` | Remove gradient, return to solid color |
| `void setLineHeight(float multiplier)` | Line spacing (1.2 = 120% of font size) |
| `void setMaxWidth(float pixels)` | Enable word wrap at width (0 = disable) |
| `void setAlignment(TextAlignment align)` | Text alignment (LEFT, CENTER, RIGHT) |
| `void show(float duration)` | Show element (fade in) over duration (seconds) |
| `void show(float duration, Direction dir)` | Show with directional slide (default 10% shift) |
| `void show(float duration, Direction dir, float shift)` | Show with custom shift amount (NDC units) |
| `void hide(float duration)` | Hide element (fade out) over duration (seconds) |
| `void hide(float duration, Direction dir)` | Hide with directional slide |
| `void hide(float duration, Direction dir, float shift)` | Hide with custom shift amount |
| `void morphTo(TextElement& target, float duration)` | Morph into another text (cross-fade with position) |

**Note:** Elements are hidden by default when created. Call `show()` to make them visible.

Each `TextElement` is independent - multiple texts can have different sizes, positions, colors, and animations running simultaneously.

### MathElement Class

For LaTeX formula rendering using MicroTeX:

| Method | Description |
|--------|-------------|
| `void setSize(float points)` | Set font size in points |
| `void setPosition(float x, float y)` | Set position in pixels from top-left |
| `void setPosition(Position anchor)` | Set position using anchor preset |
| `void setColor(const std::string& hex)` | Set color via hex string |
| `void setColor(int r, int g, int b)` | Set color via RGB (0-255 each) |
| `void setColor(float h, float s, float l)` | Set color via HSL |
| `void show(float duration)` | Show element (fade in) |
| `void show(float duration, Direction dir)` | Show with directional slide |
| `void show(float duration, Direction dir, float shift)` | Show with custom shift amount |
| `void hide(float duration)` | Hide element (fade out) |
| `void hide(float duration, Direction dir)` | Hide with directional slide |
| `void hide(float duration, Direction dir, float shift)` | Hide with custom shift amount |

**Note:** Elements are hidden by default when created. Call `show()` to make them visible.

### ShapeElement Class

For basic geometric shapes (Circle, Rectangle, Line, Triangle):

| Method | Description |
|--------|-------------|
| `void setPosition(float x, float y)` | Set position in pixels from top-left |
| `void setPosition(Position anchor)` | Set position using anchor preset |
| `void setFill(const std::string& hex)` | Set fill color via hex string |
| `void setFill(int r, int g, int b)` | Set fill color via RGB (0-255 each) |
| `void setStroke(float width)` | Set stroke/outline width in pixels |
| `void setStrokeColor(const std::string& hex)` | Set stroke color via hex string |
| `void setStrokeColor(int r, int g, int b)` | Set stroke color via RGB |
| `void show(float duration)` | Show element (fade in) |
| `void show(float duration, Direction dir)` | Show with directional slide |
| `void show(float duration, Direction dir, float shift)` | Show with custom shift amount |
| `void hide(float duration)` | Hide element (fade out) |
| `void hide(float duration, Direction dir)` | Hide with directional slide |
| `void hide(float duration, Direction dir, float shift)` | Hide with custom shift amount |
| `void MoveTo(float duration, float x, float y)` | Animate to pixel position |
| `void Scale(float duration, float targetScale)` | Animate scale (1.0 = original) |
| `void Rotate(float duration, float degrees)` | Animate rotation in degrees |
| `void morphTo(ShapeElement& target, float duration)` | Morph into another shape (true vertex interpolation) |

**Note:** Elements are hidden by default when created. Call `show()` to make them visible.

### AxesElement Class

For 2D coordinate systems with axis lines, tick marks, and labels:

| Method | Description |
|--------|-------------|
| `void setRange(float xMin, float xMax, float yMin, float yMax)` | Set the data range |
| `void setTickSpacing(float xSpacing, float ySpacing)` | Set tick mark spacing |
| `void setColor(const std::string& hex)` | Set axis color via hex string |
| `void showLabels(bool show)` | Show/hide tick labels |
| `void showArrows(bool show)` | Show/hide arrow tips on axes |
| `void show(float duration)` | Show element (fade in) |
| `void hide(float duration)` | Hide element (fade out) |
| `float toPixelX(float dataX)` | Convert data X coordinate to pixel X |
| `float toPixelY(float dataY)` | Convert data Y coordinate to pixel Y |

**Note:** Elements are hidden by default when created. Call `show()` to make them visible.

### GraphElement Class

For plotting data points and mathematical functions:

| Method | Description |
|--------|-------------|
| `void setData(const std::vector<float>& x, const std::vector<float>& y)` | Set data points |
| `void setColor(const std::string& hex)` | Set line color via hex string |
| `void setColor(int r, int g, int b)` | Set line color via RGB (0-255 each) |
| `void setThickness(float pixels)` | Set line thickness in pixels |
| `void show(float duration)` | Show element (fade in) |
| `void hide(float duration)` | Hide element (fade out) |

**Note:** Elements are hidden by default when created. Call `show()` to make them visible.

### Axes3DElement Class

For 3D coordinate systems with axis lines:

| Method | Description |
|--------|-------------|
| `void setRange(xMin, xMax, yMin, yMax, zMin, zMax)` | Set the 3D data range |
| `void setTickSpacing(float xSpacing, float ySpacing, float zSpacing)` | Set tick mark spacing |
| `void setColor(const std::string& hex)` | Set axis color via hex string |
| `void setColor(int r, int g, int b)` | Set axis color via RGB |
| `void showLabels(bool show)` | Show/hide tick labels |
| `void showArrows(bool show)` | Show/hide arrow tips on axes |
| `void setOpacity(float opacity)` | Set axis opacity (0-1) |
| `void show(float duration)` | Show element (fade in) |
| `void hide(float duration)` | Hide element (fade out) |

### Surface3DElement Class

For 3D surface plots (z = f(x, y)):

| Method | Description |
|--------|-------------|
| `void setColor(const std::string& hex)` | Set surface color via hex string |
| `void setColor(int r, int g, int b)` | Set surface color via RGB |
| `void setOpacity(float opacity)` | Set surface opacity (0-1) |
| `void setWireframe(bool wireframe)` | Toggle wireframe rendering mode |
| `void setResolution(int uSegments, int vSegments)` | Set surface tessellation detail |
| `void show(float duration)` | Show element (fade in) |
| `void hide(float duration)` | Hide element (fade out) |

### Shape3DElement Class

For 3D geometric shapes (Sphere, Cube, Cylinder, Cone, Arrow3D):

| Method | Description |
|--------|-------------|
| `void setPosition(float x, float y, float z)` | Set position in 3D space |
| `void setRotation(float rx, float ry, float rz)` | Set rotation (Euler angles in degrees) |
| `void setScale(float s)` | Set uniform scale |
| `void setScale(float sx, float sy, float sz)` | Set non-uniform scale |
| `void setColor(const std::string& hex)` | Set shape color via hex string |
| `void setColor(int r, int g, int b)` | Set shape color via RGB |
| `void setOpacity(float opacity)` | Set shape opacity (0-1) |
| `void show(float duration)` | Show element (fade in) |
| `void hide(float duration)` | Hide element (fade out) |
| `void MoveTo(float duration, float x, float y, float z)` | Animate to 3D position |
| `void ScaleTo(float duration, float scale)` | Animate scale |
| `void RotateTo(float duration, float rx, float ry, float rz)` | Animate rotation |

### Position Anchors

```cpp
enum class Position {
    CENTER,   // Center of screen (default)
    TOP,      // Top center
    BOTTOM,   // Bottom center
    LEFT,     // Left center
    RIGHT,    // Right center
    TLEFT,    // Top-left corner
    TRIGHT,   // Top-right corner
    BLEFT,    // Bottom-left corner
    BRIGHT    // Bottom-right corner
};

enum class TextAlignment {
    LEFT,     // Left-align text (default)
    CENTER,   // Center each line
    RIGHT     // Right-align text
};

enum class Direction {
    NONE,     // No directional movement (default)
    UP,       // Slide in from below / out upward
    DOWN,     // Slide in from above / out downward
    LEFT,     // Slide in from right / out leftward
    RIGHT     // Slide in from left / out rightward
};

enum class Easing {
    Linear,           // Constant speed
    // Quadratic
    EaseInQuad, EaseOutQuad, EaseInOutQuad,
    // Cubic
    EaseInCubic, EaseOutCubic, EaseInOutCubic,  // EaseOutCubic is the default
    // Quartic
    EaseInQuart, EaseOutQuart, EaseInOutQuart,
    // Quintic
    EaseInQuint, EaseOutQuint, EaseInOutQuint,
    // Sinusoidal
    EaseInSine, EaseOutSine, EaseInOutSine,
    // Exponential
    EaseInExpo, EaseOutExpo, EaseInOutExpo,
    // Circular
    EaseInCirc, EaseOutCirc, EaseInOutCirc,
    // Back (overshoot)
    EaseInBack, EaseOutBack, EaseInOutBack,
    // Elastic (spring)
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    // Bounce
    EaseInBounce, EaseOutBounce, EaseInOutBounce,
    // Manim-style
    Smooth,           // Smoothstep (3t² - 2t³)
    Smoother,         // Smootherstep (6t⁵ - 15t⁴ + 10t³)
    ThereAndBack,     // 0 → 1 → 0
    ThereAndBackWithPause  // 0 → 1 (pause) → 0
};
```

### Timing & Control

All element classes (TextElement, MathElement, ShapeElement, ImageElement, Shape3DElement) support these timing control methods:

| Method | Description |
|--------|-------------|
| `setEasing(Easing easing)` | Set easing function for subsequent animations |
| `delay(float seconds)` | Delay the next animation by given seconds |
| `then()` | Chain next animation to start after current one completes |
| `repeat(int count)` | Repeat the next animation N times |
| `repeatForever()` | Loop the next animation indefinitely |
| `pingPong()` | Alternate direction each loop (0→1→0) |
| `stopLoop()` | Cancel any active looping |

All animation methods also have easing overloads:

```cpp
// Example: show with easing
element.show(1.0f, Easing::EaseOutElastic);
element.show(1.0f, Direction::UP, 0.1f, Easing::EaseOutBack);

// Example: transform with easing
shape.MoveTo(1.0f, 500, 300, Easing::EaseInOutCubic);
shape.Scale(0.5f, 2.0f, Easing::EaseOutBounce);
shape.Rotate(1.0f, 360.0f, Easing::EaseInOutQuad);
```

### AnimationGroup Class

For parallel and sequential animations:

| Method | Description |
|--------|-------------|
| `addShow(element, duration)` | Add element show animation to group |
| `addHide(element, duration)` | Add element hide animation to group |
| `setLagRatio(float ratio)` | Set timing: 0=parallel (default), 1=sequential |
| `setEasing(Easing easing)` | Set easing for all animations in group |
| `play()` | Execute the animation group |

Factory methods on Catalyst:

| Method | Description |
|--------|-------------|
| `createAnimationGroup()` | Create parallel group (lag_ratio=0) |
| `createSequence()` | Create sequential group (lag_ratio=1) |

### Group Class

For grouping multiple elements together with layout and z-ordering:

| Method | Description |
|--------|-------------|
| `Group& add(TextElement& e)` | Add text element to group |
| `Group& add(MathElement& e)` | Add math element to group |
| `Group& add(ShapeElement& e)` | Add shape element to group |
| `Group& add(ImageElement& e)` | Add image element to group |
| `Group& add(GraphElement& e)` | Add graph element to group |
| `Group& add(Group& g)` | Add nested group (supports hierarchy) |
| `Group& remove(...)` | Remove element from group (same overloads as add) |
| `void clear()` | Remove all elements from group |
| `size_t size() const` | Get number of elements in group |
| `bool empty() const` | Check if group is empty |
| `void setPosition(float x, float y)` | Set group center position in pixels |
| `void setPosition(Position anchor)` | Set group position using anchor preset |
| `void arrange(Direction dir, float spacing)` | Arrange elements in row (RIGHT) or column (DOWN) |
| `void arrangeInGrid(int cols, float hSpacing, float vSpacing)` | Arrange elements in grid layout |
| `void setZIndex(int zIndex)` | Set z-order for all group elements (higher = on top) |
| `int getZIndex() const` | Get group z-index |
| `void setColor(const std::string& hex)` | Set color recursively (shapes: fill+stroke) |
| `void setColor(int r, int g, int b)` | Set color recursively (RGB 0-255) |
| `void setOpacity(float opacity)` | Set opacity recursively where supported |
| `void setFill(const std::string& hex)` | Set fill for shape members |
| `void setFill(int r, int g, int b)` | Set fill for shape members (RGB 0-255) |
| `void setFillOpacity(float opacity)` | Set fill opacity for shape members |
| `void setStroke(float width)` | Set stroke width for shape members |
| `void setStrokeColor(const std::string& hex)` | Set stroke color for shape members |
| `void setStrokeColor(int r, int g, int b)` | Set stroke color for shape members (RGB 0-255) |
| `void setStrokeOpacity(float opacity)` | Set stroke opacity for shape members |
| `void getBounds(float& minX, float& minY, float& maxX, float& maxY) const` | Get bounding box |
| `float getWidth() const` | Get group width in pixels |
| `float getHeight() const` | Get group height in pixels |
| `Group& setEasing(Easing e)` | Set easing for subsequent animations |
| `Group& delay(float seconds)` | Delay next animation |
| `Group& then()` | Chain next animation after current |
| `void show(float duration)` | Show all elements (fade in) |
| `void show(float duration, Direction dir)` | Show with directional slide |
| `void show(float duration, Direction dir, float shift)` | Show with custom shift |
| `void hide(float duration)` | Hide all elements (fade out) |
| `void hide(float duration, Direction dir)` | Hide with directional slide |
| `void hide(float duration, Direction dir, float shift)` | Hide with custom shift |
| `void MoveTo(float duration, float x, float y)` | Animate group to position |
| `void Scale(float duration, float scale)` | Scale all elements |
| `void Rotate(float duration, float degrees)` | Rotate all elements |

Factory method on Catalyst:

| Method | Description |
|--------|-------------|
| `Group createGroup()` | Create new empty group |
| `Group SVGMobject(const std::string& path)` | Import an SVG as vector shapes |
| `Group SVGMobject(const std::string& path, float targetHeightPx)` | Import and scale to target height |

### SVG Import

```cpp
auto logo = window.SVGMobject("assets/logo.svg", 200);  // Scale to 200px tall
logo.setFill("#FFFFFF");
logo.show(1.0f);
```

### Examples

```cpp
// Font sizing (scaled from 128pt reference atlas)
myText.setSize(24);   // Small
myText.setSize(72);   // Large
myText.setSize(128);  // Extra large

// Pixel positioning (clamped to window bounds)
myText.setPosition(100, 200);  // 100px from left, 200px from top

// Anchor positioning (resolution-independent)
myText.setPosition(Position::TLEFT);   // Top-left corner
myText.setPosition(Position::CENTER);  // Center of screen
myText.setPosition(Position::BRIGHT);  // Bottom-right corner

// Color - Hex format
myText.setColor("#FF5733");   // Orange (with #)
myText.setColor("00FF00");    // Green (without #)

// Color - RGB format (0-255)
myText.setColor(255, 87, 51);    // Orange
myText.setColor(100, 200, 255);  // Light blue

// Color - HSL format (h: 0-360, s: 0-1, l: 0-1)
myText.setColor(14.0f, 0.8f, 0.6f);   // Orange
myText.setColor(200.0f, 0.7f, 0.5f);  // Blue

// Elements are hidden by default - call show() to make visible
myText.show(2.0f);   // Show over 2 seconds
myText.hide(1.5f);   // Hide over 1.5 seconds

// Directional animations (like Manim's shift parameter)
myText.show(1.0f, Direction::UP);       // Slide up while showing
myText.show(0.5f, Direction::LEFT);     // Slide left while showing
myText.hide(0.8f, Direction::DOWN);     // Slide down while hiding
myText.hide(1.0f, Direction::RIGHT);    // Slide right while hiding

// Custom shift amount (default is 0.1 = 10% of screen in NDC)
myText.show(1.0f, Direction::UP, 0.2f);  // 20% shift

// Stroke/outline
myText.setStroke(2.0f);              // 2px stroke width
myText.setStrokeColor("#000000");    // Black outline
myText.setStrokeColor(0, 0, 0);      // RGB format

// Gradient
myText.setGradient("#FF0000", "#0000FF");         // Horizontal red-to-blue
myText.setGradient("#FF0000", "#0000FF", 90.0f);  // Vertical (90°)
myText.setGradient("#FF0000", "#0000FF", 45.0f);  // Diagonal (45°)
myText.clearGradient();                            // Back to solid color

// Multi-line / Paragraph text
auto para = window.setText("Line 1\nLine 2\nLine 3");  // Explicit newlines
para.setLineHeight(1.4f);                              // 140% line spacing
para.setAlignment(TextAlignment::CENTER);              // Center each line

// Word wrapping
auto wrapped = window.setText("Long text that wraps automatically");
wrapped.setMaxWidth(400);                              // Wrap at 400 pixels

// LaTeX formulas
auto formula = window.setMath("E = mc^2");
formula.setSize(48);
formula.setPosition(Position::CENTER);
formula.setColor("#FFFFFF");
formula.show(1.0f);

// Shapes - Circle
auto circle = window.setCircle(100);  // radius 100px
circle.setPosition(Position::CENTER);
circle.setFill("#FF5733");  // Orange fill
circle.setStroke(2.0f);
circle.setStrokeColor("#000000");
circle.show(0.5f);

// Shapes - Rectangle
auto rect = window.setRectangle(200, 100);  // 200x100 pixels
rect.setPosition(400, 300);
rect.setFill("#33FF57");  // Green fill
rect.show(0.8f);

// Shapes - Line
auto line = window.setLine(100, 100, 500, 400);  // from (100,100) to (500,400)
line.setFill("#FFFFFF");  // White line
line.show(0.5f);

// Shapes - Triangle (equilateral)
auto triangle = window.setTriangle(150);  // 150px side length
triangle.setPosition(Position::BOTTOM);
triangle.setFill("#5733FF");  // Purple fill
triangle.show(0.5f);

// Shapes - Arrow
auto arrow = window.setArrow(100, 300, 500, 300);  // from (100,300) to (500,300)
arrow.setFill("#FFFF00");  // Yellow arrow
arrow.show(0.5f);

// Shape animations
circle.MoveTo(1.5f, 960, 540);  // Move to center
circle.Scale(1.0f, 2.0f);       // Scale up to 2x
circle.Rotate(1.5f, 360.0f);    // Full rotation

// Shape morphing (true vertex interpolation)
auto circle1 = window.setCircle(100);
circle1.setPosition(Position::LEFT);
circle1.setFill("#FF5733");  // Orange
circle1.show(0.5f);

auto rect1 = window.setRectangle(200, 150);
rect1.setPosition(Position::RIGHT);
rect1.setFill("#33FF57");  // Green
// rect1 stays hidden until morph completes

window.wait(1.0f);
circle1.morphTo(rect1, 2.0f);  // Circle smoothly morphs into rectangle

// Text morphing (cross-fade with position)
auto title1 = window.setText("Hello");
title1.setSize(72);
title1.setPosition(Position::CENTER);
title1.show(0.5f);

auto title2 = window.setText("World");
title2.setSize(72);
// title2 stays hidden until morph completes

window.wait(1.0f);
title1.morphTo(title2, 1.5f);  // "Hello" morphs into "World"

// Axes - 2D coordinate system
auto axes = window.setAxes(-5.0f, 5.0f, -2.0f, 2.0f);  // x: -5 to 5, y: -2 to 2
axes.setTickSpacing(1.0f, 0.5f);  // x ticks every 1, y ticks every 0.5
axes.setColor("#FFFFFF");
axes.show(1.0f);

// Graph from lambda function (e.g., sin wave)
auto sinGraph = window.setGraph(axes, [](float x) { return std::sin(x); }, 200);
sinGraph.setColor("#FF5733");  // Orange
sinGraph.setThickness(3.0f);
sinGraph.show(1.5f);

// Graph from data points
std::vector<float> dataX = {-4.0f, -2.0f, 0.0f, 2.0f, 4.0f};
std::vector<float> dataY = {1.5f, -1.0f, 0.5f, 1.0f, -0.5f};
auto dataGraph = window.setGraph(axes, dataX, dataY);
dataGraph.setColor("#33FF57");  // Green
dataGraph.setThickness(4.0f);
dataGraph.show(1.0f);

// Multiple function plots
auto cosGraph = window.setGraph(axes, [](float x) { return std::cos(x); }, 200);
cosGraph.setColor("#5733FF");  // Purple
cosGraph.show(1.0f);

// Groups - Basic grouping
auto circle = window.setCircle(50);
circle.setFill("#FF5733");
auto label = window.setText("Item 1");
label.setSize(24);
label.setColor("#FFFFFF");

auto card = window.createGroup();
card.add(circle).add(label);
card.setPosition(Position::CENTER);
card.show(1.0f);

// Groups - Row arrangement
auto row = window.createGroup();
for (int i = 0; i < 5; i++) {
    auto item = window.setCircle(30);
    item.setFill("#4CAF50");
    row.add(item);
}
row.arrange(Direction::RIGHT, 20.0f);  // 20px spacing
row.setPosition(Position::TOP);
row.show(1.0f);

// Groups - Column arrangement
auto column = window.createGroup();
auto header = window.setText("Title");
auto body = window.setText("Body text");
auto footer = window.setText("Footer");
column.add(header).add(body).add(footer);
column.arrange(Direction::DOWN, 15.0f);  // 15px vertical spacing
column.setPosition(Position::LEFT);
column.show(0.5f);

// Groups - Grid arrangement
auto grid = window.createGroup();
for (int i = 0; i < 9; i++) {
    auto cell = window.setRectangle(80, 80);
    cell.setFill(i % 2 == 0 ? "#2196F3" : "#FF9800");
    grid.add(cell);
}
grid.arrangeInGrid(3, 10.0f, 10.0f);  // 3 columns, 10px spacing
grid.setPosition(Position::CENTER);
grid.show(1.5f);

// Groups - Z-ordering (render order)
auto background = window.setRectangle(400, 300);
background.setFill("#333333");
background.setPosition(Position::CENTER);
background.setZIndex(0);  // Render first (behind)
background.show(0.5f);

auto foreground = window.setText("On Top");
foreground.setSize(48);
foreground.setColor("#FFFFFF");
foreground.setPosition(Position::CENTER);
foreground.setZIndex(1);  // Render after (in front)
foreground.show(0.5f);

// Groups - Nested groups
auto innerGroup = window.createGroup();
innerGroup.add(window.setCircle(20)).add(window.setCircle(20));
innerGroup.arrange(Direction::RIGHT, 10.0f);

auto outerGroup = window.createGroup();
outerGroup.add(innerGroup);
outerGroup.add(window.setText("Nested!"));
outerGroup.arrange(Direction::DOWN, 15.0f);
outerGroup.setPosition(Position::BOTTOM);
outerGroup.show(1.0f);

// Groups - Animate entire group
card.MoveTo(2.0f, 500, 300);  // Move group to position
card.Scale(1.5f, 1.5f);        // Scale all elements 1.5x
card.hide(1.0f, Direction::LEFT);  // Hide with slide

// Scene & Camera - Background color
window.setBackground("#1a1a2e");  // Dark blue background

// Camera controls - animated
window.setCameraZoom(1.5f, 1.0f);       // Zoom in to 1.5x over 1 second
window.setCameraPan(0.2f, 0.0f, 0.5f);  // Pan right over 0.5 seconds
window.setCameraRotate(30.0f, 1.0f);    // Rotate 30 degrees over 1 second
window.resetCamera(0.5f);                // Reset camera over 0.5 seconds

// Camera controls - instant
window.setCameraZoom(2.0f);             // Instant zoom
window.setCameraPan(-0.1f, 0.1f);       // Instant pan
window.setCameraRotate(45.0f);          // Instant rotation

// Multiple scenes
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

window.setActiveScene(scene1);
window.wait(2.0f);
window.setActiveScene(scene2);

// ============== 3D RENDERING ==============

// Enable 3D mode
window.set3DMode(true);

// 3D Axes
auto axes3D = window.setAxes3D(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
axes3D.setColor("#AAAAAA");
axes3D.show(0.5f);

// 3D Surface (z = sin(x) * cos(y))
auto surface = window.setSurface(
    [](float x, float y) { return std::sin(x) * std::cos(y); },
    -2.0f, 2.0f, -2.0f, 2.0f
);
surface.setColor("#5599FF");
surface.setOpacity(0.8f);
surface.show(1.0f);

// 3D Sphere
auto sphere = window.setSphere(0.5f);
sphere.setPosition(0.0f, 1.5f, 0.0f);
sphere.setColor("#00FFAA");
sphere.show(0.5f);

// 3D Cube
auto cube = window.setCube3D(1.0f);
cube.setPosition(-1.5f, 0.5f, 0.0f);
cube.setColor("#FF5733");
cube.show(0.5f);

// 3D Cylinder
auto cylinder = window.setCylinder(0.3f, 1.0f);
cylinder.setPosition(1.5f, 0.5f, 0.0f);
cylinder.setColor("#33FF57");
cylinder.show(0.5f);

// 3D Cone
auto cone = window.setCone(0.4f, 1.0f);
cone.setPosition(0.0f, 0.0f, 1.5f);
cone.setColor("#FF33AA");
cone.show(0.5f);

// 3D Arrow
auto arrow3D = window.setArrow3D(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
arrow3D.setColor("#FFFF00");
arrow3D.show(0.5f);

// 3D Shape animations
sphere.MoveTo(1.5f, 1.0f, 2.0f, 0.0f);  // Move to new position
sphere.ScaleTo(1.0f, 2.0f);              // Scale up to 2x
sphere.RotateTo(2.0f, 0.0f, 180.0f, 0.0f);  // Rotate around Y

// 3D Camera controls - instant
window.setCamera3D(5.0f, 3.0f, 5.0f, 0.0f, 0.0f, 0.0f);  // Position and target

// 3D Camera controls - animated
window.setCamera3D(7.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f);  // Animate over 2 seconds
window.setCameraFOV(30.0f, 1.5f);  // Zoom in (narrower FOV)
window.setCameraFOV(60.0f, 1.5f);  // Zoom out (wider FOV)
window.resetCamera3D(1.5f);        // Animate back to default

// Orbit camera using spherical coordinates
window.orbitCamera(0.785f, 1.047f, 6.0f);  // theta, phi, distance
window.orbitCamera(1.57f, 0.785f, 8.0f, 2.0f);  // Animate orbit

// "Camera frame" style controls
window.setCamera3DTarget(0.0f, 1.0f, 0.0f, 1.0f);
window.scaleCamera3DDistance(0.8f, 1.0f);
window.rotateCamera3D(0.0f, 1.0f, 0.0f, 0.5f, 1.0f);  // 0.5 rad about Y
window.reorientCamera(-30.0f, 75.0f, 1.0f);            // degrees
window.beginAmbientCameraRotation(0.1f);
window.stopAmbientCameraRotation();

// Mixed mode: 2D overlay on 3D content
window.set3DMode(true);
auto sphere2 = window.setSphere(1.0f);
sphere2.show(0.5f);

// 2D text overlays work on top of 3D content
auto label = window.setText("3D Rendering Demo");
label.setPosition(Position::TOP);
label.setSize(48);
label.setColor("#FFFFFF");
label.fix_in_frame();  // HUD overlay (ignore 2D camera pan/zoom/rotate)
label.show(0.5f);
```

### Scene Class

| Method | Description |
|--------|-------------|
| `TextElement setText(const std::string& text)` | Create text in this scene |
| `MathElement setMath(const std::string& latex)` | Create LaTeX formula in this scene |
| `ShapeElement setCircle(float radius)` | Create circle in this scene |
| `ShapeElement setRectangle(float width, float height)` | Create rectangle in this scene |
| `ShapeElement setLine(float x1, float y1, float x2, float y2)` | Create line in this scene |
| `ShapeElement setTriangle(float size)` | Create triangle in this scene |
| `ShapeElement setArrow(float x1, float y1, float x2, float y2)` | Create arrow in this scene |
| `void setBackground(const std::string& hex)` | Set scene background color |
| `void setCameraZoom(float zoom, float duration)` | Animate scene camera zoom |
| `void setCameraPan(float x, float y, float duration)` | Animate scene camera pan |
| `void setCameraRotate(float degrees, float duration)` | Animate scene camera rotation |
| `void resetCamera(float duration)` | Reset scene camera |
| `void wait(float seconds)` | Schedule delay in scene timeline |
| `size_t getIndex()` | Get scene index |

## Key Files

| File | Purpose |
|------|---------|
| `catalyst.h` | Public API header (Catalyst, TextElement, MathElement, ShapeElement, AxesElement, GraphElement, Position) |
| `main.cpp` | Vulkan implementation + TextObject + ShapeObject + AxesObject + GraphObject + demo main() |
| `shaders/shader.vert` | Vertex shader for text (position + UV passthrough) |
| `shaders/shader.frag` | Fragment shader for text (SDF rendering + stroke + gradient) |
| `shaders/shape.vert` | Vertex shader for shapes (position only) |
| `shaders/shape.frag` | Fragment shader for shapes (solid color fill) |
| `shaders/shape3d.vert` | Vertex shader for 3D shapes (position + normal + model matrix) |
| `shaders/shape3d.frag` | Fragment shader for 3D shapes (lighting + color) |
| `stb_truetype.h` | TTF font loading library |
| `font/Raleway-Bold.ttf` | Font file for text rendering |
| `build.sh` | Build automation script |
| `CMakeLists.txt` | CMake build configuration |
| `TODO.md` | Feature roadmap comparing with Manim |
| `lib/MicroTeX/` | LaTeX rendering library (submodule) |

## Architecture

### GPU-First Design

| Feature | Implementation |
|---------|----------------|
| **GPU Selection** | Scores all GPUs, strongly prefers discrete (10000 pts) over integrated (1000 pts) |
| **Memory Strategy** | Device-local (GPU VRAM) for vertex buffers via staging pattern |
| **Queue Architecture** | Multi-queue: Graphics (priority 1.0), Compute (0.8), Transfer (0.5) |
| **Buffering** | Triple buffering (3 frames in flight) to reduce CPU-GPU stalls |
| **Load Monitoring** | GPU timestamp queries measure render time per frame |
| **Hybrid Fallback** | Auto-activates when GPU overloaded >33ms for 10+ consecutive frames |

### Animation System

- Uses Vulkan push constants to pass offset (vertex) and opacity/color (fragment)
- Time tracking via `glfwGetTime()` for smooth animations
- Each text element has independent animation state
- Animations play once then stop (not looping)
- Directional fade uses ease-out cubic easing for smooth motion
- Position offset applied in vertex shader, allowing slide effects without vertex buffer rebuilds

### Text Rendering

- **SDF (Signed Distance Field)** font rendering for high-quality text at any size
- Font baked at 128pt reference size using `stbtt_GetCodepointSDF()`
- 1024x1024 font atlas uploaded to GPU as R8_UNORM texture
- SDF enables smooth stroke/outline at any width without artifacts
- Combined vertex buffer with per-text offsets
- Multiple draw calls with per-text push constants (56 bytes)
- Fragment shader computes fill, stroke, and gradient per-fragment

### LaTeX Rendering

- Uses MicroTeX library for LaTeX formula parsing
- Renders to Cairo surface, then uploads as GPU texture
- Supports standard LaTeX math notation
- Independent from text rendering pipeline

### Multiple Text Objects

- Each `setText()` creates an independent `TextObject`
- Stored in `std::vector<TextObject>` with per-element state
- `TextElement` holds index to reference its object
- Vertex buffer rebuilt when text properties change (dirty flag optimization)

### Positioning System

- **Pixel mode:** Top-left origin (0,0), X right, Y down (like CSS)
- **Anchor mode:** Resolution-independent presets
- Automatic bounds clamping keeps text visible

### 3D Rendering System

- Separate 3D render pass with depth buffer for proper occlusion
- View matrix (LookAt) and projection matrix (Perspective) via UBO
- Model matrix per-object via push constants for position, rotation, scale
- Lighting via dedicated UBO supporting:
  - Ambient light (color + intensity)
  - Directional light (direction + color + intensity)
  - Up to 4 point lights (position + color + intensity + attenuation)
- Mixed-mode rendering: 2D overlay render pass loads 3D content (no clear)
- Shape generation for primitives: Sphere, Cube, Cylinder, Cone, Arrow3D
- Surface rendering for z=f(x,y) height functions with configurable tessellation
- 3D camera animations with timeline scheduling (eye, target, FOV)

## Vulkan Features Used

- Vulkan 1.3 API
- Swap chain with triple buffering
- Device-local memory allocation
- Multiple queue families with priorities
- Timestamp queries for GPU profiling
- Push constants for per-draw rendering (56 bytes: offset, opacity, color, stroke, gradient)
- Combined image samplers for font textures
- Alpha blending for text rendering
- Multiple draw calls per frame (one per text element)
- Depth buffer for 3D occlusion (D32_SFLOAT or D24_UNORM_S8_UINT)
- Multiple render passes (3D with depth, 2D overlay without depth)
- Uniform Buffer Objects (UBOs) for camera and lighting data
- Separate graphics pipeline for 3D shapes with normals

## Dependencies

- Vulkan SDK
- GLFW3
- glslangValidator (for shader compilation)
- MicroTeX (lib/MicroTeX, for LaTeX rendering)
- Cairo/Pango (for MicroTeX backend)

## Path Resolution

The executable automatically finds resources relative to its location using `/proc/self/exe`, allowing it to run from any working directory.
