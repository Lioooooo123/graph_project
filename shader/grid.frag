#version 330 core
out vec4 FragColor;

in vec3 vWorldPos;
in vec2 vUV;

void main() {
    // Very dark blue - near black with blue tint
    FragColor = vec4(0.02, 0.04, 0.15, 0.6);
}
