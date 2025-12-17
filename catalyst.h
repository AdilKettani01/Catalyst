#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <initializer_list>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include "vid_utils.h"

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
class ValueTracker;       // Forward declaration for per-frame value tracking
class Submobject;         // Forward declaration for group indexing

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
    void setText(const std::string& text);  // Update text content (marks geometry dirty)
    void setPosition(float x, float y);   // Precise pixel positioning
    void setPosition(Position anchor);    // Preset anchor positioning

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    TextElement& fix_in_frame();
    TextElement& unfix_from_frame();

    // Spatial helpers (Manim-style, instant)
    void shift(float dx, float dy);
    void shift(Direction direction, float amount = 20.0f);
    void move_to(float x, float y);  // Center-based
    void next_to(const Submobject& other, Direction direction = Direction::RIGHT, float buff = 20.0f);
    void to_edge(Direction direction, float buff = 20.0f);
    void to_corner(Position corner, float buff = 20.0f);
    void align_to(const Submobject& other, Direction direction);
    void setColor(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setColor(int r, int g, int b);         // RGB: 0-255
    void setColor(float h, float s, float l);   // HSL: h(0-360), s(0-1), l(0-1)
    void setOpacity(float opacity);             // 0.0-1.0 (applies to both fill and stroke)

    // Stroke/outline methods
    void setStroke(float width);                      // Stroke width in pixels
    void setStrokeColor(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setStrokeColor(int r, int g, int b);         // RGB: 0-255
    void setStrokeOpacity(float opacity);             // 0.0-1.0

    // Fill opacity
    void setFillOpacity(float opacity);               // 0.0-1.0

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

    // Updaters (Manim-style per-frame callbacks)
    TextElement& add_updater(std::function<void(TextElement&)> updater);
    TextElement& add_updater(std::function<void(TextElement&, float)> updater);
    TextElement& clear_updaters();

private:
    friend class Catalyst;
    friend class MathElement;  // For cross-type Transform access
    friend class Scene;        // For scene-based element creation
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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
    void setMath(const std::string& latex);  // Update LaTeX content (marks texture dirty)

    // Per-substring coloring (Manim-style): colors occurrences of `tex` within the LaTeX source.
    MathElement& set_color_by_tex(const std::string& tex, const std::string& color);
    MathElement& set_color_by_tex(const std::string& tex, int r, int g, int b);

    void setPosition(float x, float y);   // Precise pixel positioning
    void setPosition(Position anchor);    // Preset anchor positioning

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    MathElement& fix_in_frame();
    MathElement& unfix_from_frame();

    // Spatial helpers (Manim-style, instant)
    void shift(float dx, float dy);
    void shift(Direction direction, float amount = 20.0f);
    void move_to(float x, float y);  // Center-based
    void next_to(const Submobject& other, Direction direction = Direction::RIGHT, float buff = 20.0f);
    void to_edge(Direction direction, float buff = 20.0f);
    void to_corner(Position corner, float buff = 20.0f);
    void align_to(const Submobject& other, Direction direction);
    void setColor(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setColor(int r, int g, int b);         // RGB: 0-255
    void setColor(float h, float s, float l);   // HSL: h(0-360), s(0-1), l(0-1)
    void setOpacity(float opacity);             // 0.0-1.0

    // Relative positioning
    void under(TextElement& other, float padding = 20.0f);  // Position below a TextElement
    void under(MathElement& other, float padding = 20.0f);  // Position below another MathElement

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

    // Updaters (Manim-style per-frame callbacks)
    MathElement& add_updater(std::function<void(MathElement&)> updater);
    MathElement& add_updater(std::function<void(MathElement&, float)> updater);
    MathElement& clear_updaters();

private:
    friend class Catalyst;
    friend class TextElement;  // For cross-type Transform access
    friend class Scene;        // For scene-based element creation
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    ShapeElement& fix_in_frame();
    ShapeElement& unfix_from_frame();

    // Spatial helpers (Manim-style, instant)
    void shift(float dx, float dy);
    void shift(Direction direction, float amount = 20.0f);
    void move_to(float x, float y);  // Center-based
    void next_to(const Submobject& other, Direction direction = Direction::RIGHT, float buff = 20.0f);
    void to_edge(Direction direction, float buff = 20.0f);
    void to_corner(Position corner, float buff = 20.0f);
    void align_to(const Submobject& other, Direction direction);

    // Fill color
    void setFill(const std::string& hex);      // Hex: "#FF5733" or "FF5733"
    void setFill(int r, int g, int b);         // RGB: 0-255
    void setFillOpacity(float opacity);        // 0.0-1.0

    // Stroke/outline
    void setStroke(float width);                      // Stroke width in pixels
    void setStrokeColor(const std::string& hex);      // Hex color
    void setStrokeColor(int r, int g, int b);         // RGB: 0-255
    void setStrokeOpacity(float opacity);             // 0.0-1.0
    void setOpacity(float opacity);                   // 0.0-1.0 (applies to both fill and stroke)

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

    // Updaters (Manim-style per-frame callbacks)
    ShapeElement& add_updater(std::function<void(ShapeElement&)> updater);
    ShapeElement& add_updater(std::function<void(ShapeElement&, float)> updater);
    ShapeElement& clear_updaters();

    // VMobject-style path API (bezier editing / partial curves)
    ShapeElement& set_points_as_corners(const std::vector<std::pair<float, float>>& points);
    ShapeElement& set_points_smoothly(const std::vector<std::pair<float, float>>& points);
    std::pair<float, float> get_start() const;
    std::pair<float, float> get_end() const;
    std::pair<float, float> point_from_proportion(float alpha) const;
    ShapeElement get_subcurve(float startAlpha, float endAlpha) const;
    ShapeElement& put_start_and_end_on(float startX, float startY, float endX, float endY);

private:
    friend class Catalyst;
    friend class PathBuilder;  // PathBuilder needs to create ShapeElements
    friend class Scene;        // For scene-based element creation
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    AxesElement& fix_in_frame();
    AxesElement& unfix_from_frame();

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
    friend class Submobject;   // For group indexing
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

    // Updaters (Manim-style per-frame callbacks)
    Axes3DElement& add_updater(std::function<void(Axes3DElement&)> updater);
    Axes3DElement& add_updater(std::function<void(Axes3DElement&, float)> updater);
    Axes3DElement& clear_updaters();

private:
    friend class Catalyst;
    friend class Scene;
    friend class Submobject;   // For group indexing
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

    // Updaters (Manim-style per-frame callbacks)
    Surface3DElement& add_updater(std::function<void(Surface3DElement&)> updater);
    Surface3DElement& add_updater(std::function<void(Surface3DElement&, float)> updater);
    Surface3DElement& clear_updaters();

private:
    friend class Catalyst;
    friend class Scene;
    friend class Submobject;   // For group indexing
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

    // Updaters (Manim-style per-frame callbacks)
    Shape3DElement& add_updater(std::function<void(Shape3DElement&)> updater);
    Shape3DElement& add_updater(std::function<void(Shape3DElement&, float)> updater);
    Shape3DElement& clear_updaters();

private:
    friend class Catalyst;
    friend class Scene;
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    GraphElement& fix_in_frame();
    GraphElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

    // Updaters (Manim-style per-frame callbacks)
    GraphElement& add_updater(std::function<void(GraphElement&)> updater);
    GraphElement& add_updater(std::function<void(GraphElement&, float)> updater);
    GraphElement& clear_updaters();

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    VectorElement& fix_in_frame();
    VectorElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

    // Z-ordering
    void setZIndex(int zIndex);    // Set rendering order (higher = on top)
    int getZIndex() const;         // Get current z-index

    // Updaters (Manim-style per-frame callbacks)
    VectorElement& add_updater(std::function<void(VectorElement&)> updater);
    VectorElement& add_updater(std::function<void(VectorElement&, float)> updater);
    VectorElement& clear_updaters();

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    NumberLineElement& fix_in_frame();
    NumberLineElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    BarChartElement& fix_in_frame();
    BarChartElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    PieChartElement& fix_in_frame();
    PieChartElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    VectorFieldElement& fix_in_frame();
    VectorFieldElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    TableElement& fix_in_frame();
    TableElement& unfix_from_frame();

    // Animations
    void show(float duration);
    void hide(float duration);

private:
    friend class Catalyst;
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
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
    void setOpacity(float opacity);  // 0.0-1.0

    // Positioning
    void setPosition(float x, float y);
    void setPosition(Position anchor);

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    ImageElement& fix_in_frame();
    ImageElement& unfix_from_frame();

    // Spatial helpers (Manim-style, instant)
    void shift(float dx, float dy);
    void shift(Direction direction, float amount = 20.0f);
    void move_to(float x, float y);  // Center-based
    void next_to(const Submobject& other, Direction direction = Direction::RIGHT, float buff = 20.0f);
    void to_edge(Direction direction, float buff = 20.0f);
    void to_corner(Position corner, float buff = 20.0f);
    void align_to(const Submobject& other, Direction direction);

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

    // Updaters (Manim-style per-frame callbacks)
    ImageElement& add_updater(std::function<void(ImageElement&)> updater);
    ImageElement& add_updater(std::function<void(ImageElement&, float)> updater);
    ImageElement& clear_updaters();

private:
    friend class Catalyst;
    friend class AnimationGroup;  // For animation group access
    friend class Group;        // For group operations
    friend class Submobject;   // For group indexing
    Catalyst* parent;
    size_t imageIndex;
    ImageElement(Catalyst* p, size_t idx);
};

// Manim-style numeric tracker used for per-frame callbacks and reactive redraw.
class ValueTracker {
public:
    // Easing / timing (applies to subsequent `animate_to` calls)
    ValueTracker& setEasing(Easing easing);
    ValueTracker& delay(float seconds);
    ValueTracker& then();

    // Value access
    float get_value() const;
    void set_value(float value);
    void increment_value(float delta);

    // Schedule value animation on the scene timeline
    void animate_to(float value, float duration);
    void animate_to(float value, float duration, Easing easing);

    // Updaters (Manim-style per-frame callbacks)
    ValueTracker& add_updater(std::function<void(ValueTracker&)> updater);
    ValueTracker& add_updater(std::function<void(ValueTracker&, float)> updater);
    ValueTracker& clear_updaters();

private:
    friend class Catalyst;
    friend class Scene;
    Catalyst* parent;
    size_t trackerIndex;
    ValueTracker(Catalyst* p, size_t idx) : parent(p), trackerIndex(idx) {}
};

class Catalyst {
public:
    Catalyst(uint32_t width, uint32_t height);
    ~Catalyst();

    TextElement setText(const std::string& text);
    MathElement setMath(const std::string& latex);  // LaTeX formula
    MathElement setMath(const std::string& latex, std::initializer_list<std::pair<std::string, std::string>> t2c);

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
    VectorElement setVector(AxesElement& axes, float vx, float vy);
    VectorElement setVector(GraphElement& graph, const std::vector<float>& components);
    VectorElement setVector(GraphElement& graph, float vx, float vy);

    template <typename Derived>
    VectorElement setVector(AxesElement& axes, const Eigen::MatrixBase<Derived>& components) {
        const Eigen::Index n = components.size();
        const float vx = (n > 0) ? static_cast<float>(components.derived().coeff(0)) : 0.0f;
        const float vy = (n > 1) ? static_cast<float>(components.derived().coeff(1)) : 0.0f;
        return setVector(axes, vx, vy);
    }

    template <typename Derived>
    VectorElement setVector(GraphElement& graph, const Eigen::MatrixBase<Derived>& components) {
        const Eigen::Index n = components.size();
        const float vx = (n > 0) ? static_cast<float>(components.derived().coeff(0)) : 0.0f;
        const float vy = (n > 1) ? static_cast<float>(components.derived().coeff(1)) : 0.0f;
        return setVector(graph, vx, vy);
    }

    template <typename VecLike>
    VectorElement setVector(AxesElement& axes, const VecLike& components)
        requires(!std::is_base_of_v<Eigen::EigenBase<VecLike>, VecLike>) && requires(const VecLike& v) { v.size(); } &&
                (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); } ||
                 requires(const VecLike& v) { static_cast<float>(v[std::size_t{0}]); }) {
        const std::size_t n = static_cast<std::size_t>(components.size());
        float vx = 0.0f;
        float vy = 0.0f;
        if (n > 0) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vx = static_cast<float>(components(std::size_t{0}));
            } else {
                vx = static_cast<float>(components[std::size_t{0}]);
            }
        }
        if (n > 1) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vy = static_cast<float>(components(std::size_t{1}));
            } else {
                vy = static_cast<float>(components[std::size_t{1}]);
            }
        }
        return setVector(axes, vx, vy);
    }

    template <typename VecLike>
    VectorElement setVector(GraphElement& graph, const VecLike& components)
        requires(!std::is_base_of_v<Eigen::EigenBase<VecLike>, VecLike>) && requires(const VecLike& v) { v.size(); } &&
                (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); } ||
                 requires(const VecLike& v) { static_cast<float>(v[std::size_t{0}]); }) {
        const std::size_t n = static_cast<std::size_t>(components.size());
        float vx = 0.0f;
        float vy = 0.0f;
        if (n > 0) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vx = static_cast<float>(components(std::size_t{0}));
            } else {
                vx = static_cast<float>(components[std::size_t{0}]);
            }
        }
        if (n > 1) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vy = static_cast<float>(components(std::size_t{1}));
            } else {
                vy = static_cast<float>(components[std::size_t{1}]);
            }
        }
        return setVector(graph, vx, vy);
    }

    // Batch vector creation. For matrix input: if rows==dims, uses columns as vectors; else if cols==dims, uses rows.
    template <typename Derived>
    Group setVectors(AxesElement& axes, const Eigen::MatrixBase<Derived>& vectors);
    template <typename Derived>
    Group setVectors(GraphElement& graph, const Eigen::MatrixBase<Derived>& vectors);
    template <typename Tensor2D>
    Group setVectors(AxesElement& axes, const Tensor2D& vectors)
        requires requires(const Tensor2D& t) {
            t.shape().size();
            t.shape()[0];
            t.shape()[1];
            static_cast<float>(t(std::size_t{0}, std::size_t{0}));
        };
    template <typename Tensor2D>
    Group setVectors(GraphElement& graph, const Tensor2D& vectors)
        requires requires(const Tensor2D& t) {
            t.shape().size();
            t.shape()[0];
            t.shape()[1];
            static_cast<float>(t(std::size_t{0}, std::size_t{0}));
        };

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

    // SVG import (Manim-style SVGMobject)
    Group SVGMobject(const std::string& path);
    Group SVGMobject(const std::string& path, float targetHeightPx);

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
    Shape3DElement setVector(Axes3DElement& axes, float vx, float vy, float vz);

    template <typename Derived>
    Shape3DElement setVector(Axes3DElement& axes, const Eigen::MatrixBase<Derived>& components) {
        const Eigen::Index n = components.size();
        const float vx = (n > 0) ? static_cast<float>(components.derived().coeff(0)) : 0.0f;
        const float vy = (n > 1) ? static_cast<float>(components.derived().coeff(1)) : 0.0f;
        const float vz = (n > 2) ? static_cast<float>(components.derived().coeff(2)) : 0.0f;
        return setVector(axes, vx, vy, vz);
    }

    template <typename VecLike>
    Shape3DElement setVector(Axes3DElement& axes, const VecLike& components)
        requires(!std::is_base_of_v<Eigen::EigenBase<VecLike>, VecLike>) && requires(const VecLike& v) { v.size(); } &&
                (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); } ||
                 requires(const VecLike& v) { static_cast<float>(v[std::size_t{0}]); }) {
        const std::size_t n = static_cast<std::size_t>(components.size());
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        if (n > 0) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vx = static_cast<float>(components(std::size_t{0}));
            } else {
                vx = static_cast<float>(components[std::size_t{0}]);
            }
        }
        if (n > 1) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vy = static_cast<float>(components(std::size_t{1}));
            } else {
                vy = static_cast<float>(components[std::size_t{1}]);
            }
        }
        if (n > 2) {
            if constexpr (requires(const VecLike& v) { static_cast<float>(v(std::size_t{0})); }) {
                vz = static_cast<float>(components(std::size_t{2}));
            } else {
                vz = static_cast<float>(components[std::size_t{2}]);
            }
        }
        return setVector(axes, vx, vy, vz);
    }

    template <typename Derived>
    Group setVectors(Axes3DElement& axes, const Eigen::MatrixBase<Derived>& vectors);
    template <typename Tensor2D>
    Group setVectors(Axes3DElement& axes, const Tensor2D& vectors)
        requires requires(const Tensor2D& t) {
            t.shape().size();
            t.shape()[0];
            t.shape()[1];
            static_cast<float>(t(std::size_t{0}, std::size_t{0}));
        };

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
    void setCamera3DTarget(float targetX, float targetY, float targetZ);
    void setCamera3DTarget(float targetX, float targetY, float targetZ, float duration);
    void shiftCamera3D(float dx, float dy, float dz);
    void shiftCamera3D(float dx, float dy, float dz, float duration);
    void setCamera3DDistance(float distance);
    void setCamera3DDistance(float distance, float duration);
    void scaleCamera3DDistance(float scale);
    void scaleCamera3DDistance(float scale, float duration);
    void rotateCamera3D(float axisX, float axisY, float axisZ, float angleRadians);
    void rotateCamera3D(float axisX, float axisY, float axisZ, float angleRadians, float duration);
    void beginAmbientCameraRotation(float rateRadians);
    void stopAmbientCameraRotation();
    void reorientCamera(float thetaDegrees, float phiDegrees);
    void reorientCamera(float thetaDegrees, float phiDegrees, float duration);

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

    // Manim-style value tracker (updated during run())
    ValueTracker createValueTracker(float initialValue = 0.0f);

    void wait(float seconds);   // Delay subsequent operations by X seconds
    void clear();               // Remove all elements and reset timeline
    void run();

private:
    friend class TextElement;
    friend class MathElement;
    friend class Submobject;
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
    friend class ValueTracker;
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
    MathElement setMath(const std::string& latex, std::initializer_list<std::pair<std::string, std::string>> t2c);
    ShapeElement setCircle(float radius);
    ShapeElement setRectangle(float width, float height);
    ShapeElement setLine(float x1, float y1, float x2, float y2);
    ShapeElement setTriangle(float size);
    ShapeElement setArrow(float x1, float y1, float x2, float y2);
    Group SVGMobject(const std::string& path);
    Group SVGMobject(const std::string& path, float targetHeightPx);
    ValueTracker createValueTracker(float initialValue = 0.0f);

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

// Python-style slice helper for group indexing:
// - `group[1]` -> element at index 1 (supports negative indices)
// - `group[Slice::to(3)]` -> elements [0, 3)
// - `group[Slice::from(2)]` -> elements [2, end)
// - `group[Slice::between(1, 4)]` -> elements [1, 4)
struct Slice {
    static constexpr int kNone = (std::numeric_limits<int>::max)();

    int start = kNone;  // Inclusive
    int stop = kNone;   // Exclusive
    int step = 1;

    constexpr Slice() = default;
    constexpr explicit Slice(int start) : start(start) {}
    constexpr Slice(int start, int stop, int step = 1) : start(start), stop(stop), step(step) {}

    static constexpr Slice all() { return Slice(kNone, kNone, 1); }
    static constexpr Slice from(int start, int step = 1) { return Slice(start, kNone, step); }
    static constexpr Slice to(int stop, int step = 1) { return Slice(kNone, stop, step); }
    static constexpr Slice between(int start, int stop, int step = 1) { return Slice(start, stop, step); }
};

enum class SubmobjectType {
    Invalid,
    Text,
    Math,
    Shape,
    Image,
    Graph,
    Axes,
    NumberLine,
    BarChart,
    PieChart,
    VectorField,
    Table,
    Vector,
    Shape3D,
    Axes3D,
    Surface3D,
    Group,
};

// Variant handle returned by `Group::operator[]` (Manim-style submobject indexing).
class Submobject {
public:
    Submobject() = default;

    // Converting constructors to enable passing core elements into helpers that accept `Submobject`.
    Submobject(const TextElement& element);
    Submobject(const MathElement& element);
    Submobject(const ShapeElement& element);
    Submobject(const ImageElement& element);
    Submobject(const Group& group);

    bool valid() const;
    SubmobjectType type() const;

    // Indexing/slicing only valid when this is a Group.
    Submobject operator[](int index) const;
    Group operator[](Slice slice) const;

    // Common operations (no-ops when unsupported by the underlying element type).
    void show(float duration);
    void hide(float duration);
    void MoveTo(float duration, float x, float y);
    void Scale(float duration, float targetScale);
    void Rotate(float duration, float degrees);

    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    void fix_in_frame();
    void unfix_from_frame();

    // Spatial helpers (Manim-style, instant)
    void shift(float dx, float dy);
    void shift(Direction direction, float amount = 20.0f);
    void move_to(float x, float y);  // Center-based
    void next_to(const Submobject& other, Direction direction = Direction::RIGHT, float buff = 20.0f);
    void to_edge(Direction direction, float buff = 20.0f);
    void to_corner(Position corner, float buff = 20.0f);
    void align_to(const Submobject& other, Direction direction);

private:
    friend class Group;
    Catalyst* parent = nullptr;
    SubmobjectType subType = SubmobjectType::Invalid;
    size_t elementIndex = 0;

    Submobject(Catalyst* p, SubmobjectType t, size_t idx) : parent(p), subType(t), elementIndex(idx) {}
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

    // Submobject-style indexing/slicing
    Submobject operator[](int index) const;
    Group operator[](Slice slice) const;

    // Positioning
    void setPosition(float x, float y);       // Pixel position
    void setPosition(Position anchor);        // Anchor preset

    // Spatial helpers (Manim-style, instant)
    void shift(float dx, float dy);
    void shift(Direction direction, float amount = 20.0f);
    void move_to(float x, float y);  // Center-based
    void next_to(const Submobject& other, Direction direction = Direction::RIGHT, float buff = 20.0f);
    void to_edge(Direction direction, float buff = 20.0f);
    void to_corner(Position corner, float buff = 20.0f);
    void align_to(const Submobject& other, Direction direction);

    // Fix/unfix relative to the camera frame (HUD overlay): ignores camera pan/zoom/rotate.
    Group& fix_in_frame();
    Group& unfix_from_frame();

    // Styling (recursive to members when supported)
    void setColor(const std::string& hex);
    void setColor(int r, int g, int b);
    void setOpacity(float opacity);
    void setFill(const std::string& hex);
    void setFill(int r, int g, int b);
    void setFillOpacity(float opacity);
    void setStroke(float width);
    void setStrokeColor(const std::string& hex);
    void setStrokeColor(int r, int g, int b);
    void setStrokeOpacity(float opacity);

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

    // Updaters (Manim-style per-frame callbacks)
    Group& add_updater(std::function<void(Group&)> updater);
    Group& add_updater(std::function<void(Group&, float)> updater);
    Group& clear_updaters();

private:
    friend class Catalyst;
    friend class Scene;
    friend class Submobject;
    Catalyst* parent;
    size_t groupIndex;

    Group(Catalyst* p, size_t idx) : parent(p), groupIndex(idx) {}
};

// ================== Catalyst::setVectors template implementations ==================
template <typename Derived>
inline Group Catalyst::setVectors(AxesElement& axes, const Eigen::MatrixBase<Derived>& vectors) {
    Group group = createGroup();
    const auto& m = vectors.derived();
    const Eigen::Index rows = m.rows();
    const Eigen::Index cols = m.cols();

    if (rows == 2) {
        for (Eigen::Index j = 0; j < cols; ++j) {
            auto v = setVector(axes, m.col(j));
            group.add(v);
        }
    } else if (cols == 2) {
        for (Eigen::Index i = 0; i < rows; ++i) {
            auto v = setVector(axes, m.row(i));
            group.add(v);
        }
    } else if (m.size() == 2) {
        auto v = setVector(axes, m);
        group.add(v);
    }

    return group;
}

template <typename Derived>
inline Group Catalyst::setVectors(GraphElement& graph, const Eigen::MatrixBase<Derived>& vectors) {
    Group group = createGroup();
    const auto& m = vectors.derived();
    const Eigen::Index rows = m.rows();
    const Eigen::Index cols = m.cols();

    if (rows == 2) {
        for (Eigen::Index j = 0; j < cols; ++j) {
            auto v = setVector(graph, m.col(j));
            group.add(v);
        }
    } else if (cols == 2) {
        for (Eigen::Index i = 0; i < rows; ++i) {
            auto v = setVector(graph, m.row(i));
            group.add(v);
        }
    } else if (m.size() == 2) {
        auto v = setVector(graph, m);
        group.add(v);
    }

    return group;
}

template <typename Tensor2D>
inline Group Catalyst::setVectors(AxesElement& axes, const Tensor2D& vectors)
    requires requires(const Tensor2D& t) {
        t.shape().size();
        t.shape()[0];
        t.shape()[1];
        static_cast<float>(t(std::size_t{0}, std::size_t{0}));
    } {
    Group group = createGroup();
    const auto& shape = vectors.shape();
    if (shape.size() < 2) return group;

    const std::size_t rows = static_cast<std::size_t>(shape[0]);
    const std::size_t cols = static_cast<std::size_t>(shape[1]);

    if (rows == 2) {
        for (std::size_t j = 0; j < cols; ++j) {
            auto v = setVector(axes, static_cast<float>(vectors(std::size_t{0}, j)),
                               static_cast<float>(vectors(std::size_t{1}, j)));
            group.add(v);
        }
    } else if (cols == 2) {
        for (std::size_t i = 0; i < rows; ++i) {
            auto v = setVector(axes, static_cast<float>(vectors(i, std::size_t{0})),
                               static_cast<float>(vectors(i, std::size_t{1})));
            group.add(v);
        }
    }

    return group;
}

template <typename Tensor2D>
inline Group Catalyst::setVectors(GraphElement& graph, const Tensor2D& vectors)
    requires requires(const Tensor2D& t) {
        t.shape().size();
        t.shape()[0];
        t.shape()[1];
        static_cast<float>(t(std::size_t{0}, std::size_t{0}));
    } {
    Group group = createGroup();
    const auto& shape = vectors.shape();
    if (shape.size() < 2) return group;

    const std::size_t rows = static_cast<std::size_t>(shape[0]);
    const std::size_t cols = static_cast<std::size_t>(shape[1]);

    if (rows == 2) {
        for (std::size_t j = 0; j < cols; ++j) {
            auto v = setVector(graph, static_cast<float>(vectors(std::size_t{0}, j)),
                               static_cast<float>(vectors(std::size_t{1}, j)));
            group.add(v);
        }
    } else if (cols == 2) {
        for (std::size_t i = 0; i < rows; ++i) {
            auto v = setVector(graph, static_cast<float>(vectors(i, std::size_t{0})),
                               static_cast<float>(vectors(i, std::size_t{1})));
            group.add(v);
        }
    }

    return group;
}

template <typename Derived>
inline Group Catalyst::setVectors(Axes3DElement& axes, const Eigen::MatrixBase<Derived>& vectors) {
    Group group = createGroup();
    const auto& m = vectors.derived();
    const Eigen::Index rows = m.rows();
    const Eigen::Index cols = m.cols();

    if (rows == 3) {
        for (Eigen::Index j = 0; j < cols; ++j) {
            auto v = setVector(axes, m.col(j));
            group.add(v);
        }
    } else if (cols == 3) {
        for (Eigen::Index i = 0; i < rows; ++i) {
            auto v = setVector(axes, m.row(i));
            group.add(v);
        }
    } else if (m.size() == 3) {
        auto v = setVector(axes, m);
        group.add(v);
    }

    return group;
}

template <typename Tensor2D>
inline Group Catalyst::setVectors(Axes3DElement& axes, const Tensor2D& vectors)
    requires requires(const Tensor2D& t) {
        t.shape().size();
        t.shape()[0];
        t.shape()[1];
        static_cast<float>(t(std::size_t{0}, std::size_t{0}));
    } {
    Group group = createGroup();
    const auto& shape = vectors.shape();
    if (shape.size() < 2) return group;

    const std::size_t rows = static_cast<std::size_t>(shape[0]);
    const std::size_t cols = static_cast<std::size_t>(shape[1]);

    if (rows == 3) {
        for (std::size_t j = 0; j < cols; ++j) {
            auto v = setVector(axes, static_cast<float>(vectors(std::size_t{0}, j)),
                               static_cast<float>(vectors(std::size_t{1}, j)),
                               static_cast<float>(vectors(std::size_t{2}, j)));
            group.add(v);
        }
    } else if (cols == 3) {
        for (std::size_t i = 0; i < rows; ++i) {
            auto v = setVector(axes, static_cast<float>(vectors(i, std::size_t{0})),
                               static_cast<float>(vectors(i, std::size_t{1})),
                               static_cast<float>(vectors(i, std::size_t{2})));
            group.add(v);
        }
    }

    return group;
}

// Helper matching Manim's `always_redraw` pattern: create once, then update every frame via an updater.
template <typename CreateFn, typename UpdateFn>
inline auto always_redraw(CreateFn create, UpdateFn update) -> decltype(create()) {
    using Element = decltype(create());
    Element element = create();

    if constexpr (std::is_invocable_v<UpdateFn, Element&, float>) {
        update(element, 0.0f);
        element.add_updater([update = std::move(update)](Element& m, float dt) mutable { update(m, dt); });
    } else if constexpr (std::is_invocable_v<UpdateFn, Element&>) {
        update(element);
        element.add_updater([update = std::move(update)](Element& m) mutable { update(m); });
    } else {
        static_assert(std::is_invocable_v<UpdateFn, Element&> || std::is_invocable_v<UpdateFn, Element&, float>,
                      "always_redraw update must be callable with (Element&) or (Element&, float)");
    }

    return element;
}
