#version 450

// SDF Text Vertex Shader
// Instanced rendering of SDF glyphs for high-performance text

// Per-vertex data (quad corners)
layout(location = 0) in vec2 in_position;  // Quad corner position (0,0), (1,0), (1,1), (0,1)
layout(location = 1) in vec2 in_uv;        // Quad corner UV

// Per-instance data (one per glyph)
layout(location = 2) in vec4 instance_pos_size;  // xy = position, zw = size
layout(location = 3) in vec4 instance_uv_rect;   // UV rectangle in atlas (x, y, w, h)
layout(location = 4) in vec4 instance_color;     // RGBA color
layout(location = 5) in vec4 instance_effects;   // x = outline_width, y = glow, z = shadow, w = reserved

// Uniforms
layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 projection;
    mat4 view;
    mat4 model;
    vec2 atlas_size;          // Atlas texture dimensions
    float sdf_range;          // SDF distance range in pixels
    float time;               // Animation time
} ubo;

// Output to fragment shader
layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec4 frag_color;
layout(location = 2) out vec4 frag_effects;
layout(location = 3) out float frag_sdf_range;

void main() {
    // Compute world position
    vec2 local_pos = in_position * instance_pos_size.zw + instance_pos_size.xy;
    vec4 world_pos = ubo.model * vec4(local_pos, 0.0, 1.0);

    // Apply view and projection
    gl_Position = ubo.projection * ubo.view * world_pos;

    // Compute UV in atlas
    vec2 uv_min = instance_uv_rect.xy;
    vec2 uv_size = instance_uv_rect.zw;
    frag_uv = uv_min + in_uv * uv_size;

    // Pass through color and effects
    frag_color = instance_color;
    frag_effects = instance_effects;

    // SDF range normalized to glyph size
    frag_sdf_range = ubo.sdf_range / max(instance_pos_size.z, instance_pos_size.w);
}
