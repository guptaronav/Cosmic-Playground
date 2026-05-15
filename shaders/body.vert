#version 410 core

// Billboard vertex shader for planets and stars.
//
// The quad [-1,1] lives in view space so it always faces the camera.
// We place the body centre in world XZ (Y=0) then add the local offset
// in view space before projecting -- this is screen-space billboarding.

layout(location = 0) in vec2 a_pos;  // local quad [-1,1]
layout(location = 1) in vec2 a_uv;   // [0,1]

uniform mat4  u_V;          // view matrix
uniform mat4  u_P;          // projection matrix
uniform vec3  u_worldPos;   // body centre in world space (x, 0, z)
uniform float u_scale;      // half-size in world units (radius * glow)

out vec2 v_uv;

void main() {
    // Transform the world-space centre to view space
    vec4 viewCentre = u_V * vec4(u_worldPos, 1.0);

    // Offset in view-space XY so the quad faces the camera exactly
    viewCentre.xy += a_pos * u_scale;

    v_uv        = a_uv;
    gl_Position = u_P * viewCentre;
}
