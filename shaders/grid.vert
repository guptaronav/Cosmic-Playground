#version 410 core

// Spacetime grid vertex shader.
//
// Each vertex is displaced in Y proportional to the local gravitational
// potential: phi = -G*M/r  (summed over all mass points passed as uniforms).
// This produces the classic "rubber sheet" gravity-well visualization.

layout(location = 0) in vec2 a_pos;

uniform mat4 u_VP;

struct MassPoint {
    vec2  pos;
    float mass;
};
uniform MassPoint u_bodies[8];
uniform int       u_bodyCount;

out float v_displacement;

void main() {
    float phi = 0.0;
    for (int i = 0; i < u_bodyCount; ++i) {
        vec2  d  = a_pos - u_bodies[i].pos;
        float r  = length(d) + 0.5; // softened to avoid singularity at grid vertex
        phi     += -u_bodies[i].mass / r;
    }

    // Clamp displacement so the grid does not fold on itself
    float disp = clamp(phi * 0.002, -2.5, 0.0);
    v_displacement = disp;

    vec2 displaced = a_pos + vec2(0.0, disp);
    gl_Position    = u_VP * vec4(displaced, 0.0, 1.0);
}
