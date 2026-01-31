# CLAUDE.md - Voxel World AI Assistant Guide

This document provides essential information for AI assistants working on the Voxel World codebase.

## Project Overview

Voxel World is a C++/Vulkan-based voxel sandbox game engine featuring:
- Modern C++23 codebase with header-only engine design
- Entity Component System (ECS) architecture
- Vulkan rendering with cascade shadow mapping
- Sculptor application for voxel model editing
- VOX file format for serialization

**Current version**: 0.0.1 (early development)

## Quick Reference

```bash
# Build commands
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Compile shaders (from shaders/ directory)
./compile.sh    # Linux/macOS
compile.bat     # Windows

# Run Sculptor application
./build/apps/sculptor/sculptor
```

## Directory Structure

```
voxelworld/
├── engine/                    # Header-only engine library
│   └── include/vw/
│       ├── core/              # Fundamental types (types.h, vec3.h, mat4.h, color.h)
│       ├── gfx/
│       │   ├── camera/        # Camera systems (camera.h, fps_camera_controller.h)
│       │   ├── debug/         # Debug visualization (debug_window.h, debug_primitive.h)
│       │   ├── engine/        # Engine coordination (engine.h, app.h)
│       │   ├── model/         # Voxel models (model.h, model_registry.h)
│       │   ├── render/        # Vulkan rendering (renderer.h, vulkan_context.h)
│       │   ├── resource/      # GPU resources (buffer.h, mesh.h, shader.h)
│       │   ├── spatial/       # Spatial math (ray.h, aabb.h, frustum.h)
│       │   └── world/         # ECS (entity.h, registry.h, components/, systems/)
│       └── log/               # Logging (logger.h)
├── apps/
│   ├── sculptor/              # Main voxel editor application
│   │   └── src/
│   │       ├── app/           # Application state and main class
│   │       ├── operations/    # Undo/redo operations
│   │       ├── tools/         # Sculpting tools (add, remove, paint)
│   │       └── ui/            # ImGui panels and modals
│   ├── test_window/           # Window/input testing
│   ├── test_simple_model/     # Model rendering test
│   └── test_math_matrix/      # Math validation test
├── shaders/                   # GLSL shaders (voxel, shadow, debug)
├── docs/                      # Documentation (ENGINE.md, TASKS.md, PRD.md)
└── vcpkg.json                 # Package dependencies
```

## Architecture Concepts

### Header-Only Engine Pattern
- Engine is an INTERFACE library (no compiled .cpp files)
- Headers declare interfaces (`.h`), implementations in inline headers (`.inl.h`)
- Apps are compiled executables linking against the header library

### Entity Component System (ECS)
```cpp
// Registry is templated on component types
template <typename... Cs>
class registry { /* ... */ };

// View for iterating entities with specific components
auto view = registry.view<TransformComponent, ModelComponent>();
for (auto [ent, transform, model] : view) { /* ... */ }
```

### Entity Handle Pattern
```cpp
struct entity {
    uint32 index;       // Dense array index
    uint32 generation;  // Version for use-after-free safety
};
```

### Command Pattern for Undo/Redo
```cpp
class base_operation {
    virtual void execute() = 0;
    virtual void undo() = 0;
};
```

## Code Conventions

### C++ Standards
- **Standard**: C++23
- **Compiler flags**: `-Wall -Wextra -Wpedantic`
- **No exceptions** in hot paths; use return values for error handling

### Naming Conventions
- **Namespaces**: `vw::` (core), `vw::gfx::` (graphics), `vw::sculptor::` (editor)
- **Types**: `snake_case` for classes/structs (e.g., `entity`, `model_registry`)
- **Functions/methods**: `snake_case` (e.g., `get_voxel`, `is_valid`)
- **Member variables**: `snake_case`, no prefix (e.g., `index`, `generation`)
- **Constants**: `snake_case` (e.g., `invalid_index`)
- **Template parameters**: `PascalCase` single letters or short (e.g., `Cs`, `T`)

### Header Guards
```cpp
#pragma once

#ifndef VW_PATH_TO_FILE_H
#define VW_PATH_TO_FILE_H
// ... content ...
#endif  // VW_PATH_TO_FILE_H
```

### Return Type Syntax
Use trailing return type for all methods:
```cpp
[[nodiscard]] auto get_value() const -> int;
auto operator=(const foo&) -> foo& = delete;
```

### Formatting (.clang-format)
- **Base style**: Google
- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 100 characters
- **Braces**: Attached (K&R style)
- **Pointers**: Left-aligned (`int* ptr`)
- **Constructor initializers**: Break before comma, one per line

### Include Order
1. Standard library headers
2. Third-party headers
3. Project headers (vw/...)
4. Local headers

Includes are sorted case-insensitively with regrouping enabled.

## Type Aliases

Use project-specific types from `vw/core/types.h`:
```cpp
vw::uint8, vw::uint16, vw::uint32, vw::uint64
vw::int8, vw::int16, vw::int32, vw::int64
vw::float32, vw::float64
```

## Dependencies

Managed via vcpkg:
- **Vulkan SDK** (system requirement)
- **glfw3** - Window management
- **imgui** (with glfw-binding, vulkan-binding) - UI
- **spdlog** (>=1.15.3) - Logging

## Build System

### CMake Structure
- Root `CMakeLists.txt` - minimal, delegates to subdirectories
- `engine/CMakeLists.txt` - INTERFACE library definition
- `apps/CMakeLists.txt` - aggregator for applications
- Individual app `CMakeLists.txt` - per-application configuration

### Key CMake Settings
```cmake
cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)
```

### Shader Compilation
Shaders are GLSL compiled to SPIR-V via `glslc`:
- Source: `shaders/*.vert`, `shaders/*.frag`
- Output: `shaders/*.spv`
- Compiled automatically during Sculptor build

## Testing

No formal test framework. Testing is done via dedicated test applications:
- `test_window` - Window creation and input polling
- `test_simple_model` - Model rendering validation
- `test_math_matrix` - Math library verification

Run test apps manually to verify functionality.

## Common Development Tasks

### Adding a New Component
1. Create header in `engine/include/vw/gfx/world/components/`
2. Follow naming: `*_component.h` and `*_component.inl.h`
3. Add to `world_components.h` tuple if needed

### Adding a New System
1. Create header in `engine/include/vw/gfx/world/systems/`
2. Follow naming: `*_system.h` and `*_system.inl.h`
3. Implement `update()` method operating on registry views

### Adding a Sculptor Tool
1. Create class inheriting from `base_tool` in `apps/sculptor/src/tools/`
2. Implement required virtual methods (render, event handlers)
3. Register in tool panel UI

### Adding a Sculptor Operation (Undo/Redo)
1. Create class inheriting from `base_operation` in `apps/sculptor/src/operations/`
2. Implement `execute()` and `undo()` methods
3. Use via `operation_manager::execute()`

## Data Structures

### Voxel
- Single color value stored as `uint32` (RGBA packed)
- Empty voxel represented by color value 0

### Model
- 3D array of voxels (width x height x depth)
- Linear storage: `x + y*width + z*width*height`

### Vertex (for GPU)
```cpp
struct vertex {
    vec3f position;  // 12 bytes
    vec3f normal;    // 12 bytes
    uint32 color;    // 4 bytes (RGBA packed)
};  // Total: 28 bytes
```

## Documentation Language

Primary documentation is in Russian. Code comments may be in Russian or English.

## Important Files Reference

| Purpose | File |
|---------|------|
| Type definitions | `engine/include/vw/core/types.h` |
| Entity definition | `engine/include/vw/gfx/world/entity.h` |
| ECS Registry | `engine/include/vw/gfx/world/registry.h` |
| Vulkan context | `engine/include/vw/gfx/render/vulkan_context.h` |
| Main renderer | `engine/include/vw/gfx/render/renderer.h` |
| Voxel model | `engine/include/vw/gfx/model/model.h` |
| Color palette | `engine/include/vw/core/color.h` |
| Sculptor app | `apps/sculptor/src/app/app.h` |
| Base tool | `apps/sculptor/src/tools/base_tool.h` |
| Base operation | `apps/sculptor/src/operations/base_operation.h` |

## Clangd Integration

The project is configured for clangd with:
- C++23 standard
- ClangTidy checks (readability, performance, modernize, bugprone)
- Inlay hints enabled
- Compile commands exported (`compile_commands.json`)

Disabled checks: `readability-magic-numbers`, `readability-identifier-length`, `bugprone-easily-swappable-parameters`

## Git Workflow

Recent development focuses on:
- Sculptor tool improvements (model operations, file format)
- Shadow mapping (cascade shadows)
- Platform support (macOS via MoltenVK)

Commits follow concise format: `area: description` (e.g., `sculptor: expand model operation`)
