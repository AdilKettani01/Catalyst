# Catalyst Feature Roadmap

Comparison with Manim features. ✅ = Implemented, ⬜ = Not yet implemented

---

## Text & Typography

| Feature                  | Manim                    | Catalyst                         |
|--------------------------|--------------------------|----------------------------------|
| Basic text rendering     | ✅ Text()                 | ✅ setText()                      |
| Font size control        | ✅ font_size              | ✅ setSize()                      |
| Text positioning         | ✅ move_to()              | ✅ setPosition()                  |
| Anchor positions         | ✅ to_corner(), to_edge() | ✅ Position::TLEFT, etc.          |
| Text color               | ✅ color                  | ✅ setColor()                     |
| Multiple text objects    | ✅ Yes                    | ✅ Multiple TextElements          |
| LaTeX/Math rendering     | ✅ MathTex, Tex           | ✅ setMath()                      |
| Text stroke/outline      | ✅ stroke_width           | ✅ setStroke()                    |
| Text gradient            | ✅ gradient               | ✅ setGradient()                  |
| Paragraph/multiline      | ✅ Paragraph              | ✅ setLineHeight(), setMaxWidth() |

---

## Animations - Fade

| Feature                | Manim                 | Catalyst                   |
|------------------------|-----------------------|----------------------------|
| Fade in                | ✅ FadeIn()            | ✅ FadeIn()                 |
| Fade out               | ✅ FadeOut()           | ✅ FadeOut()                |
| Fade in from direction | ✅ FadeIn(shift=UP)    | ✅ FadeIn(Direction::UP)    |
| Fade out to direction  | ✅ FadeOut(shift=DOWN) | ✅ FadeOut(Direction::DOWN) |


---

## Animations - Transform

| Feature                    | Manim                    | Catalyst      |
|----------------------------|--------------------------|---------------|
| Move to position           | ✅ shift(), move_to()     | ✅ MoveTo()    |
| Scale                      | ✅ scale()                | ✅ Scale()     |
| Rotate                     | ✅ rotate()               | ✅ Rotate()    |
| Transform to another       | ✅ Transform()            | ✅ Transform() |
| Morph/ReplacementTransform | ✅ ReplacementTransform() | ✅ .morphTo    |

---

## Animations - Showing

| Feature                 | Manim                     | Catalyst               |
|-------------------------|---------------------------|------------------------|
| Write text (typewriter) | ✅ Write()                 | ✅ Write()              |
| Add letter by letter    | ✅ AddTextLetterByLetter() | ✅ AddLetterByLetter()  |
| Draw border then fill   | ✅ DrawBorderThenFill()    | ✅ DrawBorderThenFill() |

---

## Animations - Emphasis

| Feature        | Manim            | Catalyst         |
|----------------|------------------|------------------|
| Indicate/flash | ✅ Indicate()     | ✅ Indicate()     |
| Flash          | ✅ Flash()        | ✅ Flash()        |
| Circumscribe   | ✅ Circumscribe() | ✅ Circumscribe() |
| Wiggle         | ✅ Wiggle()       | ✅ Wiggle()       |
| Pulse          | ✅ Pulse()        | ✅ Pulse()        |
| Focus on       | ✅ FocusOn()      | ✅ FocusOn()      |

---

## Animations - Movement

| Feature         | Manim             | Catalyst                  |
|-----------------|-------------------|---------------------------|
| Move along path | ✅ MoveAlongPath() | ✅ MoveAlongPath()         |
| Spiral in/out   | ✅ SpiralIn()      | ✅ SpiralIn(), SpiralOut() |
| Homotopy        | ✅ Homotopy()      | ✅ Homotopy()              |
| PhaseFlow       | ✅ PhaseFlow()     | ✅ PhaseFlow()             |

---

## Shapes - Basic

| Feature           | Manim                | Catalyst                                  |
|-------------------|----------------------|-------------------------------------------|
| Circle            | ✅ Circle()           | ✅ setCircle()                             |
| Square            | ✅ Square()           | ✅ setRectangle() (square is width=height) |
| Rectangle         | ✅ Rectangle()        | ✅ setRectangle()                          |
| Triangle          | ✅ Triangle()         | ✅ setTriangle()                           |
| Polygon           | ✅ Polygon()          | ✅ setPolygon()                            |
| Ellipse           | ✅ Ellipse()          | ✅ setEllipse()                            |
| Arc               | ✅ Arc()              | ✅ setArc()                                |
| Dot/Point         | ✅ Dot()              | ✅ setDot()                                |
| Line              | ✅ Line()             | ✅ setLine()                               |
| Arrow             | ✅ Arrow()            | ✅ setArrow()                              |
| Double arrow      | ✅ DoubleArrow()      | ✅ setDoubleArrow()                        |
| Brace             | ✅ Brace()            | ✅ setBrace()                              |
| Star              | ✅ Star()             | ✅ setStar()                               |
| Regular polygon   | ✅ RegularPolygon()   | ✅ setRegularPolygon()                     |
| Rounded rectangle | ✅ RoundedRectangle() | ✅ setRoundedRectangle()                   |

---

## Shapes - Advanced

| Feature       | Manim           | Catalyst                    |
|---------------|-----------------|-----------------------------|
| Bezier curves | ✅ CubicBezier() | ✅ setCubicBezier()          |
| Custom path   | ✅ VMobject      | ✅ beginPath() builder       |
| SVG import    | ✅ SVGMobject    | ⬜                           |
| Image         | ✅ ImageMobject  | ✅ setImage()                |
| Vector field  | ✅ VectorField   | ✅ setVectorField()          |
| Number line   | ✅ NumberLine    | ✅ setNumberLine()           |
| Axes          | ✅ Axes          | ✅ setAxes()                 |
| Graph         | ✅ Graph         | ✅ setGraph()                |
| Bar chart     | ✅ BarChart      | ✅ setBarChart()             |
| Pie chart     | ✅ PieChart      | ✅ setPieChart()             |
| Table         | ✅ Table         | ✅ setTable()                |

---

## Scene & Camera

| Feature               | Manim                    | Catalyst                |
|-----------------------|--------------------------|-------------------------|
| Window/scene creation | ✅ Scene                  | ✅ Catalyst()            |
| Set background color  | ✅ background_color       | ✅ setBackground()       |
| Camera zoom           | ✅ camera.frame.scale()   | ✅ setCameraZoom()       |
| Camera pan            | ✅ camera.frame.move_to() | ✅ setCameraPan()        |
| Camera rotate         | ✅ camera.frame.rotate()  | ✅ setCameraRotate()     |
| Camera reset          | ✅ restore()              | ✅ resetCamera()         |
| Multiple scenes       | ✅ Yes                    | ✅ createScene(), Scene  |
| Scene transitions     | ✅ Yes                    | ⬜                       |

---

## 3D Features

| Feature              | Manim          | Catalyst                                  |
|----------------------|----------------|-------------------------------------------|
| 3D scene             | ✅ ThreeDScene  | ✅ set3DMode()                             |
| 3D axes              | ✅ ThreeDAxes   | ✅ setAxes3D()                             |
| Sphere               | ✅ Sphere()     | ✅ setSphere()                             |
| Cube                 | ✅ Cube()       | ✅ setCube3D()                             |
| Cylinder             | ✅ Cylinder()   | ✅ setCylinder()                           |
| Cone                 | ✅ Cone()       | ✅ setCone()                               |
| 3D arrow             | ✅ Arrow3D()    | ✅ setArrow3D()                            |
| Surface              | ✅ Surface()    | ✅ setSurface()                            |
| 3D camera control    | ✅ Yes          | ✅ setCamera3D(), orbitCamera()            |
| 3D camera animation  | ✅ Yes          | ✅ Animated setCamera3D(), setCameraFOV()  |
| 3D shape transforms  | ✅ Yes          | ✅ MoveTo(), ScaleTo(), RotateTo()         |
| Lighting             | ✅ Yes          | ✅ Ambient, directional, point lights      |
| 2D overlay on 3D     | ✅ Yes          | ✅ Mixed-mode rendering                    |

---

## Timing & Control

| Feature               | Manim            | Catalyst                                |
|-----------------------|------------------|-----------------------------------------|
| Animation duration    | ✅ run_time       | ✅ duration parameter                    |
| Animation delay       | ✅ Wait()         | ✅ delay()                               |
| Animation easing      | ✅ rate_func      | ✅ Easing enum + setEasing()             |
| Sequential animations | ✅ play() order   | ✅ createSequence(), then()              |
| Parallel animations   | ✅ AnimationGroup | ✅ createAnimationGroup()                |
| Animation chaining    | ✅ Succession     | ✅ then()                                |
| Loop animation        | ✅ loop=True      | ✅ repeat(), repeatForever(), pingPong() |

---

## Grouping & Hierarchy
    
| Feature           | Manim               | Catalyst                    |
|-------------------|---------------------|-----------------------------|
| Group objects     | ✅ VGroup            | ✅ createGroup(), Group      |
| Arrange in row    | ✅ arrange(RIGHT)    | ✅ arrange(Direction::RIGHT) |
| Arrange in column | ✅ arrange(DOWN)     | ✅ arrange(Direction::DOWN)  |
| Arrange in grid   | ✅ arrange_in_grid() | ✅ arrangeInGrid()           |
| Z-ordering/layers | ✅ z_index           | ✅ setZIndex()               |

---

## Export & Output

| Feature | Manim | Catalyst |
|---------|-------|----------|
| Real-time window | ✅ preview | ✅ run() |
| Export to video | ✅ -qh flag | ⬜ |
| Export to GIF | ✅ --format gif | ⬜ |
| Export frames | ✅ -s flag | ⬜ |
| Custom resolution | ✅ -r flag | ✅ Catalyst(w, h) |
| Custom FPS | ✅ --fps | ⬜ |

---

## Priority Implementation Order

### Phase 1 - Core (Current)
- [x] Window creation
- [x] Text rendering
- [x] Font sizing
- [x] Positioning (pixel + anchors)
- [x] FadeIn/FadeOut

### Phase 2 - Essential
- [x] Text color (setColor)
- [x] Multiple text elements
- [x] Background color (setBackground)
- [x] Basic easing functions
- [x] Animation delay

### Phase 3 - Shapes
- [x] Circle
- [x] Rectangle
- [x] Line
- [x] Triangle
- [x] Arrow
- [x] Polygon
- [x] Ellipse
- [x] Arc
- [x] Dot
- [x] Double Arrow
- [x] Brace
- [x] Star
- [x] Regular Polygon
- [x] Rounded Rectangle

### Phase 4 - More Animations
- [x] Scale
- [x] Rotate
- [x] MoveTo
- [x] Write (typewriter)
- [x] Transform

### Phase 5 - Advanced
- [x] Groups (AnimationGroup)
- [x] Element groups (Group class with arrange, arrangeInGrid, z-ordering)
- [x] Animation sequencing (then(), createSequence())
- [x] Camera controls (zoom, pan, rotate, reset)
- [x] Multiple scenes (Scene class)
- [ ] Scene transitions
- [ ] SVG import
- [ ] Video export

### Phase 6 - 3D
- [x] 3D scene (set3DMode())
- [x] Basic 3D shapes (Sphere, Cube, Cylinder, Cone, Arrow3D, Surface)
- [x] 3D camera (setCamera3D, orbitCamera, setCameraFOV)
- [x] Lighting (ambient, directional, point lights)
- [x] 3D shape transforms (MoveTo, ScaleTo, RotateTo)
- [x] 2D overlay on 3D (mixed-mode rendering)

---

## Notes

- Manim is Python-based, Catalyst is C++/Vulkan
- Focus on real-time rendering (Manim is primarily for video export)
- GPU-first architecture gives performance advantages
- Simpler API than Manim for basic use cases
