#version 460 core

layout(location = 0) in uint inData0;
layout(location = 1) in uint inData1;
layout(location = 2) in uint inInstanceIndex;

struct DirectionalLightData {
    mat4 light_space_matrices[4];
    vec4 cascade_splits;
    vec3 direction;
    vec3 color;
    float intensity;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    DirectionalLightData directional_light;
    uint point_lights_count;
} ubo;

layout(set = 1, binding = 0, std430) readonly buffer ModelMatrices {
    mat4 models[];
} modelMatrices;

layout(set = 1, binding = 1, std430) readonly buffer NormalMatrices {
    mat4 normals[];
} normalMatrices;

layout(set = 4, binding = 0, std430) readonly buffer PaletteBuffer {
    uint palette[];
};

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) flat out uint fragAoPacked;
layout(location = 4) out float viewDepth;
layout(location = 5) out vec2 fragUV;

const vec3 NORMALS[6] = vec3[6](
    vec3( 1,  0,  0),
    vec3(-1,  0,  0),
    vec3( 0,  1,  0),
    vec3( 0, -1,  0),
    vec3( 0,  0,  1),
    vec3( 0,  0, -1)
);

vec3 unpackPaletteColor(uint packedColor) {
    float r = float((packedColor >> 24) & 0xFFu) / 255.0;
    float g = float((packedColor >> 16) & 0xFFu) / 255.0;
    float b = float((packedColor >>  8) & 0xFFu) / 255.0;
    return vec3(r, g, b);
}

void main() {
    uint px = inData0 & 0x7Fu;
    uint py = (inData0 >> 7) & 0x7Fu;
    uint pz = (inData0 >> 14) & 0x7Fu;
    uint normal_id = (inData0 >> 21) & 0x7u;
    uint palette_idx = inData1 & 0xFFu;

    uint ao_c0 = (inData1 >> 8) & 0x3u;
    uint ao_c1 = (inData1 >> 10) & 0x3u;
    uint ao_c2 = (inData1 >> 12) & 0x3u;
    uint ao_c3 = (inData1 >> 14) & 0x3u;
    uint corner = (inData1 >> 16) & 0x3u;

    mat4 model = modelMatrices.models[inInstanceIndex];

    vec3 localPos = vec3(float(px), float(py), float(pz));
    vec4 worldPos = model * vec4(localPos, 1.0);
    fragPos = worldPos.xyz;

    fragNormal = normalize(mat3(normalMatrices.normals[inInstanceIndex]) * NORMALS[normal_id]);

    fragColor = unpackPaletteColor(palette[palette_idx]);
    fragAoPacked = (ao_c0) | (ao_c1 << 2u) | (ao_c2 << 4u) | (ao_c3 << 6u);
    fragUV = vec2(float(corner & 1u), float(corner >> 1u));

    viewDepth = -(ubo.view * worldPos).z;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
