#version 450

// Deferred lighting pass - PBR lighting from G-buffer

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

// G-Buffer textures
layout(binding = 0) uniform sampler2D g_position;
layout(binding = 1) uniform sampler2D g_normal;
layout(binding = 2) uniform sampler2D g_albedo;
layout(binding = 3) uniform sampler2D g_emission;

// Shadow map
layout(binding = 4) uniform sampler2DArray shadow_map;  // Cascaded shadow maps

// Environment maps
layout(binding = 5) uniform samplerCube irradiance_map;
layout(binding = 6) uniform samplerCube prefiltered_map;
layout(binding = 7) uniform sampler2D brdf_lut;

// Lighting data
const int MAX_LIGHTS = 16;

struct Light {
    vec4 position;      // w: type (0=directional, 1=point, 2=spot)
    vec4 direction;     // w: spot angle
    vec4 color;         // w: intensity
    vec4 params;        // x: range, y: attenuation, z,w: unused
};

layout(std140, binding = 8) uniform LightsUBO {
    Light lights[MAX_LIGHTS];
    int num_lights;
    vec3 ambient;
} lights;

layout(std140, binding = 9) uniform CameraUBO {
    mat4 view_projection;
    vec3 position;
} camera;

// Shadow cascade data
layout(std140, binding = 10) uniform ShadowUBO {
    mat4 cascade_view_proj[4];
    vec4 cascade_splits;  // xyz: split distances, w: num cascades
} shadows;

const float PI = 3.14159265359;

// ============================================================================
// PBR Functions
// ============================================================================

// Fresnel-Schlick approximation
vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

// GGX/Trowbridge-Reitz normal distribution
float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Schlick-GGX geometry function
float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

// ============================================================================
// Shadow Mapping
// ============================================================================

float sample_shadow(vec3 world_pos, vec3 normal, vec3 light_dir) {
    // Determine cascade
    float depth = length(camera.position - world_pos);
    int cascade_idx = 0;
    for (int i = 0; i < int(shadows.cascade_splits.w); ++i) {
        if (depth < shadows.cascade_splits[i]) {
            cascade_idx = i;
            break;
        }
    }

    // Transform to shadow space
    vec4 shadow_coord = shadows.cascade_view_proj[cascade_idx] * vec4(world_pos, 1.0);
    shadow_coord.xyz /= shadow_coord.w;
    shadow_coord.xyz = shadow_coord.xyz * 0.5 + 0.5;

    if (shadow_coord.z > 1.0) return 1.0;

    // Bias based on slope
    float bias = max(0.005 * (1.0 - dot(normal, light_dir)), 0.001);

    // PCF (Percentage-Closer Filtering)
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0).xy;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texel_size;
            float pcf_depth = texture(shadow_map, vec3(shadow_coord.xy + offset, cascade_idx)).r;
            shadow += (shadow_coord.z - bias) > pcf_depth ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// ============================================================================
// Main Lighting
// ============================================================================

void main() {
    // Read G-Buffer
    vec4 g_pos = texture(g_position, in_uv);
    vec3 position = g_pos.xyz;
    vec4 g_norm = texture(g_normal, in_uv);
    vec3 normal = normalize(g_norm.xyz * 2.0 - 1.0);
    float roughness = g_norm.a;

    vec4 g_alb = texture(g_albedo, in_uv);
    vec3 albedo = g_alb.rgb;
    float metallic = g_alb.a;

    vec4 g_emis = texture(g_emission, in_uv);
    vec3 emission = g_emis.rgb;
    float ao = g_emis.a;

    // Skip skybox
    if (g_pos.w >= 0.9999) {
        out_color = vec4(emission, 1.0);
        return;
    }

    // View direction
    vec3 V = normalize(camera.position - position);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Direct lighting
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < lights.num_lights; ++i) {
        Light light = lights.lights[i];

        // Light direction and attenuation
        vec3 L;
        float attenuation = 1.0;

        if (light.position.w == 0.0) {
            // Directional light
            L = normalize(-light.direction.xyz);
        } else {
            // Point/spot light
            vec3 light_vec = light.position.xyz - position;
            float distance = length(light_vec);
            L = light_vec / distance;

            // Attenuation
            attenuation = 1.0 / (1.0 + light.params.y * distance * distance);

            // Spot light cone
            if (light.position.w == 2.0) {
                float theta = dot(L, normalize(-light.direction.xyz));
                float epsilon = light.direction.w;
                attenuation *= clamp((theta - epsilon) / (1.0 - epsilon), 0.0, 1.0);
            }
        }

        vec3 H = normalize(V + L);

        // Radiance
        vec3 radiance = light.color.rgb * light.color.a * attenuation;

        // Cook-Torrance BRDF
        float NDF = distribution_ggx(normal, H, roughness);
        float G = geometry_smith(normal, V, L, roughness);
        vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        // Energy conservation
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(normal, L), 0.0);

        // Shadow
        float shadow = 1.0;
        if (light.position.w == 0.0) {  // Only directional lights cast shadows
            shadow = sample_shadow(position, normal, L);
        }

        Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadow;
    }

    // Ambient lighting (IBL)
    vec3 F = fresnel_schlick_roughness(max(dot(normal, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradiance_map, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    // Specular IBL
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 R = reflect(-V, normal);
    vec3 prefilteredColor = textureLod(prefiltered_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdf_lut, vec2(max(dot(normal, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;

    // Final color
    vec3 color = ambient + Lo + emission;

    out_color = vec4(color, 1.0);
}
