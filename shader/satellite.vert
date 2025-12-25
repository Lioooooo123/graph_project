#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float dishRotation;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vLocalPos;

void main() {
    vec3 localPos = aPos;
    vec3 localNormal = aNormal;
    
    // Rotate dish locally if it is the top part (y > 0.3)
    // The dish parts are generally above 0.25 in local Y
    if (aPos.y > 0.25) {
        float angle = dishRotation;
        float c = cos(angle);
        float s = sin(angle);
        mat3 rotY = mat3(
            c, 0.0, s,
            0.0, 1.0, 0.0,
            -s, 0.0, c
        );
        localPos = rotY * localPos;
        localNormal = rotY * localNormal;
    }

    vec4 worldPos = model * vec4(localPos, 1.0);
    vWorldPos = worldPos.xyz;
    vLocalPos = localPos; // Pass transformed local pos to frag for correct material masking if needed, though mostly material is static relative to parts

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vNormal = normalMatrix * localNormal;

    gl_Position = projection * view * worldPos;
}
