#version 330 core
layout (location = 0) in vec3 aPos; // x,y are UV coordinates (0..1), z unused

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 controlPoints[16]; // 4x4 control points
uniform float time;

out vec3 vWorldPos;
out vec2 vUV;

vec3 bezierBasis(float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    float mt = 1.0 - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    
    return vec3(mt3, 3.0 * mt2 * t, 3.0 * mt * t2); // First 3 terms
    // 4th term is t^3, handled implicitly in combination
}

vec3 evaluateBezier(vec2 uv) {
    float u = uv.x;
    float v = uv.y;
    
    float bu[4];
    bu[0] = pow(1.0 - u, 3.0);
    bu[1] = 3.0 * u * pow(1.0 - u, 2.0);
    bu[2] = 3.0 * u * u * (1.0 - u);
    bu[3] = u * u * u;
    
    float bv[4];
    bv[0] = pow(1.0 - v, 3.0);
    bv[1] = 3.0 * v * pow(1.0 - v, 2.0);
    bv[2] = 3.0 * v * v * (1.0 - v);
    bv[3] = v * v * v;
    
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
    vec3 localPos = evaluateBezier(vUV);
    
    // Animate control points slightly for "warping" effect would be done on CPU side in practice, 
    // but we can also modulate the result here:
    localPos.y += sin(localPos.x * 5.0 + time) * 0.1 * cos(localPos.z * 5.0);

    vec4 worldPos = model * vec4(localPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;
}
