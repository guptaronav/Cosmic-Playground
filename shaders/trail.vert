#version 410 core

// Trail line-strip shader -- lifts 2-D sim positions to world Y=0.

layout(location = 0) in vec2  a_xz;    // sim position = world (x, z)
layout(location = 1) in float a_alpha;

uniform mat4 u_VP;

out float v_alpha;

void main() {
    v_alpha     = a_alpha;
    // Lift slightly above Y=0 so trails sit on top of the grid
    gl_Position = u_VP * vec4(a_xz.x, 0.05, a_xz.y, 1.0);
}
