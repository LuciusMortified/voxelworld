---
description: C++ code style rules for voxelworld engine (not covered by .clang-format/.clang-tidy)
globs: ["**/*.h", "**/*.inl.h", "**/*.cpp"]
---

# C++ Style Rules

## Trailing Return Type
ALWAYS use trailing return type for all methods:
```cpp
[[nodiscard]] auto get_value() const -> int;
auto operator=(const foo&) -> foo& = delete;
```

## Header Guards
Use both pragma once AND ifndef guard:
```cpp
#pragma once
#ifndef VW_PATH_TO_FILE_H
#define VW_PATH_TO_FILE_H
// ...
#endif  // VW_PATH_TO_FILE_H
```

## Naming
- Types, functions, methods, members, constants: `snake_case`
- Template parameters: `PascalCase` (`T`, `Cs`)
- Namespaces: `vw::` (core), `vw::gfx::` (graphics), `vw::sculptor::` (editor)

## Project Types
Use types from `vw/core/types.h` instead of std/built-in:
`uint8..uint64`, `int8..int64`, `float32`, `float64`

## Include Order
1. Standard library
2. Third-party
3. Project headers (`vw/...`)
4. Local headers

## Comments Policy
- ONLY add comments for top-level classes/structs (brief purpose description)
- NEVER add comments in implementations (`.inl.h`, `.cpp`)
- NEVER add redundant comments like `// Calculate index` before obvious code
- NEVER add section separators like `// ========== Section ==========`
- Exception: truly complex logic where intent is not obvious

## Initialization
Prefer constructors over setup/init methods. Dependencies should be passed via constructor parameters.

## Template Type Aliases
Template classes that depend on other template classes MUST define type aliases:
```cpp
template <typename WC>
class my_system {
    using world_type = world<WC>;
    using registry_type = entity_registry<WC>;
    // ...
};
```

## Error Handling
No exceptions in hot paths — use return values for errors.

## Header-Only Engine Pattern
- `.h` files: interface declarations
- `.inl.h` files: inline implementations
- Engine is an INTERFACE library (no compiled .cpp files)