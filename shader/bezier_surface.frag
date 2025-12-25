#version 330 core
out vec4 FragColor;

in vec3 vWorldPos;
in vec2 vUV;

uniform float time;
uniform vec3 color;

void main() {
    // Grid pattern
    float gridU = step(0.95, fract(vUV.x * 20.0));
    float gridV = step(0.95, fract(vUV.y * 20.0));
    float grid = max(gridU, gridV);

    // Glowing energy effect
    float glow = 0.5 + 0.5 * sin(vUV.x * 10.0 + time * 2.0) * sin(vUV.y * 10.0 + time * 3.0);
    
    vec3 finalColor = color;
    float alpha = 0.1 + 0.4 * grid + 0.2 * glow;
    
    // Fade edges
    float dist = distance(vUV, vec2(0.5));
    alpha *= smoothstep(0.7, 0.3, dist);

    if (alpha <= 0.05) discard;

    FragColor = vec4(finalColor, alpha);
}
