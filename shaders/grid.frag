#version 410 core

in float v_depth;      // 0 = flat, 1 = bottom of deepest well
in float v_fogDist;    // distance from camera for edge fade

out vec4 fragColor;

void main() {
    // Color ramp: dim purple -> bright blue -> cyan -> near-white at extreme depth
    vec3 c0 = vec3(0.06, 0.06, 0.18);  // flat (barely visible)
    vec3 c1 = vec3(0.10, 0.30, 0.90);  // shallow well (blue)
    vec3 c2 = vec3(0.05, 0.85, 1.00);  // moderate well (cyan)
    vec3 c3 = vec3(0.90, 0.95, 1.00);  // deep well (near-white)

    vec3 col;
    if (v_depth < 0.35)
        col = mix(c0, c1, v_depth / 0.35);
    else if (v_depth < 0.70)
        col = mix(c1, c2, (v_depth - 0.35) / 0.35);
    else
        col = mix(c2, c3, (v_depth - 0.70) / 0.30);

    // Brightness and alpha increase toward deeper wells
    float bright = mix(0.5, 2.2, v_depth * v_depth);
    float alpha  = mix(0.12, 0.90, v_depth);

    // Exponential distance fog so distant grid lines fade cleanly
    float fog = exp(-v_fogDist * 0.012);
    alpha *= clamp(fog, 0.0, 1.0);

    fragColor = vec4(col * bright, alpha);
}
