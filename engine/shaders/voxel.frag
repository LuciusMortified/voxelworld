#version 450

// Входные данные от vertex shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec3 lightPos;
layout(location = 5) in vec3 lightColor;

// Выходной цвет пикселя
layout(location = 0) out vec4 outColor;

void main() {
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine lighting
    vec3 result = (ambient + diffuse + specular) * fragColor;
    
    // Add some fog effect based on distance
    float distance = length(viewPos - fragPos);
    float fogFactor = exp(-distance * 0.01);
    vec3 fogColor = vec3(0.7, 0.8, 0.9);
    result = mix(fogColor, result, fogFactor);
    
    outColor = vec4(result, 1.0);
}