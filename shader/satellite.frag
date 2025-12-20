#version 330 core
out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vLocalPos;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 rimColor;
uniform float rimStrength;

// Material colors
const vec3 kGoldAlbedo = vec3(1.0, 0.85, 0.4);
const vec3 kPanelAlbedo = vec3(0.02, 0.025, 0.04);
const vec3 kPanelFrameAlbedo = vec3(0.6, 0.6, 0.65);
const vec3 kAntennaAlbedo = vec3(0.85, 0.85, 0.9);
const vec3 kThrusterAlbedo = vec3(0.15, 0.15, 0.18);
const vec3 kSensorAlbedo = vec3(0.08, 0.08, 0.1);

void main() {
    vec3 n = normalize(vNormal);
    vec3 l = normalize(lightDir);
    vec3 v = normalize(viewPos - vWorldPos);
    vec3 h = normalize(l + v);

    // Determine material based on position
    float absX = abs(vLocalPos.x);
    float absY = abs(vLocalPos.y);
    float absZ = abs(vLocalPos.z);

    // Material classification
    bool isSolarPanel = absX > 0.6 && absY < 0.08;
    bool isPanelFrame = absX > 0.6 && absY >= 0.08 && absY < 0.15;
    bool isAntenna = absY > 0.4 || (absY > 0.25 && (absX < 0.2 || absZ > 0.15));
    bool isThruster = vLocalPos.y < -0.25;
    bool isSensor = absZ > 0.35;
    bool isMainBody = absX < 0.4 && absY < 0.3 && absZ < 0.35;

    // Select material properties
    vec3 baseColor;
    float metallic;
    float roughness;

    if (isSolarPanel) {
        baseColor = kPanelAlbedo;
        metallic = 0.1;
        roughness = 0.5;
    } else if (isPanelFrame) {
        baseColor = kPanelFrameAlbedo;
        metallic = 0.7;
        roughness = 0.3;
    } else if (isThruster) {
        baseColor = kThrusterAlbedo;
        metallic = 0.4;
        roughness = 0.6;
    } else if (isSensor) {
        baseColor = kSensorAlbedo;
        metallic = 0.3;
        roughness = 0.4;
    } else if (isAntenna) {
        baseColor = kAntennaAlbedo;
        metallic = 0.9;
        roughness = 0.15;
    } else {
        // Main body - gold foil
        baseColor = kGoldAlbedo;
        metallic = 0.85;
        roughness = 0.25;
    }

    // Lighting calculations
    float NdotL = max(dot(n, l), 0.0);
    float NdotH = max(dot(n, h), 0.0);
    float NdotV = max(dot(n, v), 0.0);

    // Diffuse
    float diff = NdotL;

    // Specular with variable power based on roughness
    float specPower = mix(128.0, 8.0, roughness);
    float spec = pow(NdotH, specPower) * (1.0 - roughness * 0.5);

    // Fresnel approximation for rim
    float fresnel = pow(1.0 - NdotV, 3.0);

    // Rim lighting - stronger on edges facing the light
    float rim = fresnel * rimStrength;
    rim *= smoothstep(-0.2, 0.5, NdotL);

    // Solar panel special effect - slight blue reflection
    if (isSolarPanel) {
        float panelReflect = pow(NdotH, 32.0) * 0.3;
        baseColor += vec3(0.0, 0.02, 0.08) * panelReflect;
    }

    // Combine lighting
    vec3 specularColor = mix(vec3(0.04), baseColor, metallic);
    vec3 color = baseColor * diff * (1.0 - metallic * 0.5);
    color += specularColor * spec;
    color += rimColor * rim * (0.5 + metallic * 0.5);
    color *= lightColor;

    // Ambient
    color += baseColor * 0.04;

    // Add slight emission to antenna tips
    if (isAntenna && absY > 0.45) {
        color += vec3(0.1, 0.15, 0.2) * 0.5;
    }

    FragColor = vec4(color, 1.0);
}
