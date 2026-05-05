#version 460 core

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) flat in uint fragAoPacked;
layout(location = 4) in float viewDepth;
layout(location = 5) in vec2 fragUV;

struct DirectionalLightData {
    mat4 light_space_matrices[4];
    vec4 cascade_splits;// x = split0, y = split1, z = split2, w = split3
    vec3 direction;
    vec3 color;
    float intensity;
};

struct FogData {
    vec3 color;
    float near_distance;
    float far_distance;
    uint enabled;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    DirectionalLightData directional_light;
    uint point_lights_count;
    FogData fog;
} ubo;

layout(set = 2, binding = 0) uniform sampler2DArrayShadow shadowMapArray;

struct PointLightData {
    vec4 position;
    vec4 color;
    float intensity;
    float range;
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
};

layout(set = 3, binding = 0, std430) readonly buffer PointLights {
    PointLightData lights[];
} pointLights;

int selectCascade(float viewDepth) {
    if (viewDepth < ubo.directional_light.cascade_splits.x) return 0;
    if (viewDepth < ubo.directional_light.cascade_splits.y) return 1;
    if (viewDepth < ubo.directional_light.cascade_splits.z) return 2;
    return 3;
}

float calculateShadowForCascade(int cascadeIndex, vec3 normal) {
    // Если свет почти перпендикулярен поверхности, не отбрасываем тени
    float ndot = dot(normal, normalize(-ubo.directional_light.direction));
    if (ndot < 0.01) {
        return 1.0;
    }

    // Вычисляем позицию в light space для выбранного каскада
    float ndotl = max(ndot, 0.0);
    float normalBias = max(0.015 * (1.0 - ndotl), 0.004);
    vec3 newFragPos = fragPos + fragNormal * normalBias;
    vec4 fragPosLightSpace =
        ubo.directional_light.light_space_matrices[cascadeIndex] * vec4(newFragPos, 1.0);

    // Проецируем координаты в диапазон [-1, 1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Преобразуем в диапазон [0, 1] только xy
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Проверяем, что координаты в пределах [0, 1]
    if (
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0
    ) {
        return 1.0;
    }

    ivec2 texDim = textureSize(shadowMapArray, 0).xy;
    float scale = 1.0;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadow = 0;
    int count = 0;
    int range = 0;  // 0 for off, 1 for 3x3, 2 for 5x5
    for (int x = -range; x <= range; ++x) {
        for (int y = -range; y <= range; ++y) {
            shadow += texture(
                shadowMapArray,
                    vec4(
                        vec2(projCoords.x + x*dx, projCoords.y + y*dy),
                        float(cascadeIndex),
                        projCoords.z
                )
            );
            count++;
        }
    }

    return shadow / count;
}

float calculateShadow(vec3 normal, float viewDepth) {
    int cascadeIndex = selectCascade(viewDepth);

    float shadow = calculateShadowForCascade(cascadeIndex, normal);

    if (cascadeIndex < 3) {
        float nextSplit = ubo.directional_light.cascade_splits[cascadeIndex];
        float blendStart = nextSplit * 0.75;
        float blendEnd = nextSplit;

        if (viewDepth > blendStart && viewDepth < blendEnd) {
            int nextCascadeIndex = cascadeIndex + 1;
            float nextShadow = calculateShadowForCascade(nextCascadeIndex, normal);

            float blendFactor = smoothstep(blendStart, blendEnd, viewDepth);
            shadow = mix(shadow, nextShadow, blendFactor);
        }
    }

    return shadow;
}

vec3 calculateDirectionalLight(vec3 normal, vec3 viewDir, float shadow) {
    float sunElevation = -ubo.directional_light.direction.y;

    float dayFactor = smoothstep(0.2, 0.4, sunElevation);
    float twilightFactor = smoothstep(0.0, 0.2, sunElevation) * (1.0 - dayFactor);

    vec3 lightDir = normalize(-ubo.directional_light.direction);

    float diff = max(dot(lightDir, normal), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

    vec3 sunColor = ubo.directional_light.color * ubo.directional_light.intensity;
    vec3 twilightColor = vec3(1.0, 0.5, 0.2) * ubo.directional_light.intensity * 0.3;

    vec3 diffuse = diff * shadow * (sunColor * dayFactor + twilightColor * twilightFactor);
    vec3 specular = spec * shadow * sunColor * dayFactor * 0.5;

    return diffuse + specular;
}

vec3 calculatePointLight(uint lightIndex, vec3 normal, vec3 fragPos, vec3 viewDir) {
    PointLightData light = pointLights.lights[lightIndex];

    // Направление к свету
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float distance = length(light.position.xyz - fragPos);

    if (distance > light.range) {
        return vec3(0.0);
    }

    float attenuation = light.attenuation_constant +
        light.attenuation_linear * distance +
        light.attenuation_quadratic * distance * distance;
    attenuation = 1.0 / max(attenuation, 0.001);

    float ratio = distance / light.range;
    float smoothFalloff = clamp(1.0 - ratio * ratio, 0.0, 1.0);
    smoothFalloff *= smoothFalloff;
    attenuation *= smoothFalloff;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

    vec3 diffuse = diff * light.color.xyz * light.intensity * attenuation;
    vec3 specular = spec * light.color.xyz * light.intensity * attenuation * 0.5;

    return diffuse + specular;
}

// Вычисляет rim lighting (Fresnel эффект) для подсветки краёв
float calculateRimLight(vec3 normal, vec3 viewDir, float shadow) {
    // Чем больше угол между нормалью и направлением взгляда, тем сильнее rim
    float rimFactor = 1.0 - max(dot(normal, viewDir), 0.0);
    // Усиливаем rim в тенях
    float shadowBoost = 1.0 + (1.0 - shadow) * 2.0;
    // Применяем степенную функцию для более плавного перехода
    rimFactor = pow(rimFactor, 3.0) * shadowBoost;
    return rimFactor;
}

// Вычисляет hemisphere ambient - ambient, зависящий от нормали
vec3 calculateHemisphereAmbient(vec3 normal) {
    // Верхнее небо (светлее)
    vec3 skyColor = vec3(0.3, 0.4, 0.5);
    // Нижняя земля (темнее)
    vec3 groundColor = vec3(0.1, 0.1, 0.15);

    // Смешиваем цвета в зависимости от направления нормали
    float upFactor = normal.y * 0.5 + 0.5;// Преобразуем [-1, 1] в [0, 1]
    return mix(groundColor, skyColor, upFactor);
}

// Выходной цвет пикселя
layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(ubo.viewPos - fragPos);

    float sunElevation = -ubo.directional_light.direction.y;
    float shadowReliability = smoothstep(0.15, 0.3, sunElevation);
    float shadow = mix(1.0, calculateShadow(normal, viewDepth), shadowReliability);

    // Ambient lighting
    float ambientStrength = 0.25;
    vec3 baseAmbient = ambientStrength * ubo.directional_light.color * ubo.directional_light.intensity;

    // Hemisphere ambient для более выразительного объёма
    float hemisphereStrength = 0.15;
    vec3 hemisphereAmbient = calculateHemisphereAmbient(normal) * hemisphereStrength;

    float c0_dark = float(fragAoPacked & 0x3u) / 3.0;
    float c1_dark = float((fragAoPacked >> 2u) & 0x3u) / 3.0;
    float c2_dark = float((fragAoPacked >> 4u) & 0x3u) / 3.0;
    float c3_dark = float((fragAoPacked >> 6u) & 0x3u) / 3.0;

    float ao_dark = mix(
        mix(c0_dark, c1_dark, fragUV.x),
        mix(c3_dark, c2_dark, fragUV.x),
        fragUV.y
    );

    float c0_bright = float((fragAoPacked >> 8u)  & 0x3u) / 3.0;
    float c1_bright = float((fragAoPacked >> 10u) & 0x3u) / 3.0;
    float c2_bright = float((fragAoPacked >> 12u) & 0x3u) / 3.0;
    float c3_bright = float((fragAoPacked >> 14u) & 0x3u) / 3.0;

    float ao_bright = mix(
        mix(c0_bright, c1_bright, fragUV.x),
        mix(c3_bright, c2_bright, fragUV.x),
        fragUV.y
    );

    float upFactor = max(0.0, normal.y);

    float aoStrength = 0.5;
    float brightStrength = 0.15;
    float aoFactor = 1.0 - ao_dark * aoStrength + ao_bright * brightStrength * upFactor;

    vec3 ambient = (baseAmbient + hemisphereAmbient) * aoFactor;

    // Directional light с sun_factor
    vec3 directional = calculateDirectionalLight(normal, viewDir, shadow);

    // Point lights
    vec3 pointLighting = vec3(0.0);
    for (uint i = 0; i < ubo.point_lights_count; i++) {
        pointLighting += calculatePointLight(i, normal, fragPos, viewDir);
    }

    // Rim lighting для подсветки краёв (особенно эффективно в тенях)
    float rimIntensity = 0.01;
    float rimLighting = calculateRimLight(normal, viewDir, shadow);
    vec3 rimColor = ubo.directional_light.color * rimLighting * rimIntensity;

    // Усиление контраста на гранях вокселей (edge enhancement)
    // Используем dot product нормали и направления взгляда для определения "краёв"
    float edgeIntensity = 0.01;
    float edgeFactor = abs(dot(normal, viewDir));
    // Грани, перпендикулярные взгляду (края вокселей), получают дополнительное освещение
    float edgeEnhancement = (1.0 - edgeFactor) * edgeIntensity * (1.0 - shadow);

    // Комбинируем все освещение
    vec3 lighting = ambient + (directional + pointLighting + rimColor) * aoFactor;
    vec3 result = lighting * fragColor;

    // Применяем edge enhancement
    result += edgeEnhancement * fragColor;

    // Linear fog
    if (ubo.fog.enabled != 0u) {
        float fogFactor = clamp(
            (ubo.fog.far_distance - viewDepth) / (ubo.fog.far_distance - ubo.fog.near_distance),
            0.0, 1.0
        );
        result = mix(ubo.fog.color, result, fogFactor);
    }

    outColor = vec4(result, 1.0);
}
