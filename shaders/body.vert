#version 410 core

layout(location = 0) in vec2 a_pos;   // local quad [-1,1]
layout(location = 1) in vec2 a_uv;    // [0,1]

uniform mat4 u_MVP;

out vec2 v_uv;

void main() {
    v_uv        = a_uv;
    gl_Position = u_MVP * vec4(a_pos, 0.0, 1.0);
}
