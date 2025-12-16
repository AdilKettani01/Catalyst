#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 fragPos;

// Push constants for animation transforms
layout(push_constant) uniform VertexPushConstants {
    float offsetX;
    float offsetY;
    float scale;
    float rotation;
} vpc;

// Camera UBO for view-projection matrix
layout(set = 1, binding = 0) uniform CameraUBO {
    mat4 viewProjection;
} camera;

void main() {
    // Apply rotation around origin
    float c = cos(vpc.rotation);
    float s = sin(vpc.rotation);
    vec2 rotated = vec2(
        inPosition.x * c - inPosition.y * s,
        inPosition.x * s + inPosition.y * c
    );

    // Apply scale
    vec2 scaled = rotated * vpc.scale;

    // Apply translation offset
    vec2 finalPos = scaled + vec2(vpc.offsetX, vpc.offsetY);

    // Apply camera transform
    gl_Position = camera.viewProjection * vec4(finalPos, 0.0, 1.0);
    fragPos = inPosition;
}
