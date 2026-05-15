#version 410 core

// Accretion disk fragment shader.
//
// Temperature decreases outward (Wien displacement law analogy):
//   inner edge  -- near-white hot
//   middle ring -- orange
//   outer edge  -- deep red
//
// Drawn with additive blending to simulate self-luminous plasma.

in float v_t;
in float v_radial;

uniform float u_time;

out vec4 fragColor;

// Blackbody-inspired colour ramp: t=0 inner (hot) to t=1 outer (cool)
vec3 diskColor(float t) {
    vec3 hot  = vec3(1.0,  0.95, 0.85);
    vec3 mid  = vec3(1.0,  0.55, 0.10);
    vec3 cool = vec3(0.6,  0.10, 0.02);
    if (t < 0.5)
        return mix(hot, mid, t * 2.0);
    return mix(mid, cool, (t - 0.5) * 2.0);
}

float hash(float n) { return fract(sin(n) * 43758.5453123); }

void main() {
    vec3  col    = diskColor(v_radial);
    float bright = mix(1.2, 0.25, v_radial);

    // Azimuthal flickering hotspots
    float phase   = v_t * 6.2832 + u_time * 1.2;
    float flicker = 0.8 + 0.4 * sin(phase * 3.0) * sin(phase * 7.1 + 1.3);

    // Thin the ring toward inner and outer edges
    float edge  = smoothstep(0.0, 0.15, v_radial) *
                  smoothstep(1.0, 0.75, v_radial);

    float alpha = bright * flicker * edge * 0.85;
    fragColor   = vec4(col * bright, alpha);
}
