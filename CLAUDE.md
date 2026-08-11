# Voxel World

Воксельный движок на C++23/Vulkan. Движок + ECS + приложение Sculptor (редактор
вокселей). Идёт миграция на C++ модули — план и отчёты по фазам в `docs/`.

## Architecture

Движок наполовину модульный: `vw.core`, `vw.ecs` и `vw.world` — модульные
статические библиотеки, gfx пока header-only.

- **vw.core** (`engine/core/src/`, таргет `vw_core`) — типы, math, transform,
  лог, блоки, геометрия `vw::spatial`
- **vw.ecs** (`engine/ecs/src/`, таргет `vw_ecs`) — entity, type-erased пул,
  реестр с рантайм-идентификаторами компонентов
- **vw.world** (`engine/world/src/`, таргет `vw_world`) — модели, анимации,
  сериализаторы, компоненты, системы, сетка чанков. Собирается без Vulkan
- **gfx** (`engine/include/vw/gfx/`) — header-only Vulkan renderer, окно, камера,
  ImGui, debug; станет `vw.gfx` в M5
- **Apps** — `apps/sculptor/`, `apps/test_*`, `apps/arena/`
- **Shaders** — GLSL → SPIR-V (`shaders/`)

Остальное в `engine/include/` — шимы к модулям, удаляются в M6.

Пространства имён: `vw` (core), `vw::spatial` (геометрия), `vw::asset` (данные
ассетов), `vw::ecs` (реестр, мир, компоненты, системы), `vw::gfx`,
`vw::sculptor`. Модуль ≠ namespace: `vw.world` экспортирует и `vw::asset`, и
`vw::ecs`.

Undo/redo в Sculptor — command-паттерн через `base_operation`.

## Key Commands

Только генератор Ninja и только из окружения `vcvars64.bat`.

- `cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build/release`
- `ctest --test-dir build/release --output-on-failure`

Подробности — навык `cmake-build`.

## CRITICAL RULES

- ALWAYS trailing return type: `auto foo() -> int32;`
- ALWAYS типы проекта (`uint32`, `float32`) вместо встроенных и std
- ALWAYS минимум комментариев — код самодокументируемый
- ALWAYS запускай тесты после изменений в `engine/`
- NEVER форвард-объявляй сущности модулей вне их модуля — только импортируй
- NEVER исключения в горячих путях
- NEVER коммить и создавать ветки без прямой просьбы пользователя

## Skills

- **`cpp-style`** — стиль C++, устройство модульного кода, рецепты добавления
  компонентов, систем, инструментов и операций
- **`cmake-build`** — сборка, таргеты, опции, тесты, headless-конфигурация
- **`git-workflow`** — что агент делает сам, формат коммитов и веток

## Working Style

- Сначала план, потом код
- Маленькие изменения: один файл → тесты → следующий
- Используй субагентов для исследования кодовой базы
