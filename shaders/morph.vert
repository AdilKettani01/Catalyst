#version 450

layout(location = 0) in vec2 inSourcePos;
layout(location = 1) in vec2 inTargetPos;

// Push constants for morph animation
layout(push_constant) uniform MorphPushConstants {
    float morphProgress;  // 0.0 = source, 1.0 = target
    float scale;
    float rotation;
    float offsetX;
    float offsetY;
} vpc;

// Camera UBO for view-projection matrix
layout(set = 1, binding = 0) uniform CameraUBO {
    mat4 viewProjection;
} camera;

void main() {
    // Interpolate position based on morph progress
    vec2 pos = mix(inSourcePos, inTargetPos, vpc.morphProgress);

    // Apply rotation around origin
    float c = cos(vpc.rotation);
    float s = sin(vpc.rotation);
    vec2 rotated = vec2(
        pos.x * c - pos.y * s,
        pos.x * s + pos.y * c
    );

    // Apply scale
    vec2 scaled = rotated * vpc.scale;

    // Apply translation offset
    vec2 finalPos = scaled + vec2(vpc.offsetX, vpc.offsetY);

    // Apply camera transform
    gl_Position = camera.viewProjection * vec4(finalPos, 0.0, 1.0);
}
