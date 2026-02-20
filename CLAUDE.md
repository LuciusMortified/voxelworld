# CLAUDE.md - Руководство для ИИ-ассистентов

Этот документ содержит информацию для ИИ-ассистентов, работающих с кодовой базой Voxel World.

## Обзор проекта

Voxel World — воксельный движок на C++/Vulkan со следующими особенностями:
- Современный C++23 с header-only архитектурой движка
- Entity Component System (ECS) архитектура
- Рендеринг на Vulkan с каскадными тенями
- Приложение Sculptor для редактирования воксельных моделей
- Формат VOX для сериализации

**Текущая версия**: 0.0.1 (ранняя разработка)

## Быстрый старт

```bash
# Сборка проекта
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Компиляция шейдеров (из директории shaders/)
./compile.sh    # Linux/macOS
compile.bat     # Windows

# Запуск Sculptor
./build/apps/sculptor/sculptor
```

## Структура директорий

```
voxelworld/
├── engine/                    # Header-only библиотека движка
│   └── include/vw/
│       ├── core/              # Базовые типы (types.h, vec3.h, mat4.h, color.h)
│       ├── gfx/
│       │   ├── camera/        # Камера (camera.h, fps_camera_controller.h)
│       │   ├── debug/         # Отладка (debug_window.h, debug_primitive.h)
│       │   ├── engine/        # Координация движка (engine.h, app.h)
│       │   ├── model/         # Воксельные модели (model.h, model_registry.h)
│       │   ├── render/        # Vulkan рендеринг (renderer.h, vulkan_context.h)
│       │   ├── resource/      # GPU ресурсы (buffer.h, mesh.h, shader.h)
│       │   ├── spatial/       # Пространственная математика (ray.h, aabb.h)
│       │   └── world/         # ECS (entity.h, registry.h, components/, systems/)
│       └── log/               # Логирование (logger.h)
├── apps/
│   ├── sculptor/              # Основное приложение — редактор вокселей
│   │   └── src/
│   │       ├── app/           # Состояние приложения
│   │       ├── operations/    # Операции отмены/повтора
│   │       ├── tools/         # Инструменты (добавление, удаление, покраска)
│   │       └── ui/            # ImGui панели и модальные окна
│   ├── test_window/           # Тест окна и ввода
│   ├── test_simple_model/     # Тест рендеринга модели
│   └── test_math_matrix/      # Тест математики
├── shaders/                   # GLSL шейдеры (voxel, shadow, debug)
├── docs/                      # Документация (ENGINE.md, TASKS.md, PRD.md)
└── vcpkg.json                 # Зависимости
```

## Архитектурные концепции

### Header-Only паттерн
- Движок — INTERFACE библиотека (без компилируемых .cpp файлов)
- Заголовки объявляют интерфейсы (`.h`), реализации в inline-заголовках (`.inl.h`)
- Приложения — компилируемые исполняемые файлы, линкующиеся с header-библиотекой

### Entity Component System (ECS)
```cpp
// Registry шаблонизирован типами компонентов
template <typename... Cs>
class registry { /* ... */ };

// View для итерации по сущностям с определёнными компонентами
auto view = registry.view<TransformComponent, ModelComponent>();
for (auto [ent, transform, model] : view) { /* ... */ }
```

### Паттерн Entity Handle
```cpp
struct entity {
    uint32 index;       // Индекс в плотном массиве
    uint32 generation;  // Версия для защиты от use-after-free
};
```

### Command паттерн для Undo/Redo
```cpp
class base_operation {
    virtual void execute() = 0;
    virtual void undo() = 0;
};
```

## Соглашения по коду

### Стандарт C++
- **Стандарт**: C++23
- **Флаги компилятора**: `-Wall -Wextra -Wpedantic`
- **Исключения**: не используются в горячих путях; ошибки через возвращаемые значения

### Именование
- **Пространства имён**: `vw::` (ядро), `vw::gfx::` (графика), `vw::sculptor::` (редактор)
- **Типы**: `snake_case` для классов/структур (например, `entity`, `model_registry`)
- **Функции/методы**: `snake_case` (например, `get_voxel`, `is_valid`)
- **Члены класса**: `snake_case`, без префиксов (например, `index`, `generation`)
- **Константы**: `snake_case` (например, `invalid_index`)
- **Шаблонные параметры**: `PascalCase` (например, `Cs`, `T`)

### Защита заголовков
```cpp
#pragma once

#ifndef VW_PATH_TO_FILE_H
#define VW_PATH_TO_FILE_H
// ... содержимое ...
#endif  // VW_PATH_TO_FILE_H
```

### Синтаксис возвращаемого типа
Используйте trailing return type для всех методов:
```cpp
[[nodiscard]] auto get_value() const -> int;
auto operator=(const foo&) -> foo& = delete;
```

### Форматирование (.clang-format)
- **Базовый стиль**: Google
- **Отступы**: 4 пробела (без табуляции)
- **Лимит строки**: 100 символов
- **Фигурные скобки**: присоединённые (K&R стиль)
- **Указатели**: выравнивание влево (`int* ptr`)
- **Инициализаторы конструктора**: перенос перед запятой, по одному на строку

### Порядок include
1. Заголовки стандартной библиотеки
2. Сторонние заголовки
3. Заголовки проекта (vw/...)
4. Локальные заголовки

### Комментарии
Проект придерживается философии **минимальных комментариев**:
- Код должен быть **самодокументируемым** через ясные названия функций, переменных и полей
- Комментарии добавляются **только** для классов/структур верхнего уровня (краткое описание назначения)
- Комментарии в реализациях (`.inl.h`, `.cpp`) **избегаются** — логика должна быть понятна из кода
- Комментарии допустимы только для **действительно сложной логики**, где намерение не очевидно
- **НЕ добавляйте** избыточные комментарии вроде:
  - `// Вычислить индекс` перед `uint32 index = time / frame_time;`
  - `// Обновить время` перед `current_time += delta_time;`
  - Секционные разделители вида `// ========== Section ==========`

## Псевдонимы типов

Используйте типы проекта из `vw/core/types.h`:
```cpp
vw::uint8, vw::uint16, vw::uint32, vw::uint64
vw::int8, vw::int16, vw::int32, vw::int64
vw::float32, vw::float64
```

## Зависимости

Управляются через vcpkg:
- **Vulkan SDK** (системное требование)
- **glfw3** — управление окнами
- **imgui** (с glfw-binding, vulkan-binding) — UI
- **spdlog** (>=1.15.3) — логирование

## Система сборки

### Структура CMake
- Корневой `CMakeLists.txt` — минимальный, делегирует поддиректориям
- `engine/CMakeLists.txt` — определение INTERFACE библиотеки
- `apps/CMakeLists.txt` — агрегатор приложений
- Индивидуальные `CMakeLists.txt` приложений

### Ключевые настройки CMake
```cmake
cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)
```

### Компиляция шейдеров
Шейдеры GLSL компилируются в SPIR-V через `glslc`:
- Исходники: `shaders/*.vert`, `shaders/*.frag`
- Результат: `shaders/*.spv`
- Компилируются автоматически при сборке Sculptor

## Тестирование

Фреймворк: **Catch2 v3**. Тесты разделены на два executable:
- `core_tests` — чистая математика и типы (`tests/core/`): vec, mat, quat, color, voxel, transform, math
- `ecs_tests` — ECS (`tests/ecs/`): entity_pool, component_pool, registry, socket

### Сборка и запуск тестов

Тесты собираются в **отдельной build-директории** (не в основной `build/debug` или `build/release`):

```bash
# Конфигурация (только тесты, без приложений)
cmake -S . -B build/tests \
  -DCMAKE_TOOLCHAIN_FILE=C:/Users/lucius/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows \
  -DVW_BUILD_APPS=OFF

# Сборка
cmake --build build/tests --target core_tests ecs_tests

# Запуск всех тестов
ctest --test-dir build/tests --output-on-failure

# Запуск только core или ecs
ctest --test-dir build/tests -R core
ctest --test-dir build/tests -R ecs
```

### Структура тестов

```
tests/
├── CMakeLists.txt          # Два executable: core_tests + ecs_tests
├── core/                   # Математика и типы (линкуется с vwengine)
└── ecs/                    # ECS: registry, pools, systems (линкуется с vwengine)
```

### Ручные тестовые приложения

Для интеграционного тестирования с графикой используются приложения:
- `test_window` — создание окна и обработка ввода
- `test_simple_model` — валидация рендеринга модели
- `test_math_matrix` — проверка математической библиотеки

## Типичные задачи разработки

### Добавление нового компонента
1. Создать заголовок в `engine/include/vw/gfx/world/components/`
2. Именование: `*_component.h` и `*_component.inl.h`
3. Добавить в tuple `world_components.h` при необходимости

### Добавление новой системы
1. Создать заголовок в `engine/include/vw/gfx/world/systems/`
2. Именование: `*_system.h` и `*_system.inl.h`
3. Реализовать метод `update()`, работающий с view registry

### Добавление инструмента Sculptor
1. Создать класс, наследующий `base_tool` в `apps/sculptor/src/tools/`
2. Реализовать виртуальные методы (render, обработчики событий)
3. Зарегистрировать в UI панели инструментов

### Добавление операции Sculptor (Undo/Redo)
1. Создать класс, наследующий `base_operation` в `apps/sculptor/src/operations/`
2. Реализовать методы `execute()` и `undo()`
3. Использовать через `operation_manager::execute()`

## Структуры данных

### Воксель
- Одно значение цвета, хранится как `uint32` (упакованный RGBA)
- Пустой воксель представлен значением цвета 0

### Модель
- 3D массив вокселей (ширина x высота x глубина)
- Линейное хранение: `x + y*width + z*width*height`

### Вершина (для GPU)
```cpp
struct vertex {
    vec3f position;  // 12 байт
    vec3f normal;    // 12 байт
    uint32 color;    // 4 байта (упакованный RGBA)
};  // Всего: 28 байт
```

## Справочник важных файлов

| Назначение | Файл |
|------------|------|
| Определения типов | `engine/include/vw/core/types.h` |
| Определение entity | `engine/include/vw/gfx/world/entity.h` |
| ECS Registry | `engine/include/vw/gfx/world/registry.h` |
| Vulkan контекст | `engine/include/vw/gfx/render/vulkan_context.h` |
| Основной рендерер | `engine/include/vw/gfx/render/renderer.h` |
| Воксельная модель | `engine/include/vw/gfx/model/model.h` |
| Цветовая палитра | `engine/include/vw/core/color.h` |
| Приложение Sculptor | `apps/sculptor/src/app/app.h` |
| Базовый инструмент | `apps/sculptor/src/tools/base_tool.h` |
| Базовая операция | `apps/sculptor/src/operations/base_operation.h` |

## Интеграция с Clangd

Проект настроен для clangd:
- Стандарт C++23
- Проверки ClangTidy (readability, performance, modernize, bugprone)
- Inlay hints включены
- Экспорт compile commands (`compile_commands.json`)

Отключённые проверки: `readability-magic-numbers`, `readability-identifier-length`, `bugprone-easily-swappable-parameters`

## Git Workflow

Текущий фокус разработки:
- Улучшения инструмента Sculptor (операции с моделями, формат файлов)
- Каскадные тени
- Поддержка платформ (macOS через MoltenVK)

### Ветки
- **Формат**: `kebab-case`, краткое описание задачи
- **Примеры**: `fix-shadow-artifacts`, `add-rotate-tool`, `refactor-ecs-registry`

### Коммиты
- **Язык**: английский (обязательно)
- **Формат**: `область: описание` (например, `sculptor: expand model operation`)
- **Области**: `engine`, `sculptor`, `docs`, `shaders`, `build`
