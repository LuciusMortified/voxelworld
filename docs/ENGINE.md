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

## Раскладка исходников

Исходники каждой библиотеки разложены по смысловым каталогам внутри `src/`;
корень каталога держит только первичный интерфейс модуля.

| Модуль | Каталоги |
|---|---|
| `vw.core` | `types/`, `math/`, `utils/`, `spatial/`, `blocks/`, `log/` |
| `vw.ecs` | плоско: четыре юнита |
| `vw.world` | `asset/`, `components/`, `systems/`, `grid/`, `light/`, `spatial/` |
| `vw.platform` | `input/`, `window/` |
| `vw.gfx` | `camera/`, `resource/`, `render/`, `debug/`, `engine/` |

## Партиции

Партиция — один `.cppm`. Взаимно зависимые сущности обязаны лежать в одной
партиции: циклы между партициями запрещены. Имя партиции повторяет каталог:
`:systems.transform`, `:render.shadow_map`, `:model.occupancy`.

Крупные партиции — агрегаторы: они только реэкспортируют части (`:systems`,
`:components`, `:model`, `:anim`, `:light`, `:grid`, `:terrain`, `:serial`,
`:resource`, `:render`, `:renderer`, `:debug`, `:gpu_buffers`).

- **`vw.core`**: `:types`, `:vector`, `:matrix`, `:transform`, `:math`, `:color`,
  `:blocks`, `:timing`, `:log`, `:spatial` (aabb, plane, ray, frustum — они
  взаимно зависимы).
- **`vw.world`**: `:model.*` (identity, occupancy, links, light_field, volume,
  edit, chunk), `:anim.*` (keyframe, channel, clip, fsm), `:serial.*` (vox, voxa,
  storage, writer, scene), `:spatial`, `:components.*` (по группам компонентов),
  `:terrain.*` (generator, column, loader, perlin), `:light.*` (column, baker),
  `:grid.*` (visibility, chunk, world_grid), `:systems.*` (hooks плюс партиция
  на каждую систему); класс `world` — в первичном юните `world.cppm`.
- **`vw.platform`**: `:input`, `:event`, `:window`.
- **`vw.gfx`**: `:camera` и `:camera.*` (контроллеры), `:resource.*` (shader,
  buffer, deletion_queue, staging_buffer, palette/light/blob-буферы,
  light_grid, combined_buffer и его пул), `:meshing`, `:mesh_pool`,
  `:render.*` (vulkan_context, gpu_timer, shadow_map, cull_pipeline),
  `:renderer` с `:renderer.settings|uniforms|stats`, `:debug.*` (window,
  primitive), `:engine` с `:engine.app|stats|frame_recorder`, плюс
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

## Отладочная панель

`CTRL+F12` открывает Debug Tool. Главное окно держится маленьким: кадровые
числа, память, вызовы отрисовки и график пиков. Всё остальное — окна, которые
открывает его менюбар: `Stats` (Systems, Render, Buffers, World) и `Settings`
(View, Lighting, Shadows, Lights, Fog). Настройки рендера живут здесь, а не в
приложении: их правит движок, и дублировать их в каждом стенде значит держать
несколько разъезжающихся копий одних и тех же ползунков.

Панель — метод `debug_window`, а не отдельный класс: состояние у них общее
(максимумы метрик, флаги открытости), а тела разложены по имплементационным
юнитам одного модуля — `debug_window.cpp` (оболочка, меню, кадровые числа),
`debug_panels_stats.cpp` и `debug_panels_settings.cpp`.

Время систем меряет сам `world::update`: каждая система носит своё имя
(`system_name`), а `world_update_stats` держит массив в порядке кортежа. Замер
идёт всегда — метрика, которую надо включить, показывает не тот кадр, из-за
которого её открыли, — и попадает не только в окно Systems, но и в отчёт
бенчмарка отдельной таблицей `system (ms)` и секцией `systems` в JSON.

## Границы со сторонним кодом

Три места, где C API легален, и все три проверяет линт:

- `engine/platform/src/window/window.cpp` — GLFW создаёт `VkSurfaceKHR`; наружу
  сюрфейс уходит как `uint64`, чтобы Vulkan не попал в интерфейс платформы.
- `engine/gfx/src/render/renderer.cpp` — бэкенд `imgui_impl_vulkan` принимает
  только C-хэндлы, на границе стоят четыре `static_cast`.
- `engine/gfx/src/render/vulkan_context.cpp` — единственный файл, называющий
  `vk::detail`, за собственным аксессором `dispatcher()`.

ImGui и GLFW остаются заголовочными и живут только в global module fragment
имплементационных юнитов. Наружу из `vw.gfx` их типы не выходят: debug-окна
регистрируются колбэком, приложение зовёт ImGui через свой include.

## Сборка

Только генератор Ninja. На Windows — из окружения `vcvars64.bat`: `import std`
требует модуля стандартной библиотеки, а Clang, целящийся в MSVC-ABI, берёт его
из `std.ixx` от MSVC.

На Linux `import std` работает штатным путём CMake, без `std.ixx`: Clang с
libc++ приносит собственный std-модуль. Там собирается и headless-ядро — так
устроены санитайзерные джобы, — и полная конфигурация с `vw.platform`, `vw.gfx`
и приложениями. Последней нужен overlay-триплет `x64-linux-libcxx`
(`cmake/triplets/`), иначе зависимости приедут из vcpkg собранными libstdc++ и
ABI разъедется:

```
export CC=clang CXX=clang++          # тот же компилятор соберёт порты vcpkg
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_EXE_LINKER_FLAGS=-stdlib=libc++ \
      -DVCPKG_TARGET_TRIPLET=x64-linux-libcxx \
      -DVCPKG_OVERLAY_TRIPLETS=$PWD/cmake/triplets \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

Собирается — не значит проверена на ходу: GPU в CI нет, и `sculptor` под Linux
никто не запускал.

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

## Известные долги

- **`:resource` и `:render` экспортируются.** Формально это оставляет vk-типы в
  интерфейсе — как типы приватных членов. Перевести партиции во внутренние
  мешает `gfx::shadow_map::cascade_count`, который читает Sculptor. Назвать
  `vk::`-имя снаружи `vw.gfx` всё равно нельзя — это проверяет линт.
- **Системы обходят мир через `view`, а не `for_each`.** Итераторная обёртка
  стоит 0,50 мс против 0,32 мс на проход по 160 000 сущностей. На сегодняшних
  сценах `world_update` — 0,005 мс от кадра, поэтому перевод не приоритетен.
- **CI ни разу не прогонялся.** Workflow написан по известным требованиям
  сборки; проверяется первым пушем.

Способ мерить кадровое время — навык `render-bench`; что в этом времени ещё
стоит править — `docs/optimization-plan.md`. Продукт, ради которого всё это, —
`docs/PRD.md`.
