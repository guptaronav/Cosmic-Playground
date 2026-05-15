#version 410 core

in vec2 v_uv;

uniform vec4  u_color;
uniform float u_radius;   // visual radius in world units
uniform float u_glow;     // glow multiplier (>= 1)
uniform bool  u_isStar;   // stars get a brighter core

out vec4 fragColor;

void main() {
    // Distance from centre in UV space (range 0..1 maps to centre..edge of quad)
    vec2  centred = v_uv * 2.0 - 1.0;  // [-1, 1]
    float dist    = length(centred);

    // Hard edge at r = 1/glow (the physical surface)
    float surfaceR = 1.0 / u_glow;

    // Core disk – solid body colour
    float core = smoothstep(surfaceR + 0.02, surfaceR - 0.02, dist);

    // Glow halo – additive soft glow extending beyond the surface
    float falloff = 1.0 - smoothstep(0.0, 1.0, dist / surfaceR);
    float halo    = pow(falloff, 3.0) * 0.6;

    // Stars get a sharper, brighter corona
    float corona = 0.0;
    if (u_isStar) {
        corona = pow(max(0.0, 1.0 - dist / surfaceR), 5.0) * 1.2;
    }

    float alpha  = clamp(core + halo + corona, 0.0, 1.0);
    vec3  colour = u_color.rgb * (1.0 + corona * 0.5);

    fragColor = vec4(colour, alpha * u_color.a);
}
