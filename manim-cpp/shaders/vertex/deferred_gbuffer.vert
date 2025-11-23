#version 450

// Deferred rendering G-buffer vertex shader

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in vec4 in_color;

// Output to fragment shader
layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec2 out_uv;
layout(location = 4) out vec4 out_color;
layout(location = 5) out vec4 out_curr_pos;  // Current frame position
layout(location = 6) out vec4 out_prev_pos;  // Previous frame position (for TAA)

// Uniforms
layout(std140, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    mat4 prev_view_projection;  // For motion vectors
    vec3 camera_position;
    float _padding0;
} camera;

layout(std140, binding = 1) uniform ObjectUBO {
    mat4 model;
    mat4 prev_model;  // Previous frame transform
    mat4 normal_matrix;
} object;

void main() {
    // Transform position to world space
    vec4 world_pos = object.model * vec4(in_position, 1.0);
    out_world_pos = world_pos.xyz;

    // Transform normal and tangent
    out_normal = mat3(object.normal_matrix) * in_normal;
    out_tangent = mat3(object.normal_matrix) * in_tangent;

    // Pass through UV and color
    out_uv = in_uv;
    out_color = in_color;

    // Compute current and previous clip space positions (for motion vectors)
    out_curr_pos = camera.view_projection * world_pos;
    out_prev_pos = camera.prev_view_projection * (object.prev_model * vec4(in_position, 1.0));

    // Output clip space position
    gl_Position = out_curr_pos;
}
