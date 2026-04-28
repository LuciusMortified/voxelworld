# Voxel World

Воксельный движок на C++23/Vulkan. Header-only движок + ECS + приложение Sculptor (редактор вокселей).

## Architecture
- **Engine**: header-only INTERFACE библиотека (`engine/include/vw/`)
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
- `cmake -S . -B build/release && cmake --build build/release --config Release` — сборка
- `cmake -S . -B build/tests -DCMAKE_TOOLCHAIN_FILE=C:/Users/lucius/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DVW_BUILD_APPS=OFF && cmake --build build/tests --target core_tests ecs_tests` — сборка тестов
- `ctest --test-dir build/tests --output-on-failure` — запуск всех тестов
- `ctest --test-dir build/tests -R core` / `-R ecs` — запуск по группе

## CRITICAL RULES
- ALWAYS используй trailing return type: `auto foo() -> int;`
- ALWAYS используй типы проекта (`vw::uint32` и т.д.) вместо std/built-in
- ALWAYS минимум комментариев — код самодокументируемый
- ALWAYS запускай тесты после изменений в `engine/`
- NEVER добавляй комментарии к реализациям в `.inl.h` / `.cpp`
- NEVER используй исключения в горячих путях
- Полные правила стиля: `.claude/rules/cpp-style.md`
- Рецепты разработки: `.claude/rules/dev-workflow.md`

## Git
- Коммиты на английском: `область: описание` (области: `engine`, `sculptor`, `docs`, `shaders`, `build`)
- Ветки: `kebab-case` (`fix-shadow-artifacts`, `add-rotate-tool`)

## Working Style
- Сначала план, потом код
- Маленькие изменения: один файл → тесты → следующий
- Используй субагентов для исследования кодовой базы
