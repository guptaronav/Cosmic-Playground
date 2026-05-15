#version 410 core

in  float v_alpha;
uniform vec4  u_color;
out vec4 fragColor;

void main() {
    // Cubic fade makes the tail vanish smoothly
    float a = v_alpha * v_alpha * v_alpha;
    fragColor = vec4(u_color.rgb, a * 0.80);
}
