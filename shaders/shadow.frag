#version 460 core

// Пустой fragment shader для depth-only rendering
// В shadow pass мы рендерим только depth, поэтому фрагментный шейдер не используется
// Но Vulkan требует fragment shader для graphics pipeline

void main() {
    // Пусто - используется только depth
}
