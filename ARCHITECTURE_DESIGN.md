# Manim C++ GPU-First Architecture Design

## Executive Summary

This document outlines a **GPU-first architecture** for Manim C++ that prioritizes GPU computation, modern graphics APIs (Vulkan primary, OpenGL/DirectX12/Metal fallback), and high-performance parallel processing. The design eliminates the dual Cairo/OpenGL renderer architecture in favor of a unified GPU pipeline.

**Key Design Principles**:
1. **GPU-First**: All rendering and heavy computation on GPU
2. **Zero-Copy Where Possible**: Minimize CPU-GPU data transfer
3. **Parallel Everything**: Multi-threaded CPU, parallel GPU compute
4. **Modern APIs**: Vulkan (primary), DirectX 12, Metal support
5. **Extensible**: Plugin architecture, custom shaders, hot-reload

---

## 1. High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                         │
│  ┌──────────────┬────────────────┬───────────────┬────────────┐ │
│  │  Scene API   │  Animation API │  Mobject API  │  Camera API│ │
│  └──────────────┴────────────────┴───────────────┴────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                          ECS LAYER                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Entity Component System (EnTT)                          │   │
│  │  - Transform Component    - Render Component             │   │
│  │  - Animation Component    - GPU Buffer Component         │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       RENDERING LAYER                            │
│  ┌────────────┬──────────────┬────────────────┬──────────────┐  │
│  │   Vulkan   │  DirectX 12  │     Metal      │  OpenGL 4.6  │  │
│  │ (Primary)  │  (Windows)   │    (macOS)     │  (Fallback)  │  │
│  └────────────┴──────────────┴────────────────┴──────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         GPU LAYER                                │
│  ┌──────────────┬────────────────┬────────────────┬──────────┐  │
│  │  Graphics    │    Compute     │  Ray Tracing   │ Transfer │  │
│  │  Pipeline    │    Pipeline    │    Pipeline    │  Queue   │  │
│  └──────────────┴────────────────┴────────────────┴──────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         HARDWARE                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  GPU (NVIDIA RTX / AMD RDNA / Apple Silicon / Intel Xe)  │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. GPU Memory Management Strategy

### 2.1 Memory Architecture

```
CPU Memory (Host)                GPU Memory (Device)
┌──────────────────┐            ┌──────────────────────┐
│   Scene Graph    │            │   Vertex Buffers     │
│   (Metadata)     │◄──────────►│   (Positions, UVs)   │
│   - Transform    │   Upload   │                      │
│   - Hierarchy    │   ◄────    ├──────────────────────┤
│   - Animations   │   Download │   Index Buffers      │
├──────────────────┤   ────►    │   (Triangulation)    │
│  Staging Buffer  │            ├──────────────────────┤
│  (Temp Upload)   │            │   Uniform Buffers    │
└──────────────────┘            │   (Transforms, etc.) │
                                ├──────────────────────┤
                                │   Texture Atlas      │
                                │   (Glyphs, Images)   │
                                ├──────────────────────┤
                                │   Render Targets     │
                                │   (FBOs, G-Buffer)   │
                                └──────────────────────┘
```

### 2.2 Memory Allocation Strategy

**Use Vulkan Memory Allocator (VMA)** for intelligent allocation:

```cpp
class GPUMemoryManager {
    VmaAllocator allocator;

    // Memory pools
    struct MemoryPools {
        VmaPool vertex_pool;      // Vertex/index data
        VmaPool uniform_pool;     // Transform matrices, uniforms
        VmaPool texture_pool;     // Textures, render targets
        VmaPool staging_pool;     // CPU->GPU transfer
    };

public:
    // Smart allocation based on usage
    GPUBuffer allocate_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage
    ) {
        VkBufferCreateInfo buffer_info{};
        buffer_info.size = size;
        buffer_info.usage = usage;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = memory_usage;

        VkBuffer buffer;
        VmaAllocation allocation;

        vmaCreateBuffer(allocator, &buffer_info, &alloc_info,
                       &buffer, &allocation, nullptr);

        return GPUBuffer{buffer, allocation};
    }

    // Persistent mapped buffers for frequently updated data
    MappedBuffer create_mapped_buffer(VkDeviceSize size) {
        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;

        vmaCreateBuffer(allocator, &buffer_info, &alloc_info,
                       &buffer, &allocation, &allocation_info);

        return MappedBuffer{
            buffer,
            allocation,
            allocation_info.pMappedData  // Persistent mapping
        };
    }
};
```

### 2.3 Memory Types

| Memory Type | Usage | Example | Transfer Frequency |
|-------------|-------|---------|-------------------|
| **Device Local** | GPU-only data | Static meshes, textures | Rare (load time) |
| **Device Local + Host Visible** | Frequently updated | Transform matrices, animation data | Every frame |
| **Host Cached** | GPU → CPU readback | Frame capture, debugging | Infrequent |
| **Staging** | Temporary upload | Large data transfers | Per upload |

### 2.4 Buffer Management

```cpp
class BufferManager {
    struct BufferAllocation {
        VkBuffer buffer;
        VmaAllocation allocation;
        VkDeviceSize size;
        VkDeviceSize offset;
    };

    // Ring buffer for per-frame data (triple buffering)
    struct RingBuffer {
        static constexpr int NUM_FRAMES = 3;

        VkBuffer buffers[NUM_FRAMES];
        VmaAllocation allocations[NUM_FRAMES];
        void* mapped_data[NUM_FRAMES];
        VkDeviceSize offset[NUM_FRAMES] = {0};
        int current_frame = 0;

        // Allocate from current frame's buffer
        BufferAllocation allocate(VkDeviceSize size, VkDeviceSize alignment) {
            int frame = current_frame;
            VkDeviceSize aligned_offset = align_up(offset[frame], alignment);

            BufferAllocation alloc{
                buffers[frame],
                allocations[frame],
                size,
                aligned_offset
            };

            offset[frame] = aligned_offset + size;
            return alloc;
        }

        void next_frame() {
            current_frame = (current_frame + 1) % NUM_FRAMES;
            offset[current_frame] = 0;  // Reset offset
        }
    };

    RingBuffer uniform_ring_buffer;  // For per-frame uniforms

public:
    // Allocate per-frame uniform data
    BufferAllocation allocate_uniform(VkDeviceSize size) {
        return uniform_ring_buffer.allocate(size, 256);  // 256-byte alignment
    }

    void begin_frame() {
        uniform_ring_buffer.next_frame();
    }
};
```

---

## 3. CPU-GPU Work Distribution Algorithm

### 3.1 Work Classification

```cpp
enum class WorkType {
    CPU_Only,       // Scene graph traversal, animation logic
    GPU_Preferred,  // Transforms, tessellation, large-scale computation
    GPU_Only,       // Rendering, ray tracing, post-processing
    Hybrid          // Can be done on either, choose dynamically
};

struct Work {
    WorkType type;
    std::function<void()> cpu_implementation;
    std::function<void(VkCommandBuffer)> gpu_implementation;
    size_t data_size;  // For dynamic scheduling
};
```

### 3.2 Dynamic Load Balancing

```cpp
class WorkScheduler {
    struct GPUCapabilities {
        bool supports_ray_tracing;
        bool supports_mesh_shaders;
        uint32_t max_compute_workgroups;
        size_t gpu_memory_available;
    };

    GPUCapabilities gpu_caps;
    std::atomic<size_t> gpu_load{0};  // Estimate of GPU workload

public:
    // Decide CPU vs GPU execution dynamically
    bool should_use_gpu(const Work& work) const {
        switch (work.type) {
            case WorkType::CPU_Only:
                return false;

            case WorkType::GPU_Only:
                return true;

            case WorkType::GPU_Preferred:
                // Use GPU unless it's overloaded
                return gpu_load.load() < 0.9 * gpu_caps.max_compute_workgroups;

            case WorkType::Hybrid:
                // Choose based on data size and current load
                if (work.data_size < 1024) {
                    return false;  // Too small, CPU is faster
                }
                if (work.data_size > 100'000 && gpu_load.load() < 0.8) {
                    return true;   // Large data, GPU wins
                }
                return false;
        }
    }

    void execute(Work& work, VkCommandBuffer cmd) {
        if (should_use_gpu(work)) {
            work.gpu_implementation(cmd);
            gpu_load.fetch_add(1);
        } else {
            work.cpu_implementation();
        }
    }
};
```

### 3.3 CPU Task Assignment

```cpp
namespace CPUTasks {
    // Scene graph management
    void update_scene_graph(Scene& scene, float dt);

    // Animation sequencing
    void update_animations(std::span<Animation*> animations, float dt);

    // File I/O
    void export_video_frame(const FrameBuffer& fb, const std::string& path);

    // Configuration parsing
    void load_config(const std::filesystem::path& config_path);

    // Plugin management
    void load_plugins(const std::filesystem::path& plugin_dir);
}

namespace GPUTasks {
    // Rendering
    void render_mobjects(VkCommandBuffer cmd, std::span<Mobject*> mobjects);

    // Particle physics
    void simulate_particles(VkCommandBuffer cmd, ParticleSystem& ps, float dt);

    // Tessellation
    void tessellate_bezier_curves(VkCommandBuffer cmd, const VMobject& vmob);

    // Lighting and shading
    void compute_lighting(VkCommandBuffer cmd, const Scene& scene);

    // Post-processing
    void apply_bloom(VkCommandBuffer cmd, VkImageView source, VkImageView target);
}
```

---

## 4. Rendering API Backend Architecture

### 4.1 Backend Abstraction Layer

```cpp
// Abstract rendering interface
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    // Device management
    virtual void initialize(const DeviceConfig& config) = 0;
    virtual void shutdown() = 0;

    // Command submission
    virtual CommandBuffer* begin_frame() = 0;
    virtual void end_frame(CommandBuffer* cmd) = 0;
    virtual void submit(CommandBuffer* cmd) = 0;

    // Resource creation
    virtual GPUBuffer create_buffer(const BufferDesc& desc) = 0;
    virtual Texture create_texture(const TextureDesc& desc) = 0;
    virtual Pipeline create_pipeline(const PipelineDesc& desc) = 0;

    // Capabilities
    virtual bool supports_ray_tracing() const = 0;
    virtual bool supports_mesh_shaders() const = 0;
};
```

### 4.2 Vulkan Backend (Primary)

```cpp
class VulkanBackend : public RenderBackend {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;

    VmaAllocator allocator;
    VkDescriptorPool descriptor_pool;

    // Ray tracing extensions
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_properties;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

public:
    void initialize(const DeviceConfig& config) override {
        create_instance();
        select_physical_device();
        create_logical_device();
        create_allocator();
        load_extensions();
    }

    bool supports_ray_tracing() const override {
        return rt_properties.maxRayRecursionDepth > 0;
    }

    void trace_rays(
        VkCommandBuffer cmd,
        const RayTracingPipeline& pipeline,
        uint32_t width, uint32_t height
    ) {
        vkCmdTraceRaysKHR(
            cmd,
            &pipeline.raygen_sbt,
            &pipeline.miss_sbt,
            &pipeline.hit_sbt,
            &pipeline.callable_sbt,
            width, height, 1
        );
    }
};
```

### 4.3 DirectX 12 Backend (Windows)

```cpp
class DirectX12Backend : public RenderBackend {
    ComPtr<ID3D12Device5> device;
    ComPtr<ID3D12CommandQueue> command_queue;
    ComPtr<ID3D12CommandAllocator> command_allocator;
    ComPtr<ID3D12GraphicsCommandList4> command_list;

    // DXR (DirectX Raytracing)
    ComPtr<ID3D12StateObject> rt_state_object;
    ComPtr<ID3D12Resource> shader_table;

public:
    void initialize(const DeviceConfig& config) override {
        create_device();
        create_command_queue();
        create_command_allocator();
        create_command_list();
        init_dxr();
    }

    bool supports_ray_tracing() const override {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};
        device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS5,
            &options,
            sizeof(options)
        );
        return options.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
    }
};
```

### 4.4 Metal Backend (macOS/iOS)

```cpp
class MetalBackend : public RenderBackend {
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    id<MTLLibrary> shader_library;

    // Metal Performance Shaders
    id<MTLComputePipelineState> compute_pipeline;

public:
    void initialize(const DeviceConfig& config) override {
        device = MTLCreateSystemDefaultDevice();
        command_queue = [device newCommandQueue];
        load_shaders();
    }

    bool supports_ray_tracing() const override {
        return [device supportsRaytracing];
    }

    void dispatch_compute(
        id<MTLCommandBuffer> cmd_buffer,
        id<MTLComputePipelineState> pipeline,
        MTLSize grid_size
    ) {
        id<MTLComputeCommandEncoder> encoder =
            [cmd_buffer computeCommandEncoder];

        [encoder setComputePipelineState:pipeline];
        [encoder dispatchThreads:grid_size
               threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

        [encoder endEncoding];
    }
};
```

### 4.5 OpenGL Backend (Fallback)

```cpp
class OpenGLBackend : public RenderBackend {
    GLuint vao;
    GLuint default_fbo;

    // Compute shader support
    bool has_compute_shaders;

public:
    void initialize(const DeviceConfig& config) override {
        // Load OpenGL functions via GLAD/GLEW
        gladLoadGL();

        // Check extensions
        has_compute_shaders = GLAD_GL_VERSION_4_3;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }

    bool supports_ray_tracing() const override {
        return false;  // No ray tracing in OpenGL
    }

    // Fallback to rasterization
    void render_scene(const Scene& scene) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto* mobject : scene.get_mobjects()) {
            render_mobject_gl(mobject);
        }

        glFlush();
    }
};
```

### 4.6 Backend Selection Strategy

```cpp
class RenderBackendFactory {
public:
    static std::unique_ptr<RenderBackend> create_backend(
        BackendType preferred = BackendType::Auto
    ) {
        if (preferred == BackendType::Auto) {
            preferred = detect_best_backend();
        }

        switch (preferred) {
            case BackendType::Vulkan:
                if (is_vulkan_available()) {
                    return std::make_unique<VulkanBackend>();
                }
                [[fallthrough]];

            case BackendType::DirectX12:
                #ifdef _WIN32
                if (is_dx12_available()) {
                    return std::make_unique<DirectX12Backend>();
                }
                #endif
                [[fallthrough]];

            case BackendType::Metal:
                #ifdef __APPLE__
                if (is_metal_available()) {
                    return std::make_unique<MetalBackend>();
                }
                #endif
                [[fallthrough]];

            case BackendType::OpenGL:
                return std::make_unique<OpenGLBackend>();

            default:
                throw std::runtime_error("No suitable backend available");
        }
    }

private:
    static BackendType detect_best_backend() {
        #ifdef __APPLE__
            return BackendType::Metal;
        #elif _WIN32
            return is_vulkan_available() ? BackendType::Vulkan
                                         : BackendType::DirectX12;
        #else
            return BackendType::Vulkan;
        #endif
    }
};
```

---

## 5. Vulkan Compute Pipeline

### 5.1 Compute Pipeline Architecture

```cpp
class ComputePipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout descriptor_set_layout;
    VkShaderModule shader_module;

public:
    struct ComputeDispatch {
        uint32_t group_count_x;
        uint32_t group_count_y;
        uint32_t group_count_z;

        // Calculate from total size and local size
        static ComputeDispatch calculate(
            uint32_t total_x, uint32_t total_y, uint32_t total_z,
            uint32_t local_x, uint32_t local_y, uint32_t local_z
        ) {
            return {
                (total_x + local_x - 1) / local_x,
                (total_y + local_y - 1) / local_y,
                (total_z + local_z - 1) / local_z
            };
        }
    };

    void dispatch(VkCommandBuffer cmd, const ComputeDispatch& dispatch) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdDispatch(cmd, dispatch.group_count_x,
                          dispatch.group_count_y,
                          dispatch.group_count_z);
    }
};
```

### 5.2 Example: Bezier Tessellation Compute Shader

```glsl
// bezier_tessellation.comp
#version 450

layout(local_size_x = 256) in;

// Input: Bezier control points
layout(std430, binding = 0) readonly buffer BezierCurves {
    vec4 control_points[];  // 4 control points per curve (cubic Bezier)
};

// Output: Tessellated vertices
layout(std430, binding = 1) writeonly buffer TessellatedVertices {
    vec3 vertices[];
};

layout(push_constant) uniform Constants {
    uint num_curves;
    uint segments_per_curve;
};

// Cubic Bezier evaluation
vec3 evaluate_bezier(vec4 p0, vec4 p1, vec4 p2, vec4 p3, float t) {
    float s = 1.0 - t;
    return s*s*s * p0.xyz
         + 3.0*s*s*t * p1.xyz
         + 3.0*s*t*t * p2.xyz
         + t*t*t * p3.xyz;
}

void main() {
    uint curve_idx = gl_GlobalInvocationID.x / segments_per_curve;
    uint segment_idx = gl_GlobalInvocationID.x % segments_per_curve;

    if (curve_idx >= num_curves) return;

    // Load control points
    vec4 p0 = control_points[curve_idx * 4 + 0];
    vec4 p1 = control_points[curve_idx * 4 + 1];
    vec4 p2 = control_points[curve_idx * 4 + 2];
    vec4 p3 = control_points[curve_idx * 4 + 3];

    // Compute parameter t
    float t = float(segment_idx) / float(segments_per_curve);

    // Evaluate Bezier
    vec3 pos = evaluate_bezier(p0, p1, p2, p3, t);

    // Write output
    vertices[gl_GlobalInvocationID.x] = pos;
}
```

**C++ Host Code**:
```cpp
void tessellate_bezier_curves(
    VkCommandBuffer cmd,
    const std::vector<BezierCurve>& curves,
    uint32_t segments_per_curve
) {
    // Upload control points to GPU
    GPUBuffer control_points_buffer = upload_to_gpu(curves);

    // Allocate output buffer
    size_t total_vertices = curves.size() * segments_per_curve;
    GPUBuffer output_buffer = allocate_buffer(
        total_vertices * sizeof(glm::vec3),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    );

    // Bind compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                      bezier_tessellation_pipeline);

    // Bind descriptor set (buffers)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                           pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

    // Push constants
    struct {
        uint32_t num_curves;
        uint32_t segments_per_curve;
    } constants{curves.size(), segments_per_curve};

    vkCmdPushConstants(cmd, pipeline_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(constants), &constants);

    // Dispatch compute shader
    uint32_t total_invocations = curves.size() * segments_per_curve;
    uint32_t workgroups = (total_invocations + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);

    // Memory barrier
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr
    );
}
```

---

## 6. CUDA/OpenCL Integration

### 6.1 CUDA for NVIDIA GPUs

```cpp
#ifdef MANIM_ENABLE_CUDA

class CUDACompute {
    CUcontext cuda_context;
    CUdevice cuda_device;
    CUstream cuda_stream;

public:
    void initialize() {
        cuInit(0);
        cuDeviceGet(&cuda_device, 0);
        cuCtxCreate(&cuda_context, 0, cuda_device);
        cuStreamCreate(&cuda_stream, CU_STREAM_DEFAULT);
    }

    // Interop with Vulkan
    void import_vulkan_buffer(VkBuffer vk_buffer) {
        // Get external memory handle from Vulkan
        VkMemoryGetFdInfoKHR fd_info{};
        fd_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        fd_info.memory = vk_memory;
        fd_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

        int fd;
        vkGetMemoryFdKHR(device, &fd_info, &fd);

        // Import into CUDA
        CUDA_EXTERNAL_MEMORY_HANDLE_DESC mem_desc{};
        mem_desc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
        mem_desc.handle.fd = fd;
        mem_desc.size = buffer_size;

        CUexternalMemory cuda_ext_mem;
        cuImportExternalMemory(&cuda_ext_mem, &mem_desc);

        // Map to CUDA device pointer
        CUDA_EXTERNAL_MEMORY_BUFFER_DESC buf_desc{};
        buf_desc.size = buffer_size;

        CUdeviceptr cuda_ptr;
        cuExternalMemoryGetMappedBuffer(&cuda_ptr, cuda_ext_mem, &buf_desc);
    }

    // Run CUDA kernel
    template<typename... Args>
    void launch_kernel(
        CUfunction kernel,
        dim3 grid_size,
        dim3 block_size,
        Args... args
    ) {
        void* kernel_args[] = {&args...};

        cuLaunchKernel(
            kernel,
            grid_size.x, grid_size.y, grid_size.z,
            block_size.x, block_size.y, block_size.z,
            0, cuda_stream,
            kernel_args, nullptr
        );
    }
};

#endif
```

**CUDA Kernel Example**:
```cuda
// particle_simulation.cu
__global__ void simulate_particles(
    float3* positions,
    float3* velocities,
    uint32_t num_particles,
    float dt
) {
    uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_particles) return;

    // Update velocity (gravity)
    velocities[idx].y -= 9.81f * dt;

    // Update position
    positions[idx] += velocities[idx] * dt;

    // Collision with ground
    if (positions[idx].y < 0.0f) {
        positions[idx].y = 0.0f;
        velocities[idx].y = -velocities[idx].y * 0.8f;  // Bounce
    }
}
```

### 6.2 OpenCL for Cross-Vendor Support

```cpp
#ifdef MANIM_ENABLE_OPENCL

class OpenCLCompute {
    cl_context context;
    cl_device_id device;
    cl_command_queue queue;
    cl_program program;

public:
    void initialize() {
        // Get platform and device
        cl_platform_id platform;
        clGetPlatformIDs(1, &platform, nullptr);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);

        // Create context and queue
        context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
        queue = clCreateCommandQueueWithProperties(context, device, nullptr, nullptr);
    }

    void compile_kernel(const std::string& source_code) {
        const char* source = source_code.c_str();
        program = clCreateProgramWithSource(context, 1, &source, nullptr, nullptr);
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    }

    void run_kernel(
        const std::string& kernel_name,
        size_t global_work_size,
        std::span<cl_mem> buffers
    ) {
        cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), nullptr);

        // Set arguments
        for (size_t i = 0; i < buffers.size(); ++i) {
            clSetKernelArg(kernel, i, sizeof(cl_mem), &buffers[i]);
        }

        // Execute
        clEnqueueNDRangeKernel(queue, kernel, 1, nullptr,
                              &global_work_size, nullptr,
                              0, nullptr, nullptr);

        clFinish(queue);
        clReleaseKernel(kernel);
    }
};

#endif
```

---

## 7. Metal Compute Support (macOS)

```cpp
#ifdef __APPLE__

class MetalCompute {
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLLibrary> library;

public:
    void initialize() {
        device = MTLCreateSystemDefaultDevice();
        queue = [device newCommandQueue];

        // Compile Metal shaders
        NSError* error = nullptr;
        library = [device newLibraryWithSource:@(shader_source)
                                        options:nil
                                          error:&error];
    }

    void dispatch_compute(
        const std::string& kernel_name,
        MTLSize grid_size,
        MTLSize threadgroup_size,
        std::span<id<MTLBuffer>> buffers
    ) {
        id<MTLCommandBuffer> cmd_buffer = [queue commandBuffer];
        id<MTLComputeCommandEncoder> encoder =
            [cmd_buffer computeCommandEncoder];

        // Load kernel function
        id<MTLFunction> function =
            [library newFunctionWithName:@(kernel_name.c_str())];

        id<MTLComputePipelineState> pipeline =
            [device newComputePipelineStateWithFunction:function error:nil];

        [encoder setComputePipelineState:pipeline];

        // Bind buffers
        for (size_t i = 0; i < buffers.size(); ++i) {
            [encoder setBuffer:buffers[i] offset:0 atIndex:i];
        }

        // Dispatch
        [encoder dispatchThreads:grid_size
               threadsPerThreadgroup:threadgroup_size];

        [encoder endEncoding];
        [cmd_buffer commit];
        [cmd_buffer waitUntilCompleted];
    }
};

#endif
```

**Metal Shader Example**:
```metal
// transform.metal
#include <metal_stdlib>
using namespace metal;

kernel void transform_points(
    device float3* points [[buffer(0)]],
    constant float4x4& matrix [[buffer(1)]],
    uint idx [[thread_position_in_grid]]
) {
    float4 p = float4(points[idx], 1.0);
    points[idx] = (matrix * p).xyz;
}
```

---

## 8. Summary: Technology Stack

### Core Technologies

| Component | Primary | Alternatives |
|-----------|---------|--------------|
| **Graphics API** | Vulkan 1.3+ | DirectX 12, Metal, OpenGL 4.6 |
| **Memory Allocator** | VMA (Vulkan Memory Allocator) | D3D12MA, Metal allocator |
| **Windowing** | GLFW 3.4+ | SDL3, Platform-native |
| **Math Library** | GLM + Eigen | DirectXMath (Windows) |
| **Compute** | Vulkan Compute | CUDA (NVIDIA), OpenCL, Metal Compute |
| **Ray Tracing** | Vulkan RT | DXR (DirectX), Metal RT |
| **Shader Compiler** | glslang + SPIRV-Cross | DXC (DirectX), Metal shader compiler |

### Support Matrix

| Feature | Vulkan | DX12 | Metal | OpenGL |
|---------|--------|------|-------|--------|
| **Ray Tracing** | ✅ RTX | ✅ DXR | ✅ (Apple Silicon) | ❌ |
| **Mesh Shaders** | ✅ | ✅ | ✅ | ❌ |
| **Compute** | ✅ | ✅ | ✅ | ✅ (4.3+) |
| **Tessellation** | ✅ | ✅ | ✅ | ✅ (4.0+) |
| **Bindless** | ✅ | ✅ | ✅ | ✅ (ARB_bindless_texture) |
| **Multi-GPU** | ✅ | ✅ | ✅ | ❌ |

---

## Next Steps
See `RENDERING_PIPELINE.md` for advanced 3D rendering pipeline design.
See `CONVERSION_CHECKLIST.md` for detailed conversion tracking.
