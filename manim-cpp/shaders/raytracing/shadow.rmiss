#version 460
#extension GL_EXT_ray_tracing : require

// Miss shader for shadow rays
// Called when shadow ray doesn't hit anything (fully lit)

layout(location = 0) rayPayloadInEXT struct {
    float shadow_factor;  // 0 = fully shadowed, 1 = fully lit
    float distance;       // Distance to occluder
} shadow_payload;

void main() {
    // No occlusion - fully lit
    shadow_payload.shadow_factor = 1.0;
    shadow_payload.distance = gl_RayTmaxEXT;
}
