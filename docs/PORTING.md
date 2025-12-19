# Porting Guide (Manim → Catalyst) — for AI Agents

This repository contains many auto-generated Catalyst translation scaffolds under `animations/VID/**`. These are placed next to the original Manim (Python) sources. This guide describes how to turn those stubs into working Catalyst scenes.

If you’re unsure about an API name, prefer:

- `docs/DOCUMENTATION.md` (user-facing overview + examples)
- `catalyst.h` (authoritative API surface)
- `vid_utils.h` (NumPy/SciPy-style helpers used by VID ports)

## Core Differences

### 1) “Create” does not exist

Manim:

- `Create(mobj)`, `ShowCreation(mobj)` → “make it appear”

Catalyst:

- All elements are hidden by default
- Use `mobj.show(duration)` to make it visible
- Use `mobj.hide(duration)` to remove it

If Manim uses `self.add(mobj)` (instant add), use `mobj.show(0.0f)` in Catalyst.

### 2) Timeline is explicit

Manim:

- `self.play(...)` schedules animations in order
- `self.wait(t)`

Catalyst:

- Calling an animation method schedules it at the current timeline cursor
- `window.wait(t)` advances the cursor

Example:

```cpp
auto t = window.setText("Hello");
t.show(0.5f);
window.wait(1.0f);
t.hide(0.3f);
```

### 3) Coordinates are pixels (unless you use Axes)

- Most element positioning uses pixel coordinates (top-left origin).
- For graphing/linear-algebra scenes, prefer `AxesElement` + `GraphElement` + `setVector(...)`.

The VID scaffolds usually define helpers like:

- `kManimUnit`, `X(manimX)`, `Y(manimY)` for quick “Manim-units → pixels” conversion.

## Porting Workflow (Recommended)

1. Open the corresponding Python source in `animations/VID/**.py` and locate the `construct(...)` method.
2. Identify:
   - What is static (axes, labels, shapes)
   - What animates (show/hide, MoveTo/Scale/Rotate, transforms, updaters)
3. Create Catalyst elements first (they’re hidden), style them, then call `.show(...)`.
4. Use `window.wait(...)` between beats.
5. If the Manim code uses `numpy/scipy`, check `vid_utils.h` first.

## Common Mappings (Manim → Catalyst)

### Visibility / fades

| Manim | Catalyst |
|------|---------|
| `Create(m)` / `ShowCreation(m)` | `m.show(duration)` |
| `FadeIn(m)` | `m.show(duration)` |
| `FadeOut(m)` | `m.hide(duration)` |
| `self.add(m)` | `m.show(0.0f)` |
| `self.remove(m)` | `m.hide(0.0f)` (or `window.clear()` for reset) |

Directional fades: Catalyst supports `show(duration, Direction::LEFT/RIGHT/UP/DOWN, ...)` and the same for `hide(...)`.

### Transforms

| Manim | Catalyst |
|------|---------|
| `m.move_to(p)` | `m.move_to(xPx, yPx)` |
| `m.shift(v)` | `m.shift(dx, dy)` or `m.shift(Direction::RIGHT, amount)` |
| `m.scale(s)` | `m.Scale(duration, s)` (animated) or `m.scaleCamera...` for camera |
| `m.rotate(θ)` | `m.Rotate(duration, degrees)` |
| `Transform(a, b)` | `a.Transform(duration, b)` (Text/Math cross-fades) |
| `ReplacementTransform(a, b)` | Usually `a.Transform(duration, b)` |
| Shape morphing | `shape.morphTo(targetShape, duration)` |

Note: For `Transform(a, b)`, create `b` but don’t call `b.show(...)` first — `Transform` handles the cross-fade.

### “Write” / character effects

| Manim | Catalyst |
|------|---------|
| `Write(text)` | `text.Write(duration)` |
| `AddTextLetterByLetter(text)` | `text.AddLetterByLetter(duration)` |
| `DrawBorderThenFill(m)` | `m.DrawBorderThenFill(duration)` (text only) |

### Groups (VGroup)

Use `Group` to group elements and apply layout/styling:

```cpp
auto g = window.createGroup();
g.add(t1).add(t2).add(shape);
g.arrange(Direction::DOWN, 24.0f);
g.show(0.5f);
```

### Axes / graphs / vectors (linear algebra videos)

Prefer data coordinates + conversion:

- `AxesElement axes = window.setAxes(xMin, xMax, yMin, yMax);`
- `GraphElement g = window.setGraph(axes, func, points);`
- **Vectors**: `setVector(...)` draws an arrow from the axes origin to `(vx, vy)` (or `(vx, vy, vz)` in 3D).

Pixel vs data space:

- `setLine(...)` / `setArrow(...)` are pixel-space shapes.
- To draw a segment in axes coordinates, use `setGraph(axes, {x1, x2}, {y1, y2})`.

Supported vector inputs:

- `std::vector<float>{vx, vy}`
- Eigen vectors/matrices (`Eigen::Vector2f`, `Eigen::VectorXf`, `Eigen::MatrixBase`)
- “xtensor-like” containers that provide `size()` + `operator()(i)` or `operator[](i)`

Batch creation:

- `setVectors(axes, matrixOrTensor)` returns a `Group`
  - 2D: `2xN` (columns=vectors) or `Nx2` (rows=vectors)
  - 3D: `3xN` or `Nx3`

### Updaters / always_redraw

Catalyst supports Manim-style updaters and `always_redraw(...)`:

```cpp
auto t = window.setText("x");
t.add_updater([](TextElement& self, float dt) {
    (void)dt;
    self.setText("updated");
});
```

`always_redraw(create, update)` is useful for “recompute every frame” patterns.

For `ValueTracker`-driven ports (counters, sliders, etc.), it’s common to update multiple elements from a single updater. Prefer capturing element *handles* by value (they’re lightweight) and throttle work when possible:

```cpp
auto label = window.setText("0");
ValueTracker v = window.createValueTracker(0.0f);
v.animate_to(10.0f, 2.0f, Easing::Linear);
v.add_updater([label, last = -1](ValueTracker& t, float) mutable {
    const int cur = static_cast<int>(std::lround(t.get_value()));
    if (cur == last) return;
    last = cur;
    label.setText(std::to_string(cur));
});
```

## NumPy/SciPy Replacements (VID Utilities)

Use `vid_utils.h` (included by `#include <Catalyst>`) instead of pulling in Python-style dependencies:

- `vid::linspace`, `vid::arange`, `vid::cumsum`
- `vid::interpolate`, `vid::inverse_interpolate`, `vid::match_interpolate`
- `vid::color_gradient_hex`
- RK4 ODE solver: `vid::solve_ivp_rk4`
- 1D interpolation: `vid::LinearInterpolator`

For linear algebra, prefer Eigen types (`vid::Vec2f`, `vid::Vec3f`, `vid::VecXf`).

## Practical Tips

- Start with a “static” port: create everything and `show(0.0f)` to verify layout.
- Then add animation beats with `show/hide/MoveTo/Scale/Rotate` and `window.wait(...)`.
- Prefer `AxesElement` for any scene that thinks in “mathematical coordinates”.
- Keep colors as hex strings (Manim palette constants do not exist here).
- If a Manim feature doesn’t exist, approximate with a simpler visual (line/graph/text) rather than blocking the whole port.
