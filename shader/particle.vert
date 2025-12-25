#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aVelocity;
layout(location = 2) in float aLife;

out float vLife;
out float vSize;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main() {
    vLife = aLife;

    // Particle size decreases with age
    vSize = mix(8.0, 2.0, 1.0 - aLife);

    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position = projection * viewPos;
    gl_PointSize = vSize / (-viewPos.z * 0.1);
}
