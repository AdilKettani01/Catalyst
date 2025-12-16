#version 450

layout(location = 0) out vec4 outColor;

// Push constants (offset 20 bytes to skip vertex stage's morphProgress/scale/rotation/offsetX/offsetY)
layout(push_constant) uniform PushConstants {
    layout(offset = 20) float opacity;
    float sourceR;
    float sourceG;
    float sourceB;
    float targetR;
    float targetG;
    float targetB;
    float morphProgress;
} pc;

void main() {
    // Interpolate color based on morph progress
    vec3 color = mix(
        vec3(pc.sourceR, pc.sourceG, pc.sourceB),
        vec3(pc.targetR, pc.targetG, pc.targetB),
        pc.morphProgress
    );

    outColor = vec4(color, pc.opacity);
}
