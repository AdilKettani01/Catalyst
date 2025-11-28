import manim_cpp as m

def main():
    scene = m.GPU3DScene()
    circle = m.Circle(radius=1.5)
    circle.set_color(m.PINK)
    scene.play(m.Create(circle), run_time=1.0)
    scene.play(m.FadeOut(circle), run_time=0.5)
    scene.render_frame()
    print("Rendered 1 frame (GPU if available, CPU fallback otherwise).")

if __name__ == "__main__":
    main()
