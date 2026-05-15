#version 410 core

in  float v_displacement;
out vec4  fragColor;

void main() {
    // Deeper wells → warmer (bluish-purple) tint; flat grid → dim grey
    float t     = clamp(-v_displacement / 2.5, 0.0, 1.0);
    vec3  flat  = vec3(0.15, 0.15, 0.25);
    vec3  deep  = vec3(0.40, 0.25, 0.80);
    vec3  col   = mix(flat, deep, t);

    // Make the grid semi-transparent so space shows through
    float alpha = mix(0.18, 0.55, t);

    fragColor = vec4(col, alpha);
}
