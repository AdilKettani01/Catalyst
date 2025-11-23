#version 450

// Deferred rendering G-buffer fragment shader
// Outputs to multiple render targets (MRT)

// Inputs from vertex shader
layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in vec4 in_color;
layout(location = 5) in vec4 in_curr_pos;
layout(location = 6) in vec4 in_prev_pos;

// G-Buffer outputs
layout(location = 0) out vec4 out_position;       // RGB: position, A: depth
layout(location = 1) out vec4 out_normal;         // RGB: normal, A: roughness
layout(location = 2) out vec4 out_albedo;         // RGB: albedo, A: metallic
layout(location = 3) out vec4 out_emission;       // RGB: emission, A: AO
layout(location = 4) out vec2 out_velocity;       // RG: motion vectors

// Material properties
layout(binding = 2) uniform sampler2D albedo_tex;
layout(binding = 3) uniform sampler2D normal_tex;
layout(binding = 4) uniform sampler2D metallic_roughness_tex;
layout(binding = 5) uniform sampler2D ao_tex;
layout(binding = 6) uniform sampler2D emission_tex;

layout(std140, binding = 7) uniform MaterialUBO {
    vec3 albedo_factor;
    float metallic_factor;
    float roughness_factor;
    float ao_factor;
    vec3 emission_factor;
    float _padding;
} material;

// Compute TBN matrix for normal mapping
mat3 compute_tbn() {
    vec3 N = normalize(in_normal);
    vec3 T = normalize(in_tangent);
    T = normalize(T - dot(T, N) * N);  // Gram-Schmidt orthogonalization
    vec3 B = cross(N, T);
    return mat3(T, B, N);
}

void main() {
    // Sample textures
    vec3 albedo = texture(albedo_tex, in_uv).rgb * material.albedo_factor * in_color.rgb;
    vec2 mr = texture(metallic_roughness_tex, in_uv).rg;
    float metallic = mr.r * material.metallic_factor;
    float roughness = mr.g * material.roughness_factor;
    float ao = texture(ao_tex, in_uv).r * material.ao_factor;
    vec3 emission = texture(emission_tex, in_uv).rgb * material.emission_factor;

    // Normal mapping
    vec3 tangent_normal = texture(normal_tex, in_uv).xyz * 2.0 - 1.0;
    mat3 TBN = compute_tbn();
    vec3 normal = normalize(TBN * tangent_normal);

    // Motion vectors (for TAA)
    vec2 curr_ndc = (in_curr_pos.xy / in_curr_pos.w) * 0.5 + 0.5;
    vec2 prev_ndc = (in_prev_pos.xy / in_prev_pos.w) * 0.5 + 0.5;
    vec2 velocity = curr_ndc - prev_ndc;

    // Write to G-Buffer
    out_position = vec4(in_world_pos, gl_FragCoord.z);
    out_normal = vec4(normal * 0.5 + 0.5, roughness);  // Pack normal [-1,1] to [0,1]
    out_albedo = vec4(albedo, metallic);
    out_emission = vec4(emission, ao);
    out_velocity = velocity;
}
