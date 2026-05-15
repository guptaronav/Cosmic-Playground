#version 410 core

// ──────────────────────────────────────────────────────────────────────────────
//  Spacetime grid vertex shader.
//
//  Each vertex is displaced radially toward the viewer (along -Z in 2-D)
//  proportional to the local gravitational potential Φ = -G*M/r.
//  We encode this as a Y-offset to simulate the "rubber sheet" analogy.
//
//  In the vertex shader we can only read the potential, not write it; we do
//  not need compute shaders for this effect.
// ──────────────────────────────────────────────────────────────────────────────

layout(location = 0) in vec2 a_pos; // flat grid position

uniform mat4  u_VP;

struct MassPoint {
    vec2  pos;
    float mass;
};
uniform MassPoint u_bodies[8];
uniform int       u_bodyCount;

out float v_displacement; // passed to fragment for colour tinting

void main() {
    float phi = 0.0;
    for (int i = 0; i < u_bodyCount; ++i) {
        vec2  d   = a_pos - u_bodies[i].pos;
        float r   = length(d) + 0.5; // softened to avoid singularity at grid vertex
        phi      += -u_bodies[i].mass / r;
    }

    // Clamp the visual displacement so the grid doesn't fold on itself
    float disp = clamp(phi * 0.002, -2.5, 0.0);

    v_displacement = disp;

    // Displace vertex in Y to create the gravity-well visual
    vec2 displaced = a_pos + vec2(0.0, disp);
    gl_Position    = u_VP * vec4(displaced, 0.0, 1.0);
}
