# GPU Export Guide (Zero-Copy Vulkan -> CUDA/NVENC)

This guide explains how to install Catalyst system-wide and use the GPU zero-copy export path from any external project.

## What "GPU Fully" Means
- Rendering still happens in Vulkan.
- Export frames stay on the GPU and are encoded with NVENC via FFmpeg libs.
- No GPU->CPU readback and no raw-frame pipe to CLI ffmpeg.

Zero-copy export is enabled at runtime with:
- `CATALYST_EXPORT_ZEROCOPY=1`
- `CATALYST_VIDEO_CODEC=h264_nvenc` (or `auto`, but nvenc must be available)

## System-Wide Install
Build and install Catalyst with FFmpeg + CUDA driver API:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/catalyst \
  -DCATALYST_ENABLE_FFMPEG=ON \
  -DCATALYST_ENABLE_CUDA=ON
cmake --build build -j
sudo cmake --install build
```

Notes:
- Pick any prefix you want (e.g. `/usr/local` or `/opt/catalyst`).
- The install includes headers, the shared library, and runtime resources (shaders/fonts).
- You should not need `CATALYST_RESOURCE_PATH` when running against an installed prefix, but you can set it if you want to override.

## Using Catalyst in an External Project (CMake)
Add Catalyst as a dependency:

```cmake
find_package(Catalyst REQUIRED)

add_executable(anim_my_scene main.cpp)
target_link_libraries(anim_my_scene PRIVATE Catalyst::Catalyst)
```

Configure your project with the install prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/catalyst
cmake --build build -j
```

In code, include the umbrella header:

```cpp
#include <Catalyst>
```

## External Main Entry (export-aware)
To make your binary understand `export` and `--export`, use `parseOutputArgs()`:

```cpp
int main(int argc, char** argv) {
    const auto opts = Catalyst::parseOutputArgs(argc, argv, 1280, 720, 0.0f);
    Catalyst window(opts.width, opts.height);

    // build scene...

    if (opts.exportVideo) {
        window.exportVideo(opts.outputPath, opts.fps, opts.previewWhileExporting);
    } else if (opts.fps > 0.0f) {
        window.run(opts.fps);
    } else {
        window.run();
    }
    return 0;
}
```

## Run Zero-Copy GPU Export
Export a video using GPU zero-copy:

```bash
CATALYST_EXPORT_ZEROCOPY=1 \
CATALYST_VIDEO_CODEC=h264_nvenc \
./build/anim_my_scene export out.mp4 --fps 60
```

Optional tuning:
- `CATALYST_EXPORT_QUEUE=16` increases the capture/encode buffer pool depth.
- `--preview` shows a realtime preview (slower).

## Verify Zero-Copy Is Active
Look for these signs:
- Log line mentions `h264_nvenc zero-copy`.
- No errors about `external_memory_fd` or `timeline semaphores`.
- CPU usage drops vs the raw-pipe export path.

If zero-copy is not supported, Catalyst falls back to the existing CPU path and prints why.

## Common Requirements
- NVIDIA GPU + driver.
- Vulkan support with `VK_KHR_external_memory_fd` and timeline semaphores.
- FFmpeg built with NVENC (`h264_nvenc` available).
- CUDA Toolkit (driver API) available at build and run time.

