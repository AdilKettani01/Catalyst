● Catalyst GPU Library – Checklist

Updated: 2025-11-27

===============================================================================
STATUS
===============================================================================
- Build: ✅ Release build clean
- Tests: ✅ All C++ tests passing
- GPU: ✅ Vulkan path stable (geometry visible, MSAA on, render-to-file works)

===============================================================================
HOW TO CONSUME THIS LIBRARY
===============================================================================
- Add as a subproject in your CMake build:
  ```cmake
  add_subdirectory(path/to/CPPmath-independent/manim-cpp)
  target_link_libraries(your_app PRIVATE catalyst)
  target_include_directories(your_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/manim-cpp/include)
  ```
- Options (set before add_subdirectory): `MANIM_ENABLE_VULKAN` (ON), `MANIM_BUILD_TESTS` (ON/OFF), `MANIM_BUILD_PYTHON_BINDINGS` (ON/OFF).
- Sample target: `examples/gpu_render_to_file.cpp` builds `gpu_render_to_file` (run with `--gpu`).

===============================================================================
COMPLETED POLISH ITEMS
===============================================================================
- ✅ Backface culling enabled (CW front face for Y-flipped projection)
- ✅ Alpha passed through shaders for opacity support
- ✅ "Render to File" documentation in manim-cpp/README.md

===============================================================================
BUILD & TEST
===============================================================================
cd /home/adil/CPPmath-independent/manim-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMANIM_ENABLE_VULKAN=ON -DMANIM_BUILD_TESTS=ON
cmake --build build --config Release -j$(nproc)
cd build && ctest --output-on-failure

GPU-focused tests:
./bin/manim_tests --gtest_filter="GPUComputeTest.*:GPUMemoryTest.*:GPUErrorHandling.*:MultiGPUTest.*"
