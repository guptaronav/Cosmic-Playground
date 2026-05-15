#version 410 core

// Billboard vertex shader for black holes -- identical strategy to body.vert.

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;

uniform mat4  u_V;
uniform mat4  u_P;
uniform vec3  u_worldPos;
uniform float u_scale;

out vec2 v_uv;
out vec2 v_local;

void main() {
    vec4 viewCentre = u_V * vec4(u_worldPos, 1.0);
    viewCentre.xy  += a_pos * u_scale;

    v_uv        = a_uv;
    v_local     = a_pos;
    gl_Position = u_P * viewCentre;
}
