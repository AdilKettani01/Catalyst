#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec4 color;
} pc;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    outColor = pc.color;  // Pass full vec4 with alpha
}
