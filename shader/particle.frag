#version 330 core
out vec4 FragColor;

in float vLife;
in float vSize;

void main() {
    // Circular particle shape
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    if (dist > 0.5) discard;

    // Soft edge falloff
    float alpha = smoothstep(0.5, 0.2, dist) * vLife;

    // Color gradient from white-hot to orange to red as particle ages
    vec3 hotColor = vec3(1.0, 0.95, 0.8);   // White-hot core
    vec3 warmColor = vec3(1.0, 0.5, 0.1);   // Orange
    vec3 coolColor = vec3(0.8, 0.2, 0.05);  // Red

    vec3 color;
    if (vLife > 0.7) {
        color = mix(warmColor, hotColor, (vLife - 0.7) / 0.3);
    } else if (vLife > 0.3) {
        color = mix(coolColor, warmColor, (vLife - 0.3) / 0.4);
    } else {
        color = coolColor * (vLife / 0.3);
    }

    // Add bloom-friendly HDR values
    color *= 2.0;

    FragColor = vec4(color, alpha);
}
