#version 450

// Geomorph vertex shader for smooth LOD transitions
// Interpolates between current and target LOD positions based on morph factor

// Per-vertex attributes (GeomorphVertex - 128 bytes)
layout(location = 0) in vec3 in_position;        // Current LOD position
layout(location = 1) in vec3 in_morph_target;    // Target LOD position
layout(location = 2) in vec3 in_normal;          // Current LOD normal
layout(location = 3) in vec3 in_morph_normal;    // Target LOD normal
layout(location = 4) in vec2 in_uv;
layout(location = 5) in vec4 in_tangent;

// Per-instance attributes (optional, for instanced rendering)
layout(location = 6) in mat4 in_model_matrix;    // Uses locations 6-9
layout(location = 10) in vec4 in_instance_color;
layout(location = 11) in float in_metallic;
layout(location = 12) in float in_roughness;
layout(location = 13) in float in_ao;

// Uniforms
layout(push_constant) uniform PushConstants {
    mat4 view_projection;
    float morph_factor;     // 0 = from LOD, 1 = to LOD
    float time;
    vec2 padding;
} pc;

// Output to fragment shader
layout(location = 0) out vec3 frag_world_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out vec4 frag_tangent;
layout(location = 4) out vec4 frag_color;
layout(location = 5) out float frag_metallic;
layout(location = 6) out float frag_roughness;
layout(location = 7) out float frag_ao;

void main() {
    // Interpolate between LOD levels based on morph factor
    vec3 morphed_position = mix(in_position, in_morph_target, pc.morph_factor);
    vec3 morphed_normal = normalize(mix(in_normal, in_morph_normal, pc.morph_factor));

    // Apply instance transform (or identity if not instanced)
    mat4 model = in_model_matrix;
    mat3 normal_matrix = transpose(inverse(mat3(model)));

    // Transform to world space
    vec4 world_pos = model * vec4(morphed_position, 1.0);
    frag_world_pos = world_pos.xyz;
    frag_normal = normalize(normal_matrix * morphed_normal);

    // Pass through other attributes
    frag_uv = in_uv;
    frag_tangent = vec4(normalize(normal_matrix * in_tangent.xyz), in_tangent.w);
    frag_color = in_instance_color;
    frag_metallic = in_metallic;
    frag_roughness = in_roughness;
    frag_ao = in_ao;

    // Final position
    gl_Position = pc.view_projection * world_pos;
}
