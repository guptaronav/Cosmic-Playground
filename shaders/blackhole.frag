#version 410 core

// ──────────────────────────────────────────────────────────────────────────────
//  Black hole rendering shader.
//
//  Layers (inside-out):
//    1. Event horizon – absolute black disk
//    2. Photon sphere glow – thin bright ring just outside the horizon
//    3. Gravitational lensing approximation – radial UV distortion bends
//       the "star background" texture (here we fake it with a procedural
//       gradient that mimics light bent around the singularity)
//    4. Outer glow – dimly lit halo
// ──────────────────────────────────────────────────────────────────────────────

in vec2  v_uv;
in vec2  v_local;

uniform float u_radius;
uniform float u_time;
uniform vec2  u_resolution;

out vec4 fragColor;

// ── Pseudo-random helpers ──────────────────────────────────────────────────────
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
    // Local radius (the quad spans -1..1; event horizon at r=1/3 of quad)
    float r        = length(v_local);
    float horizonR = 1.0 / 3.0;  // fraction of quad radius

    // ── 1. Absolute black inside the event horizon ─────────────────────────────
    if (r < horizonR * 0.92) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // ── 2. Photon sphere – bright glowing ring ─────────────────────────────────
    float ringR    = horizonR;
    float ringW    = 0.07;
    float ringDist = abs(r - ringR);
    float ring     = smoothstep(ringW, 0.0, ringDist);

    // Animate the ring with a slow flicker
    float flicker  = 0.85 + 0.15 * sin(u_time * 2.3 + r * 30.0);
    vec3  ringCol  = vec3(1.0, 0.75, 0.35) * ring * flicker;

    // ── 3. Lensing glow – bent starlight approximation ─────────────────────────
    // Inverse square brightness falloff from ring outward
    float lensR    = max(0.0, r - ringR);
    float lensFade = exp(-lensR * 8.0);

    // Add swirl noise to simulate hot plasma / bent light
    float angle    = atan(v_local.y, v_local.x);
    float swirlT   = u_time * 0.4;
    float swirl    = noise(vec2(angle * 2.0 + swirlT, r * 6.0));
    vec3  lensCol  = vec3(0.8, 0.45, 0.15) * lensFade * (0.6 + 0.4 * swirl);

    // ── 4. Outer halo ──────────────────────────────────────────────────────────
    float halo     = pow(1.0 - smoothstep(0.0, 1.0, r), 3.0) * 0.25;
    vec3  haloCol  = vec3(0.3, 0.15, 0.5) * halo;

    // ── Composite ─────────────────────────────────────────────────────────────
    vec3 col  = ringCol + lensCol + haloCol;
    float a   = clamp(ring + lensFade * 0.8 + halo, 0.0, 1.0);

    // Black out the event horizon interior cleanly
    float mask = smoothstep(horizonR * 0.92, horizonR * 0.95, r);
    col  *= mask;
    a    *= mask;

    fragColor = vec4(col, a);
}
