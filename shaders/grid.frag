#version 410 core

in  float v_displacement;
out vec4  fragColor;

void main() {
    // Deeper wells get a warmer purple tint; flat areas stay dim grey
    float t    = clamp(-v_displacement / 2.5, 0.0, 1.0);
    vec3  flat_col = vec3(0.15, 0.15, 0.25);
    vec3  deep_col = vec3(0.40, 0.25, 0.80);
    vec3  col  = mix(flat_col, deep_col, t);
    float alpha = mix(0.18, 0.55, t);
    fragColor = vec4(col, alpha);
}
