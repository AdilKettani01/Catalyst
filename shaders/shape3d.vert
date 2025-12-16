#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPos;

// Push constants for model matrix (64 bytes)
layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

// 3D Camera UBO (view + projection matrices)
layout(set = 1, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

void main() {
    // Transform position to world space
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;

    // Transform normal to world space (using normal matrix)
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);

    // Transform to clip space
    gl_Position = camera.projection * camera.view * worldPos;
}
