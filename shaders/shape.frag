#version 450

layout(location = 0) in vec2 fragPos;

layout(location = 0) out vec4 outColor;

// Push constants (offset 16 bytes to skip vertex stage's offsetX/offsetY/scale/rotation)
layout(push_constant) uniform PushConstants {
    layout(offset = 16) float opacity;
    float fillR;
    float fillG;
    float fillB;
    float strokeWidth;
    float strokeR;
    float strokeG;
    float strokeB;
} pc;

void main() {
    // Simple solid color fill with opacity
    outColor = vec4(pc.fillR, pc.fillG, pc.fillB, pc.opacity);
}
