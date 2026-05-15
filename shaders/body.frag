#version 410 core

in vec2 v_uv;

uniform vec4  u_color;
uniform float u_radius;
uniform float u_glow;
uniform int   u_isStar;   // 1 = star/neutron star, 0 = planet

out vec4 fragColor;

void main() {
    // Map UV [0,1] to [-1,1] and compute radial distance from centre
    vec2  centred = v_uv * 2.0 - 1.0;
    float dist    = length(centred);

    // Physical surface sits at 1/glow fraction of the quad radius
    float surfaceR = 1.0 / u_glow;

    // Solid core disk
    float core = smoothstep(surfaceR + 0.02, surfaceR - 0.02, dist);

    // Soft glow halo fading outward from the surface
    float falloff = 1.0 - smoothstep(0.0, 1.0, dist / surfaceR);
    float halo    = pow(falloff, 3.0) * 0.6;

    // Stars get a sharper, brighter corona
    float corona = 0.0;
    if (u_isStar != 0) {
        corona = pow(max(0.0, 1.0 - dist / surfaceR), 5.0) * 1.2;
    }

    float alpha  = clamp(core + halo + corona, 0.0, 1.0);
    vec3  colour = u_color.rgb * (1.0 + corona * 0.5);

    fragColor = vec4(colour, alpha * u_color.a);
}
