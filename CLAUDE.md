# Voxel World

Воксельный движок на C++23/Vulkan. Header-only движок + ECS + приложение Sculptor (редактор вокселей).

## Architecture
Идёт миграция на C++ модули (`docs/modules-migration-plan.md`). Движок сейчас в
переходном состоянии: `vw.core` — модульная статическая библиотека, остальное
пока header-only.

- **vw.core**: модуль (`engine/core/src/*.cppm` + `*.cpp`), таргет `vw_core`
- **Engine**: header-only INTERFACE библиотека (`engine/include/vw/`) — остальные модули
- **Apps**: компилируемые executable (`apps/sculptor/`, `apps/test_*`)
- **Shaders**: GLSL → SPIR-V (`shaders/`)
- **Модули движка** (namespace = путь):
  - `vw::core` — типы, math, transform, voxel, block_registry
  - `vw::spatial` — чистая геометрия (aabb, frustum, plane, ray)
  - `vw::asset` — модели, анимации, vox/voxa parsers, asset_storage
  - `vw::ecs` — entity/registry/world, компоненты, системы, world_grid, dynamic_aabb_tree
  - `vw::gfx` — Vulkan renderer, окно, камера, ImGui, debug
- **Undo/Redo**: command паттерн через `base_operation`
- **Deps**: vcpkg (glfw3, imgui, spdlog, catch2, Vulkan SDK)

## Key Commands
ТОЛЬКО генератор Ninja — Visual Studio не поддерживает `import std`. Собирать из
окружения Developer Command Prompt (нужен `VCToolsInstallDir`).
- `cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build/release` — сборка
- `cmake -S . -B build/tests -DCMAKE_TOOLCHAIN_FILE=C:/Users/lucius/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DVW_BUILD_APPS=OFF && cmake --build build/tests --target core_tests ecs_tests` — сборка тестов
- `ctest --test-dir build/tests --output-on-failure` — запуск всех тестов
- `ctest --test-dir build/tests -R core` / `-R ecs` — запуск по группе

## CRITICAL RULES
- ALWAYS используй trailing return type: `auto foo() -> int;`
- ALWAYS используй типы проекта (`vw::uint32` и т.д.) вместо std/built-in
- ALWAYS минимум комментариев — код самодокументируемый
- ALWAYS запускай тесты после изменений в `engine/`
- NEVER добавляй комментарии к реализациям в `.inl.h` / `.cpp`
- NEVER форвард-объявляй типы из `vw.core` — модульные сущности нельзя
  объявлять в глобальном модуле, только импортировать
- NEVER добавляй `import std` в модульные юниты до M6: пока потребители
  включают стандартные заголовки текстуально, это ломает сборку на MSVC
- NEVER используй исключения в горячих путях
- Полные правила стиля: `.claude/rules/cpp-style.md`
- Рецепты разработки: `.claude/rules/dev-workflow.md`

## Модульный код (`engine/core/`)
- Партиция = один `.cppm`. Взаимно зависимые сущности обязаны лежать в одной
  партиции — циклы между партициями запрещены (так `math` и `transform` вместе)
- Интерфейс в `.cppm`, реализация нешаблонного кода в `.cpp`; `.inl.h` не бывает
- Шаблоны и `constexpr` остаются в интерфейсной партиции
- В `.cpp` не пиши `inline` — определение обязано быть единственным
- Стандартная библиотека — через global module fragment (`module;` + `#include`),
  и тот же набор заголовков должен быть в `vw/core/detail/module_prelude.h`
- Старые заголовки `engine/include/vw/core/*.h` — шимы: прелюдия, затем
  `import vw.core;`. Удаляются в M6
- Настройки сборки доходят до кода как экспортированные `constexpr`, а не
  макросы: cache-переменная → `target_compile_definitions(... PRIVATE ...)` →
  `inline constexpr` в партиции. Образец — `vw::log::min_level`. Макрос в
  заголовке потребителя ломает ODR, если TU соберут его по-разному

## Git
- Коммиты на английском: `область: описание` (области: `engine`, `sculptor`, `docs`, `shaders`, `build`)
- Ветки: `kebab-case` (`fix-shadow-artifacts`, `add-rotate-tool`)

## Working Style
- Сначала план, потом код
- Маленькие изменения: один файл → тесты → следующий
- Используй субагентов для исследования кодовой базы
