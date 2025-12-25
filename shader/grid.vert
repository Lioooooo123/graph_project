#version 330 core
layout (location = 0) in vec3 aPos; // x,y are UV coordinates (0..1)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 controlPoints[16]; // 4x4 Bezier control points

out vec3 vWorldPos;
out vec2 vUV;

vec3 evaluateBezierSurface(vec2 uv) {
    float u = uv.x;
    float v = uv.y;

    // Cubic Bernstein basis functions for u
    float bu[4];
    bu[0] = pow(1.0 - u, 3.0);
    bu[1] = 3.0 * u * pow(1.0 - u, 2.0);
    bu[2] = 3.0 * u * u * (1.0 - u);
    bu[3] = u * u * u;

    // Cubic Bernstein basis functions for v
    float bv[4];
    bv[0] = pow(1.0 - v, 3.0);
    bv[1] = 3.0 * v * pow(1.0 - v, 2.0);
    bv[2] = 3.0 * v * v * (1.0 - v);
    bv[3] = v * v * v;

    // Evaluate Bezier surface
    vec3 p = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            p += controlPoints[i * 4 + j] * bu[i] * bv[j];
        }
    }
    return p;
}

void main() {
    vUV = aPos.xy;
    vec3 localPos = evaluateBezierSurface(vUV);

    vec4 worldPos = model * vec4(localPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;
}
