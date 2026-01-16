#version 460 core
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uint inColor;
layout(location = 3) in uint inInstanceIndex;

// Uniform buffer object для shadow pass (set 0, binding 0)
layout(set = 0, binding = 0) uniform ShadowUniformBufferObject {
    mat4 light_space_matrix;
} shadowUbo;

// Storage buffer для матриц моделей (set 1, binding 0)
layout(set = 1, binding = 0, std430) readonly buffer ModelMatrices {
    mat4 models[];
} modelMatrices;

void main() {
    // Получаем матрицу модели для текущего объекта
    mat4 model = modelMatrices.models[inInstanceIndex];
    
    // Трансформация позиции в world space
    vec4 worldPos = model * vec4(inPosition, 1.0);
    
    // Преобразуем в light space для shadow mapping
    gl_Position = shadowUbo.light_space_matrix * worldPos;
}
