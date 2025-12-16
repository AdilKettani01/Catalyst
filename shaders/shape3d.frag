#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

// Push constants (offset 64 bytes after model matrix)
layout(push_constant) uniform PushConstants {
    layout(offset = 64) float opacity;
    float colorR;
    float colorG;
    float colorB;
} pc;

// Lighting UBO (set 2)
layout(set = 2, binding = 0) uniform LightingUBO {
    // Ambient light
    vec4 ambientColor;      // xyz = color, w = intensity

    // Directional light
    vec4 dirLightDirection; // xyz = direction (normalized), w = enabled (1.0 or 0.0)
    vec4 dirLightColor;     // xyz = color, w = intensity

    // Point lights (up to 4)
    vec4 pointLight0Pos;    // xyz = position, w = enabled
    vec4 pointLight0Color;  // xyz = color, w = intensity
    vec4 pointLight0Atten;  // x = constant, y = linear, z = quadratic, w = unused

    vec4 pointLight1Pos;
    vec4 pointLight1Color;
    vec4 pointLight1Atten;

    vec4 pointLight2Pos;
    vec4 pointLight2Color;
    vec4 pointLight2Atten;

    vec4 pointLight3Pos;
    vec4 pointLight3Color;
    vec4 pointLight3Atten;

    // Camera position for specular
    vec4 cameraPos;         // xyz = camera position, w = unused

    // Material properties
    vec4 materialProps;     // x = shininess, y = specular strength, z = unused, w = unused
} lighting;

vec3 calcPointLight(vec4 lightPos, vec4 lightColor, vec4 lightAtten, vec3 normal, vec3 fragPos, vec3 baseColor, vec3 viewDir) {
    if (lightPos.w < 0.5) return vec3(0.0);  // Light disabled

    vec3 lightDir = normalize(lightPos.xyz - fragPos);
    float distance = length(lightPos.xyz - fragPos);

    // Attenuation
    float attenuation = 1.0 / (lightAtten.x + lightAtten.y * distance + lightAtten.z * distance * distance);

    // Diffuse (two-sided)
    float diff = abs(dot(normal, lightDir));
    vec3 diffuse = diff * lightColor.xyz * lightColor.w * baseColor;

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(abs(dot(normal, halfwayDir)), 0.0), lighting.materialProps.x);
    vec3 specular = lighting.materialProps.y * spec * lightColor.xyz * lightColor.w;

    return (diffuse + specular) * attenuation;
}

void main() {
    vec3 baseColor = vec3(pc.colorR, pc.colorG, pc.colorB);
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(lighting.cameraPos.xyz - fragPos);

    // Ambient component
    vec3 ambient = lighting.ambientColor.xyz * lighting.ambientColor.w * baseColor;

    // Directional light
    vec3 dirLight = vec3(0.0);
    if (lighting.dirLightDirection.w > 0.5) {  // Light enabled
        vec3 lightDir = normalize(lighting.dirLightDirection.xyz);
        float diff = abs(dot(normal, lightDir));  // Two-sided lighting
        dirLight = diff * lighting.dirLightColor.xyz * lighting.dirLightColor.w * baseColor;

        // Specular for directional light
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(abs(dot(normal, halfwayDir)), 0.0), lighting.materialProps.x);
        dirLight += lighting.materialProps.y * spec * lighting.dirLightColor.xyz * lighting.dirLightColor.w;
    }

    // Point lights
    vec3 pointLights = vec3(0.0);
    pointLights += calcPointLight(lighting.pointLight0Pos, lighting.pointLight0Color, lighting.pointLight0Atten, normal, fragPos, baseColor, viewDir);
    pointLights += calcPointLight(lighting.pointLight1Pos, lighting.pointLight1Color, lighting.pointLight1Atten, normal, fragPos, baseColor, viewDir);
    pointLights += calcPointLight(lighting.pointLight2Pos, lighting.pointLight2Color, lighting.pointLight2Atten, normal, fragPos, baseColor, viewDir);
    pointLights += calcPointLight(lighting.pointLight3Pos, lighting.pointLight3Color, lighting.pointLight3Atten, normal, fragPos, baseColor, viewDir);

    // Final color
    vec3 result = ambient + dirLight + pointLights;
    outColor = vec4(result, pc.opacity);
}
