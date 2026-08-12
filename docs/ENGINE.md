# Архитектура движка

Воксельный движок на C++23 и Vulkan. Всё — именованные модули C++: заголовков
движка не существует, потребитель пишет `import`.

## Модули

| Модуль | Каталог | Таргет | Содержимое |
|---|---|---|---|
| `vw.core` | `engine/core/src/` | `vw_core` | типы, векторы и матрицы, цвет, math+transform, логгер, блоки, геометрия `vw::spatial` |
| `vw.ecs` | `engine/ecs/src/` | `vw_ecs` | `entity`, пулы, реестр с рантайм-идентификаторами компонентов |
| `vw.world` | `engine/world/src/` | `vw_world` | модели, анимации, сериализаторы, компоненты, системы, сетка чанков |
| `vw.platform` | `engine/platform/src/` | `vw_platform` | окно, ввод, события; GLFW ровно в одном `.cpp` |
| `vw.gfx` | `engine/gfx/src/` | `vw_gfx` | рендер на Vulkan-Hpp, камера, ImGui, debug |

Приложения тоже модули: `vw.sculptor` (`apps/sculptor/`) и `vw.arena`
(`apps/arena/`). `apps/test_*` — по одному `main.cpp`.

Зависимости строго односторонние: `core ← ecs ← world ← gfx`, `platform` стоит
между `core` и `gfx`. `vw.world` собирается и тестируется без Vulkan — на этом
держится headless-конфигурация.

Модуль не равен пространству имён: `vw.world` экспортирует и `vw::asset`
(данные ассетов), и `vw::ecs` (мир, компоненты, системы).

## Партиции

Партиция — один `.cppm`. Взаимно зависимые сущности обязаны лежать в одной
партиции: циклы между партициями запрещены.

- **`vw.core`**: `:types`, `:vector`, `:matrix`, `:math`, `:color`, `:blocks`,
  `:timing`, `:log`, `:spatial` (aabb, plane, ray, frustum — они взаимно
  зависимы).
- **`vw.world`**: `:model`, `:anim`, `:serial`, `:index`, `:components`,
  `:terrain`, `:grid`, `:systems`; класс `world` и оба ECS-сериализатора — в
  первичном юните `world.cppm`.
- **`vw.platform`**: `:input`, `:event`, `:window`.
- **`vw.gfx`**: `:camera`, `:resource`, `:render`, `:renderer`, `:engine` плюс
  неэкспортируемая `:vk` (хелперы ошибок и единственное упоминание
  `vk::detail`).
- **`vw.sculptor`**: `:state`, `:operations`, `:services`, `:tools`, `:ui`,
  `:app`. Состояние вынесено в отдельную партицию, иначе граф зависимостей
  замыкается в цикл.

Интерфейс — в `.cppm`, тела нешаблонного кода — в `.cpp` (`module vw.x;`).
В интерфейсе остаются шаблоны, `constexpr` и то, что зовут пер-воксельно:
через границу модуля инлайнится только определение, лежащее в интерфейсе.

## Рендер

Vulkan-Hpp через официальный модуль `import vulkan` (Vulkan-Headers из vcpkg,
не из SDK). Конфигурация биндинга задаётся один раз в
`cmake/vw_vulkan_module.cmake` и в исходники не попадает: без исключений,
`std::expected` вместо `ResultValue`, без конструкторов (designated
initializers), без smart handles, динамический диспетчер.

Диспетчер инициализируется в три шага в `vulkan_context` — загрузчик, инстанс,
устройство. Из-за этого `vulkan-1` не линкуется вовсе.

Ошибки — по двухуровневой политике: `vk_must` роняет процесс с контекстом
(создание ресурсов), а вызовы с несколькими легальными кодами
(`acquireNextImageKHR`, `presentKHR`) разбирают их по месту.

Владение GPU-ресурсами — пулы и обёртки (`combined_buffer`, `mesh_pool`,
`page_pool`), без `vk::raii`: скоуповый RAII враждует с кадрами в полёте.

## Границы со сторонним кодом

Три места, где C API легален, и все три проверяет линт:

- `engine/platform/src/window.cpp` — GLFW создаёт `VkSurfaceKHR`; наружу сюрфейс
  уходит как `uint64`, чтобы Vulkan не попал в интерфейс платформы.
- `engine/gfx/src/renderer.cpp` — бэкенд `imgui_impl_vulkan` принимает только
  C-хэндлы, на границе стоят четыре `static_cast`.
- `engine/gfx/src/vulkan_context.cpp` — единственный файл, называющий
  `vk::detail`, за собственным аксессором `dispatcher()`.

ImGui и GLFW остаются заголовочными и живут только в global module fragment
имплементационных юнитов. Наружу из `vw.gfx` их типы не выходят: debug-окна
регистрируются колбэком, приложение зовёт ImGui через свой include.

## Сборка

Только генератор Ninja и только из окружения `vcvars64.bat` — `import std`
требует модуля стандартной библиотеки, а Clang здесь берёт его из `std.ixx` от
MSVC. Отсюда же следует, что сборка возможна только на Windows.

```
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
ctest --test-dir build/release --output-on-failure
```

Опции: `VW_BUILD_GFX`, `VW_BUILD_APPS`, `VW_BUILD_TESTS`. Headless —
`-DVW_BUILD_GFX=OFF -DVW_BUILD_APPS=OFF -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON`.

Зависимости: Catch2, GLFW, ImGui (glfw-binding, vulkan-binding), Vulkan-Headers.
Версии зафиксированы в `vcpkg.json` через `builtin-baseline` и `version>=`.

Гигиену интерфейсов проверяет `python scripts/lint_modules.py`.

## Что за пределами

Подробности каждой фазы миграции — в `docs/m0`…`docs/m6`. План и зафиксированные
решения — `docs/modules-migration-plan.md`.
