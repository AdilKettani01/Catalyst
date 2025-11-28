# Quick Start: Basic Scene (Python)

This walkthrough shows how to run a minimal scene using the Python bindings on a Vulkan-capable GPU. CPU fallback works if no GPU is available.

## Prerequisites
1) Build the project (C++ + shaders):
```bash
cd /home/adil/CPPmath-independent/manim-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMANIM_ENABLE_VULKAN=ON
cmake --build build --config Release -j$(nproc)
```
2) Ensure Vulkan uses your GPU (optional if already detected):
```bash
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json  # adjust path for your system/GPU
```
3) Python env (if not already set):
```bash
cd /home/adil/CPPmath-independent
python3 -m venv .venv
. .venv/bin/activate
pip install numpy
```

## Example Script
Create `example_scene.py` (any location is fine):
```python
import manim_cpp as m


def main():
    # Use GPU3DScene to engage the GPU path; falls back to CPU if no GPU.
    scene = m.GPU3DScene()

    # Simple shapes
    square = m.Square(side_length=2.0)
    square.set_color(m.BLUE)
    circle = m.Circle(radius=1.5)
    circle.set_color(m.PINK)

    # Play a few animations
    scene.play(m.Create(square), run_time=1.0)
    scene.play(m.Transform(square, circle), run_time=1.0)
    scene.play(m.FadeOut(square), run_time=0.5)

    # Render a frame (in-memory); no file output needed for a quick check
    scene.render_frame()
    print("Rendered 1 frame (GPU if available, CPU fallback otherwise).")


if __name__ == "__main__":
    main()
```

## Run
```bash
cd /home/adil/CPPmath-independent
PYTHONPATH=manim-cpp/build/python .venv/bin/python example_scene.py
```

Expected output: log lines showing the renderer initialization (GPU path if available) and a final message:
```
Rendered 1 frame (GPU if available, CPU fallback otherwise).
```

## Notes
- If you want a video file, extend the script to write frames via your preferred encoder (not included here).
- If you see llvmpipe in logs, set `VK_ICD_FILENAMES` to your GPU ICD to force discrete GPU usage.
- The same pattern works for 2D `Scene`; switch to `m.Scene()` if you only need CPU/basic rendering.
