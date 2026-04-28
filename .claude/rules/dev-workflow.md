---
description: Recipes for adding new entities to the voxelworld project
globs: ["engine/**", "apps/**"]
---

# Development Workflow Recipes

## New ECS Component
1. Create `engine/include/vw/ecs/components/<name>_component.h` + `.inl.h`
2. Namespace: `vw::ecs`
3. Add to `world_components.h` tuple if needed

## New ECS System
1. Create `engine/include/vw/ecs/systems/<name>_system.h` + `.inl.h`
2. Namespace: `vw::ecs`
3. Implement `update()` method working with registry view

## New Sculptor Tool
1. Create class inheriting `base_tool` in `apps/sculptor/src/tools/`
2. Implement virtual methods (render, event handlers)
3. Register in toolbar UI panel

## New Sculptor Operation (Undo/Redo)
1. Create class inheriting `base_operation` in `apps/sculptor/src/operations/`
2. Implement `execute()` and `undo()`
3. Use via `operation_manager::execute()`