# Advanced 3D Rendering Pipeline Design

## Overview

This document outlines a state-of-the-art rendering pipeline for Manim C++ featuring:
- **Physically-Based Rendering (PBR)**
- **Real-time Ray Tracing** (RTX/DXR/Metal RT)
- **Global Illumination**
- **Advanced Shadow Techniques**
- **Post-Processing Pipeline**
- **LOD & Culling Systems**

---

## 1. Rendering Pipeline Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      SCENE SUBMISSION                            │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Scene Graph → Visibility Culling → Draw Call Generation  │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     GEOMETRY PROCESSING                          │
│  ┌──────────────┬────────────────┬────────────────────────────┐ │
│  │   Frustum    │   Occlusion    │   LOD Selection            │ │
│  │   Culling    │   Culling      │   (Distance/Screen Space)  │ │
│  └──────────────┴────────────────┴────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      SHADOW PASS                                 │
│  ┌────────────────┬──────────────────┬───────────────────────┐  │
│  │  Cascaded      │  Variance        │  Ray-Traced           │  │
│  │  Shadow Maps   │  Shadow Maps     │  Shadows (RTX)        │  │
│  └────────────────┴──────────────────┴───────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      G-BUFFER PASS                               │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Position | Normal | Albedo | Metallic-Roughness | Depth  │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LIGHTING PASS                                 │
│  ┌──────────────┬──────────────┬──────────────────────────────┐ │
│  │   Direct     │   Indirect   │   Ambient Occlusion          │ │
│  │   Lighting   │   (GI/RT)    │   (SSAO/HBAO/RT)             │ │
│  └──────────────┴──────────────┴──────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  POST-PROCESSING                                 │
│  ┌─────────┬──────────┬──────────┬──────────┬──────────┬─────┐ │
│  │  Bloom  │   SSAO   │   TAA    │   DoF    │  Motion  │ Tone│ │
│  │         │          │          │          │   Blur   │ Map │ │
│  └─────────┴──────────┴──────────┴──────────┴──────────┴─────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      PRESENTATION                                │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Swapchain / Frame Buffer → Display / Video File          │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Physically-Based Rendering (PBR)

### 2.1 Material System

```cpp
struct PBRMaterial {
    // Albedo (base color)
    glm::vec3 albedo{1.0f};
    VkImageView albedo_texture = VK_NULL_HANDLE;

    // Metallic-Roughness workflow
    float metallic{0.0f};
    float roughness{0.5f};
    VkImageView metallic_roughness_texture = VK_NULL_HANDLE;

    // Normal mapping
    VkImageView normal_map = VK_NULL_HANDLE;

    // Ambient occlusion
    float ao{1.0f};
    VkImageView ao_texture = VK_NULL_HANDLE;

    // Emission
    glm::vec3 emissive{0.0f};
    VkImageView emissive_texture = VK_NULL_HANDLE;

    // Additional properties
    float alpha{1.0f};
    bool double_sided{false};
};
```

### 2.2 PBR Shader (Fragment)

```glsl
#version 450

// G-Buffer outputs
layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_albedo;
layout(location = 3) out vec4 out_metallic_roughness;

// Inputs from vertex shader
layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_tangent;
layout(location = 3) in vec2 frag_uv;

// Material uniforms
layout(binding = 0) uniform sampler2D albedo_tex;
layout(binding = 1) uniform sampler2D metallic_roughness_tex;
layout(binding = 2) uniform sampler2D normal_tex;
layout(binding = 3) uniform sampler2D ao_tex;

layout(binding = 4) uniform MaterialData {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
} material;

// TBN matrix for normal mapping
mat3 get_tbn_matrix() {
    vec3 N = normalize(frag_normal);
    vec3 T = normalize(frag_tangent);
    vec3 B = cross(N, T);
    return mat3(T, B, N);
}

void main() {
    // Sample textures
    vec3 albedo = texture(albedo_tex, frag_uv).rgb * material.albedo;
    vec2 mr = texture(metallic_roughness_tex, frag_uv).rg;
    float metallic = mr.r * material.metallic;
    float roughness = mr.g * material.roughness;
    float ao = texture(ao_tex, frag_uv).r * material.ao;

    // Normal mapping
    vec3 tangent_normal = texture(normal_tex, frag_uv).xyz * 2.0 - 1.0;
    mat3 TBN = get_tbn_matrix();
    vec3 normal = normalize(TBN * tangent_normal);

    // Write to G-Buffer
    out_position = vec4(frag_position, 1.0);
    out_normal = vec4(normal, 1.0);
    out_albedo = vec4(albedo, 1.0);
    out_metallic_roughness = vec4(metallic, roughness, ao, 1.0);
}
```

### 2.3 PBR Lighting (Deferred)

```glsl
#version 450

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D g_position;
layout(binding = 1) uniform sampler2D g_normal;
layout(binding = 2) uniform sampler2D g_albedo;
layout(binding = 3) uniform sampler2D g_metallic_roughness;

layout(binding = 4) uniform LightData {
    vec3 light_position;
    vec3 light_color;
    float light_intensity;
    vec3 camera_position;
} light;

const float PI = 3.14159265359;

// Fresnel-Schlick approximation
vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

// GGX/Trowbridge-Reitz normal distribution
float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Smith's Schlick-GGX geometry function
float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(g_position, 0);

    // Read G-Buffer
    vec3 position = texture(g_position, uv).rgb;
    vec3 normal = texture(g_normal, uv).rgb;
    vec3 albedo = texture(g_albedo, uv).rgb;
    vec4 mr = texture(g_metallic_roughness, uv);
    float metallic = mr.r;
    float roughness = mr.g;
    float ao = mr.b;

    // Lighting vectors
    vec3 V = normalize(light.camera_position - position);
    vec3 L = normalize(light.light_position - position);
    vec3 H = normalize(V + L);

    // Calculate radiance
    float distance = length(light.light_position - position);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.light_color * light.light_intensity * attenuation;

    // Cook-Torrance BRDF
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
    float NDF = distribution_ggx(normal, H, roughness);
    float G = geometry_smith(normal, V, L, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    out_color = vec4(color, 1.0);
}
```

---

## 3. Real-Time Ray Tracing

### 3.1 Acceleration Structure Setup

```cpp
class AccelerationStructure {
public:
    // Bottom-Level Acceleration Structure (BLAS) - per object
    struct BLAS {
        VkAccelerationStructureKHR handle;
        VkBuffer buffer;
        VmaAllocation allocation;

        static BLAS create_from_geometry(
            VkDevice device,
            const GeometryData& geometry
        ) {
            VkAccelerationStructureGeometryKHR as_geometry{};
            as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            as_geometry.geometry.triangles.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            as_geometry.geometry.triangles.vertexData.deviceAddress =
                get_buffer_device_address(geometry.vertex_buffer);
            as_geometry.geometry.triangles.maxVertex = geometry.vertex_count;
            as_geometry.geometry.triangles.vertexStride = sizeof(Vertex);
            as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            as_geometry.geometry.triangles.indexData.deviceAddress =
                get_buffer_device_address(geometry.index_buffer);

            // Build BLAS
            VkAccelerationStructureBuildGeometryInfoKHR build_info{};
            build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            build_info.geometryCount = 1;
            build_info.pGeometries = &as_geometry;

            // Get size requirements
            VkAccelerationStructureBuildSizesInfoKHR size_info{};
            size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            vkGetAccelerationStructureBuildSizesKHR(
                device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &build_info,
                &geometry.triangle_count,
                &size_info
            );

            // Create acceleration structure
            BLAS blas;
            blas.buffer = create_buffer(size_info.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);

            VkAccelerationStructureCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            create_info.buffer = blas.buffer;
            create_info.size = size_info.accelerationStructureSize;
            create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

            vkCreateAccelerationStructureKHR(device, &create_info, nullptr, &blas.handle);

            return blas;
        }
    };

    // Top-Level Acceleration Structure (TLAS) - entire scene
    struct TLAS {
        VkAccelerationStructureKHR handle;
        VkBuffer buffer;
        VmaAllocation allocation;

        static TLAS create_from_instances(
            VkDevice device,
            const std::vector<VkAccelerationStructureInstanceKHR>& instances
        ) {
            // Upload instances to GPU
            GPUBuffer instance_buffer = upload_instances(instances);

            VkAccelerationStructureGeometryKHR as_geometry{};
            as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            as_geometry.geometry.instances.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            as_geometry.geometry.instances.data.deviceAddress =
                get_buffer_device_address(instance_buffer.buffer);

            // Build TLAS (similar to BLAS)
            // ...

            return tlas;
        }
    };
};
```

### 3.2 Ray Tracing Pipeline

```cpp
class RayTracingPipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    // Shader binding table
    struct ShaderBindingTable {
        VkBuffer buffer;
        VkDeviceAddress raygen_sbt;
        VkDeviceAddress miss_sbt;
        VkDeviceAddress hit_sbt;
    } sbt;

public:
    void create_pipeline(
        VkShaderModule raygen_shader,
        VkShaderModule miss_shader,
        VkShaderModule closest_hit_shader
    ) {
        // Shader stages
        VkPipelineShaderStageCreateInfo stages[3] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                .module = raygen_shader,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
                .module = miss_shader,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                .module = closest_hit_shader,
                .pName = "main"
            }
        };

        // Shader groups
        VkRayTracingShaderGroupCreateInfoKHR groups[3] = {
            {
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader = 0,  // raygen
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR
            },
            {
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader = 1,  // miss
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR
            },
            {
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                .generalShader = VK_SHADER_UNUSED_KHR,
                .closestHitShader = 2,  // closest hit
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR
            }
        };

        // Create ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        pipeline_info.stageCount = 3;
        pipeline_info.pStages = stages;
        pipeline_info.groupCount = 3;
        pipeline_info.pGroups = groups;
        pipeline_info.maxPipelineRayRecursionDepth = 2;
        pipeline_info.layout = pipeline_layout;

        vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                       1, &pipeline_info, nullptr, &pipeline);

        // Build shader binding table
        build_shader_binding_table();
    }

    void trace_rays(VkCommandBuffer cmd, uint32_t width, uint32_t height) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);

        VkStridedDeviceAddressRegionKHR raygen_sbt{
            .deviceAddress = sbt.raygen_sbt,
            .stride = shader_group_handle_size,
            .size = shader_group_handle_size
        };

        VkStridedDeviceAddressRegionKHR miss_sbt{
            .deviceAddress = sbt.miss_sbt,
            .stride = shader_group_handle_size,
            .size = shader_group_handle_size
        };

        VkStridedDeviceAddressRegionKHR hit_sbt{
            .deviceAddress = sbt.hit_sbt,
            .stride = shader_group_handle_size,
            .size = shader_group_handle_size
        };

        VkStridedDeviceAddressRegionKHR callable_sbt{};

        vkCmdTraceRaysKHR(cmd, &raygen_sbt, &miss_sbt, &hit_sbt, &callable_sbt,
                          width, height, 1);
    }
};
```

### 3.3 Ray Tracing Shaders

**Ray Generation Shader** (raygen.rgen):
```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;
layout(binding = 1, set = 0, rgba8) uniform image2D output_image;

layout(binding = 2, set = 0) uniform CameraData {
    mat4 view_inverse;
    mat4 proj_inverse;
} camera;

layout(location = 0) rayPayloadEXT vec3 hit_value;

void main() {
    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 in_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = in_uv * 2.0 - 1.0;

    vec4 origin = camera.view_inverse * vec4(0, 0, 0, 1);
    vec4 target = camera.proj_inverse * vec4(d.x, d.y, 1, 1);
    vec4 direction = camera.view_inverse * vec4(normalize(target.xyz), 0);

    float tmin = 0.001;
    float tmax = 10000.0;

    traceRayEXT(
        tlas,           // acceleration structure
        gl_RayFlagsOpaqueEXT,  // ray flags
        0xff,           // cull mask
        0,              // sbt record offset
        0,              // sbt record stride
        0,              // miss index
        origin.xyz,     // ray origin
        tmin,           // ray min range
        direction.xyz,  // ray direction
        tmax,           // ray max range
        0               // payload location
    );

    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), vec4(hit_value, 1.0));
}
```

**Closest Hit Shader** (closesthit.rchit):
```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;  // Barycentric coordinates

layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) buffer Normals { vec4 n[]; } normals;

void main() {
    // Get triangle indices
    uint idx0 = indices.i[3 * gl_PrimitiveID + 0];
    uint idx1 = indices.i[3 * gl_PrimitiveID + 1];
    uint idx2 = indices.i[3 * gl_PrimitiveID + 2];

    // Get vertex positions
    vec3 v0 = vertices.v[idx0].xyz;
    vec3 v1 = vertices.v[idx1].xyz;
    vec3 v2 = vertices.v[idx2].xyz;

    // Get normals
    vec3 n0 = normals.n[idx0].xyz;
    vec3 n1 = normals.n[idx1].xyz;
    vec3 n2 = normals.n[idx2].xyz;

    // Interpolate normal using barycentric coordinates
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 normal = normalize(n0 * barycentrics.x +
                           n1 * barycentrics.y +
                           n2 * barycentrics.z);

    // Simple shading
    vec3 light_dir = normalize(vec3(1, 1, 1));
    float ndotl = max(dot(normal, light_dir), 0.0);

    hit_value = vec3(0.8) * ndotl;
}
```

**Miss Shader** (miss.rmiss):
```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hit_value;

void main() {
    // Sky gradient
    vec3 ray_dir = normalize(gl_WorldRayDirectionEXT);
    float t = 0.5 * (ray_dir.y + 1.0);
    hit_value = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
}
```

---

## 4. Global Illumination

### 4.1 Screen Space Global Illumination (SSGI)

```glsl
#version 450

layout(binding = 0) uniform sampler2D g_position;
layout(binding = 1) uniform sampler2D g_normal;
layout(binding = 2) uniform sampler2D g_albedo;
layout(binding = 3) uniform sampler2D depth;

layout(binding = 4) uniform CameraData {
    mat4 view;
    mat4 projection;
    mat4 view_projection_inverse;
} camera;

layout(location = 0) out vec4 out_gi;

const int NUM_SAMPLES = 16;
const float RADIUS = 0.5;
const float BIAS = 0.025;

vec3 random_hemisphere_sample(vec3 normal, vec2 xi) {
    float phi = 2.0 * 3.14159265 * xi.x;
    float cos_theta = 1.0 - xi.y;
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    vec3 sample_dir = vec3(
        cos(phi) * sin_theta,
        sin(phi) * sin_theta,
        cos_theta
    );

    // Orient sample to normal
    vec3 up = abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(tangent * sample_dir.x +
                    bitangent * sample_dir.y +
                    normal * sample_dir.z);
}

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(g_position, 0);

    vec3 position = texture(g_position, uv).xyz;
    vec3 normal = texture(g_normal, uv).xyz;
    vec3 albedo = texture(g_albedo, uv).xyz;

    vec3 indirect_light = vec3(0.0);

    // Random seed
    vec2 noise = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        // Generate random sample
        vec2 xi = vec2(
            fract(noise.x + float(i) / float(NUM_SAMPLES)),
            fract(noise.y + float(i) / float(NUM_SAMPLES + 1))
        );

        vec3 sample_dir = random_hemisphere_sample(normal, xi);
        vec3 sample_pos = position + sample_dir * RADIUS;

        // Project to screen space
        vec4 proj_sample = camera.projection * camera.view * vec4(sample_pos, 1.0);
        proj_sample.xy /= proj_sample.w;
        vec2 sample_uv = proj_sample.xy * 0.5 + 0.5;

        // Sample scene at projected position
        vec3 sample_position = texture(g_position, sample_uv).xyz;
        vec3 sample_albedo = texture(g_albedo, sample_uv).xyz;

        // Check occlusion
        float distance_check = smoothstep(0.0, 1.0, RADIUS / abs(position.z - sample_position.z));
        float ndotl = max(dot(normal, sample_dir), 0.0);

        indirect_light += sample_albedo * ndotl * distance_check;
    }

    indirect_light /= float(NUM_SAMPLES);

    out_gi = vec4(indirect_light, 1.0);
}
```

### 4.2 Voxel-Based Global Illumination (VXGI)

```cpp
class VoxelGI {
    struct VoxelGrid {
        VkImage voxel_texture;          // 3D texture
        VkImageView voxel_view;
        uint32_t resolution = 256;      // 256^3 voxels

        // Mipmap chain for cone tracing
        uint32_t mip_levels;
    };

    VoxelGrid voxel_grid;

public:
    void voxelize_scene(VkCommandBuffer cmd, const Scene& scene) {
        // 1. Clear voxel grid
        clear_voxel_grid(cmd);

        // 2. Voxelize geometry using conservative rasterization
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                         conservative_voxelization_pipeline);

        for (auto* mobject : scene.get_mobjects()) {
            render_to_voxel_grid(cmd, mobject);
        }

        // 3. Generate mipmaps for cone tracing
        generate_voxel_mipmaps(cmd);
    }

    void trace_cones(VkCommandBuffer cmd) {
        // Use compute shader to trace cones through voxel grid
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                         cone_tracing_pipeline);

        vkCmdDispatch(cmd, width / 16, height / 16, 1);
    }
};
```

---

## 5. Shadow Mapping

### 5.1 Cascaded Shadow Maps (CSM)

```cpp
class CascadedShadowMaps {
    static constexpr int NUM_CASCADES = 4;

    struct Cascade {
        glm::mat4 view_projection;
        float split_depth;
        VkFramebuffer framebuffer;
        VkImageView depth_image_view;
    };

    std::array<Cascade, NUM_CASCADES> cascades;

public:
    void calculate_cascades(const Camera& camera, const glm::vec3& light_dir) {
        float near_clip = camera.get_near_clip();
        float far_clip = camera.get_far_clip();
        float clip_range = far_clip - near_clip;

        float min_z = near_clip;
        float max_z = near_clip + clip_range;

        float range = max_z - min_z;
        float ratio = max_z / min_z;

        // Calculate split depths based on practical split scheme
        for (int i = 0; i < NUM_CASCADES; ++i) {
            float p = (i + 1) / static_cast<float>(NUM_CASCADES);
            float log = min_z * std::pow(ratio, p);
            float uniform = min_z + range * p;
            float d = 0.95f * log + 0.05f * uniform;  // Blend logarithmic and uniform

            cascades[i].split_depth = (d - near_clip) / clip_range;

            // Calculate view-projection matrix for this cascade
            cascades[i].view_projection = calculate_light_vp(camera, d, light_dir);
        }
    }

    void render_cascades(VkCommandBuffer cmd, const Scene& scene) {
        for (int i = 0; i < NUM_CASCADES; ++i) {
            // Render to cascade i
            VkRenderPassBeginInfo begin_info{};
            begin_info.framebuffer = cascades[i].framebuffer;
            // ...

            vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

            // Set cascade view-projection
            update_shadow_uniform(cascades[i].view_projection);

            // Render scene from light's perspective
            for (auto* mobject : scene.get_mobjects()) {
                render_shadow(cmd, mobject);
            }

            vkCmdEndRenderPass(cmd);
        }
    }
};
```

### 5.2 Variance Shadow Maps (VSM)

```glsl
// Shadow map generation (fragment shader)
#version 450

layout(location = 0) out vec2 moments;

void main() {
    float depth = gl_FragCoord.z;

    // Store depth and depth^2
    moments = vec2(depth, depth * depth);
}
```

```glsl
// Shadow sampling (fragment shader)
#version 450

layout(binding = 0) uniform sampler2D shadow_map;

float variance_shadow(vec2 moments, float depth) {
    float p = (depth <= moments.x) ? 1.0 : 0.0;

    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, 0.00002);

    float d = depth - moments.x;
    float p_max = variance / (variance + d * d);

    return max(p, p_max);
}

float sample_shadow(vec3 shadow_coord) {
    vec2 moments = texture(shadow_map, shadow_coord.xy).rg;
    return variance_shadow(moments, shadow_coord.z);
}
```

### 5.3 Ray-Traced Shadows

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 1) rayPayloadEXT bool is_shadowed;

bool trace_shadow_ray(vec3 origin, vec3 light_dir, float max_distance) {
    is_shadowed = true;

    traceRayEXT(
        tlas,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xff,
        0,  // sbt record offset
        0,  // sbt record stride
        1,  // miss index (shadow miss shader)
        origin,
        0.001,  // tmin
        light_dir,
        max_distance,  // tmax
        1   // payload location
    );

    return !is_shadowed;  // Return true if NOT shadowed
}
```

---

## 6. Post-Processing Pipeline

### 6.1 Bloom Effect

```glsl
// Downsampling and threshold
#version 450

layout(binding = 0) uniform sampler2D input_image;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Constants {
    float threshold;
    float soft_threshold;
} bloom;

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(input_image, 0);
    vec3 color = texture(input_image, uv).rgb;

    // Brightness threshold
    float brightness = max(max(color.r, color.g), color.b);
    float soft = brightness - bloom.threshold + bloom.soft_threshold;
    soft = clamp(soft, 0.0, 2.0 * bloom.soft_threshold);
    soft = soft * soft / (4.0 * bloom.soft_threshold + 0.00001);
    float contribution = max(soft, brightness - bloom.threshold) / max(brightness, 0.00001);

    out_color = vec4(color * contribution, 1.0);
}
```

```glsl
// Gaussian blur (separable)
#version 450

layout(binding = 0) uniform sampler2D input_image;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Constants {
    vec2 direction;  // (1, 0) for horizontal, (0, 1) for vertical
} blur;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(input_image, 0);
    vec2 uv = gl_FragCoord.xy * tex_offset;

    vec3 result = texture(input_image, uv).rgb * weights[0];

    for (int i = 1; i < 5; ++i) {
        vec2 offset = blur.direction * tex_offset * float(i);
        result += texture(input_image, uv + offset).rgb * weights[i];
        result += texture(input_image, uv - offset).rgb * weights[i];
    }

    out_color = vec4(result, 1.0);
}
```

### 6.2 Temporal Anti-Aliasing (TAA)

```glsl
#version 450

layout(binding = 0) uniform sampler2D current_frame;
layout(binding = 1) uniform sampler2D previous_frame;
layout(binding = 2) uniform sampler2D velocity;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Constants {
    float blend_factor;
} taa;

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(current_frame, 0);

    // Sample current frame
    vec3 current = texture(current_frame, uv).rgb;

    // Sample velocity and reproject
    vec2 motion = texture(velocity, uv).rg;
    vec2 prev_uv = uv - motion;

    // Sample previous frame
    vec3 previous = texture(previous_frame, prev_uv).rgb;

    // Neighborhood clamping to reduce ghosting
    vec3 near_color0 = textureOffset(current_frame, uv, ivec2(1, 0)).rgb;
    vec3 near_color1 = textureOffset(current_frame, uv, ivec2(0, 1)).rgb;
    vec3 near_color2 = textureOffset(current_frame, uv, ivec2(-1, 0)).rgb;
    vec3 near_color3 = textureOffset(current_frame, uv, ivec2(0, -1)).rgb;

    vec3 box_min = min(current, min(near_color0, min(near_color1, min(near_color2, near_color3))));
    vec3 box_max = max(current, max(near_color0, max(near_color1, max(near_color2, near_color3))));

    previous = clamp(previous, box_min, box_max);

    // Blend current and previous frames
    vec3 result = mix(previous, current, taa.blend_factor);

    out_color = vec4(result, 1.0);
}
```

### 6.3 Tone Mapping

```glsl
#version 450

layout(binding = 0) uniform sampler2D hdr_image;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Constants {
    float exposure;
    float gamma;
} tonemap;

// ACES filmic tone mapping
vec3 aces_tonemap(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(hdr_image, 0);
    vec3 hdr = texture(hdr_image, uv).rgb;

    // Exposure
    vec3 color = hdr * tonemap.exposure;

    // Tone mapping
    color = aces_tonemap(color);

    // Gamma correction
    color = pow(color, vec3(1.0 / tonemap.gamma));

    out_color = vec4(color, 1.0);
}
```

---

## 7. Level-of-Detail (LOD) System

```cpp
class LODSystem {
public:
    struct LODLevel {
        uint32_t vertex_count;
        uint32_t index_count;
        VkBuffer vertex_buffer;
        VkBuffer index_buffer;
        float distance_threshold;  // Switch to this LOD at this distance
    };

    struct LODMesh {
        std::vector<LODLevel> lod_levels;
        glm::vec3 bounding_sphere_center;
        float bounding_sphere_radius;
    };

    uint32_t select_lod(
        const LODMesh& mesh,
        const glm::vec3& camera_position
    ) const {
        float distance = glm::length(camera_position - mesh.bounding_sphere_center);
        distance -= mesh.bounding_sphere_radius;

        for (size_t i = 0; i < mesh.lod_levels.size(); ++i) {
            if (distance < mesh.lod_levels[i].distance_threshold) {
                return i;
            }
        }

        return mesh.lod_levels.size() - 1;  // Return lowest detail
    }

    // Screen-space LOD selection (more accurate)
    uint32_t select_lod_screen_space(
        const LODMesh& mesh,
        const glm::vec3& camera_position,
        const glm::mat4& projection,
        float screen_height
    ) const {
        float distance = glm::length(camera_position - mesh.bounding_sphere_center);

        // Project bounding sphere to screen space
        glm::vec4 proj = projection * glm::vec4(mesh.bounding_sphere_center, 1.0);
        float screen_radius = (mesh.bounding_sphere_radius / proj.w) *
                             (screen_height * 0.5f);

        // Select LOD based on screen-space size
        for (size_t i = 0; i < mesh.lod_levels.size(); ++i) {
            float lod_threshold = 100.0f / (i + 1);  // Example thresholds
            if (screen_radius > lod_threshold) {
                return i;
            }
        }

        return mesh.lod_levels.size() - 1;
    }
};
```

---

## 8. Occlusion & Frustum Culling

### 8.1 Frustum Culling

```cpp
class FrustumCuller {
    struct Plane {
        glm::vec3 normal;
        float distance;

        float distance_to_point(const glm::vec3& point) const {
            return glm::dot(normal, point) + distance;
        }
    };

    std::array<Plane, 6> frustum_planes;  // Left, right, top, bottom, near, far

public:
    void extract_planes(const glm::mat4& view_projection) {
        // Extract frustum planes from view-projection matrix
        // Left plane
        frustum_planes[0].normal = glm::vec3(
            view_projection[0][3] + view_projection[0][0],
            view_projection[1][3] + view_projection[1][0],
            view_projection[2][3] + view_projection[2][0]
        );
        frustum_planes[0].distance = view_projection[3][3] + view_projection[3][0];

        // Right plane
        frustum_planes[1].normal = glm::vec3(
            view_projection[0][3] - view_projection[0][0],
            view_projection[1][3] - view_projection[1][0],
            view_projection[2][3] - view_projection[2][0]
        );
        frustum_planes[1].distance = view_projection[3][3] - view_projection[3][0];

        // Similar for top, bottom, near, far...

        // Normalize planes
        for (auto& plane : frustum_planes) {
            float length = glm::length(plane.normal);
            plane.normal /= length;
            plane.distance /= length;
        }
    }

    bool is_sphere_visible(const glm::vec3& center, float radius) const {
        for (const auto& plane : frustum_planes) {
            if (plane.distance_to_point(center) < -radius) {
                return false;  // Sphere is outside this plane
            }
        }
        return true;
    }

    bool is_aabb_visible(const glm::vec3& min, const glm::vec3& max) const {
        for (const auto& plane : frustum_planes) {
            // Get positive vertex (farthest along plane normal)
            glm::vec3 p_vertex;
            p_vertex.x = (plane.normal.x >= 0) ? max.x : min.x;
            p_vertex.y = (plane.normal.y >= 0) ? max.y : min.y;
            p_vertex.z = (plane.normal.z >= 0) ? max.z : min.z;

            if (plane.distance_to_point(p_vertex) < 0) {
                return false;
            }
        }
        return true;
    }
};
```

### 8.2 Hardware Occlusion Queries

```cpp
class OcclusionCuller {
    std::vector<VkQueryPool> query_pools;

public:
    bool is_visible(
        VkCommandBuffer cmd,
        const Mobject& mobject,
        uint32_t query_index
    ) {
        // Reset query
        vkCmdResetQueryPool(cmd, query_pools[frame_index], query_index, 1);

        // Begin occlusion query
        vkCmdBeginQuery(cmd, query_pools[frame_index], query_index, 0);

        // Render bounding box (or simplified proxy geometry)
        render_bounding_box(cmd, mobject.get_bounds());

        // End query
        vkCmdEndQuery(cmd, query_pools[frame_index], query_index);

        // Get results (from previous frame to avoid stalling)
        uint64_t sample_count;
        VkResult result = vkGetQueryPoolResults(
            device,
            query_pools[previous_frame_index],
            query_index,
            1,
            sizeof(uint64_t),
            &sample_count,
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT
        );

        return sample_count > 0;  // Visible if any samples passed
    }
};
```

---

## Summary

This rendering pipeline provides:
- **PBR** for realistic materials
- **Ray tracing** for accurate lighting and reflections
- **Global illumination** for indirect lighting
- **Multiple shadow techniques** (CSM, VSM, RT)
- **Advanced post-processing** (bloom, TAA, tone mapping)
- **LOD system** for performance
- **Frustum & occlusion culling** for efficiency

**Next**: See `CONVERSION_CHECKLIST.md` for detailed conversion tracking.
