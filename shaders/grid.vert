#version 410 core

// Spacetime grid -- a flat XZ mesh whose vertices are displaced downward
// in Y proportional to the local Newtonian gravitational potential:
//
//   phi(x,z) = sum_i( -mass_i / sqrt(r_i^2 + eps^2) )
//
// With perspective projection from above, the downward bowls look like
// genuine spacetime curvature wells.

layout(location = 0) in vec2 a_xz;   // flat grid position (x, z)

struct MassPoint { vec2 pos; float mass; float _pad; };
uniform MassPoint u_bodies[8];
uniform int       u_bodyCount;
uniform mat4      u_VP;
uniform vec3      u_eyePos;    // camera eye for fog computation

out float v_depth;      // normalised displacement depth [0,1]
out float v_fogDist;    // world-space distance to camera

const float EPS      = 0.5;   // softening to avoid singularity
const float SCALE    = 0.025; // potential -> world units
const float MAX_DISP = 14.0;  // maximum downward displacement (world units)

void main() {
    float phi = 0.0;
    for (int i = 0; i < u_bodyCount; ++i) {
        vec2  d = a_xz - u_bodies[i].pos;
        float r = length(d);
        phi    += -u_bodies[i].mass / (r + EPS);
    }

    // Clamp and scale -- phi is negative, displacement goes downward (-Y)
    float disp = max(phi * SCALE, -MAX_DISP);

    vec3 worldPos = vec3(a_xz.x, disp, a_xz.y);

    v_depth   = clamp(-disp / MAX_DISP, 0.0, 1.0);  // 0=flat, 1=deepest well
    v_fogDist = length(worldPos - u_eyePos);

    gl_Position = u_VP * vec4(worldPos, 1.0);
}
