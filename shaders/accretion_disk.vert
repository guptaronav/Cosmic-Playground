#version 410 core

layout(location = 0) in vec2  a_pos;    // ring vertex position
layout(location = 1) in float a_t;      // parametric position along ring [0,1]

uniform mat4  u_MVP;
uniform float u_time;

out float v_t;
out float v_radial; // 0 = inner edge, 1 = outer edge (derived from position)

void main() {
    v_t      = a_t;
    // Determine radial coordinate from position magnitude;
    // outer ring vertices have length ≈ 2.2, inner ≈ 1.05
    float r  = length(a_pos);
    v_radial = clamp((r - 1.05) / (2.2 - 1.05), 0.0, 1.0);

    gl_Position = u_MVP * vec4(a_pos, 0.0, 1.0);
}
