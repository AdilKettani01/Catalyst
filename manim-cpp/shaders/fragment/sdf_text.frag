#version 450

// SDF Text Fragment Shader
// High-quality text rendering using signed distance fields
// Supports: anti-aliasing, outlines, glow, shadow

// Input from vertex shader
layout(location = 0) in vec2 frag_uv;
layout(location = 1) in vec4 frag_color;
layout(location = 2) in vec4 frag_effects;   // x = outline_width, y = glow_intensity, z = shadow_offset, w = shadow_blur
layout(location = 3) in float frag_sdf_range;

// Output
layout(location = 0) out vec4 out_color;

// SDF atlas texture
layout(set = 0, binding = 1) uniform sampler2D sdf_atlas;

// Additional uniforms for effects
layout(set = 0, binding = 2) uniform EffectsBuffer {
    vec4 outline_color;
    vec4 glow_color;
    vec4 shadow_color;
    vec2 shadow_offset;
    float smoothing;          // Base smoothing factor
    float gamma;              // Gamma correction
} effects;

// Sample SDF value and convert to alpha
float sample_sdf(vec2 uv, float threshold) {
    float distance = texture(sdf_atlas, uv).r;

    // Compute smoothing based on screen-space derivatives
    float smoothing_factor = frag_sdf_range * fwidth(distance);
    smoothing_factor = max(smoothing_factor, effects.smoothing);

    // Smooth threshold
    return smoothstep(threshold - smoothing_factor, threshold + smoothing_factor, distance);
}

// Multi-sample anti-aliasing for higher quality
float sample_sdf_msaa(vec2 uv, float threshold) {
    // 4-sample pattern
    vec2 dx = dFdx(uv) * 0.25;
    vec2 dy = dFdy(uv) * 0.25;

    float alpha = 0.0;
    alpha += sample_sdf(uv + dx + dy, threshold);
    alpha += sample_sdf(uv + dx - dy, threshold);
    alpha += sample_sdf(uv - dx + dy, threshold);
    alpha += sample_sdf(uv - dx - dy, threshold);

    return alpha * 0.25;
}

// Generate outline
vec4 apply_outline(vec2 uv, float base_alpha, float outline_width) {
    if (outline_width <= 0.0) {
        return vec4(frag_color.rgb, base_alpha * frag_color.a);
    }

    // Sample at inner and outer thresholds
    float inner_threshold = 0.5;
    float outer_threshold = 0.5 - outline_width * frag_sdf_range;

    float inner_alpha = sample_sdf(uv, inner_threshold);
    float outer_alpha = sample_sdf(uv, outer_threshold);

    // Blend between outline and fill
    vec3 color = mix(effects.outline_color.rgb, frag_color.rgb, inner_alpha);
    float alpha = outer_alpha * frag_color.a;

    return vec4(color, alpha);
}

// Generate glow effect
vec4 apply_glow(vec2 uv, vec4 base_color, float glow_intensity) {
    if (glow_intensity <= 0.0) {
        return base_color;
    }

    // Sample SDF at wider threshold for glow
    float glow_threshold = 0.5 - glow_intensity * frag_sdf_range * 2.0;
    float glow_alpha = sample_sdf(uv, glow_threshold);

    // Blend glow under the text
    vec4 glow = vec4(effects.glow_color.rgb, glow_alpha * effects.glow_color.a * glow_intensity);

    // Composite: glow behind text
    return vec4(
        mix(glow.rgb, base_color.rgb, base_color.a),
        max(glow.a, base_color.a)
    );
}

// Generate drop shadow
vec4 apply_shadow(vec2 uv, vec4 base_color, float shadow_offset, float shadow_blur) {
    if (shadow_offset <= 0.0) {
        return base_color;
    }

    // Offset UV for shadow
    vec2 shadow_uv = uv - effects.shadow_offset * frag_sdf_range;

    // Sample shadow (blurred)
    float shadow_threshold = 0.5 - shadow_blur * frag_sdf_range;
    float shadow_alpha = sample_sdf(shadow_uv, shadow_threshold);

    // Shadow color
    vec4 shadow = vec4(effects.shadow_color.rgb, shadow_alpha * effects.shadow_color.a);

    // Composite: shadow behind text
    return vec4(
        mix(shadow.rgb, base_color.rgb, base_color.a),
        max(shadow.a * (1.0 - base_color.a), base_color.a)
    );
}

void main() {
    // Extract effects parameters
    float outline_width = frag_effects.x;
    float glow_intensity = frag_effects.y;
    float shadow_offset = frag_effects.z;
    float shadow_blur = frag_effects.w;

    // Sample base SDF
    float base_alpha = sample_sdf(frag_uv, 0.5);

    // Apply outline
    vec4 color = apply_outline(frag_uv, base_alpha, outline_width);

    // Apply glow
    color = apply_glow(frag_uv, color, glow_intensity);

    // Apply shadow
    color = apply_shadow(frag_uv, color, shadow_offset, shadow_blur);

    // Gamma correction
    color.rgb = pow(color.rgb, vec3(1.0 / effects.gamma));

    // Discard fully transparent pixels
    if (color.a < 0.001) {
        discard;
    }

    out_color = color;
}
