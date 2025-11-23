# Manim C++ Validation Guide

This document describes the comprehensive validation infrastructure for the Manim C++ conversion.

## Overview

The validation suite ensures that:
1. ✅ All Python Manim features work correctly in C++
2. ✅ 10-100x performance improvement for 2D animations
3. ✅ 100-1000x performance improvement for 3D rendering
4. ✅ GPU utilization > 80% for complex scenes
5. ✅ Real-time rendering (60 FPS) is achieved
6. ✅ No memory leaks detected
7. ✅ Python compatibility maintained
8. ✅ Visual output quality matched or exceeded

## Quick Start

Run the complete validation suite:

```bash
./validate_all.sh
```

For a quick validation (skips benchmarks and memory checks):

```bash
./validate_all.sh --quick
```

To skip rebuilding:

```bash
./validate_all.sh --skip-build
```

## Validation Components

### 1. System Verification (`tests/validation/system_verification.cpp`)

Verifies that ALL major components are implemented and functional:

**Core Systems:**
- GPU Memory Management (VMA-based pools)
- Compute Engine (NumPy replacement on GPU)
- Math Library (GPU-accelerated operations)

**Rendering Pipeline:**
- Vulkan/OpenGL/DX12 renderers
- Deferred rendering for 3D
- Shadow mapping (cascaded, ray-traced)
- Global illumination
- PBR materials
- Volumetric rendering
- Post-processing pipeline

**Mobject System:**
- Base Mobject with GPU-resident geometry
- VMobject (vectorized)
- 2D geometry (Circle, Square, Polygon, etc.)
- 3D objects (Sphere, Mesh, etc.)
- Text/LaTeX rendering (if available)
- SVG support (if available)

**Animation System:**
- GPU-parallel animation processing
- All animation types from Python Manim
- Hybrid CPU-GPU scheduling
- Transform animations
- Fading animations
- Creation animations

**Scene Management:**
- Base Scene
- 3D scenes with full lighting
- GPU culling (frustum, occlusion)
- LOD system

**Running:**
```bash
./build/system_verification
```

**Output:**
- Console report with ✓/✗ status for each component
- `system_verification_report.md` with detailed results

### 2. GPU 3D Integration Tests (`tests/validation/gpu_3d_integration_tests.cpp`)

Tests GPU-accelerated 3D rendering with complex scenes:

**Test Scenarios:**
- **Complex Scene Rendering**: 10,000 objects with shadows
  - Target: < 33ms per frame (30 FPS minimum)
- **Advanced Lighting**: 50 point lights + shadows + GI
  - Tests cascaded shadow maps
  - Tests ray-traced shadows (if RTX available)
  - Tests global illumination
- **Volumetric Rendering**: Fog and volumetric lighting
- **Post-Processing**: Bloom, TAA, SSAO, tone mapping
- **Hybrid Rendering**: Automatic CPU/GPU work distribution
- **GPU Culling**: Frustum and occlusion culling tests
- **LOD System**: Level-of-detail switching
- **Stress Tests**:
  - 1 million polygons
  - 100 lights simultaneously

**Running:**
```bash
./build/gpu_3d_integration_tests
```

### 3. Performance Benchmarks (`tests/validation/performance_benchmarks.cpp`)

Compares C++ performance against Python Manim:

**2D Animation Benchmarks:**
- SquareToCircle animation
  - Expected: 10-100x faster than Python
- Multiple animations (100 objects)
  - Expected: 20x faster

**3D Scene Benchmarks:**
- Simple sphere rendering
  - Expected: 50-100x faster (GPU vs CPU)
- Complex scene (1000 objects + shadows)
  - Expected: 100-1000x faster

**Particle System Benchmarks:**
- 100K particles
  - Target: 60 FPS (< 16.67ms per frame)
- 1M particles
  - Target: 60 FPS (real-time)

**Bezier Tessellation:**
- CPU implementation
- GPU batch implementation (1000 curves)
  - Expected: 50-100x faster

**Transform Operations:**
- 100K point transforms
  - Target: < 1ms

**Ray Tracing** (if available):
- Simple scene (10 objects)
  - Target: 30 FPS (< 33ms)

**Running:**
```bash
./build/manim_benchmarks
# or for validation version:
./build/performance_benchmarks
```

**Output:**
- Console output with timing results
- `performance_report.html` with comparison tables
- `benchmarks.json` with raw data

### 4. Python Compatibility Tests (`tests/validation/test_python_compatibility.py`)

Tests that Python Manim scripts work unchanged:

**Test Categories:**
- **Backward Compatibility**: Classic Manim code works
  - Scene creation
  - Basic shapes (Circle, Square)
  - SquareToCircle animation
  - Fade animations
  - Multiple mobjects
- **3D Features**: ThreeDScene functionality
  - 3D sphere creation
  - Camera orientation
  - 3D surfaces
  - Enhanced GPU3DScene with lighting
- **GPU Acceleration**: Automatic GPU usage
  - GPU detection
  - Automatic acceleration for large scenes
  - Performance verification
- **New Features** (C++ enhancements):
  - PBR materials
  - Real-time rendering mode (60 FPS target)
  - GPU particle systems
  - Ray tracing (if available)

**Running:**
```bash
export PYTHONPATH=build/python:$PYTHONPATH
python3 manim-cpp/tests/validation/test_python_compatibility.py
```

### 5. Visual Regression Tests (if implemented)

Would compare rendered output with reference images:
- SSIM (Structural Similarity) > 0.95 for 2D
- SSIM > 0.85 for 3D (may have enhancements)

### 6. Memory Leak Detection

Uses Valgrind to detect memory leaks:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/manim_tests
```

Target: 0 bytes definitely lost

### 7. GPU Validation

Tests GPU-specific features:
- GPU detection and initialization
- Memory allocation and deallocation
- Compute shader execution
- CPU fallback when GPU unavailable
- Multi-GPU support (if available)

## Performance Targets

The validation ensures these performance targets are met:

| Operation | Target | Metric |
|-----------|--------|--------|
| Transform 100K points | < 1ms | Time |
| Render 10K 3D objects | < 16ms | 60 FPS |
| 1M particles | Real-time | 60 FPS |
| Complex 3D scene | 100-1000x | Speedup vs Python |
| 2D animations | 10-100x | Speedup vs Python |
| Memory usage | 5x reduction | vs Python |
| Startup time | < 100ms | Cold start |
| First frame render | < 500ms | Initialization |

## Validation Report

After running `./validate_all.sh`, check:

1. **Console output**: Real-time test results
2. **validation_results/validation_report.md**: Comprehensive summary
3. **validation_results/system_verification_report.md**: Component status
4. **validation_results/performance_report.html**: Performance comparison
5. **validation_results/*.log**: Detailed logs for each test suite

### Report Interpretation

**✓ PASS**: Component/test working correctly
**✗ FAIL**: Component/test failed
**⚠ SKIP**: Test skipped (usually due to missing GPU/dependencies)

## Troubleshooting

### Build Failures

If CMake configuration fails:
```bash
# Check dependencies
cmake --find-package Vulkan -DMODE=EXIST
cmake --find-package glfw3 -DMODE=EXIST
cmake --find-package GTest -DMODE=EXIST
```

### Test Failures

**GPU tests failing:**
- Check if GPU drivers are installed: `nvidia-smi` or `rocm-smi`
- Verify Vulkan: `vulkaninfo`
- Tests should gracefully skip if no GPU available

**Memory tests failing:**
- Install Valgrind: `sudo apt-get install valgrind`
- Or skip with `--quick` mode

**Python tests failing:**
- Ensure bindings built: `-DMANIM_BUILD_PYTHON_BINDINGS=ON`
- Check Python path: `export PYTHONPATH=build/python:$PYTHONPATH`

### Performance Targets Not Met

If performance targets aren't met:

1. **Check GPU utilization:**
   ```bash
   nvidia-smi dmon  # Monitor during test run
   ```

2. **Verify GPU is being used:**
   ```bash
   ./build/system_verification | grep GPU
   ```

3. **Check CPU governor:**
   ```bash
   cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   # Should be "performance" for benchmarks
   ```

4. **Build in Release mode:**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

5. **Enable optimizations:**
   - AVX2/NEON SIMD
   - Link-time optimization (LTO)
   - GPU-specific tuning

## Continuous Integration

For CI/CD pipelines:

```yaml
# Example .github/workflows/validate.yml
name: Validate
on: [push, pull_request]
jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build vulkan-sdk
      - name: Run validation
        run: |
          chmod +x validate_all.sh
          ./validate_all.sh --quick
      - name: Upload results
        uses: actions/upload-artifact@v3
        with:
          name: validation-results
          path: validation_results/
```

## Adding New Tests

### 1. System Verification Test

Edit `tests/validation/system_verification.cpp`:

```cpp
TEST(SystemVerification, NewFeature) {
    ComponentVerification result;
    result.name = "New Feature Name";
    result.exists = true;

    try {
        // Test feature initialization
        auto feature = std::make_shared<NewFeature>();
        result.initializes = true;

        // Test functionality
        feature->do_something();
        result.functional = true;
    } catch (const std::exception& e) {
        result.error_msg = e.what();
    }

    g_verification_report.add_result(result);
    EXPECT_TRUE(result.is_valid()) << result.error_msg;
}
```

### 2. Performance Benchmark

Edit `tests/validation/performance_benchmarks.cpp`:

```cpp
static void BM_NewFeature(benchmark::State& state) {
    // Setup
    auto feature = std::make_shared<NewFeature>();

    for (auto _ : state) {
        // Benchmark code
        feature->operation();
    }

    // Record results
    BenchmarkResult result;
    result.name = "New Feature Benchmark";
    result.cpp_time_ms = /* calculate from state */;
    result.python_time_ms = /* reference time */;
    result.speedup = result.python_time_ms / result.cpp_time_ms;
    result.achieved_target = result.speedup >= 10.0;
    g_benchmark_results.push_back(result);
}
BENCHMARK(BM_NewFeature)->Unit(benchmark::kMillisecond);
```

### 3. Python Compatibility Test

Edit `tests/validation/test_python_compatibility.py`:

```python
class TestNewFeature(unittest.TestCase):
    def setUp(self):
        if not HAS_CPP_BINDINGS:
            self.skipTest("C++ bindings not available")

    def test_new_feature(self):
        """Test new feature works from Python."""
        feature = manim.NewFeature()
        result = feature.do_something()
        self.assertIsNotNone(result)
```

## Validation Schedule

Recommended validation schedule:

- **Every commit**: Quick validation (`--quick`)
- **Before PR merge**: Full validation
- **Weekly**: Full validation + memory leak detection
- **Before release**: Complete validation + stress tests

## Expected Results

A successful validation should show:

```
==========================================
VALIDATION COMPLETE
==========================================

✓ Build successful
✓ Unit tests passed (XXX tests)
✓ System verification passed (XX/XX components)
✓ GPU/3D integration tests passed
✓ Performance benchmarks completed
  - Average speedup: XXXx over Python
✓ Python compatibility tests passed
✓ No memory leaks detected

Results saved to: validation_results/
==========================================
```

## Support

For issues or questions:
- Check logs in `validation_results/`
- Review individual test output
- Verify GPU/driver installation
- Consult TROUBLESHOOTING.md

## References

- [ARCHITECTURE_DESIGN.md](ARCHITECTURE_DESIGN.md) - System architecture
- [RENDERING_PIPELINE.md](RENDERING_PIPELINE.md) - Rendering details
- [PYTHON_TO_CPP_PATTERNS.md](PYTHON_TO_CPP_PATTERNS.md) - Conversion patterns
- [CPP_FILE_MAPPING.md](CPP_FILE_MAPPING.md) - File structure
