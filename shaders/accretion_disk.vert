#version 410 core

layout(location = 0) in vec2  a_pos;
layout(location = 1) in float a_t;      // parametric angle [0,1] around ring

uniform mat4  u_MVP;
uniform float u_time;

out float v_t;
out float v_radial;  // 0 = inner edge, 1 = outer edge

void main() {
    v_t = a_t;
    // Derive radial position from vertex distance: outer ~ 2.2, inner ~ 1.05
    float r  = length(a_pos);
    v_radial = clamp((r - 1.05) / (2.2 - 1.05), 0.0, 1.0);
    gl_Position = u_MVP * vec4(a_pos, 0.0, 1.0);
}
