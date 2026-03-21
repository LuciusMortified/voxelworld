#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uint inColor;
layout(location = 3) in uint inInstanceIndex;
layout(location = 4) in float inAo;

// Uniform buffer object для shadow pass (set 0, binding 0)
layout(set = 0, binding = 0) uniform ShadowUniformBufferObject {
    mat4 light_space_matrices[4];
} shadowUbo;

// Storage buffers для трансформов (set 1)
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
    mat4 model = modelMatrices.models[inInstanceIndex];

    vec4 worldPos = model * vec4(inPosition, 1.0);

    // Преобразуем в light space для shadow mapping
    mat4 lightSpaceMatrix = shadowUbo.light_space_matrices[pushConstants.cascadeIndex];
    gl_Position = lightSpaceMatrix * worldPos;
}
