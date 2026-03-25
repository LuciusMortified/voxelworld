#version 460 core

layout(location = 0) in uint inData0;
layout(location = 1) in uint inData1;
layout(location = 2) in uint inInstanceIndex;

layout(set = 0, binding = 0) uniform ShadowUniformBufferObject {
    mat4 light_space_matrices[4];
} shadowUbo;

layout(set = 1, binding = 0, std430) readonly buffer ModelMatrices {
    mat4 models[];
} modelMatrices;

layout(set = 1, binding = 1, std430) readonly buffer NormalMatrices {
    mat4 normals[];
} normalMatrices;

layout(push_constant) uniform ShadowPushConstants {
    uint cascadeIndex;
} pushConstants;

void main() {
    uint px = inData0 & 0x7Fu;
    uint py = (inData0 >> 7) & 0x7Fu;
    uint pz = (inData0 >> 14) & 0x7Fu;

    mat4 model = modelMatrices.models[inInstanceIndex];

    vec3 localPos = vec3(float(px), float(py), float(pz));
    vec4 worldPos = model * vec4(localPos, 1.0);

    mat4 lightSpaceMatrix = shadowUbo.light_space_matrices[pushConstants.cascadeIndex];
    gl_Position = lightSpaceMatrix * worldPos;
}
