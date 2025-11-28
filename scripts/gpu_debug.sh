#!/usr/bin/env bash
#
# GPU debug helper: rebuild with NVIDIA ICD, gather Vulkan info, run GPU checks and example scene.
#
# Usage: bash scripts/gpu_debug.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ICD="/usr/share/vulkan/icd.d/nvidia_icd.json"
BUILD_DIR="$ROOT/manim-cpp/build"
PYTHON="$ROOT/.venv/bin/python"
PYTHONPATH="$ROOT/manim-cpp/build/python"

echo "=== GPU DEBUG SCRIPT ==="
echo "Root: $ROOT"
echo "ICD:  $ICD"
echo "Build: $BUILD_DIR"

if [[ ! -f "$ICD" ]]; then
  echo "ERROR: NVIDIA ICD not found at $ICD"
  exit 1
fi

export VK_ICD_FILENAMES="$ICD"
echo "VK_ICD_FILENAMES=$VK_ICD_FILENAMES"

echo
echo "=== Vulkan Info (head) ==="
vulkaninfo | head -30 || true

echo
echo "=== Rebuild (Release) ==="
cmake -S "$ROOT/manim-cpp" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DMANIM_ENABLE_VULKAN=ON -DMANIM_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --config Release -j"$(nproc)"

echo
echo "=== GPU Info via bindings ==="
if [[ ! -x "$PYTHON" ]]; then
  echo "ERROR: Python venv not found at $PYTHON"
  exit 1
fi

cat > "$ROOT/example_scene.py" <<'PY'
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
PY

cat > "$ROOT/gpu_check.py" <<'PY'
import manim_cpp as m
print("GPU available:", m.check_gpu_available())
try:
    info = m.get_gpu_info()
    print("GPU info count:", len(info))
    for gpu in info:
        print(" -", gpu.name, "| mem MB:", getattr(gpu, "total_memory_mb", "n/a"))
except Exception as e:
    print("Failed to get GPU info:", e)
PY

echo
echo "=== Python GPU check ==="
PYTHONPATH="$PYTHONPATH" "$PYTHON" "$ROOT/gpu_check.py" || true

echo
echo "=== Run example scene ==="
PYTHONPATH="$PYTHONPATH" "$PYTHON" "$ROOT/example_scene.py" || true

echo
echo "=== GPU-focused tests (short) ==="
(cd "$BUILD_DIR" && ./bin/manim_tests --gtest_filter="GPUComputeTest.*:GPUMemoryTest.*:GPUErrorHandling.*:MultiGPUTest.*") || true

echo
echo "=== Done ==="
