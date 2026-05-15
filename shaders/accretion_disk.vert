#version 410 core

// Accretion disk -- local XY ring transformed to lie flat in world XZ
// via a model matrix that includes a 90-degree X-axis rotation.

layout(location = 0) in vec2  a_pos;   // ring vertex in local XY
layout(location = 1) in float a_t;     // parametric angle [0,1]

uniform mat4  u_MVP;
uniform float u_time;

out float v_t;
out float v_radial;

void main() {
    v_t = a_t;
    float r  = length(a_pos);
    v_radial = clamp((r - 1.05) / (2.2 - 1.05), 0.0, 1.0);
    gl_Position = u_MVP * vec4(a_pos.x, a_pos.y, 0.0, 1.0);
}
