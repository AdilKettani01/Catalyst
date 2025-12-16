#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <functional>

class Catalyst;
class MathElement;   // Forward declaration for Transform
class ShapeElement;  // Forward declaration
class AxesElement;   // Forward declaration
class GraphElement;  // Forward declaration
class VectorElement; // Forward declaration
class NumberLineElement; // Forward declaration
class BarChartElement;   // Forward declaration
class PieChartElement;   // Forward declaration
class VectorFieldElement; // Forward declaration
class PathBuilder;        // Forward declaration
class TableElement;       // Forward declaration
class ImageElement;       // Forward declaration
class Scene;              // Forward declaration
class AnimationGroup;     // Forward declaration
class Axes3DElement;      // Forward declaration for 3D axes
class Surface3DElement;   // Forward declaration for 3D surfaces
class Shape3DElement;     // Forward declaration for 3D shapes
class Group;              // Forward declaration for groups

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
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
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

class TextElement {
public:
    // Easing control
    TextElement& setEasing(Easing easing);  // Set easing for subsequent animations

    // Timing control
    TextElement& delay(float seconds);  // Delay the next animation by given seconds
    TextElement& then();  // Chain next animation to start after current one completes

    // Loop animation
    TextElement& repeat(int count);    // Repeat animation N times
    TextElement& repeatForever();      // Loop animation indefinitely
    TextElement& pingPong();           // Alternate direction each loop (0→1→0)
    void stopLoop();                   // Cancel any active looping

    // Basic show/hide with optional easing
    void show(float duration);
    void show(float duration, Easing easing);
    void show(float duration, Direction direction);
    void show(float duration, Direction direction, float shiftAmount);
    void show(float duration, Direction direction, float shiftAmount, Easing easing);
    void hide(float duration);
    void hide(float duration, Easing easing);
    void hide(float duration, Direction direction);
    void hide(float duration, Direction direction, float shiftAmount);
    void hide(float duration, Direction direction, float shiftAmount, Easing easing);

    // "Showing" animations - Manim-style character reveal effects
    void Write(float duration);                                    // Typewriter effect
    void Write(float duration, Direction direction);               // With directional slide
    void Write(float duration, Direction direction, float shiftAmount);
    void AddLetterByLetter(float duration);                        // Letter by letter fade-in
    void AddLetterByLetter(float duration, Direction direction);
    void AddLetterByLetter(float duration, Direction direction, float shiftAmount);
    void DrawBorderThenFill(float duration);                       // Stroke first, then fill

    // Transform animations
    void MoveTo(float duration, float x, float y);  // Animate to pixel position
    void MoveTo(float duration, float x, float y, Easing easing);
    void Scale(float duration, float targetScale);  // Animate scale (1.0 = original)
    void Scale(float duration, float targetScale, Easing easing);
    void Rotate(float duration, float degrees);     // Animate rotation (degrees)
    void Rotate(float duration, float degrees, Easing easing);
    void Transform(float duration, TextElement& target);  // Cross-fade to another TextElement
    void Transform(float duration, MathElement& target);  // Cross-fade to a MathElement
    void morphTo(TextElement& target, float duration);    // Morph into another text (cross-fade with position interpolation)

    // Emphasis animations - attention-grabbing effects that return to original state
    void Indicate(float duration);                              // Color flash + scale pulse
    void Indicate(float duration, const std::string& flashColorHex);
    void Flash(float duration);                                 // Quick brightness flash
    void Flash(float duration, float intensity);                // intensity: multiplier (1.0 = default)
    void Circumscribe(float duration);                          // Draw animated rectangle around element
    void Circumscribe(float duration, const std::string& colorHex);
    void Wiggle(float duration);                                // Oscillate rotation back and forth
    void Wiggle(float duration, int cycles);                    // cycles: number of oscillations
    void Pulse(float duration);                                 // Scale up then back to original
    void Pulse(float duration, float peakScale);                // peakScale: max scale (1.2 = 20% larger)
    void FocusOn(float duration);                               // Dim other elements, highlight this one

    // Movement animations - path-based position changes
    void MoveAlongPath(float duration, const std::vector<float>& pathX, const std::vector<float>& pathY);
    void MoveAlongPath(float duration, const std::vector<float>& pathX, const std::vector<float>& pathY, bool smooth);
    void SpiralIn(float duration);                              // Spiral inward toward center
    void SpiralIn(float duration, float rotations);             // rotations: number of full rotations
    void SpiralOut(float duration);                             // Spiral outward from center
    void SpiralOut(float duration, float rotations);
    void Homotopy(float duration, int type);                    // Predefined homotopy types (0=wave, 1=ripple, 2=shear, 3=twist)
    void Homotopy(float duration, int type, float amplitude, float frequency);
    void PhaseFlow(float duration, int fieldType);              // Flow along vector field (0=circular, 1=sink, 2=source, 3=saddle)
    void PhaseFlow(float duration, int fieldType, float strength);

    void setSize(float points);
    void setPosition(float x, float y);   // Precise pixel positioning
    void setPosition(Position anchor);    // Preset anchor positioning
    void setColor(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setColor(int r, int g, int b);         // RGB: 0-255
    void setColor(float h, float s, float l);   // HSL: h(0-360), s(0-1), l(0-1)

    // Stroke/outline methods
    void setStroke(float width);                      // Stroke width in pixels
    void setStrokeColor(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setStrokeColor(int r, int g, int b);         // RGB: 0-255

    // Gradient methods
    void setGradient(const std::string& startHex, const std::string& endHex);  // Horizontal gradient
    void setGradient(const std::string& startHex, const std::string& endHex, float angleDegrees);
    void clearGradient();  // Return to solid color

    // Multi-line / Paragraph methods
    void setLineHeight(float multiplier);        // Line spacing (1.2 = 120% of font size)
    void setMaxWidth(float pixels);              // Enable word wrap at width (0 = disable)
    void setAlignment(TextAlignment alignment);  // Text alignment within block

    // Relative positioning
    void under(TextElement& other, float padding = 20.0f);  // Position below another element
    void under(MathElement& other, float padding = 20.0f);  // Position below a MathElement

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class MathElement;  // For cross-type Transform access
    friend class Scene;        // For scene-based element creation
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t textIndex;  // Index into parent's textObjects vector
    TextElement(Catalyst* p, size_t idx);
};

class MathElement {
public:
    // Easing control
    MathElement& setEasing(Easing easing);

    // Timing control
    MathElement& delay(float seconds);  // Delay the next animation by given seconds
    MathElement& then();  // Chain next animation to start after current one completes

    // Loop animation
    MathElement& repeat(int count);    // Repeat animation N times
    MathElement& repeatForever();      // Loop animation indefinitely
    MathElement& pingPong();           // Alternate direction each loop (0→1→0)
    void stopLoop();                   // Cancel any active looping

    // Basic show/hide with optional easing
    void show(float duration);
    void show(float duration, Easing easing);
    void show(float duration, Direction direction);
    void show(float duration, Direction direction, float shiftAmount);
    void show(float duration, Direction direction, float shiftAmount, Easing easing);
    void hide(float duration);
    void hide(float duration, Easing easing);
    void hide(float duration, Direction direction);
    void hide(float duration, Direction direction, float shiftAmount);
    void hide(float duration, Direction direction, float shiftAmount, Easing easing);

    // Transform animations
    void MoveTo(float duration, float x, float y);  // Animate to pixel position
    void MoveTo(float duration, float x, float y, Easing easing);
    void Scale(float duration, float targetScale);  // Animate scale (1.0 = original)
    void Scale(float duration, float targetScale, Easing easing);
    void Rotate(float duration, float degrees);     // Animate rotation (degrees)
    void Rotate(float duration, float degrees, Easing easing);
    void Transform(float duration, TextElement& target);  // Cross-fade to a TextElement
    void Transform(float duration, MathElement& target);  // Cross-fade to another MathElement

    // Emphasis animations - attention-grabbing effects that return to original state
    void Indicate(float duration);                              // Color flash + scale pulse
    void Indicate(float duration, const std::string& flashColorHex);
    void Flash(float duration);                                 // Quick brightness flash
    void Flash(float duration, float intensity);
    void Circumscribe(float duration);                          // Draw animated rectangle around element
    void Circumscribe(float duration, const std::string& colorHex);
    void Wiggle(float duration);                                // Oscillate rotation back and forth
    void Wiggle(float duration, int cycles);
    void Pulse(float duration);                                 // Scale up then back to original
    void Pulse(float duration, float peakScale);
    void FocusOn(float duration);                               // Dim other elements, highlight this one

    // Movement animations - path-based position changes
    void MoveAlongPath(float duration, const std::vector<float>& pathX, const std::vector<float>& pathY);
    void MoveAlongPath(float duration, const std::vector<float>& pathX, const std::vector<float>& pathY, bool smooth);
    void SpiralIn(float duration);
    void SpiralIn(float duration, float rotations);
    void SpiralOut(float duration);
    void SpiralOut(float duration, float rotations);
    void Homotopy(float duration, int type);
    void Homotopy(float duration, int type, float amplitude, float frequency);
    void PhaseFlow(float duration, int fieldType);
    void PhaseFlow(float duration, int fieldType, float strength);

    void setSize(float points);
    void setPosition(float x, float y);   // Precise pixel positioning
    void setPosition(Position anchor);    // Preset anchor positioning
    void setColor(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setColor(int r, int g, int b);         // RGB: 0-255
    void setColor(float h, float s, float l);   // HSL: h(0-360), s(0-1), l(0-1)

    // Relative positioning
    void under(TextElement& other, float padding = 20.0f);  // Position below a TextElement
    void under(MathElement& other, float padding = 20.0f);  // Position below another MathElement

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class TextElement;  // For cross-type Transform access
    friend class Scene;        // For scene-based element creation
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t mathIndex;  // Index into parent's mathObjects vector
    MathElement(Catalyst* p, size_t idx);
};

class ShapeElement {
public:
    // Easing control
    ShapeElement& setEasing(Easing easing);

    // Timing control
    ShapeElement& delay(float seconds);  // Delay the next animation by given seconds
    ShapeElement& then();  // Chain next animation to start after current one completes

    // Loop animation
    ShapeElement& repeat(int count);    // Repeat animation N times
    ShapeElement& repeatForever();      // Loop animation indefinitely
    ShapeElement& pingPong();           // Alternate direction each loop (0→1→0)
    void stopLoop();                   // Cancel any active looping

    // Basic show/hide with optional easing
    void show(float duration);
    void show(float duration, Easing easing);
    void show(float duration, Direction direction);
    void show(float duration, Direction direction, float shiftAmount);
    void show(float duration, Direction direction, float shiftAmount, Easing easing);
    void hide(float duration);
    void hide(float duration, Easing easing);
    void hide(float duration, Direction direction);
    void hide(float duration, Direction direction, float shiftAmount);
    void hide(float duration, Direction direction, float shiftAmount, Easing easing);

    // Transform animations
    void MoveTo(float duration, float x, float y);  // Animate to pixel position
    void MoveTo(float duration, float x, float y, Easing easing);
    void Scale(float duration, float targetScale);  // Animate scale (1.0 = original)
    void Scale(float duration, float targetScale, Easing easing);
    void Rotate(float duration, float degrees);     // Animate rotation (degrees)
    void Rotate(float duration, float degrees, Easing easing);
    void morphTo(ShapeElement& target, float duration);  // Morph into another shape

    // Emphasis animations - attention-grabbing effects that return to original state
    void Indicate(float duration);                              // Color flash + scale pulse
    void Indicate(float duration, const std::string& flashColorHex);
    void Flash(float duration);                                 // Quick brightness flash
    void Flash(float duration, float intensity);
    void Circumscribe(float duration);                          // Draw animated rectangle around element
    void Circumscribe(float duration, const std::string& colorHex);
    void Wiggle(float duration);                                // Oscillate rotation back and forth
    void Wiggle(float duration, int cycles);
    void Pulse(float duration);                                 // Scale up then back to original
    void Pulse(float duration, float peakScale);
    void FocusOn(float duration);                               // Dim other elements, highlight this one

    // Movement animations - path-based position changes
    void MoveAlongPath(float duration, const std::vector<float>& pathX, const std::vector<float>& pathY);
    void MoveAlongPath(float duration, const std::vector<float>& pathX, const std::vector<float>& pathY, bool smooth);
    void SpiralIn(float duration);
    void SpiralIn(float duration, float rotations);
    void SpiralOut(float duration);
    void SpiralOut(float duration, float rotations);
    void Homotopy(float duration, int type);
    void Homotopy(float duration, int type, float amplitude, float frequency);
    void PhaseFlow(float duration, int fieldType);
    void PhaseFlow(float duration, int fieldType, float strength);

    // Positioning
    void setPosition(float x, float y);   // Precise pixel positioning
    void setPosition(Position anchor);    // Preset anchor positioning

    // Fill color
    void setFill(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setFill(int r, int g, int b);         // RGB: 0-255

    // Stroke/outline
    void setStroke(float width);                      // Stroke width in pixels
    void setStrokeColor(const std::string& hex);      // Hex color
    void setStrokeColor(int r, int g, int b);         // RGB: 0-255

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class PathBuilder;  // PathBuilder needs to create ShapeElements
    friend class Scene;        // For scene-based element creation
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t shapeIndex;  // Index into parent's shapeObjects vector
    ShapeElement(Catalyst* p, size_t idx);
};

class AxesElement {
public:
    // Configuration
    void setRange(float xMin, float xMax, float yMin, float yMax);
    void setTickSpacing(float xSpacing, float ySpacing);
    void setColor(const std::string& hex);
    void showLabels(bool show);
    void showArrows(bool show);

    // Animations
    void show(float duration);
    void hide(float duration);

    // Coordinate conversion for users (data coords to pixels)
    float toPixelX(float dataX);
    float toPixelY(float dataY);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class GraphElement;
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t axesIndex;
    AxesElement(Catalyst* p, size_t idx);
};

// 3D Axes for mathematical visualization
class Axes3DElement {
public:
    // Configuration
    void setRange(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
    void setTickSpacing(float xSpacing, float ySpacing, float zSpacing);
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void showLabels(bool show);
    void showArrows(bool show);
    void setOpacity(float opacity);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Scene;
    Catalyst* parent;
    size_t axes3DIndex;
    Axes3DElement(Catalyst* p, size_t idx);
};

// 3D Surface for plotting z = f(x, y) or parametric surfaces
class Surface3DElement {
public:
    // Appearance
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void setOpacity(float opacity);
    void setWireframe(bool wireframe);
    void setResolution(int uSegments, int vSegments);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Scene;
    Catalyst* parent;
    size_t surface3DIndex;
    Surface3DElement(Catalyst* p, size_t idx);
};

// 3D Shapes (Sphere, Cube, Cylinder, Cone, Arrow3D)
class Shape3DElement {
public:
    // Easing control
    Shape3DElement& setEasing(Easing easing);

    // Timing control
    Shape3DElement& delay(float seconds);  // Delay the next animation by given seconds
    Shape3DElement& then();  // Chain next animation to start after current one completes

    // Loop animation
    Shape3DElement& repeat(int count);    // Repeat animation N times
    Shape3DElement& repeatForever();      // Loop animation indefinitely
    Shape3DElement& pingPong();           // Alternate direction each loop (0→1→0)
    void stopLoop();                       // Cancel any active looping

    // Transform
    void setPosition(float x, float y, float z);
    void setRotation(float rx, float ry, float rz);  // Euler angles in degrees
    void setScale(float s);
    void setScale(float sx, float sy, float sz);

    // Appearance
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void setOpacity(float opacity);

    // Animations
    void show(float duration);
    void show(float duration, Easing easing);
    void hide(float duration);
    void hide(float duration, Easing easing);
    void MoveTo(float duration, float x, float y, float z);
    void MoveTo(float duration, float x, float y, float z, Easing easing);
    void ScaleTo(float duration, float scale);
    void ScaleTo(float duration, float scale, Easing easing);
    void RotateTo(float duration, float rx, float ry, float rz);
    void RotateTo(float duration, float rx, float ry, float rz, Easing easing);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class Scene;
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t shape3DIndex;
    Shape3DElement(Catalyst* p, size_t idx);
};

class GraphElement {
public:
    // Data modification
    void setData(const std::vector<float>& x, const std::vector<float>& y);

    // Appearance
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void setThickness(float pixels);

    // Animations
    void show(float duration);
    void hide(float duration);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t graphIndex;
    GraphElement(Catalyst* p, size_t idx);
};

class VectorElement {
public:
    // Appearance
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void setThickness(float pixels);
    void setOrigin(float x, float y);  // Change origin from default (0,0)

    // Animations
    void show(float duration);
    void hide(float duration);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t vectorIndex;
    VectorElement(Catalyst* p, size_t idx);
};

class NumberLineElement {
public:
    // Configuration
    void setRange(float min, float max);
    void setTickSpacing(float spacing);
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void showLabels(bool show);
    void showArrows(bool show);

    // Positioning
    void setPosition(float x, float y);
    void setPosition(Position anchor);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t numberLineIndex;
    NumberLineElement(Catalyst* p, size_t idx);
};

class BarChartElement {
public:
    // Data modification
    void setData(const std::vector<float>& values);
    void setData(const std::vector<float>& values, const std::vector<std::string>& labels);

    // Appearance
    void setColors(const std::vector<std::string>& hexColors);
    void setBarWidth(float width);
    void setSpacing(float spacing);
    void setMaxHeight(float height);

    // Positioning
    void setPosition(float x, float y);
    void setPosition(Position anchor);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t barChartIndex;
    BarChartElement(Catalyst* p, size_t idx);
};

class PieChartElement {
public:
    // Data modification
    void setData(const std::vector<float>& values);

    // Appearance
    void setColors(const std::vector<std::string>& hexColors);
    void setRadius(float radius);
    void setStartAngle(float degrees);

    // Positioning
    void setPosition(float x, float y);
    void setPosition(Position anchor);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t pieChartIndex;
    PieChartElement(Catalyst* p, size_t idx);
};

class VectorFieldElement {
public:
    // Configuration
    void setGridSize(int gridX, int gridY);
    void setArrowScale(float scale);

    // Appearance
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t vectorFieldIndex;
    VectorFieldElement(Catalyst* p, size_t idx);
};

class PathBuilder {
public:
    // Path building commands (chainable)
    PathBuilder& moveTo(float x, float y);           // Move to position (start new subpath)
    PathBuilder& lineTo(float x, float y);           // Draw line to position
    PathBuilder& curveTo(float cx1, float cy1,       // Cubic bezier curve
                         float cx2, float cy2,
                         float x, float y);
    PathBuilder& close();                            // Close current subpath

    // Build the final shape
    ShapeElement build();

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t shapeIndex;
    PathBuilder(Catalyst* p, size_t idx);
};

class TableElement {
public:
    // Data modification
    void setData(const std::vector<std::vector<std::string>>& data);

    // Appearance
    void setCellSize(float width, float height);
    void setColumnWidth(int col, float width);
    void setRowHeight(int row, float height);
    void setFontSize(float size);
    void setTextColor(const std::string& hex);
    void setTextColor(int r, int g, int b);
    void setGridColor(const std::string& hex);
    void setGridColor(int r, int g, int b);
    void setHeaderColor(const std::string& hex);
    void setHeaderColor(int r, int g, int b);
    void showGrid(bool show);
    void showHeader(bool show);

    // Positioning
    void setPosition(float x, float y);
    void setPosition(Position anchor);

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t tableIndex;
    TableElement(Catalyst* p, size_t idx);
};

class ImageElement {
public:
    // Easing control
    ImageElement& setEasing(Easing easing);

    // Timing control
    ImageElement& delay(float seconds);  // Delay the next animation by given seconds
    ImageElement& then();  // Chain next animation to start after current one completes

    // Loop animation
    ImageElement& repeat(int count);    // Repeat animation N times
    ImageElement& repeatForever();      // Loop animation indefinitely
    ImageElement& pingPong();           // Alternate direction each loop (0→1→0)
    void stopLoop();                    // Cancel any active looping

    // Size control
    void setSize(float width, float height);

    // Positioning
    void setPosition(float x, float y);
    void setPosition(Position anchor);

    // Animations
    void show(float duration);
    void show(float duration, Easing easing);
    void show(float duration, Direction direction);
    void show(float duration, Direction direction, float shiftAmount);
    void show(float duration, Direction direction, float shiftAmount, Easing easing);
    void hide(float duration);
    void hide(float duration, Easing easing);
    void hide(float duration, Direction direction);
    void hide(float duration, Direction direction, float shiftAmount);
    void hide(float duration, Direction direction, float shiftAmount, Easing easing);

    // Transform animations
    void MoveTo(float duration, float x, float y);
    void MoveTo(float duration, float x, float y, Easing easing);
    void Scale(float duration, float targetScale);
    void Scale(float duration, float targetScale, Easing easing);
    void Rotate(float duration, float degrees);
    void Rotate(float duration, float degrees, Easing easing);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

private:
    friend class Catalyst;
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    Catalyst* parent;
    size_t imageIndex;
    ImageElement(Catalyst* p, size_t idx);
};

class Catalyst {
public:
    Catalyst(uint32_t width, uint32_t height);
    ~Catalyst();

    TextElement setText(const std::string& text);
    MathElement setMath(const std::string& latex);  // LaTeX formula

    // Shape factory methods
    ShapeElement setCircle(float radius);
    ShapeElement setRectangle(float width, float height);
    ShapeElement setLine(float x1, float y1, float x2, float y2);
    ShapeElement setTriangle(float size);  // Equilateral triangle
    ShapeElement setArrow(float x1, float y1, float x2, float y2);  // Arrow from (x1,y1) to (x2,y2)
    ShapeElement setArc(float radius, float startAngle, float endAngle);  // Arc from startAngle to endAngle (degrees)
    ShapeElement setEllipse(float radiusX, float radiusY);  // Ellipse with separate X/Y radii
    ShapeElement setDot();  // Small dot (default 5px radius)
    ShapeElement setDot(float radius);  // Dot with custom radius
    ShapeElement setPolygon(const std::vector<float>& x, const std::vector<float>& y);  // Arbitrary polygon
    ShapeElement setRegularPolygon(int sides, float radius);  // N-sided regular polygon
    ShapeElement setStar(int points, float outerRadius);  // Star with default inner radius (40% of outer)
    ShapeElement setStar(int points, float outerRadius, float innerRadius);  // Star with custom inner radius
    ShapeElement setDoubleArrow(float x1, float y1, float x2, float y2);  // Arrow with heads on both ends
    ShapeElement setBrace(float height);  // Curly brace with default width
    ShapeElement setBrace(float height, float width);  // Curly brace with custom width
    ShapeElement setBrace(float height, float width, bool flipped);  // Curly brace, flipped = } instead of {
    ShapeElement setRoundedRectangle(float width, float height, float cornerRadius);  // Rectangle with rounded corners
    ShapeElement setCubicBezier(float p0x, float p0y, float p1x, float p1y,
                                float p2x, float p2y, float p3x, float p3y);  // Cubic Bezier curve

    // Axes and Graph factory methods
    AxesElement setAxes(float xMin, float xMax, float yMin, float yMax);
    GraphElement setGraph(AxesElement& axes, const std::vector<float>& x, const std::vector<float>& y);
    GraphElement setGraph(AxesElement& axes, std::function<float(float)> func, int numPoints = 100);

    // Vector factory method (mathematical vector arrow)
    VectorElement setVector(AxesElement& axes, const std::vector<float>& components);

    // Number line factory method
    NumberLineElement setNumberLine(float min, float max);
    NumberLineElement setNumberLine(float min, float max, bool vertical);

    // Chart factory methods
    BarChartElement setBarChart(const std::vector<float>& values);
    BarChartElement setBarChart(const std::vector<float>& values, const std::vector<std::string>& labels);
    PieChartElement setPieChart(const std::vector<float>& values, float radius);

    // Vector field factory method
    VectorFieldElement setVectorField(AxesElement& axes,
        std::function<std::pair<float, float>(float, float)> func,
        int gridX = 10, int gridY = 10);

    // Custom path factory method (builder pattern)
    PathBuilder beginPath();

    // Table factory method
    TableElement setTable(const std::vector<std::vector<std::string>>& data);

    // Image factory methods
    ImageElement setImage(const std::string& path);
    ImageElement setImage(const std::string& path, float width, float height);

    // 3D Mode and Elements
    void set3DMode(bool enable);  // Enable 3D rendering for current scene

    // 3D Axes factory method
    Axes3DElement setAxes3D(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);

    // 3D Surface factory methods
    Surface3DElement setSurface(std::function<float(float, float)> heightFunc,
                                float xMin, float xMax, float yMin, float yMax);
    Surface3DElement setSurface(std::function<float(float, float)> heightFunc,
                                float xMin, float xMax, float yMin, float yMax,
                                int uSegments, int vSegments);

    // 3D Shape factory methods
    Shape3DElement setSphere(float radius);
    Shape3DElement setSphere(float radius, int segments);
    Shape3DElement setCube3D(float size);
    Shape3DElement setCube3D(float width, float height, float depth);
    Shape3DElement setCylinder(float radius, float height);
    Shape3DElement setCylinder(float radius, float height, int segments);
    Shape3DElement setCone(float radius, float height);
    Shape3DElement setCone(float radius, float height, int segments);
    Shape3DElement setArrow3D(float x1, float y1, float z1, float x2, float y2, float z2);

    // 3D Camera controls
    void setCamera3D(float eyeX, float eyeY, float eyeZ,
                     float targetX, float targetY, float targetZ);
    void setCamera3D(float eyeX, float eyeY, float eyeZ,
                     float targetX, float targetY, float targetZ, float duration);
    void setCameraFOV(float fov);
    void setCameraFOV(float fov, float duration);
    void orbitCamera(float theta, float phi, float distance);
    void orbitCamera(float theta, float phi, float distance, float duration);
    void resetCamera3D();
    void resetCamera3D(float duration);

    // Scene & Camera
    void setBackground(const std::string& hex);    // Set background color via hex
    void setBackground(int r, int g, int b);       // Set background color via RGB (0-255)
    void setBackground(float r, float g, float b); // Set background color via normalized RGB (0-1)

    // Camera controls
    void setCameraZoom(float zoom);                         // Set camera zoom instantly
    void setCameraZoom(float zoom, float duration);         // Animate camera zoom
    void setCameraPan(float x, float y);                    // Set camera pan instantly (NDC units)
    void setCameraPan(float x, float y, float duration);    // Animate camera pan
    void setCameraRotate(float degrees);                    // Set camera rotation instantly
    void setCameraRotate(float degrees, float duration);    // Animate camera rotation
    void resetCamera();                                      // Reset camera to default instantly
    void resetCamera(float duration);                        // Animate camera reset

    // Scene management
    Scene createScene();                                     // Create a new scene
    void setActiveScene(size_t index);                       // Switch to scene by index
    void setActiveScene(Scene& scene);                       // Switch to scene
    Scene& getScene(size_t index);                           // Get scene by index
    size_t getActiveSceneIndex() const;                      // Get active scene index

    // Animation groups (parallel/sequential)
    AnimationGroup createAnimationGroup();                   // Create parallel group (lag_ratio=0)
    AnimationGroup createSequence();                         // Create sequential group (lag_ratio=1)

    // Element groups (for layout and batch operations)
    Group createGroup();                                     // Create empty group

    void wait(float seconds);   // Delay subsequent operations by X seconds
    void clear();               // Remove all elements and reset timeline
    void run();

private:
    friend class TextElement;
    friend class MathElement;
    friend class ShapeElement;
    friend class AxesElement;
    friend class GraphElement;
    friend class VectorElement;
    friend class NumberLineElement;
    friend class BarChartElement;
    friend class PieChartElement;
    friend class VectorFieldElement;
    friend class PathBuilder;
    friend class TableElement;
    friend class ImageElement;
    friend class Scene;
    friend class Axes3DElement;
    friend class Surface3DElement;
    friend class Shape3DElement;
    friend class AnimationGroup;
    friend class Group;
    class Impl;
    Impl* pImpl;
};

// Scene class for multi-scene animations
class Scene {
public:
    // Element creation (mirrors Catalyst API)
    TextElement setText(const std::string& text);
    MathElement setMath(const std::string& latex);
    ShapeElement setCircle(float radius);
    ShapeElement setRectangle(float width, float height);
    ShapeElement setLine(float x1, float y1, float x2, float y2);
    ShapeElement setTriangle(float size);
    ShapeElement setArrow(float x1, float y1, float x2, float y2);

    // Scene-specific settings
    void setBackground(const std::string& hex);
    void setBackground(int r, int g, int b);
    void setBackground(float r, float g, float b);

    // Scene camera controls
    void setCameraZoom(float zoom);
    void setCameraZoom(float zoom, float duration);
    void setCameraPan(float x, float y);
    void setCameraPan(float x, float y, float duration);
    void setCameraRotate(float degrees);
    void setCameraRotate(float degrees, float duration);
    void resetCamera();
    void resetCamera(float duration);

    // Timeline
    void wait(float seconds);

    // Get scene index
    size_t getIndex() const { return sceneIndex; }

private:
    friend class Catalyst;
    Catalyst* parent;
    size_t sceneIndex;
    Scene(Catalyst* p, size_t idx) : parent(p), sceneIndex(idx) {}
};

// Animation type for groups
enum class GroupAnimationType {
    Show,      // FadeIn
    Hide,      // FadeOut
    MoveTo,    // Move to position
    Scale,     // Scale animation
    Rotate     // Rotation animation
};

// Animation group for parallel/sequential animations with lag ratio
class AnimationGroup {
public:
    // Add elements with show animation
    AnimationGroup& addShow(TextElement& el, float duration);
    AnimationGroup& addShow(MathElement& el, float duration);
    AnimationGroup& addShow(ShapeElement& el, float duration);
    AnimationGroup& addShow(ImageElement& el, float duration);
    AnimationGroup& addShow(Shape3DElement& el, float duration);

    // Add elements with hide animation
    AnimationGroup& addHide(TextElement& el, float duration);
    AnimationGroup& addHide(MathElement& el, float duration);
    AnimationGroup& addHide(ShapeElement& el, float duration);
    AnimationGroup& addHide(ImageElement& el, float duration);
    AnimationGroup& addHide(Shape3DElement& el, float duration);

    // Configuration
    AnimationGroup& setLagRatio(float ratio);  // 0=parallel (default), 1=sequential
    AnimationGroup& setEasing(Easing easing);  // Set easing for all animations

    // Execute the animation group
    void play();

private:
    friend class Catalyst;
    Catalyst* parent;

    enum class ElementType { Text, Math, Shape, Image, Shape3D };

    struct Entry {
        ElementType type;
        size_t index;
        GroupAnimationType animType;
        float duration;
        Easing easing = Easing::EaseOutCubic;
    };

    std::vector<Entry> entries;
    float lagRatio = 0.0f;
    Easing groupEasing = Easing::EaseOutCubic;

    AnimationGroup(Catalyst* p) : parent(p) {}
};

// Group class for grouping elements and layout arrangement (like Manim's VGroup)
class Group {
public:
    // Member management - add individual elements
    Group& add(TextElement& element);
    Group& add(MathElement& element);
    Group& add(ShapeElement& element);
    Group& add(ImageElement& element);
    Group& add(GraphElement& element);
    Group& add(AxesElement& element);
    Group& add(VectorElement& element);
    Group& add(Shape3DElement& element);
    Group& add(Group& subgroup);  // For nesting

    // Remove elements
    Group& remove(TextElement& element);
    Group& remove(MathElement& element);
    Group& remove(ShapeElement& element);
    Group& remove(ImageElement& element);
    Group& remove(GraphElement& element);
    Group& remove(AxesElement& element);
    Group& remove(VectorElement& element);
    Group& remove(Shape3DElement& element);
    Group& remove(Group& subgroup);

    // Clear all members
    void clear();

    // Query
    size_t size() const;
    bool empty() const;

    // Positioning
    void setPosition(float x, float y);       // Pixel position
    void setPosition(Position anchor);        // Anchor preset

    // Layout arrangement
    void arrange(Direction direction, float spacing = 20.0f);
    void arrangeInGrid(int cols, float hSpacing = 20.0f, float vSpacing = 20.0f);

    // Z-ordering (sets z-index for all members recursively)
    void setZIndex(int zIndex);
    int getZIndex() const;

    // Bounds query
    void getBounds(float& minX, float& minY, float& maxX, float& maxY) const;
    float getWidth() const;
    float getHeight() const;

    // Easing control
    Group& setEasing(Easing easing);

    // Timing control
    Group& delay(float seconds);
    Group& then();

    // Animations (recursive to all members)
    void show(float duration);
    void show(float duration, Easing easing);
    void show(float duration, Direction direction);
    void show(float duration, Direction direction, float shiftAmount);
    void show(float duration, Direction direction, float shiftAmount, Easing easing);

    void hide(float duration);
    void hide(float duration, Easing easing);
    void hide(float duration, Direction direction);
    void hide(float duration, Direction direction, float shiftAmount);
    void hide(float duration, Direction direction, float shiftAmount, Easing easing);

    // Transform animations (applied to all members)
    void MoveTo(float duration, float x, float y);
    void MoveTo(float duration, float x, float y, Easing easing);
    void Scale(float duration, float targetScale);
    void Scale(float duration, float targetScale, Easing easing);
    void Rotate(float duration, float degrees);
    void Rotate(float duration, float degrees, Easing easing);

private:
    friend class Catalyst;
    friend class Scene;
    Catalyst* parent;
    size_t groupIndex;

    Group(Catalyst* p, size_t idx) : parent(p), groupIndex(idx) {}
};
