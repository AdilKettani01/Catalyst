#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragCharIndex;  // Character index from vertex shader

layout(binding = 0) uniform sampler2D fontTexture;

// Push constants (offset 16 bytes to skip vertex stage's offsetX/offsetY/scale/rotation)
layout(push_constant) uniform PushConstants {
    layout(offset = 16) float opacity;
    float colorR;
    float colorG;
    float colorB;
    float strokeWidth;
    float strokeR;
    float strokeG;
    float strokeB;
    float gradEndR;
    float gradEndG;
    float gradEndB;
    float gradientAngle;  // In radians, 0 = no gradient
    // New fields for per-character and stroke/fill animations
    float charRevealProgress;  // 0.0 = no chars visible, 1.0 = all visible
    float totalCharCount;      // Total number of characters
    float strokeOpacity;       // Independent stroke opacity (0.0-1.0)
    float fillOpacity;         // Independent fill opacity (0.0-1.0)
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample the SDF texture (distance field value)
    float dist = texture(fontTexture, fragTexCoord).r;

    // SDF parameters
    // For SDF: values > 0.5 are inside the glyph, < 0.5 are outside
    // onedge_value was 128, so 0.5 in normalized range is the edge
    float edgeCenter = 0.5;
    float edgeWidth = 0.07;  // Anti-aliasing width (adjust for smoothness)

    // Calculate per-character visibility for Write/LetterByLetter animations
    float charVisibility = 1.0;
    if (pc.totalCharCount > 0.0 && pc.charRevealProgress < 1.0) {
        // Calculate smooth per-character reveal
        // Each character fades in as charRevealProgress passes its threshold
        float charThreshold = fragCharIndex / pc.totalCharCount;
        float charProgress = clamp((pc.charRevealProgress - charThreshold) * pc.totalCharCount, 0.0, 1.0);
        charVisibility = charProgress;
    }

    // Fill alpha: smoothstep from edge inward
    float fillAlpha = smoothstep(edgeCenter - edgeWidth, edgeCenter + edgeWidth, dist);

    // Calculate fill color (with gradient support)
    vec3 fillColor;
    if (abs(pc.gradientAngle) > 0.001) {
        // Gradient enabled - interpolate along angle direction
        float cosA = cos(pc.gradientAngle);
        float sinA = sin(pc.gradientAngle);
        float t = fragTexCoord.x * cosA + fragTexCoord.y * sinA;
        t = clamp(t, 0.0, 1.0);

        vec3 startColor = vec3(pc.colorR, pc.colorG, pc.colorB);
        vec3 endColor = vec3(pc.gradEndR, pc.gradEndG, pc.gradEndB);
        fillColor = mix(startColor, endColor, t);
    } else {
        fillColor = vec3(pc.colorR, pc.colorG, pc.colorB);
    }

    // Stroke calculation
    float strokeAlpha = 0.0;
    if (pc.strokeWidth > 0.0) {
        // Stroke extends outward from edge
        // strokeWidth is in pixels, convert to SDF space (roughly)
        float strokeEdge = edgeCenter - pc.strokeWidth * 0.015;
        strokeAlpha = smoothstep(strokeEdge - edgeWidth, strokeEdge + edgeWidth, dist);
    }

    // Combine fill and stroke with separate opacities
    vec3 strokeColor = vec3(pc.strokeR, pc.strokeG, pc.strokeB);

    // Apply separate stroke/fill opacities and character visibility
    float effectiveFillAlpha = fillAlpha * pc.fillOpacity * charVisibility;
    float effectiveStrokeAlpha = strokeAlpha * pc.strokeOpacity * charVisibility;

    // Mix colors: fill on top of stroke
    vec3 finalColor;
    float finalAlpha;

    if (pc.strokeWidth > 0.0 && effectiveStrokeAlpha > 0.0) {
        // Blend fill over stroke using proper alpha compositing
        float blendFactor = effectiveFillAlpha / max(effectiveStrokeAlpha, 0.001);
        blendFactor = clamp(blendFactor, 0.0, 1.0);
        finalColor = mix(strokeColor, fillColor, blendFactor);
        finalAlpha = max(effectiveFillAlpha, effectiveStrokeAlpha);
    } else {
        finalColor = fillColor;
        finalAlpha = effectiveFillAlpha;
    }

    // Apply overall opacity
    finalAlpha *= pc.opacity;

    outColor = vec4(finalColor, finalAlpha);
}
