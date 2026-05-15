#version 410 core

layout(location = 0) in vec2  a_pos;    // world-space position
layout(location = 1) in float a_alpha;  // 0 = oldest (transparent), 1 = newest

uniform mat4 u_VP;

out float v_alpha;

void main() {
    v_alpha     = a_alpha;
    gl_Position = u_VP * vec4(a_pos, 0.0, 1.0);
}
