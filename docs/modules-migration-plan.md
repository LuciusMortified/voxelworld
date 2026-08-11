# Voxelworld — миграция на статические библиотеки и C++ модули (M-план)

**Версия:** 1.0 · **Дата:** 2026-08-01
**База:** `wiredreamer/voxelworld @ master`
**Статус относительно перфоманс-плана:** выполняется **до** эпиков E1–E8. Все эпики перфоманс-рефакторинга после завершения M-плана ложатся в новую структуру библиотек (карта соответствия — в конце документа).

Цель: превратить header-only шаблонный движок в набор статических библиотек, каждая из которых является именованным C++ модулем; максимально сократить шаблонность (ECS — через де-шаблонизацию, остальное — через конверсию в имплементационные юниты); перевести рендер на Vulkan-Hpp с официальным модулем `import vulkan`.

Целевые тулчейны: **MSVC (свежая VS 17.x) и Clang (свежий мажор)**. GCC не является целевым компилятором. Стандарт: C++23 как база, фичи C++26 допускаются при поддержке обоими целевыми компиляторами (проверка feature-test макросами, не версией компилятора).

---

## Сводка фаз

| Фаза | Содержание | Риск | Выход |
|------|-----------|------|-------|
| M0 | Тулчейн-спайк: CMake 4.x, Ninja, `import std`, `import vulkan`, матрица MSVC+Clang | низкий | ✅ **выполнено**, отчёт: `docs/m0-toolchain-spike.md` |
| M1 | `vw.core`: типы, математика, лог (выпил spdlog), block_registry | низкий | ✅ **выполнено**, отчёт: `docs/m1-vw-core.md` |
| M2 | Де-шаблонизация ECS → `vw.ecs` | **высокий** | ✅ **выполнено** (кроме бенчмарка итерации), отчёт: `docs/m2-vw-ecs.md` |
| M3 | `vw.world`: вынос мира из vw::gfx, headless-сборка | средний | ✅ **выполнено**, отчёт: `docs/m3-vw-world.md` |
| M4 | `vw.platform`: окно/ввод, GLFW спрятан | низкий | ✅ **выполнено**, отчёт: `docs/m4-vw-platform.md` |
| M5 | `vw.gfx`: конверсия + миграция на Vulkan-Hpp | **высокий** | рендер на vk::, import vulkan |
| M6 | Приложения, тесты, зачистка, обновление CLAUDE.md | низкий | удалено старое include-дерево |

Порядок фаз строгий: M2 разблокирует M3 (мир нельзя вынести, пока он шаблонный), M3 разблокирует M5 (gfx должен зависеть от world, а не содержать его). M4 независима и может идти параллельно M3.

---

## Ключевые решения (зафиксированы, обоснование ниже по тексту)

| # | Решение | Выбор |
|---|---------|-------|
| D1 | Механизм модулей | Именованные модули C++20/23, партиции; `import std` в имплементационных юнитах — сразу, в интерфейсных партициях — **только с M6** (уточнено в M1), до тех пор их стандартная библиотека идёт через global module fragment |
| D2 | Vulkan-биндинги | Vulkan-Hpp через официальный модуль `import vulkan` (Khronos); требуется Vulkan-Headers ≥ 1.4.334 |
| D3 | Политика ошибок | Двухуровневая: исключения = фатальный путь (panic), `expected` = обрабатываемые; биндинг с `VULKAN_HPP_NO_EXCEPTIONS`, хелперы `vk_must`/`vk_expect` |
| D4 | RAII-слой | Без `vk::raii`; голые `vk::*`-хэндлы + собственные обёртки владения + очередь отложенной деструкции |
| D5 | Конструкторы структур | `VULKAN_HPP_NO_CONSTRUCTORS` — designated initializers |
| D6 | Диспетчеризация | `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1`, единый глобальный диспетчер |
| D7 | spdlog | Удаляется; собственный логгер на `std::format`/`std::print` в `vw.core:log` |
| D8 | imgui | Остаётся заголовочным, живёт только в имплементационных юнитах debug-слоя `vw.gfx` |
| D9 | GLFW | Остаётся заголовочным (C-библиотека), живёт только в имплементационных юнитах `vw.platform` |
| D10 | Макросы конфигурации движка | ~~Один традиционный заголовок `vw/config.h`~~ → **отменено в M1**: настройки сборки доходят до кода как экспортированные `constexpr` (первая — `vw::log::min_level`), макрос живёт только в `target_compile_definitions` соответствующей библиотеки. Заголовок удалён; вернётся, только если понадобятся асерты с `__FILE__`/`__LINE__` |
| D11 | Catch2 | Без изменений; тестовые TU — обычные единицы трансляции, импортирующие наши модули |

### D3 — двухуровневая политика ошибок

Философия проекта: исключение — легальный, но **фатальный** путь (нарушение инварианта, невозможность сконструировать объект), приводящий к остановке приложения; всё, что обрабатывается без падения, идёт через `std::expected`. Отсюда три правила:

1. **Исключения на уровне компилятора включены** (никаких `-fno-exceptions`/`/EHs-`). Собственный код вправе бросить из конструктора при нарушении инварианта. Конвенция: такие исключения нигде не ловятся, кроме единственного catch на границе `main()` — залогировать контекст и завершиться. Исключение = panic, не control flow.
2. **Биндинг Vulkan собирается с `VULKAN_HPP_NO_EXCEPTIONS`.** Дефолтная конфигурация Vulkan-Hpp бросает на *всех* кодах ошибок, включая рутинно-обрабатываемые (`eErrorOutOfDateKHR` при ресайзе свапчейна) — это навязало бы try/catch как control flow в прямое нарушение правила 1. `ResultValue` на границе возвращает нам решение о фатальности каждого кода.
3. **Два хелпера на границе vk::** материализуют оба пути:

```cpp
// фатальный путь: любой не-success = нарушение инварианта рендера.
// Возвращает значение напрямую — эргономика уровня исключений,
// но паника явная и с контекстом.
template <typename T>
auto vk_must(vk::ResultValue<T>&& rv, std::string_view what) -> T {
    if (rv.result != vk::Result::eSuccess) [[unlikely]] {
        log::error("vulkan: {} failed: {}", what, vk::to_string(rv.result));
        std::terminate();   // либо vw::panic() с дампом состояния кадра
    }
    return std::move(rv.value);
}

// рекавери-путь: ошибки, которые движок переживает
template <typename T>
auto vk_expect(vk::ResultValue<T>&& rv, std::string_view what)
    -> vw::expected<T> {
    if (rv.result != vk::Result::eSuccess) [[unlikely]] {
        return vw::unexpected(gfx_error{rv.result, what});
    }
    return std::move(rv.value);
}
```

Раскладка по коду: init-время и создание ресурсов — почти всегда `vk_must`; пер-кадровые вызовы с множественными легальными исходами (`acquireNextImageKHR`, `presentKHR`, `waitForFences`) обрабатывают свои коды (`eErrorOutOfDateKHR`, `eSuboptimalKHR`, `eTimeout`) явно по месту, остальное — `vk_expect`. Правило выбора: «может ли приложение осмысленно продолжить?» — да → `vk_expect`, нет → `vk_must`. Будущий headless-хост (E5) наследует ту же дисциплину.

### D4 — почему без vk::raii

Три причины, в порядке важности:

1. **Скоуповый RAII враждует с кадрами в полёте.** GPU-ресурс нельзя уничтожать в деструкторе в момент выхода из скоупа — он может использоваться кадрами N-1, N-2, которые GPU ещё исполняет. Движку нужна отложенная деструкция: хэндл паркуется в очередь удаления с номером кадра и уничтожается, когда fence кадра пройден. `vk::raii` с его «деструктор = vkDestroy» решает не ту задачу; в реальных движках владение живёт в пулах, аренах и deferred-очередях, а vk::raii остаётся инструментом сэмплов и утилит. У вас пуловое владение уже построено (page_pool, mesh_pool, combined_buffer) — vk::raii его не заменяет, а конфликтует с ним.
2. **Трение с NO_EXCEPTIONS.** RAII-конструкторы сообщают об ошибках исключениями; в no-exceptions конфигурации этот слой Vulkan-Hpp исторически самый шершавый. Комбинация D3+vk::raii — худшее сочетание из возможных.
3. **Миграция M5 остаётся механической.** `VkBuffer` → `vk::Buffer`, `vkCmdDraw(cb, ...)` → `cb.draw(...)` — построчная замена с сохранением архитектуры владения. С vk::raii пришлось бы параллельно перепроектировать владение всех объектов — второй большой рефакторинг внутри и так самой рискованной фазы.

Что берём от Vulkan-Hpp вместо raii: типобезопасные хэндлы и enum'ы, member-function синтаксис, designated initializers (D5), structure chains для pNext, единый диспетчер (D6). Очередь отложенной деструкции — своя (эскиз в M5).

### D6 — единый динамический диспетчер

`VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1`: все вызовы идут через `VULKAN_HPP_DEFAULT_DISPATCHER`, инициализируемый в три шага (loader → instance → device). Плюсы: функции расширений доступны без ручного `vkGetDeviceProcAddr`-бойлерплейта; device-специфичные указатели минуют трамплин лоадера (микровыигрыш на CPU-стороне при высокой частоте вызовов); конфигурация обязана совпадать у модуля и потребителей — задаётся `PUBLIC` на CMake-таргете модуля, что делает рассинхрон невозможным по построению.

---

# M0. Тулчейн-спайк

Цель фазы — вскрыть все инфраструктурные сюрпризы на пустом скелете, до того как тронут хоть один файл движка. Делается в отдельной ветке, результат — либо зелёная матрица, либо список блокеров с обходами.

## Состав

1. **CMake ≥ 4.0** (текущие 3.16/3.25 в корне и engine — поднять), генератор **Ninja**. Модульное сканирование зависимостей на MSVC+Clang через Ninja — самый отлаженный путь.
2. Скелет `vw_core_spike`: одна интерфейсная единица + одна имплементационная, `import std`, экспорт пары функций.
3. Скелет потребителя: исполняемый таргет, `import vw.core.spike;` + Catch2-тест.
4. Прекомпиляция модуля `vulkan` из Vulkan-Headers и smoke-тест `import vulkan;` с созданием `vk::ApplicationInfo`.
5. Прогон всей матрицы: {MSVC, Clang} × {Debug, Release}.

## Эскиз CMake

```cmake
# корневой CMakeLists.txt
cmake_minimum_required(VERSION 4.0)
project(voxelworld CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_MODULE_STD ON)          # import std для всех таргетов проекта
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

```cmake
# библиотека-модуль (образец для всех vw_*)
add_library(vw_core STATIC)
target_sources(vw_core
    PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES
        src/core.cppm          # первичный интерфейс
        src/types.cppm         # партиция :types
        src/math.cppm          # партиция :math
        src/log.cppm           # партиция :log
    PRIVATE
        src/log.cpp            # имплементационные юниты
        src/blocks.cpp
)
target_compile_features(vw_core PUBLIC cxx_std_23)
```

```cmake
# официальный модуль Vulkan-Hpp (vulkan.cppm поставляется с SDK / Vulkan-Headers)
find_package(Vulkan REQUIRED)
add_library(VulkanHppModule STATIC)
target_sources(VulkanHppModule PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${Vulkan_INCLUDE_DIR}
    FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm)
target_compile_definitions(VulkanHppModule PUBLIC
    VULKAN_HPP_NO_EXCEPTIONS            # D3
    VULKAN_HPP_NO_CONSTRUCTORS          # D5
    VULKAN_HPP_NO_SMART_HANDLE          # D4: UniqueHandle не используем
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1  # D6, обязан быть PUBLIC
)
target_link_libraries(VulkanHppModule PUBLIC Vulkan::Headers)
target_compile_features(VulkanHppModule PUBLIC cxx_std_23)
```

Все конфигурационные макросы Vulkan-Hpp заданы `PUBLIC` на одном таргете — потребители физически не могут разойтись с модулем по конфигурации (пункт из документации Khronos о consistency выполняется по построению).

## Известные точки риска спайка

- **Clang на Windows**: clang-cl поверх MS STL против clang+libc++. `import std` на связке clang-cl+MS STL исторически отставал. Спайк обязан дать ответ, какая конфигурация Clang рабочая; при проблемах с clang-cl — Clang-сборка живёт на libc++ (полезный побочный эффект: репетиция Linux-хоста из E5).

  **Результат M0:** libc++ не нужен — Clang собирает MS STL `std.ixx` и корректно потребляет `import std`. Но драйвер обязан быть `clang++`, а не `clang-cl`: CMake включает сканирование графа импортов только при GNU-подобном фронтенде. Модуль `std` для Clang собирается собственным таргетом, так как штатный `CMAKE_CXX_MODULE_STD` покрывает лишь MSVC и Clang+libc++.
- **BMI-гигиена**: одинаковые флаги стандарта на всех таргетах (глобальные переменные выше это дают). Проверить, что смена флага корректно инвалидирует BMI, а не даёт загадочные ошибки.
- **clangd/IntelliSense**: зафиксировать фактическое состояние подсветки по модульному коду на свежих версиях; принять как известное ограничение, не как блокер.
- **ccache/sccache**: проверить поведение кэша с модульными TU; при деградации — исключить модульные юниты из кэширования (их и так мало).

## Приёмка M0 — ✅ пройдена

- ✅ Матрица {MSVC, Clang} × {Debug, Release} зелёная: скелет + `import std` + `import vulkan` + Catch2-тест.
- ✅ Замерены и записаны: время полной сборки скелета, время инкрементальной сборки после правки интерфейса и после правки имплементации; дополнительно снят базлайн текущего header-only движка.
- ✅ Задокументирован выбор Clang-конфигурации: `clang++` + MS STL + собственный модуль `std`.

Подробности, цифры и находки — `docs/m0-toolchain-spike.md`.

---

# M1. `vw.core`

Первая боевая библиотека. Состав: `vw/core/types.h`, математика, лог, block_registry, базовые утилиты (`expected`, хэши, slot-идиома из model_identity_pool — она понадобится всем).

## Структура модуля

```cpp
// src/core.cppm — первичный интерфейс: только реэкспорт партиций
export module vw.core;
export import :types;
export import :math;
export import :log;
export import :blocks;
```

```cpp
// src/math.cppm — партиция, полностью в интерфейсе (шаблоны математики остаются)
export module vw.core:math;
import std;
import :types;

export namespace vw {

template <typename T>
struct vec3 { /* без изменений */ };

using vec3f = vec3<float32>;
using vec3i = vec3<int32>;
// ...

}  // namespace vw
```

Математика — пример шаблонов, которые **остаются**: они стабильны, header-only по природе и экспортируются из интерфейсной партиции без потери преимуществ модулей (BMI кэширует разобранный шаблон).

## Выпил spdlog (D7)

Текущее состояние: spdlog живёт в одном файле (`log/logger.inl.h`) и уже сконфигурирован на `SPDLOG_USE_STD_FORMAT` — то есть от библиотеки используется только диспетчеризация в синки. Собственный логгер на C++23:

```cpp
// src/log.cppm
export module vw.core:log;
import std;

export namespace vw::log {

enum class level : uint8 { trace, debug, info, warn, error };

void set_level(level lvl);
void write(level lvl, std::string_view msg);  // имплементация в log.cpp

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
    write(level::info, std::format(fmt, std::forward<Args>(args)...));
}
// trace/debug/warn/error аналогично

}  // namespace vw::log
```

Имплементация (`log.cpp`): форматирование в вызывающем потоке (уже сделано шаблоном выше), MPSC-очередь строк, фоновый поток с консольным (`std::print` + цвет по level) и файловым синком, flush по таймеру и на `error`. ~150–200 строк. Compile-time отсечение уровней — через экспортированную из модуля константу: `if constexpr (lvl >= min_level)`, где `min_level` задаётся cache-переменной `VW_LOG_MIN_LEVEL` (см. отмену D10).

Зависимость spdlog удаляется из vcpkg.json по завершении фазы.

## Приёмка M1 — ✅ пройдена

- ✅ `vw_core` собирается как модуль на обоих компиляторах; остальной движок потребляет его смешанным режимом через шимы.
- ✅ spdlog удалён; лог-вывод функционально эквивалентен (уровни, файл, цвет консоли).
- ✅ Тесты core переведены на `import vw.core` и зелёные.

Уточнение переходного режима, полученное в M1: шим обязан воспроизвести
include-набор global module fragment модуля **до** `import` — MSVC сливает эти
объявления с текстовыми включениями потребителя только в таком порядке. Общий
набор вынесен в `vw/core/detail/module_prelude.h`. Подробности и метрики —
`docs/m1-vw-core.md`.

---

# M2. Де-шаблонизация ECS → `vw.ecs`

Самая содержательная фаза. Вся вирусная шаблонность движка растёт из параметра `WC` (кортеж компонентов): `world<WC>` → `chunk<WC>` → `world_grid<WC>` → системы → `entity_guard<WC>`. Цель фазы — нешаблонное ECS-ядро с рантайм-регистрацией компонентов; типизированность остаётся тонкими шаблонными обёртками на границе API.

Побочная, но стратегическая выгода: рантайм-регистрация компонентов — необходимое условие будущего mod API (скриптовый мод не может участвовать в компайл-тайм кортеже, а зарегистрировать компонент по `(size, align, деструктор)` в рантайме — может).

## 2.1 Ядро: type-erased пул

```cpp
// src/ecs/pool.cppm
export module vw.ecs:pool;
import std;
import vw.core;

export namespace vw::ecs {

struct component_ops {
    uint32 size;
    uint32 align;
    void (*destroy)(void*);                    // nullptr для trivially destructible
    void (*relocate)(void* dst, void* src);    // move + destroy src
};

class component_pool {
public:
    explicit component_pool(component_ops ops);

    auto emplace(entity e) -> void*;   // память под компонент (конструирует вызывающий)
    auto get(entity e) -> void*;       // nullptr если нет
    auto get(entity e) const -> const void*;
    void remove(entity e);
    auto size() const -> uint32;

    // плотная итерация: сырой массив + параллельный массив entity
    auto raw_data() -> void*;
    auto entities() const -> std::span<const entity>;

private:
    component_ops ops_;
    std::vector<std::byte> dense_;      // size() * ops_.size, плотно
    std::vector<entity> dense_entities_;
    // sparse-индексация entity -> dense index — сохранить текущую схему
    // (страничный sparse-массив; generation в entity уже есть)
};

}  // namespace vw::ecs
```

Ключевое: раскладка данных не меняется — компоненты как лежали плотными массивами, так и лежат. Меняется только то, что тип стёрт из хранилища и восстанавливается на границе.

## 2.2 Рантайм-идентификаторы и реестр

```cpp
// src/ecs/registry.cppm
export module vw.ecs:registry;
import std;
import vw.core;
import :pool;

namespace vw::ecs::detail {
inline std::atomic<uint32> next_component_id{0};
}

export namespace vw::ecs {

template <typename T>
auto component_id_of() -> uint32 {
    static const uint32 id = detail::next_component_id.fetch_add(1);
    return id;
}

template <typename T>
consteval auto ops_of() -> component_ops {
    return {
        .size = sizeof(T), .align = alignof(T),
        .destroy = std::is_trivially_destructible_v<T>
            ? nullptr
            : +[](void* p) { static_cast<T*>(p)->~T(); },
        .relocate = +[](void* dst, void* src) {
            std::construct_at(static_cast<T*>(dst), std::move(*static_cast<T*>(src)));
            std::destroy_at(static_cast<T*>(src));
        },
    };
}

class registry {
public:
    // --- типизированная поверхность: тонкие однострочные шаблоны ---
    template <typename T, typename... Args>
    auto emplace(entity e, Args&&... args) -> T& {
        auto& pool = ensure_pool(component_id_of<T>(), ops_of<T>());
        return *std::construct_at(static_cast<T*>(pool.emplace(e)),
                                  std::forward<Args>(args)...);
    }

    template <typename T>
    auto get(entity e) -> T* {
        auto* pool = try_pool(component_id_of<T>());
        return pool ? static_cast<T*>(pool->get(e)) : nullptr;
    }

    template <typename T>
    void remove(entity e) { /* аналогично */ }

    // --- view: итерация по пересечению компонентов ---
    template <typename... Ts, typename Fn>
    void for_each(Fn&& fn) {
        // ведущий пул — наименьший; указатели на dense кэшируются ОДИН раз,
        // внутри цикла — только индексная арифметика, без виртуальности
        ...
    }

    // --- нетипизированная поверхность (mod API, сериализация) ---
    auto ensure_pool(uint32 component_id, component_ops ops) -> component_pool&;
    auto try_pool(uint32 component_id) -> component_pool*;

    // entity lifecycle: без изменений против текущего (generation-хэндлы)
    auto create() -> entity;
    void destroy(entity e);

private:
    std::vector<std::unique_ptr<component_pool>> pools_;  // индекс = component_id
};

}  // namespace vw::ecs
```

Производительность итерации: `for_each` кэширует `raw_data()` ведущего пула один раз за вызов — внутри горячего цикла та же индексная арифметика по плотному массиву, что и сейчас. Косвенность появляется на входе в итерацию (один раз), не на каждом элементе. На масштабах проекта (тысячи сущностей) разница неизмерима; если какая-то система когда-то станет горячей — для неё вводится типизированный view с сортировкой (EnTT-стиль owning group), это локальное расширение, не смена парадигмы.

## 2.3 Каскад де-шаблонизации

После того как `registry` стал конкретным классом, механически конкретизируются:

| Было | Становится |
|------|-----------|
| `world<WC>` | `class world` (композиция: registry + системы) |
| `chunk<WC>` | `class chunk` |
| `world_grid<WC>` | `class world_grid` |
| `entity_guard<WC>`, `entity_guard_group<WC>` | конкретные классы |
| `*_system<WC>` (все системы) | конкретные классы с `.cpp` |
| `entity_archetype<...>` | заменяется функцией-фабрикой над registry |
| `base_world_components` (кортеж) | удаляется; компоненты регистрируются лениво при первом emplace |

Порядок работ внутри фазы:

1. `component_pool` + тесты (включая нетривиальные типы, relocate, генерации).
2. `registry` c типизированной поверхностью; дифф-тесты против текущего реестра (одинаковое поведение lifecycle/view на общем наборе сценариев).
3. Конкретизация `world`/`chunk`/`entity_guard` — правки в основном стирание `<WC>` из сигнатур.
4. Системы: перенос тел из `.inl.h` в `.cpp` (пока в старом include-дереве — модульное оформление придёт в M3/M5 по принадлежности).
5. Удаление `base_world_components`, `entity_archetype`; фабрики сущностей.
6. Прогон всех тестов; бенчмарк итерации view до/после (ожидание: в пределах шума).

## Приёмка M2 — ✅ пройдена, кроме бенчмарка

- ✅ Ни один заголовок движка не содержит параметра `WC`/`WD` (в коде он звался `WD`).
- ✅ Все тесты ECS и мира зелёные; поведение lifecycle (create/destroy/generation) сохранено.
- ❌ Бенчмарк системной итерации: деградация 117% на `view`, 38% на `for_each` при цели ≤10%. Причина не в модулях и не в стирании типа — измерено; разбор в `docs/m2-vw-ecs.md`.
- ✅ Есть нетипизированный путь регистрации компонента (задел mod API), покрытый тестом.

Отклонения от эскиза фазы: `entity_archetype` не переписан на рантайм-идентификаторы, а удалён — sparse-set в пуле уже даёт O(1) на тот же вопрос. `component_change_deps` заменён на данные (`registry::add_change_dep`). Тела систем переехали в новый таргет `vw_world`, который в M3 станет модулем.

---

# M3. `vw.world`

Вынос мирового состояния из `vw::gfx` в самостоятельную headless-библиотеку. Это одновременно задача 1 эпика E5 (там она называлась «выделение world_state») — выполняя её здесь, E5 получает готовый фундамент.

## Состав

Переезжают: `model` (воксельные данные), `page_pool`, `chunk`, `world_grid`, `gen_column`/`terrain_generator`, block-related части, сериализаторы vox/voxa (они про данные, не про рендер), компоненты мира (`transform`, `spatial`). Остаются в gfx: `model_component` (связь с мешем), рендерные компоненты, mesh_pool.

Развязка «мир → рендер»: мир не знает о мешах. Вместо прямых вызовов ремеша мир ведёт **dirty-set** — множество чанков, изменённых за тик; gfx опрашивает и планирует ремеш сам:

```cpp
// vw.world: часть интерфейса world_grid
export namespace vw::ws {

class world_grid {
public:
    // ... set_voxel, get_voxel, колонки — как сейчас, но конкретные типы
    auto drain_dirty_chunks() -> std::vector<vec3i>;  // забрать и очистить
private:
    std::vector<vec3i> dirty_chunks_;
};

}
```

(В E2 этот же механизм станет приоритетной очередью — интерфейс `drain` сохранится.)

## Структура модуля

```cpp
export module vw.world;
export import :model;       // воксельные данные, страницы, палитры
export import :anim;        // клипы, треки, слои, FSM
export import :serial;      // vox/voxa
export import :index;       // слои и дерево широкой фазы
export import :components;  // компоненты мира
export import :terrain;     // генератор и загрузчик колонок
export import :grid;        // chunk, world_grid
export import :systems;     // системы
// класс world живёт в этом же первичном юните
```

Генерация в потоках (`gen_thread_function`) остаётся внутри библиотеки — `std::jthread` и очереди не требуют ничего из gfx.

## Приёмка M3

- Таргет `vw_world` линкуется и тестируется **без** Vulkan, GLFW, imgui — отдельный CI-джоб без GPU-зависимостей зелёный.
- `import vw.world` ни транзитивно, ни явно не тащит vk::/GLFW-символы (проверка: тестовый TU с `import vw.world;` собирается при отсутствии Vulkan SDK в окружении CI-джоба).
- Sculptor и приложения работают в смешанном режиме (модули core/ecs/world + header-only gfx).

✅ **выполнено.** Конфигурация `-DVW_BUILD_GFX=OFF -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON` собирается и проходит 140/140 без единой GPU-зависимости; `world_tests` импортирует `vw.world` напрямую и зависит только от CRT. Геометрия `vw::spatial` при этом уехала в `vw.core:spatial`, а не в мир, — обоснование в отчёте. Dirty-set не понадобился: мир и так ничего не знает о мешах.

---

# M4. `vw.platform`

Окно, ввод, поверхность. GLFW включается ровно в один файл сегодня — фаза короткая и служит разминкой перед M5.

```cpp
// src/platform.cppm
export module vw.platform;
import std;
import vw.core;

export namespace vw::plat {

class window {
public:
    static auto create(const window_desc& desc) -> vw::expected<window>;
    auto poll_events() -> void;
    auto framebuffer_size() const -> vec2i;
    auto native_handle() const -> void*;   // GLFWwindow*, opaque для потребителей
    // колбэки ввода — как сейчас, но типы свои
};

}
```

```cpp
// src/platform.cpp — единственное место с GLFW
module;
#include <vulkan/vulkan.h>       // C-декларации нужны GLFW для сюрфейса
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
module vw.platform;
import std;

// glfwCreateWindowSurface и вся оконная механика — здесь
```

Тонкость: создание `VkSurfaceKHR` требует C-типов Vulkan рядом с GLFW. C-заголовок в global module fragment имплементационного юнита сосуществует с `import vulkan` в остальном коде без конфликтов (C-декларации присоединяются к глобальному модулю; хэндлы vk::SurfaceKHR ↔ VkSurfaceKHR взаимно конвертируемы). Функция создания сюрфейса экспортируется из vw.platform как принимающая/возвращающая opaque `uint64`/свой тип, чтобы Vulkan не попал в интерфейс платформы.

## Приёмка M4

- `grep -r "GLFW" --include="*.cppm"` пуст (GLFW только в .cpp).
- Sculptor работает на новой оконной обвязке.

✅ **выполнено.** GLFW остался в одном юните — `engine/platform/src/window.cpp`;
в `.cppm` слово встречается только в двух комментариях. Namespace стал
`vw::plat`, сюрфейс уходит наружу сырым `uint64`, нативный хэндл — `void*`.
Колбэки бэкенда добираются до приватных хуков окна через
`friend struct detail::window_callbacks`. Sculptor и `test_window` запущены и
работают.

---

# M5. `vw.gfx` + миграция на Vulkan-Hpp

Самая большая фаза: ~25 файлов с Vulkan-кодом конвертируются в имплементационные юниты **и одновременно** переводятся с C API на `vk::`. Совмещение сознательное: оба изменения трогают каждую строку с Vulkan-вызовом, делать их в два прохода — двойная работа. Именно из-за совмещения оценку фазы следует закладывать как две «обычных» фазы.

## 5.1 Порядок внутри фазы: по партициям, снизу вверх

```text
:resource   combined_buffer, staging, palette_buffer, mesh, mesh_pool, текстуры
:spatial    transform_buffer, пространственные структуры
:render     vulkan_context, renderer, пайплайны, shadow_map, debug/imgui
```

Interop C ↔ Hpp бесшовный (`vk::Buffer` ↔ `VkBuffer` — explicit-конверсии), поэтому мигрировать можно партицию за партицией при живом рендере: смигрированный `:resource` отдаёт `vk::Buffer`, ещё-не-мигрированный renderer берёт `static_cast<VkBuffer>(...)` на переходный период.

## 5.2 Инициализация диспетчера (D6)

Три шага, строго по порядку, в vulkan_context. Уточнено по итогам M0: хранилище диспетчера определяет сам модуль, поэтому макрос `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` в наших TU не нужен (и даёт дубликат символа при линковке), а безаргументный `init()` сам находит загрузчик — C-заголовки не требуются:

```cpp
// src/render/vulkan_context.cpp
module vw.gfx;
import vulkan;
import std;

namespace {
auto dispatcher() -> vk::detail::DispatchLoaderDynamic& {
    return vk::detail::defaultDispatchLoaderDynamic;
}
}

auto vulkan_context::init() -> vw::expected<void> {
    // 1) загрузчик: сам найдёт vulkan-1.dll / libvulkan.so
    dispatcher().init();

    // 2) instance-функции
    auto instance_rv = vk::createInstance({...});
    if (instance_rv.result != vk::Result::eSuccess) { return ...; }
    instance_ = instance_rv.value;
    dispatcher().init(instance_);

    // 3) device-функции (после выбора physical device и createDevice)
    dispatcher().init(device_);
    return {};
}
```

С динамическим диспетчером линковка `vulkan-1` из engine/CMakeLists больше не нужна (loader находится в рантайме) — зависимость линковки заменяется на `Vulkan::Headers` + `VulkanHppModule`.

## 5.3 Стиль кода после миграции (D3+D5)

```cpp
// было (C API):
VkBufferCreateInfo info{};
info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
info.size = size;
info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
VkBuffer buffer;
if (vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS) { ... }

// стало (D5: designated initializers, sType проставляется библиотекой;
// D3: создание ресурса — фатальный путь, vk_must возвращает значение напрямую):
auto buffer = vk_must(device_.createBuffer({
    .size  = size,
    .usage = vk::BufferUsageFlagBits::eVertexBuffer
           | vk::BufferUsageFlagBits::eTransferDst,
}), "create vertex buffer");
```

pNext-цепочки — через `vk::StructureChain` (типобезопасность цепочек — одно из главных практических преимуществ над C API, ловит несовместимые pNext на компиляции).

## 5.4 Отложенная деструкция (D4)

Замена скоупового владения для GPU-объектов:

```cpp
// src/render/deletion_queue.cppm (партиция :render, НЕ экспортируется из vw.gfx)
class deletion_queue {
public:
    void defer(uint64 frame, vk::Buffer b)       { push(frame, b); }
    void defer(uint64 frame, vk::ImageView v)    { push(frame, v); }
    // ... по типам; хранение — tagged union {тип, uint64 handle}

    // вызывается, когда fence кадра completed_frame пройден
    void collect(vk::Device device, uint64 completed_frame);
};
```

Обёртки уровня ресурсов (`combined_buffer`, `mesh_pool`, текстуры) в деструкторах не зовут `vkDestroy*` напрямую, а паркуют хэндлы в очередь. Сегодняшние места с `vkDeviceWaitIdle` перед освобождением заменяются на defer — это заодно убирает stalls (синергия с E2).

## 5.5 imgui (D8)

imgui остаётся заголовочным и живёт в двух имплементационных юнитах debug-слоя. Бэкенд `imgui_impl_vulkan` работает с C-типами — на границе передаются `static_cast`-нутые хэндлы. Наружу из `vw.gfx` imgui-типы не выходят: debug-окна регистрируются колбэком `std::function<void()>`, внутри которого приложение зовёт ImGui через отдельный include (Sculptor и так это делает).

## 5.6 Интерфейсная гигиена

Дополнительное правило, подтверждённое M0: **в наших исходниках нет ни одного макроса Vulkan-Hpp**. Конфигурация (`VULKAN_HPP_NO_EXCEPTIONS`, `NO_CONSTRUCTORS`, `DISPATCH_LOADER_DYNAMIC=1`) задаётся только через `target_compile_definitions` на таргете модуля, а `VULKAN_HPP_DEFAULT_DISPATCHER` не используется — вместо него прямое обращение к экспортированной `vk::detail::defaultDispatchLoaderDynamic`. TU, работающие с Vulkan, не включают заголовков вообще.

Отсюда следует ещё одно правило: **`vk::detail::` упоминается ровно в одном файле — `vulkan_context.cpp`**, за собственным аксессором `dispatcher()`. Не-`detail` альтернативы нет (и тип диспетчера, и его хранилище живут в `detail`, а официальный макрос — лишь алиас на то же имя), поэтому смысл имеет не написание, а локализация: Khronos уже переносили эти символы однажды, и следующий переезд должен стоить одну строку. Во всех остальных TU `vk::detail` запрещён — тем же линтом, что проверяет интерфейсную гигиену.

Правило (закрепить линтом в CI): `import vulkan;` разрешён только в имплементационных юнитах и неэкспортируемых партициях `vw.gfx`; в экспортируемых интерфейсах vk-типы не появляются. Публичная поверхность gfx оперирует своими типами (`vw::gfx::buffer_handle`, дескрипторы форматов) — по факту это уже так, миграция это правило только формализует.

## Приёмка M5

- Весь рендер на `vk::`; `grep -rn "vkCreate\|vkCmd\|VkBuffer\|VkImage" src/` вне vulkan_context-бутстрапа пуст.
- Нет `vkDeviceWaitIdle` в путях освобождения ресурсов (только shutdown) — владение через deletion_queue.
- Валидационные слои чистые на сценарии: запуск → генерация мира → полёт → правки → ресайз окна → выход.
- Кадровая производительность не хуже базлайна M0-замера (vk:: — zero-cost обёртка, деградация недопустима).
- Линт интерфейсной гигиены в CI зелёный.

---

# M6. Приложения, тесты, зачистка

1. Sculptor и все тесты переводятся на чистые `import` (шимы-заголовки из переходного режима удаляются).
2. Удаляется старое include-дерево `engine/include` (то, что не стало модульными исходниками).
3. vcpkg.json: удалён spdlog; зафиксированы версии остальных.
4. CI: матрица {MSVC, Clang} × {Debug, Release} + headless-джоб `vw_world` без GPU + линт интерфейсной гигиены.
5. **CLAUDE.md переписывается** — половина текущих конвенций описывает header-only идиомы (`.inl.h`-пары, include-порядок). Новые конвенции: структура модуля (интерфейсная партиция ↔ имплементационные юниты), правила экспорта, правило про настройки сборки как `constexpr` вместо макросов (отмена D10), правило интерфейсной гигиены vk::, шаблоны «только на поверхности API».
6. docs/ENGINE.md обновляется под новую структуру каталогов.

## Приёмка M6

- В репозитории нет `.inl.h`-файлов и не осталось ни одного `#include` наших собственных заголовков.
- Полный тестовый прогон зелёный на обоих компиляторах.
- Замеры против базлайна M0/до-миграции: полная сборка, инкрементальная после правки одного интерфейса, инкрементальная после правки одной имплементации (ожидание: имплементационные правки перестают пересобирать потребителей вообще).

---

# Сквозное: переходный режим, риски, метрики

## Переходный режим «модули + заголовки»

Между фазами движок живёт в смешанном состоянии: готовые библиотеки потребляются через `import`, остальное — через старые include. Это легально и устойчиво, правила простые:

- Старый заголовок мигрировавшей части превращается в шим: `import vw.core;` (+ `using`-алиасы при переименованиях) — потребители не правятся до своей фазы.
- Один TU может и импортировать модули, и включать заголовки; C-заголовки третьих сторон при необходимости — в global module fragment этого TU.
- Обратное запрещено: модульный код не включает старые заголовки движка (иначе граф зависимостей фаз ломается). Направление всегда «новое ← старое».

## Реестр рисков

| Риск | Вероятность | Смягчение |
|------|-------------|-----------|
| clang-cl + import std нерабочий | средняя | M0 решает заранее; фолбэк clang+libc++ |
| Скрытые циклы включений в .inl.h вскрываются как запрещённые циклы импортов | высокая | буфер в оценках M3/M5; разрыв циклов через выделение общей партиции |
| IntelliSense/clangd врёт на модулях | высокая | принято как ограничение; истина — сборка; свежие версии тулинга |
| vk::raii-соблазн в процессе M5 | — | D4 зафиксирован документом; deletion_queue делается первой задачей M5 |
| M2 ломает поведение ECS тонко (генерации, порядок деструкции) | средняя | дифф-тесты lifecycle до начала конкретизации; бит-в-бит сценарии |
| Оценка M5 оптимистична | высокая | явно заложено ×2 против механической конверсии; партиционная миграция позволяет остановиться в консистентном состоянии |

## Метрики (снимаются в M0 как базлайн, далее на каждой фазе)

| Метрика | Цель к M6 |
|---------|-----------|
| Полная пересборка (Release, оба компилятора) | не хуже базлайна ×1.2 (модуль vulkan прекомпилируется один раз — ожидаем лучше) |
| Инкрементальная: правка имплементации (.cpp) | пересборка 1 TU + линковка; потребители не пересобираются |
| Инкрементальная: правка интерфейса одной партиции | пересборка партиции + прямых потребителей, не всего мира |
| Headless-сборка vw_world | собирается и тестируется без Vulkan SDK |
| Валидационные слои | 0 ошибок на эталонном сценарии |
| FPS эталонной сцены | ≥ базлайна (vk:: обязан быть zero-cost) |

## Карта соответствия с перфоманс-планом (E1–E8)

После M-плана эпики ложатся так:

| Эпик | Где живёт после миграции |
|------|--------------------------|
| E1 binary meshing | `vw.gfx:resource` (мешер) + occupancy-структуры в `vw.world:model` |
| E2 бюджеты/transfer queue | `vw.gfx:render` (очереди Vulkan) + общая приоритетная очередь в `vw.core` |
| E3 формат вокселя v2 | `vw.world:model` + шейдеры |
| E4 LOD-кольца | `vw.world:grid` (кольца, даунсэмпл) + `vw.gfx` (юбки) |
| E5 журнал правок | `vw.world:journal` — фундамент (вынос мира) уже сделан в M3 |
| E6 occlusion | `vw.gfx:render` + связность в `vw.world` |
| E7 тени | `vw.gfx:render` |
| E8 контеншен | `vw.world` (page_pool кэши) + `vw.core` (task system) |

Замечание по E2/M5: если в M5 руки уже в vulkan_context, завести transfer-семейство очередей (задача 4 эпика E2) дешевле сразу — вынести её из E2 в хвост M5 по ситуации.

---

*Документ фиксирует решения D1–D11 и порядок фаз M0–M6. Примеры кода — эскизы для адаптации. Первый тикет каждой фазы — замер базлайна метрик фазы.*
