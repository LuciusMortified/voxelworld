#version 450

// Входные данные от вертексов
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uint inColor;

// Push constants для матрицы модели
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

// Uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    vec3 lightPos;
    vec3 lightColor;
} ubo;

// Выходные данные для fragment shader
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec3 viewPos;
layout(location = 4) out vec3 lightPos;
layout(location = 5) out vec3 lightColor;

vec3 unpackColor(uint packedColor) {
    float r = float((packedColor >> 24) & 0xFF) / 255.0;
    float g = float((packedColor >> 16) & 0xFF) / 255.0;
    float b = float((packedColor >> 8) & 0xFF) / 255.0;
    return vec3(r, g, b);
}

void main() {
    // Трансформация позиции
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    // Трансформация нормали
    fragNormal = normalize(mat3(transpose(inverse(push.model))) * inNormal);
    
    // Распаковка цвета
    fragColor = unpackColor(inColor);
    
    // Передача данных освещения
    viewPos = ubo.viewPos;
    lightPos = ubo.lightPos;
    lightColor = ubo.lightColor;
    
    // Финальная позиция вершины
    gl_Position = ubo.proj * ubo.view * worldPos;
}