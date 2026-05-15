#version 410 core

// Black hole rendering shader.
//
// Layers rendered inside-out:
//   1. Event horizon  -- absolute black disk
//   2. Photon sphere  -- thin bright glowing ring just outside the horizon
//   3. Lensing halo   -- bent starlight approximation (procedural gradient)
//   4. Outer glow     -- dim ambient halo

in vec2  v_uv;
in vec2  v_local;

uniform float u_radius;
uniform float u_time;
uniform vec2  u_resolution;

out vec4 fragColor;

// Pseudo-random helpers for hot-spot animation
float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 17.5);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i),             hash(i + vec2(1,0)), f.x),
               mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x), f.y);
}

void main() {
    float r        = length(v_local);
    float horizonR = 1.0 / 3.0;  // event horizon at 1/3 of the quad half-width

    // 1. Solid black event horizon interior
    if (r < horizonR * 0.92) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 2. Photon sphere -- glowing ring at the horizon boundary
    float ringW    = 0.07;
    float ringDist = abs(r - horizonR);
    float ring     = smoothstep(ringW, 0.0, ringDist);
    float flicker  = 0.85 + 0.15 * sin(u_time * 2.3 + r * 30.0);
    vec3  ringCol  = vec3(1.0, 0.75, 0.35) * ring * flicker;

    // 3. Lensing glow -- inverse-square falloff with plasma swirl noise
    float lensR    = max(0.0, r - horizonR);
    float lensFade = exp(-lensR * 8.0);
    float angle    = atan(v_local.y, v_local.x);
    float swirl    = noise(vec2(angle * 2.0 + u_time * 0.4, r * 6.0));
    vec3  lensCol  = vec3(0.8, 0.45, 0.15) * lensFade * (0.6 + 0.4 * swirl);

    // 4. Outer halo
    float halo    = pow(1.0 - smoothstep(0.0, 1.0, r), 3.0) * 0.25;
    vec3  haloCol = vec3(0.3, 0.15, 0.5) * halo;

    // Composite all layers
    vec3  col  = ringCol + lensCol + haloCol;
    float a    = clamp(ring + lensFade * 0.8 + halo, 0.0, 1.0);

    // Mask out the solid interior cleanly
    float mask = smoothstep(horizonR * 0.92, horizonR * 0.95, r);
    col  *= mask;
    a    *= mask;

    fragColor = vec4(col, a);
}
