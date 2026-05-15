#version 410 core

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;

uniform mat4 u_MVP;

out vec2 v_uv;
out vec2 v_local; // local quad position [-1,1] for SDF calculations

void main() {
    v_uv        = a_uv;
    v_local     = a_pos;
    gl_Position = u_MVP * vec4(a_pos, 0.0, 1.0);
}
