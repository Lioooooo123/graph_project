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
uniform float time; // For animated effects

// NEW: Environment map for reflections
uniform samplerCube galaxy;

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
        metallic = 0.95; // Increased metallic for better reflections
        roughness = 0.15; // Smoother for better reflections
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

    // Environment Reflection
    vec3 reflectionDir = reflect(-v, n);
    // Simple box-correction could be added if we had bounds, but for infinity skybox this is fine
    // Sample texture LOD based on roughness (simulated by mixing or if extension supported)
    vec3 envColor = texture(galaxy, reflectionDir).rgb;

    // Tone down environment map a bit so it doesn't overwhelm
    envColor *= 1.5;

    // Combine lighting
    // F0 for dielectrics is around 0.04, for metals it is the albedo
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    vec3 kS = mix(vec3(pow(1.0 - roughness, 2.0)), F0, metallic); // Specular reflection amount
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic); // Diffuse amount

    vec3 diffusePortion = baseColor * diff * kD;
    vec3 specularPortion = (envColor * kS) + (lightColor * spec); // simplified IBL + analytic spec

    // Add rim
    // rim should be additive

    vec3 color = diffusePortion + specularPortion;
    color += rimColor * rim * (0.5 + metallic * 0.5);
    color *= lightColor;

    // Ambient (simplified IBL ambient)
    color += baseColor * 0.02 * envColor;

    // ========== ANIMATED INDICATOR LIGHTS ==========

    // Blue beacon light on antenna tips (fast blink)
    if (isAntenna && absY > 0.45) {
        float blueBlink = smoothstep(0.4, 0.6, sin(time * 6.0) * 0.5 + 0.5);
        color += vec3(0.3, 0.7, 1.0) * 3.0 * blueBlink;
    }

    // Red warning light on body corners (slow pulse)
    bool isCorner = absX > 0.25 && absZ > 0.25 && absY < 0.1 && absY > -0.1;
    if (isCorner) {
        float redPulse = smoothstep(0.3, 0.7, sin(time * 2.0) * 0.5 + 0.5);
        color += vec3(1.0, 0.1, 0.05) * 2.5 * redPulse;
    }

    // Green status light on sensor package (steady with occasional flicker)
    if (isSensor && absZ > 0.4) {
        float greenFlicker = 0.7 + 0.3 * sin(time * 15.0 + sin(time * 3.0) * 5.0);
        color += vec3(0.1, 1.0, 0.3) * 1.5 * greenFlicker;
    }

    // White strobe on solar panel tips (very fast strobe)
    bool isPanelTip = absX > 1.8 && absY < 0.05;
    if (isPanelTip) {
        float strobe = step(0.9, fract(time * 4.0));
        color += vec3(1.0, 1.0, 1.0) * 4.0 * strobe;
    }

    // Thruster glow (orange pulsing when active)
    if (isThruster && vLocalPos.y < -0.28) {
        float thrusterGlow = 0.3 + 0.7 * (sin(time * 8.0) * 0.5 + 0.5);
        color += vec3(1.0, 0.5, 0.1) * 2.0 * thrusterGlow;
    }

    FragColor = vec4(color, 1.0);
}
