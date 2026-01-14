#version 460 core
#extension GL_ARB_shader_draw_parameters : require

// Входные данные от вертексов
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uint inColor;

// Структура directional light данных (соответствует directional_light_data в C++)
struct DirectionalLightData {
    mat4 light_space_matrix;
    vec3 direction;
    vec3 color;
    float intensity;
};

// Uniform buffer object (set 0, binding 0)
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    // Directional light
    DirectionalLightData directional_light;
    // Point lights count
    uint point_lights_count;
} ubo;

// Storage buffer для матриц моделей (set 1, binding 0)
layout(set = 1, binding = 0, std430) readonly buffer ModelMatrices {
    mat4 models[];
} modelMatrices;

// Выходные данные для fragment shader
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec3 viewPos;
layout(location = 6) out vec4 fragPosLightSpace;

vec3 unpackColor(uint packedColor) {
    float r = float((packedColor >> 24) & uint(0xFF)) / 255.0;
    float g = float((packedColor >> 16) & uint(0xFF)) / 255.0;
    float b = float((packedColor >> 8) & uint(0xFF)) / 255.0;
    return vec3(r, g, b);
}

void main() {
    // Получаем индекс объекта через gl_BaseInstanceARB
    // gl_BaseInstanceARB содержит first_instance из VkDrawIndexedIndirectCommand
    // В коде first_instance устанавливается в instance_index для каждого draw call
    // uint draw_index = gl_BaseInstanceARB;
    mat4 model = modelMatrices.models[gl_DrawID];
    
    // Трансформация позиции
    vec4 worldPos = model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    // Трансформация нормали
    fragNormal = normalize(mat3(transpose(inverse(model))) * inNormal);
    
    // Распаковка цвета
    fragColor = unpackColor(inColor);
    
    // Передача данных освещения
    viewPos = ubo.viewPos;
    
    // Вычисляем позицию в light space для shadow mapping
    fragPosLightSpace = ubo.directional_light.light_space_matrix * worldPos;
    
    // Финальная позиция вершины
    gl_Position = ubo.proj * ubo.view * worldPos;
}