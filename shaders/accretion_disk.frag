#version 410 core

// ──────────────────────────────────────────────────────────────────────────────
//  Accretion disk fragment shader.
//
//  The disk is drawn with additive blending on top of the black hole body.
//  Temperature decreases outward (Wien's displacement law analogy):
//    inner → white/blue-white, middle → yellow-orange, outer → deep red.
// ──────────────────────────────────────────────────────────────────────────────

in float v_t;
in float v_radial;

uniform float u_time;

out vec4 fragColor;

// Blackbody-inspired colour ramp: 0=inner (hot) → 1=outer (cool)
vec3 diskColor(float t) {
    vec3 hot   = vec3(1.0,  0.95, 0.85);  // near-white hot inner edge
    vec3 mid   = vec3(1.0,  0.55, 0.10);  // orange
    vec3 cool  = vec3(0.6,  0.10, 0.02);  // deep red outer edge
    if (t < 0.5)
        return mix(hot, mid, t * 2.0);
    return mix(mid, cool, (t - 0.5) * 2.0);
}

float hash(float n) { return fract(sin(n) * 43758.5453123); }

void main() {
    vec3  col   = diskColor(v_radial);

    // Brightness: inner edge much brighter than outer
    float bright = mix(1.2, 0.25, v_radial);

    // Azimuthal flickering hotspots to animate the plasma
    float phase   = v_t * 6.2832 + u_time * 1.2;
    float flicker = 0.8 + 0.4 * sin(phase * 3.0) * sin(phase * 7.1 + 1.3);

    // Thin the ring toward inner/outer edges
    float edge    = smoothstep(0.0, 0.15, v_radial) *
                    smoothstep(1.0, 0.75, v_radial);

    float alpha   = bright * flicker * edge * 0.85;

    fragColor = vec4(col * bright, alpha);
}
