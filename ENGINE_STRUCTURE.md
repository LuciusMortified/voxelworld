# Структура Воксельного Движка

## Файловая структура

```
engine/
├── include/
│   ├── engine.h           # Главный класс движка
│   ├── types.h            # Базовые типы данных (vec3, uint32, etc.)
│   ├── voxel.h            # Структура воксела
│   ├── world.h            # Мир с коллекцией моделей
│   ├── model.h            # Воксельная модель (3D массив)
│   ├── window.h           # Система окон (GLFW)
│   ├── vulkan_context.h   # Инициализация Vulkan
│   ├── camera.h           # Камера и матрицы
│   ├── buffer.h           # Управление Vulkan буферами
│   ├── mesh.h             # Геометрия для GPU
│   ├── shader.h           # Загрузка шейдеров
│   └── renderer.h         # Главный рендерер
├── src/
│   ├── engine.cpp
│   ├── world.cpp
│   ├── model.cpp
│   ├── window.cpp
│   ├── vulkan_context.cpp
│   ├── camera.cpp
│   ├── buffer.cpp
│   ├── mesh.cpp
│   ├── shader.cpp
│   └── renderer.cpp
├── shaders/
│   ├── voxel.vert         # Vertex shader
│   ├── voxel.frag         # Fragment shader
│   └── compile.sh         # Скрипт компиляции
└── CMakeLists.txt
```

## Основные классы и их назначение

### 1. Система Core
- **engine**: Главный координатор всех подсистем
- **types**: Математические типы (vec3f, ivec3, и т.д.)

### 2. Мир и данные
- **world**: Контейнер для воксельных моделей и их позиций
- **model**: 3D массив вокселей с операциями доступа
- **voxel**: Простая структура с цветом

### 3. Рендеринг Core
- **window**: GLFW окно и Vulkan surface
- **vulkan_context**: Инициализация Vulkan (instance, device, queues)
- **renderer**: Главный рендерер с swapchain и pipeline

### 4. Геометрия и ресурсы
- **mesh**: Представление геометрии (vertices + indices)
- **buffer**: Vulkan буферы (vertex, index, uniform)
- **shader**: Загрузка и управление шейдерами

### 5. Камера
- **camera**: View/Projection матрицы и управление

## Поток данных

```
Voxel Model → Mesh Generator → GPU Buffers → Vulkan Pipeline → Framebuffer
```

## Шейдеры

### Vertex Shader (voxel.vert)
- Трансформирует вершины из model space в clip space
- Передает данные для освещения в fragment shader
- Распаковывает упакованные цвета

### Fragment Shader (voxel.frag) 
- Реализует модель освещения Фонга
- Добавляет эффект тумана
- Выводит финальный цвет пикселя

## Зависимости

- **Vulkan SDK**: Графический API
- **GLFW**: Система окон и ввода
- **C++17**: Стандарт языка

## Сборка

```bash
mkdir build && cd build
cmake ..
make
```

## Использование

```cpp
#include "engine.h"

int main() {
    voxel::engine engine(1280, 720, "Voxel World");
    
    // Создать простую модель
    voxel::model model(4, 4, 4);
    model.set_voxel(1, 1, 1, 0xFF0000FF); // Красный воксель
    
    // Добавить в мир
    engine.get_world().add_model(model, {0, 0, 0});
    
    // Запустить движок
    engine.run();
    
    return 0;
}
```